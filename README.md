# OBSIDIAN-Neural

### Related Repositories

| Repository                                                                              | Description                                  |
| --------------------------------------------------------------------------------------- | -------------------------------------------- |
| [obsidian-neural-central](https://github.com/innermost47/obsidian-neural-central)       | Central inference server                     |
| [obsidian-neural-provider](https://github.com/innermost47/obsidian-neural-provider)     | Provider kit - run a GPU node on the network |
| [obsidian-neural-frontend](https://github.com/innermost47/obsidian-neural-frontend)     | Storefront & dashboard                       |
| [obsidian-neural-controller](https://github.com/innermost47/obsidian-neural-controller) | Mobile MIDI controller app                   |
| **[ai-dj](https://github.com/innermost47/ai-dj)** ← you are here                        | VST3 / AU / Standalone                       |
| [raveMorph](https://github.com/innermost47/raveMorph)                                   | Neural sound morphing plugin (RAVE-based)    |
| [beatcrafter](https://github.com/innermost47/beatcrafter)                               | MIDI drum sequencer VST                      |

## AI music generation for live performance - VST3, AU, Standalone

<div align="center">
    <img src="screenshot.png" alt="OBSIDIAN-Neural Interface" width="800"/>
    <p><i>Live AI music generation. Standalone or in your DAW (VST3 / AU).</i></p>
</div>

---

> _"I've cycled through almost every AI music tool on the market, but Obsidian is the first one that actually feels like a **real production tool** rather than a novelty. While other AI apps try to replace the songwriter, Obsidian treats AI like a powerful, playable instrument. The 8-track MIDI-triggering is a total game-changer. Because it lives directly in my DAW, there is **zero latency** and zero break in my workflow. It stays perfectly locked to my project's tempo and vibe, serving as the ultimate **intelligent jam partner VST**."_
>
> **- Moteka, Electronic Music Producer**
> [SoundCloud](https://soundcloud.com/moteka) · [Instagram](https://www.instagram.com/pmoteka/)

---

## What OBSIDIAN Neural does

Type words → Get musical loops in ~30s. No stopping your creative flow.

### Performance
- **8-track sampler** with MIDI triggering (C3-B3)
- **4 pages per track** (A/B/C/D) - Switch variations instantly
- **8 sequences per page** - 256 total patterns for complex live sets
- **16-step sequencer** with multi-measure support
- **Quantized page changes** - Seamless transitions locked to measure boundaries
- **4 pair crossfaders** - Blend each deck A/B pair independently with model-aware color morphing
- **Plug-and-play MIDI mapping** - Auto-configured for the [companion mobile controller](https://github.com/innermost47/obsidian-neural-controller), with bidirectional feedback (LED states, knob positions) on dedicated MIDI channels.
  *To use: open the MIDI panel (piano icon, bottom-right) → Load Default Mapping.*
- **MIDI learn on every parameter** - Map any control to any hardware override, with persistent user mappings

### Sound design
- **Per-page ADSR envelope** - Shape the dynamics of each variation independently, editable directly on the waveform
- **Per-track gain control** - Adjust each sample's level (-12 / +12 dB) before mixing, with visual waveform feedback
- **Tempo-synced delay send** - 8 time divisions (1/16 to 2 bars), Stereo / Ping-Pong / Mono modes, per-track send level
- **Reverb send** - Per-track reverb with size, damping, width and mix controls
- **Airwindows Console6 master bus** - Analog-modeled saturation for cohesive mix glue

### AI generation

- **Prompt bank with editor** - Build, organize and reuse your prompts with model-aware keywords (genres, elements, moods, negatives)
- **Drag-and-drop prompts** - Drop a prompt on a track to assign both prompt and AI model in one gesture
- **Sample bank with drag-and-drop** - Every generation is automatically saved and can be reused across tracks and projects
- **LLM bypass mode** - Skip prompt enhancement for faster generation when you already know what you want
- **Non-blocking generation** - No pre-recorded samples, renders in background

### Multi-model engine

OBSIDIAN Neural ships with **8 specialized AI engines** - assign a different one to each track for its strengths:

1. **stable-audio-open-1.0** - Versatile foundation, drums and full-mix textures (80–160 BPM)
2. **Foundation-1** - Tag-based melodic and harmonic phrasing (100–150 BPM)
3. **Audialab EDM Elements** - High-energy EDM leads, supersaws, plucks (100–150 BPM)
4. **RC Infinite Pianos** - Grand and electric piano performances (100–150 BPM)
5. **RC Vocal Textures** - Choral, operatic and atmospheric vocals (100–150 BPM)
6. **SAO Instrumental** - Melodic trap, lofi jazz rap, indie stems (75–160 BPM)
7. **StableBeaT** - Trap beats and 808 grooves (75–160 BPM)
8. **gluten_v1** - Loopable melodic trap and wavy motifs (90–160 BPM)

All models are hosted on [Hugging Face](https://huggingface.co/innermost47/obsidian-neural-models).

> ⚠️ AI generation can produce unexpected results. Feedback welcome on [Issues](https://github.com/innermost47/ai-dj/issues) or [Discussions](https://github.com/innermost47/ai-dj/discussions).

### DAW & Standalone

- **Automatic tempo sync** - Incoming samples are BPM-detected with [MiniBPM](https://breakfastquay.com/minibpm/) and time-stretched on load via [Signalsmith](https://github.com/Signalsmith-Audio/signalsmith-stretch) Stretch to lock to your host tempo, with zero pitch drift
- **Standalone version** - Run OBSIDIAN Neural without a DAW, with built-in transport and tempo control
- **Ableton Link** (Standalone) - Bidirectional network sync of tempo and start/stop with any Link-enabled app

**OBSIDIAN Neural is NOT a song generator** like Suno or Udio. It's a performance tool: you build your track loop by loop, you're the composer, AI is your loop generator.

---

### Windows: low-latency audio

For best performance and sync accuracy with Ableton Link, select
**"Windows Audio (Exclusive Mode)"** in the audio settings, then your
audio interface. The default shared mode adds 20-50ms of latency.

If you have ASIO drivers from your audio interface vendor, those will
work even better - but require a separate build with the Steinberg
ASIO SDK (not redistributable).

---

## Quick Start

> ☁️ **No GPU required** - Generation runs on our servers. Any laptop can run the plugin.

1. Download VST3 from [Releases](https://github.com/innermost47/ai-dj/releases)
2. Get your API key from [obsidian-neural.com](https://obsidian-neural.com)
3. Load the VST in your DAW → Settings → Enter Server URL + API key

**Pricing:**

| Plan    | Price        | Credits/month |
| ------- | ------------ | ------------- |
| Free    | -            | 20 samples    |
| Base    | €7.99/month  | 150 samples   |
| Starter | €11.99/month | 300 samples   |
| Pro     | €14.99/month | 500 samples   |

---

## Download

| Platform           | Install path                            |
| ------------------ | --------------------------------------- |
| Windows VST3       | `C:\Program Files\Common Files\VST3\`   |
| Windows Standalone | Run `OBSIDIAN-Neural.exe` from anywhere |
| macOS VST3         | `~/Library/Audio/Plug-Ins/VST3/`        |
| macOS AU           | `~/Library/Audio/Plug-Ins/Components/`  |
| macOS Standalone   | `/Applications/` (drag the `.app`)      |
| Linux VST3         | `~/.vst3/`                              |
| Linux Standalone   | Run the binary from anywhere            |

→ **[Download from Releases](https://github.com/innermost47/ai-dj/releases)**

---

## How it works - The Distributed GPU Network

OBSIDIAN Neural runs on a **distributed GPU provider network**. When you generate a loop, the request is routed to an available community provider. If none is available, the system falls back to a cloud inference service.

```
VST Plugin → Central server → Provider GPU pool → WAV returned to DAW
                                    ↓ if unavailable
                               Cloud inference fallback
```

**Revenue sharing - full transparency:**  
Subscription revenue is redistributed **strictly equally** among all eligible providers each month via Stripe Connect, after a 15% platform fee covering infrastructure costs. Redistribution history is public:

**[obsidian-neural.com/public.php](https://obsidian-neural.com/public.php)**

No authentication required. No data is ever deleted.

**Provider eligibility:**

- Uptime score ≥ 80% (based on random unpredictable pings)
- At least 1 real job processed during the month

**🚀 We're looking for our first 10 GPU providers.**  
If you have a GPU and want to earn a share of the monthly revenue while supporting an open-source project: → **[Provider kit](https://github.com/innermost47/obsidian-neural-provider)**

---

## Community

📧 [Contact](https://obsidian-neural.com/contact.php) · 💬 [GitHub Discussions](https://github.com/innermost47/ai-dj/discussions) · 🌐 [obsidian-neural.com](https://obsidian-neural.com)

---

## More Projects

🥁 **[BeatCrafter](https://github.com/innermost47/beatcrafter)** - Intelligent MIDI drum pattern generator VST3  
🎛️ **[Randomizer](https://randomizer.anthony-charretier.fr/)** - Generative music studio  
🎵 **[YouTube](https://www.youtube.com/@innermost9675)** - Original compositions (electronic, ambient, metal, experimental)

---

## License

🆓 **GNU Affero General Public License v3.0** - Stability AI Community License for the AI model.

---

## Credits

**Developed by InnerMost47 (Anthony Charretier)**

Special thanks to **Moteka** for the testimonial and early adoption, Stability AI, and all beta testers.

---

_Made with 🎵 in France_

[![Website](https://img.shields.io/badge/Website-obsidian--neural.com-blue)](https://obsidian-neural.com)
[![License](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](LICENSE)
[![GitHub Stars](https://img.shields.io/github/stars/innermost47/ai-dj?style=social)](https://github.com/innermost47/ai-dj)
