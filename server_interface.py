# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright (C) 2025 Anthony Charretier
# Contributor: Iyari Cancino Gomez

import platform
import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext, filedialog
import subprocess
import shutil
import threading
import os
import json
import sys
import time
from pathlib import Path
import psutil
import sqlite3
from core.paths import get_config_db_path

try:
    # Importación condicional de pystray para permitir ejecutar sin esta dependencia
    import pystray
    from pystray import MenuItem, Menu

    TRAY_AVAILABLE = True
    try:
        from PIL import Image, ImageDraw, ImageTk

        PIL_AVAILABLE = True
    except ImportError:
        PIL_AVAILABLE = False
        TRAY_AVAILABLE = False

except ImportError:
    TRAY_AVAILABLE = False
    print("pystray not available - tray functionality disabled")
from core.secure_storage import SecureStorage


class ObsidianNeuralLauncher:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("OBSIDIAN-Neural - Launcher & Control Panel")
        self.root.geometry("900x700")
        self.root.minsize(850, 650)
        self.root.resizable(True, True)
        self.api_keys_list = []
        self.root.update_idletasks()
        x = (self.root.winfo_screenwidth() // 2) - (900 // 2)
        y = (self.root.winfo_screenheight() // 2) - (700 // 2)
        self.root.geometry(f"900x700+{x}+{y}")
        self.tray_icon = None
        self.is_in_tray = False
        self.server_process = None
        self.is_server_running = False
        self.root.protocol("WM_DELETE_WINDOW", self.on_window_close)
        self.log_text = None

        self.init_database()
        self.installation_dir = self.detect_installation_dir()
        if self.installation_dir != Path.cwd():
            os.chdir(self.installation_dir)
        self.setup_variables()

        self.setup_ui()

        self.load_config()
        self.load_api_keys()

        if self.check_first_launch():
            self.load_config()
            self.load_api_keys()

        self.enable_auto_save()
        self.monitor_server()

    def create_tray_image(self):
        if platform.system() == "Darwin":
            width, height = 32, 32
        elif platform.system() == "Linux":
            width, height = 48, 48
        else:
            width, height = 64, 64

        image = Image.new("RGBA", (width, height), color=(0, 0, 0, 0))
        draw = ImageDraw.Draw(image)

        if platform.system() == "Darwin":
            green_main = (0, 200, 80, 255)
            green_dark = (0, 150, 60, 255)
        else:
            green_main = (0, 255, 100, 255)
            green_dark = (0, 180, 70, 255)

        padding = max(4, width // 16)
        triangle_points = [
            (width // 2, padding),
            (padding, height - padding),
            (width - padding, height - padding),
        ]

        shadow_points = [(p[0] + 1, p[1] + 1) for p in triangle_points]
        draw.polygon(shadow_points, fill=(0, 0, 0, 100))

        draw.polygon(triangle_points, fill=green_main, outline=green_dark, width=2)

        inner_padding = padding + width // 8
        inner_points = [
            (width // 2, inner_padding),
            (inner_padding, height - inner_padding),
            (width - inner_padding, height - inner_padding),
        ]
        draw.polygon(
            inner_points, fill=(255, 255, 255, 180), outline=green_dark, width=1
        )

        return image

    def create_tray_menu(self):
        return Menu(
            MenuItem("🎵 OBSIDIAN Neural", lambda: None, enabled=False),
            Menu.SEPARATOR,
            MenuItem(
                "🟢 Server Running" if self.is_server_running else "🔴 Server Stopped",
                lambda: None,
                enabled=False,
            ),
            MenuItem(
                f"📡 {self.server_url.get()}" if self.server_url.get() else "No URL",
                self.copy_server_url if self.server_url.get() else lambda: None,
                enabled=bool(self.server_url.get()),
            ),
            Menu.SEPARATOR,
            MenuItem(
                "▶️ Start Server",
                self.start_server,
                enabled=not self.is_server_running,
            ),
            MenuItem(
                "⏹️ Stop Server",
                self.stop_server,
                enabled=self.is_server_running,
            ),
            MenuItem(
                "🔄 Restart Server",
                self.restart_server,
                enabled=self.is_server_running,
            ),
            Menu.SEPARATOR,
            MenuItem("📊 Show Control Panel", self.show_window),
            MenuItem("📁 Open Project Folder", self.open_project_folder),
            Menu.SEPARATOR,
            MenuItem("❌ Quit", self.quit_application),
        )

    def copy_server_url(self):
        try:
            import tkinter as tk

            root = tk.Tk()
            root.withdraw()
            root.clipboard_clear()
            root.clipboard_append(self.server_url.get())
            root.destroy()
            self.log("Server URL copied to clipboard", "SUCCESS")
        except:
            pass

    def setup_tray_icon(self):
        try:
            image = Image.new("RGBA", (64, 64), color=(0, 0, 0, 0))
            draw = ImageDraw.Draw(image)

            green_main = (0, 255, 100, 255)
            green_dark = (0, 180, 70, 255)

            triangle_points = [(32, 12), (16, 48), (48, 48)]

            shadow_points = [(p[0] + 2, p[1] + 2) for p in triangle_points]
            draw.polygon(shadow_points, fill=(0, 0, 0, 100))

            draw.polygon(triangle_points, fill=green_main, outline=green_dark, width=2)

            inner_points = [(32, 20), (24, 36), (40, 36)]
            draw.polygon(
                inner_points, fill=(255, 255, 255, 180), outline=green_dark, width=1
            )

            self.tray_icon = pystray.Icon(
                "obsidian_neural",
                image,
                "OBSIDIAN Neural Server",
                menu=self.create_tray_menu(),
            )

            self.log("Tray icon configured with green triangle ✅")

        except Exception as e:
            self.log(f"Error setting up tray icon: {e}", "ERROR")
            self.tray_icon = None

    def show_in_tray(self):
        try:
            if not TRAY_AVAILABLE:
                messagebox.showinfo(
                    "Not Available",
                    "System tray functionality requires pystray and PIL packages",
                )
                return

            self.setup_tray_icon()

            self.root.withdraw()
            self.is_in_tray = True

            self.root.after(100, self._start_tray_icon)

            self.log("Application minimized to system tray", "SUCCESS")

        except Exception as e:
            self.log(f"Error showing in tray: {e}", "ERROR")
            self.root.deiconify()
            self.is_in_tray = False

    def _start_tray_icon(self):
        try:
            if self.tray_icon and self.is_in_tray:
                import threading

                self.tray_thread = threading.Thread(target=self._run_tray, daemon=True)
                self.tray_thread.start()
        except Exception as e:
            self.log(f"Error starting tray icon: {e}", "ERROR")
            self.show_window()

    def _run_tray(self):
        try:
            if self.tray_icon:
                self.tray_icon.run()
        except Exception as e:
            self.log(f"Tray icon stopped: {e}", "WARNING")

    def show_window(self):
        try:
            if self.is_in_tray and self.tray_icon:
                self.tray_icon.stop()
                self.tray_icon = None

            self.is_in_tray = False

            self.root.deiconify()
            self.root.lift()
            self.root.focus_force()

            self.log("Window restored from system tray")

        except Exception as e:
            self.log(f"Error showing window: {e}", "ERROR")

    def update_tray_menu(self):
        if self.tray_icon and self.is_in_tray:
            try:
                self.tray_icon.menu = self.create_tray_menu()
            except Exception as e:
                self.log(f"Error updating tray menu: {e}", "WARNING")

    def on_window_close(self):
        if self.is_server_running:
            result = messagebox.askyesnocancel(
                "OBSIDIAN Neural",
                "Server is running. What would you like to do?\n\n"
                "• Yes: Minimize to system tray (keep server running)\n"
                "• No: Stop server and quit\n"
                "• Cancel: Return to application",
            )

            if result is True:
                self.show_in_tray()
            elif result is False:
                self.quit_application()
        else:
            self.quit_application()

    def quit_application(self):
        try:
            if self.is_server_running:
                self.stop_server()
                time.sleep(1)

            if self.tray_icon and self.is_in_tray:
                self.tray_icon.stop()
                self.tray_icon = None

            self.root.quit()

        except Exception as e:
            self.log(f"Error during quit: {e}", "ERROR")
            self.root.quit()

    def handle_admin_install(self):
        install_dir = self.detect_installation_dir()
        is_system_install = False

        if platform.system() == "Windows":
            system_paths = [
                "C:\\ProgramData",
                "C:\\Program Files",
                "C:\\Program Files (x86)",
            ]
            is_system_install = any(
                str(install_dir).startswith(path) for path in system_paths
            )
            local_config_name = ".env"

        elif platform.system() == "Darwin":
            system_paths = ["/Applications", "/usr/local", "/opt"]
            is_system_install = any(
                str(install_dir).startswith(path) for path in system_paths
            )
            local_config_name = ".env"

        else:
            system_paths = ["/usr", "/opt", "/usr/local"]
            is_system_install = any(
                str(install_dir).startswith(path) for path in system_paths
            )
            local_config_name = ".env"

        if is_system_install:
            self.log(f"System installation detected at {install_dir}")

            admin_env = install_dir / ".env"
            local_env = Path(local_config_name)

            if admin_env.exists() and not local_env.exists():
                try:
                    import shutil

                    shutil.copy2(admin_env, local_env)
                    self.log("Configuration copied to local directory")
                except Exception as e:
                    self.log(f"Could not copy config: {e}", "WARNING")

            info_msg = f"""Installation Mode: System-wide
    Install Location: {install_dir}
    Config Location: {local_env.absolute()}

    Note: Configuration is stored locally for security.
    Changes will apply to the local server instance."""

            messagebox.showinfo("System Installation Detected", info_msg)
            return local_env
        else:
            return install_dir / ".env"

    def create_path_management_section(self, parent):
        path_frame = ttk.LabelFrame(parent, text="📁 Installation Path", padding="15")
        path_frame.pack(fill="x", pady=(0, 15), padx=10)

        status_frame = ttk.Frame(path_frame)
        status_frame.pack(fill="x", pady=(0, 10))

        installation_path = getattr(self, "installation_dir", Path.cwd())

        if installation_path and (installation_path / "main.py").exists():
            status_icon = "✅"
            status_text = "Valid installation detected"
            status_color = "dark green"
        else:
            status_icon = "❌"
            status_text = "Installation may be invalid"
            status_color = "red"

        ttk.Label(
            status_frame,
            text=f"{status_icon} {status_text}",
            foreground=status_color,
            font=("Arial", 10, "bold"),
        ).pack(anchor="w")

        ttk.Label(path_frame, text="Current installation path:").pack(
            anchor="w", pady=(10, 5)
        )

        if not hasattr(self, "install_path_var"):
            self.install_path_var = tk.StringVar(value=str(installation_path))

        path_display_frame = ttk.Frame(path_frame)
        path_display_frame.pack(fill="x", pady=(0, 10))

        path_entry = ttk.Entry(
            path_display_frame,
            textvariable=self.install_path_var,
            state="readonly",
            font=("Consolas", 9),
        )
        path_entry.pack(side="left", fill="x", expand=True)

        ttk.Button(
            path_display_frame, text="📁 Change", command=self.change_installation_path
        ).pack(side="left", padx=(5, 0))

        ttk.Button(
            path_display_frame, text="🔍 Verify", command=self.verify_installation_path
        ).pack(side="left", padx=(5, 0))

        info_text = "This path should contain main.py, server/, core/, and other OBSIDIAN-Neural files."
        ttk.Label(
            path_frame,
            text=info_text,
            font=("Arial", 9),
            foreground="gray",
            wraplength=400,
        ).pack(anchor="w", pady=(5, 0))

    def change_installation_path(self):
        from tkinter import filedialog, messagebox

        new_path = filedialog.askdirectory(
            title="Select OBSIDIAN-Neural Installation Folder",
            initialdir=str(self.installation_dir),
            mustexist=True,
        )

        if new_path:
            new_path = Path(new_path)

            if not (new_path / "main.py").exists():
                messagebox.showerror(
                    "Invalid Installation",
                    f"Selected folder does not contain main.py:\n{new_path}",
                )
                return

            self.installation_dir = new_path
            self.install_path_var.set(str(new_path))

            self.save_installation_path(new_path)

            os.chdir(new_path)

            self.load_config()

            self.log(f"Installation path changed to: {new_path}", "SUCCESS")
            messagebox.showinfo(
                "Path Updated", f"Installation path updated to:\n{new_path}"
            )

    def save_installation_path(self, install_path):
        try:
            registry_path = self.get_installation_registry_path()
            registry_path.parent.mkdir(parents=True, exist_ok=True)

            installation_info = {
                "installation_path": str(install_path),
                "version": "1.0",
                "found_date": time.strftime("%Y-%m-%d %H:%M:%S"),
                "method": "user_selection",
            }

            with open(registry_path, "w") as f:
                json.dump(installation_info, f, indent=2)

            self.log(f"Installation path saved for future use: {install_path}")
        except Exception as e:
            self.log(f"Could not save installation path: {e}", "WARNING")

    def prompt_for_installation_path(self):
        if hasattr(self, "root"):
            from tkinter import filedialog, messagebox

            messagebox.showwarning(
                "Installation Not Found",
                "Could not automatically locate OBSIDIAN-Neural installation.\n"
                "Please select the installation folder containing main.py",
            )

            folder = filedialog.askdirectory(
                title="Select OBSIDIAN-Neural Installation Folder", mustexist=True
            )

            if folder and (Path(folder) / "main.py").exists():
                self.save_installation_path(Path(folder))
                return Path(folder)
            elif folder:
                messagebox.showerror(
                    "Invalid Folder", "Selected folder does not contain main.py"
                )

        return Path.cwd()

    def verify_installation_path(self):
        try:
            current_path = Path(self.install_path_var.get())

            missing_items = []
            found_items = []

            required_items = [
                ("main.py", "file", "Main server script"),
                ("server", "dir", "Server modules"),
                ("core", "dir", "Core functionality"),
                (".env", "file", "Environment configuration"),
            ]

            optional_items = [
                ("models", "dir", "AI models directory"),
                ("vst", "dir", "VST plugin source"),
                ("env", "dir", "Python virtual environment"),
            ]

            for item_name, item_type, description in required_items:
                item_path = current_path / item_name
                if item_type == "file" and item_path.is_file():
                    found_items.append(f"✅ {item_name} - {description}")
                elif item_type == "dir" and item_path.is_dir():
                    found_items.append(f"✅ {item_name}/ - {description}")
                else:
                    missing_items.append(f"❌ {item_name} - {description} (REQUIRED)")

            for item_name, item_type, description in optional_items:
                item_path = current_path / item_name
                if item_type == "file" and item_path.is_file():
                    found_items.append(f"✅ {item_name} - {description}")
                elif item_type == "dir" and item_path.is_dir():
                    found_items.append(f"✅ {item_name}/ - {description}")
                else:
                    found_items.append(f"⚠️ {item_name} - {description} (Optional)")

            result_message = (
                f"Installation Path Verification\n{'='*50}\n\nPath: {current_path}\n\n"
            )

            if missing_items:
                result_message += "❌ MISSING REQUIRED ITEMS:\n"
                result_message += "\n".join(missing_items) + "\n\n"

                result_message += "✅ FOUND ITEMS:\n"
                result_message += "\n".join(found_items)

                result_message += f"\n\n⚠️ This does not appear to be a valid OBSIDIAN-Neural installation."
                result_message += f"\n\nPlease select the correct installation folder containing main.py"

                messagebox.showwarning("Installation Issues Found", result_message)
                self.log(
                    "Installation verification failed - missing required files",
                    "WARNING",
                )

            else:
                result_message += "✅ ALL REQUIRED ITEMS FOUND:\n"
                result_message += "\n".join(
                    [
                        item
                        for item in found_items
                        if "✅" in item and "REQUIRED" not in item
                    ]
                )

                optional_found = [item for item in found_items if "⚠️" in item]
                if optional_found:
                    result_message += "\n\n⚠️ OPTIONAL ITEMS:\n"
                    result_message += "\n".join(optional_found)

                result_message += (
                    f"\n\n🎉 This appears to be a valid OBSIDIAN-Neural installation!"
                )

                messagebox.showinfo("Installation Verified", result_message)
                self.log("Installation verification successful", "SUCCESS")

        except Exception as e:
            error_msg = f"Error verifying installation path: {e}"
            self.log(error_msg, "ERROR")
            messagebox.showerror("Verification Error", error_msg)

    def search_installation_recursively(self, start_path, max_depth=3):
        def search_in_path(path, depth):
            if depth > max_depth:
                return None

            if (path / "main.py").exists():
                return path

            try:
                for child in path.iterdir():
                    if child.is_dir() and not child.name.startswith("."):
                        result = search_in_path(child, depth + 1)
                        if result:
                            return result
            except PermissionError:
                pass

            return None

        result = search_in_path(start_path, 0)
        if result:
            self.log(f"Found installation via recursive search: {result}")
            return result

        return self.prompt_for_installation_path()

    def detect_installation_dir(self):
        registry_path = self.get_installation_registry_path()
        if registry_path and registry_path.exists():
            try:
                with open(registry_path, "r") as f:
                    install_info = json.load(f)
                    registered_path = Path(install_info["installation_path"])
                    if (registered_path / "main.py").exists():
                        self.log(f"Found installation via registry: {registered_path}")
                        return registered_path
            except Exception as e:
                self.log(f"Could not read installation registry: {e}", "WARNING")

        current_dir = Path.cwd()

        if platform.system() == "Windows":
            possible_paths = [
                Path("C:/ProgramData/OBSIDIAN-Neural"),
                Path.home() / "OBSIDIAN-Neural",
                Path(os.environ.get("LOCALAPPDATA", Path.home() / "AppData/Local"))
                / "OBSIDIAN-Neural",
                current_dir,
                current_dir.parent,
            ]
        elif platform.system() == "Darwin":
            possible_paths = [
                Path("/Applications/OBSIDIAN-Neural"),
                Path.home() / "Applications/OBSIDIAN-Neural",
                Path.home() / "OBSIDIAN-Neural",
                Path.home() / "Documents/OBSIDIAN-Neural",
                current_dir,
                current_dir.parent,
            ]
        else:
            possible_paths = [
                Path("/opt/OBSIDIAN-Neural"),
                Path("/usr/local/share/OBSIDIAN-Neural"),
                Path.home() / "OBSIDIAN-Neural",
                Path.home() / ".local/share/OBSIDIAN-Neural",
                Path.home() / "Documents/OBSIDIAN-Neural",
                current_dir,
                current_dir.parent,
            ]

        for path in possible_paths:
            if (path / "main.py").exists():
                return path

        return self.search_installation_recursively(current_dir)

    def get_installation_registry_path(self):
        if platform.system() == "Windows":
            return (
                Path(os.environ.get("APPDATA", ""))
                / "OBSIDIAN-Neural"
                / "installation.json"
            )
        elif platform.system() == "Darwin":
            return (
                Path.home()
                / "Library/Application Support/OBSIDIAN-Neural/installation.json"
            )
        else:
            return Path.home() / ".config/obsidian-neural/installation.json"

    def get_env_path(self):
        return self.handle_admin_install()

    def setup_variables(self):
        self.api_keys_list = []
        self.model_path = tk.StringVar(value="")
        self.environment = tk.StringVar(value="dev")
        self.host = tk.StringVar(value="127.0.0.1")
        self.port = tk.StringVar(value="8000")
        self.audio_model = tk.StringVar(value="stabilityai/stable-audio-open-1.0")
        self.server_status = tk.StringVar(value="Server Stopped")
        self.server_url = tk.StringVar(value="")
        self.auto_save_enabled = False
        self.hf_token_var = tk.StringVar(value="")
        self.bypass_llm = tk.BooleanVar(value=False)

    def enable_auto_save(self):
        if self.auto_save_enabled:
            return

        self.auto_save_enabled = True
        variables = [
            self.model_path,
            self.environment,
            self.host,
            self.port,
            self.audio_model,
            self.hf_token_var,
            self.bypass_llm,
        ]

        for var in variables:
            var.trace_add("write", lambda *args: self.auto_save_config())

    def auto_save_config(self):
        if hasattr(self, "db_path") and self.auto_save_enabled:
            if hasattr(self, "_save_timer"):
                self.root.after_cancel(self._save_timer)
            self._save_timer = self.root.after(1000, self.save_config)

    def setup_ui(self):
        style = ttk.Style()
        if "clam" in style.theme_names():
            style.theme_use("clam")
        style.configure("Success.TLabel", foreground="dark green")
        style.configure("Warning.TLabel", foreground="orange")
        style.configure("Error.TLabel", foreground="red")
        style.configure("Running.TButton", background="light green")
        style.configure("Stopped.TButton", background="light coral")

        main_frame = ttk.Frame(self.root, padding="20")
        main_frame.pack(fill="both", expand=True)

        self.create_header(main_frame)
        self.notebook = ttk.Notebook(main_frame)
        self.notebook.pack(fill="both", expand=True, pady=(20, 0))

        self.create_control_tab()
        self.create_config_tab()

        self.create_logs_tab()

    def create_header(self, parent):
        header_frame = ttk.Frame(parent)
        header_frame.pack(fill="x", pady=(0, 20))

        logo_path = Path("logo.png")
        if logo_path.exists() and PIL_AVAILABLE:
            try:
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
                    text="Real-time AI Music Generation Control Panel",
                    font=("Arial", 12),
                )
                subtitle_label.pack(anchor="w")
            except Exception:
                self.create_title_without_logo(header_frame)
        else:
            self.create_title_without_logo(header_frame)

    def create_title_without_logo(self, parent):
        title_label = ttk.Label(
            parent, text="OBSIDIAN-Neural", font=("Arial", 20, "bold")
        )
        title_label.pack()

        subtitle_label = ttk.Label(
            parent,
            text="Real-time AI Music Generation Control Panel",
            font=("Arial", 12),
        )
        subtitle_label.pack()

    def create_control_tab(self):
        control_frame = ttk.Frame(self.notebook, padding="20")
        self.notebook.add(control_frame, text="🚀 Server Control")

        status_frame = ttk.LabelFrame(control_frame, text="Server Status", padding="15")
        status_frame.pack(fill="x", pady=(0, 20))

        status_display_frame = ttk.Frame(status_frame)
        status_display_frame.pack(fill="x", pady=(0, 15))

        self.status_label = ttk.Label(
            status_display_frame,
            textvariable=self.server_status,
            font=("Arial", 14, "bold"),
        )
        self.status_label.pack(side="left")

        self.url_label = ttk.Label(
            status_display_frame,
            textvariable=self.server_url,
            font=("Arial", 10),
            foreground="blue",
        )
        self.url_label.pack(side="left", padx=(20, 0))
        button_frame = ttk.Frame(status_frame)
        button_frame.pack(fill="x")

        self.start_button = ttk.Button(
            button_frame, text="🎵 Start Server", command=self.start_server
        )
        self.start_button.pack(side="left", padx=(0, 10))

        self.stop_button = ttk.Button(
            button_frame,
            text="⏹ Stop Server",
            command=self.stop_server,
            state="disabled",
        )
        self.stop_button.pack(side="left", padx=(0, 10))

        self.restart_button = ttk.Button(
            button_frame,
            text="🔄 Restart Server",
            command=self.restart_server,
            state="disabled",
        )
        self.restart_button.pack(side="left", padx=(0, 10))
        actions_frame = ttk.LabelFrame(
            control_frame, text="Quick Actions", padding="15"
        )
        actions_frame.pack(fill="x", pady=(0, 20))

        actions_button_frame = ttk.Frame(actions_frame)
        actions_button_frame.pack()

        ttk.Button(
            actions_button_frame,
            text="📁 Open Project Folder",
            command=self.open_project_folder,
        ).pack(side="left", padx=(0, 10))

        ttk.Button(
            actions_button_frame, text="📊 System Info", command=self.show_system_info
        ).pack(side="left", padx=(0, 10))

        ttk.Button(
            actions_button_frame,
            text="📍 Minimize to Tray",
            command=self.show_in_tray,
        ).pack(side="left", padx=(0, 10))

        ttk.Button(
            actions_button_frame,
            text="🔄 Check for Updates",
            command=self.check_for_updates,
        ).pack(side="left")

    def save_hf_token(self):
        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()

            token = self.hf_token_var.get().strip()
            if token:
                encrypted_token = self.secure_storage.encrypt(token)
                cursor.execute(
                    "INSERT OR REPLACE INTO config (key, value, is_encrypted) VALUES (?, ?, ?)",
                    ("hf_token", encrypted_token, 1),
                )

                os.environ["HF_TOKEN"] = token
                self.log("Hugging Face token saved and set in environment", "SUCCESS")
            else:
                cursor.execute("DELETE FROM config WHERE key = ?", ("hf_token",))
                os.environ.pop("HF_TOKEN", None)
                self.log("Hugging Face token removed", "SUCCESS")

            conn.commit()
            conn.close()
            return True

        except Exception as e:
            self.log(f"Error saving HF token: {e}", "ERROR")
            return False

    def verify_hf_token_config(self):
        token = self.hf_token_var.get().strip()
        if not token:
            messagebox.showerror("Error", "Please enter your Hugging Face token")
            return

        if not token.startswith("hf_") or len(token) < 30:
            messagebox.showerror(
                "Invalid Format",
                "Hugging Face tokens should start with 'hf_' and be ~37 characters long.\n\n"
                "Create a token at: https://huggingface.co/settings/tokens",
            )
            return

        try:
            import requests

            self.log("Verifying Hugging Face token...", "INFO")

            headers = {"Authorization": f"Bearer {token}"}

            response = requests.get(
                "https://huggingface.co/api/whoami-v2", headers=headers, timeout=15
            )

            if response.status_code != 200:
                if response.status_code == 401:
                    messagebox.showerror(
                        "Invalid Token",
                        "❌ Token is invalid or expired\n\n"
                        "Please check your token at:\n"
                        "https://huggingface.co/settings/tokens",
                    )
                else:
                    messagebox.showerror(
                        "Verification Failed",
                        f"❌ Could not verify token\n"
                        f"HTTP {response.status_code}: {response.text[:200]}",
                    )
                return

            user_info = response.json()
            username = user_info.get("name", user_info.get("fullname", "Unknown"))
            self.log(f"Token valid for user: {username}", "SUCCESS")

            model_tests = [
                (
                    "HEAD request",
                    "HEAD",
                    "https://huggingface.co/stabilityai/stable-audio-open-1.0",
                ),
                (
                    "Model info API",
                    "GET",
                    "https://huggingface.co/api/models/stabilityai/stable-audio-open-1.0",
                ),
                (
                    "Files API",
                    "GET",
                    "https://huggingface.co/api/models/stabilityai/stable-audio-open-1.0/tree/main",
                ),
            ]

            access_results = []
            for test_name, method, url in model_tests:
                try:
                    if method == "HEAD":
                        test_response = requests.head(url, headers=headers, timeout=10)
                    else:
                        test_response = requests.get(url, headers=headers, timeout=10)

                    access_results.append(f"{test_name}: {test_response.status_code}")
                    self.log(f"{test_name}: HTTP {test_response.status_code}")

                    if test_response.status_code == 200:
                        messagebox.showinfo(
                            "Token Valid",
                            f"✅ Token verified!\n"
                            f"User: {username}\n"
                            f"✅ Stable Audio access confirmed via {test_name}",
                        )
                        self.save_hf_token()
                        return

                except Exception as e:
                    access_results.append(f"{test_name}: Error - {str(e)[:50]}")
                    self.log(f"{test_name}: Error - {e}")

            if any("403" in result for result in access_results):
                messagebox.showwarning(
                    "Access Required",
                    f"✅ Token is valid for user: {username}\n"
                    f"❌ Access to Stable Audio model required\n\n"
                    f"Steps to fix:\n"
                    f"1. Go to: https://huggingface.co/stabilityai/stable-audio-open-1.0\n"
                    f"2. Click 'Request access'\n"
                    f"3. Wait for approval (usually instant)\n"
                    f"4. Create a NEW token with 'read' permissions\n\n"
                    f"Debug info:\n" + "\n".join(access_results),
                )
            else:
                messagebox.showerror(
                    "Access Issues",
                    f"✅ Token is valid for user: {username}\n"
                    f"❌ Cannot access Stable Audio model\n\n"
                    f"Debug info:\n" + "\n".join(access_results),
                )

        except requests.exceptions.Timeout:
            messagebox.showerror(
                "Timeout", "❌ Request timed out. Check your internet connection."
            )
        except requests.exceptions.ConnectionError:
            messagebox.showerror(
                "Connection Error", "❌ Could not connect to Hugging Face."
            )
        except Exception as e:
            messagebox.showerror("Verification Error", f"Could not verify token:\n{e}")

    def create_config_tab(self):
        config_frame = ttk.Frame(self.notebook, padding="20")
        self.notebook.add(config_frame, text="⚙️ Configuration")

        canvas = tk.Canvas(config_frame, highlightthickness=0, bd=0)
        scrollbar = ttk.Scrollbar(config_frame, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)

        self.create_path_management_section(scrollable_frame)

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

        def _on_canvas_mousewheel(event):
            canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")

        def _on_listbox_mousewheel(event):
            self.api_keys_listbox.yview_scroll(int(-1 * (event.delta / 120)), "units")

        def _bind_canvas_mousewheel(event):
            canvas.bind_all("<MouseWheel>", _on_canvas_mousewheel)

        def _unbind_canvas_mousewheel(event):
            canvas.unbind_all("<MouseWheel>")

        def _bind_listbox_mousewheel(event):
            canvas.unbind_all("<MouseWheel>")
            self.api_keys_listbox.bind_all("<MouseWheel>", _on_listbox_mousewheel)

        def _unbind_listbox_mousewheel(event):
            self.api_keys_listbox.unbind_all("<MouseWheel>")
            canvas.bind_all("<MouseWheel>", _on_canvas_mousewheel)

        canvas.bind("<Enter>", _bind_canvas_mousewheel)
        canvas.bind("<Leave>", _unbind_canvas_mousewheel)

        api_frame = ttk.LabelFrame(
            scrollable_frame, text="API Keys Management", padding="15"
        )
        api_frame.pack(fill="x", pady=(0, 15), padx=10)

        ttk.Label(api_frame, text="API Keys:").pack(anchor="w")

        list_frame = ttk.Frame(api_frame)
        list_frame.pack(fill="x", pady=(5, 10))

        list_scroll = ttk.Scrollbar(list_frame, orient="vertical")
        self.api_keys_listbox = tk.Listbox(
            list_frame,
            height=16,
            yscrollcommand=list_scroll.set,
            font=("Consolas", 9),
            selectmode=tk.EXTENDED,
        )
        list_scroll.config(command=self.api_keys_listbox.yview)

        self.api_keys_listbox.pack(side="left", fill="both", expand=True)
        list_scroll.pack(side="right", fill="y")

        self.api_keys_listbox.bind("<Enter>", _bind_listbox_mousewheel)
        self.api_keys_listbox.bind("<Leave>", _unbind_listbox_mousewheel)

        api_buttons_frame = ttk.Frame(api_frame)
        api_buttons_frame.pack(fill="x", pady=(0, 10))

        ttk.Button(
            api_buttons_frame,
            text="🔑 Generate New Key",
            command=self.generate_api_key_with_options,
        ).pack(side="left", padx=(0, 5))
        ttk.Button(
            api_buttons_frame, text="📋 Add Custom Key", command=self.add_custom_api_key
        ).pack(side="left", padx=(0, 5))
        ttk.Button(
            api_buttons_frame, text="✏️ Edit Selected", command=self.edit_api_key
        ).pack(side="left", padx=(0, 5))
        ttk.Button(
            api_buttons_frame, text="🗑 Remove Selected", command=self.remove_api_keys
        ).pack(side="left", padx=(0, 5))
        ttk.Button(
            api_buttons_frame, text="📋 Copy Selected", command=self.copy_api_key
        ).pack(side="left")

        hf_frame = ttk.LabelFrame(
            scrollable_frame, text="Hugging Face Configuration", padding="15"
        )
        hf_frame.pack(fill="x", pady=(0, 15), padx=10)

        ttk.Label(hf_frame, text="Access Token (for Stable Audio model):").pack(
            anchor="w"
        )

        hf_token_frame = ttk.Frame(hf_frame)
        hf_token_frame.pack(fill="x", pady=(5, 10))

        self.hf_show_token = tk.BooleanVar(value=False)

        def update_hf_display():
            if self.hf_show_token.get():
                hf_entry.config(show="")
                hf_show_btn.config(text="🙈 Hide")
            else:
                hf_entry.config(show="*")
                hf_show_btn.config(text="👁 Show")

        hf_entry = ttk.Entry(
            hf_token_frame,
            textvariable=self.hf_token_var,
            font=("Consolas", 10),
            show="*",
            width=50,
        )
        hf_entry.pack(side="left", fill="x", expand=True)

        def on_hf_token_change(*args):
            if hasattr(self, "auto_save_enabled") and self.auto_save_enabled:
                if hasattr(self, "_hf_save_timer"):
                    self.root.after_cancel(self._hf_save_timer)
                self._hf_save_timer = self.root.after(2000, self.save_hf_token)

        self.hf_token_var.trace_add("write", on_hf_token_change)
        hf_show_btn = ttk.Button(
            hf_token_frame,
            text="👁 Show",
            width=8,
            command=lambda: [
                self.hf_show_token.set(not self.hf_show_token.get()),
                update_hf_display(),
            ],
        )
        hf_show_btn.pack(side="left", padx=(5, 5))
        ttk.Button(
            hf_token_frame, text="🔍 Verify", command=self.verify_hf_token_config
        ).pack(side="left")

        hf_help_text = "Get your token at: https://huggingface.co/settings/tokens\nRequest access to: https://huggingface.co/stabilityai/stable-audio-open-1.0"
        ttk.Label(
            hf_frame, text=hf_help_text, font=("Arial", 9), foreground="gray"
        ).pack(anchor="w", pady=(5, 0))

        model_frame = ttk.LabelFrame(
            scrollable_frame, text="Model Configuration", padding="15"
        )
        model_frame.pack(fill="x", pady=(0, 15), padx=10)

        ttk.Label(model_frame, text="LLM Model Path:").pack(anchor="w")
        model_path_frame = ttk.Frame(model_frame)
        model_path_frame.pack(fill="x", pady=(5, 10))

        ttk.Entry(model_path_frame, textvariable=self.model_path, width=50).pack(
            side="left", fill="x", expand=True
        )
        ttk.Button(
            model_path_frame, text="Browse", command=self.browse_model_path
        ).pack(side="right", padx=(5, 0))

        ttk.Label(model_frame, text="Audio Model:").pack(anchor="w")
        ttk.Label(
            model_frame,
            text="(Only these two models are currently supported)",
            font=("Arial", 9),
            foreground="gray",
        ).pack(anchor="w", pady=(0, 5))

        audio_model_combo = ttk.Combobox(
            model_frame,
            textvariable=self.audio_model,
            values=[
                "stabilityai/stable-audio-open-1.0",
                "stabilityai/stable-audio-open-small",
            ],
        )
        audio_model_combo.pack(fill="x", pady=(5, 0))

        server_frame = ttk.LabelFrame(
            scrollable_frame, text="Server Configuration", padding="15"
        )
        server_frame.pack(fill="x", pady=(0, 15), padx=10)

        host_port_frame = ttk.Frame(server_frame)
        host_port_frame.pack(fill="x", pady=(0, 10))

        ttk.Label(host_port_frame, text="Host:").pack(side="left")
        ttk.Entry(host_port_frame, textvariable=self.host, width=15).pack(
            side="left", padx=(5, 20)
        )

        ttk.Label(host_port_frame, text="Port:").pack(side="left")
        ttk.Entry(host_port_frame, textvariable=self.port, width=10).pack(
            side="left", padx=(5, 0)
        )

        ttk.Label(server_frame, text="Environment:").pack(anchor="w")
        env_combo = ttk.Combobox(
            server_frame,
            textvariable=self.environment,
            values=["dev", "prod"],
            state="readonly",
        )
        env_combo.pack(fill="x", pady=(5, 0))

        button_frame = ttk.Frame(scrollable_frame)
        button_frame.pack(fill="x", padx=10, pady=15)

        ttk.Button(
            button_frame, text="💾 Save Configuration", command=self.manual_save_config
        ).pack(side="left", padx=(0, 10))

        ttk.Button(
            button_frame, text="🔄 Reload Configuration", command=self.reload_config
        ).pack(side="left", padx=(0, 10))

        ttk.Button(
            button_frame, text="🗑 Clear All Data", command=self.clear_all_data
        ).pack(side="left", padx=(0, 10))

        ttk.Button(
            button_frame, text="🔧 Reset to Defaults", command=self.reset_to_defaults
        ).pack(side="left")

        bypass_frame = ttk.Frame(server_frame)
        bypass_frame.pack(fill="x", pady=(10, 0))

        self.bypass_checkbox = ttk.Checkbutton(
            bypass_frame,
            text="🚀 Bypass LLM (Direct Audio Generation)",
            variable=self.bypass_llm,
        )
        self.bypass_checkbox.pack(anchor="w")

        bypass_help = ttk.Label(
            bypass_frame,
            text="Skip AI brain analysis, use raw prompts directly with Stable Audio",
            font=("Arial", 9),
            foreground="gray",
        )
        bypass_help.pack(anchor="w", pady=(2, 0))

    def remove_api_keys(self):
        selections = self.api_keys_listbox.curselection()
        if not selections:
            messagebox.showwarning(
                "No Selection", "Please select one or more API keys to remove"
            )
            return

        count = len(selections)
        if count == 1:
            self.remove_api_key()
            return

        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()

            cursor.execute(
                "SELECT id, key_value_encrypted, name FROM api_keys ORDER BY created_at"
            )
            rows = cursor.fetchall()

            keys_to_delete = []
            for index in selections:
                if index < len(rows):
                    db_id, encrypted_key, name = rows[index]
                    decrypted_key = self.secure_storage.decrypt(encrypted_key)
                    display_name = name if name else f"Key {index + 1}"
                    keys_to_delete.append(
                        {
                            "id": db_id,
                            "name": display_name,
                            "preview": f"{decrypted_key[:8]}...{decrypted_key[-4:]}",
                        }
                    )

            if not keys_to_delete:
                conn.close()
                messagebox.showerror("Error", "Invalid selection")
                return

            key_list = "\n".join(
                [f"• {key['name']} ({key['preview']})" for key in keys_to_delete]
            )
            if not messagebox.askyesno(
                "Confirm Multiple Deletion",
                f"Are you sure you want to delete {count} API keys?\n\n{key_list}",
            ):
                conn.close()
                return

            deleted_count = 0
            for key_info in keys_to_delete:
                cursor.execute("DELETE FROM api_keys WHERE id = ?", (key_info["id"],))
                deleted_count += cursor.rowcount

            conn.commit()
            conn.close()

            if deleted_count > 0:
                self.load_api_keys()
                self.log(f"{deleted_count} API key(s) removed successfully", "SUCCESS")
            else:
                messagebox.showerror("Error", "Could not remove API keys from database")

        except Exception as e:
            self.log(f"Error removing API keys: {e}", "ERROR")
            messagebox.showerror("Error", f"Could not remove API keys: {e}")

    def manual_save_config(self):
        if self.save_config():
            messagebox.showinfo("Saved", "Configuration saved successfully!")

    def reload_config(self):
        self.load_config()
        self.load_api_keys()
        messagebox.showinfo("Reloaded", "Configuration reloaded from database!")

    def clear_all_data(self):
        if messagebox.askyesno(
            "Clear All Data",
            "This will delete all API keys and configuration.\nAre you sure?",
        ):
            try:
                conn = sqlite3.connect(self.db_path)
                cursor = conn.cursor()

                cursor.execute("DELETE FROM api_keys")
                cursor.execute("DELETE FROM config")

                conn.commit()
                conn.close()

                self.api_keys_list = []
                self.update_api_keys_listbox()
                self.reset_to_defaults()

                self.log("All data cleared from database", "SUCCESS")
                messagebox.showinfo("Cleared", "All data has been cleared!")

            except Exception as e:
                self.log(f"Error clearing data: {e}", "ERROR")
                messagebox.showerror("Error", f"Could not clear data: {e}")

    def init_database(self):
        config_dir = get_config_db_path()

        config_dir.mkdir(parents=True, exist_ok=True)
        self.db_path = config_dir / "config.db"

        self.log(f"Config database: {self.db_path}")
        self.secure_storage = SecureStorage(self.db_path)
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()

        cursor.execute(
            """
            CREATE TABLE IF NOT EXISTS api_keys (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                key_value_encrypted TEXT UNIQUE NOT NULL,
                name TEXT,
                is_limited INTEGER DEFAULT 1,
                is_expired INTEGER DEFAULT 0,
                total_credits INTEGER DEFAULT 50,
                credits_used INTEGER DEFAULT 0,
                date_of_expiration TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                last_used TIMESTAMP
            )
        """
        )

        cursor.execute(
            """
            CREATE TABLE IF NOT EXISTS config (
                key TEXT PRIMARY KEY,
                value TEXT,
                is_encrypted INTEGER DEFAULT 0
            )
        """
        )

        cursor.execute("PRAGMA table_info(api_keys)")
        existing_columns = [column[1] for column in cursor.fetchall()]

        new_columns = [
            ("is_limited", "INTEGER DEFAULT 1"),
            ("is_expired", "INTEGER DEFAULT 0"),
            ("total_credits", "INTEGER DEFAULT 50"),
            ("credits_used", "INTEGER DEFAULT 0"),
            ("date_of_expiration", "TEXT"),
        ]

        for col_name, col_def in new_columns:
            if col_name not in existing_columns:
                cursor.execute(f"ALTER TABLE api_keys ADD COLUMN {col_name} {col_def}")
                self.log(f"Added column {col_name} to api_keys table", "SUCCESS")

        cursor.execute("PRAGMA table_info(api_keys)")
        api_columns = [column[1] for column in cursor.fetchall()]

        if "key_value" in api_columns:
            self.log("Migrating API keys to encrypted storage...", "WARNING")
            cursor.execute("SELECT key_value, name FROM api_keys")
            old_data = cursor.fetchall()

            cursor.execute("DROP TABLE api_keys")
            cursor.execute(
                """
                CREATE TABLE api_keys (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    key_value_encrypted TEXT UNIQUE NOT NULL,
                    name TEXT,
                    is_limited INTEGER DEFAULT 1,
                    is_expired INTEGER DEFAULT 0,
                    total_credits INTEGER DEFAULT 50,
                    credits_used INTEGER DEFAULT 0,
                    date_of_expiration TEXT,
                    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    last_used TIMESTAMP
                )
            """
            )

            for key_value, name in old_data:
                encrypted_key = self.secure_storage.encrypt(key_value)
                cursor.execute(
                    """INSERT INTO api_keys 
                    (key_value_encrypted, name, is_limited, total_credits, credits_used) 
                    VALUES (?, ?, 1, 50, 0)""",
                    (encrypted_key, name),
                )

            self.log(
                f"Migrated {len(old_data)} API keys to encrypted storage with credit system",
                "SUCCESS",
            )

        cursor.execute("PRAGMA table_info(config)")
        config_columns = [column[1] for column in cursor.fetchall()]

        if "is_encrypted" not in config_columns:
            self.log("Adding encryption support to config table...", "WARNING")
            cursor.execute(
                "ALTER TABLE config ADD COLUMN is_encrypted INTEGER DEFAULT 0"
            )
            self.log("Config table updated with encryption support", "SUCCESS")

        conn.commit()
        conn.close()
        self.log(f"Secure database initialized at: {self.db_path}")

    def generate_api_key_with_options(self):
        dialog = tk.Toplevel(self.root)
        dialog.title("Generate API Key")
        dialog.geometry("500x400")
        dialog.transient(self.root)
        dialog.grab_set()

        frame = ttk.Frame(dialog, padding="20")
        frame.pack(fill="both", expand=True)

        ttk.Label(frame, text="Key Name:").pack(anchor="w")
        name_var = tk.StringVar(value=f"API Key {len(self.api_keys_list) + 1}")
        ttk.Entry(frame, textvariable=name_var, width=40).pack(fill="x", pady=(5, 15))

        ttk.Label(frame, text="Key Type:").pack(anchor="w")
        is_limited = tk.BooleanVar(value=True)
        ttk.Radiobutton(
            frame,
            text="🔒 Limited (with credit limit)",
            variable=is_limited,
            value=True,
        ).pack(anchor="w")
        ttk.Radiobutton(
            frame, text="🔓 Unlimited (no limits)", variable=is_limited, value=False
        ).pack(anchor="w", pady=(0, 15))

        limited_frame = ttk.LabelFrame(frame, text="Limited Key Options", padding="10")
        limited_frame.pack(fill="x", pady=(0, 15))

        ttk.Label(limited_frame, text="Total Credits:").pack(anchor="w")
        credits_var = tk.StringVar(value="50")
        ttk.Entry(limited_frame, textvariable=credits_var, width=10).pack(
            anchor="w", pady=(5, 10)
        )

        ttk.Label(limited_frame, text="Expiration Date (optional):").pack(anchor="w")
        exp_var = tk.StringVar()
        exp_frame = ttk.Frame(limited_frame)
        exp_frame.pack(fill="x", pady=(5, 0))
        ttk.Entry(exp_frame, textvariable=exp_var, width=20).pack(side="left")
        ttk.Label(exp_frame, text="Format: YYYY-MM-DD", font=("Arial", 8)).pack(
            side="left", padx=(10, 0)
        )

        def toggle_limited_options():
            if is_limited.get():
                for child in limited_frame.winfo_children():
                    child.configure(state="normal")
            else:
                for child in limited_frame.winfo_children():
                    if hasattr(child, "configure"):
                        child.configure(state="disabled")

        is_limited.trace_add("write", lambda *args: toggle_limited_options())

        def create_key():
            name = name_var.get().strip()
            if not name:
                messagebox.showerror("Error", "Please enter a key name")
                return

            api_key = self.generate_unique_api_key()

            total_credits = 0 if not is_limited.get() else int(credits_var.get() or 50)
            exp_date = exp_var.get().strip() if is_limited.get() else None

            if self.save_api_key_advanced(
                api_key, name, is_limited.get(), total_credits, exp_date
            ):
                self.load_api_keys()
                self.show_api_key_dialog(api_key, f"Generated: {name}")
                self.log(
                    f"API key '{name}' created: {'Limited' if is_limited.get() else 'Unlimited'}",
                    "SUCCESS",
                )
                dialog.destroy()

        button_frame = ttk.Frame(frame)
        button_frame.pack(fill="x", pady=(20, 0))
        ttk.Button(button_frame, text="Generate Key", command=create_key).pack(
            side="right"
        )
        ttk.Button(button_frame, text="Cancel", command=dialog.destroy).pack(
            side="right", padx=(0, 10)
        )

    def save_api_key_advanced(
        self,
        api_key,
        name=None,
        is_limited=True,
        total_credits=50,
        expiration_date=None,
    ):
        try:
            encrypted_key = self.secure_storage.encrypt(api_key)

            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()

            cursor.execute(
                """
                INSERT OR IGNORE INTO api_keys 
                (key_value_encrypted, name, is_limited, is_expired, total_credits, credits_used, date_of_expiration) 
                VALUES (?, ?, ?, ?, ?, ?, ?)
            """,
                (
                    encrypted_key,
                    name,
                    int(is_limited),
                    0,
                    total_credits,
                    0,
                    expiration_date,
                ),
            )

            conn.commit()
            conn.close()
            return True

        except Exception as e:
            self.log(f"Error saving API key: {e}", "ERROR")
            return False

    def create_logs_tab(self):
        logs_frame = ttk.Frame(self.notebook, padding="20")
        self.notebook.add(logs_frame, text="📝 Logs")

        controls_frame = ttk.Frame(logs_frame)
        controls_frame.pack(fill="x", pady=(0, 10))

        ttk.Button(controls_frame, text="🗑 Clear Logs", command=self.clear_logs).pack(
            side="left", padx=(0, 10)
        )

        ttk.Button(controls_frame, text="💾 Save Logs", command=self.save_logs).pack(
            side="left", padx=(0, 10)
        )

        self.auto_scroll = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            controls_frame, text="Auto-scroll", variable=self.auto_scroll
        ).pack(side="left", padx=(10, 0))

        self.log_text = scrolledtext.ScrolledText(
            logs_frame, height=20, font=("Consolas", 9)
        )
        self.log_text.pack(fill="both", expand=True)

        self.log_text.tag_configure("INFO", foreground="black")
        self.log_text.tag_configure("SUCCESS", foreground="dark green")
        self.log_text.tag_configure("WARNING", foreground="orange")
        self.log_text.tag_configure("ERROR", foreground="red")

    def log(self, message, level="INFO"):
        timestamp = time.strftime("%H:%M:%S")
        emoji = {"INFO": "ℹ️ ", "SUCCESS": "✅", "WARNING": "⚠️ ", "ERROR": "❌"}.get(
            level, "ℹ️"
        )
        formatted_msg = f"[{timestamp}] {emoji} {message}\n"
        print(f"{formatted_msg.strip()}")
        try:
            if (
                self.log_text is not None
                and not self.is_in_tray
                and self.root.winfo_exists()
            ):

                self.log_text.insert(tk.END, formatted_msg, level)

                if hasattr(self, "auto_scroll") and self.auto_scroll.get():
                    self.log_text.see(tk.END)

                self.root.update_idletasks()
        except:
            pass

    def load_api_keys(self):
        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()

            cursor.execute(
                "SELECT key_value_encrypted FROM api_keys ORDER BY created_at"
            )
            rows = cursor.fetchall()

            self.api_keys_list = []
            for row in rows:
                decrypted_key = self.secure_storage.decrypt(row[0])
                if decrypted_key:
                    self.api_keys_list.append(decrypted_key)

            self.update_api_keys_listbox()
            conn.close()

            if self.api_keys_list:
                self.log(f"Loaded {len(self.api_keys_list)} encrypted API key(s)")

        except Exception as e:
            self.log(f"Error loading API keys: {e}", "WARNING")
            self.api_keys_list = []

    def remove_api_key(self):
        selection = self.api_keys_listbox.curselection()
        if not selection:
            messagebox.showwarning("No Selection", "Please select an API key to remove")
            return

        index = selection[0]

        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()

            cursor.execute(
                "SELECT id, key_value_encrypted, name FROM api_keys ORDER BY created_at"
            )
            rows = cursor.fetchall()

            if index >= len(rows):
                conn.close()
                messagebox.showerror("Error", "Invalid selection")
                return

            db_id, encrypted_key, name = rows[index]

            decrypted_key = self.secure_storage.decrypt(encrypted_key)
            display_name = name if name else f"Key {index + 1}"

            if not messagebox.askyesno(
                "Confirm Deletion",
                f"Are you sure you want to delete:\n'{display_name}'\n{decrypted_key[:8]}...{decrypted_key[-4:]}?",
            ):
                conn.close()
                return

            cursor.execute("DELETE FROM api_keys WHERE id = ?", (db_id,))
            deleted_count = cursor.rowcount

            conn.commit()
            conn.close()

            if deleted_count > 0:
                self.load_api_keys()
                self.log(f"API key '{display_name}' removed successfully", "SUCCESS")
            else:
                messagebox.showerror("Error", "Could not remove API key from database")

        except Exception as e:
            self.log(f"Error removing API key: {e}", "ERROR")
            messagebox.showerror("Error", f"Could not remove API key: {e}")

    def remove_api_key_from_db(self, api_key):
        try:
            encrypted_key = self.secure_storage.encrypt(api_key)

            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()

            cursor.execute(
                "DELETE FROM api_keys WHERE key_value_encrypted = ?", (encrypted_key,)
            )
            deleted_count = cursor.rowcount

            conn.commit()
            conn.close()

            if deleted_count > 0:
                self.log(f"Removed {deleted_count} API key from database")
                return True
            else:
                self.log("No matching API key found in database", "WARNING")
                return False

        except Exception as e:
            self.log(f"Error removing API key: {e}", "ERROR")
            return False

    def save_config(self):
        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()

            config_items = [
                ("model_path", self.model_path.get(), 0),
                ("environment", self.environment.get(), 0),
                ("host", self.host.get(), 0),
                ("port", self.port.get(), 0),
                ("audio_model", self.audio_model.get(), 0),
                ("bypass_llm", str(self.bypass_llm.get()), 0),
            ]

            if hasattr(self, "hf_token_var") and self.hf_token_var.get().strip():
                self.save_hf_token()

            for key, value, is_encrypted in config_items:
                cursor.execute(
                    "INSERT OR REPLACE INTO config (key, value, is_encrypted) VALUES (?, ?, ?)",
                    (key, value, is_encrypted),
                )

            conn.commit()
            conn.close()

            return True
        except Exception as e:
            self.log(f"Error saving configuration: {e}", "ERROR")
            return False

    def load_config(self):
        try:
            old_auto_save = self.auto_save_enabled
            self.auto_save_enabled = False
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()

            cursor.execute("SELECT key, value, is_encrypted FROM config")
            rows = cursor.fetchall()

            for key, value, is_encrypted in rows:
                if is_encrypted:
                    decrypted_value = self.secure_storage.decrypt(value)
                    if key == "hf_token" and hasattr(self, "hf_token_var"):
                        self.hf_token_var.set(decrypted_value)
                else:
                    if key == "model_path":
                        self.model_path.set(value)
                    elif key == "environment":
                        self.environment.set(value)
                    elif key == "host":
                        self.host.set(value)
                    elif key == "port":
                        self.port.set(value)
                    elif key == "audio_model":
                        self.audio_model.set(value)
                    elif key == "bypass_llm":
                        self.bypass_llm.set(value.lower() == "true")

            conn.close()

            def restore_auto_save():
                self.auto_save_enabled = old_auto_save
                self.log("Auto-save re-enabled after config load")

            self.root.after(1000, restore_auto_save)
            self.log(f"Configuration loaded with encryption support")

        except Exception as e:
            self.log(f"Error loading configuration: {e}", "WARNING")

    def add_custom_api_key(self):
        dialog = tk.Toplevel(self.root)
        dialog.title("Add Custom API Key")
        dialog.geometry("500x450")
        dialog.transient(self.root)
        dialog.grab_set()

        dialog.update_idletasks()
        x = (dialog.winfo_screenwidth() // 2) - (500 // 2)
        y = (dialog.winfo_screenheight() // 2) - (450 // 2)
        dialog.geometry(f"500x450+{x}+{y}")

        frame = ttk.Frame(dialog, padding="20")
        frame.pack(fill="both", expand=True)

        ttk.Label(frame, text="Key Name:").pack(anchor="w")
        key_name_var = tk.StringVar(value="Custom Key")
        name_entry = ttk.Entry(frame, textvariable=key_name_var, width=50)
        name_entry.pack(fill="x", pady=(5, 15))

        ttk.Label(frame, text="API Key:").pack(anchor="w")
        key_var = tk.StringVar()

        key_frame = ttk.Frame(frame)
        key_frame.pack(fill="x", pady=(5, 15))

        entry = ttk.Entry(key_frame, textvariable=key_var, width=40, show="*")
        entry.pack(side="left", fill="x", expand=True)
        entry.focus()

        show_custom = tk.BooleanVar(value=False)

        def toggle_show():
            if show_custom.get():
                entry.config(show="")
                show_btn.config(text="🙈 Hide")
            else:
                entry.config(show="*")
                show_btn.config(text="👁 Show")

        show_btn = ttk.Button(
            key_frame,
            text="👁 Show",
            width=8,
            command=lambda: [show_custom.set(not show_custom.get()), toggle_show()],
        )
        show_btn.pack(side="left", padx=(5, 0))

        options_frame = ttk.LabelFrame(frame, text="Key Options", padding="10")
        options_frame.pack(fill="x", pady=(0, 15))

        is_limited = tk.BooleanVar(value=True)
        ttk.Radiobutton(
            options_frame,
            text="🔒 Limited (with credit limit)",
            variable=is_limited,
            value=True,
        ).pack(anchor="w")
        ttk.Radiobutton(
            options_frame,
            text="🔓 Unlimited (no limits)",
            variable=is_limited,
            value=False,
        ).pack(anchor="w", pady=(0, 10))

        limited_frame = ttk.Frame(options_frame)
        limited_frame.pack(fill="x")

        credits_frame = ttk.Frame(limited_frame)
        credits_frame.pack(fill="x", pady=(0, 10))
        ttk.Label(credits_frame, text="Total Credits:").pack(side="left")
        credits_var = tk.StringVar(value="50")
        credits_entry = ttk.Entry(credits_frame, textvariable=credits_var, width=10)
        credits_entry.pack(side="left", padx=(10, 0))

        exp_frame = ttk.Frame(limited_frame)
        exp_frame.pack(fill="x")
        ttk.Label(exp_frame, text="Expiration Date (optional):").pack(side="left")
        exp_var = tk.StringVar()
        exp_entry = ttk.Entry(exp_frame, textvariable=exp_var, width=15)
        exp_entry.pack(side="left", padx=(10, 5))
        ttk.Label(
            exp_frame, text="YYYY-MM-DD", font=("Arial", 8), foreground="gray"
        ).pack(side="left")

        def toggle_limited_options():
            state = "normal" if is_limited.get() else "disabled"
            credits_entry.configure(state=state)
            exp_entry.configure(state=state)

        is_limited.trace_add("write", lambda *args: toggle_limited_options())

        button_frame = ttk.Frame(frame)
        button_frame.pack(fill="x", pady=(20, 0))

        def add_key():
            key = key_var.get().strip()
            name = key_name_var.get().strip()

            if not name:
                messagebox.showerror("Error", "Please enter a key name")
                return

            if not key:
                messagebox.showerror("Error", "Please enter a valid API key")
                return

            if key in self.api_keys_list:
                messagebox.showwarning("Duplicate", "This API key already exists")
                return

            limited = is_limited.get()
            total_credits = 0 if not limited else int(credits_var.get() or 50)
            exp_date = exp_var.get().strip() if limited else None

            if exp_date:
                try:
                    from datetime import datetime

                    datetime.fromisoformat(exp_date)
                except ValueError:
                    messagebox.showerror("Error", "Invalid date format. Use YYYY-MM-DD")
                    return

            if self.save_api_key_advanced(key, name, limited, total_credits, exp_date):
                self.api_keys_list.append(key)
                self.update_api_keys_listbox()
                key_type = "Limited" if limited else "Unlimited"
                self.log(
                    f"Custom API key '{name}' added: {key[:8]}... ({key_type})",
                    "SUCCESS",
                )
                dialog.destroy()
            else:
                messagebox.showerror("Error", "Could not save API key to database")

        ttk.Button(button_frame, text="Add Key", command=add_key).pack(side="right")
        ttk.Button(button_frame, text="Cancel", command=dialog.destroy).pack(
            side="right", padx=(0, 10)
        )

        entry.bind("<Return>", lambda e: add_key())

    def edit_api_key(self):
        selection = self.api_keys_listbox.curselection()
        if not selection:
            messagebox.showwarning("No Selection", "Please select an API key to edit")
            return

        index = selection[0]

        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()

            cursor.execute(
                """
                SELECT id, key_value_encrypted, name, is_limited, is_expired, 
                    total_credits, credits_used, date_of_expiration 
                FROM api_keys ORDER BY created_at
            """
            )
            rows = cursor.fetchall()

            if index >= len(rows):
                conn.close()
                messagebox.showerror("Error", "Invalid selection")
                return

            (
                db_id,
                encrypted_key,
                current_name,
                is_limited,
                is_expired,
                total_credits,
                credits_used,
                exp_date,
            ) = rows[index]
            decrypted_key = self.secure_storage.decrypt(encrypted_key)

            dialog = tk.Toplevel(self.root)
            dialog.title("Edit API Key")
            dialog.geometry("550x700")
            dialog.transient(self.root)
            dialog.grab_set()

            dialog.update_idletasks()
            x = (dialog.winfo_screenwidth() // 2) - (550 // 2)
            y = (dialog.winfo_screenheight() // 2) - (700 // 2)
            dialog.geometry(f"550x700+{x}+{y}")

            frame = ttk.Frame(dialog, padding="20")
            frame.pack(fill="both", expand=True)

            ttk.Label(frame, text="API Key:", font=("Arial", 10, "bold")).pack(
                anchor="w"
            )
            key_display = ttk.Label(
                frame,
                text=f"{decrypted_key[:12]}...{decrypted_key[-6:]}",
                font=("Consolas", 10),
                foreground="gray",
            )
            key_display.pack(anchor="w", pady=(0, 15))

            ttk.Label(frame, text="Key Name:").pack(anchor="w")
            name_var = tk.StringVar(value=current_name or f"Key {index + 1}")
            ttk.Entry(frame, textvariable=name_var, width=50).pack(
                fill="x", pady=(5, 15)
            )

            status_frame = ttk.LabelFrame(frame, text="Key Status", padding="10")
            status_frame.pack(fill="x", pady=(0, 15))

            expired_var = tk.BooleanVar(value=bool(is_expired))
            ttk.Checkbutton(
                status_frame, text="🔴 Mark as Expired", variable=expired_var
            ).pack(anchor="w")

            type_frame = ttk.LabelFrame(frame, text="Key Type", padding="10")
            type_frame.pack(fill="x", pady=(0, 15))

            limited_var = tk.BooleanVar(value=bool(is_limited))
            ttk.Radiobutton(
                type_frame,
                text="🔒 Limited (with credit limit)",
                variable=limited_var,
                value=True,
            ).pack(anchor="w")
            ttk.Radiobutton(
                type_frame,
                text="🔓 Unlimited (no limits)",
                variable=limited_var,
                value=False,
            ).pack(anchor="w")

            credits_frame = ttk.LabelFrame(
                frame, text="Credit Management", padding="10"
            )
            credits_frame.pack(fill="x", pady=(0, 15))

            total_frame = ttk.Frame(credits_frame)
            total_frame.pack(fill="x", pady=(0, 10))
            ttk.Label(total_frame, text="Total Credits:").pack(side="left")
            total_var = tk.StringVar(value=str(total_credits or 50))
            total_entry = ttk.Entry(total_frame, textvariable=total_var, width=10)
            total_entry.pack(side="left", padx=(10, 0))

            used_frame = ttk.Frame(credits_frame)
            used_frame.pack(fill="x", pady=(0, 10))
            ttk.Label(used_frame, text="Credits Used:").pack(side="left")
            used_var = tk.StringVar(value=str(credits_used or 0))
            used_entry = ttk.Entry(used_frame, textvariable=used_var, width=10)
            used_entry.pack(side="left", padx=(10, 0))

            actions_frame = ttk.Frame(credits_frame)
            actions_frame.pack(fill="x")

            def reset_credits():
                used_var.set("0")

            def add_credits():
                try:
                    current = int(total_var.get() or 0)
                    total_var.set(str(current + 50))
                except:
                    pass

            ttk.Button(actions_frame, text="🔄 Reset Used", command=reset_credits).pack(
                side="left", padx=(0, 5)
            )
            ttk.Button(actions_frame, text="➕ Add 50", command=add_credits).pack(
                side="left"
            )

            exp_frame = ttk.LabelFrame(frame, text="Expiration", padding="10")
            exp_frame.pack(fill="x", pady=(0, 15))

            exp_var = tk.StringVar(value=exp_date or "")
            exp_entry_frame = ttk.Frame(exp_frame)
            exp_entry_frame.pack(fill="x")

            ttk.Label(exp_entry_frame, text="Expiration Date:").pack(side="left")
            exp_entry = ttk.Entry(exp_entry_frame, textvariable=exp_var, width=15)
            exp_entry.pack(side="left", padx=(10, 5))
            ttk.Label(
                exp_entry_frame,
                text="YYYY-MM-DD (empty = no expiration)",
                font=("Arial", 8),
                foreground="gray",
            ).pack(side="left")

            def clear_expiration():
                exp_var.set("")

            ttk.Button(
                exp_frame, text="🗑 Clear Expiration", command=clear_expiration
            ).pack(anchor="w", pady=(5, 0))

            def toggle_credit_controls():
                state = "normal" if limited_var.get() else "disabled"
                total_entry.configure(state=state)
                used_entry.configure(state=state)
                exp_entry.configure(state=state)

            limited_var.trace_add("write", lambda *args: toggle_credit_controls())
            toggle_credit_controls()

            button_frame = ttk.Frame(frame)
            button_frame.pack(fill="x", pady=(20, 0))

            def save_changes():
                try:
                    save_conn = sqlite3.connect(self.db_path)
                    save_cursor = save_conn.cursor()
                    name = name_var.get().strip()
                    if not name:
                        messagebox.showerror("Error", "Please enter a key name")
                        return

                    is_limited_new = limited_var.get()
                    is_expired_new = expired_var.get()

                    if is_limited_new:
                        try:
                            total_new = int(total_var.get() or 50)
                            used_new = int(used_var.get() or 0)
                            if used_new > total_new:
                                messagebox.showerror(
                                    "Error",
                                    "Credits used cannot be greater than total credits",
                                )
                                return
                        except ValueError:
                            messagebox.showerror(
                                "Error", "Credits must be valid numbers"
                            )
                            return
                    else:
                        total_new = 0
                        used_new = 0

                    exp_new = exp_var.get().strip() or None
                    if exp_new:
                        try:
                            from datetime import datetime

                            datetime.fromisoformat(exp_new)
                        except ValueError:
                            messagebox.showerror(
                                "Error", "Invalid date format. Use YYYY-MM-DD"
                            )
                            return

                    save_cursor.execute(
                        """
                        UPDATE api_keys 
                        SET name = ?, is_limited = ?, is_expired = ?, 
                            total_credits = ?, credits_used = ?, date_of_expiration = ?
                        WHERE id = ?
                    """,
                        (
                            name,
                            int(is_limited_new),
                            int(is_expired_new),
                            total_new,
                            used_new,
                            exp_new,
                            db_id,
                        ),
                    )

                    save_conn.commit()
                    save_conn.close()

                    self.update_api_keys_listbox()

                    status = (
                        "Expired"
                        if is_expired_new
                        else ("Unlimited" if not is_limited_new else "Limited")
                    )
                    self.log(f"API key '{name}' updated: {status}", "SUCCESS")
                    dialog.destroy()

                except Exception as e:
                    self.log(f"Error updating API key: {e}", "ERROR")
                    messagebox.showerror("Error", f"Could not update API key: {e}")

            ttk.Button(button_frame, text="💾 Save Changes", command=save_changes).pack(
                side="right"
            )
            ttk.Button(button_frame, text="Cancel", command=dialog.destroy).pack(
                side="right", padx=(0, 10)
            )

            conn.close()

        except Exception as e:
            self.log(f"Error editing API key: {e}", "ERROR")
            messagebox.showerror("Error", f"Could not edit API key: {e}")

    def copy_api_key(self):
        selection = self.api_keys_listbox.curselection()
        if selection:
            index = selection[0]
            api_key = self.api_keys_list[index]
            self.root.clipboard_clear()
            self.root.clipboard_append(api_key)
            self.log(f"API key copied to clipboard: {api_key[:8]}...", "SUCCESS")
            messagebox.showinfo(
                "Copied", f"API key copied to clipboard:\n{api_key[:16]}..."
            )
        else:
            messagebox.showwarning("No Selection", "Please select an API key to copy")

    def show_api_key_dialog(self, api_key, title):
        dialog = tk.Toplevel(self.root)
        dialog.title(title)
        dialog.geometry("600x300")
        dialog.transient(self.root)
        dialog.grab_set()

        dialog.update_idletasks()
        x = (dialog.winfo_screenwidth() // 2) - (600 // 2)
        y = (dialog.winfo_screenheight() // 2) - (300 // 2)
        dialog.geometry(f"600x300+{x}+{y}")

        frame = ttk.Frame(dialog, padding="20")
        frame.pack(fill="both", expand=True)

        ttk.Label(frame, text="Your API Key:", font=("Arial", 12, "bold")).pack(
            anchor="w"
        )

        warning_label = ttk.Label(
            frame,
            text="⚠️ Save this key securely - it won't be shown again in full!",
            foreground="red",
            font=("Arial", 10),
        )
        warning_label.pack(anchor="w", pady=(5, 15))

        key_frame = ttk.Frame(frame)
        key_frame.pack(fill="x", pady=(0, 20))

        key_entry = ttk.Entry(
            key_frame,
            font=("Consolas", 12),
            width=60,
        )
        key_entry.pack(fill="x", pady=(0, 10))

        key_entry.insert(0, api_key)

        def select_all():
            key_entry.select_range(0, tk.END)
            key_entry.focus()

        dialog.after(200, select_all)

        button_frame = ttk.Frame(frame)
        button_frame.pack(fill="x")

        def copy_and_close():
            self.root.clipboard_clear()
            self.root.clipboard_append(api_key)
            self.log("API key copied to clipboard", "SUCCESS")
            messagebox.showinfo("Copied!", "API key copied to clipboard!")
            dialog.destroy()

        def select_all_key():
            key_entry.select_range(0, tk.END)
            key_entry.focus()

        ttk.Button(button_frame, text="📋 Copy & Close", command=copy_and_close).pack(
            side="left", padx=(0, 10)
        )
        ttk.Button(button_frame, text="🔍 Select All", command=select_all_key).pack(
            side="left", padx=(0, 10)
        )
        ttk.Button(button_frame, text="Close", command=dialog.destroy).pack(side="left")

        key_entry.focus()

    def update_api_keys_listbox(self):
        self.api_keys_listbox.delete(0, tk.END)

        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()

            cursor.execute(
                """
                SELECT key_value_encrypted, name, is_limited, is_expired, 
                    total_credits, credits_used, date_of_expiration 
                FROM api_keys ORDER BY created_at
            """
            )
            rows = cursor.fetchall()

            self.api_keys_list = []

            for i, row in enumerate(rows):
                (
                    encrypted_key,
                    name,
                    is_limited,
                    is_expired,
                    total_credits,
                    credits_used,
                    exp_date,
                ) = row
                decrypted_key = self.secure_storage.decrypt(encrypted_key)

                if decrypted_key:
                    self.api_keys_list.append(decrypted_key)

                    # Créer l'affichage
                    display_name = name if name else f"Unnamed Key {i+1}"
                    key_preview = f"{decrypted_key[:8]}...{decrypted_key[-4:]}"

                    # Indicateurs de statut
                    status_indicators = []
                    if is_expired:
                        status_indicators.append("🔴 EXPIRED")
                    elif not is_limited:
                        status_indicators.append("🔓 UNLIMITED")
                    else:
                        remaining = total_credits - credits_used
                        if remaining <= 0:
                            status_indicators.append("❌ NO CREDITS")
                        elif remaining <= 5:
                            status_indicators.append(f"⚠️ {remaining} left")
                        else:
                            status_indicators.append(f"✅ {remaining}/{total_credits}")

                    status_text = " ".join(status_indicators)
                    display_text = f"[{display_name}] {key_preview} {status_text}"

                    self.api_keys_listbox.insert(tk.END, display_text)

            conn.close()

        except Exception as e:
            self.log(f"Error updating listbox: {e}", "ERROR")

    def generate_unique_api_key(self, length=32):
        import secrets
        import string

        alphabet = string.ascii_letters + string.digits

        while True:
            api_key = "".join(secrets.choice(alphabet) for _ in range(length))
            if not self.api_key_exists(api_key):
                return api_key
            self.log(
                f"Generated key collision (rare!), generating new one...", "WARNING"
            )

    def api_key_exists(self, api_key):
        return api_key in self.api_keys_list

    def browse_model_path(self):
        filename = filedialog.askopenfilename(
            title="Select LLM Model File",
            filetypes=[("GGUF files", "*.gguf"), ("All files", "*.*")],
            initialdir=self.model_path.get() if self.model_path.get() else ".",
        )
        if filename:
            self.model_path.set(filename)

    def reset_to_defaults(self):
        if messagebox.askyesno(
            "Reset Configuration",
            "Reset all settings to defaults? (API keys will be kept)",
        ):
            self.model_path.set("")
            self.environment.set("dev")
            self.host.set("127.0.0.1")
            self.port.set("8000")
            self.audio_model.set("stabilityai/stable-audio-open-1.0")
            self.log("Configuration reset to defaults")

            self.save_config()

    def is_git_repo(self):
        git_dir = self.installation_dir / ".git"
        return git_dir.exists() and git_dir.is_dir()

    def check_for_updates(self):
        if not self.is_git_repo():
            messagebox.showinfo(
                "Not a Git Installation",
                "This installation doesn't appear to be a git repository.\n"
                "Updates via git pull are only available for git installations.",
            )
            return False

        try:
            result = subprocess.run(
                ["git", "status", "--porcelain"],
                cwd=str(self.installation_dir),
                capture_output=True,
                text=True,
                check=True,
            )

            if result.stdout.strip():
                if not messagebox.askyesno(
                    "Uncommitted Changes",
                    "You have uncommitted local changes.\n\n"
                    "Pulling updates may cause conflicts.\n"
                    "Continue anyway?",
                ):
                    return False

            subprocess.run(["git", "fetch"], cwd=str(self.installation_dir), check=True)

            result = subprocess.run(
                ["git", "rev-list", "HEAD...origin/main", "--count"],
                cwd=str(self.installation_dir),
                capture_output=True,
                text=True,
                check=True,
            )

            commits_behind = int(result.stdout.strip())

            if commits_behind == 0:
                messagebox.showinfo(
                    "Up to Date", "You're already running the latest version!"
                )
                return False

            return commits_behind

        except subprocess.CalledProcessError as e:
            self.log(f"Git check failed: {e}", "ERROR")
            messagebox.showerror("Git Error", f"Could not check for updates:\n{e}")
            return False
        except Exception as e:
            self.log(f"Error checking updates: {e}", "ERROR")
            return False

    def install_dependencies(self):
        requirements_file = self.installation_dir / "requirements.txt"

        if not requirements_file.exists():
            messagebox.showerror(
                "Requirements Not Found",
                f"requirements.txt not found at:\n{requirements_file}",
            )
            return False

        if platform.system() == "Windows":
            pip_exe = self.installation_dir / "env" / "Scripts" / "pip.exe"
        else:
            pip_exe = self.installation_dir / "env" / "bin" / "pip"

        if not pip_exe.exists():
            messagebox.showerror(
                "Pip Not Found",
                f"Could not find pip in virtual environment:\n{pip_exe}",
            )
            return False

        if not messagebox.askyesno(
            "Install Dependencies",
            f"This will install/update packages from:\n{requirements_file}\n\n"
            f"Using pip: {pip_exe}\n\n"
            "Continue?",
        ):
            return False

        try:
            self.log("Installing dependencies...", "INFO")

            progress_window = tk.Toplevel(self.root)
            progress_window.title("Installing Dependencies")
            progress_window.geometry("600x400")
            progress_window.transient(self.root)
            progress_window.grab_set()

            progress_frame = ttk.Frame(progress_window, padding="20")
            progress_frame.pack(fill="both", expand=True)

            ttk.Label(
                progress_frame,
                text="Installing Python packages...",
                font=("Arial", 12, "bold"),
            ).pack(pady=(0, 10))

            output_text = scrolledtext.ScrolledText(
                progress_frame, height=15, font=("Consolas", 9)
            )
            output_text.pack(fill="both", expand=True, pady=(0, 10))

            status_label = ttk.Label(progress_frame, text="Working...")
            status_label.pack()

            def run_install():
                try:
                    cmd = [str(pip_exe), "install", "-r", str(requirements_file)]

                    if platform.system() == "Windows":
                        process = subprocess.Popen(
                            cmd,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT,
                            universal_newlines=True,
                            bufsize=1,
                            cwd=str(self.installation_dir),
                            creationflags=subprocess.CREATE_NO_WINDOW,
                        )
                    else:
                        process = subprocess.Popen(
                            cmd,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT,
                            universal_newlines=True,
                            bufsize=1,
                            cwd=str(self.installation_dir),
                        )

                    for line in iter(process.stdout.readline, ""):
                        if line:
                            self.root.after(
                                0, lambda l=line: output_text.insert(tk.END, l)
                            )
                            self.root.after(0, lambda: output_text.see(tk.END))

                    return_code = process.wait()

                    if return_code == 0:
                        self.root.after(
                            0,
                            lambda: status_label.config(
                                text="✅ Installation complete!", foreground="green"
                            ),
                        )
                        self.log("Dependencies installed successfully", "SUCCESS")
                    else:
                        self.root.after(
                            0,
                            lambda: status_label.config(
                                text="❌ Installation failed", foreground="red"
                            ),
                        )
                        self.log("Dependencies installation failed", "ERROR")

                    self.root.after(2000, progress_window.destroy)

                except Exception as e:
                    self.root.after(
                        0,
                        lambda: status_label.config(
                            text=f"❌ Error: {str(e)}", foreground="red"
                        ),
                    )
                    self.log(f"Error installing dependencies: {e}", "ERROR")
                    self.root.after(3000, progress_window.destroy)

            install_thread = threading.Thread(target=run_install, daemon=True)
            install_thread.start()

            return True

        except Exception as e:
            self.log(f"Error installing dependencies: {e}", "ERROR")
            messagebox.showerror("Error", f"Could not install dependencies:\n{e}")
            return False

    def update_from_git(self):
        if not self.is_git_repo():
            messagebox.showinfo(
                "Not a Git Installation",
                "This installation doesn't appear to be a git repository.\n"
                "Updates via git pull are only available for git installations.",
            )
            return

        if self.is_server_running:
            if not messagebox.askyesno(
                "Server Running",
                "The server must be stopped before updating.\n\n"
                "Stop server and continue with update?",
            ):
                return
            self.stop_server()
            time.sleep(2)

        try:
            self.log("Checking for updates...", "INFO")

            subprocess.run(
                ["git", "fetch"],
                cwd=str(self.installation_dir),
                check=True,
                capture_output=True,
            )

            result = subprocess.run(
                ["git", "rev-list", "HEAD...origin/main", "--count"],
                cwd=str(self.installation_dir),
                capture_output=True,
                text=True,
                check=True,
            )

            commits_behind = int(result.stdout.strip())

            if commits_behind == 0:
                messagebox.showinfo(
                    "Up to Date", "You're already running the latest version!"
                )
                return

            if not messagebox.askyesno(
                "Update Available",
                f"There are {commits_behind} new commit(s) available.\n\n"
                "Do you want to update now?",
            ):
                return

            self.log("Updating...", "INFO")

            result = subprocess.run(
                ["git", "pull"],
                cwd=str(self.installation_dir),
                capture_output=True,
                text=True,
                check=True,
            )

            self.log(result.stdout, "SUCCESS")

            if "requirements.txt" in result.stdout:
                if messagebox.askyesno(
                    "Dependencies Updated",
                    "requirements.txt was updated.\n\n"
                    "Install new dependencies now?\n"
                    "(Recommended)",
                ):
                    self.install_dependencies()

            messagebox.showinfo(
                "Update Complete",
                "✅ Update successful!\n\n" "You may need to restart the launcher.",
            )

            self.log("Update completed successfully", "SUCCESS")

        except subprocess.CalledProcessError as e:
            self.log(f"Git pull failed: {e.stderr}", "ERROR")
            messagebox.showerror("Update Failed", f"Could not update:\n\n{e.stderr}")

    def start_server(self):
        if self.is_server_running:
            self.log("Server is already running", "WARNING")
            return

        if not self.model_path.get() or not Path(self.model_path.get()).exists():
            messagebox.showerror("Error", "Please specify a valid model path")
            return

        try:
            install_dir = self.detect_installation_dir()
            main_py = install_dir / "main.py"

            env = os.environ.copy()
            env["PYTHONIOENCODING"] = "utf-8"

            if hasattr(self, "hf_token_var") and self.hf_token_var.get().strip():
                env["HF_TOKEN"] = self.hf_token_var.get().strip()
                self.log("Hugging Face token added to environment")
            elif hasattr(self, "secure_storage"):
                try:
                    conn = sqlite3.connect(self.db_path)
                    cursor = conn.cursor()
                    cursor.execute(
                        "SELECT value FROM config WHERE key = ? AND is_encrypted = 1",
                        ("hf_token",),
                    )
                    result = cursor.fetchone()
                    conn.close()

                    if result:
                        decrypted_token = self.secure_storage.decrypt(result[0])
                        if decrypted_token:
                            env["HF_TOKEN"] = decrypted_token
                            self.log("Hugging Face token loaded from database")
                except Exception as e:
                    self.log(f"Could not load HF token: {e}", "WARNING")

            if not main_py.exists():
                messagebox.showerror("Error", f"main.py not found at {install_dir}")
                return

            if platform.system() == "Windows":
                possible_python_paths = [
                    install_dir / "env" / "Scripts" / "python.exe",
                    install_dir / "venv" / "Scripts" / "python.exe",
                    Path(sys.executable),
                ]
            else:
                possible_python_paths = [
                    install_dir / "env" / "bin" / "python",
                    install_dir / "venv" / "bin" / "python",
                    install_dir / "env" / "bin" / "python3",
                    install_dir / "venv" / "bin" / "python3",
                    Path(sys.executable),
                ]

            python_exe = None
            for py_path in possible_python_paths:
                if py_path.exists():
                    python_exe = py_path
                    break

            if not python_exe:
                import shutil

                for py_name in (
                    ["python", "python3"]
                    if platform.system() != "Windows"
                    else ["python.exe", "python3.exe"]
                ):
                    py_path = shutil.which(py_name)
                    if py_path:
                        python_exe = Path(py_path)
                        break

            if not python_exe:
                messagebox.showerror("Error", "No Python executable found")
                return

            self.log(f"Using Python: {python_exe}")
            self.log(f"Starting server from: {install_dir}")

            cmd = [
                str(python_exe),
                str(main_py),
                "--host",
                self.host.get(),
                "--port",
                self.port.get(),
                "--model-path",
                self.model_path.get(),
                "--environment",
                self.environment.get(),
                "--audio-model",
                self.audio_model.get(),
            ]

            if self.bypass_llm.get():
                cmd.append("--bypass-llm")
                self.log("LLM bypass enabled - direct audio generation mode")

            environment = self.environment.get()
            has_api_keys = len(self.api_keys_list) > 0

            if environment == "prod":
                cmd.append("--use-stored-keys")
                self.log("Production mode: using stored API keys")
            elif environment == "dev" and has_api_keys:
                use_keys = messagebox.askyesno(
                    "Development Mode",
                    "You have API keys configured.\n\n"
                    "Use stored API keys for authentication?\n\n"
                    "• Yes: Use API authentication\n"
                    "• No: Development bypass (no auth)",
                )
                if use_keys:
                    cmd.append("--use-stored-keys")
                    self.log("Development mode: using stored API keys")
                else:
                    self.log("Development mode: authentication bypass enabled")
            else:
                self.log("Development mode: no API keys configured, bypass enabled")

            self.log(f"Command: {' '.join(cmd)}")
            popen_kwargs = {
                "stdout": subprocess.PIPE,
                "stderr": subprocess.STDOUT,
                "universal_newlines": True,
                "bufsize": 1,
                "cwd": str(install_dir),
                "env": env,
                "encoding": "utf-8",
                "errors": "replace",
            }

            if platform.system() == "Windows":
                popen_kwargs["creationflags"] = subprocess.CREATE_NO_WINDOW
            elif platform.system() == "Darwin":
                popen_kwargs["start_new_session"] = True

            self.server_process = subprocess.Popen(cmd, **popen_kwargs)
            self.is_server_running = True
            self.update_ui_state()

            threading.Thread(target=self.monitor_output, daemon=True).start()

            self.log("Server started successfully", "SUCCESS")

        except Exception as e:
            self.log(f"Failed to start server: {e}", "ERROR")
            messagebox.showerror("Error", f"Failed to start server:\n{e}")

    def stop_server(self):
        if not self.is_server_running or not self.server_process:
            self.log("No server process to stop", "WARNING")
            return

        try:
            self.log("Stopping server...")

            self.server_process.terminate()

            try:
                self.server_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.server_process.kill()
                self.log("Server force-killed", "WARNING")

            self.server_process = None
            self.is_server_running = False
            self.update_ui_state()

            self.log("Server stopped", "SUCCESS")

        except Exception as e:
            self.log(f"Error stopping server: {e}", "ERROR")

    def restart_server(self):
        self.log("Restarting server...")
        self.stop_server()
        time.sleep(1)
        self.start_server()

    def monitor_output(self):
        if not self.server_process:
            return

        try:
            for line in iter(self.server_process.stdout.readline, ""):
                if line:
                    line = line.rstrip()
                    if line:
                        if (
                            "ERROR" in line
                            or "Exception" in line
                            or "Traceback" in line
                        ):
                            self.log(f"[SERVER ERROR] {line}", "ERROR")
                        elif "WARNING" in line or "⚠️" in line:
                            self.log(f"[SERVER] {line}", "WARNING")
                        else:
                            self.log(f"[SERVER] {line}")

                if self.server_process.poll() is not None:
                    break

        except Exception as e:
            self.log(f"Error monitoring output: {e}", "ERROR")

        if self.server_process:
            return_code = self.server_process.poll()
            if return_code is not None:
                self.log(
                    f"Server exited with code: {return_code}",
                    "ERROR" if return_code != 0 else "INFO",
                )

            try:
                if self.server_process.stderr:
                    stderr_output = self.server_process.stderr.read()
                    if stderr_output:
                        self.log(f"[SERVER STDERR] {stderr_output}", "ERROR")
            except:
                pass

        if self.is_server_running:
            self.is_server_running = False
            self.server_process = None
            self.root.after(0, self.update_ui_state)
            self.log("Server process ended", "WARNING")

    def monitor_server(self):
        if self.is_server_running and self.server_process:
            if self.server_process.poll() is not None:
                self.is_server_running = False
                self.server_process = None
                self.update_ui_state()
                self.log("Server stopped unexpectedly", "ERROR")

        self.update_ui_state()

        self.root.after(2000, self.monitor_server)

    def update_ui_state(self):
        if self.is_server_running:
            self.server_status.set("🟢 Server Running")
            self.server_url.set(f"http://{self.host.get()}:{self.port.get()}")
            self.status_label.configure(style="Success.TLabel")

            self.start_button.configure(state="disabled")
            self.stop_button.configure(state="normal")
            self.restart_button.configure(state="normal")
        else:
            self.server_status.set("🔴 Server Stopped")
            self.server_url.set("")
            self.status_label.configure(style="Error.TLabel")

            self.start_button.configure(state="normal")
            self.stop_button.configure(state="disabled")
            self.restart_button.configure(state="disabled")
        self.update_tray_menu()

    def check_first_launch(self):
        try:
            conn = sqlite3.connect(self.db_path)
            cursor = conn.cursor()

            cursor.execute("SELECT COUNT(*) FROM config")
            config_count = cursor.fetchone()[0]

            cursor.execute("SELECT COUNT(*) FROM api_keys")
            api_count = cursor.fetchone()[0]

            conn.close()

            if config_count == 0 and api_count == 0:
                self.show_setup_wizard()
                return True

            return False

        except Exception as e:
            self.log(f"Error checking first launch: {e}", "WARNING")
            return False

    def show_setup_wizard(self):
        wizard = tk.Toplevel(self.root)
        wizard.title("OBSIDIAN-Neural - First Time Setup")
        wizard.geometry("650x900")
        wizard.transient(self.root)
        wizard.grab_set()
        wizard.resizable(True, True)

        wizard.update_idletasks()
        x = (wizard.winfo_screenwidth() // 2) - (650 // 2)
        y = (wizard.winfo_screenheight() // 2) - (900 // 2)
        wizard.geometry(f"650x900+{x}+{y}")

        wizard_model_path = tk.StringVar()
        wizard_api_key = tk.StringVar()
        wizard_environment = tk.StringVar(value="dev")
        wizard_host = tk.StringVar(value="127.0.0.1")
        wizard_port = tk.StringVar(value="8000")
        wizard_audio_model = tk.StringVar(value="stabilityai/stable-audio-open-1.0")

        main_frame = ttk.Frame(wizard, padding="30")
        main_frame.pack(fill="both", expand=True)

        header_label = ttk.Label(
            main_frame,
            text="🎵 Welcome to OBSIDIAN-Neural!",
            font=("Arial", 18, "bold"),
        )
        header_label.pack(pady=(0, 10))

        subtitle_label = ttk.Label(
            main_frame,
            text="Let's set up your AI music generation system",
            font=("Arial", 12),
        )
        subtitle_label.pack(pady=(0, 30))

        step1_frame = ttk.LabelFrame(
            main_frame, text="Step 1: Select Your AI Model", padding="15"
        )
        step1_frame.pack(fill="x", pady=(0, 15))

        ttk.Label(step1_frame, text="Choose your LLM model file (.gguf):").pack(
            anchor="w"
        )

        model_frame = ttk.Frame(step1_frame)
        model_frame.pack(fill="x", pady=(5, 0))

        model_entry = ttk.Entry(model_frame, textvariable=wizard_model_path, width=50)
        model_entry.pack(side="left", fill="x", expand=True)

        def browse_wizard_model():
            filename = filedialog.askopenfilename(
                title="Select LLM Model File",
                filetypes=[("GGUF files", "*.gguf"), ("All files", "*.*")],
                initialdir=".",
            )
            if filename:
                wizard_model_path.set(filename)

        ttk.Button(model_frame, text="Browse", command=browse_wizard_model).pack(
            side="right", padx=(5, 0)
        )

        hf_step_frame = ttk.LabelFrame(
            main_frame, text="Step 2: Hugging Face Access Token", padding="15"
        )
        hf_step_frame.pack(fill="x", pady=(0, 20))

        hf_info_frame = ttk.Frame(hf_step_frame)
        hf_info_frame.pack(fill="x", pady=(0, 10))

        ttk.Label(
            hf_info_frame,
            text="⚠️ Required for Stable Audio model download",
            foreground="red",
            font=("Arial", 10, "bold"),
        ).pack(anchor="w")

        instructions_text = """1. Go to https://huggingface.co/stabilityai/stable-audio-open-1.0
        2. Click "Request access to this model" 
        3. Wait for approval (usually instant)
        4. Generate a token at https://huggingface.co/settings/tokens"""

        instructions_label = ttk.Label(
            hf_step_frame, text=instructions_text, font=("Arial", 9)
        )
        instructions_label.pack(anchor="w", pady=(0, 10))

        wizard_hf_token = tk.StringVar()
        token_frame = ttk.Frame(hf_step_frame)
        token_frame.pack(fill="x", pady=(5, 10))

        show_token = tk.BooleanVar(value=False)

        def update_token_display():
            if show_token.get():
                token_entry.config(show="")
                token_show_button.config(text="🙈 Hide")
            else:
                token_entry.config(show="*")
                token_show_button.config(text="👁 Show")

        ttk.Label(token_frame, text="HF Token:").pack(side="left")
        token_entry = ttk.Entry(
            token_frame,
            textvariable=wizard_hf_token,
            width=35,
            font=("Consolas", 10),
            show="*",
        )
        token_entry.pack(side="left", padx=(10, 5), fill="x", expand=True)

        token_show_button = ttk.Button(
            token_frame,
            text="👁 Show",
            width=8,
            command=lambda: [
                show_token.set(not show_token.get()),
                update_token_display(),
            ],
        )
        token_show_button.pack(side="left", padx=(0, 5))

        def verify_hf_token():
            token = wizard_hf_token.get().strip()
            if not token:
                messagebox.showerror("Error", "Please enter your Hugging Face token")
                return

            if not token.startswith("hf_") or len(token) < 30:
                messagebox.showerror(
                    "Invalid Format",
                    "Hugging Face tokens should start with 'hf_' and be ~37 characters long.\n\n"
                    "Create a token at: https://huggingface.co/settings/tokens",
                )
                return

            try:
                import requests

                headers = {"Authorization": f"Bearer {token}"}

                response = requests.get(
                    "https://huggingface.co/api/whoami-v2", headers=headers, timeout=15
                )

                if response.status_code != 200:
                    if response.status_code == 401:
                        messagebox.showerror(
                            "Invalid Token",
                            "❌ Token is invalid or expired\n\n"
                            "Please check your token at:\n"
                            "https://huggingface.co/settings/tokens",
                        )
                    else:
                        messagebox.showerror(
                            "Verification Failed",
                            f"❌ Could not verify token\n"
                            f"HTTP {response.status_code}: {response.text[:200]}",
                        )
                    return False

                user_info = response.json()
                username = user_info.get("name", user_info.get("fullname", "Unknown"))

                model_tests = [
                    (
                        "HEAD request",
                        "HEAD",
                        "https://huggingface.co/stabilityai/stable-audio-open-1.0",
                    ),
                    (
                        "Model info API",
                        "GET",
                        "https://huggingface.co/api/models/stabilityai/stable-audio-open-1.0",
                    ),
                ]

                for test_name, method, url in model_tests:
                    try:
                        if method == "HEAD":
                            test_response = requests.head(
                                url, headers=headers, timeout=10
                            )
                        else:
                            test_response = requests.get(
                                url, headers=headers, timeout=10
                            )

                        if test_response.status_code == 200:
                            messagebox.showinfo(
                                "Token Valid",
                                f"✅ Token verified!\n"
                                f"User: {username}\n"
                                f"✅ Stable Audio access confirmed",
                            )
                            return True

                    except Exception as e:
                        continue

                messagebox.showwarning(
                    "Access Required",
                    f"✅ Token is valid for user: {username}\n"
                    f"❌ Access to Stable Audio model required\n\n"
                    f"Steps to fix:\n"
                    f"1. Go to: https://huggingface.co/stabilityai/stable-audio-open-1.0\n"
                    f"2. Click 'Request access'\n"
                    f"3. Wait for approval (usually instant)\n"
                    f"4. Create a NEW token with 'read' permissions",
                )
                return False

            except requests.exceptions.Timeout:
                messagebox.showerror(
                    "Timeout", "❌ Request timed out. Check your internet connection."
                )
                return False
            except requests.exceptions.ConnectionError:
                messagebox.showerror(
                    "Connection Error", "❌ Could not connect to Hugging Face."
                )
                return False
            except Exception as e:
                messagebox.showerror(
                    "Verification Error", f"Could not verify token:\n{e}"
                )
                return False

        ttk.Button(token_frame, text="🔍 Verify", command=verify_hf_token).pack(
            side="left"
        )

        step2_frame = ttk.LabelFrame(
            main_frame, text="Step 3: Create Your First API Key", padding="15"
        )
        step2_frame.pack(fill="x", pady=(0, 20))

        ttk.Label(
            step2_frame, text="Generate a secure API key for authentication:"
        ).pack(anchor="w")

        name_frame = ttk.Frame(step2_frame)
        name_frame.pack(fill="x", pady=(5, 10))

        ttk.Label(name_frame, text="Key Name:").pack(side="left")
        wizard_api_name = tk.StringVar(value="Main API Key")
        name_entry = ttk.Entry(name_frame, textvariable=wizard_api_name, width=25)
        name_entry.pack(side="left", padx=(10, 0))

        api_frame = ttk.Frame(step2_frame)
        api_frame.pack(fill="x", pady=(8, 0))

        show_key = tk.BooleanVar(value=False)

        def update_api_display():
            if show_key.get():
                api_entry.config(show="")
                show_button.config(text="🙈 Hide")
            else:
                api_entry.config(show="*")
                show_button.config(text="👁 Show")

        api_entry = ttk.Entry(
            api_frame,
            textvariable=wizard_api_key,
            width=35,
            font=("Consolas", 10),
            show="*",
        )
        api_entry.pack(side="left", fill="x", expand=True)

        show_button = ttk.Button(
            api_frame,
            text="👁 Show",
            width=8,
            command=lambda: [show_key.set(not show_key.get()), update_api_display()],
        )
        show_button.pack(side="left", padx=(5, 5))

        def generate_wizard_key():
            import secrets
            import string

            alphabet = string.ascii_letters + string.digits
            api_key = "".join(secrets.choice(alphabet) for _ in range(32))
            wizard_api_key.set(api_key)

        ttk.Button(api_frame, text="🔑 Generate", command=generate_wizard_key).pack(
            side="left"
        )

        step3_frame = ttk.LabelFrame(
            main_frame, text="Step 4: Server Configuration", padding="15"
        )
        step3_frame.pack(fill="x", pady=(0, 15))

        server_frame = ttk.Frame(step3_frame)
        server_frame.pack(fill="x", pady=(0, 10))

        ttk.Label(server_frame, text="Host:").pack(side="left")
        ttk.Entry(server_frame, textvariable=wizard_host, width=15).pack(
            side="left", padx=(5, 20)
        )

        ttk.Label(server_frame, text="Port:").pack(side="left")
        ttk.Entry(server_frame, textvariable=wizard_port, width=8).pack(
            side="left", padx=(5, 20)
        )

        ttk.Label(server_frame, text="Environment:").pack(side="left")
        env_combo = ttk.Combobox(
            server_frame,
            textvariable=wizard_environment,
            values=["dev", "prod"],
            width=8,
            state="readonly",
        )
        env_combo.pack(side="left", padx=(5, 0))

        audio_frame = ttk.Frame(step3_frame)
        audio_frame.pack(fill="x", pady=(10, 0))

        ttk.Label(audio_frame, text="Audio Model:").pack(anchor="w")
        audio_combo = ttk.Combobox(
            audio_frame,
            textvariable=wizard_audio_model,
            values=[
                "stabilityai/stable-audio-open-1.0",
                "stabilityai/stable-audio-open-small",
            ],
            state="readonly",
            width=40,
        )
        audio_combo.pack(anchor="w", pady=(5, 0))

        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill="x", pady=(20, 0))

        def validate_and_save():
            errors = []

            if not wizard_model_path.get():
                errors.append("Please select a model file")
            elif not Path(wizard_model_path.get()).exists():
                errors.append("Model file does not exist")

            if not wizard_hf_token.get().strip():
                errors.append("Please enter your Hugging Face token")
            elif len(wizard_hf_token.get().strip()) < 10:
                errors.append("Hugging Face token seems too short")

            if not wizard_api_name.get().strip():
                errors.append("Please enter a name for your API key")

            if not wizard_api_key.get():
                errors.append("Please generate an API key")
            elif len(wizard_api_key.get()) < 16:
                errors.append("API key must be at least 16 characters")

            try:
                port = int(wizard_port.get())
                if port < 1 or port > 65535:
                    errors.append("Port must be between 1 and 65535")
            except ValueError:
                errors.append("Port must be a valid number")

            if errors:
                messagebox.showerror("Configuration Error", "\n".join(errors))
                return

            try:
                self.model_path.set(wizard_model_path.get())
                self.host.set(wizard_host.get())
                self.port.set(wizard_port.get())
                self.environment.set(wizard_environment.get())
                self.audio_model.set(wizard_audio_model.get())

                hf_token = wizard_hf_token.get().strip()
                if hf_token:
                    conn = sqlite3.connect(self.db_path)
                    cursor = conn.cursor()

                    encrypted_token = self.secure_storage.encrypt(hf_token)
                    cursor.execute(
                        "INSERT OR REPLACE INTO config (key, value, is_encrypted) VALUES (?, ?, ?)",
                        ("hf_token", encrypted_token, 1),
                    )

                    conn.commit()
                    conn.close()

                    self.hf_token_var.set(hf_token)

                if self.save_api_key_advanced(
                    wizard_api_key.get(),
                    wizard_api_name.get(),
                    is_limited=False,
                    total_credits=0,
                    expiration_date=None,
                ):
                    self.api_keys_list.append(wizard_api_key.get())
                    self.update_api_keys_listbox()

                if self.save_config():
                    self.log("Initial configuration completed successfully!", "SUCCESS")

                    messagebox.showinfo(
                        "Setup Complete",
                        "🎉 Configuration saved successfully!\n\n"
                        "✅ Model configured\n"
                        "✅ Hugging Face token saved\n"
                        "✅ API key generated and saved\n"
                        "✅ Server settings configured\n\n"
                        "You can now start the OBSIDIAN-Neural server!",
                    )

                    wizard.destroy()
                else:
                    messagebox.showerror("Error", "Failed to save configuration")

            except Exception as e:
                messagebox.showerror("Error", f"Failed to save configuration:\n{e}")

        def skip_setup():
            if messagebox.askyesno(
                "Skip Setup",
                "Are you sure you want to skip the setup?\n\n"
                "You'll need to configure everything manually.",
            ):
                wizard.destroy()

        ttk.Button(
            button_frame, text="💾 Save Configuration", command=validate_and_save
        ).pack(side="right")
        ttk.Button(button_frame, text="Skip", command=skip_setup).pack(
            side="right", padx=(0, 10)
        )

        generate_wizard_key()

        wizard.protocol("WM_DELETE_WINDOW", skip_setup)
        wizard.focus_set()

    def open_project_folder(self):
        try:
            project_path = Path.cwd()

            if platform.system() == "Windows":
                os.startfile(str(project_path))
            elif platform.system() == "Darwin":
                subprocess.run(["open", str(project_path)], check=True)
            else:
                file_managers = ["xdg-open", "nautilus", "dolphin", "thunar", "pcmanfm"]

                opened = False
                for fm in file_managers:
                    if shutil.which(fm):
                        proc = subprocess.run([fm, str(project_path)], check=False)
                        if proc.returncode == 0:
                            opened = True
                            break

                if not opened:
                    subprocess.run(["ls", "-la", str(project_path)], check=False)
                    messagebox.showinfo("Folder", f"Project folder: {project_path}")

            self.log(f"Project folder opened: {project_path}")

        except subprocess.CalledProcessError as e:
            self.log(f"Error opening folder (subprocess): {e}", "ERROR")
            messagebox.showerror("Error", f"Could not open project folder:\n{e}")
        except Exception as e:
            self.log(f"Error opening folder: {e}", "ERROR")
            messagebox.showerror("Error", f"Could not open project folder:\n{e}")

    def show_system_info(self):
        try:
            cpu_count = psutil.cpu_count(logical=False)
            cpu_threads = psutil.cpu_count(logical=True)
            memory = psutil.virtual_memory()
            disk = psutil.disk_usage(".")
            os_specific = ""

            if platform.system() == "Darwin":
                try:
                    mac_version = platform.mac_ver()[0]
                    architecture = platform.machine()
                    os_specific = (
                        f"macOS Version: {mac_version}\nArchitecture: {architecture}\n"
                    )

                    if architecture in ["arm64", "aarch64"]:
                        os_specific += "Apple Silicon: Yes (M1/M2/M3/M4)\n"
                    else:
                        os_specific += "Apple Silicon: No (Intel Mac)\n"

                except:
                    pass

            elif platform.system() == "Linux":
                try:
                    with open("/etc/os-release", "r") as f:
                        for line in f:
                            if line.startswith("PRETTY_NAME="):
                                distro = line.split("=")[1].strip().strip('"')
                                os_specific = f"Distribution: {distro}\n"
                                break
                except:
                    os_specific = f"Kernel: {platform.release()}\n"

            elif platform.system() == "Windows":
                try:
                    import winreg

                    with winreg.OpenKey(
                        winreg.HKEY_LOCAL_MACHINE,
                        r"SOFTWARE\Microsoft\Windows NT\CurrentVersion",
                    ) as key:
                        build = winreg.QueryValueEx(key, "CurrentBuild")[0]
                        release = winreg.QueryValueEx(key, "ReleaseId")[0]
                        os_specific = f"Windows Build: {build}\nRelease: {release}\n"
                except:
                    pass

            gpu_info = "None detected"
            try:
                import GPUtil

                gpus = GPUtil.getGPUs()
                if gpus:
                    gpu = gpus[0]
                    gpu_info = f"{gpu.name} ({gpu.memoryTotal}MB)"
            except:
                pass

            info = f"""System Information:

    Operating System:
    {platform.system()} {platform.release()}
    {os_specific}

    Hardware:
    CPU: {platform.processor()}
    Cores: {cpu_count} physical ({cpu_threads} logical)
    Memory: {memory.total // (1024**3)} GB total, {memory.available // (1024**3)} GB available
    GPU: {gpu_info}

    Storage:
    Free: {disk.free // (1024**3)} GB / {disk.total // (1024**3)} GB total
    Usage: {((disk.total - disk.free) / disk.total * 100):.1f}%

    Python Environment:
    Version: {platform.python_version()}
    Executable: {sys.executable}
    Architecture: {platform.architecture()[0]}

    Project:
    Directory: {Path.cwd()}
    Database: {self.db_path if hasattr(self, 'db_path') else 'Not initialized'}
    """
            messagebox.showinfo("System Information", info)

        except Exception as e:
            messagebox.showerror("Error", f"Could not get system info: {e}")

    def clear_logs(self):
        self.log_text.delete(1.0, tk.END)
        self.log("Logs cleared")

    def save_logs(self):
        filename = filedialog.asksaveasfilename(
            defaultextension=".log",
            filetypes=[("Log files", "*.log"), ("Text files", "*.txt")],
            title="Save Logs",
        )

        if filename:
            try:
                with open(filename, "w", encoding="utf-8") as f:
                    f.write(self.log_text.get(1.0, tk.END))
                self.log(f"Logs saved to {filename}", "SUCCESS")
            except Exception as e:
                self.log(f"Error saving logs: {e}", "ERROR")

    def on_closing(self):
        if self.is_server_running:
            if messagebox.askokcancel("Quit", "Server is running. Stop it and quit?"):
                self.stop_server()
                time.sleep(1)
                self.root.quit()
        else:
            self.root.quit()

    def run(self):
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.log("OBSIDIAN-Neural Launcher started")
        self.log("Configure your settings and click 'Start Server' to begin")
        self.root.mainloop()


if __name__ == "__main__":
    app = ObsidianNeuralLauncher()
    app.run()
