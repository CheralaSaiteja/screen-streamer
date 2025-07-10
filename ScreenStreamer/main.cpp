// opencv
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

// x11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>

// std
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define BUFFER_SIZE 384 * 1024

Window findWindowByName(Display *display, Window root, const char *name) {
  Window returnedRoot, returnedParent;
  Window *children;
  unsigned int nchildren;

  if (!XQueryTree(display, root, &returnedRoot, &returnedParent, &children,
                  &nchildren))
    return 0;

  Window result = 0;

  for (unsigned int i = 0; i < nchildren; ++i) {
    // Try _NET_WM_NAME first (UTF-8)
    Atom prop = XInternAtom(display, "_NET_WM_NAME", False);
    Atom type;
    int format;
    unsigned long nitems, bytesAfter;
    unsigned char *propValue = nullptr;

    if (Success == XGetWindowProperty(display, children[i], prop, 0, (~0L),
                                      False, AnyPropertyType, &type, &format,
                                      &nitems, &bytesAfter, &propValue)) {
      if (propValue) {
        if (strcmp((char *)propValue, name) == 0) {
          result = children[i];
          XFree(propValue);
          break;
        }
        XFree(propValue);
      }
    }

    // Fallback to WM_NAME (legacy)
    char *windowName = nullptr;
    if (XFetchName(display, children[i], &windowName)) {
      if (windowName && strcmp(windowName, name) == 0) {
        result = children[i];
        XFree(windowName);
        break;
      }
      if (windowName)
        XFree(windowName);
    }

    // Recursively search child windows
    result = findWindowByName(display, children[i], name);
    if (result)
      break;
  }

  if (children)
    XFree(children);

  return result;
}

struct REQPACKET {
  char key[8] = {'0', '0', '0', '0'};
  uint16_t resolution[2] = {1280, 720};
  uint8_t quality = 75;
  uint16_t chunk_size = 48;
  void print() {
    printf("packet: key: %s\t, resolution: %d - %d\t, quality: %d\n", key,
           resolution[0], resolution[1], quality);
  }
};

int main() {
  Display *display = XOpenDisplay(nullptr);
  if (!display) {
    fprintf(stderr, "Cannot open X Display\n");
    return 1;
  }

  Window root = DefaultRootWindow(display);

  Window targetWindow =
      findWindowByName(display, root, "X11-Window-Application");

  if (!targetWindow) {
    fprintf(stderr, "Unable to find the window\n");
    return 0;
  }

  fprintf(stdout, "Found window\n");

  // Get correct depth of the root window
  XWindowAttributes gwa;
  XGetWindowAttributes(display, targetWindow, &gwa);

  int width = gwa.width;
  int height = gwa.height;

  fprintf(stdout, "width: %d\theight: %d\n", width, height);

  // Create UDP socket
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    printf("Error: Could not create socket");
    return -1;
  }

  // Setup UDP address
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(9400);

  // Bind the socket
  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    printf("Error: Could not bind socket");
    close(sockfd);
    return -1;
  }

  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  char buffer[BUFFER_SIZE];

  XImage *image;
  cv::Mat mat;

  std::vector<uchar> frame_buffer;
  frame_buffer.reserve(BUFFER_SIZE);

  int frame_size = 0;
  int current_chunk_size = 0;
  uint8_t num_chunks = 0;

  REQPACKET packet;

  while (true) {
    image = XGetImage(display, targetWindow, 0, 0, width, height, AllPlanes,
                      ZPixmap);
    if (!image) {
      fprintf(stderr, "Failed to get display buffer\n");
      break;
    }

    fprintf(stdout, "width: %d\t height: %d\tBPP: %d\n", image->width,
            image->height, image->bits_per_pixel);

    mat = cv::Mat(height, width, CV_8UC4, image->data);

    cv::cvtColor(mat, mat, cv::COLOR_RGBA2RGB);

    // cv::resize(mat, mat, cv::Size(1280, 720));

    // cv::imshow("Screen recording", mat);

    // if (cv::waitKey(1) == 27) // ESC key
    //   break;

    cv::imencode(".jpg", mat, frame_buffer,
                 {cv::IMWRITE_JPEG_QUALITY, packet.quality});

    int recv_len = recvfrom(sockfd, &packet, sizeof(packet), 0,
                            (struct sockaddr *)&client_addr, &client_addr_len);

    printf("quality: %zu\t", frame_buffer.size());
    packet.print();

    if (recv_len < 0) {
      printf("request error\n");
      continue;
    }

    buffer[recv_len] = '\0';

    // Check if the command is "data"
    if (strcmp(packet.key, "data") == 0) {
      // Send frame size
      frame_size = frame_buffer.size();
      current_chunk_size = 0;
      num_chunks = std::ceil((double)frame_size / (packet.chunk_size * 1024));

      printf("%d 32KB chunks\n", num_chunks);

      sendto(sockfd, &num_chunks, sizeof(num_chunks), 0,
             (struct sockaddr *)&client_addr, client_addr_len);

      // Send each chunk
      for (int i = 0; i < num_chunks; i++) {
        // Calculate the start and end positions for the chunk
        size_t start_pos = i * (packet.chunk_size * 1024);
        size_t end_pos = std::min(start_pos + (packet.chunk_size * 1024),
                                  (size_t)frame_size);
        current_chunk_size = end_pos - start_pos; // Size of the current chunk

        // Send the chunk size first
        sendto(sockfd, &current_chunk_size, sizeof(current_chunk_size), 0,
               (struct sockaddr *)&client_addr, client_addr_len);

        // Send the chunk data

        sendto(sockfd, frame_buffer.data() + start_pos, current_chunk_size, 0,
               (struct sockaddr *)&client_addr, client_addr_len);

        printf("chunk %d: Sent chunk size %d bytes, (from byte %zu to %zu)\n",
               i + 1, current_chunk_size, start_pos, end_pos);
      }
    }

    XDestroyImage(image);

    usleep(16000);
  }

  close(sockfd);
  XCloseDisplay(display);
  return 0;
}