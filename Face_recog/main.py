import cv2
import threading
import time
import sys
import numpy as np
from config import FRAME_WIDTH, FRAME_HEIGHT, MODEL_NAME
from recognition import frame_q, result_q, processing_worker
from build_embeddings import build_embeddings
from ui_overlay import draw_overlay
from database_manager import init_db

# Initialize DB
init_db()

# Load embeddings (convert to NumPy arrays)
embeddings = build_embeddings()
if embeddings:
    embeddings = [
        {"name": e["name"], "embedding": np.array(e["embedding"], dtype=np.float32)}
        for e in embeddings
    ]
else:
    print("[WARN] No embeddings — add images to 'database/' and restart.")
    sys.exit()

# Start background recognition thread
worker = threading.Thread(target=processing_worker, args=(MODEL_NAME, embeddings), daemon=True)
worker.start()

# Start camera
cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, FRAME_WIDTH)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT)

if not cap.isOpened():
    print("❌ Cannot open webcam")
    sys.exit()

print("🎥 Camera started. Press 'q' to quit.")
fps_smooth = 0.0
frame_id = 0
last_time = time.time()

try:
    while True:
        ret, frame = cap.read()
        if not ret:
            break
        frame_id += 1

        # Send frame to processing thread
        if not frame_q.full():
            frame_q.put_nowait((frame_id, frame.copy()))

        # Get latest results
        latest = None
        while not result_q.empty():
            latest = result_q.get_nowait()
        results = latest[1] if latest else []

        # Draw overlay (names, boxes, etc.)
        frame = draw_overlay(frame, results)

        # FPS calculation
        now = time.time()
        fps = 1.0 / (now - last_time) if now != last_time else 0
        last_time = now
        fps_smooth = 0.9 * fps_smooth + 0.1 * fps if fps_smooth else fps

        cv2.putText(frame, f"FPS: {fps_smooth:.1f}", (10, 20),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 0), 2)
        cv2.imshow("Face Recognition", frame)

        # Quit on 'q'
        if cv2.waitKey(1) & 0xFF == ord('q'):
            print("🛑 Quitting...")
            break

finally:
    cap.release()
    frame_q.put(None)
    worker.join(timeout=2)
    cv2.destroyAllWindows()
    print("✅ Clean exit")
