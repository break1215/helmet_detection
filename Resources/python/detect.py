import sys
import cv2
import os
from ultralytics import YOLO

if len(sys.argv) < 4:
    print("[ERROR] 使用方法: python detect.py <model.pt> <input> <output>")
    sys.exit(1)

model_path = sys.argv[1]
input_path = sys.argv[2]
output_path = sys.argv[3]

try:
    model = YOLO(model_path)
    img = cv2.imread(input_path)
    if img is None:
        print(f"[ERROR] 无法读取图片: {input_path}")
        sys.exit(1)

    # 运行检测（使用较低的置信度来获取所有结果）
    results = model(img, conf=0.3, verbose=False)

    # 只保留置信度 >= 0.5 的检测结果
    detected_img = img.copy()
    for r in results:
        # 过滤置信度
        filtered_boxes = r.boxes[r.boxes.conf >= 0.5]
        r.boxes = filtered_boxes
        detected_img = r.plot()

    success = cv2.imwrite(output_path, detected_img)

    if success:
        print("[SUCCESS] 检测完成")
    else:
        print(f"[ERROR] 保存图片失败")
        sys.exit(1)

except Exception as e:
    print(f"[ERROR] {e}")
    sys.exit(1)
