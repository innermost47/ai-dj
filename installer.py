import sys
import subprocess
import os
from pathlib import Path


def safe_run(cmd, **kwargs):
    if sys.platform == "win32":
        kwargs.setdefault("creationflags", subprocess.CREATE_NO_WINDOW)
    return subprocess.run(cmd, **kwargs)


def check_and_install_dependencies():
    print("=== OBSIDIAN-Neural Installer Bootstrap ===\n")

    missing = []

    try:
        import tkinter

        print("✓ tkinter available")
    except ImportError:
        missing.append(("tkinter", "CRITICAL - Reinstall Python with tkinter support"))

    try:
        from PIL import Image

        print("✓ Pillow available")
    except ImportError:
        missing.append(("Pillow", "pip"))

    try:
        import psutil

        print("✓ psutil available")
    except ImportError:
        missing.append(("psutil", "pip"))

    try:
        import GPUtil

        print("✓ GPUtil available")
    except ImportError:
        missing.append(("GPUtil", "pip"))

    if not missing:
        print("\n✓ All installer dependencies available\n")
        return True

    print("\n⚠ Missing installer dependencies:")
    for pkg, install_method in missing:
        if install_method == "CRITICAL":
            print(f"  ✗ {pkg} - {install_method}")
        else:
            print(f"  ✗ {pkg}")

    tkinter_missing = any(pkg == "tkinter" for pkg, _ in missing)
    if tkinter_missing:
        print("\n❌ CRITICAL: tkinter is not available")
        print("tkinter is required for the installer GUI.")
        print("\nSolutions:")
        print("  - Windows: Reinstall Python and check 'tcl/tk and IDLE'")
        print("  - Linux: sudo apt install python3-tk")
        print("  - macOS: tkinter should be included with Python")
        input("\nPress Enter to exit...")
        return False

    print("\nThese packages are required to run the installer GUI.")
    response = input("Install them now? (y/n): ").strip().lower()

    if response != "y":
        print("\n❌ Cannot proceed without dependencies.")
        input("Press Enter to exit...")
        return False

    print("\nInstalling missing packages...\n")
    for pkg, install_method in missing:
        if install_method == "pip":
            print(f"Installing {pkg}...")
            try:
                safe_run(
                    [sys.executable, "-m", "pip", "install", pkg],
                    check=True,
                    capture_output=True,
                )
                print(f"  ✓ {pkg} installed successfully")
            except subprocess.CalledProcessError as e:
                print(f"  ✗ Failed to install {pkg}")
                print(f"    Error: {e.stderr.decode() if e.stderr else 'Unknown'}")
                return False

    print("\n✓ All dependencies installed successfully!")
    print("Please restart the installer manually.\n")
    input("Press Enter to exit...")
    return False


if __name__ == "__main__":
    if not check_and_install_dependencies():
        sys.exit(1)

import platform
import time
import ctypes
import re
import json
import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext, filedialog
import threading
import shutil
import urllib.request
import psutil
import GPUtil

try:
    from PIL import Image, ImageTk

    PIL_AVAILABLE = True
except ImportError:
    PIL_AVAILABLE = False


if platform.system() == "Windows":
    try:
        if hasattr(sys.stdout, "reconfigure"):
            sys.stdout.reconfigure(encoding="utf-8")
            sys.stderr.reconfigure(encoding="utf-8")
        else:
            os.environ["PYTHONIOENCODING"] = "utf-8"
    except (AttributeError, OSError):
        os.environ["PYTHONIOENCODING"] = "utf-8"


def is_frozen():
    return getattr(sys, "frozen", False) and hasattr(sys, "_MEIPASS")


