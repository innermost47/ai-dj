# Installation Guide

Complete installation instructions for OBSIDIAN-Neural VST3 plugin.

## Prerequisites

### System Requirements

- **Windows:** 10/11 x64
- **macOS:** 10.15+ (Intel & Apple Silicon)
- **Linux:** Ubuntu 20.04+ x64
- **RAM:** 16GB+ recommended for local mode
- **Storage:** ~3GB for models and dependencies

### Stability AI Access

1. Create account on [Hugging Face](https://huggingface.co/)
2. Request access to [Stable Audio Open](https://huggingface.co/stabilityai/stable-audio-open-small)
3. Wait for approval (usually minutes to hours)
4. Generate access token in HF settings

---

## Option 1: Local Models (Windows only)

**Completely offline. No servers, Python, or GPU needed.**

### Step 1: Download Models

1. Get approval for Stable Audio Open (see Prerequisites)
2. Download all files from [innermost47/stable-audio-open-small-tflite](https://huggingface.co/innermost47/stable-audio-open-small-tflite)
3. Copy models to: `%APPDATA%\OBSIDIAN-Neural\stable-audio\` (Windows)

### Step 2: Install VST3

1. Download VST3 from [Releases](https://github.com/innermost47/ai-dj/releases)
2. Extract and copy to VST3 folder:
   - `C:\Program Files\Common Files\VST3\`

### Step 3: Configure Plugin

1. Load OBSIDIAN-Neural in your DAW
2. Choose "Local Model" option
3. Plugin will automatically find models in AppData folder

### Current Limitations

- Fixed 10-second generation
- Some timing/quantization issues
- High RAM usage during generation

---

## Option 2: Build from Source

**Full control with local server - for developers and advanced users.**

### Prerequisites

- **Python 3.10+** from [python.org](https://python.org)
- **Git** for cloning repository
- **Build tools** for VST3 compilation (optional)
- **CUDA/ROCm** for GPU acceleration (optional)

#### Manual Build

```bash
git clone https://github.com/innermost47/ai-dj.git
cd ai-dj
python installer.py
```

### What the Installer Does

- Creates isolated Python virtual environment
- Installs all dependencies (PyTorch, FastAPI, Stable Audio Tools, etc.)
- Downloads AI models (Gemma-3-4B, ~2.5GB)
- Detects and configures CUDA/ROCm if available
- Compiles VST3 plugin (optional, requires build tools)
- Sets up configuration files

### Post-Installation Setup

#### 1. Launch Server Interface

```bash
python server_interface.py
```

**Features:**

- System tray integration with green triangle icon
- Real-time server status and controls
- Configuration management with API keys
- Secure Hugging Face token storage
- Live server logs with color coding
- First-time setup wizard

#### 2. Configure Server

- **First launch:** Setup wizard guides through configuration
- **Hugging Face Token:** Enter approved token (verification available)
- **API Keys:** Generate with credit limits or unlimited access

#### 3. Start Server

- Click "Start" in server interface
- **Authentication prompt:**
  - **Yes:** Use API keys (for production/network access)
  - **No:** Development bypass (for localhost only)
- Note the server URL (usually `http://localhost:8000`)

#### 4. Configure VST3

- Load OBSIDIAN-Neural in your DAW
- **Server URL:** Paste from server GUI
- **API Key:** Copy from server interface (if using authentication)

### Troubleshooting

#### Common Issues

- **No Hugging Face access:** Must be approved for Stable Audio Open first
- **Build errors:** Download pre-compiled VST3 from releases instead
- **Connection failed:** Ensure server is running before configuring VST
- **API confusion:** Choose "No" authentication for simple localhost setup

#### Windows Specific

- Ensure Python is added to PATH
- Visual Studio Build Tools may be required for some dependencies

#### macOS Specific

- May require Xcode command line tools: `xcode-select --install`
- For Apple Silicon: Ensure Python supports ARM64

#### Linux Specific

- Install development packages: `sudo apt install python3-dev build-essential`
- May require additional audio libraries

### Performance Optimization

#### GPU Acceleration

- **NVIDIA:** CUDA will be detected and configured automatically
- **AMD:** ROCm support available on Linux
- **No GPU:** CPU-only mode works but slower generation

#### Memory Management

- **16GB+ RAM:** Recommended for stable operation
- **8GB RAM:** May work with reduced model size
- **Swap/Virtual Memory:** Ensure adequate if RAM limited

---

## Verification

### Test Installation

1. Load plugin in DAW
2. Type simple prompt: "techno kick"
3. Click generate button
4. Wait for audio generation
5. Play generated sample

### Expected Behavior

- Generation takes 10-30 seconds depending on setup
- Audio appears in waveform display
- Sample plays when triggered via MIDI (C3-B3)
- DAW sync works with project tempo

---

## Uninstallation

### Remove Files

- Delete VST3 plugin from VST folder
- Remove `%APPDATA%\OBSIDIAN-Neural\` folder (Windows)
- Delete downloaded models and cache

### Server Installation

- Stop server process
- Delete cloned repository folder
- Remove Python virtual environment

---

## Getting Help

### Documentation

- **Tutorial:** [YouTube Video](https://youtu.be/-qdFo_PcKoY)
- **Community:** [GitHub Discussions](https://github.com/innermost47/ai-dj/discussions)
- **Documentation:** [obsidian-neural.com/documentation.html](https://obsidian-neural.com/documentation.html)

### Support

- **Issues:** [GitHub Issues](https://github.com/innermost47/ai-dj/issues)
- **Status:** Check server interface logs for detailed error messages

### Before Reporting Issues

Include the following information:

- Operating system and version
- DAW name and version
- Installation method used
- Complete error messages
- Steps to reproduce problem

---

_For the latest installation instructions and troubleshooting, always check the main repository._
