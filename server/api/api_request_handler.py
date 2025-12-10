import os
import time
from fastapi import HTTPException
from core.dj_system import DJSystem
from server.api.models import GenerateRequest


class APIRequestHandler:
    def __init__(self, dj_system):
        self.dj_system: DJSystem = dj_system

    def setup_llm_session(self, request: GenerateRequest, request_id, user_id):
        print(f"🔄 Minimal LLM Setup...")

        self.dj_system.dj_brain.init_model()

        self.dj_system.dj_brain.session_state = {
            "current_tempo": request.bpm,
            "current_key": request.key or "C minor",
            "user_prompt": request.prompt,
            "request_id": request_id,
            "last_action_time": time.time(),
            "user_id": user_id,
        }

    def get_llm_decision(self):

        print(f"🧠 LLM Consultation...")

        llm_decision = self.dj_system.dj_brain.get_next_decision()

        reasoning = llm_decision.get("reasoning", "N/A")
        sample_details = llm_decision.get("parameters", {}).get("sample_details", {})
        musicgen_prompt = sample_details.get("musicgen_prompt", "")

        print(f"💭 LLM Reasoning: {reasoning}")
        print(f"🎵 MusicGen Prompt: '{musicgen_prompt}'")

        self.dj_system.dj_brain.destroy_model()

        return llm_decision

    def generate_simple(self, request: GenerateRequest, llm_decision: dict):
        if isinstance(llm_decision, str):
            musicgen_prompt = llm_decision
        else:
            sample_details = llm_decision.get("parameters", {}).get(
                "sample_details", {}
            )
            musicgen_prompt = sample_details.get("musicgen_prompt", request.prompt)

        self.dj_system.music_gen.init_model()
        audio, sample_info = self.dj_system.music_gen.generate_sample(
            musicgen_prompt=musicgen_prompt,
            tempo=request.bpm,
            generation_duration=request.generation_duration or 6,
            sample_rate=int(request.sample_rate),
        )
        self.dj_system.music_gen.destroy_model()
        return audio, sample_info

    def process_audio_pipeline(self, audio, request: GenerateRequest, request_id):
        temp_path = os.path.join(
            self.dj_system.output_dir_base, f"temp_raw_{request_id}.wav"
        )
        self.dj_system.music_gen.save_sample(
            audio, temp_path, sample_rate=int(request.sample_rate)
        )

        self.dj_system.layer_manager.master_tempo = request.bpm
        processed_path = self.dj_system.layer_manager._prepare_sample_for_loop(
            original_audio_path=temp_path,
            layer_id=f"simple_loop_{request_id}",
            sample_rate=int(request.sample_rate),
        )

        if not processed_path:
            raise HTTPException(status_code=500, detail="Loop preparation failure")

        if os.path.exists(temp_path) and temp_path != processed_path:
            os.remove(temp_path)

        return processed_path
