import time
import sys
import socket
import cv2
import struct
import numpy as np

class CameraStreamerClient:
    def __init__(self, address, device_id_text):
        self.device_id_text = device_id_text # for degugging
        self.address = address

        self.key = 'data'
        self.resolution = [800, 600]
        self.quality = 50

        while True:
            try:
                self.sock_fd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                break
            except Exception as e:
                time.sleep(5)
                print(f"[{self.device_id_text}] [Init] Exception: {e}")


    def setResolution(self, width, height):
        self.resolution = [width, height]

    def setQuality(self, quality):
        self.quality = quality

    def getPacketPacket(self):
        return struct.pack('8s2HB', self.key.encode('utf-8').ljust(8, b'\x00'), self.resolution[0], self.resolution[1], self.quality)
        
    def update(self):
        try:
            # print(f"sending data to {self.address}")
            self.sock_fd.sendto(self.getPacketPacket(), self.address)
            
            self.sock_fd.settimeout(0.5)

            size, _ = self.sock_fd.recvfrom(4)
            size = struct.unpack("!I", size)[0]
            compressed_frame, _ = self.sock_fd.recvfrom(size)
            frame = cv2.imdecode(np.frombuffer(compressed_frame, dtype=np.uint8), cv2.IMREAD_COLOR)
            
            self.sock_fd.settimeout(None)

            cv2.imshow("stream", frame)
            cv2.waitKey(1)
        except Exception as e:
            print(f"[{self.device_id_text}] [Main Loop] Exception: {e}")


    def close(self):
        cv2.destroyAllWindows()
        self.sock_fd.close()

if __name__ == "__main__":
    obj = CameraStreamerClient(("192.168.0.152", 9000), "TEST")
    while True:
        obj.update()
    obj.close()