import sys
import cv2

if len(sys.argv) < 3:
    print("[ERROR] Usage: python get_first_frame.py <video_path> <output_path>")
    sys.exit(1)

video_path = sys.argv[1]
output_path = sys.argv[2]

try:
    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        print("[ERROR] Cannot open video")
        sys.exit(1)

    ret, frame = cap.read()
    if ret:
        cv2.imwrite(output_path, frame)
        print("[SUCCESS] First frame saved")
    else:
        print("[ERROR] Cannot read first frame")
        sys.exit(1)

    cap.release()
except Exception as e:
    print(f"[ERROR] {e}")
    sys.exit(1)
