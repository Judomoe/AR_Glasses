# recognition.py
from deepface import DeepFace
import numpy as np
import os
import cv2
import queue
import threading
from config import DB_PATH, MODEL_NAME

frame_q = queue.Queue(maxsize=2)
result_q = queue.Queue(maxsize=2)

def cosine_distance(a, b):
    """Cosine similarity distance."""
    return np.dot(a, b) / (np.linalg.norm(a) * np.linalg.norm(b))

def recognize_face(frame, embeddings, threshold=0.65):
    """Recognize faces in a frame."""
    try:
        detections = DeepFace.extract_faces(frame, detector_backend="retinaface", enforce_detection=False)
    except Exception as e:
        print("Detection error:", e)
        return []

    results = []
    for det in detections:
        area = det.get("facial_area", {})
        x = int(area.get("x", 0))
        y = int(area.get("y", 0))
        w = int(area.get("w", 0))
        h = int(area.get("h", 0))
        if w <= 0 or h <= 0:
            continue

        face_img = det.get("face")
        if face_img is None:
            continue

        try:
            rep = DeepFace.represent(img_path=face_img, model_name=MODEL_NAME, enforce_detection=False)[0]["embedding"]
        except Exception as e:
            print("Embedding error:", e)
            continue

        best_match = None
        best_score = -1
        for person in embeddings:
            score = cosine_distance(rep, person["embedding"])
            if score > best_score:
                best_match = person["name"]
                best_score = score

        results.append({
            "bbox": (x, y, x + w, y + h),
            "name": best_match if best_score > threshold else None,
            "distance": best_score
        })

    return results


def processing_worker(model_name, embeddings):
    """Background thread to process frames."""
    while True:
        item = frame_q.get()
        if item is None:
            break
        frame_id, frame = item
        results = recognize_face(frame, embeddings)
        result_q.put((frame_id, results))
