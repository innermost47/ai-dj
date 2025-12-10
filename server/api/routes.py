import time
import re
import os
from os import walk
import random
import librosa
import base64
import tempfile
import hashlib
from fastapi import APIRouter, HTTPException, Depends, Request
from fastapi.security import APIKeyHeader
from fastapi.responses import Response
from .models import GenerateRequest
from core.dj_system import DJSystem
from config.config import API_KEYS, ENVIRONMENT, lock, IS_TEST, BYPASS_LLM
from server.api.api_request_handler import APIRequestHandler
from core.api_keys_manager import check_api_key_status, increment_api_key_usage
from core.image_to_sonic import (
    generate_img_description,
    extract_musicgen_prompt,
)

router = APIRouter()
api_key_header = APIKeyHeader(name="X-API-Key", auto_error=False)


def get_user_id_from_api_key(api_key):
    return hashlib.sha256(api_key.encode()).hexdigest()[:16]


def get_dj_system(request: Request) -> DJSystem:
    if hasattr(request.app, "dj_system"):
        return request.app.dj_system
    if hasattr(request.app, "state") and hasattr(request.app.state, "dj_system"):
        return request.app.state.dj_system
    raise RuntimeError("No DJSystem instance found in FastAPI application!")


def create_error_response(
    error_code: str, message: str, status_code: int = 400
) -> HTTPException:
    return HTTPException(
        status_code=status_code,
        detail={"error": {"code": error_code, "message": message}},
    )


def clean_base64(base64_string: str) -> str:
    cleaned = re.sub(r"\s+", "", base64_string)
    if "," in cleaned:
        cleaned = cleaned.split(",", 1)[1]
    missing_padding = len(cleaned) % 4
    if missing_padding:
        cleaned += "=" * (4 - missing_padding)

    return cleaned


async def verify_api_key(api_key: str = Depends(api_key_header)):
    if ENVIRONMENT == "dev":
        return "dev-bypass"

    if not API_KEYS:
        raise create_error_response("SERVER_ERROR", "No API keys configured", 500)

    if not api_key:
        raise create_error_response("INVALID_KEY", "API Key required", 401)

    is_valid, error_code, key_info = check_api_key_status(api_key)

    if not is_valid:
        if error_code == "INVALID_KEY":
            raise create_error_response("INVALID_KEY", "Invalid API key", 403)
        elif error_code == "KEY_EXPIRED":
            raise create_error_response(
                "KEY_EXPIRED",
                "Your API key has expired. Please contact support to renew your access.",
                403,
            )
        elif error_code == "CREDITS_EXHAUSTED":
            used = key_info.get("credits_used", 0)
            total = key_info.get("total_credits", 50)
            raise create_error_response(
                "CREDITS_EXHAUSTED",
                f"No generation credits remaining. You have used {used}/{total} generations for this API key.",
                403,
            )

    return api_key


@router.post("/verify_key")
async def verify_key(_: str = Depends(verify_api_key)):
    return {"status": "valid", "message": "API Key valid"}


