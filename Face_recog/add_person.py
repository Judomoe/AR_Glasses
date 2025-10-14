# add_person.py
import os
import json
import shutil

def add_person():
    name = input("Enter person's name: ").strip()
    desc = input("Enter a short description: ").strip()

    folder_path = os.path.join("database", name)
    os.makedirs(folder_path, exist_ok=True)

    # Ask for image files
    print("Place the image files (e.g. 1.jpg, 2.jpg) in the same folder as this script.")
    img_files = input("Enter image filenames (comma separated): ").split(",")

    # Move images to the person’s folder
    for img in img_files:
        img = img.strip()
        if os.path.exists(img):
            shutil.move(img, os.path.join(folder_path, os.path.basename(img)))
        else:
            print(f"⚠️ File not found: {img}")

    # Create info.json
    info = {"name": name, "description": desc}
    with open(os.path.join(folder_path, "info.json"), "w") as f:
        json.dump(info, f, indent=4)

    print(f"✅ Added {name} to database.")

if __name__ == "__main__":
    add_person()
