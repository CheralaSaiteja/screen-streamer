#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define PORT 9000
#define BUFFER_SIZE 384 * 1024

#define DEVICE_NAME "virDriveCam: virDriveCam"

#define DEBUG

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

void getCameraIndexFromName(const char *camera_name, char *camera_device) {
  char device[20];
  struct v4l2_capability cap;
  int fd;
  while (true) {
    for (int i = 0; i < 10; ++i) { // Check first 10 video devices
      snprintf(device, sizeof(device), "/dev/video%d", i);

      fd = open(device, O_RDWR);
      if (fd == -1) {
        continue; // Skip if unable to open
      }

      if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
        if (strcmp(camera_name, (const char *)cap.card) == 0) {
          strcpy(camera_device, device);
          close(fd);
          return;
        }

        close(fd);
      }
    }
    // Sleep for 2 seconds
    sleep(2);
    printf("Device %s not found\n", camera_name);
  }
}

int main(int argc, char **argv) {
  if (argc != 5) {
    printf("provide camera device name, port and resolution\n");
    return -1;
  }
  char camera_device[16];
  getCameraIndexFromName(argv[1], camera_device);

  // Open the USB camera
  cv::VideoCapture cap(camera_device);
  if (!cap.isOpened()) {
    printf("Error: Could not open camera");
    return -1;
  }

  cap.set(cv::CAP_PROP_FRAME_WIDTH, atoi(argv[3]));
  cap.set(cv::CAP_PROP_FRAME_HEIGHT, atoi(argv[4]));
  // cap.set(cv::CAP_PROP_FPS, 60);

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
  server_addr.sin_port = htons(atoi(argv[2]));

  // Bind the socket
  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    printf("Error: Could not bind socket");
    close(sockfd);
    return -1;
  }

  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  char buffer[BUFFER_SIZE];

  cv::Mat frame;

  std::vector<uchar> frame_buffer;
  frame_buffer.reserve(BUFFER_SIZE);

  int frame_size = 0;
  int current_chunk_size = 0;
  uint8_t num_chunks = 0;

  std::chrono::high_resolution_clock::time_point start, end;
  std::chrono::duration<double, std::milli> elapsed;

  REQPACKET packet;

  while (true) {
    cap.read(frame);

    if (frame.empty()) {
      cap.release();
      getCameraIndexFromName(argv[1], camera_device);
      cap.open(camera_device);
      continue;
    }

    cv::resize(frame, frame, {packet.resolution[0], packet.resolution[1]});

    cv::imencode(".jpg", frame, frame_buffer,
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
  }

  // Clean up
  cap.release();
  cv::destroyAllWindows();
  close(sockfd);

  return 0;
}
