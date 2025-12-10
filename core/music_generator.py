import torch
import numpy as np
import tempfile
import os
import time
import random
import gc
import librosa
import soundfile as sf
from stable_audio_tools import get_pretrained_model
from einops import rearrange
from stable_audio_tools.inference.generation import (
    generate_diffusion_cond,
)


class MusicGenerator:
    def __init__(self, model_id="stable-audio-open-1.0"):
        self.model_id = model_id
        self.model = None
        self.sample_rate = 44100
        self.default_duration = 6
        self.sample_cache = {}

    def init_model(self):
        print(f"⚡ Initializing Stable Audio Model..")

        device = "cuda" if torch.cuda.is_available() else "cpu"
        print(f"ℹ️  Using device: {device}")

        self.model, self.model_config = get_pretrained_model(self.model_id)
        self.sample_rate = self.model_config["sample_rate"]
        self.sample_size = self.model_config["sample_size"]
        self.model = self.model.to(device)
        self.device = device

        print(f"✅ Stable Audio initialized (sample rate: {self.sample_rate}Hz)!")

    def destroy_model(self):
        self.model = None
        if torch.cuda.is_available():
            torch.cuda.empty_cache()
        gc.collect()

    def generate_sample(
        self,
        musicgen_prompt,
        tempo,
        sample_type="custom",
        generation_duration=6,
        sample_rate=48000,
    ):
        try:
            print(f"🔮 Direct generation with prompt: '{musicgen_prompt}'")

            seconds_total = generation_duration
            conditioning = [
                {
                    "prompt": musicgen_prompt,
                    "seconds_start": 0,
                    "seconds_total": seconds_total,
                }
            ]

            cfg_scale = 7.0
            sampler_type = "dpmpp-3m-sde"
            steps_value = 75
            if self.model_id == "stabilityai/stable-audio-open-small":
                cfg_scale = 1.0
                steps_value = 8
                sampler_type = "pingpong"

            seed_value = random.randint(0, 2**31 - 1)

            print(f"⚙️  Stable Audio: steps={steps_value}, cfg_scale={cfg_scale}")

            output = generate_diffusion_cond(
                self.model,
                steps=steps_value,
                cfg_scale=cfg_scale,
                conditioning=conditioning,
                sample_size=self.sample_size,
                sigma_min=0.3,
                sigma_max=500,
                sampler_type=sampler_type,
                device=self.device,
                seed=seed_value,
            )

            target_samples = int(seconds_total * self.sample_rate)

            print(f"✅ Diffusion steps complete!")
            start_post = time.time()

            start = time.time()
            output = rearrange(output, "b d n -> d (b n)")
            print(f"⏱️  Rearrange: {time.time() - start:.2f}s")

            start = time.time()
            target_samples = int(seconds_total * self.sample_rate)
            if output.shape[1] > target_samples:
                output = output[:, :target_samples]
            print(f"⏱️  Truncate: {time.time() - start:.2f}s")

            start = time.time()
            output_normalized = (
                output.to(torch.float32)
                .div(torch.max(torch.abs(output) + 1e-8))
                .cpu()
                .numpy()
            )

            print(f"⏱️  Normalize + CPU transfer: {time.time() - start:.2f}s")
            print(f"⏱️  Total post-processing: {time.time() - start_post:.2f}s")

            sample_audio = (
                output_normalized[0]
                if output_normalized.shape[0] > 1
                else output_normalized
            )

            del output, output_normalized

            print(f"✅ Generation complete!")

            sample_info = {
                "type": sample_type,
                "tempo": tempo,
                "prompt": musicgen_prompt,
            }

            return sample_audio, sample_info

        except Exception as e:
            print(f"❌ Generation error: {str(e)}")
            silence = np.zeros(sample_rate * 4)
            error_info = {"type": sample_type, "tempo": tempo, "error": str(e)}
            return silence, error_info

    def save_sample(self, sample_audio, filename, sample_rate=48000):
        try:
            if filename.endswith(".wav"):
                path = filename
            else:
                temp_dir = tempfile.gettempdir()
                path = os.path.join(temp_dir, filename)
            if not isinstance(sample_audio, np.ndarray):
                sample_audio = np.array(sample_audio)
            if self.sample_rate != sample_rate:
                print(
                    f"🔄 Resampling {str(self.sample_rate)}Hz → "
                    + str(sample_rate)
                    + "hz"
                )
                sample_audio = librosa.resample(
                    sample_audio, orig_sr=self.sample_rate, target_sr=sample_rate
                )
                save_sample_rate = sample_rate
            else:
                save_sample_rate = self.sample_rate

            max_val = np.max(np.abs(sample_audio))
            if max_val > 0:
                sample_audio = sample_audio / max_val * 0.9

            sf.write(path, sample_audio, save_sample_rate)

            return path
        except Exception as e:
            print(f"❌ Error saving sample: {e}")
            return None