@router.post("/generate")
async def generate_loop(
    request: GenerateRequest,
    api_key: str = Depends(verify_api_key),
    dj_system: DJSystem = Depends(get_dj_system),
):
    processed_path = None
    try:
        request_id = int(time.time())
        temp_image_path = None
        if request.use_image and request.image_base64:
            cleaned_base64 = clean_base64(request.image_base64)
            print(f"✅ Cleaned base64 length: {len(cleaned_base64)} chars")
            print(f"📊 Length % 4 = {len(cleaned_base64) % 4}")

            try:
                image_bytes = base64.b64decode(cleaned_base64, validate=True)
            except Exception as e:
                print(f"⚠️  Standard decode failed: {e}")
                print(f"🔄 Retrying with validate=False...")
                image_bytes = base64.b64decode(cleaned_base64, validate=False)

            print(f"✅ Image decoded: {len(image_bytes)} bytes")

            if not image_bytes.startswith(b"\x89PNG"):
                raise ValueError("Decoded data is not a valid PNG image")

            with tempfile.NamedTemporaryFile(delete=False, suffix=".png") as tmp:
                tmp.write(image_bytes)
                temp_image_path = tmp.name

            print(f"✅ Image saved to: {temp_image_path}")

            print(
                f"🎨 Analyzing image with BPM={request.bpm}, Key={request.key or 'C Major'}"
            )
            if request.keywords and len(request.keywords) > 0:
                print(f"🏷️  User keywords: {', '.join(request.keywords)}")

            sonic_data = generate_img_description(
                img_path=temp_image_path,
                bpm=request.bpm,
                scale=request.key or "C Major",
                keywords=request.keywords,
                temperature=request.image_temperature,
            )

            generated_prompt = extract_musicgen_prompt(
                sonic_data, request.bpm, request.key or "C Major"
            )

            print(f"🎵 Generated sonic prompt: {generated_prompt}")
            print(f"🎭 Mood: {sonic_data['parameters']['sonic_analysis']['mood']}")
            print(
                f"⚡ Energy: {sonic_data['parameters']['sonic_analysis']['energy_level']}/10"
            )

            request.prompt = generated_prompt

        elif request.use_image and not request.image_base64:
            raise create_error_response(
                "INVALID_REQUEST", "use_image=true but no image_base64 provided"
            )
        if not request.prompt or len(request.prompt.strip()) < 3:
            raise create_error_response(
                "INVALID_PROMPT", "Prompt required (or provide image)"
            )
        print(f"===== 🎵 QUERY #{request_id} =====")
        print(
            f"📝 '{request.prompt}' | {request.bpm} BPM | {request.key} | SAMPLE RATE {str(int(request.sample_rate))} | GENERATION DURATION {str(int(request.generation_duration))}"
        )
        if not request.prompt or len(request.prompt.strip()) < 3:
            raise create_error_response(
                "INVALID_PROMPT", "Prompt must be at least 3 characters long"
            )
        user_id = get_user_id_from_api_key(api_key)
        handler = APIRequestHandler(dj_system)
        if not dj_system:
            raise create_error_response(
                "GPU_UNAVAILABLE",
                "Audio generation system is currently unavailable. Please try again later.",
                503,
            )
        if not IS_TEST:
            async with lock:
                if request.use_image:
                    llm_decision = request.prompt
                    print(
                        f"🖼️  Using image-generated prompt directly (bypassing LLM decision)"
                    )
                else:
                    if not BYPASS_LLM:
                        handler.setup_llm_session(request, request_id, user_id)
                        llm_decision = handler.get_llm_decision()
                    else:
                        llm_decision = (
                            f"{request.bpm} BPM {request.prompt} {request.key}"
                        )
                audio, _ = handler.generate_simple(request, llm_decision)
                processed_path = handler.process_audio_pipeline(
                    audio, request, request_id
                )
        else:
            test_files_path = "./testfiles"
            test_files = []
            for _, _, filenames in walk(test_files_path):
                test_files.extend(filenames)
                break
            test_files.remove(".gitkeep")
            processed_path = dj_system.layer_manager._prepare_sample_for_loop(
                original_audio_path="./testfiles/" + random.choice(test_files),
                layer_id=f"simple_loop_{request_id}",
                sample_rate=int(request.sample_rate),
            )
            time.sleep(3)
        if not processed_path or not os.path.exists(processed_path):
            raise create_error_response(
                "SERVER_ERROR", "Audio generation completed but file not found", 500
            )
        audio_data, sr = librosa.load(processed_path, sr=None)
        duration = len(audio_data) / sr
        try:
            tempo, _ = librosa.beat.beat_track(y=audio_data, sr=sr)
            detected_bpm = float(tempo)
            print(
                f"🎯 BPM detected by librosa: {detected_bpm:.2f} BPM (expected: {request.bpm} BPM)"
            )
        except Exception as e:
            print(f"⚠️ BPM detection failure: {e}")
            detected_bpm = None

        if duration < 0.1:
            raise create_error_response(
                "SERVER_ERROR", "Generated audio is too short or empty", 500
            )
        with open(processed_path, "rb") as f:
            wav_data = f.read()
        increment_api_key_usage(api_key)
        _, _, key_info = check_api_key_status(api_key)
        remaining_credits = "unlimited"
        if key_info.get("is_limited"):
            remaining_credits = str(
                key_info.get("total_credits", 0) - key_info.get("credits_used", 0)
            )
        print(f"✅ SUCCESS: {duration:.1f}")
        headers = {
            "X-Duration": str(duration),
            "X-BPM": str(request.bpm),
            "X-Detected-BPM": str(detected_bpm) if detected_bpm else "",
            "X-Key": str(request.key or ""),
            "X-Stems-Used": "",
            "X-Credits-Remaining": remaining_credits,
        }
        if key_info.get("is_limited") and key_info.get("date_of_expiration"):
            headers["X-Key-Expires"] = key_info["date_of_expiration"]
        return Response(
            content=wav_data,
            media_type="audio/wav",
            headers=headers,
        )

    except HTTPException:
        raise
    except Exception as e:
        print(f"❌ UNEXPECTED ERROR #{request_id}: {str(e)}")
        raise create_error_response(
            "SERVER_ERROR",
            "An unexpected error occurred. Please try again or contact support.",
            500,
        )
    finally:
        if temp_image_path and os.path.exists(temp_image_path):
            try:
                os.remove(temp_image_path)
            except Exception as e:
                print(f"⚠️  Warning: Could not delete temp image: {e}")

        if processed_path and os.path.exists(processed_path):
            try:
                os.remove(processed_path)
            except Exception as e:
                print(f"⚠️  Warning: Could not delete processed audio: {e}")


@router.get("/auth/credits/check/vst")
async def check_credits_local(api_key: str = Depends(api_key_header)):
    if not api_key:
        raise HTTPException(
            status_code=401,
            detail={"error": {"code": "INVALID_KEY", "message": "API Key required"}},
        )

    is_valid, error_code, key_info = check_api_key_status(api_key)

    if not is_valid:
        if error_code == "INVALID_KEY":
            raise HTTPException(
                status_code=403,
                detail={"error": {"code": "INVALID_KEY", "message": "Invalid API key"}},
            )
        elif error_code == "KEY_EXPIRED":
            raise HTTPException(
                status_code=403,
                detail={"error": {"code": "KEY_EXPIRED", "message": "API key expired"}},
            )
        elif error_code == "CREDITS_EXHAUSTED":
            raise HTTPException(
                status_code=403,
                detail={
                    "error": {
                        "code": "CREDITS_EXHAUSTED",
                        "message": "No credits remaining",
                    }
                },
            )

    if key_info.get("is_limited"):
        credits_remaining = key_info.get("total_credits", 0) - key_info.get(
            "credits_used", 0
        )
        credits_total = key_info.get("total_credits", 0)
    else:
        credits_remaining = -1
        credits_total = -1

    return {
        "credits_remaining": credits_remaining,
        "credits_total": credits_total,
        "can_generate_standard": credits_remaining == -1 or credits_remaining >= 2,
        "cost_standard": 2,
    }
