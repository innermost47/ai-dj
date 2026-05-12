# OBSIDIAN-Neural

### Related Repositories

| Repository                                                                              | Description                                  |
| --------------------------------------------------------------------------------------- | -------------------------------------------- |
| [obsidian-neural-central](https://github.com/innermost47/obsidian-neural-central)       | Central inference server                     |
| [obsidian-neural-provider](https://github.com/innermost47/obsidian-neural-provider)     | Provider kit — run a GPU node on the network |
| [obsidian-neural-frontend](https://github.com/innermost47/obsidian-neural-frontend)     | Storefront & dashboard                       |
| [obsidian-neural-controller](https://github.com/innermost47/obsidian-neural-controller) | Mobile MIDI controller app                   |
| **[ai-dj](https://github.com/innermost47/ai-dj)** ← you are here                        | VST3/AU plugin (client)                      |

## AI music generation VST3 plugin designed for live performance

<div align="center">
  <img src="assets/screenshot.png" alt="OBSIDIAN-Neural Interface" width="800"/>
  <p><i>Live AI music generation in your DAW</i></p>
</div>

---

> _"I've cycled through almost every AI music tool on the market, but Obsidian is the first one that actually feels like a **real production tool** rather than a novelty. While other AI apps try to replace the songwriter, Obsidian treats AI like a powerful, playable instrument. The 8-track MIDI-triggering is a total game-changer. Because it lives directly in my DAW, there is **zero latency** and zero break in my workflow. It stays perfectly locked to my project's tempo and vibe, serving as the ultimate **intelligent jam partner VST**."_
>
> **— Moteka, Electronic Music Producer**
> [SoundCloud](https://soundcloud.com/moteka) · [Instagram](https://www.instagram.com/pmoteka/)

## The Multi-Model Era

OBSIDIAN Neural now features **8 specialized AI engines** in a single interface. You can now assign a different "brain" to each of your 8 tracks, each optimized for specific tempos and styles:

1.  **[stabilityai/stable-audio-open-1.0](https://huggingface.co/stabilityai/stable-audio-open-1.0)** — The versatile foundation for full-mix textures and drum loops.
    - _Best BPM: Versatile (80–160 BPM)._
2.  **[RoyalCities/Foundation-1](https://huggingface.co/RoyalCities/Foundation-1)** — Surgical tag-based control for melodic and harmonic phrasing.
    - _Best BPM: 100–150 BPM (Sweet spots: 120, 128)._
3.  **[adlb/Audialab_EDM_Elements](https://huggingface.co/adlb/Audialab_EDM_Elements)** — High-energy EDM leads, supersaws, and plucks.
    - _Best BPM: 100–150 BPM (Ideal at 128)._
4.  **[RoyalCities/RC_Infinite_Pianos](https://huggingface.co/RoyalCities/RC_Infinite_Pianos)** — High-fidelity grand and electric piano performances.
    - _Best BPM: 100–150 BPM (Optimal at 120)._
5.  **[RoyalCities/Vocal_Textures_Main](https://huggingface.co/RoyalCities/Vocal_Textures_Main)** — Choral, operatic, and atmospheric vocal chord progressions.
    - _Best BPM: 100–150 BPM (Best for atmospheric pads)._
6.  **[santifiorino/SAO-Instrumental-Finetune](https://huggingface.co/santifiorino/SAO-Instrumental-Finetune)** — Melodic trap, lofi jazz rap, and modern indie stems.
    - _Best BPM: 75–160 BPM (Great for 90 BPM Lofi)._
7.  **[gab-gdp/StableBeaT](https://huggingface.co/gab-gdp/StableBeaT)** — The drum machine: specialized in trap beats and 808 grooves.
    - _Best BPM: 75–160 BPM (Tightest at 140)._
8.  **[atoof/gluten_v1](https://huggingface.co/atoof/gluten_v1)** — Specialized engine for loopable melodic trap and wavy motifs.
    - _Best BPM: 90–160 BPM (Optimal at 135)._

⚠️ **Disclaimer:** This is a first implementation of the multi-model architecture. While I strive for the highest audio quality, AI generation can sometimes produce unexpected results. I am very much **open to feedback**! If you notice any issues with sound consistency or tempo sync for a specific model, please open a [GitHub Issue](https://github.com/innermost47/ai-dj/issues) or join the [Discussions](https://github.com/innermost47/ai-dj/discussions). Your feedback helps me fine-tune the engine!

🎁 New accounts now receive **20 free credits** on signup — no credit card required.

---

**⚡ Quick Start:** [Get your API key](https://obsidian-neural.com) and start generating in minutes.  
📄 **[Late Breaking Paper — AIMLA 2025](https://drive.google.com/file/d/1cwqmrV0_qC462LLQgQUz-5Cd422gL-8F/view)** — Presented at the AES International Conference on AI and Machine Learning for Audio, Queen Mary University London.  
🎓 **[Tutorial](https://youtu.be/-qdFo_PcKoY)** — From DAW setup to live performance (French + English subtitles)

---

## What OBSIDIAN Neural does

Type words → Get musical loops in ~30s. No stopping your creative flow.

### Performance
- **8-track sampler** with MIDI triggering (C3-B3)
- **4 pages per track** (A/B/C/D) — Switch variations instantly
- **8 sequences per page** — 256 total patterns for complex live sets
- **16-step sequencer** with multi-measure support
- **Quantized page changes** — Seamless transitions locked to measure boundaries
- **4 pair crossfaders** — Blend each deck A/B pair independently with model-aware color morphing
- **Plug-and-play MIDI mapping** — Auto-configured for the [companion mobile controller](https://github.com/innermost47/obsidian-neural-controller), with bidirectional feedback (LED states, knob positions) on dedicated MIDI channels
- **MIDI learn on every parameter** — Map any control to any hardware override, with persistent user mappings

### Sound design
- **Per-page ADSR envelope** — Shape the dynamics of each variation independently, editable directly on the waveform
- **Tempo-synced delay send** — 8 time divisions (1/16 to 2 bars), Stereo / Ping-Pong / Mono modes, per-track send level
- **Airwindows Console6 master bus** — Analog-modeled saturation for cohesive mix glue

### AI generation
- **Prompt bank with editor** — Build, organize and reuse your prompts with model-aware keywords (genres, elements, moods, negatives)
- **Drag-and-drop prompts** — Drop a prompt on a track to assign both prompt and AI model in one gesture
- **LLM bypass mode** — Skip prompt enhancement for faster generation when you already know what you want
- **Non-blocking generation** — No pre-recorded samples, renders in background

### Sync
- **Perfect DAW sync** — Auto time-stretch to project tempo
- **Standalone version** — Run OBSIDIAN Neural without a DAW, with built-in transport and tempo control
- **Ableton Link** (Standalone) — Bidirectional network sync of tempo and start/stop with any Link-enabled app

**OBSIDIAN Neural is NOT a song generator** like Suno or Udio. It's a performance tool: you build your track loop by loop, you're the composer, AI is your loop generator.

---

### Windows: low-latency audio

For best performance and sync accuracy with Ableton Link, select 
**"Windows Audio (Exclusive Mode)"** in the audio settings, then your 
audio interface. The default shared mode adds 20-50ms of latency.

If you have ASIO drivers from your audio interface vendor, those will 
work even better — but require a separate build with the Steinberg 
ASIO SDK (not redistributable).

---   

## How it works — The Distributed GPU Network

OBSIDIAN Neural runs on a **distributed GPU provider network**. When you generate a loop, the request is routed to an available community provider. If none is available, the system falls back to a cloud inference service.

```
VST Plugin → Central server → Provider GPU pool → WAV returned to DAW
                                    ↓ if unavailable
                               Cloud inference fallback
```

**Revenue sharing — full transparency:**  
Subscription revenue is redistributed **strictly equally** among all eligible providers each month via Stripe Connect, after a 15% platform fee covering infrastructure costs. Redistribution history is public:

**[obsidian-neural.com/public.html](https://obsidian-neural.com/public.html)**

No authentication required. No data is ever deleted.

**Provider eligibility:**

- Uptime score ≥ 80% (based on random unpredictable pings)
- At least 1 real job processed during the month

**🚀 We're looking for our first 10 GPU providers.**  
If you have a GPU and want to earn a share of the monthly revenue while supporting an open-source project: → **[Provider kit](https://github.com/innermost47/obsidian-neural-provider)**

---

## Quick Start

### 🟣 Cloud API 

> ☁️ **No GPU required** — Generation runs on our servers. Any laptop can run the plugin.

1. Download VST3 from [Releases](https://github.com/innermost47/ai-dj/releases)
2. Get your API key from [obsidian-neural.com](https://obsidian-neural.com)
3. Load the VST in your DAW → Settings → Enter Server URL + API key

**Pricing:**

| Plan    | Price        | Credits/month |
| ------- | ------------ | ------------- |
| Free    | —            | 20 samples    |
| Base    | €7.99/month  | 150 samples   |
| Starter | €11.99/month | 300 samples   |
| Pro     | €14.99/month | 500 samples   |

---

## Download

| Platform   | Install path                           |
| ---------- | -------------------------------------- |
| Windows    | `C:\Program Files\Common Files\VST3\`  |
| macOS VST3 | `~/Library/Audio/Plug-Ins/VST3/`       |
| macOS AU   | `~/Library/Audio/Plug-Ins/Components/` |
| Linux      | `~/.vst3/`                             |

→ **[Download from Releases](https://github.com/innermost47/ai-dj/releases)**

---

## Community

📧 [Contact](https://obsidian-neural.com/contact.html) · 💬 [GitHub Discussions](https://github.com/innermost47/ai-dj/discussions) · 🌐 [obsidian-neural.com](https://obsidian-neural.com)

---

## More Projects

🥁 **[BeatCrafter](https://github.com/innermost47/beatcrafter)** — Intelligent MIDI drum pattern generator VST3  
🎛️ **[Randomizer](https://randomizer.anthony-charretier.fr/)** — Generative music studio  
🎵 **[YouTube](https://www.youtube.com/@innermost9675)** — Original compositions (electronic, ambient, metal, experimental)

---

## License

🆓 **GNU Affero General Public License v3.0** — Stability AI Community License for the AI model.

---

## Credits

**Developed by InnerMost47 (Anthony Charretier)**

Special thanks to **Moteka** for the testimonial and early adoption, Stability AI, and all beta testers.

---

_Made with 🎵 in France_

[![Website](https://img.shields.io/badge/Website-obsidian--neural.com-blue)](https://obsidian-neural.com)
[![License](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](LICENSE)
[![GitHub Stars](https://img.shields.io/github/stars/innermost47/ai-dj?style=social)](https://github.com/innermost47/ai-dj)
