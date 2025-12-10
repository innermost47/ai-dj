import argparse
import os
import sqlite3
import secrets
import string
from pathlib import Path
from fastapi import FastAPI, Request, Depends
import uvicorn
from dotenv import load_dotenv
from core.dj_system import DJSystem
from core.secure_storage import SecureStorage
from core.paths import get_config_db_path

load_dotenv()


def generate_first_api_key():
    alphabet = string.ascii_letters + string.digits
    api_key = "".join(secrets.choice(alphabet) for _ in range(32))

    config_dir = get_config_db_path()
    db_path = config_dir / "config.db"
    secure_storage = SecureStorage(db_path)

    encrypted_key = secure_storage.encrypt(api_key)

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    cursor.execute(
        """INSERT INTO api_keys 
        (key_value_encrypted, name, is_limited, total_credits, credits_used) 
        VALUES (?, ?, ?, ?, ?)""",
        (encrypted_key, "First API Key", 0, 0, 0),
    )

    conn.commit()
    conn.close()

    return api_key


def ensure_database_exists():

    config_dir = get_config_db_path()
    config_dir.mkdir(parents=True, exist_ok=True)
    db_path = config_dir / "config.db"

    if not db_path.exists():
        print(f"⚠️  Database not found, creating at: {db_path}")

        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()

        cursor.execute(
            """
            CREATE TABLE IF NOT EXISTS api_keys (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                key_value_encrypted TEXT UNIQUE NOT NULL,
                name TEXT,
                is_limited INTEGER DEFAULT 1,
                is_expired INTEGER DEFAULT 0,
                total_credits INTEGER DEFAULT 50,
                credits_used INTEGER DEFAULT 0,
                date_of_expiration TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                last_used TIMESTAMP
            )
        """
        )

        cursor.execute(
            """
            CREATE TABLE IF NOT EXISTS config (
                key TEXT PRIMARY KEY,
                value TEXT,
                is_encrypted INTEGER DEFAULT 0
            )
        """
        )

        conn.commit()
        conn.close()

        print("✅ Database created successfully")
        return True

    return False


def is_database_empty():
    config_dir = get_config_db_path()
    db_path = config_dir / "config.db"

    if not db_path.exists():
        return True

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    cursor.execute("SELECT COUNT(*) FROM api_keys")
    count = cursor.fetchone()[0]
    conn.close()

    return count == 0


def get_dj_system(request: Request):
    if hasattr(request.app, "dj_system"):
        return request.app.dj_system
    if hasattr(request.app, "state") and hasattr(request.app.state, "dj_system"):
        return request.app.state.dj_system
    raise RuntimeError("No DJSystem instance found in FastAPI application!")


def create_api_app():
    app = FastAPI(
        title="OBSIDIAN-Neural API",
        description="API for the VST OBSIDIAN-Neural plugin",
        version="1.0.0",
    )

    from fastapi.exceptions import RequestValidationError
    from fastapi.responses import JSONResponse

    @app.exception_handler(RequestValidationError)
    async def validation_exception_handler(
        request: Request, exc: RequestValidationError
    ):
        print(f"❌ Validation Error on {request.method} {request.url}")
        print(f"❌ Error details: {exc.errors()}")

        try:
            body = await request.body()
            print(f"❌ Raw request body: {body.decode('utf-8')}")
        except:
            print("❌ Could not read request body")

        return JSONResponse(
            status_code=422,
            content={
                "error": {
                    "code": "VALIDATION_ERROR",
                    "message": f"Request validation failed: {exc.errors()}",
                }
            },
        )

    from server.api.routes import router

    app.include_router(router, prefix="/api/v1", dependencies=[Depends(get_dj_system)])

    return app


def load_encrypted_api_keys():
    try:

        db_path = Path.home() / ".obsidian_neural" / "config.db"
        if not db_path.exists():
            return []

        secure_storage = SecureStorage(db_path)

        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()

        cursor.execute("SELECT key_value_encrypted FROM api_keys ORDER BY created_at")
        rows = cursor.fetchall()

        api_keys = []
        for row in rows:
            decrypted_key = secure_storage.decrypt(row[0])
            if decrypted_key:
                api_keys.append(decrypted_key)

        conn.close()
        return api_keys

    except Exception as e:
        print(f"Warning: Could not load encrypted API keys: {e}")
        return []


