import sys
import cv2
import os
from ultralytics import YOLO
import time

print("[DEBUG] 摄像头脚本开始", flush=True)

if len(sys.argv) < 3:
    print("[ERROR] 使用方法: python process_camera.py <model_path> <output_dir>", flush=True)
    sys.exit(1)

model_path = sys.argv[1]
output_dir = sys.argv[2]

# 创建输出目录
try:
    os.makedirs(output_dir, exist_ok=True)
except Exception as e:
    print(f"[ERROR] 创建输出目录失败: {e}", flush=True)
    sys.exit(1)

# 打开摄像头
print("[DEBUG] 打开摄像头...", flush=True)
try:
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    cap.set(cv2.CAP_PROP_FPS, 30)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

    if not cap.isOpened():
        print("[ERROR] 无法打开摄像头", flush=True)
        sys.exit(1)

    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    print(f"[SUCCESS] 摄像头已打开: {width}x{height}", flush=True)

except Exception as e:
    print(f"[ERROR] 打开摄像头失败: {e}", flush=True)
    sys.exit(1)

# 加载模型
print("[DEBUG] 加载YOLOv8模型...", flush=True)
try:
    model = YOLO(model_path)
    print("[SUCCESS] 模型加载成功", flush=True)
except Exception as e:
    print(f"[ERROR] 模型加载失败: {e}", flush=True)
    sys.exit(1)

print("[DEBUG] 开始处理摄像头帧...", flush=True)

frame_count = 0
detect_mode = False
detect_interval = 2
mode_check_interval = 10
write_interval = 5  # Only write latest.jpg every N frames to reduce disk I/O
# ========== 关键：置信度阈值设置 ==========
CONFIDENCE_THRESHOLD = 0.8  # 只显示置信度 >= 80% 的检测结果

try:
    while True:
        ret, frame = cap.read()
        if not ret:
            continue

        # 检查模式文件
        if frame_count % mode_check_interval == 0:
            mode_file = os.path.join(output_dir, "camera_mode.txt")
            try:
                if os.path.exists(mode_file):
                    with open(mode_file, 'r') as f:
                        mode_str = f.read().strip()
                        new_mode = mode_str == "detect"
                        if new_mode != detect_mode:
                            detect_mode = new_mode
                            print(f"[DEBUG] 切换模式: {'检测' if detect_mode else '显示'}", flush=True)
            except:
                pass

        # Save original frame only every write_interval frames to reduce disk I/O
        if frame_count % write_interval == 0:
            original_path = os.path.join(output_dir, "latest.jpg")
            cv2.imwrite(original_path, frame)

        # 检测模式处理
        if detect_mode and frame_count % detect_interval == 0:
            try:
                # ========== 关键：使用更严格的置信度 ==========
                results = model(frame, conf=CONFIDENCE_THRESHOLD, verbose=False)

                detected_frame = frame.copy()

                # ========== 额外过滤：再次确保只显示高置信度结果 ==========
                for r in results:
                    # 获取原始检测框
                    if len(r.boxes) > 0:
                        # 过滤置信度 >= 阈值 的框
                        high_conf_boxes = r.boxes[r.boxes.conf >= CONFIDENCE_THRESHOLD]

                        if len(high_conf_boxes) > 0:
                            # 临时替换boxes进行绘制
                            r.boxes = high_conf_boxes
                            detected_frame = r.plot()
                        else:
                            # 如果没有高置信度的框，就不绘制
                            detected_frame = frame.copy()
                    else:
                        detected_frame = frame.copy()

                # 保存检测结果
                detected_path = os.path.join(output_dir, "latest_detected.jpg")
                cv2.imwrite(detected_path, detected_frame)

            except Exception as e:
                print(f"[ERROR] 检测失败: {e}", flush=True)
        else:
            # 显示模式：删除检测文件
            detected_path = os.path.join(output_dir, "latest_detected.jpg")
            if os.path.exists(detected_path):
                try:
                    os.remove(detected_path)
                except:
                    pass

        frame_count += 1

except KeyboardInterrupt:
    print("[DEBUG] 用户中断", flush=True)
except Exception as e:
    print(f"[ERROR] {e}", flush=True)
finally:
    cap.release()
    print("[DEBUG] 摄像头已关闭", flush=True)
