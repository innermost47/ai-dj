# OBSIDIAN-Neural

### Related Repositories

| Repository                                                                              | Description                                                |
| --------------------------------------------------------------------------------------- | ---------------------------------------------------------- |
| **[ai-dj](https://github.com/innermost47/ai-dj)** ← you are here                        | VST3 / AU / Standalone                                     |
| [obsidian-neural-gpu-server](https://github.com/innermost47/obsidian-neural-gpu-server) | Self-hosted GPU server for the 9 engines (Windows for now) |
| [obsidian-neural-controller](https://github.com/innermost47/obsidian-neural-controller) | Mobile MIDI controller app                                 |
| [raveMorph](https://github.com/innermost47/raveMorph)                                   | Neural sound morphing plugin (RAVE-based)                  |
| [beatcrafter](https://github.com/innermost47/beatcrafter)                               | MIDI drum sequencer VST                                    |

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

## 🆓 OBSIDIAN Neural is now 100% free and open source

The whole plugin - including the local CPU engine - is now free and released under the **GNU AGPL v3.0**. No license key, no subscription, no credits, no account.

- **Stable Audio 3 Medium runs entirely on your own CPU.** No GPU, no cloud, no internet required after downloading the model once. Nothing ever leaves your computer.
- **The 9 GPU engines run on your own machine too.** With [obsidian-neural-gpu-server](https://github.com/innermost47/obsidian-neural-gpu-server), you host the inference server yourself on an NVIDIA GPU - no account, no API key.

> ⚡ Runs on a standard CPU. Reference: ~11s per generation on a recent laptop CPU, alongside a full DAW session.
> 🍎 macOS: Apple Silicon (M1+) only - Intel Macs not supported.

---

## 🖥️ GPU engines: run your own server

Out of the box, OBSIDIAN Neural generates locally with **Stable Audio 3 Medium** on your CPU. To unlock the **9 specialized GPU engines**, run **[obsidian-neural-gpu-server](https://github.com/innermost47/obsidian-neural-gpu-server)** on a machine with an NVIDIA GPU - the same computer as your DAW, or another one on your network.

> 🪟 The server currently supports **Windows** only.

1. Clone [obsidian-neural-gpu-server](https://github.com/innermost47/obsidian-neural-gpu-server) and follow its README to install it and download the models
2. Start the server
3. In the plugin: Settings → **Server/API** → enter the server URL (e.g. `http://localhost:8000`)
4. Pick one of the 9 engines and generate

### Contributions welcome

- Linux and macOS support for the GPU server
- Local GPU inference directly in the plugin (CUDA, Metal, DirectML…)
- More models exported to ONNX for the CPU engine

→ **[Start a discussion](https://github.com/innermost47/ai-dj/discussions)** · **[Open an issue](https://github.com/innermost47/ai-dj/issues)**

---

## What OBSIDIAN Neural does

Type words → Get musical loops. No stopping your creative flow.

### Performance

- **8-track sampler** with MIDI triggering (C3-B3)
- **4 pages per track** (A/B/C/D) - Switch variations instantly
- **8 sequences per page** - 256 total patterns for complex live sets
- **16-step sequencer** with multi-measure support
- **Quantized page changes** - Seamless transitions locked to measure boundaries
- **4 pair crossfaders with master bypass** - Blend each deck A/B pair independently with model-aware color morphing, or toggle the entire crossfader section off for direct routing
- **Plug-and-play MIDI mapping** - Auto-configured for the [companion mobile controller](https://github.com/innermost47/obsidian-neural-controller), with bidirectional feedback (LED states, knob positions) on dedicated MIDI channels.
  _To use: open the MIDI panel (piano icon, bottom-right) → Load Default Mapping._
- **MIDI learn on every parameter** - Map any control to any hardware override, with persistent user mappings

### Sound design

- **Per-page ADSR envelope** - Shape the dynamics of each variation independently, editable directly on the waveform
- **Per-track gain control** - Adjust each sample's level (-12 / +12 dB) before mixing, with visual waveform feedback
- **Per-track reverse** - Instantly flip any page's playback direction for reversed textures and risers
- **Per-track transient scatter** - Randomize and reposition transients for glitchy, stuttering rhythmic variations
- **Per-track glitch sequencer** - Sequence glitch effects step by step for rhythmic, stuttering variations
- **8 modulators per track** - Eight independent modulation sources on every track to bring movement to your sounds
- **Per-track multi-mode filter** - LP/HP/BP with 12 or 24 dB slopes, drive, cutoff and resonance for sculpting each voice
- **Per-track 8-band graphic EQ + master EQ** - Independent frequency shaping on every voice and global bus, from 40 Hz to 15 kHz
- **Per-track compressor + master compressor** - Full dynamic control on each voice and on the master bus, with threshold, ratio, attack, release and makeup gain
- **Per-track limiter + master limiter** - Peak protection on every voice and on the master bus with adjustable release and ceiling
- **Per-track distortion** - 6 character modes (Soft, Hard, Tube, Fold, Diode, Cubic) with pre/post gain and high-pass cutoff
- **Per-track bitcrusher** - Bit depth and sample rate reduction for lo-fi, digital degradation
- **Per-track chorus** - Modulation effect with rate, depth, delay, feedback and dry/wet control for added stereo width
- **Per-track phaser** - Sweeping notch modulation with rate, depth, feedback and stages for movement and width
- **Per-track flanger** - Classic jet-swoosh modulation with rate, depth, feedback and delay control
- **Tempo-synced delay send** - 8 time divisions (1/16 to 2 bars), Stereo / Ping-Pong / Mono modes, per-track send level
- **Reverb send** - Per-track reverb with size, damping, width and mix controls
- **Airwindows Console6 master bus** - Analog-modeled saturation for cohesive mix glue

### AI generation

- **Local CPU generation** with Stable Audio 3 Medium - fully offline
- **Prompt bank with editor** - Build, organize and reuse your prompts with model-aware keywords (genres, elements, moods, negatives)
- **Drag-and-drop prompts** - Drop a prompt on a track to assign both prompt and AI model in one gesture
- **Sample bank with drag-and-drop** - Every generation is automatically saved and can be reused across tracks and projects
- **Non-blocking generation** - No pre-recorded samples, renders in background

### GPU engines (server mode)

The plugin supports **9 specialized AI engines** in server mode, powered by your own [obsidian-neural-gpu-server](https://github.com/innermost47/obsidian-neural-gpu-server) - see [GPU engines: run your own server](#️-gpu-engines-run-your-own-server).

1. **stable-audio-open-1.0** - Versatile foundation, drums and full-mix textures (80–160 BPM)
2. **Stable Audio 3 Medium** - Next-gen flexible full tracks, isolated stems, FX (80–160 BPM) _(also the local CPU engine)_
3. **Foundation-1** - Tag-based melodic and harmonic phrasing (100–150 BPM)
4. **Audialab EDM Elements** - High-energy EDM leads, supersaws, plucks (100–150 BPM)
5. **RC Infinite Pianos** - Grand and electric piano performances (100–150 BPM)
6. **RC Vocal Textures** - Choral, operatic and atmospheric vocals (100–150 BPM)
7. **SAO Instrumental** - Melodic trap, lofi jazz rap, indie stems (75–160 BPM)
8. **StableBeaT** - Trap beats and 808 grooves (75–160 BPM)
9. **gluten_v1** - Loopable melodic trap and wavy motifs (90–160 BPM)

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

1. Download the plugin from [Releases](https://github.com/innermost47/ai-dj/releases)
2. Load it in your DAW (or run the Standalone) → Settings → **Local Model**
3. Accept the model licenses and download the model once
4. Type a prompt and generate - fully offline from now on

Want the 9 GPU engines? See [GPU engines: run your own server](#️-gpu-engines-run-your-own-server).

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

## Contributing

Contributions of any size are welcome: bug fixes, new effects, UI improvements, documentation, and especially Linux/macOS support for the [GPU server](https://github.com/innermost47/obsidian-neural-gpu-server).

💬 [GitHub Discussions](https://github.com/innermost47/ai-dj/discussions) · 🐛 [Issues](https://github.com/innermost47/ai-dj/issues)

---

## License

🆓 **GNU Affero General Public License v3.0** - see [LICENSE](LICENSE.txt).

The AI models are not part of this repository and are not covered by the AGPL. They are downloaded separately and remain subject to their own terms: the [Stability AI Community License](https://stability.ai/license) and the [Gemma Terms of Use](https://ai.google.dev/gemma/terms).

---

## Credits

**Developed by InnerMost47 (Anthony Charretier)**

Special thanks to **Moteka** for the testimonial and early adoption, Stability AI, and all beta testers and early adopters.

---

_Made with 🎵 in France_

[![License](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](LICENSE.txt)
[![GitHub Stars](https://img.shields.io/github/stars/innermost47/ai-dj?style=social)](https://github.com/innermost47/ai-dj)
