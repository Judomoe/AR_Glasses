# database_manager.py
import sqlite3
import os
from config import SQLITE_DB

def init_db():
    """Create database if it doesn’t exist."""
    if not os.path.exists(SQLITE_DB):
        conn = sqlite3.connect(SQLITE_DB)
        cur = conn.cursor()
        cur.execute("""
            CREATE TABLE people (
                name TEXT PRIMARY KEY,
                description TEXT
            )
        """)
        conn.commit()
        conn.close()
        print("📘 Created new database: people.db")

def get_description(name):
    """Fetch description by name."""
    try:
        conn = sqlite3.connect(SQLITE_DB)
        cur = conn.cursor()
        cur.execute("SELECT description FROM people WHERE name=?", (name,))
        row = cur.fetchone()
        conn.close()
        return row[0] if row else "No description available."
    except Exception as e:
        print("DB error:", e)
        return "Error fetching description."
