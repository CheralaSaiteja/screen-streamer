import cv2
import serial
# import socket

ser = serial.Serial("/dev/ttyACM0", 115200, timeout=0.5)

def sendCommand(command):
    try:
        ser.write(command.encode())
    
    except Exception as e:
        print(f"command Exception: {e}")

# cap = cv2.VideoCapture(0)

cap = cv2.VideoCapture("/dev/video2")
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)

tracker = cv2.TrackerCSRT_create()

tracking = False
bbox = None

frame_center_y = 0
frame_center_x = 0

# Mouse callback function to capture click position and create a 100x100 bbox
def mouse_callback(event, x, y, flags, param):
    global bbox, tracking

    if event == cv2.EVENT_LBUTTONDOWN:  # On mouse click
        bbox = (x - 50, y - 50, 100, 100)  # Create a 100x100 bounding box
        tracker.init(frame, bbox)
        tracking = True

# Set up the mouse callback
cv2.namedWindow("Frame")
cv2.setMouseCallback("Frame", mouse_callback)

while True:
    ret, frame = cap.read()
    if not ret or frame is None:
        continue

    pan_dir = 0
    tilt_dir = 0
    center_x, center_y = 0, 0
    frame_height, frame_width = frame.shape[:2]

    if tracking and bbox is not None:
        success, bbox = tracker.update(frame)
        if success:
            p1 = (int(bbox[0]), int(bbox[1]))
            p2 = (int(bbox[0] + bbox[2]), int(bbox[1] + bbox[3]))

            center_x, center_y = int(bbox[0] + bbox[2] / 2) - (frame_width / 2), int(bbox[1] + bbox[3] / 2) - (frame_height / 2)

            pan_dir = int(center_x < 0)
            tilt_dir = int(center_y < 0)

            center_x, center_y = (abs(center_x) / (frame_width / 2)) * 28000, (abs(center_y) / (frame_height / 2)) * 200
            
            print(f"${pan_dir}, {int(center_x)}, {tilt_dir}, {int(center_y)}, {0}\n")
            
            sendCommand(f"${pan_dir}, {int(center_x)}, {tilt_dir}, {int(center_y)}, {90}\n")
            cv2.rectangle(frame, p1, p2, (255, 0, 0), 2, 1)
        else:
            sendCommand(f"${pan_dir}, {int(center_x)}, {tilt_dir}, {int(center_y)}, {90}\n")
    else:
        sendCommand(f"${pan_dir}, {int(center_x)}, {tilt_dir}, {int(center_y)}, {90}\n")

    cv2.imshow('Frame', frame)

    key = cv2.waitKey(1) & 0xFF

    # if key == ord('a'):
    #     h, w = frame.shape[:2]
    #     frame_center_y = h / 2
    #     frame_center_x = w / 2
    #     bbox = (w//2 - 75, h//2 - 75, 150, 150)
    #     tracker.init(frame, bbox)
    #     tracking = True

    if key == ord('b'):
        tracking = False

    if key == ord('a'):
        sendCommand(f"$0, 10000, 0, 0, 90\n")
    elif key == ord('d'):
        sendCommand(f"$1, 10000, 0, 0, 90\n")
    elif key == ord('w'):
        sendCommand(f"$0, 0, 0, 200, 90\n")
    elif key == ord('s'):
        sendCommand(f"$0, 0, 1, 200, 90\n")


    elif key == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()