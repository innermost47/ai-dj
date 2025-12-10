import os
from core.llm_interface import DJAILL
from core.music_generator import MusicGenerator
from core.layer_manager import LayerManager

BEATS_PER_BAR = 4


class DJSystem:
    _instance = None

    @classmethod
    def get_instance(cls, *args, **kwargs):
        if cls._instance is None:
            print("✨ First initialization of the OBSIDIAN-Neural system...")
            cls._instance = cls(*args, **kwargs)
        else:
            print("♻️ Reusing existing OBSIDIAN-Neural instance...")
        return cls._instance

    def __init__(self, args):
        if hasattr(self, "initialized") and self.initialized:
            print("⚠️ Reset attempt ignored - instance already initialized")
            return
        self.model_path = args.model_path
        self.output_dir_base = "./output"

        initial_llm_state = {
            "current_tempo": 126,
            "current_key": "C minor",
            "active_layers": {},
            "set_phase": "intro",
            "time_elapsed_beats": 0,
            "session_duration": 0,
            "last_action_time": 0,
            "history": [],
        }
        self.dj_brain = DJAILL(self.model_path, initial_llm_state)
        if not os.path.exists(self.output_dir_base):
            os.makedirs(self.output_dir_base)
        print("⌛ Loading sample generator...")
        self.music_gen = MusicGenerator(args.audio_model)

        print("⏳ Initializing LayerManager...")
        self.layer_manager = LayerManager(
            output_dir=os.path.join(self.output_dir_base),
        )

        self.initialized = True
        print("✅ OBSIDIAN-Neural system initialized successfully")
