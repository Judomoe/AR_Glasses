# ui_overlay.py
import cv2
from database_manager import get_description

def draw_overlay(frame, results):
    for res in results:
        bbox = res.get("bbox")
        if not bbox:
            continue
        x1, y1, x2, y2 = bbox
        name = res.get("name") or "Unknown"
        color = (0, 255, 0) if res.get("name") else (0, 0, 255)

        cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
        label = f"{name}"
        cv2.putText(frame, label, (x1, max(10, y1 - 10)),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, color, 2)

        if res.get("name"):
            desc = get_description(res["name"])
            cv2.putText(frame, desc, (x1, y2 + 20),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 1)
    return frame