class ObsidianNeuralInstaller:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("OBSIDIAN-Neural - Full Installer")
        self.root.geometry("850x750")
        self.root.minsize(800, 600)
        self.root.resizable(True, True)
        self.skip_vst_build = tk.BooleanVar(value=False)
        self.root.update_idletasks()
        x = (self.root.winfo_screenwidth() // 2) - (850 // 2)
        y = (self.root.winfo_screenheight() // 2) - (750 // 2)
        self.root.geometry(f"850x750+{x}+{y}")
        self.is_admin = self.check_admin()
        if platform.system() == "Windows":
            if self.is_admin:
                default_install_path = "C:/ProgramData/OBSIDIAN-Neural"
            else:
                default_install_path = str(Path.home() / "OBSIDIAN-Neural")
        elif platform.system() == "Darwin":
            default_install_path = str(Path.home() / "OBSIDIAN-Neural")
        else:
            if self.is_admin:
                default_install_path = "/opt/OBSIDIAN-Neural"
            else:
                default_install_path = str(Path.home() / "OBSIDIAN-Neural")

        self.install_path = tk.StringVar(value=default_install_path)
        detected_vst_path_val = self._do_detect_vst_folder()
        self.vst_path = tk.StringVar(value=detected_vst_path_val)
        self.progress_var = tk.DoubleVar()
        self.status_var = tk.StringVar(value="Ready to install")

        self.install_python = tk.BooleanVar(value=not self.check_python())
        self.install_cmake = tk.BooleanVar(value=not self.check_cmake())
        self.install_git = tk.BooleanVar(value=not self.check_git())
        self.install_buildtools = tk.BooleanVar(value=not self.check_buildtools())
        self.install_cuda = tk.BooleanVar(value=False)
        self.run_benchmark = tk.BooleanVar(value=True)
        self.auto_copy_vst = tk.BooleanVar(value=True)
        self.start_after_install = tk.BooleanVar(value=False)

        self.system_info = self.get_system_info()
        self.setup_ui()
        self.log_system_info()

    def safe_subprocess_run(self, cmd, **kwargs):
        if is_frozen() and sys.platform == "win32":
            kwargs.setdefault("creationflags", subprocess.CREATE_NO_WINDOW)
            kwargs.setdefault("close_fds", True)
        return subprocess.run(cmd, **kwargs)

    def check_admin(self):
        if platform.system() == "Windows":
            try:
                return ctypes.windll.shell32.IsUserAnAdmin() != 0
            except:
                return False
        else:
            return os.geteuid() == 0

    def request_admin(self):
        if platform.system() == "Windows":
            if not self.is_admin:
                executable_path = sys.executable
                params = " ".join(sys.argv)
                try:
                    ret_shell = ctypes.windll.shell32.ShellExecuteW(
                        None, "runas", executable_path, params, None, 1
                    )
                    sys.exit(0)
                except Exception as e_shell:
                    os._exit(1)
        elif platform.system() == "Darwin":
            if not self.is_admin:
                script = f"""
                do shell script "sudo {sys.executable} {' '.join(sys.argv)}" with administrator privileges
                """
                try:
                    subprocess.run(["osascript", "-e", script])
                    sys.exit(0)
                except:
                    self.log("Could not request admin privileges on macOS", "WARNING")
        else:
            if not self.is_admin:
                try:
                    os.execvp("sudo", ["sudo", sys.executable] + sys.argv)
                except:
                    self.log("Could not request admin privileges on Linux", "WARNING")

    def _do_detect_vst_folder(self):
        print("Detecting VST3 folder (initial scan, output to console):")
        potential_paths = []

        if platform.system() == "Windows":
            program_files_vst3 = (
                Path(os.environ.get("PROGRAMFILES", "C:/Program Files"))
                / "Common Files"
                / "VST3"
            )
            program_files_x86_vst3 = (
                Path(os.environ.get("PROGRAMFILES(X86)", "C:/Program Files (x86)"))
                / "Common Files"
                / "VST3"
            )
            appdata_vst3 = Path.home() / "AppData/Roaming/VST3"
            home_dot_vst3 = Path.home() / ".vst3"

            if self.is_admin:
                potential_paths.extend(
                    [
                        program_files_vst3,
                        program_files_x86_vst3,
                        appdata_vst3,
                        home_dot_vst3,
                    ]
                )
            else:
                potential_paths.extend(
                    [
                        appdata_vst3,
                        home_dot_vst3,
                        program_files_vst3,
                        program_files_x86_vst3,
                    ]
                )

            custom_paths_to_check = [
                Path("C:/VSTPlugins/VST3"),
                Path("C:/VST3Plugins"),
                Path("D:/VSTPlugins/VST3"),
                Path("D:/VST3Plugins"),
            ]
            for cp in custom_paths_to_check:
                if cp.exists() and cp.is_dir():
                    if cp not in potential_paths:
                        potential_paths.append(cp)

        elif platform.system() == "Darwin":
            potential_paths = [
                Path("/Library/Audio/Plug-Ins/VST3"),
                Path.home() / "Library/Audio/Plug-Ins/VST3",
            ]
        else:
            potential_paths = [
                Path.home() / ".vst3",
                Path("/usr/lib/vst3"),
                Path("/usr/local/lib/vst3"),
                Path("/usr/lib/x86_64-linux-gnu/vst3"),
                Path("/usr/lib64/vst3"),
                Path.home() / ".local/lib/vst3",
                Path("/opt/vst3"),
                Path("/usr/share/vst3"),
            ]

        unique_ordered_paths = []
        seen_str_paths = set()
        for p in potential_paths:
            try:
                p_resolved = p.resolve()
                p_str = str(p_resolved)
                if p_str not in seen_str_paths:
                    unique_ordered_paths.append(p)
                    seen_str_paths.add(p_str)
            except Exception:
                if str(p) not in seen_str_paths:
                    unique_ordered_paths.append(p)
                    seen_str_paths.add(str(p))

        for path_to_check in unique_ordered_paths:
            exists = path_to_check.exists()
            is_dir = path_to_check.is_dir() if exists else False
            print(
                f"  Checking: {str(path_to_check)} (Exists: {exists}, IsDir: {is_dir})"
            )
            if exists and is_dir:
                print(f"  Found existing VST3 folder: {str(path_to_check)}")
                return str(path_to_check)

        default_vst_path_str = ""
        if platform.system() == "Windows":
            default_vst_path_str = str(
                program_files_vst3 if self.is_admin else appdata_vst3
            )
        elif platform.system() == "Darwin":
            default_vst_path_str = str(Path("/Library/Audio/Plug-Ins/VST3"))
        else:
            default_vst_path_str = str(Path.home() / ".vst3")

        print(
            f"  No suitable existing VST3 folder found. Defaulting to: {default_vst_path_str} (will be created if necessary)."
        )
        return default_vst_path_str

    def detect_vst_folder(self):
        if platform.system() == "Windows":
            common_paths = [
                Path.home() / "AppData/Roaming/VST3",
                Path("C:/Program Files/Common Files/VST3"),
                Path("C:/Program Files (x86)/Common Files/VST3"),
            ]
        elif platform.system() == "Darwin":
            common_paths = [
                Path.home() / "Library/Audio/Plug-Ins/VST3",
                Path("/Library/Audio/Plug-Ins/VST3"),
                Path("/System/Library/Audio/Plug-Ins/VST3"),
            ]
        else:
            common_paths = [
                Path.home() / ".vst3",
                Path("/usr/lib/vst3"),
                Path("/usr/local/lib/vst3"),
            ]

        for path in common_paths:
            if path.exists():
                return str(path)

        return str(
            Path.home() / "VST3"
            if platform.system() != "Windows"
            else Path.home() / "AppData/Roaming/VST3"
        )

    def check_python(self):
        try:
            if getattr(sys, "frozen", False):
                python_candidates = [
                    "python",
                    "python3",
                    "python.exe",
                    "C:\\Python311\\python.exe",
                    "C:\\Python310\\python.exe",
                ]

                for py_cmd in python_candidates:
                    try:
                        result = self.safe_subprocess_run(
                            [py_cmd, "--version"], capture_output=True, text=True
                        )
                        if result.returncode == 0:
                            return True
                    except FileNotFoundError:
                        continue
                return False
            else:
                result = self.safe_subprocess_run(
                    [sys.executable, "--version"], capture_output=True, text=True
                )
                if result.returncode == 0:
                    version = result.stdout.strip().split()[1]
                    major, minor = map(int, version.split(".")[:2])
                    return major >= 3 and minor >= 10
        except:
            pass
        return False

    def check_cmake(self):
        try:
            result = self.safe_subprocess_run(
                ["cmake", "--version"], capture_output=True, text=True
            )
            return result.returncode == 0
        except:
            return False

    def check_git(self):
        try:
            result = self.safe_subprocess_run(
                ["git", "--version"], capture_output=True, text=True
            )
            return result.returncode == 0
        except:
            return False

    def check_buildtools(self):
        if platform.system() != "Windows":
            return True

        try:
            vs_paths = [
                "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools",
                "C:/Program Files/Microsoft Visual Studio/2022/BuildTools",
                "C:/Program Files/Microsoft Visual Studio/2022/Community",
                "C:/Program Files/Microsoft Visual Studio/2022/Professional",
                "C:/Program Files/Microsoft Visual Studio/2022/Enterprise",
                "C:/Program Files (x86)/Microsoft Visual Studio/2019/BuildTools",
                "C:/Program Files/Microsoft Visual Studio/2019/BuildTools",
                "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community",
                "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional",
                "C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise",
                "C:/Program Files (x86)/Microsoft Visual Studio/2017/BuildTools",
                "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community",
                "C:/Program Files (x86)/Microsoft Visual Studio/2017/Professional",
                "C:/Program Files (x86)/Microsoft Visual Studio/2017/Enterprise",
            ]

            for path in vs_paths:
                if Path(path).exists():
                    msbuild_paths = [
                        Path(path) / "MSBuild/Current/Bin/MSBuild.exe",
                        Path(path) / "MSBuild/Current/Bin/amd64/MSBuild.exe",
                        Path(path) / "VC/Tools",
                    ]
                    if any(msbuild_path.exists() for msbuild_path in msbuild_paths):
                        return True

            return False

        except Exception:
            return False

    def get_system_info(self):
        info = {
            "os": f"{platform.system()} {platform.release()}",
            "cpu": platform.processor(),
            "cpu_cores": psutil.cpu_count(logical=False),
            "cpu_threads": psutil.cpu_count(logical=True),
            "ram_gb": round(psutil.virtual_memory().total / (1024**3), 1),
            "gpu": "None",
        }

        info["cuda_available"] = False
        info["rocm_available"] = False
        info["mps_available"] = False
        info["gpu_type"] = "cpu"
        info["gpu_list"] = []
        info["recommended_install"] = "cpu"

        if platform.system() == "Darwin":
            try:
                mac_version = platform.mac_ver()[0]
                if mac_version:
                    version_parts = mac_version.split(".")
                    major = int(version_parts[0])
                    minor = int(version_parts[1]) if len(version_parts) > 1 else 0

                    if major >= 13 or (major == 12 and minor >= 3):
                        result = self.safe_subprocess_run(
                            ["system_profiler", "SPDisplaysDataType"],
                            capture_output=True,
                            text=True,
                            timeout=15,
                        )

                        if result.returncode == 0:
                            gpu_info = result.stdout

                            apple_chips = ["M1", "M2", "M3", "M4", "M5", "Apple"]
                            for line in gpu_info.split("\n"):
                                if "Chipset Model:" in line:
                                    chipset = line.split(":")[1].strip()
                                    if any(chip in chipset for chip in apple_chips):
                                        info["gpu"] = chipset
                                        info["gpu_type"] = "apple_metal"
                                        info["mps_available"] = True
                                        info["recommended_install"] = "mps"

                                        unified_memory = int(
                                            info["ram_gb"] * 1024 * 0.75
                                        )
                                        info["gpu_list"].append(
                                            {
                                                "name": chipset,
                                                "memory_mb": unified_memory,
                                                "type": "apple_metal",
                                                "unified_memory": True,
                                            }
                                        )
                                        break

                            if not info["mps_available"]:
                                for line in gpu_info.split("\n"):
                                    if "Chipset Model:" in line:
                                        chipset = line.split(":")[1].strip()
                                        if any(
                                            vendor in chipset.lower()
                                            for vendor in [
                                                "intel",
                                                "amd",
                                                "nvidia",
                                                "radeon",
                                            ]
                                        ):
                                            vram_mb = 0
                                            for vram_line in gpu_info.split("\n"):
                                                if (
                                                    "VRAM" in vram_line
                                                    and "Total" in vram_line
                                                ):
                                                    vram_match = re.search(
                                                        r"(\d+)\s*(MB|GB)", vram_line
                                                    )
                                                    if vram_match:
                                                        vram_value = int(
                                                            vram_match.group(1)
                                                        )
                                                        if vram_match.group(2) == "GB":
                                                            vram_value *= 1024
                                                        vram_mb = vram_value
                                                        break

                                            info["gpu"] = (
                                                f"{chipset} ({vram_mb}MB)"
                                                if vram_mb > 0
                                                else chipset
                                            )
                                            info["gpu_type"] = "intel_metal"
                                            info["mps_available"] = True
                                            info["recommended_install"] = "mps"

                                            info["gpu_list"].append(
                                                {
                                                    "name": chipset,
                                                    "memory_mb": vram_mb,
                                                    "type": "intel_metal",
                                                    "unified_memory": False,
                                                }
                                            )
                                            break

                    try:
                        import torch

                        if (
                            hasattr(torch.backends, "mps")
                            and torch.backends.mps.is_available()
                        ):
                            if not info["mps_available"]:
                                info["mps_available"] = True
                                info["recommended_install"] = "mps"
                    except ImportError:
                        pass

                    else:
                        self.log(
                            f"macOS {mac_version} trop ancien pour MPS (nécessite 12.3+)",
                            "INFO",
                        )

            except Exception as e:
                self.log(f"Erreur détection macOS Metal: {e}", "WARNING")

        if not info["mps_available"]:
            try:
                gpus = GPUtil.getGPUs()
                if gpus:
                    gpu = gpus[0]
                    info["gpu"] = f"{gpu.name} ({gpu.memoryTotal}MB)"
                    info["cuda_available"] = True
                    info["gpu_type"] = "nvidia"
                    info["recommended_install"] = "cuda"
                    info["gpu_list"].append(
                        {
                            "name": gpu.name,
                            "memory_mb": gpu.memoryTotal,
                            "type": "nvidia",
                        }
                    )
            except Exception:
                pass

        if not info["cuda_available"] and not info["mps_available"]:
            amd_gpus = []

            try:
                result = self.safe_subprocess_run(
                    ["rocm-smi", "--showproductname"],
                    capture_output=True,
                    text=True,
                    timeout=5,
                )

                if result.returncode == 0:
                    lines = result.stdout.strip().split("\n")
                    for line in lines:
                        if "Card series:" in line or "GPU" in line:
                            gpu_name = line.split(":")[-1].strip()
                            if gpu_name and gpu_name != "N/A":
                                amd_gpus.append(
                                    {
                                        "name": gpu_name,
                                        "memory_mb": 0,
                                        "type": "amd",
                                        "detected_via": "rocm-smi",
                                    }
                                )
                                break
            except (
                subprocess.TimeoutExpired,
                subprocess.CalledProcessError,
                FileNotFoundError,
            ):
                pass

            if not amd_gpus:
                try:
                    if platform.system() == "Windows":
                        result = self.safe_subprocess_run(
                            [
                                "wmic",
                                "path",
                                "win32_VideoController",
                                "get",
                                "name,AdapterRAM",
                                "/format:csv",
                            ],
                            capture_output=True,
                            text=True,
                            timeout=10,
                        )

                        if result.returncode == 0:
                            lines = result.stdout.split("\n")
                            for line in lines:
                                if any(
                                    amd_keyword in line.lower()
                                    for amd_keyword in [
                                        "amd",
                                        "radeon",
                                        "rx ",
                                        "vega",
                                        "navi",
                                    ]
                                ):
                                    parts = line.split(",")
                                    if len(parts) >= 2:
                                        gpu_name = (
                                            parts[2].strip()
                                            if len(parts) > 2
                                            else "AMD GPU"
                                        )
                                        memory_bytes = (
                                            parts[1].strip() if len(parts) > 1 else "0"
                                        )

                                        try:
                                            memory_mb = (
                                                int(memory_bytes) // (1024 * 1024)
                                                if memory_bytes.isdigit()
                                                else 0
                                            )
                                        except:
                                            memory_mb = 0

                                        amd_gpus.append(
                                            {
                                                "name": gpu_name,
                                                "memory_mb": memory_mb,
                                                "type": "amd",
                                                "detected_via": "wmic",
                                            }
                                        )
                                        break

                    elif platform.system() in ["Linux", "Darwin"]:
                        result = self.safe_subprocess_run(
                            ["lspci", "-nn"], capture_output=True, text=True, timeout=5
                        )
                        if result.returncode == 0:
                            for line in result.stdout.split("\n"):
                                if re.search(
                                    r"VGA.*AMD|VGA.*ATI|VGA.*Radeon|Display.*AMD",
                                    line,
                                    re.IGNORECASE,
                                ):
                                    match = re.search(r":\s*(.+)", line)
                                    if match:
                                        gpu_name = match.group(1).strip()
                                        pci_match = re.search(
                                            r"\[([0-9a-f]{4}:[0-9a-f]{4})\]", line
                                        )
                                        if pci_match:
                                            gpu_name += f" [{pci_match.group(1)}]"

                                        amd_gpus.append(
                                            {
                                                "name": gpu_name,
                                                "memory_mb": 0,
                                                "type": "amd",
                                                "detected_via": "lspci",
                                            }
                                        )
                                        break
                except (
                    subprocess.TimeoutExpired,
                    subprocess.CalledProcessError,
                    FileNotFoundError,
                ):
                    pass

            if amd_gpus:
                info["gpu_type"] = "amd"
                info["gpu_list"] = amd_gpus
                info["gpu"] = amd_gpus[0]["name"]
                if amd_gpus[0]["memory_mb"] > 0:
                    info["gpu"] += f" ({amd_gpus[0]['memory_mb']}MB)"

                rocm_detected = False

                rocm_paths = [
                    "/opt/rocm",
                    "/usr/lib/x86_64-linux-gnu/rocm",
                    "/usr/lib64/rocm",
                    Path.home() / ".local/rocm",
                ]

                for path in rocm_paths:
                    if Path(path).exists():
                        rocm_detected = True
                        break

                if not rocm_detected:
                    for cmd in ["rocm-smi", "rocminfo", "hipcc"]:
                        try:
                            result = self.safe_subprocess_run(
                                [cmd, "--version"],
                                capture_output=True,
                                text=True,
                                timeout=5,
                            )
                            if result.returncode == 0:
                                rocm_detected = True
                                break
                        except:
                            continue

                if not rocm_detected:
                    rocm_env_vars = ["ROCM_PATH", "HIP_PATH", "ROCM_HOME", "ROCM_ROOT"]
                    for var in rocm_env_vars:
                        if os.environ.get(var):
                            rocm_detected = True
                            break

                info["rocm_available"] = rocm_detected
                if rocm_detected:
                    info["recommended_install"] = "rocm"

        if info["gpu_type"] == "cpu":
            try:
                if platform.system() == "Windows":
                    result = self.safe_subprocess_run(
                        ["wmic", "path", "win32_VideoController", "get", "name"],
                        capture_output=True,
                        text=True,
                        timeout=5,
                    )

                    if result.returncode == 0:
                        for line in result.stdout.split("\n"):
                            line_lower = line.lower()
                            if "intel" in line_lower and "arc" in line_lower:
                                info["gpu"] = line.strip()
                                info["gpu_type"] = "intel_arc"
                                break
                            elif "intel" in line_lower and any(
                                x in line_lower for x in ["xe", "iris", "uhd"]
                            ):
                                info["gpu"] = line.strip()
                                info["gpu_type"] = "intel_integrated"
                                break

                elif platform.system() in ["Linux", "Darwin"]:
                    result = self.safe_subprocess_run(
                        ["lspci"], capture_output=True, text=True, timeout=5
                    )
                    if result.returncode == 0:
                        for line in result.stdout.split("\n"):
                            if re.search(r"VGA.*Intel.*Arc", line, re.IGNORECASE):
                                match = re.search(r":\s*(.+)", line)
                                if match:
                                    info["gpu"] = match.group(1).strip()
                                    info["gpu_type"] = "intel_arc"
                                    break
                            elif re.search(
                                r"VGA.*Intel.*(UHD|Iris|Xe)", line, re.IGNORECASE
                            ):
                                match = re.search(r":\s*(.+)", line)
                                if match:
                                    info["gpu"] = match.group(1).strip()
                                    info["gpu_type"] = "intel_integrated"
                                    break
            except:
                pass

        info["cuda_installed"] = (
            self.check_cuda_installed() if info["cuda_available"] else False
        )

        if platform.system() == "Darwin" and info["mps_available"]:
            try:
                import torch

                if hasattr(torch.backends, "mps"):
                    info["mps_ready"] = torch.backends.mps.is_available()
                else:
                    info["mps_ready"] = False
            except ImportError:
                info["mps_ready"] = False

        return info

    def verify_mps_installation(self, python_path):
        test_script = """
    import torch
    import sys

    try:
        print(f"PyTorch version: {torch.__version__}")
        print(f"MPS available: {torch.backends.mps.is_available()}")
        
        if torch.backends.mps.is_available():
            # Test simple tensor MPS
            device = torch.device("mps")
            x = torch.randn(3, 3, device=device)
            y = torch.randn(3, 3, device=device)
            z = torch.mm(x, y)
            print(f"MPS test successful: {z.shape}")
            print("✅ Metal Performance Shaders ready!")
        else:
            print("❌ MPS not available")
            sys.exit(1)
            
    except Exception as e:
        print(f"❌ MPS test failed: {e}")
        sys.exit(1)
    """

        try:
            result = self.safe_subprocess_run(
                [str(python_path), "-c", test_script],
                capture_output=True,
                text=True,
                timeout=30,
            )

            if result.returncode == 0:
                self.log("✅ MPS verification successful!", "SUCCESS")
                for line in result.stdout.strip().split("\n"):
                    if line.strip():
                        self.log(f"   {line}")
            else:
                self.log("⚠️ MPS verification failed", "WARNING")
                self.log(f"Error: {result.stderr}", "WARNING")

        except Exception as e:
            self.log(f"Could not verify MPS: {e}", "WARNING")

    def log_system_info(self):
        self.log("🎵 OBSIDIAN-Neural Installer v1.0")
        self.log(f"🖥️ System: {self.system_info['os']}")
        self.log(f"💾 RAM: {self.system_info['ram_gb']} GB")

        if self.system_info.get("cuda_available"):
            self.log(f"🟢 NVIDIA GPU: {self.system_info['gpu']}")
            self.log("🎯 Recommendation: CUDA Installation")
        elif self.system_info.get("mps_available"):
            self.log(f"🍎 Apple GPU: {self.system_info['gpu']}")
            self.log("🎯 Recommendation: Metal Performance Shaders (MPS)")
            if self.system_info["gpu_type"] == "apple_metal":
                self.log("💡 Unified Memory: GPU partage la RAM système")
            else:
                self.log("💡 Metal support disponible")
        elif self.system_info.get("rocm_available"):
            self.log(f"🔴 AMD GPU: {self.system_info['gpu']}")
            self.log("🎯 Recommendation: ROCm Installation")
        elif self.system_info["gpu_type"] == "amd":
            self.log(f"🔴 AMD GPU detected: {self.system_info['gpu']}")
            self.log("⚠️ ROCm not installed")
            if platform.system() == "Darwin":
                self.log("💡 ROCm not available on macOS - using CPU")
            else:
                self.log("💡 Install ROCm for better performance")
        else:
            self.log("🎯 Recommendation: CPU Installation")

    def setup_ui(self):
        style = ttk.Style()
        if "clam" in style.theme_names():
            style.theme_use("clam")
        main_frame = ttk.Frame(self.root, padding="20")
        main_frame.pack(fill="both", expand=True)
        header_frame = ttk.Frame(main_frame)
        header_frame.pack(fill="x", pady=(0, 20))
        logo_path = Path(__file__).parent / "logo.png"
        if logo_path.exists() and PIL_AVAILABLE:
            try:
                from PIL import Image, ImageTk

                image = Image.open(logo_path)
                image = image.resize((64, 64), Image.Resampling.LANCZOS)
                self.logo_photo = ImageTk.PhotoImage(image)

                logo_title_frame = ttk.Frame(header_frame)
                logo_title_frame.pack()

                logo_label = ttk.Label(logo_title_frame, image=self.logo_photo)
                logo_label.pack(side="left", padx=(0, 15))

                titles_frame = ttk.Frame(logo_title_frame)
                titles_frame.pack(side="left")

                title_label = ttk.Label(
                    titles_frame, text="OBSIDIAN-Neural", font=("Arial", 20, "bold")
                )
                title_label.pack(anchor="w")

                subtitle_label = ttk.Label(
                    titles_frame,
                    text="Full Installer - Real-time AI music generation VST3 plugin for live performance",
                    font=("Arial", 12),
                )
                subtitle_label.pack(anchor="w")
            except ImportError:
                self._create_title_without_logo(header_frame)
        else:
            self._create_title_without_logo(header_frame)

        if not self.is_admin:
            admin_frame = ttk.Frame(main_frame)
            admin_frame.pack(fill="x", pady=(0, 10))

            if platform.system() == "Windows":
                warning_text = "⚠️ Administrator privileges recommended for VST3 system installation"
                detail_text = "(Required to install VST3 plugin to Program Files and system dependencies)"
                button_text = "Restart as administrator"
            elif platform.system() == "Darwin":
                warning_text = "⚠️ Administrator privileges recommended for system VST3 installation"
                detail_text = (
                    "(Required to install VST3 plugin to /Library/Audio/Plug-Ins/VST3)"
                )
                button_text = "Request administrator privileges"
            else:
                warning_text = (
                    "⚠️ Administrator privileges recommended for system installation"
                )
                detail_text = "(Required to install VST3 plugin to /usr/lib/vst3 and system dependencies)"
                button_text = "Restart with sudo"

            ttk.Label(
                admin_frame,
                text=warning_text,
                foreground="red",
                font=("Arial", 10, "bold"),
                wraplength=400,
            ).pack()

            ttk.Label(
                admin_frame,
                text=detail_text,
                foreground="gray",
                font=("Arial", 8),
                wraplength=400,
            ).pack(pady=(0, 5))

            ttk.Button(
                admin_frame,
                text=button_text,
                command=self.request_admin,
            ).pack(pady=5)

        canvas = tk.Canvas(main_frame, highlightthickness=0, bd=0)
        scrollbar = ttk.Scrollbar(main_frame, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)
        scrollable_frame.bind(
            "<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
        canvas_window = canvas.create_window(
            (0, 0), window=scrollable_frame, anchor="nw"
        )

        def configure_scroll_region(event=None):
            canvas.configure(scrollregion=canvas.bbox("all"))
            canvas.itemconfig(canvas_window, width=canvas.winfo_width())

        canvas.bind("<Configure>", configure_scroll_region)
        canvas.configure(yscrollcommand=scrollbar.set)
        content_frame = scrollable_frame
        sysinfo_frame = ttk.LabelFrame(
            content_frame, text="🖥️ System Information", padding="10"
        )
        sysinfo_frame.pack(fill="x", pady=(0, 20), padx=10)
        info_text = (
            f"OS: {self.system_info['os']}\n"
            f"CPU: {self.system_info['cpu']} ({self.system_info['cpu_cores']} cores, "
            f"{self.system_info['cpu_threads']} threads)\n"
            f"RAM: {self.system_info['ram_gb']} GB\n"
            f"GPU: {self.system_info['gpu']}"
        )
        ttk.Label(sysinfo_frame, text=info_text, font=("Consolas", 9)).pack(anchor="w")

        prereq_frame = ttk.LabelFrame(
            content_frame, text="🔧 Prerequisites Status", padding="10"
        )
        prereq_frame.pack(fill="x", pady=(0, 20), padx=10)

        prereq_options = [
            (self.install_python, "Python 3.10+", "main language", self.check_python()),
            (self.install_cmake, "CMake 4.0.2+", "compilation", self.check_cmake()),
            (self.install_git, "Git", "code management", self.check_git()),
            (
                self.install_buildtools,
                "Visual Studio Build Tools",
                "Windows compilation",
                self.check_buildtools() if platform.system() == "Windows" else True,
            ),
            (
                self.install_cuda,
                "CUDA Toolkit",
                "NVIDIA GPU acceleration",
                self.system_info.get("cuda_installed", False),
            ),
        ]

        for var, name, description, is_installed in prereq_options:
            item_frame = ttk.Frame(prereq_frame)
            item_frame.pack(fill="x", pady=2)

            if name == "CUDA Toolkit":
                if not self.system_info.get("cuda_available", False):
                    var.set(False)
                    cb = ttk.Checkbutton(item_frame, variable=var, state="disabled")
                    cb.pack(side="left")

                    gpu_type = self.system_info.get("gpu_type", "cpu")
                    if gpu_type == "cpu":
                        reason = "No dedicated GPU detected"
                        color = "gray"
                        status_text = f"❌ {name} - {reason}"
                    elif gpu_type == "amd":
                        reason = "AMD GPU detected (use ROCm instead)"
                        color = "orange"
                        status_text = f"❌ {name} - {reason}"
                    elif gpu_type == "intel":
                        reason = "Intel GPU detected (not supported)"
                        color = "gray"
                        status_text = f"❌ {name} - {reason}"
                    else:
                        reason = "NVIDIA GPU not detected"
                        color = "gray"
                        status_text = f"❌ {name} - {reason}"

                else:
                    suggested = not is_installed
                    var.set(suggested)
                    cb = ttk.Checkbutton(item_frame, variable=var)
                    cb.pack(side="left")

                    if is_installed:
                        status_text = f"✅ {name} - Already installed"
                        color = "dark green"
                    else:
                        status_text = f"📦 {name} - Will be installed"
                        color = "dark orange"
            else:
                suggested = not is_installed
                var.set(suggested)
                cb = ttk.Checkbutton(item_frame, variable=var)
                cb.pack(side="left")

                if is_installed:
                    status_text = f"✅ {name} - Already installed"
                    color = "dark green"
                else:
                    status_text = f"📦 {name} - Will be installed"
                    color = "dark orange"

            status_label = ttk.Label(item_frame, text=status_text, foreground=color)
            status_label.pack(side="left", padx=(5, 0))

            desc_label = ttk.Label(
                item_frame,
                text=f"({description})",
                font=("Arial", 8),
                foreground="gray",
            )
            desc_label.pack(side="left", padx=(10, 0))

        paths_frame = ttk.LabelFrame(
            content_frame, text="📁 Installation Paths", padding="10"
        )
        paths_frame.pack(fill="x", pady=(0, 20), padx=10)

        ttk.Label(paths_frame, text="Main file:").pack(anchor="w")
        path_frame1 = ttk.Frame(paths_frame)
        path_frame1.pack(fill="x", pady=(0, 10))
        ttk.Entry(path_frame1, textvariable=self.install_path, width=50).pack(
            side="left", fill="x", expand=True
        )
        ttk.Button(
            path_frame1,
            text="Browse",
            command=lambda: self.browse_folder(self.install_path),
        ).pack(side="right", padx=(5, 0))

        ttk.Label(paths_frame, text="VST3 File:").pack(anchor="w")
        path_frame2 = ttk.Frame(paths_frame)
        path_frame2.pack(fill="x")
        ttk.Entry(path_frame2, textvariable=self.vst_path, width=50).pack(
            side="left", fill="x", expand=True
        )
        ttk.Button(
            path_frame2,
            text="Browse",
            command=lambda: self.browse_folder(self.vst_path),
        ).pack(side="right", padx=(5, 0))

        options_frame = ttk.LabelFrame(content_frame, text="⚙️ Options", padding="10")
        options_frame.pack(fill="x", pady=(0, 20), padx=10)

        ttk.Checkbutton(
            options_frame,
            text="🧪 Run a performance benchmark",
            variable=self.run_benchmark,
        ).pack(anchor="w")
        ttk.Checkbutton(
            options_frame,
            text="📋 Automatically copy VST",
            variable=self.auto_copy_vst,
        ).pack(anchor="w")
        ttk.Checkbutton(
            options_frame,
            text="🚀 Start the server after installation",
            variable=self.start_after_install,
        ).pack(anchor="w")
        ttk.Checkbutton(
            options_frame,
            text="⬇️ Skip VST compilation (manual download required)",
            variable=self.skip_vst_build,
        ).pack(anchor="w")
        info_label = ttk.Label(
            options_frame,
            text="      💡 You'll need to manually download the VST from releases",
            font=("Arial", 8),
            foreground="gray",
        )
        info_label.pack(anchor="w", padx=(20, 0))
        progress_frame = ttk.LabelFrame(content_frame, text="📊 Progress", padding="10")
        progress_frame.pack(fill="x", pady=(0, 20), padx=10)

        self.progress_bar = ttk.Progressbar(
            progress_frame, variable=self.progress_var, maximum=100
        )
        self.progress_bar.pack(fill="x", pady=(0, 5))

        status_label = ttk.Label(progress_frame, textvariable=self.status_var)
        status_label.pack()

        log_frame = ttk.LabelFrame(
            content_frame, text="📝 Installation Log", padding="5"
        )
        log_frame.pack(fill="both", expand=True, pady=(0, 15), padx=10)

        self.log_text = scrolledtext.ScrolledText(
            log_frame, height=8, font=("Consolas", 9)
        )
        self.log_text.pack(fill="both", expand=True)

        button_frame = ttk.Frame(content_frame)
        button_frame.pack(fill="x", padx=10, pady=(0, 10))

        self.install_button = ttk.Button(
            button_frame,
            text="🚀 Install OBSIDIAN-Neural",
            command=self.start_installation,
        )
        self.install_button.pack(side="left", padx=(0, 10))

        self.cancel_button = ttk.Button(
            button_frame, text="❌ Cancel", command=self.root.quit
        )
        self.cancel_button.pack(side="left")

        def _on_mousewheel(event):
            canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")

        def _bind_mousewheel(event):
            canvas.bind_all("<MouseWheel>", _on_mousewheel)

        def _unbind_mousewheel(event):
            canvas.unbind_all("<MouseWheel>")

        canvas.bind("<Enter>", _bind_mousewheel)
        canvas.bind("<Leave>", _unbind_mousewheel)

        def _unbind_global_scroll(event):
            canvas.unbind_all("<MouseWheel>")

        def _rebind_global_scroll(event):
            canvas.bind_all("<MouseWheel>", _on_mousewheel)

        self.log_text.bind("<Enter>", _unbind_global_scroll)
        self.log_text.bind("<Leave>", _rebind_global_scroll)

    def check_cuda_installed(self):
        try:
            result = self.safe_subprocess_run(
                ["nvcc", "--version"], capture_output=True, text=True
            )
            if result.returncode == 0:
                return True

            cuda_paths = [
                "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA",
                "C:/Program Files (x86)/NVIDIA GPU Computing Toolkit/CUDA",
            ]
            for path in cuda_paths:
                if Path(path).exists():
                    return True

        except:
            pass

        return False

    def _create_title_without_logo(self, header_frame):
        title_label = ttk.Label(
            header_frame, text="OBSIDIAN-Neural", font=("Arial", 20, "bold")
        )
        title_label.pack()

        subtitle_label = ttk.Label(
            header_frame,
            text="Full Installer - Real-time AI music generation VST3 plugin for live performance",
            font=("Arial", 12),
        )
        subtitle_label.pack()

    def browse_folder(self, var):
        folder = filedialog.askdirectory(initialdir=var.get())
        if folder:
            var.set(folder)

    def log(self, message, level="INFO"):
        timestamp = time.strftime("%H:%M:%S")
        emoji = {"INFO": "ℹ️", "SUCCESS": "✅", "WARNING": "⚠️", "ERROR": "❌"}.get(
            level, "ℹ️"
        )
        formatted_msg = f"[{timestamp}] {emoji} {message}\n"

        if hasattr(self, "log_text") and self.log_text:
            self.log_text.insert(tk.END, formatted_msg)
            self.log_text.see(tk.END)
            if hasattr(self, "root") and self.root:
                self.root.update()
        else:
            print(f"LOG (UI not ready): {formatted_msg.strip()}")

    def update_progress(self, value, status=""):
        self.progress_var.set(value)
        if status:
            self.status_var.set(status)
        self.root.update()

    def start_installation(self):
        install_dir = Path(self.install_path.get())
        confirmation = self.confirm_installation_path(install_dir)

        if not confirmation["confirmed"]:
            if confirmation["new_path"]:
                self.install_path.set(confirmation["new_path"])
                self.start_installation()
            else:
                self.log("Installation cancelled by user", "INFO")
            return

        self.log(f"User confirmed installation to: {install_dir}")
        self.log("User accepted responsibility for installation path")

        self.install_button.config(state="disabled")
        thread = threading.Thread(target=self.install_process)
        thread.start()

    def install_process(self):
        try:
            install_dir = Path(self.install_path.get())
            install_dir.mkdir(parents=True, exist_ok=True)
            steps = []
            if self.install_python.get():
                steps.append(("Installing Python 3.10", self.install_python_func))
            if self.install_cmake.get():
                steps.append(("Installing CMake", self.install_cmake_func))
            if self.install_git.get():
                steps.append(("Installing Git", self.install_git_func))
            if self.install_buildtools.get():
                steps.append(
                    ("Installing the Build Tools", self.install_buildtools_func)
                )
            if self.install_cuda.get():
                steps.append(("Installing CUDA Toolkit", self.install_cuda_func))

            steps.extend(
                [
                    ("Downloading the source code", self.download_source),
                    ("Creating the Python Environment", self.create_venv),
                    ("Installing Python Dependencies", self.install_python_deps),
                    (
                        "Server setup & desktop shortcut",
                        self.create_server_executable_and_shortcut,
                    ),
                    ("AI GGUF Models Download", self.download_model),
                    ("VST Setup", self.setup_vst),
                    ("Environment configuration", self.setup_environment),
                ]
            )

            if self.auto_copy_vst.get() and not self.skip_vst_build.get():
                steps.append(("Installing the VST plugin", self.install_vst))

            if self.run_benchmark.get():
                steps.append(("Performance benchmark", self.run_benchmark_func))

            for i, (step_name, step_func) in enumerate(steps):
                progress = (i / len(steps)) * 100
                self.update_progress(progress, f"Étape {i+1}/{len(steps)}: {step_name}")
                self.log(f"Startup: {step_name}")

                try:
                    step_func(install_dir)
                    self.log(f"Finished: {step_name}", "SUCCESS")
                except Exception as e:
                    self.log(f"Error during {step_name}: {str(e)}", "ERROR")
                    raise
            self.create_installation_registry(install_dir=install_dir)
            self.update_progress(100, "Installation completed successfully!")
            self.log(
                "🎉 OBSIDIAN-Neural installation completed successfully!", "SUCCESS"
            )
            self.cancel_button.config(text="✅ Close")

            if self.start_after_install.get():
                self.start_server(install_dir)

            messagebox.showinfo(
                "Success",
                "Installation completed successfully!\n\n"
                "Check the log for configuration details.",
            )

        except Exception as e:
            self.log(f"Installation failed: {str(e)}", "ERROR")
            messagebox.showerror("Error", f"Installation failed:\n{str(e)}")
        finally:
            self.install_button.config(state="normal")

    def setup_vst(self, install_dir):
        if self.skip_vst_build.get():
            self.skip_vst_manual(install_dir)
        else:
            self.build_vst(install_dir)

    def create_server_executable_and_shortcut(self, install_dir):
        self.log("Creating desktop shortcut...")

        if platform.system() == "Darwin":
            exe_name = "OBSIDIAN-Neural-Server-macos"
            exe_path = install_dir / "bin" / exe_name

            if exe_path.exists():
                exe_path.chmod(0o755)
                desktop = Path.home() / "Desktop"
                self.create_macos_shortcut(exe_path, desktop, install_dir)
                self.log("macOS server executable shortcut created!", "SUCCESS")
                return

        elif platform.system() == "Linux":
            exe_name = "OBSIDIAN-Neural-Server-linux"
            exe_path = install_dir / "bin" / exe_name

            if exe_path.exists():
                exe_path.chmod(0o755)
                desktop = Path.home() / "Desktop"
                self.create_linux_shortcut(exe_path, desktop, install_dir)
                self.log("Linux server executable shortcut created!", "SUCCESS")
                return

        self.create_python_shortcut(install_dir)

    def create_python_shortcut(self, install_dir):

        desktop = Path.home() / "Desktop"

        if platform.system() == "Windows":
            python_path = install_dir / "env" / "Scripts" / "python.exe"
            script_path = install_dir / "server_interface.py"

            shortcut_path = desktop / "OBSIDIAN-Neural Server.lnk"
            ico_path = install_dir / "logo.ico"

            icon_line = (
                f'$Shortcut.IconLocation = "{ico_path}"' if ico_path.exists() else ""
            )

            ps_script = f"""
            $WshShell = New-Object -comObject WScript.Shell
            $Shortcut = $WshShell.CreateShortcut("{shortcut_path}")
            $Shortcut.TargetPath = "{python_path}"
            $Shortcut.Arguments = "server_interface.py"
            $Shortcut.WorkingDirectory = "{install_dir}"
            $Shortcut.Description = "OBSIDIAN-Neural AI Music Generation Server Interface"
            {icon_line}
            $Shortcut.Save()
            """

            cmd = ["powershell", "-Command", ps_script]
            result = self.safe_subprocess_run(cmd, capture_output=True, text=True)

            if result.returncode == 0:
                self.log("✅ Windows Python shortcut created")
            else:
                self.log(
                    f"Could not create Windows shortcut: {result.stderr}", "WARNING"
                )

        elif platform.system() == "Darwin":
            python_path = install_dir / "env" / "bin" / "python"
            script_path = install_dir / "server_interface.py"

            shortcut_path = desktop / "OBSIDIAN-Neural Server.command"

            script_content = f"""#!/bin/bash
    cd "{install_dir}"
    exec "{python_path}" server_interface.py
    """

            try:
                shortcut_path.write_text(script_content)
                shortcut_path.chmod(0o755)
                self.log("✅ macOS Python shortcut created")
            except Exception as e:
                self.log(f"Could not create macOS shortcut: {e}", "WARNING")

        else:
            python_path = install_dir / "env" / "bin" / "python"
            script_path = install_dir / "server_interface.py"

            shortcut_path = desktop / "obsidian-neural-server.desktop"
            logo_path = install_dir / "logo.png"

            icon_line = f"Icon={logo_path}" if logo_path.exists() else ""

            desktop_content = f"""[Desktop Entry]
    Version=1.0
    Name=OBSIDIAN-Neural Server
    Comment=AI Music Generation Server Interface
    Exec={python_path} server_interface.py
    Path={install_dir}
    {icon_line}
    Terminal=false
    Type=Application
    Categories=AudioVideo;Audio;
    """

            try:
                shortcut_path.write_text(desktop_content)
                shortcut_path.chmod(0o755)
                self.log("✅ Linux Python shortcut created")
            except Exception as e:
                self.log(f"Could not create Linux shortcut: {e}", "WARNING")

    def create_windows_shortcut(self, exe_path, desktop, install_dir):
        shortcut_path = desktop / "OBSIDIAN-Neural Server.lnk"
        ico_path = install_dir / "logo.ico"

        icon_line = (
            f'$Shortcut.IconLocation = "{ico_path}"' if ico_path.exists() else ""
        )

        ps_script = f"""
        $WshShell = New-Object -comObject WScript.Shell
        $Shortcut = $WshShell.CreateShortcut("{shortcut_path}")
        $Shortcut.TargetPath = "{exe_path}"
        $Shortcut.WorkingDirectory = "{install_dir}"
        $Shortcut.Description = "OBSIDIAN-Neural AI Music Generation Server"
        {icon_line}
        $Shortcut.Save()
        """

        cmd = ["powershell", "-Command", ps_script]
        result = self.safe_subprocess_run(cmd, capture_output=True, text=True)

        if result.returncode == 0:
            self.log("✅ Windows desktop shortcut created")
        else:
            self.log(f"Could not create Windows shortcut: {result.stderr}", "WARNING")

    def create_macos_shortcut(self, exe_path, desktop, install_dir):
        shortcut_path = desktop / "OBSIDIAN-Neural Server.command"

        script_content = f"""#!/bin/bash
    cd "{install_dir}"
    exec "{exe_path}"
    """

        try:
            shortcut_path.write_text(script_content)
            shortcut_path.chmod(0o755)
            self.log("✅ macOS desktop shortcut created")
        except Exception as e:
            self.log(f"Could not create macOS shortcut: {e}", "WARNING")

    def create_linux_shortcut(self, exe_path, desktop, install_dir):
        shortcut_path = desktop / "obsidian-neural-server.desktop"
        logo_path = install_dir / "logo.png"

        icon_line = f"Icon={logo_path}" if logo_path.exists() else ""

        desktop_content = f"""[Desktop Entry]
    Version=1.0
    Name=OBSIDIAN-Neural Server
    Comment=AI Music Generation Server
    Exec={exe_path}
    Path={install_dir}
    {icon_line}
    Terminal=false
    Type=Application
    Categories=AudioVideo;Audio;
    """

        try:
            shortcut_path.write_text(desktop_content)
            shortcut_path.chmod(0o755)
            self.log("✅ Linux desktop shortcut created")
        except Exception as e:
            self.log(f"Could not create Linux shortcut: {e}", "WARNING")

    def skip_vst_manual(self, install_dir):
        self.log("VST build skipped - manual download required")
        self.log("=" * 60)
        self.log("📋 MANUAL VST INSTALLATION REQUIRED", "WARNING")
        self.log("=" * 60)
        self.log("1. Go to: https://github.com/innermost47/ai-dj/releases")
        self.log("2. Download the latest .vst3 file")
        self.log("3. Extract/copy it to your VST3 folder:")
        self.log(f"   → {self.vst_path.get()}")
        self.log("=" * 60)
        self.log("💡 The installer will continue without VST compilation")

        vst_build_dir = install_dir / "vst" / "build"
        vst_build_dir.mkdir(parents=True, exist_ok=True)

        desktop = Path.home() / "Desktop"
        readme_content = f"""OBSIDIAN-Neural VST - MANUAL DOWNLOAD REQUIRED

    The VST3 plugin compilation was skipped during installation.
    Please follow these steps to install the VST3 plugin:

    1. Visit: https://github.com/innermost47/ai-dj/releases
    2. Download the latest .vst3 file from the releases
    3. Copy it to your VST3 folder:
    {self.vst_path.get()}

    The server will work without the VST, but you'll need the VST 
    for DAW integration (FL Studio, Ableton, etc.).

    Installation completed at: {install_dir}
    """

        readme_file = desktop / "OBSIDIAN-Neural_VST_Instructions.txt"
        readme_file.write_text(readme_content)

        self.log(f"📄 Instructions saved to desktop: {readme_file}")
        self.log("💡 Check your desktop for VST installation instructions")

    def run_benchmark_func(self, install_dir):
        self.log("🧪 Starting the performance benchmark...")
        import json
        import math

        self.log("⚡ CPU test in progress...")
        start_time = time.time()

        result = 0
        iterations = 2000000
        for i in range(iterations):
            result += math.sqrt(i) * math.sin(i / 1000)

        cpu_time = time.time() - start_time
        cpu_score = max(1, min(100, int(100 / max(cpu_time, 0.1))))

        self.log(f"⏱️ CPU time: {cpu_time:.2f}s")

        self.log("💾 Memory test in progress...")
        memory_info = psutil.virtual_memory()
        available_gb = memory_info.available / (1024**3)
        total_gb = memory_info.total / (1024**3)

        memory_score = min(100, int(available_gb * 10))

        self.log(f"💾 Available RAM: {available_gb:.1f}GB / {total_gb:.1f}GB")

        self.log("🎮 GPU testing in progress...")
        gpu_score = 0
        gpu_info = "No GPU detected"
        gpu_test_details = {}

        if self.system_info.get("mps_available"):
            try:
                self.log("🍎 Testing Apple Metal Performance Shaders...")
                import torch

                if torch.backends.mps.is_available():
                    device = torch.device("mps")
                    start_time = time.time()

                    matrix_size = 1024
                    iterations = 20

                    self.log(
                        f"   Running {iterations} matrix operations ({matrix_size}x{matrix_size})..."
                    )

                    x = torch.randn(matrix_size, matrix_size, device=device)
                    y = torch.randn(matrix_size, matrix_size, device=device)

                    for i in range(iterations):
                        z = torch.mm(x, y)
                        if i % 5 == 0:
                            z = torch.matmul(z, x.transpose(0, 1))

                    torch.mps.synchronize()
                    mps_time = time.time() - start_time

                    if mps_time < 0.2:
                        gpu_score = 95
                    elif mps_time < 0.3:
                        gpu_score = 85
                    elif mps_time < 0.5:
                        gpu_score = 75
                    elif mps_time < 0.8:
                        gpu_score = 65
                    elif mps_time < 1.2:
                        gpu_score = 50
                    else:
                        gpu_score = 35

                    if self.system_info["gpu_type"] == "apple_metal":
                        unified_gb = self.system_info["ram_gb"]
                        gpu_info = (
                            f"{self.system_info['gpu']} (Unified: {unified_gb}GB)"
                        )
                    else:
                        gpu_info = f"{self.system_info['gpu']} (Metal)"

                    gpu_test_details = {
                        "type": "mps",
                        "test_time": f"{mps_time:.3f}s",
                        "operations": f"{iterations} matrix ops",
                        "device": str(device),
                    }

                    self.log(f"🍎 MPS test completed: {mps_time:.2f}s")

                else:
                    self.log("⚠️ MPS not available despite detection", "WARNING")
                    gpu_score = 20
                    gpu_info = "MPS unavailable"

            except Exception as e:
                self.log(f"❌ MPS benchmark error: {e}", "WARNING")
                gpu_score = 15
                gpu_info = "MPS test failed"

        elif self.system_info.get("cuda_available"):
            try:
                self.log("🟢 Testing NVIDIA CUDA GPU...")
                gpus = GPUtil.getGPUs()
                if gpus:
                    gpu = gpus[0]
                    gpu_memory_gb = gpu.memoryTotal / 1024

                    if gpu_memory_gb >= 24:
                        gpu_score = 100
                    elif gpu_memory_gb >= 16:
                        gpu_score = 90
                    elif gpu_memory_gb >= 12:
                        gpu_score = 80
                    elif gpu_memory_gb >= 8:
                        gpu_score = 70
                    elif gpu_memory_gb >= 6:
                        gpu_score = 60
                    elif gpu_memory_gb >= 4:
                        gpu_score = 45
                    else:
                        gpu_score = 30

                    gpu_info = f"{gpu.name} ({gpu.memoryTotal}MB VRAM)"

                    try:
                        import torch

                        if torch.cuda.is_available():
                            device = torch.device("cuda")
                            start_time = time.time()

                            x = torch.randn(1000, 1000, device=device)
                            y = torch.randn(1000, 1000, device=device)
                            for _ in range(10):
                                z = torch.mm(x, y)
                            torch.cuda.synchronize()

                            cuda_time = time.time() - start_time
                            gpu_test_details = {
                                "type": "cuda",
                                "test_time": f"{cuda_time:.3f}s",
                                "vram_gb": f"{gpu_memory_gb:.1f}",
                                "utilization": (
                                    f"{gpu.load * 100:.1f}%"
                                    if hasattr(gpu, "load")
                                    else "N/A"
                                ),
                            }

                            self.log(f"🟢 CUDA test: {cuda_time:.2f}s")

                    except ImportError:
                        self.log("PyTorch not available for CUDA test")

                    try:
                        gpu_usage = gpu.load * 100
                        self.log(f"🎮 Current GPU usage: {gpu_usage:.1f}%")
                    except:
                        pass
                else:
                    gpu_score = 0
                    gpu_info = "NVIDIA GPU not accessible"

            except Exception as e:
                self.log(f"⚠️ NVIDIA GPU test error: {e}", "WARNING")
                gpu_score = 0

        elif self.system_info.get("rocm_available"):
            try:
                self.log("🔴 Testing AMD ROCm GPU...")

                gpu_name = self.system_info.get("gpu", "").lower()

                if any(model in gpu_name for model in ["7900", "6900", "6950"]):
                    gpu_score = 85
                elif any(model in gpu_name for model in ["7800", "6800", "6700"]):
                    gpu_score = 75
                elif any(model in gpu_name for model in ["7600", "6600", "5700"]):
                    gpu_score = 65
                elif any(model in gpu_name for model in ["6500", "5600", "5500"]):
                    gpu_score = 50
                else:
                    gpu_score = 40

                gpu_info = self.system_info["gpu"]

                try:
                    import torch

                    if hasattr(torch.version, "hip") and torch.version.hip:
                        self.log("🔴 ROCm PyTorch detected")
                        try:
                            device = torch.device("cuda")
                            x = torch.randn(500, 500, device=device)
                            y = torch.randn(500, 500, device=device)
                            start_time = time.time()
                            z = torch.mm(x, y)
                            rocm_time = time.time() - start_time

                            gpu_test_details = {
                                "type": "rocm",
                                "test_time": f"{rocm_time:.3f}s",
                                "framework": "PyTorch ROCm",
                            }

                            self.log(f"🔴 ROCm test: {rocm_time:.2f}s")
                        except:
                            self.log("ROCm device not accessible via PyTorch")

                except ImportError:
                    self.log("PyTorch not available for ROCm test")

            except Exception as e:
                self.log(f"⚠️ ROCm test error: {e}", "WARNING")

        elif self.system_info.get("gpu_type") == "amd":
            self.log("🔴 AMD GPU detected without ROCm")
            gpu_score = 25
            gpu_info = f"{self.system_info['gpu']} (No ROCm)"

        elif self.system_info.get("gpu_type") in ["intel_arc", "intel_integrated"]:
            if self.system_info.get("gpu_type") == "intel_arc":
                self.log("⚡ Intel Arc GPU detected")
                gpu_score = 45
                gpu_info = f"{self.system_info['gpu']} (Limited AI support)"
            else:
                self.log("💻 Intel integrated GPU detected")
                gpu_score = 20
                gpu_info = f"{self.system_info['gpu']} (Integrated)"

        else:
            gpu_score = 0
            gpu_info = "No GPU detected"

        self.log(f"🎮 GPU: {gpu_info}")

        self.log("💿 Storage test in progress...")
        try:
            disk_usage = psutil.disk_usage(install_dir)
            free_gb = disk_usage.free / (1024**3)
            total_gb = disk_usage.total / (1024**3)
            storage_score = min(100, int(free_gb / 2))

            self.log(f"💿 Free storage: {free_gb:.1f}GB / {total_gb:.1f}GB")
        except:
            storage_score = 50
            self.log("💿 Storage: Unable to determine")

        weights = {
            "cpu": 0.3,
            "memory": 0.25,
            "gpu": 0.35,
            "storage": 0.1,
        }

        global_score = int(
            cpu_score * weights["cpu"]
            + memory_score * weights["memory"]
            + gpu_score * weights["gpu"]
            + storage_score * weights["storage"]
        )

        if global_score >= 85:
            performance_level = "🚀 Excellent"
            perf_color = "SUCCESS"
        elif global_score >= 70:
            performance_level = "✅ Very good"
            perf_color = "SUCCESS"
        elif global_score >= 55:
            performance_level = "⚡ Good"
            perf_color = "INFO"
        elif global_score >= 40:
            performance_level = "⚠️ Correct"
            perf_color = "WARNING"
        else:
            performance_level = "❌ Insufficient"
            perf_color = "ERROR"

        recommendations = []

        if cpu_score < 50:
            recommendations.append(
                "🔧 Slow CPU - Consider a more powerful processor (Intel i5+ or AMD Ryzen 5+)"
            )

        if memory_score < 60:
            recommendations.append(
                "💾 Insufficient RAM - 16GB+ recommended for large AI models"
            )

        if gpu_score == 0:
            if platform.system() == "Darwin":
                recommendations.append(
                    "🍎 No GPU acceleration - Consider Mac with Apple Silicon (M1/M2/M3)"
                )
            else:
                recommendations.append(
                    "🎮 No dedicated GPU detected - RTX 3060+ or RX 6600+ highly recommended"
                )
        elif gpu_score < 30:
            recommendations.append(
                "🎮 Low-end GPU detected - Expect very long generation times (20+ minutes)"
            )
        elif gpu_score < 50:
            if self.system_info.get("mps_available"):
                recommendations.append(
                    "🍎 Entry-level Mac GPU - Generation will be slow (5-15 minutes)"
                )
            else:
                recommendations.append(
                    "🎮 Entry-level GPU - Generation will be slow (5-15 minutes)"
                )
        elif gpu_score < 70:
            recommendations.append(
                "🎮 Good GPU - Decent generation times (1-5 minutes)"
            )
        else:
            if self.system_info.get("mps_available"):
                recommendations.append(
                    "🍎 Excellent Apple Silicon - Fast generation times"
                )
            else:
                recommendations.append("🎮 Excellent GPU - Fast generation times")

        if self.system_info.get("gpu_type") == "amd" and not self.system_info.get(
            "rocm_available"
        ):
            recommendations.append(
                "🔴 Install ROCm for AMD GPU acceleration (Linux recommended)"
            )

        if platform.system() == "Darwin" and not self.system_info.get("mps_available"):
            recommendations.append(
                "🍎 Consider upgrading to macOS 12.3+ for Metal Performance Shaders"
            )

        if global_score < 70:
            recommendations.append(
                "⚡ Performance limited - Consider upgrading GPU for faster audio generation"
            )
        else:
            recommendations.append(
                "🎉 Good configuration for OBSIDIAN-Neural audio generation!"
            )

        if self.system_info.get("mps_available") and gpu_score >= 80:
            perf_estimate = (
                "Audio generation: 1-3 minutes for 10sec sample (Apple Silicon M2 Pro+)"
            )
        elif self.system_info.get("mps_available") and gpu_score >= 60:
            perf_estimate = (
                "Audio generation: 3-8 minutes for 10sec sample (Apple Silicon M1/M2)"
            )
        elif global_score >= 90:
            perf_estimate = (
                "Audio generation: 30-60 seconds for 10sec sample (High-end RTX 4080+)"
            )
        elif global_score >= 80:
            perf_estimate = (
                "Audio generation: 1-2 minutes for 10sec sample (RTX 3070/4070 class)"
            )
        elif global_score >= 70:
            perf_estimate = (
                "Audio generation: 2-5 minutes for 10sec sample (RTX 3060/laptop RTX)"
            )
        elif global_score >= 60:
            perf_estimate = (
                "Audio generation: 5-15 minutes for 10sec sample (GTX 1660/lower RTX)"
            )
        elif global_score >= 40:
            perf_estimate = (
                "Audio generation: 15-30 minutes for 10sec sample (Integrated/old GPU)"
            )
        else:
            perf_estimate = (
                "Audio generation: 30+ minutes for 10sec sample (CPU only - very slow)"
            )

        benchmark_results = {
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "system_info": self.system_info,
            "scores": {
                "cpu": cpu_score,
                "memory": memory_score,
                "gpu": gpu_score,
                "storage": storage_score,
                "global": global_score,
            },
            "performance_level": performance_level,
            "performance_estimate": perf_estimate,
            "recommendations": recommendations,
            "hardware_details": {
                "cpu_time": f"{cpu_time:.2f}s",
                "available_ram_gb": f"{available_gb:.1f}",
                "gpu_info": gpu_info,
                "gpu_test_details": gpu_test_details,
                "free_storage_gb": (
                    f"{free_gb:.1f}" if "free_gb" in locals() else "Unknown"
                ),
            },
        }

        benchmark_file = install_dir / "benchmark_results.json"
        with open(benchmark_file, "w", encoding="utf-8") as f:
            json.dump(benchmark_results, f, indent=2, ensure_ascii=False)

        self.log("=" * 50)
        self.log("📊 BENCHMARK RESULTS", "SUCCESS")
        self.log("=" * 50)
        self.log(f"🔥 CPU Score: {cpu_score}/100")
        self.log(f"💾 Memory Score: {memory_score}/100")
        self.log(f"🎮 GPU Score: {gpu_score}/100")
        self.log(f"💿 Storage Score: {storage_score}/100")
        self.log("=" * 30)
        self.log(f"🎯 Global Score: {global_score}/100", perf_color)
        self.log(f"⭐ Performance: {performance_level}", perf_color)
        self.log(f"⚡ Estimate: {perf_estimate}")
        self.log("=" * 30)

        self.log("💡 RECOMMENDATIONS:")
        for i, rec in enumerate(recommendations, 1):
            self.log(f" {i}. {rec}")

        self.log("=" * 50)
        self.log(f"📋 Saved report: {benchmark_file}")

        return benchmark_results

    def is_python_installed_registry(self):
        if platform.system() != "Windows":
            return False
        try:
            import winreg

            key = winreg.OpenKey(
                winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Python\PythonCore\3.10"
            )
            winreg.CloseKey(key)
            return True
        except:
            return False

    def install_python_func(self, install_dir):
        if self.check_python():
            self.log("Python 3.10+ already installed, skipping installation", "SUCCESS")
            return
        if self.is_python_installed_registry():
            self.log("Python detected in registry, skipping installation", "SUCCESS")
            return
        if platform.system() == "Windows":
            self.log("Downloading Python 3.10...")
            python_url = (
                "https://www.python.org/ftp/python/3.10.11/python-3.10.11-amd64.exe"
            )
            python_installer = install_dir / "python_installer.exe"
            urllib.request.urlretrieve(python_url, python_installer)

            self.log("Installing Python...")
            cmd = [
                str(python_installer),
                "/passive",
                "InstallAllUsers=0",
                "PrependPath=1",
                "Include_test=0",
            ]

            creation_flags = 0
            if getattr(sys, "frozen", False) and sys.platform == "win32":
                creation_flags = subprocess.CREATE_NO_WINDOW

            if sys.platform == "win32":
                result = subprocess.run(cmd, creationflags=creation_flags)
            else:
                result = subprocess.run(cmd)

            if result.returncode != 0:
                self.log(
                    f"Python installation failed with code {result.returncode}",
                    "WARNING",
                )
                self.log("Trying user installation...", "INFO")
                cmd[2] = "InstallAllUsers=0"
                subprocess.run(cmd, check=True, creationflags=creation_flags)

    def install_cmake_func(self, install_dir):
        if platform.system() == "Windows":
            self.log("Downloading CMake...")
            cmake_url = "https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1-windows-x86_64.msi"
            cmake_installer = install_dir / "cmake_installer.msi"

            urllib.request.urlretrieve(cmake_url, cmake_installer)

            self.log("Installing CMake...")
            cmd = [
                "msiexec",
                "/i",
                str(cmake_installer),
                "/quiet",
                "ADD_CMAKE_TO_PATH=System",
            ]
            self.safe_subprocess_run(cmd, check=True)

            cmake_installer.unlink()
        else:
            if shutil.which("apt-get"):
                self.safe_subprocess_run(["sudo", "apt-get", "update"], check=True)
                self.safe_subprocess_run(
                    ["sudo", "apt-get", "install", "-y", "cmake"], check=True
                )
            elif shutil.which("brew"):
                self.safe_subprocess_run(["brew", "install", "cmake"], check=True)
            else:
                raise Exception(
                    "Package manager not supported. Install CMake manually."
                )

    def download_model(self, install_dir):
        models_dir = install_dir / "models"
        gemma_model_path = models_dir / "gemma-3-4b-it.gguf"
        if gemma_model_path.exists():
            self.log("Gemma model already uploaded, ignored")
        else:
            gemma_url = "https://huggingface.co/unsloth/gemma-3-4b-it-GGUF/resolve/main/gemma-3-4b-it-Q4_K_M.gguf"
            self.log("Download the Gemma-3-4B model...")

            def download_gemma_progress(block_num, block_size, total_size):
                downloaded = block_num * block_size
                percent = min(100, (downloaded / total_size) * 100)
                self.update_progress(
                    percent * 0.4, f"Download Gemma model: {percent:.1f}%"
                )

            urllib.request.urlretrieve(
                gemma_url, gemma_model_path, reporthook=download_gemma_progress
            )
            self.log("✅ Gemma model downloaded successfully.")

        vision_text_model_path = models_dir / "ggml-model-Q4_K_M.gguf"
        if vision_text_model_path.exists():
            self.log("Vision text model already uploaded, ignored")
        else:
            vision_text_url = "https://huggingface.co/openbmb/MiniCPM-V-2_6-gguf/resolve/main/ggml-model-Q4_K_M.gguf"
            self.log("Download MiniCPM-V text model...")

            def download_vision_text_progress(block_num, block_size, total_size):
                downloaded = block_num * block_size
                percent = min(100, (downloaded / total_size) * 100)
                self.update_progress(
                    40 + (percent * 0.3), f"Download vision text model: {percent:.1f}%"
                )

            urllib.request.urlretrieve(
                vision_text_url,
                vision_text_model_path,
                reporthook=download_vision_text_progress,
            )
            self.log("✅ Vision text model downloaded successfully.")

        mmproj_model_path = models_dir / "mmproj-model-f16.gguf"
        if mmproj_model_path.exists():
            self.log("Vision mmproj model already uploaded, ignored")
        else:
            mmproj_url = "https://huggingface.co/openbmb/MiniCPM-V-2_6-gguf/resolve/main/mmproj-model-f16.gguf"
            self.log("Download MiniCPM-V mmproj model...")

            def download_mmproj_progress(block_num, block_size, total_size):
                downloaded = block_num * block_size
                percent = min(100, (downloaded / total_size) * 100)
                self.update_progress(
                    70 + (percent * 0.3),
                    f"Download vision mmproj model: {percent:.1f}%",
                )

            urllib.request.urlretrieve(
                mmproj_url, mmproj_model_path, reporthook=download_mmproj_progress
            )
            self.log("✅ Vision mmproj model downloaded successfully.")

        self.log("✅ All GGUF models downloaded successfully.")

    def create_installation_registry(self, install_dir):
        if platform.system() == "Windows":
            registry_dir = Path(os.environ.get("APPDATA")) / "OBSIDIAN-Neural"
        elif platform.system() == "Darwin":
            registry_dir = Path.home() / "Library/Application Support/OBSIDIAN-Neural"
        else:
            registry_dir = Path.home() / ".config/obsidian-neural"

        registry_dir.mkdir(parents=True, exist_ok=True)
        registry_file = registry_dir / "installation.json"

        installation_info = {
            "installation_path": str(install_dir),
            "version": "1.0",
            "install_date": time.strftime("%Y-%m-%d %H:%M:%S"),
            "platform": platform.system(),
        }

        with open(registry_file, "w") as f:
            json.dump(installation_info, f, indent=2)

        self.log(f"Installation registry created: {registry_file}")

    def install_git_func(self, install_dir):
        if platform.system() == "Windows":
            self.log("Downloading Git...")
            git_url = "https://github.com/git-for-windows/git/releases/download/v2.43.0.windows.1/Git-2.43.0-64-bit.exe"
            git_installer = install_dir / "git_installer.exe"

            urllib.request.urlretrieve(git_url, git_installer)

            self.log("Installing Git...")
            cmd = [
                str(git_installer),
                "/SILENT",
                "/COMPONENTS=icons,ext\\reg\\shellhere,assoc,assoc_sh",
            ]
            self.safe_subprocess_run(cmd, check=True)

            git_installer.unlink()
        else:
            if shutil.which("apt-get"):
                self.safe_subprocess_run(
                    ["sudo", "apt-get", "install", "-y", "git"], check=True
                )
            elif shutil.which("brew"):
                self.safe_subprocess_run(["brew", "install", "git"], check=True)

    def install_buildtools_func(self, install_dir):
        if platform.system() != "Windows":
            return

        self.log("Downloading Visual Studio Build Tools...")
        buildtools_url = "https://aka.ms/vs/17/release/vs_buildtools.exe"
        buildtools_installer = install_dir / "vs_buildtools.exe"

        urllib.request.urlretrieve(buildtools_url, buildtools_installer)

        self.log("Installing the Build Tools (this may take some time)...")
        cmd = [
            str(buildtools_installer),
            "--quiet",
            "--wait",
            "--add",
            "Microsoft.VisualStudio.Workload.VCTools",
            "--add",
            "Microsoft.VisualStudio.Component.VC.CMake.Project",
        ]
        self.safe_subprocess_run(cmd, check=True)

        buildtools_installer.unlink()

    def install_cuda_func(self, install_dir):
        if platform.system() == "Windows":
            self.log("Downloading CUDA Toolkit 11.8...")
            cuda_url = "https://developer.download.nvidia.com/compute/cuda/11.8.0/local_installers/cuda_11.8.0_522.06_windows.exe"
            cuda_installer = install_dir / "cuda_installer.exe"
            self.log("⚠️ CUDA download (3+ GB) - this may take some time...")
            urllib.request.urlretrieve(cuda_url, cuda_installer)

            self.log("CUDA installation in progress...")
            cmd = [str(cuda_installer), "-s"]
            self.safe_subprocess_run(cmd, check=True)

            cuda_installer.unlink()

    def validate_install_dir(self, install_dir):
        dangerous_paths = [
            "/Applications",
            "/System",
            "/Library",
            "/usr",
            "/bin",
            "/sbin",
            "C:\\Windows",
            "C:\\Program Files",
            str(Path.home()),
        ]

        for danger in dangerous_paths:
            if str(install_dir).startswith(danger) or str(install_dir) == danger:
                raise Exception(
                    f"REFUSED: Cannot install to dangerous path: {install_dir}"
                )

    def confirm_installation_path(self, install_dir):
        dangerous_paths = [
            "/Applications",
            "/System",
            "/Library",
            "/usr",
            "/bin",
            "/sbin",
            "C:\\Windows",
            "C:\\Program Files",
            "C:\\Program Files (x86)",
            str(Path.home()),
            "/",
            "C:\\",
        ]

        is_dangerous = any(
            str(install_dir).startswith(danger) or str(install_dir) == danger
            for danger in dangerous_paths
        )

        if is_dangerous:
            warning_title = "⚠️ DANGEROUS INSTALLATION PATH"
            warning_color = "red"
            warning_text = f"""🚨 WARNING: You are about to install to a SYSTEM DIRECTORY:

    {install_dir}

    This could potentially:
    - Overwrite existing files
    - Damage your system
    - Require administrator privileges
    - Make uninstallation difficult

    RECOMMENDATION: Choose a safer location like:
    - {Path.home()}/OBSIDIAN-Neural
    - {Path.home()}/Documents/OBSIDIAN-Neural
    - {Path.home()}/Desktop/OBSIDIAN-Neural"""
        else:
            warning_title = "📁 Confirm Installation Location"
            warning_color = "blue"
            warning_text = f"""Installation will create files and folders in:

    {install_dir}

    This directory will contain:
    - Source code and Python environment (~500MB)
    - AI models (~3GB)
    - Configuration files
    - Build tools and dependencies"""

        dialog = tk.Toplevel(self.root)
        dialog.title(warning_title)
        dialog.geometry("1200x600")
        dialog.transient(self.root)
        dialog.grab_set()
        dialog.resizable(True, True)
        dialog.minsize(600, 500)

        dialog.update_idletasks()
        x = (dialog.winfo_screenwidth() // 2) - (1200 // 2)
        y = (dialog.winfo_screenheight() // 2) - (600 // 2)
        dialog.geometry(f"1200x600+{x}+{y}")

        main_frame = ttk.Frame(dialog, padding="10")
        main_frame.pack(fill="both", expand=True)

        title_frame = ttk.Frame(main_frame)
        title_frame.pack(fill="x", pady=(0, 10))

        title_label = ttk.Label(
            title_frame,
            text=warning_title,
            font=("Arial", 14, "bold"),
            foreground=warning_color,
        )
        title_label.pack()

        canvas = tk.Canvas(main_frame, highlightthickness=0, bd=0)
        scrollbar = ttk.Scrollbar(main_frame, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)

        scrollable_frame.bind(
            "<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )

        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")

        canvas_window = canvas.create_window(
            (0, 0), window=scrollable_frame, anchor="nw"
        )

        def configure_scroll_region(event=None):
            canvas.configure(scrollregion=canvas.bbox("all"))
            canvas.itemconfig(canvas_window, width=canvas.winfo_width())

        canvas.bind("<Configure>", configure_scroll_region)
        canvas.configure(yscrollcommand=scrollbar.set)

        content_frame = scrollable_frame

        warning_label = ttk.Label(
            content_frame,
            text=warning_text,
            font=("Arial", 10),
            wraplength=550,
            justify="left",
        )
        warning_label.pack(pady=(0, 15), padx=10)

        path_frame = ttk.LabelFrame(
            content_frame, text="Installation Path", padding="10"
        )
        path_frame.pack(fill="x", pady=(0, 15), padx=10)

        path_label = ttk.Label(
            path_frame,
            text=str(install_dir),
            font=("Consolas", 11, "bold"),
            foreground="darkblue",
            wraplength=500,
        )
        path_label.pack()

        details_frame = ttk.LabelFrame(
            content_frame, text="📋 What will be installed", padding="10"
        )
        details_frame.pack(fill="x", pady=(0, 15), padx=10)

        details_text = """• Python virtual environment and dependencies
    - PyTorch with GPU support (if available)
    - AI models: Gemma-3-4B (2.49 GB)
    - FastAPI server and web interface
    - VST3 plugin (if compilation enabled)
    - Desktop shortcuts and configuration files
    - Benchmark and performance testing tools

    Total disk space required: ~4-5 GB"""

        details_label = ttk.Label(
            details_frame,
            text=details_text,
            font=("Arial", 9),
            wraplength=500,
            justify="left",
        )
        details_label.pack()

        disclaimer_frame = ttk.LabelFrame(
            content_frame, text="⚖️ Legal Disclaimer & User Responsibility", padding="10"
        )
        disclaimer_frame.pack(fill="x", pady=(0, 15), padx=10)

        disclaimer_text = """BY PROCEEDING WITH THIS INSTALLATION, YOU ACKNOWLEDGE AND AGREE THAT:

    RESPONSIBILITY:
    - You understand exactly where files will be installed
    - You accept FULL RESPONSIBILITY for any system changes
    - You have verified the installation path is safe and appropriate
    - You have backups of important data before proceeding

    WARRANTY DISCLAIMER:
    - OBSIDIAN-Neural is provided "AS IS" without warranty of any kind
    - No guarantee of fitness for any particular purpose
    - No warranty that the software will be error-free or uninterrupted

    LIABILITY LIMITATION:
    - The developer is NOT LIABLE for any damage, data loss, or system issues
    - You use this software entirely at your own risk
    - You are responsible for any consequences of the installation

    TECHNICAL RISKS:
    - Installing to system directories may damage your operating system
    - Large files will be downloaded and disk space will be used
    - Python environment and dependencies will be installed
    - Configuration files will be created in system directories

    USER OBLIGATIONS:
    - Choose installation paths carefully
    - Avoid system directories (/Applications, /System, C:\\Windows, etc.)
    - Ensure you have sufficient disk space (5+ GB)
    - Read and understand what will be installed

    LICENSE:
    This software is open source under Mozilla Public License 2.0
    Full license: https://mozilla.org/MPL/2.0/

    By checking the box below, you confirm you have read, understood, 
    and accept these terms and the associated risks."""

        disclaimer_label = ttk.Label(
            disclaimer_frame,
            text=disclaimer_text,
            font=("Arial", 9),
            wraplength=500,
            justify="left",
            foreground="black",
        )
        disclaimer_label.pack()

        bottom_frame = ttk.Frame(main_frame)
        bottom_frame.pack(fill="x", pady=(10, 0))

        separator = ttk.Separator(bottom_frame, orient="horizontal")
        separator.pack(fill="x", pady=(0, 10))

        confirmation_frame = ttk.Frame(bottom_frame)
        confirmation_frame.pack(fill="x", pady=(0, 10))

        confirm_var = tk.BooleanVar(value=False)
        confirm_check = ttk.Checkbutton(
            confirmation_frame,
            text="✅ I have read and accept all terms above, understand the risks, and take full responsibility",
            variable=confirm_var,
            command=lambda: proceed_btn.config(
                state="normal" if confirm_var.get() else "disabled"
            ),
        )
        confirm_check.pack(anchor="w")

        note_label = ttk.Label(
            confirmation_frame,
            text="⚠️ You must scroll up and read the complete disclaimer before proceeding",
            font=("Arial", 8),
            foreground="red",
        )
        note_label.pack(anchor="w", pady=(5, 0))

        button_frame = ttk.Frame(bottom_frame)
        button_frame.pack(fill="x")

        result = {"confirmed": False, "new_path": None}

        def proceed():
            if confirm_var.get():
                result["confirmed"] = True
                dialog.destroy()
            else:
                messagebox.showerror(
                    "Confirmation Required",
                    "Please read the disclaimer and check the confirmation box to proceed.",
                )

        def change_path():
            new_path = filedialog.askdirectory(
                title="Choose Safe Installation Directory", initialdir=str(Path.home())
            )
            if new_path:
                result["new_path"] = new_path
                dialog.destroy()

        def cancel():
            result["confirmed"] = False
            dialog.destroy()

        proceed_btn = ttk.Button(
            button_frame,
            text="🚀 I Accept - Proceed with Installation",
            command=proceed,
            state="disabled",
        )
        proceed_btn.pack(side="right", padx=(5, 0))

        change_btn = ttk.Button(
            button_frame, text="📁 Choose Different Path", command=change_path
        )
        change_btn.pack(side="left", padx=(5, 0))

        cancel_btn = ttk.Button(
            button_frame, text="❌ Cancel Installation", command=cancel
        )
        cancel_btn.pack(side="right", padx=(5, 0))

        def _on_mousewheel(event):
            canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")

        def _bind_mousewheel(event):
            canvas.bind_all("<MouseWheel>", _on_mousewheel)

        def _unbind_mousewheel(event):
            canvas.unbind_all("<MouseWheel>")

        canvas.bind("<Enter>", _bind_mousewheel)
        canvas.bind("<Leave>", _unbind_mousewheel)

        dialog.focus_set()
        dialog.wait_window()

        return result

    def update_confirm_buttons(self, confirmed, proceed_btn, change_btn):
        if confirmed:
            proceed_btn.config(state="normal")
        else:
            proceed_btn.config(state="disabled")

    def download_source(self, install_dir):
        self.validate_install_dir(install_dir)
        current_dir = Path(__file__).parent

        repo_markers = [
            current_dir / "main.py",
            current_dir / "vst",
            current_dir / "server",
            current_dir / "core",
            current_dir / "install.bat",
            current_dir / "install.sh",
        ]

        is_in_repo = all(marker.exists() for marker in repo_markers)

        if is_in_repo:
            self.log("Source code detected locally (development mode)")
            self.log(f"Using code from: {current_dir}")

            if str(install_dir) == str(current_dir):
                self.log("Installing in same folder - skipping copy")
                return install_dir

            import shutil

            exclude_items = {
                "installer.py",
                "__pycache__",
                ".git",
                "env",
                "venv",
                ".env",
                "models",
                "node_modules",
                "build",
                "dist",
                ".pytest_cache",
                "screenshot.png",
                ".DS_Store",
                "Thumbs.db",
                ".directory",
                "*.pyc",
                ".gitignore",
                ".gitmodules",
            }

            try:
                install_dir.mkdir(parents=True, exist_ok=True)
                self.log(f"Created installation directory: {install_dir}")
            except PermissionError as e:
                self.log(f"Permission denied creating directory: {e}", "ERROR")
                if platform.system() != "Windows":
                    self.log(
                        "Try running with sudo or check directory permissions",
                        "WARNING",
                    )
                raise Exception(f"Cannot create installation directory: {e}")

            if platform.system() == "Windows" and self.is_admin:
                try:
                    result = self.safe_subprocess_run(
                        [
                            "icacls",
                            str(install_dir),
                            "/grant",
                            "Authenticated Users:(OI)(CI)F",
                        ],
                        check=False,
                        capture_output=True,
                        text=True,
                        timeout=30,
                    )
                    if result.returncode == 0:
                        self.log("Windows permissions set successfully")
                    else:
                        self.log(f"icacls warning: {result.stderr}", "WARNING")
                except Exception as e:
                    self.log(f"Could not set Windows permissions: {e}", "WARNING")

            elif platform.system() in ["Darwin", "Linux"]:
                try:
                    os.chmod(install_dir, 0o755)
                    self.log("Unix permissions set (755)")
                except Exception as e:
                    self.log(f"Could not set Unix permissions: {e}", "WARNING")

            copied_files = 0
            copied_dirs = 0

            for item in current_dir.iterdir():
                if item.name not in exclude_items:
                    target = install_dir / item.name

                    try:
                        if item.is_file():
                            if target.exists():
                                if item.stat().st_mtime <= target.stat().st_mtime:
                                    continue

                            shutil.copy2(item, target)
                            copied_files += 1

                            if platform.system() in ["Darwin", "Linux"]:
                                if item.suffix in [".py", ".sh", ".bat"]:
                                    os.chmod(target, 0o755)
                                else:
                                    os.chmod(target, 0o644)

                            self.log(f"Copied file: {item.name}")

                        elif item.is_dir():
                            if target.exists():
                                self.log(
                                    f"Directory {item.name}/ already exists, updating..."
                                )

                            shutil.copytree(item, target, dirs_exist_ok=True)
                            copied_dirs += 1

                            if platform.system() in ["Darwin", "Linux"]:
                                self.set_directory_permissions_recursive(target)

                            self.log(f"Copied directory: {item.name}/")

                    except PermissionError as e:
                        self.log(f"Permission denied copying {item.name}: {e}", "ERROR")
                        if platform.system() != "Windows":
                            self.log(
                                "Try running with appropriate permissions", "WARNING"
                            )
                        raise Exception(f"Cannot copy {item.name}: permission denied")

                    except Exception as e:
                        self.log(f"Warning copying {item.name}: {e}", "WARNING")

            self.log(
                f"Local copy completed: {copied_files} files, {copied_dirs} directories"
            )

        else:
            self.log("Cloning innermost47/ai-dj repository from GitHub...")

            if install_dir.exists():
                main_py = install_dir / "main.py"
                if main_py.exists():
                    self.log(
                        "Project already exists in target directory, skipping clone",
                        "SUCCESS",
                    )
                    return install_dir

                self.log("Target directory exists but incomplete, cleaning...")
                try:
                    import shutil

                    shutil.rmtree(install_dir)
                    self.log("Cleaned existing directory")
                except Exception as e:
                    self.log(f"Could not clean directory: {e}", "WARNING")
                    temp_dir = install_dir.parent / f"obsidian_clone_{int(time.time())}"
                    self.log(f"Using temporary directory: {temp_dir}")
                    install_dir = temp_dir

            try:
                install_dir.mkdir(parents=True, exist_ok=True)
            except PermissionError as e:
                self.log(f"Permission denied creating directory: {e}", "ERROR")
                raise Exception(f"Cannot create installation directory: {e}")

            if platform.system() == "Windows" and self.is_admin:
                try:
                    result = self.safe_subprocess_run(
                        [
                            "icacls",
                            str(install_dir),
                            "/grant",
                            "Authenticated Users:(OI)(CI)F",
                        ],
                        check=False,
                        capture_output=True,
                        text=True,
                        timeout=30,
                    )
                    if result.returncode == 0:
                        self.log("Windows clone directory permissions set")
                except Exception as e:
                    self.log(f"Could not set Windows permissions: {e}", "WARNING")

            elif platform.system() in ["Darwin", "Linux"]:
                try:
                    os.chmod(install_dir, 0o755)
                    self.log("Unix clone directory permissions set")
                except Exception as e:
                    self.log(f"Could not set Unix permissions: {e}", "WARNING")

            cmd = [
                "git",
                "clone",
                "--depth",
                "1",
                "--single-branch",
                "--branch",
                "main",
                "https://github.com/innermost47/ai-dj.git",
                str(install_dir),
            ]

            self.log("Starting Git clone (this may take a moment)...")

            try:
                env = os.environ.copy()
                if platform.system() == "Windows":
                    env["GIT_CONFIG_NOSYSTEM"] = "1"
                    env["LC_ALL"] = "C.UTF-8"

                result = self.safe_subprocess_run(
                    cmd, capture_output=True, text=True, timeout=300, env=env
                )

                if result.returncode != 0:
                    self.log(f"Git clone failed (code {result.returncode})", "ERROR")
                    self.log(f"Git error: {result.stderr}", "ERROR")

                    self.log("Retrying with basic clone...")
                    basic_cmd = [
                        "git",
                        "clone",
                        "https://github.com/innermost47/ai-dj.git",
                        str(install_dir),
                    ]

                    result = self.safe_subprocess_run(
                        basic_cmd, capture_output=True, text=True, timeout=600, env=env
                    )

                    if result.returncode != 0:
                        raise Exception(f"Git clone error: {result.stderr}")

                self.log("Repository cloned successfully")

                if platform.system() in ["Darwin", "Linux"]:
                    self.set_directory_permissions_recursive(install_dir)

            except subprocess.TimeoutExpired:
                self.log("Git clone timed out", "ERROR")
                raise Exception("Git clone operation timed out (network issue?)")

            except FileNotFoundError:
                self.log("Git command not found", "ERROR")
                raise Exception("Git is not installed or not in PATH")

        essential_dirs = ["server", "core", "vst"]
        missing_dirs = []

        for dir_name in essential_dirs:
            if (install_dir / dir_name).exists():
                self.log(f"✅ Directory {dir_name}/ found")
            else:
                self.log(f"❌ Directory {dir_name}/ missing", "WARNING")
                missing_dirs.append(dir_name)

        critical_files = ["main.py"]
        for file_name in critical_files:
            if (install_dir / file_name).exists():
                self.log(f"✅ File {file_name} found")
            else:
                self.log(f"⚠️ File {file_name} missing", "WARNING")

        if missing_dirs:
            self.log(
                f"Warning: Missing essential directories: {missing_dirs}", "WARNING"
            )
            self.log("Installation may be incomplete", "WARNING")

        return install_dir

    def set_directory_permissions_recursive(self, directory):
        try:
            for root, dirs, files in os.walk(directory):
                os.chmod(root, 0o755)

                for file in files:
                    file_path = os.path.join(root, file)
                    try:
                        if file.endswith((".py", ".sh", ".bat", ".exe")):
                            os.chmod(file_path, 0o755)
                        else:
                            os.chmod(file_path, 0o644)
                    except OSError:
                        pass

        except Exception as e:
            self.log(f"Could not set recursive permissions: {e}", "WARNING")

    def create_venv(self, install_dir):
        venv_path = install_dir / "env"
        models_dir = install_dir / "models"

        try:
            install_dir.mkdir(parents=True, exist_ok=True)
            models_dir.mkdir(exist_ok=True)
            self.log(f"Directories created: {install_dir}")

            if platform.system() == "Windows" and self.is_admin:
                try:
                    result = self.safe_subprocess_run(
                        [
                            "icacls",
                            str(install_dir),
                            "/grant",
                            "Authenticated Users:(OI)(CI)F",
                        ],
                        check=False,
                        capture_output=True,
                        text=True,
                        timeout=30,
                    )
                    if result.returncode == 0:
                        self.log("Windows permissions set for virtual environment")
                    else:
                        self.log(f"icacls warning: {result.stderr}", "WARNING")
                except Exception as e:
                    self.log(f"Could not set Windows permissions: {e}", "WARNING")

            elif platform.system() in ["Darwin", "Linux"]:
                try:
                    os.chmod(install_dir, 0o755)
                    os.chmod(models_dir, 0o755)
                    self.log("Unix permissions set for virtual environment")
                except Exception as e:
                    self.log(f"Could not set Unix permissions: {e}", "WARNING")

        except PermissionError as e:
            self.log(f"Permission error creating directories: {e}", "ERROR")
            if platform.system() != "Windows":
                self.log(
                    "Try running with sudo or check directory permissions", "WARNING"
                )
            raise Exception(f"Cannot create installation directory: {e}")

        if venv_path.exists():
            self.log(f"Virtual environment directory found: {venv_path}")

            if platform.system() == "Windows":
                python_in_venv = venv_path / "Scripts" / "python.exe"
                pip_in_venv = venv_path / "Scripts" / "pip.exe"
                activate_script = venv_path / "Scripts" / "activate.bat"
            else:
                python_in_venv = venv_path / "bin" / "python"
                pip_in_venv = venv_path / "bin" / "pip"
                activate_script = venv_path / "bin" / "activate"

            if python_in_venv.exists() and activate_script.exists():
                try:
                    test_result = self.safe_subprocess_run(
                        [str(python_in_venv), "--version"],
                        capture_output=True,
                        text=True,
                        timeout=10,
                    )
                    if test_result.returncode == 0:
                        python_version = test_result.stdout.strip()
                        self.log("Virtual environment already exists and is functional")
                        self.log(f"Using existing environment: {venv_path}")
                        self.log(f"Python version: {python_version}")
                        return install_dir
                    else:
                        self.log(
                            "Existing virtual environment appears corrupted", "WARNING"
                        )
                except Exception as e:
                    self.log(
                        f"Cannot test existing virtual environment: {e}", "WARNING"
                    )
            else:
                self.log(
                    "Invalid virtual environment detected - missing executables",
                    "WARNING",
                )

            self.log("Removing invalid virtual environment...")
            try:
                import shutil

                if platform.system() == "Windows":
                    try:
                        import gc

                        gc.collect()
                        time.sleep(0.5)
                    except:
                        pass

                shutil.rmtree(venv_path)
                self.log("Old environment removed successfully")

            except PermissionError as e:
                self.log(f"Permission denied removing old environment: {e}", "ERROR")
                if platform.system() == "Windows":
                    self.log("Try closing any Python processes and retry", "WARNING")
                else:
                    self.log("Try running with appropriate permissions", "WARNING")
                raise Exception(f"Cannot remove old virtual environment: {e}")

            except Exception as e:
                self.log(f"Could not remove old environment: {e}", "WARNING")
                timestamp = int(time.time())
                venv_path = install_dir / f"env_{timestamp}"
                self.log(f"Using alternative environment path: {venv_path}")

        self.log("Creating Python virtual environment...")
        self.log(f"Target directory: {venv_path}")

        python_exe = self.find_system_python()
        self.log(f"Using Python: {python_exe}")

        cmd = [python_exe, "-m", "venv", str(venv_path)]

        if platform.system() == "Windows":
            cmd.append("--copies")
        elif platform.system() == "Darwin":
            cmd.extend(["--symlinks"])

        try:
            env = os.environ.copy()
            env_vars_to_clean = [
                "PYTHONPATH",
                "VIRTUAL_ENV",
                "CONDA_DEFAULT_ENV",
                "CONDA_PREFIX",
                "PIPENV_ACTIVE",
                "POETRY_ACTIVE",
            ]

            for var in env_vars_to_clean:
                env.pop(var, None)

            if platform.system() == "Windows":
                env["PYTHONIOENCODING"] = "utf-8"
                env["PYTHONLEGACYWINDOWSSTDIO"] = "1"
            elif platform.system() == "Darwin":
                env["LC_ALL"] = "en_US.UTF-8"
                env["LANG"] = "en_US.UTF-8"
            else:
                env["LC_ALL"] = "C.UTF-8"
                env["LANG"] = "C.UTF-8"

            self.log("Starting virtual environment creation...")
            self.log(f"Command: {' '.join(cmd)}")

            result = self.safe_subprocess_run(
                cmd, capture_output=True, text=True, check=True, env=env, timeout=180
            )

            self.log("Virtual environment created successfully")
            if platform.system() == "Windows":
                python_in_new_venv = venv_path / "Scripts" / "python.exe"
                pip_in_new_venv = venv_path / "Scripts" / "pip.exe"
            else:
                python_in_new_venv = venv_path / "bin" / "python"
                pip_in_new_venv = venv_path / "bin" / "pip"

            if not python_in_new_venv.exists():
                raise Exception(
                    f"Python executable not found in new virtual environment: {python_in_new_venv}"
                )

            try:
                test_result = self.safe_subprocess_run(
                    [str(python_in_new_venv), "--version"],
                    capture_output=True,
                    text=True,
                    timeout=10,
                )
                if test_result.returncode == 0:
                    python_version = test_result.stdout.strip()
                    self.log(f"✅ Virtual environment verified: {python_version}")
                else:
                    self.log("⚠️ Warning: Cannot verify virtual environment", "WARNING")
            except Exception as e:
                self.log(f"Warning during verification: {e}", "WARNING")

            if platform.system() in ["Darwin", "Linux"]:
                try:
                    os.chmod(venv_path, 0o755)

                    if python_in_new_venv.exists():
                        os.chmod(python_in_new_venv, 0o755)
                    if pip_in_new_venv.exists():
                        os.chmod(pip_in_new_venv, 0o755)

                    activate_script = venv_path / "bin" / "activate"
                    if activate_script.exists():
                        os.chmod(activate_script, 0o755)

                    self.log("Virtual environment permissions set")

                except Exception as e:
                    self.log(f"Could not set venv permissions: {e}", "WARNING")

            return install_dir

        except subprocess.TimeoutExpired:
            self.log("Virtual environment creation timed out", "ERROR")
            self.log(
                "This might indicate a problem with Python installation", "WARNING"
            )
            raise Exception("Virtual environment creation timed out")

        except subprocess.CalledProcessError as e:
            self.log(
                f"Failed to create virtual environment (code {e.returncode})", "ERROR"
            )

            error_msg = e.stderr if e.stderr else "Unknown error"
            self.log(f"Error output: {error_msg}", "ERROR")

            if "ensurepip" in error_msg.lower():
                self.log(
                    "💡 Try installing/updating pip: python -m pip install --upgrade pip",
                    "INFO",
                )
            elif "permission" in error_msg.lower():
                if platform.system() != "Windows":
                    self.log(
                        "💡 Try running with sudo or check directory permissions",
                        "INFO",
                    )
                else:
                    self.log("💡 Try running as administrator", "INFO")
            elif "ssl" in error_msg.lower() or "certificate" in error_msg.lower():
                self.log(
                    "💡 SSL/Certificate issue - check network/firewall settings", "INFO"
                )

            raise Exception(f"Virtual environment creation failed: {error_msg}")

        except FileNotFoundError:
            self.log(f"Python executable not found: {python_exe}", "ERROR")
            self.log("Make sure Python is properly installed and in PATH", "WARNING")
            raise Exception(f"Python executable not found: {python_exe}")

        except Exception as e:
            self.log(
                f"Unexpected error during virtual environment creation: {e}", "ERROR"
            )
            raise Exception(f"Virtual environment creation failed: {e}")

    def install_vst(self, install_dir):
        vst_build_dir = install_dir / "vst" / "build"
        vst_target_dir_str = self.vst_path.get()
        self.log(f"Attempting to use VST3 path: {vst_target_dir_str}")
        vst_target_dir = Path(vst_target_dir_str)

        if not vst_target_dir.exists():
            self.log(
                f"Provided VST3 path '{vst_target_dir}' does not exist. Attempting to create it.",
                "WARNING",
            )
        elif not vst_target_dir.is_dir():
            self.log(
                f"Provided VST3 path '{vst_target_dir}' is not a directory. Cannot install VST here.",
                "ERROR",
            )
            raise Exception(f"VST3 path '{vst_target_dir}' is not a directory.")
        else:
            self.log(f"VST3 path '{vst_target_dir}' exists and is a directory.")

        possible_vst_locations = [
            vst_build_dir,
            vst_build_dir / "VST3",
            vst_build_dir / "Release" / "VST3",
            vst_build_dir / "RelWithDebInfo" / "VST3",
            vst_build_dir / "Debug" / "VST3",
            vst_build_dir / "ObsidianNeural_artefacts" / "Release" / "VST3",
            vst_build_dir / "ObsidianNeural_artefacts" / "VST3",
            vst_build_dir / "ObsidianNeuralVST_artefacts" / "Release" / "VST3",
            vst_build_dir / "ObsidianNeuralVST_artefacts" / "VST3",
            vst_build_dir
            / "ObsidianNeuralVST_artefacts"
            / "Release"
            / "VST3"
            / "OBSIDIAN-Neural.vst3"
            / "Contents"
            / "x86_64-win",
            vst_build_dir
            / "ObsidianNeuralVST_artefacts"
            / "Release"
            / "VST3"
            / "OBSIDIAN-Neural.vst3"
            / "Contents"
            / "MacOS",
            vst_build_dir
            / "ObsidianNeuralVST_artefacts"
            / "Release"
            / "VST3"
            / "OBSIDIAN-Neural.vst3"
            / "Contents"
            / "x86_64-linux",
        ]

        vst_files_found = []
        for loc in possible_vst_locations:
            if loc.exists() and loc.is_dir():
                self.log(f"Searching for .vst3 in {loc}")
                vst_files_found.extend(list(loc.glob("*.vst3")))
                vst_files_found.extend(list(loc.glob("**/*.vst3")))

        vst_files = []
        seen_paths = set()
        for f_path in vst_files_found:
            try:
                resolved_path = f_path.resolve()
                if resolved_path not in seen_paths:
                    vst_files.append(f_path)
                    seen_paths.add(resolved_path)
                    self.log(
                        f"Found potential VST3: {f_path} (Is Dir: {f_path.is_dir()})"
                    )
            except Exception as e:
                self.log(f"Error resolving path {f_path}: {e}", "WARNING")

        if not vst_files:
            self.log(
                f"No VST3 file/bundle found after compilation in standard build output locations under {vst_build_dir}.",
                "ERROR",
            )
            self.log(
                f"Checked locations: {[str(loc) for loc in possible_vst_locations]}",
                "INFO",
            )

            if vst_build_dir.exists():
                try:
                    build_contents = list(vst_build_dir.rglob("*"))
                    self.log(
                        f"Build directory contains {len(build_contents)} items", "INFO"
                    )

                    vst_like_files = [
                        f
                        for f in build_contents
                        if f.suffix in [".vst3", ".dll", ".so", ".dylib"]
                    ]
                    if vst_like_files:
                        self.log(
                            f"Found VST-like files: {[str(f) for f in vst_like_files[:5]]}",
                            "INFO",
                        )

                except Exception as e:
                    self.log(f"Could not analyze build directory: {e}", "WARNING")

            raise Exception(
                "VST3 plugin compilation result not found. Check build logs."
            )

        vst_file_to_copy = None
        for f in vst_files:
            if f.is_dir():
                vst_file_to_copy = f
                break

        if not vst_file_to_copy and vst_files:
            vst_file_to_copy = vst_files[0]
            self.log(
                f"Warning: Found .vst3 as a file, not a directory: {vst_file_to_copy}",
                "WARNING",
            )
        elif not vst_file_to_copy:
            self.log(
                f"Critical error: vst_files list was populated but no suitable vst_file_to_copy selected.",
                "ERROR",
            )
            raise Exception("VST3 selection logic error after finding candidates.")

        self.log(
            f"Selected VST3 source for copy: {vst_file_to_copy} (Name: {vst_file_to_copy.name}, Is Dir: {vst_file_to_copy.is_dir()})"
        )
        target_plugin_path = vst_target_dir / vst_file_to_copy.name

        try:
            self.log(f"Ensuring VST3 parent directory exists: {vst_target_dir}")
            vst_target_dir.mkdir(parents=True, exist_ok=True)
            self.log(f"VST3 parent directory '{vst_target_dir}' confirmed/created.")

            if platform.system() == "Windows" and self.is_admin:
                self.log(
                    f"Setting Windows permissions for VST3 directory (Admin context)"
                )
                acl_result = self.safe_subprocess_run(
                    [
                        "icacls",
                        str(vst_target_dir),
                        "/grant",
                        "Users:(OI)(CI)F",
                        "/T",
                    ],
                    check=False,
                    capture_output=True,
                    text=True,
                    timeout=30,
                )
                if acl_result.returncode != 0:
                    self.log(
                        f"icacls for {vst_target_dir} failed (code {acl_result.returncode}): {acl_result.stderr.strip()}",
                        "WARNING",
                    )
                else:
                    self.log(
                        f"Windows VST3 directory permissions set successfully", "INFO"
                    )

            elif platform.system() in ["Darwin", "Linux"]:
                try:
                    os.chmod(vst_target_dir, 0o755)
                    self.log("Unix VST3 directory permissions set (755)")
                except Exception as e:
                    self.log(
                        f"Could not set Unix VST3 directory permissions: {e}", "WARNING"
                    )

            self.log(f"VST3 source for copy operation: {vst_file_to_copy}")
            self.log(f"VST3 target destination for copy: {target_plugin_path}")

            if not vst_file_to_copy.exists():
                self.log(
                    f"Source VST file/bundle {vst_file_to_copy} does not exist before copy attempt!",
                    "ERROR",
                )
                raise Exception(
                    f"Source VST {vst_file_to_copy} vanished or was not found correctly."
                )

            if vst_file_to_copy.is_dir():
                self.log(
                    f"Source {vst_file_to_copy.name} is a directory. Using shutil.copytree."
                )

                if target_plugin_path.exists():
                    self.log(
                        f"Target {target_plugin_path} already exists. Removing it first."
                    )
                    try:
                        if platform.system() == "Windows":
                            import gc

                            gc.collect()
                            time.sleep(0.5)

                        shutil.rmtree(target_plugin_path)
                        self.log(f"Successfully removed existing {target_plugin_path}.")
                    except PermissionError as e_rm:
                        self.log(
                            f"Permission denied removing existing VST: {e_rm}", "ERROR"
                        )
                        if platform.system() == "Windows":
                            self.log(
                                "Try closing your DAW and any audio applications",
                                "WARNING",
                            )
                        raise Exception(
                            f"Cannot overwrite existing VST (in use?): {e_rm}"
                        )
                    except Exception as e_rm:
                        self.log(
                            f"Failed to remove existing {target_plugin_path}: {e_rm}",
                            "ERROR",
                        )
                        raise Exception(
                            f"Failed to overwrite existing VST at {target_plugin_path}: {e_rm}"
                        )

                shutil.copytree(
                    vst_file_to_copy, target_plugin_path, dirs_exist_ok=False
                )
                self.log(
                    f"VST3 bundle copied from {vst_file_to_copy} to {target_plugin_path}"
                )

            else:
                self.log(
                    f"Source {vst_file_to_copy.name} is a file (unexpected for VST3). Using shutil.copy2.",
                    "WARNING",
                )

                if target_plugin_path.exists() and target_plugin_path.is_dir():
                    self.log(
                        f"Target {target_plugin_path} is a directory, but source is a file. Removing target directory.",
                        "WARNING",
                    )
                    shutil.rmtree(target_plugin_path)
                elif target_plugin_path.exists():
                    self.log(
                        f"Target file {target_plugin_path} exists. It will be overwritten."
                    )

                shutil.copy2(vst_file_to_copy, target_plugin_path)
                self.log(
                    f"VST3 file copied from {vst_file_to_copy} to {target_plugin_path}"
                )

            time.sleep(0.3)

            if target_plugin_path.exists():
                self.log(
                    f"VST plugin successfully installed to: {target_plugin_path}",
                    "SUCCESS",
                )

                if platform.system() == "Windows" and self.is_admin:
                    self.log(f"Setting Windows permissions for installed VST3")
                    acl_target_result = self.safe_subprocess_run(
                        [
                            "icacls",
                            str(target_plugin_path),
                            "/grant",
                            "Users:(OI)(CI)R",
                            "/T",
                        ],
                        check=False,
                        capture_output=True,
                        text=True,
                        timeout=30,
                    )
                    if acl_target_result.returncode != 0:
                        self.log(
                            f"icacls for installed VST3 failed: {acl_target_result.stderr.strip()}",
                            "WARNING",
                        )
                    else:
                        self.log(f"Windows VST3 permissions set successfully", "INFO")

                elif platform.system() in ["Darwin", "Linux"]:
                    try:
                        if target_plugin_path.is_dir():
                            self.set_vst_permissions_recursive(target_plugin_path)
                            self.log("Unix VST3 bundle permissions set recursively")
                        else:
                            os.chmod(target_plugin_path, 0o755)
                            self.log("Unix VST3 file permissions set (755)")
                    except Exception as e:
                        self.log(
                            f"Could not set Unix VST3 final permissions: {e}", "WARNING"
                        )
                try:
                    if target_plugin_path.is_dir():
                        if platform.system() == "Darwin":
                            binary_path = target_plugin_path / "Contents" / "MacOS"
                            if binary_path.exists():
                                self.log("✅ macOS VST3 bundle structure verified")
                            else:
                                self.log(
                                    "⚠️ macOS VST3 bundle missing MacOS binary",
                                    "WARNING",
                                )
                        elif platform.system() == "Linux":
                            binary_path = (
                                target_plugin_path / "Contents" / "x86_64-linux"
                            )
                            if binary_path.exists():
                                self.log("✅ Linux VST3 bundle structure verified")
                            else:
                                self.log(
                                    "⚠️ Linux VST3 bundle missing x86_64-linux binary",
                                    "WARNING",
                                )
                        else:
                            binary_path = target_plugin_path / "Contents" / "x86_64-win"
                            if binary_path.exists():
                                self.log("✅ Windows VST3 bundle structure verified")
                            else:
                                self.log(
                                    "⚠️ Windows VST3 bundle missing x86_64-win binary",
                                    "WARNING",
                                )
                    else:
                        file_size = target_plugin_path.stat().st_size
                        if file_size > 1024:
                            self.log(f"✅ VST3 file verified ({file_size} bytes)")
                        else:
                            self.log(
                                f"⚠️ VST3 file seems too small ({file_size} bytes)",
                                "WARNING",
                            )

                except Exception as e:
                    self.log(f"Could not verify VST3 integrity: {e}", "WARNING")

            else:
                self.log(
                    f"CRITICAL: VST copy verification failed. Target {target_plugin_path} does not exist after copy attempt.",
                    "ERROR",
                )
                self.log(
                    f"Source was: {vst_file_to_copy} (exists: {vst_file_to_copy.exists()})",
                    "INFO",
                )
                self.log(
                    f"Target VST3 parent folder: {vst_target_dir} (exists: {vst_target_dir.exists()})",
                    "INFO",
                )

                try:
                    if vst_target_dir.exists():
                        target_contents = list(vst_target_dir.iterdir())
                        self.log(
                            f"Target directory contents: {[f.name for f in target_contents]}",
                            "INFO",
                        )
                except Exception:
                    pass

                raise Exception(
                    f"VST copy verification failed: {target_plugin_path} not found after copy."
                )

        except PermissionError as e:
            self.log(f"Permission denied during VST installation: {e}", "ERROR")
            self.log(
                f"Details: Source='{vst_file_to_copy}', Target='{target_plugin_path}', TargetParent='{vst_target_dir}'",
                "INFO",
            )

            if platform.system() == "Windows":
                self.log(
                    "This usually happens when targeting system VST folders (Program Files).",
                    "WARNING",
                )
                self.log("Solutions:", "INFO")
                self.log("  1. Run installer as administrator", "INFO")
                self.log("  2. Choose user VST3 folder: %APPDATA%/VST3", "INFO")
                self.log("  3. Close your DAW and try again", "INFO")
            elif platform.system() == "Darwin":
                self.log("macOS permission issue. Solutions:", "WARNING")
                self.log(
                    "  1. Try user VST3 folder: ~/Library/Audio/Plug-Ins/VST3", "INFO"
                )
                self.log("  2. Run with sudo (not recommended)", "INFO")
                self.log("  3. Check System Preferences > Security & Privacy", "INFO")
            else:
                self.log("Linux permission issue. Solutions:", "WARNING")
                self.log("  1. Try user VST3 folder: ~/.vst3", "INFO")
                self.log("  2. Run with sudo", "INFO")
                self.log("  3. Check directory ownership", "INFO")

            self.log("Manual copy instructions:", "WARNING")
            self.log(f"   Copy FROM: {vst_file_to_copy}", "INFO")
            self.log(f"   Copy TO:   {target_plugin_path}", "INFO")
            raise

        except Exception as e:
            self.log(f"Unexpected error during VST installation: {e}", "ERROR")
            self.log(
                f"Details: Source='{vst_file_to_copy if 'vst_file_to_copy' in locals() else 'N/A'}', "
                f"Target='{target_plugin_path if 'target_plugin_path' in locals() else 'N/A'}', "
                f"TargetParent='{vst_target_dir}'",
                "INFO",
            )
            self.log("Manual copy may be required (see paths above).", "WARNING")
            raise

    def set_vst_permissions_recursive(self, vst_bundle_path):
        try:
            for root, dirs, files in os.walk(vst_bundle_path):
                os.chmod(root, 0o755)

                for file in files:
                    file_path = os.path.join(root, file)
                    try:
                        if any(
                            arch in root
                            for arch in ["MacOS", "x86_64-linux", "x86_64-win"]
                        ):
                            os.chmod(file_path, 0o755)
                        elif file.endswith((".plist", ".xml", ".txt")):
                            os.chmod(file_path, 0o644)
                        else:
                            os.chmod(file_path, 0o644)
                    except OSError as e:
                        self.log(
                            f"Could not set permissions for {file_path}: {e}", "WARNING"
                        )

        except Exception as e:
            self.log(f"Could not set VST3 recursive permissions: {e}", "WARNING")

    def find_system_python(self):
        if getattr(sys, "frozen", False):
            import shutil

            for py_name in ["python", "python3", "python.exe"]:
                py_path = shutil.which(py_name)
                if py_path and self.test_python_executable(py_path):
                    return py_path
            if platform.system() == "Windows":
                common_paths = [
                    "C:\\Python311\\python.exe",
                    "C:\\Python310\\python.exe",
                    "C:\\Python39\\python.exe",
                    "C:\\Python38\\python.exe",
                    "C:\\Program Files\\Python311\\python.exe",
                    "C:\\Program Files\\Python310\\python.exe",
                    "C:\\Users\\Public\\AppData\\Local\\Programs\\Python\\Python311\\python.exe",
                ]
                for py_path in common_paths:
                    if Path(py_path).exists() and self.test_python_executable(py_path):
                        return py_path

            return "python"

        if not (hasattr(sys, "base_prefix") and sys.base_prefix != sys.prefix):
            current_python = sys.executable
            if self.test_python_executable(current_python):
                return current_python

        if hasattr(sys, "base_prefix") and sys.base_prefix != sys.prefix:
            if platform.system() == "Windows":
                base_python = Path(sys.base_prefix) / "python.exe"
            else:
                base_python = Path(sys.base_prefix) / "bin" / "python"
            if base_python.exists() and self.test_python_executable(str(base_python)):
                return str(base_python)

        import shutil

        for py_name in ["python", "python3", "python.exe"]:
            py_path = shutil.which(py_name)
            if py_path and self.test_python_executable(py_path):
                return py_path

        return "python"

    def test_python_executable(self, python_path):
        try:
            result = self.safe_subprocess_run(
                [python_path, "--version"], capture_output=True, text=True, timeout=10
            )
            if result.returncode == 0:
                version_output = result.stdout.strip()
                if "Python 3." in version_output:
                    version_parts = version_output.split()[1].split(".")
                    major, minor = int(version_parts[0]), int(version_parts[1])
                    return major == 3 and minor >= 8
        except:
            pass
        return False

    def install_python_deps(self, install_dir):
        self.log(f"Installing dependencies in: {install_dir}")
        if platform.system() == "Windows":
            pip_path = install_dir / "env" / "Scripts" / "pip.exe"
            python_path = install_dir / "env" / "Scripts" / "python.exe"
        else:
            pip_path = install_dir / "env" / "bin" / "pip"
            python_path = install_dir / "env" / "bin" / "python"

        self.log(f"Looking for Python at: {python_path}")

        if not python_path.exists():
            self.log(f"Python executable not found at: {python_path}", "ERROR")
            self.log("Searching for Python in virtual environment...", "INFO")

            venv_dir = install_dir / "env"
            self.log(f"Checking virtual environment directory: {venv_dir}")

            if not venv_dir.exists():
                self.log(
                    f"Virtual environment directory does not exist: {venv_dir}", "ERROR"
                )
                raise Exception(f"Virtual environment not found at: {venv_dir}")

            if platform.system() == "Windows":
                possible_python_paths = [
                    venv_dir / "Scripts" / "python.exe",
                    venv_dir / "Scripts" / "python3.exe",
                    venv_dir / "python.exe",
                ]
            else:
                possible_python_paths = [
                    venv_dir / "bin" / "python",
                    venv_dir / "bin" / "python3",
                ]

            for py_path in possible_python_paths:
                self.log(f"Checking: {py_path}")
                if py_path.exists():
                    python_path = py_path
                    self.log(f"Found Python at: {python_path}")
                    break
            else:
                try:
                    venv_contents = list(venv_dir.iterdir())
                    self.log(
                        f"Virtual environment contents: {[str(p) for p in venv_contents]}"
                    )

                    if (
                        platform.system() == "Windows"
                        and (venv_dir / "Scripts").exists()
                    ):
                        scripts_contents = list((venv_dir / "Scripts").iterdir())
                        self.log(
                            f"Scripts directory contents: {[str(p) for p in scripts_contents]}"
                        )
                    elif (
                        platform.system() in ["Darwin", "Linux"]
                        and (venv_dir / "bin").exists()
                    ):
                        bin_contents = list((venv_dir / "bin").iterdir())
                        self.log(
                            f"Bin directory contents: {[str(p) for p in bin_contents]}"
                        )
                except Exception as e:
                    self.log(f"Could not list virtual environment contents: {e}")

                raise Exception(
                    f"No Python executable found in virtual environment: {venv_dir}"
                )

        if not pip_path.exists():
            self.log(f"Pip executable not found at: {pip_path}", "WARNING")
            self.log("Will use python -m pip instead")
            pip_path = None

        if self.system_info.get("cuda_available"):
            self.log("Installing PyTorch with CUDA support...")
            cmd = [
                str(python_path),
                "-m",
                "pip",
                "install",
                "torch",
                "torchvision",
                "torchaudio",
                "--index-url",
                "https://download.pytorch.org/whl/cu118",
            ]

            try:
                self.safe_subprocess_run(cmd, check=True, timeout=900)
                self.log("PyTorch with CUDA installed successfully")
            except subprocess.TimeoutExpired:
                self.log("PyTorch CUDA installation timed out", "ERROR")
                raise Exception("PyTorch CUDA installation timed out")
            except subprocess.CalledProcessError as e:
                self.log(f"PyTorch CUDA installation failed: {e.stderr}", "ERROR")
                raise

            self.log("Installing Llama CPP Python with CUDA support...")
            cmd = [
                str(python_path),
                "-m",
                "pip",
                "install",
                "llama-cpp-python==0.3.9",
                "--prefer-binary",
                "--extra-index-url=https://jllllll.github.io/llama-cpp-python-cuBLAS-wheels/AVX2/cu118",
            ]

            try:
                self.safe_subprocess_run(cmd, check=True, timeout=900)
                self.log("Llama CPP Python with CUDA installed successfully")
            except subprocess.TimeoutExpired:
                self.log("Llama CPP Python CUDA installation timed out", "ERROR")
                raise Exception("Llama CPP Python CUDA installation timed out")
            except subprocess.CalledProcessError as e:
                self.log(
                    f"Llama CPP Python CUDA installation failed: {e.stderr}", "ERROR"
                )
                raise

        elif self.system_info.get("mps_available"):
            self.log(
                "Installing PyTorch with Metal Performance Shaders (MPS) support..."
            )
            cmd = [
                str(python_path),
                "-m",
                "pip",
                "install",
                "torch",
                "torchvision",
                "torchaudio",
            ]

            try:
                self.safe_subprocess_run(cmd, check=True, timeout=900)
                self.log("PyTorch with MPS support installed successfully")

                self.verify_mps_installation(python_path)

            except subprocess.TimeoutExpired:
                self.log("PyTorch MPS installation timed out", "ERROR")
                raise Exception("PyTorch MPS installation timed out")
            except subprocess.CalledProcessError as e:
                self.log(f"PyTorch MPS installation failed: {e.stderr}", "ERROR")
                raise

            self.log("Installing Llama CPP Python with potential Metal support...")

            try:
                cmd = [
                    str(python_path),
                    "-m",
                    "pip",
                    "install",
                    "llama-cpp-python==0.3.9",
                    "--prefer-binary",
                ]
                self.safe_subprocess_run(cmd, check=True, timeout=600)
                self.log(
                    "Llama CPP Python installed successfully (Metal support if available)"
                )

            except subprocess.CalledProcessError as e:
                self.log(f"Llama CPP Python installation error: {e.stderr}", "WARNING")
                try:
                    cmd = [
                        str(python_path),
                        "-m",
                        "pip",
                        "install",
                        "llama-cpp-python==0.3.9",
                    ]
                    self.safe_subprocess_run(cmd, check=True, timeout=600)
                    self.log("Llama CPP Python installed successfully (CPU fallback)")
                except Exception as e2:
                    self.log(f"Llama CPP Python fallback failed: {e2}", "ERROR")
                    raise

        elif self.system_info.get("rocm_available"):
            self.log("Installing PyTorch with ROCm support...")
            cmd = [
                str(python_path),
                "-m",
                "pip",
                "install",
                "torch",
                "torchvision",
                "torchaudio",
                "--index-url",
                "https://download.pytorch.org/whl/rocm5.6",
            ]

            try:
                self.safe_subprocess_run(cmd, check=True, timeout=900)
                self.log("PyTorch with ROCm installed successfully")
            except subprocess.TimeoutExpired:
                self.log("PyTorch ROCm installation timed out", "ERROR")
                raise Exception("PyTorch ROCm installation timed out")
            except subprocess.CalledProcessError as e:
                self.log(f"PyTorch ROCm installation failed: {e.stderr}", "ERROR")
                self.log("Falling back to CPU-only PyTorch...", "WARNING")
                cmd = [
                    str(python_path),
                    "-m",
                    "pip",
                    "install",
                    "torch",
                    "torchvision",
                    "torchaudio",
                ]
                try:
                    self.safe_subprocess_run(cmd, check=True, timeout=900)
                    self.log("PyTorch CPU fallback installed successfully")
                except Exception as e2:
                    self.log(f"PyTorch CPU fallback failed: {e2}", "ERROR")
                    raise

            self.log("Installing Llama CPP Python for ROCm...")
            cmd = [str(python_path), "-m", "pip", "install", "llama-cpp-python==0.3.9"]

            try:
                self.safe_subprocess_run(cmd, check=True, timeout=600)
                self.log("Llama CPP Python for ROCm installed successfully")
            except subprocess.TimeoutExpired:
                self.log("Llama CPP Python ROCm installation timed out", "ERROR")
                raise Exception("Llama CPP Python ROCm installation timed out")
            except subprocess.CalledProcessError as e:
                self.log(
                    f"Llama CPP Python ROCm installation failed: {e.stderr}", "ERROR"
                )
                raise

        elif self.system_info.get("gpu_type") == "amd":
            self.log("Installing PyTorch for CPU (AMD GPU without ROCm)...")
            cmd = [
                str(python_path),
                "-m",
                "pip",
                "install",
                "torch",
                "torchvision",
                "torchaudio",
            ]

            try:
                self.safe_subprocess_run(cmd, check=True, timeout=900)
                self.log("PyTorch for CPU installed successfully")
            except subprocess.TimeoutExpired:
                self.log("PyTorch installation timed out", "ERROR")
                raise Exception("PyTorch installation timed out")
            except subprocess.CalledProcessError as e:
                self.log(f"PyTorch installation failed: {e.stderr}", "ERROR")
                raise

            self.log("Installing Llama CPP Python for CPU...")
            cmd = [str(python_path), "-m", "pip", "install", "llama-cpp-python==0.3.9"]

            try:
                self.safe_subprocess_run(cmd, check=True, timeout=600)
                self.log("Llama CPP Python for CPU installed successfully")
            except subprocess.TimeoutExpired:
                self.log("Llama CPP Python installation timed out", "ERROR")
                raise Exception("Llama CPP Python installation timed out")
            except subprocess.CalledProcessError as e:
                self.log(f"Llama CPP Python installation failed: {e.stderr}", "ERROR")
                raise

        else:
            self.log("Installing PyTorch for CPU...")
            cmd = [
                str(python_path),
                "-m",
                "pip",
                "install",
                "torch",
                "torchvision",
                "torchaudio",
            ]

            try:
                self.safe_subprocess_run(cmd, check=True, timeout=900)
                self.log("PyTorch for CPU installed successfully")
            except subprocess.TimeoutExpired:
                self.log("PyTorch installation timed out", "ERROR")
                raise Exception("PyTorch installation timed out")
            except subprocess.CalledProcessError as e:
                self.log(f"PyTorch installation failed: {e.stderr}", "ERROR")
                raise

            self.log("Installing Llama CPP Python for CPU...")
            cmd = [str(python_path), "-m", "pip", "install", "llama-cpp-python==0.3.9"]

            try:
                self.safe_subprocess_run(cmd, check=True, timeout=600)
                self.log("Llama CPP Python for CPU installed successfully")
            except subprocess.TimeoutExpired:
                self.log("Llama CPP Python installation timed out", "ERROR")
                raise Exception("Llama CPP Python installation timed out")
            except subprocess.CalledProcessError as e:
                self.log(f"Llama CPP Python installation failed: {e.stderr}", "ERROR")
                raise

        self.log("Installing additional libraries...")

        packages = [
            "stable-audio-tools",
            "librosa",
            "soundfile",
            "fastapi",
            "uvicorn",
            "python-dotenv",
            "requests",
            "apscheduler",
            "demucs",
            "cryptography",
            "pyinstaller",
            "pystray",
        ]

        if platform.system() == "Darwin":
            packages.extend(
                [
                    "psutil",
                    "Pillow",
                ]
            )
        elif platform.system() == "Linux":
            packages.extend(
                [
                    "psutil",
                    "Pillow",
                    "PyQt5",
                ]
            )
        elif platform.system() == "Windows":
            packages.extend(
                [
                    "psutil",
                    "Pillow",
                    "pywin32",
                ]
            )

        successful_packages = []
        failed_packages = []

        for package in packages:
            self.log(f"Installing {package}...")
            cmd = [str(python_path), "-m", "pip", "install", package]

            try:
                result = self.safe_subprocess_run(
                    cmd, check=True, capture_output=True, text=True, timeout=300
                )
                self.log(f"{package} installed successfully")
                successful_packages.append(package)

            except subprocess.CalledProcessError as e:
                self.log(f"Installation error for {package}: {e.stderr}", "WARNING")
                failed_packages.append(package)

                if package == "PyQt5" and platform.system() == "Linux":
                    self.log("Trying PyQt5 alternative installation...")
                    try:
                        alt_cmd = [
                            str(python_path),
                            "-m",
                            "pip",
                            "install",
                            "PyQt5",
                            "--no-deps",
                        ]
                        self.safe_subprocess_run(
                            alt_cmd,
                            check=True,
                            capture_output=True,
                            text=True,
                            timeout=300,
                        )
                        self.log("PyQt5 installed with alternative method")
                        successful_packages.append(package)
                        failed_packages.remove(package)
                    except:
                        pass

            except subprocess.TimeoutExpired:
                self.log(f"Installation timeout for {package}", "WARNING")
                failed_packages.append(package)

        self.log("=" * 50)
        self.log("📦 PYTHON DEPENDENCIES INSTALLATION REPORT")
        self.log("=" * 50)

        if self.system_info.get("cuda_available"):
            self.log("🟢 PyTorch: CUDA acceleration enabled")
        elif self.system_info.get("mps_available"):
            self.log("🍎 PyTorch: Metal Performance Shaders (MPS) enabled")
        elif self.system_info.get("rocm_available"):
            self.log("🔴 PyTorch: ROCm acceleration enabled")
        else:
            self.log("💻 PyTorch: CPU-only mode")

        self.log(f"✅ Successfully installed: {len(successful_packages)} packages")
        for pkg in successful_packages[:5]:
            self.log(f"   • {pkg}")
        if len(successful_packages) > 5:
            self.log(f"   • ... and {len(successful_packages) - 5} more")

        if failed_packages:
            self.log(f"⚠️ Failed to install: {len(failed_packages)} packages")
            for pkg in failed_packages:
                self.log(f"   • {pkg}")
            self.log(
                "💡 These packages are optional and shouldn't affect core functionality"
            )
        else:
            self.log("🎉 All packages installed successfully!")

        self.log("=" * 50)
        self.log("All Python dependencies installation completed")

        try:
            test_script = """
    import torch
    print(f"PyTorch version: {torch.__version__}")
    print(f"CUDA available: {torch.cuda.is_available()}")
    """

            if self.system_info.get("mps_available"):
                test_script += """
    if hasattr(torch.backends, 'mps'):
        print(f"MPS available: {torch.backends.mps.is_available()}")
    """

            test_script += """
    print("✅ PyTorch import successful")
    """

            result = self.safe_subprocess_run(
                [str(python_path), "-c", test_script],
                capture_output=True,
                text=True,
                timeout=30,
            )

            if result.returncode == 0:
                self.log("✅ PyTorch verification successful!")
                for line in result.stdout.strip().split("\n"):
                    if line.strip():
                        self.log(f"   {line}")
            else:
                self.log("⚠️ PyTorch verification failed", "WARNING")

        except Exception as e:
            self.log(f"Could not verify PyTorch installation: {e}", "WARNING")

    def verify_mps_installation(self, python_path):
        """Vérifie que MPS fonctionne correctement sur macOS"""
        test_script = """
    import torch
    import sys

    try:
        print(f"PyTorch version: {torch.__version__}")
        print(f"MPS available: {torch.backends.mps.is_available()}")
        
        if torch.backends.mps.is_available():
            # Test simple tensor MPS
            device = torch.device("mps")
            x = torch.randn(3, 3, device=device)
            y = torch.randn(3, 3, device=device)
            z = torch.mm(x, y)
            print(f"MPS test successful: {z.shape}")
            print("✅ Metal Performance Shaders ready!")
        else:
            print("❌ MPS not available")
            sys.exit(1)
            
    except Exception as e:
        print(f"❌ MPS test failed: {e}")
        sys.exit(1)
    """

        try:
            result = self.safe_subprocess_run(
                [str(python_path), "-c", test_script],
                capture_output=True,
                text=True,
                timeout=30,
            )

            if result.returncode == 0:
                self.log("✅ MPS verification successful!", "SUCCESS")
                for line in result.stdout.strip().split("\n"):
                    if line.strip():
                        self.log(f"   {line}")
            else:
                self.log("⚠️ MPS verification failed", "WARNING")
                self.log(f"Error: {result.stderr}", "WARNING")

        except Exception as e:
            self.log(f"Could not verify MPS: {e}", "WARNING")

    def setup_environment(self, install_dir):
        if self.system_info.get("cuda_available"):
            device_hint = "cuda"
        elif self.system_info.get("mps_available"):
            device_hint = "mps"
        elif self.system_info.get("rocm_available"):
            device_hint = "cuda"
        else:
            device_hint = "cpu"

        env_content = f"""DJ_IA_API_KEYS=api keys separated by commas
    LLM_MODEL_PATH={install_dir}/models/gemma-3-4b-it.gguf
    ENVIRONMENT=dev
    HOST=127.0.0.1
    PORT=8000
    AUDIO_MODEL=stabilityai/stable-audio-open-1.0
    TORCH_DEVICE={device_hint}
    """

        (install_dir / ".env").write_text(env_content)
        self.log("✅ .env configuration created with optimal device")
        self.log(f"   Configured device: {device_hint}")
        self.log("=" * 50)
        self.log("🎉 INSTALLATION COMPLETED SUCCESSFULLY!")
        self.log("=" * 50)
        self.log("📋 HOW TO USE OBSIDIAN-NEURAL:")
        self.log("")
        self.log("1️⃣ START THE SERVER:")
        if platform.system() == "Windows":
            self.log("   • Use desktop shortcut: 'OBSIDIAN-Neural Server'")
            self.log("   • OR run: python server_interface.py")
        else:
            self.log("   • Use desktop shortcut: 'OBSIDIAN-Neural Server'")
            self.log("   • OR run the executable from bin/ folder")
        self.log("")
        self.log("2️⃣ CONFIGURE THE SERVER:")
        self.log("   • Enter your Hugging Face token")
        self.log("   • Generate API keys")
        self.log("   • Start the server from the GUI")
        self.log("")
        self.log("3️⃣ SETUP YOUR DAW:")
        self.log("   • Load the VST3 plugin")
        self.log("   • Set server URL (usually http://localhost:8000)")
        self.log("   • Enter API key from server interface")
        self.log("")
        self.log("=" * 50)

    def build_vst(self, install_dir):
        vst_dir = install_dir / "vst"

        if not vst_dir.exists():
            raise Exception("VST source code missing")

        cmake_file = vst_dir / "CMakeLists.txt"
        if not cmake_file.exists():
            raise Exception("CMakeLists.txt not found in vst/")

        if platform.system() == "Linux":
            self.log("Installing Linux development dependencies...")
            try:
                if shutil.which("apt-get"):
                    deps = [
                        "sudo",
                        "apt-get",
                        "install",
                        "-y",
                        "libasound2-dev",
                        "libx11-dev",
                        "libxext-dev",
                        "libxrandr-dev",
                        "libxcomposite-dev",
                        "libxinerama-dev",
                        "libxcursor-dev",
                        "libfreetype6-dev",
                        "libfontconfig1-dev",
                        "libgtk-3-dev",
                        "libwebkit2gtk-4.1-dev",
                        "libcurl4-openssl-dev",
                    ]
                    self.safe_subprocess_run(deps, check=True)
                    self.log("Ubuntu/Debian dependencies installed")

                elif shutil.which("dnf"):
                    deps = [
                        "sudo",
                        "dnf",
                        "install",
                        "-y",
                        "alsa-lib-devel",
                        "libX11-devel",
                        "libXext-devel",
                        "libXrandr-devel",
                        "libXcomposite-devel",
                        "libXinerama-devel",
                        "libXcursor-devel",
                        "freetype-devel",
                        "fontconfig-devel",
                        "gtk3-devel",
                        "webkit2gtk4.1-devel",
                        "libcurl-devel",
                    ]
                    self.safe_subprocess_run(deps, check=True)
                    self.log("Fedora dependencies installed")

                elif shutil.which("pacman"):
                    deps = [
                        "sudo",
                        "pacman",
                        "-S",
                        "--noconfirm",
                        "alsa-lib",
                        "libx11",
                        "libxext",
                        "libxrandr",
                        "libxcomposite",
                        "libxinerama",
                        "libxcursor",
                        "freetype2",
                        "fontconfig",
                        "gtk3",
                        "webkit2gtk",
                        "curl",
                    ]
                    self.safe_subprocess_run(deps, check=True)
                    self.log("Arch Linux dependencies installed")

                elif shutil.which("zypper"):
                    deps = [
                        "sudo",
                        "zypper",
                        "install",
                        "-y",
                        "alsa-devel",
                        "libX11-devel",
                        "libXext-devel",
                        "libXrandr-devel",
                        "libXcomposite-devel",
                        "freetype2-devel",
                        "fontconfig-devel",
                        "gtk3-devel",
                        "webkit2gtk3-devel",
                    ]
                    self.safe_subprocess_run(deps, check=True)
                    self.log("openSUSE dependencies installed")

                else:
                    self.log("Unsupported Linux distribution for auto-deps", "WARNING")
                    self.log("Please install JUCE dependencies manually:", "WARNING")
                    self.log(
                        "  Ubuntu/Debian: sudo apt install libasound2-dev libx11-dev libfreetype6-dev",
                        "INFO",
                    )
                    self.log(
                        "  Fedora: sudo dnf install alsa-lib-devel libX11-devel freetype-devel",
                        "INFO",
                    )
                    self.log("  Arch: sudo pacman -S alsa-lib libx11 freetype2", "INFO")

            except Exception as e:
                self.log(f"Could not install Linux dependencies: {e}", "WARNING")
                self.log("Please install JUCE development packages manually", "WARNING")

        juce_dir = vst_dir / "JUCE"
        if not juce_dir.exists():
            self.log("Downloading JUCE framework...")
            cmd = [
                "git",
                "clone",
                "https://github.com/juce-framework/JUCE.git",
                str(juce_dir),
                "--depth",
                "1",
            ]
            result = self.safe_subprocess_run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                raise Exception(f"JUCE download failed: {result.stderr}")

        soundtouch_dir = vst_dir / "soundtouch"
        if not soundtouch_dir.exists():
            self.log("Downloading SoundTouch library...")
            cmd = [
                "git",
                "clone",
                "https://codeberg.org/soundtouch/soundtouch.git",
                str(soundtouch_dir),
                "--depth",
                "1",
            ]
            result = self.safe_subprocess_run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                raise Exception(f"SoundTouch download failed: {result.stderr}")

        build_dir = vst_dir / "build"

        if build_dir.exists():
            self.log("Cleaning previous build cache...")
            import shutil

            shutil.rmtree(build_dir)

        build_dir.mkdir(exist_ok=True)

        if platform.system() == "Windows" and self.is_admin:
            self.safe_subprocess_run(
                ["icacls", str(build_dir), "/grant", "Authenticated Users:(OI)(CI)F"],
                check=False,
                capture_output=True,
            )
        elif platform.system() in ["Darwin", "Linux"]:
            try:
                os.chmod(build_dir, 0o755)
                self.log("Unix build directory permissions set")
            except Exception as e:
                self.log(f"Could not set build permissions: {e}", "WARNING")

        self.log("Configuring CMake...")
        cmake_cmd = ["cmake", ".."]

        if platform.system() == "Linux":
            cmake_cmd.extend(
                ["-DCMAKE_BUILD_TYPE=Release", "-DJUCE_ENABLE_GPL_MODE=ON"]
            )
        elif platform.system() == "Darwin":
            cmake_cmd.extend(
                ["-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15"]
            )

        result = self.safe_subprocess_run(
            cmake_cmd, cwd=build_dir, capture_output=True, text=True
        )

        if result.returncode != 0:
            self.log("CMake configuration failed", "ERROR")
            self.log(f"Stdout: {result.stdout}", "ERROR")
            self.log(f"Stderr: {result.stderr}", "ERROR")

            if platform.system() == "Linux":
                self.log("💡 Make sure JUCE dependencies are installed", "INFO")
                self.log("💡 Try: sudo apt install libasound2-dev libx11-dev", "INFO")
            elif platform.system() == "Darwin":
                self.log("💡 Make sure Xcode Command Line Tools are installed", "INFO")
                self.log("💡 Try: xcode-select --install", "INFO")

            raise Exception("CMake configuration failed")

        self.log("CMake configuration successful")
        self.log("Compiling VST plugin...")
        build_cmd = ["cmake", "--build", ".", "--config", "Release"]

        if platform.system() in ["Darwin", "Linux"]:
            import multiprocessing

            cpu_count = multiprocessing.cpu_count()
            build_cmd.extend(["--parallel", str(cpu_count)])

        result = self.safe_subprocess_run(
            build_cmd, cwd=build_dir, capture_output=True, text=True
        )

        if result.returncode != 0:
            self.log("VST compilation failed", "ERROR")
            self.log(f"Stdout: {result.stdout}", "ERROR")
            self.log(f"Stderr: {result.stderr}", "ERROR")
            raise Exception("VST compilation failed")
        else:
            self.log("VST plugin compiled successfully")

    def start_server(self, install_dir):
        try:
            if platform.system() == "Darwin":
                exe_path = install_dir / "bin" / "OBSIDIAN-Neural-Server-macos"
            elif platform.system() == "Linux":
                exe_path = install_dir / "bin" / "OBSIDIAN-Neural-Server-linux"
            else:
                exe_path = None

            if exe_path and exe_path.exists():
                subprocess.Popen([str(exe_path)], cwd=install_dir)
                self.log("✅ Server started via executable!")
            else:
                if platform.system() == "Windows":
                    python_path = install_dir / "env" / "Scripts" / "python.exe"
                else:
                    python_path = install_dir / "env" / "bin" / "python"

                server_script = install_dir / "server_interface.py"
                if server_script.exists():
                    subprocess.Popen(
                        [str(python_path), str(server_script)], cwd=install_dir
                    )
                    self.log("✅ Server interface started via Python!")
                else:
                    main_script = install_dir / "main.py"
                    subprocess.Popen(
                        [str(python_path), str(main_script)], cwd=install_dir
                    )
                    self.log("✅ Server started via main.py!")

        except Exception as e:
            self.log(f"❌ Startup error: {e}", "ERROR")

    def run(self):

        if not self.is_admin:
            self.log("⚠️ Recommended admin privileges", "WARNING")

        self.root.mainloop()


if __name__ == "__main__":
    try:
        app = ObsidianNeuralInstaller()
        app.run()
    except Exception as e_main:
        import tkinter as tk
        from tkinter import messagebox

        root = tk.Tk()
        root.withdraw()
        messagebox.showerror(
            "Fatal Error",
            f"A critical error occurred: {e_main}",
        )
        root.destroy()
