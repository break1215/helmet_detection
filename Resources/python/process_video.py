import sys
import cv2
import os
from pathlib import Path
from ultralytics import YOLO

print("[DEBUG] 脚本开始")
print(f"[DEBUG] 命令行参数: {sys.argv}")

if len(sys.argv) < 4:
    print("[ERROR] 使用方法: python process_video.py <video_path> <model_path> <output_dir>")
    sys.exit(1)

video_path = sys.argv[1]
model_path = sys.argv[2]
output_dir = sys.argv[3]

print(f"[DEBUG] 视频路径: {video_path}")
print(f"[DEBUG] 模型路径: {model_path}")
print(f"[DEBUG] 输出目录: {output_dir}")

# 检查文件是否存在
if not os.path.exists(video_path):
    print(f"[ERROR] 视频文件不存在: {video_path}")
    sys.exit(1)

if not os.path.exists(model_path):
    print(f"[ERROR] 模型文件不存在: {model_path}")
    sys.exit(1)

# 创建输出目录
try:
    os.makedirs(output_dir, exist_ok=True)
    print(f"[DEBUG] 输出目录已创建: {output_dir}")
except Exception as e:
    print(f"[ERROR] 创建输出目录失败: {e}")
    sys.exit(1)

# 打开视频
print("[DEBUG] 正在打开视频...")
try:
    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        print("[ERROR] 无法打开视频文件")
        sys.exit(1)

    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    fps = cap.get(cv2.CAP_PROP_FPS)
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

    print(f"[DEBUG] 视频信息:")
    print(f"       总帧数: {total_frames}")
    print(f"       FPS: {fps}")
    print(f"       分辨率: {width}x{height}")
except Exception as e:
    print(f"[ERROR] 读取视频信息失败: {e}")
    sys.exit(1)

# 加载模型
print("[DEBUG] 正在加载YOLOv8模型...")
try:
    model = YOLO(model_path)
    print("[DEBUG] 模型加载成功")
except Exception as e:
    print(f"[ERROR] 模型加载失败: {e}")
    sys.exit(1)

# ========== 关键：置信度阈值设置 ==========
CONFIDENCE_THRESHOLD = 0.5  # 只显示置信度 >= 50% 的检测结果

# 处理视频帧
print("[DEBUG] 开始处理视频帧...")
frame_count = 0
skip_frames = max(1, int(fps / 5))  # 每秒提取5帧

try:
    while True:
        ret, frame = cap.read()
        if not ret:
            print(f"[DEBUG] 已读取所有帧，停止")
            break

        # 每隔skip_frames帧处理一次
        if frame_count % skip_frames != 0:
            frame_count += 1
            continue

        # 运行检测
        try:
            # ========== 关键：使用更严格的置信度 ==========
            results = model(frame, conf=CONFIDENCE_THRESHOLD, verbose=False)

            # 绘制检测结果
            detected_frame = frame.copy()
            for r in results:
                # 过滤置信度 >= 阈值 的框
                if len(r.boxes) > 0:
                    high_conf_boxes = r.boxes[r.boxes.conf >= CONFIDENCE_THRESHOLD]
                    if len(high_conf_boxes) > 0:
                        r.boxes = high_conf_boxes
                        detected_frame = r.plot()
                    else:
                        detected_frame = frame.copy()

            # 保存原始帧
            original_frame_num = frame_count // skip_frames
            original_path = os.path.join(output_dir, f"original_{original_frame_num:04d}.jpg")
            cv2.imwrite(original_path, frame)

            # 保存检测后的帧
            detected_path = os.path.join(output_dir, f"frame_{original_frame_num:04d}.jpg")
            cv2.imwrite(detected_path, detected_frame)

            if original_frame_num % 10 == 0:
                print(f"[DEBUG] 已处理 {original_frame_num} 帧")

        except Exception as e:
            print(f"[ERROR] 处理第 {frame_count} 帧时出错: {e}")
            frame_count += 1
            continue

        frame_count += 1

    cap.release()
    print(f"[SUCCESS] 视频处理完成，共处理 {frame_count} 帧")

    # 验证输出文件
    frame_files = [f for f in os.listdir(output_dir) if f.startswith("frame_")]
    print(f"[DEBUG] 输出目录中的文件数: {len(frame_files)}")

except Exception as e:
    print(f"[ERROR] 处理过程中发生错误: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
