import os
import json
from deepface import DeepFace
from config import DB_PATH, MODEL_NAME

EMBED_FILE = os.path.join(DB_PATH, "embeddings.json")
MODEL_META_FILE = os.path.join(DB_PATH, "model_used.txt")

def build_embeddings():
    """Load or build embeddings from the database, ensuring model consistency."""
    # 1️⃣ Check if cached embeddings exist
    if os.path.exists(EMBED_FILE) and os.path.exists(MODEL_META_FILE):
        with open(MODEL_META_FILE, "r") as f:
            cached_model = f.read().strip()

        # Model consistency check
        if cached_model == MODEL_NAME:
            with open(EMBED_FILE, "r") as f:
                data = json.load(f)
            print(f"✅ Loaded {len(data)} embeddings from cache (model: {cached_model}).")
            return data
        else:
            print(f"⚠️ Model changed from '{cached_model}' → '{MODEL_NAME}'. Rebuilding embeddings...")

    # 2️⃣ Otherwise, build new embeddings
    print("⚙️ Building embeddings from 'database/'...")
    data = []

    for person_name in os.listdir(DB_PATH):
        person_folder = os.path.join(DB_PATH, person_name)
        if not os.path.isdir(person_folder):
            continue

        images = [f for f in os.listdir(person_folder) if f.lower().endswith((".jpg", ".jpeg", ".png"))]
        if not images:
            continue

        print(f"🧠 Processing {person_name}...")
        for img_name in images:
            img_path = os.path.join(person_folder, img_name)
            try:
                rep = DeepFace.represent(
                    img_path=img_path,
                    model_name=MODEL_NAME,
                    enforce_detection=False
                )[0]["embedding"]
                data.append({
                    "name": person_name,
                    "embedding": rep
                })
            except Exception as e:
                print(f"⚠️ Failed to process {img_path}: {e}")

    # 3️⃣ Save embeddings and model name
    with open(EMBED_FILE, "w") as f:
        json.dump(data, f)
    with open(MODEL_META_FILE, "w") as f:
        f.write(MODEL_NAME)

    print(f"💾 Saved {len(data)} embeddings (model: {MODEL_NAME}).")
    return data