def main():
    parser = argparse.ArgumentParser(
        description="OBSIDIAN-Neural System with Layer Manager"
    )
    parser.add_argument("--model-path", type=str, help="Path of the LLM model")
    parser.add_argument("--host", default="127.0.0.1", help="Host for API server")
    parser.add_argument("--port", type=int, default=8000, help="Port for API server")
    parser.add_argument("--environment", default="dev", help="Environment (dev/prod)")
    parser.add_argument(
        "--audio-model", default="stabilityai/stable-audio-open-1.0", help="Audio model"
    )
    parser.add_argument(
        "--use-stored-keys",
        action="store_true",
        help="Load API keys from encrypted database",
    )
    parser.add_argument(
        "--is-test",
        action="store_true",
        help="Bypass models generations for faster testing",
    )
    parser.add_argument(
        "--bypass-llm",
        action="store_true",
        help="Bypass LLM for direct stable audio generation",
    )

    args = parser.parse_args()

    model_path = args.model_path or os.environ.get("LLM_MODEL_PATH")
    host = args.host or os.environ.get("HOST", "127.0.0.1")
    port = args.port or int(os.environ.get("PORT", 8000))
    environment = args.environment or os.environ.get("ENVIRONMENT", "dev")
    audio_model = args.audio_model or os.environ.get(
        "AUDIO_MODEL", "stabilityai/stable-audio-open-1.0"
    )

    db_was_created = ensure_database_exists()
    db_is_empty = is_database_empty()
    api_keys = []

    if environment == "prod":
        api_keys = load_encrypted_api_keys()
        if not api_keys and (db_was_created or db_is_empty):
            print("\n" + "=" * 60)
            print("No API keys found - Generating first API key...")
            first_key = generate_first_api_key()
            api_keys = [first_key]
            print("\nYour first API key (SAVE THIS - won't be shown again):")
            print(f"\n{first_key}\n")
            print("=" * 60 + "\n")

        print(f"Production mode: loaded {len(api_keys)} API keys from database")
    elif args.use_stored_keys:
        api_keys = load_encrypted_api_keys()
        if not api_keys and (db_was_created or db_is_empty):
            print("\n" + "=" * 60)
            print("No API keys found - Generating first API key...")
            first_key = generate_first_api_key()
            api_keys = [first_key]
            print("\nYour first API key (SAVE THIS):")
            print(f"\n{first_key}\n")
            print("=" * 60 + "\n")

        print(f"Development mode: loaded {len(api_keys)} API keys from database")
    else:
        print("Development mode: no API keys loaded (dev bypass active)")

    print(f"🎵 Starting OBSIDIAN-Neural Server")
    print(f"   Host: {host}:{port}")
    print(f"   Environment: {environment}")
    print(f"   Model: {model_path}")
    print(f"   Audio Model: {audio_model}")
    print(f"   Use LLM: {not args.bypass_llm}")
    print(
        f"   API Authentication: {len(api_keys)} keys"
        if api_keys
        else "   API Authentication: Development bypass"
    )

    from config.config import init_config_from_args

    config_args = argparse.Namespace(
        api_keys=",".join(api_keys) if api_keys else "",
        environment=environment,
        audio_model=audio_model,
        use_stored_keys=args.use_stored_keys,
        is_test=args.is_test,
        bypass_llm=args.bypass_llm,
    )

    init_config_from_args(config_args)

    app = create_api_app()
    dj_args = argparse.Namespace(
        model_path=model_path,
        host=host,
        port=port,
        environment=environment,
        audio_model=audio_model,
    )

    dj_system = DJSystem.get_instance(dj_args)
    app.state.dj_system = dj_system

    uvicorn.run(app, host=host, port=port)
    print("✅ Server closed.")


if __name__ == "__main__":
    main()
