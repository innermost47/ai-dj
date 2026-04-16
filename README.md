# OBSIDIAN-Neural

### Related Repositories

| Repository                                                                              | Description                                  |
| --------------------------------------------------------------------------------------- | -------------------------------------------- |
| [obsidian-neural-central](https://github.com/innermost47/obsidian-neural-central)       | Central inference server                     |
| [obsidian-neural-provider](https://github.com/innermost47/obsidian-neural-provider)     | Provider kit — run a GPU node on the network |
| [obsidian-neural-frontend](https://github.com/innermost47/obsidian-neural-frontend)     | Storefront & dashboard                       |
| [obsidian-neural-controller](https://github.com/innermost47/obsidian-neural-controller) | Mobile MIDI controller app                   |
| **[ai-dj](https://github.com/innermost47/ai-dj)** ← you are here                        | VST3/AU plugin (client)                      |

## 🎵 Real-time AI music generation VST3 plugin for live performance

<div align="center">

[![Live Session](https://img.youtube.com/vi/5aXvwh3zIFE/maxresdefault.jpg)](https://youtu.be/5aXvwh3zIFE)

_Live improvisation — Foundation-1 by RoyalCities for melodic content · Stable Audio Open for drums_

</div>

---

> _"I've cycled through almost every AI music tool on the market, but Obsidian is the first one that actually feels like a **real production tool** rather than a novelty. While other AI apps try to replace the songwriter, Obsidian treats AI like a powerful, playable instrument. The 8-track MIDI-triggering is a total game-changer. Because it lives directly in my DAW, there is **zero latency** and zero break in my workflow. It stays perfectly locked to my project's tempo and vibe, serving as the ultimate **intelligent jam partner VST**."_
>
> **— Moteka, Electronic Music Producer**
> [SoundCloud](https://soundcloud.com/moteka) · [Instagram](https://www.instagram.com/pmoteka/)

## 🔥 What's New — April 2025: The Multi-Model Era

OBSIDIAN Neural now features **8 specialized AI engines** in a single interface. You can now assign a different "brain" to each of your 8 tracks:

1.  **Stable Audio Open 1.0** — The versatile foundation for full-mix textures and drum loops.
2.  **Foundation-1** — Surgical tag-based control for melodic and harmonic phrasing.
3.  **Audialab EDM Elements** — High-energy EDM leads, supersaws, and plucks.
4.  **RC Infinite Pianos** — High-fidelity grand and electric piano performances.
5.  **RC Vocal Textures** — Choral, operatic, and atmospheric vocal chord progressions.
6.  **SAO Instrumental** — Melodic trap, lofi jazz rap, and modern indie stems.
7.  **StableBeaT** — The drum machine: specialized in trap beats and 808 grooves.
8.  **Gluten-V1** — Specialized engine for loopable melodic trap and wavy motifs.

🎁 New accounts now receive **20 free credits** on signup — no credit card required.

---

**⚡ Quick Start:** [Get your API key](https://obsidian-neural.com) and start generating in minutes.  
📄 **[Late Breaking Paper — AIMLA 2025](https://drive.google.com/file/d/1cwqmrV0_qC462LLQgQUz-5Cd422gL-8F/view)** — Presented at the AES International Conference on AI and Machine Learning for Audio, Queen Mary University London.  
🎓 **[Tutorial](https://youtu.be/-qdFo_PcKoY)** — From DAW setup to live performance (French + English subtitles)

---

## What OBSIDIAN Neural does

Type words → Get musical loops instantly. No stopping your creative flow.

- **8-track sampler** with MIDI triggering (C3-B3)
- **4 pages per track** (A/B/C/D) — Switch variations instantly
- **8 sequences per page** — 256 total patterns for complex live sets
- **16-step sequencer** with multi-measure support
- **Quantized page changes** — Seamless transitions locked to measure boundaries
- **Draw-to-sound** — Sketch your ideas visually and let AI interpret them musically
- **Perfect DAW sync** — Auto time-stretch to project tempo
- **Real-time generation** — No pre-recorded samples

**OBSIDIAN Neural is NOT a song generator** like Suno or Udio. It's a performance tool: you build your track loop by loop, you're the composer, AI is your loop generator.

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

### 🟣 Cloud API (Recommended)

1. Download VST3 from [Releases](https://github.com/innermost47/ai-dj/releases)
2. Get your API key from [obsidian-neural.com](https://obsidian-neural.com)
3. Load the VST in your DAW → Settings → Enter Server URL + API key

📘 [Getting Started](https://obsidian-neural.com/documentation.html?page=getting-started)  
🎚️ [First Step](https://obsidian-neural.com/documentation.html?page=first-step)  
🎨 [Draw-to-Audio](https://obsidian-neural.com/documentation.html?page=draw-to-audio)  
🎛️ [Bank Management](https://obsidian-neural.com/documentation.html?page=bank-management)

**Pricing:**

| Plan    | Price        | Credits/month |
| ------- | ------------ | ------------- |
| Free    | —            | 20 samples    |
| Base    | €7.99/month  | 150 samples   |
| Starter | €11.99/month | 300 samples   |
| Pro     | €14.99/month | 500 samples   |

### 🔵 Self-Hosted (Advanced)

Best for privacy, customization, and unlimited generations.

1. Get [Stability AI access](https://huggingface.co/stabilityai/stable-audio-open-1.0)
2. Follow [INSTALLATION.md](INSTALLATION.md)
3. Run `python server_interface.py` → point the plugin to `http://localhost:8000`

**Windows:** `install-win.bat` · **macOS:** `./install-mac.sh` · **Linux:** `./install-lnx.sh`

### 🟢 Local Models (Offline — Windows only)

Runs completely offline. No GPU required, 16GB+ RAM.

1. Get [Stability AI access](https://huggingface.co/stabilityai/stable-audio-open-small)
2. Download models from [innermost47/stable-audio-open-small-tflite](https://huggingface.co/innermost47/stable-audio-open-small-tflite)
3. Copy to `%APPDATA%\OBSIDIAN-Neural\stable-audio\` → choose "Local Model" in plugin

_Note: Fixed 10s generation, higher RAM usage, some timing limitations._

---

## Which option should I choose?

| Feature           | Cloud API    | Self-Hosted      | Local Models  |
| ----------------- | ------------ | ---------------- | ------------- |
| Setup             | ⭐ Easy      | ⭐⭐⭐ Advanced  | ⭐⭐ Moderate |
| Hardware          | None         | GPU + CUDA/Metal | 16GB+ RAM     |
| Quality           | ⭐⭐⭐ Best  | ⭐⭐ Good        | ⭐ Basic      |
| Variable duration | ✅ Up to 30s | ✅ Yes           | ❌ Fixed 10s  |
| Cost              | Subscription | Free after setup | Free          |
| Internet          | Required     | Not required     | Not required  |

---

## 🌍 Press Coverage

Featured in **8 countries** and **6 languages**:

- 🇺🇸 **[Synthtopia](https://www.synthtopia.com/content/2025/12/22/obsidian-neural-brings-ai-generated-samples-to-your-daw/)** — "Brings AI-Generated Samples To Your DAW"
- 🇨🇳 **[MIDIFAN](https://www.midifan.com/modulenews-detailview-57259.htm)** — Leading Chinese music tech publication
- 🇳🇱 **[Rekkerd](https://rekkerd.org/obsidian-neural-real-time-ai-music-generation-vst3/)** — "Real-time AI music generation VST3"
- 🇫🇷 **[Audiofanzine](https://fr.audiofanzine.com/sequenceur-divers/obsidian-neural/obsidian-neural/news/a.play,n.78783.html)** — Major French music tech publication
- 🇪🇸 **[FutureMusic](https://www.futuremusic-es.com/obsidian-neural-vst3-ia-generativa/)** — Spanish coverage
- 🇰🇷 **[S1 Forum](https://s1forum.kr/news/innermost47%EC%97%90%EC%84%9C-obsidian-neural-%EA%B3%B5%EA%B0%9C/)** — Korean music production community
- 🇯🇵 **[DTM Plugin Sale](https://projectofnapskint.com/obsidian-2/)** — Japanese music production community
- 🇺🇸 **[Bedroom Producers Blog](https://bedroomproducersblog.com/2025/06/06/obsidian-neural-sound-engine/)** — "FREE AI-powered jam partner"

> _"Too many AI projects focus on the things AI can save you from doing rather than how AI can help you get better at what you do."_  
> **— James Nugent, Bedroom Producers Blog**

**[See all press coverage →](PRESS.md)**

---

## 🎯 Community Milestone — Road to 200 Stars!

Currently at **195+ stars** 🌟

When we hit 200 stars, we're celebrating with a community giveaway:

**Prize:** 1 year of Starter Pack free access (€143.88 value)  
**Eligibility:** Active community members (stars, discussions, issues, contributions)

👉 [Join the discussion](https://github.com/innermost47/ai-dj/discussions/156)

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

## 📚 Documentation

- **[Installation Guide](INSTALLATION.md)**
- **[Video Tutorial](https://youtu.be/-qdFo_PcKoY)** — French + English subtitles
- **[Online Documentation](https://obsidian-neural.com/documentation.html)**
- **[Press Coverage](PRESS.md)**

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

Special thanks to **A.D.[167]** for the original draw-to-audio concept, **Moteka** for the testimonial and early adoption, Stability AI, and all beta testers.

---

_Made with 🎵 in France_

[![Website](https://img.shields.io/badge/Website-obsidian--neural.com-blue)](https://obsidian-neural.com)
[![License](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](LICENSE)
[![GitHub Stars](https://img.shields.io/github/stars/innermost47/ai-dj?style=social)](https://github.com/innermost47/ai-dj)
