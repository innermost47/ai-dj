from typing import Dict, Any
import sqlite3
from datetime import datetime
from pathlib import Path
from config.config import ENVIRONMENT


def get_api_key_info(api_key: str) -> Dict[str, Any]:
    try:
        from core.secure_storage import SecureStorage

        db_path = Path.home() / ".obsidian_neural" / "config.db"
        if not db_path.exists():
            return None

        secure_storage = SecureStorage(db_path)
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()

        cursor.execute(
            """
            SELECT id, is_limited, is_expired, total_credits, credits_used, date_of_expiration, created_at
            FROM api_keys 
            ORDER BY created_at
        """
        )
        rows = cursor.fetchall()

        for row in rows:
            cursor.execute(
                "SELECT key_value_encrypted FROM api_keys WHERE id = ?", (row[0],)
            )
            encrypted_key = cursor.fetchone()[0]
            decrypted_key = secure_storage.decrypt(encrypted_key)

            if decrypted_key == api_key:
                conn.close()
                return {
                    "id": row[0],
                    "is_limited": bool(row[1]),
                    "is_expired": bool(row[2]),
                    "total_credits": row[3],
                    "credits_used": row[4],
                    "date_of_expiration": row[5],
                    "created_at": row[6],
                }

        conn.close()
        return None

    except Exception as e:
        print(f"Error getting API key info: {e}")
        return None


def check_api_key_status(api_key: str) -> tuple[bool, str, Dict[str, Any]]:
    if ENVIRONMENT == "dev":
        return True, None, {"unlimited": True}

    key_info = get_api_key_info(api_key)
    if not key_info:
        return False, "INVALID_KEY", {}

    if key_info["is_expired"]:
        return False, "KEY_EXPIRED", key_info

    if key_info["date_of_expiration"]:
        try:
            expiration_date = datetime.fromisoformat(key_info["date_of_expiration"])
            if datetime.now() > expiration_date:
                update_api_key_expired_status(key_info["id"], True)
                return False, "KEY_EXPIRED", key_info
        except:
            pass

    if key_info["is_limited"]:
        if key_info["credits_used"] >= key_info["total_credits"]:
            return False, "CREDITS_EXHAUSTED", key_info

    return True, None, key_info


def increment_api_key_usage(api_key: str):
    if ENVIRONMENT == "dev":
        return

    try:
        key_info = get_api_key_info(api_key)
        if key_info and key_info["is_limited"]:
            db_path = Path.home() / ".obsidian_neural" / "config.db"
            conn = sqlite3.connect(db_path)
            cursor = conn.cursor()

            cursor.execute(
                """
                UPDATE api_keys 
                SET credits_used = credits_used + 1 
                WHERE id = ?
            """,
                (key_info["id"],),
            )

            conn.commit()
            conn.close()

    except Exception as e:
        print(f"Error incrementing API key usage: {e}")


def update_api_key_expired_status(key_id: int, is_expired: bool):
    try:
        db_path = Path.home() / ".obsidian_neural" / "config.db"
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()

        cursor.execute(
            """
            UPDATE api_keys 
            SET is_expired = ? 
            WHERE id = ?
        """,
            (is_expired, key_id),
        )

        conn.commit()
        conn.close()

    except Exception as e:
        print(f"Error updating API key expired status: {e}")
