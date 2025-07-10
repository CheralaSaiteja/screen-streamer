#include <arpa/inet.h>
#include <fcntl.h>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 384 * 1024
#define TIMEOUT_USEC 33300

struct REQPACKET {
  char key[8] = {'d', 'a', 't', 'a', '\0', '\0', '\0', '\0'};
  uint16_t resolution[2] = {800, 600};
  uint8_t quality = 35;
  uint16_t chunk_size = 48;
  void print() {
    printf("packet: key: %s\t, resolution: %d - %d\t, quality: %d\n", key,
           resolution[0], resolution[1], quality);
  }
};

class CameraStreamerClient {
public:
  CameraStreamerClient(const char *_address, int _port) //, int count)
  {
    address = _address;
    port = _port;

    m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_sockfd < 0) {
      fprintf(stderr, "Error: Could not create socket\n");
      exit(-1);
    }

    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = 0;             // Set to 0 seconds
    timeout.tv_usec = TIMEOUT_USEC; // Set to 33 milliseconds
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                   sizeof(timeout)) < 0) {
      fprintf(stderr, "Error: Could not set socket timeout\n");
      close(m_sockfd);
      exit(-1);
    }

    // Setup UDP address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address); // Server IP address
    server_addr.sin_port = htons(port);

    strcpy(packet.key, "data");
  }
  ~CameraStreamerClient() {
    if (m_sockfd >= 0) {
      close(m_sockfd);
      m_sockfd = -1;
    }
    cv::destroyAllWindows();
  }

  // socket handler
  void run() {

    from_len = sizeof(from_addr);
    int frame_size;

    std::vector<uchar> frame_buffer(BUFFER_SIZE);
    cv::Mat frame;

    size_t ret;

    uint8_t num_chunks = 0;
    int current_chunk_size = 0;
    size_t current_chunk_position = 0;

    while (true) {
      // packet.print();
      ret = sendto(m_sockfd, &packet, sizeof(packet), 0,
                   (struct sockaddr *)&server_addr, sizeof(server_addr));

      num_chunks = 0;
      current_chunk_size = 0;
      current_chunk_position = 0;

      // Receive chunk count
      ret = recvfrom(m_sockfd, &num_chunks, sizeof(num_chunks), 0,
                     (struct sockaddr *)&from_addr, &from_len);

      printf("number of chunks: %d\n", num_chunks);

      for (int i = 0; i < num_chunks; i++) {
        // Receive the chunk size first
        ret =
            recvfrom(m_sockfd, &current_chunk_size, sizeof(current_chunk_size),
                     0, (struct sockaddr *)&server_addr, &from_len);
        if (ret < 0) {
          std::cerr << "Failed to receive chunk size for chunk " << i + 1
                    << std::endl;
          continue;
        }

        // Ensure we have enough space in frame_buffer to receive this chunk
        if (current_chunk_position + current_chunk_size > BUFFER_SIZE) {
          std::cerr << "Error: Not enough space in frame_buffer!" << std::endl;
          break; // Or handle error as needed
        }

        // Receive the actual data into frame_buffer, starting from the current
        // position
        ret = recvfrom(m_sockfd, frame_buffer.data() + current_chunk_position,
                       current_chunk_size, 0, (struct sockaddr *)&server_addr,
                       &from_len);
        if (ret < 0) {
          std::cerr << "Failed to receive data for chunk " << i + 1
                    << std::endl;
          continue;
        }

        current_chunk_position += ret;

        printf("Chunk %d: %zd bytes\t", i + 1, ret);
      }

      printf("\n");

      // Decode JPEG image
      frame = cv::imdecode(frame_buffer, cv::IMREAD_COLOR);
      if (frame.empty()) {
        //   std::cerr << "Error: Failed to decode image" << std::endl;
        frame.release();
        continue;
      }

      // Display the frame
      cv::imshow("UDP Client", frame);
      if (cv::waitKey(1) == 'q') {
        break;
      }

      frame.release();
    }
  }

private:
  REQPACKET packet;

  int m_sockfd;
  struct sockaddr_in server_addr, from_addr;
  socklen_t from_len;

  int numOfArgs;
  const char *address;
  uint16_t port;
};

int main(int argc, char **argv) {
  CameraStreamerClient streamer(argv[1], atoi(argv[2]));
  streamer.run();
  return 0;
}