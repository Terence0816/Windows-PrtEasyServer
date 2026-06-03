#!/usr/bin/env python3
"""
Unified Build Script for PrtEasyServer.

Supports:
- modern build: current Windows releases
- win7 build: Windows 7 compatible packaging, which must be built with Python 3.8
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import textwrap
import time

APP_NAME = "PrtEasyServer"
APP_TITLE = "PrtEasyServer - Windows Network Print Server"
APP_REPO_NAME = "Windows-PrtEasyServer"
APP_GITHUB_URL = "https://github.com/Terence0816/Windows-PrtEasyServer"
APP_VERSION = "1.2.0.0"
APP_VERSION_TAG = f"v{APP_VERSION}"
APP_EXE = f"{APP_NAME}.exe"
WIN7_APP_EXE = f"{APP_NAME}_win7.exe"

COMPANY_NAME = "Terence0816"
FILE_DESCRIPTION = APP_TITLE
PRODUCT_NAME = APP_REPO_NAME
BUILD_COPYRIGHT = "Copyright (c) 2026 Terence0816"
WINDOWS_COPYRIGHT = APP_GITHUB_URL
COMMENTS = "Copyright (c) 2026 Terence0816. This project is based on PrinterOne by xtieume."
LEGACY_APP_NAME = "PrinterOne"
LEGACY_EXE = f"{LEGACY_APP_NAME}.exe"
WIN7_MAX_PYTHON = (3, 8)


def parse_args():
    """Parse build options."""
    parser = argparse.ArgumentParser(description="Build PrtEasyServer executables.")
    parser.add_argument(
        "--target",
        choices=["modern", "win7"],
        default="modern",
        help="Build target profile.",
    )
    return parser.parse_args()


def version_tuple(version_text):
    """Convert a dotted version string to a 4-int tuple."""
    parts = [int(part) for part in str(version_text).strip().split(".") if part != ""]
    while len(parts) < 4:
        parts.append(0)
    return tuple(parts[:4])


def get_output_exe_name(target):
    """Return the output executable name for the selected target."""
    return WIN7_APP_EXE if target == "win7" else APP_EXE


def get_requirements_file(target):
    """Return the requirements file for the selected target."""
    return "requirements-win7.txt" if target == "win7" else "requirements.txt"


def build_metadata(target):
    """Return the Windows version resource fields."""
    output_exe = get_output_exe_name(target)
    return {
        "CompanyName": COMPANY_NAME,
        "FileDescription": FILE_DESCRIPTION,
        "FileVersion": APP_VERSION,
        "InternalName": APP_NAME,
        "OriginalFilename": output_exe,
        "ProductName": PRODUCT_NAME,
        "ProductVersion": APP_VERSION,
        "LegalCopyright": WINDOWS_COPYRIGHT,
        "Comments": COMMENTS,
    }


def print_win7_build_notice():
    """Warn when the current Python cannot produce a Win7-compatible EXE."""
    current_version = sys.version_info[:2]
    if current_version > WIN7_MAX_PYTHON:
        print(
            "[WARN] This build is running on Python "
            f"{current_version[0]}.{current_version[1]}, so the packaged EXE will not run on Windows 7."
        )
        print(
            "[WARN] For a Windows 7 compatible build, use Python 3.8 on a Windows 7 SP1 "
            "or Windows Server 2008 R2 SP1 build machine."
        )
        print()


def validate_target_environment(target):
    """Validate the active build environment."""
    current_version = sys.version_info[:2]
    if target == "win7" and current_version > WIN7_MAX_PYTHON:
        print(
            "[ERROR] Win7 builds must use Python 3.8.x. "
            f"Current interpreter: {current_version[0]}.{current_version[1]}"
        )
        print(
            "[ERROR] Rebuild on Windows 7 SP1 or Windows Server 2008 R2 SP1 "
            "with Python 3.8, then run build_exe_win7.bat."
        )
        return False
    return True


def create_version_file(target):
    """Create a temporary PyInstaller version file."""
    version_parts = version_tuple(APP_VERSION)
    metadata = build_metadata(target)
    template = textwrap.dedent(
        f"""
        VSVersionInfo(
          ffi=FixedFileInfo(
            filevers={version_parts},
            prodvers={version_parts},
            mask=0x3F,
            flags=0x0,
            OS=0x40004,
            fileType=0x1,
            subtype=0x0,
            date=(0, 0)
          ),
          kids=[
            StringFileInfo([
              StringTable(
                '040904B0',
                [
                  StringStruct('CompanyName', {metadata["CompanyName"]!r}),
                  StringStruct('FileDescription', {metadata["FileDescription"]!r}),
                  StringStruct('FileVersion', {metadata["FileVersion"]!r}),
                  StringStruct('InternalName', {metadata["InternalName"]!r}),
                  StringStruct('OriginalFilename', {metadata["OriginalFilename"]!r}),
                  StringStruct('ProductName', {metadata["ProductName"]!r}),
                  StringStruct('ProductVersion', {metadata["ProductVersion"]!r}),
                  StringStruct('LegalCopyright', {metadata["LegalCopyright"]!r}),
                  StringStruct('Comments', {metadata["Comments"]!r})
                ]
              )
            ]),
            VarFileInfo([VarStruct('Translation', [1033, 1200])])
          ]
        )
        """
    ).strip()

    handle = tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        suffix="_version_info.txt",
        delete=False,
    )
    with handle:
        handle.write(template)
        handle.write("\n")
    return handle.name


def install_requirements(target):
    """Install required packages."""
    requirements_file = get_requirements_file(target)
    print("Installing requirements...")
    try:
        subprocess.run(
            [sys.executable, "-m", "pip", "install", "-r", requirements_file],
            check=True,
        )
        print(f"[OK] Requirements installed successfully from {requirements_file}")
    except subprocess.CalledProcessError as error:
        print(f"[ERROR] Error installing requirements: {error}")
        return False
    return True


def force_remove_file(filepath):
    """Force remove a file, trying multiple methods."""
    if not os.path.exists(filepath):
        return True

    try:
        os.remove(filepath)
        return True
    except PermissionError:
        print(f"[WARNING] Permission denied for {filepath} - file may be in use")
        return False
    except Exception as error:
        print(f"[ERROR] Error removing {filepath}: {error}")
        return False


def kill_running_processes():
    """Kill any running PrtEasyServer or legacy PrinterOne processes."""
    print(f"Checking for running {APP_NAME} processes...")
    killed_count = 0

    try:
        try:
            import psutil
        except ImportError:
            print("[WARNING] psutil is not installed yet, skipping process scan.")
            return False

        for proc in psutil.process_iter(["pid", "name", "exe"]):
            try:
                process_name = (proc.info["name"] or "").lower()

                if process_name in {APP_EXE.lower(), WIN7_APP_EXE.lower(), LEGACY_EXE.lower()}:
                    print(f"Killing process: {proc.info['name']} (PID: {proc.info['pid']})")
                    proc.terminate()
                    try:
                        proc.wait(timeout=5)
                    except psutil.TimeoutExpired:
                        proc.kill()
                        proc.wait(timeout=2)
                    killed_count += 1
                elif process_name == "python.exe" and proc.info["exe"]:
                    try:
                        cmdline = " ".join(proc.cmdline())
                        if "server.py" in cmdline:
                            print(f"Killing Python process running server.py: PID {proc.info['pid']}")
                            proc.terminate()
                            try:
                                proc.wait(timeout=5)
                            except psutil.TimeoutExpired:
                                proc.kill()
                                proc.wait(timeout=2)
                            killed_count += 1
                    except (psutil.AccessDenied, psutil.NoSuchProcess):
                        pass
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                pass
    except Exception as error:
        print(f"Error killing processes: {error}")

    if killed_count > 0:
        print(f"[OK] Killed {killed_count} process(es)")
        time.sleep(2)
    else:
        print("[OK] No running processes found")

    return killed_count > 0


def clean_build():
    """Clean previous build files."""
    print("Cleaning previous build...")
    try:
        kill_running_processes()

        if os.path.exists("build"):
            shutil.rmtree("build")
        if os.path.exists("dist"):
            try:
                shutil.rmtree("dist")
            except PermissionError:
                print("[WARNING] Permission denied removing dist folder, trying to force...")
                for root, dirs, files in os.walk("dist", topdown=False):
                    for filename in files:
                        force_remove_file(os.path.join(root, filename))
                    for dirname in dirs:
                        try:
                            os.rmdir(os.path.join(root, dirname))
                        except OSError:
                            pass
                try:
                    os.rmdir("dist")
                except OSError:
                    print("[WARNING] Could not fully clean dist folder, continuing...")

        for spec_file in [
            "PrtEasyServer.spec",
            "PrtEasyServer_win7.spec",
            "PrinterOne.spec",
            "PrinterOneManager.spec",
        ]:
            if os.path.exists(spec_file):
                force_remove_file(spec_file)

        print("[OK] Build cleaned successfully")
        return True
    except Exception as error:
        print(f"[ERROR] Error cleaning build: {error}")
        return False


def build_gui_exe(target):
    """Build the PrtEasyServer GUI executable with embedded version info."""
    output_exe = get_output_exe_name(target)
    output_name = os.path.splitext(output_exe)[0]
    print(f"Building {output_exe}...")
    kill_running_processes()

    gui_exe_path = os.path.join("dist", output_exe)
    if os.path.exists(gui_exe_path):
        print(f"Removing existing {gui_exe_path}...")
        force_remove_file(gui_exe_path)

    version_file = create_version_file(target)
    generated_spec_file = f"{output_name}.spec"
    try:
        cmd = [
            sys.executable,
            "-m",
            "PyInstaller",
            "--onefile",
            "--noconsole",
            f"--name={output_name}",
            "--icon=printer.ico",
            "--add-data=config.json;.",
            "--add-data=printer.png;.",
            f"--version-file={version_file}",
            "--hidden-import=pystray",
            "--hidden-import=PIL",
            "--hidden-import=PIL.Image",
            "--hidden-import=psutil",
            "--hidden-import=win32print",
            "--hidden-import=win32api",
            "--hidden-import=win32con",
            "--hidden-import=winreg",
            "server.py",
        ]

        subprocess.run(cmd, check=True)
        print(f"[OK] {output_exe} built successfully")
        return True
    except subprocess.CalledProcessError as error:
        print(f"[ERROR] Error building {output_exe}: {error}")
        return False
    finally:
        force_remove_file(version_file)
        force_remove_file(generated_spec_file)


def check_gui_executable(target):
    """Check if the executable was built successfully."""
    output_exe = get_output_exe_name(target)
    print(f"Checking {output_exe}...")
    try:
        exe_path = os.path.join("dist", output_exe)
        if os.path.exists(exe_path):
            file_size = os.path.getsize(exe_path)
            print(f"[OK] {output_exe} built successfully ({file_size:,} bytes)")
            return True
        print(f"[ERROR] {output_exe} not found in dist directory")
        return False
    except Exception as error:
        print(f"[ERROR] Error checking executable: {error}")
        return False


def print_build_summary(target):
    """Print a concise release summary after a successful build."""
    metadata = build_metadata(target)
    output_exe = get_output_exe_name(target)
    print()
    print("[SUCCESS] Build completed successfully!")
    print(f"[VERSION] Release version: {APP_VERSION_TAG}")
    print(f"[TARGET] {target}")
    print(f"[OUTPUT] dist\\{output_exe}")
    print("[EXE INFO] Embedded Windows file details:")
    print(f"  - ProductName: {metadata['ProductName']}")
    print(f"  - FileDescription: {metadata['FileDescription']}")
    print(f"  - CompanyName: {metadata['CompanyName']}")
    print(f"  - FileVersion: {metadata['FileVersion']}")
    print(f"  - ProductVersion: {metadata['ProductVersion']}")
    print(f"  - OriginalFilename: {metadata['OriginalFilename']}")
    print(f"  - Copyright: {metadata['LegalCopyright']}")
    print()
    print("Usage:")
    print(f"  - Double-click {output_exe} to launch the GUI")
    print("  - Sign the EXE after build if you are publishing a release")
    print(f"  - Create the GitHub release tag as {APP_VERSION_TAG} when ready")
    print()


def main():
    """Main build function."""
    args = parse_args()

    print(f"{APP_NAME} - Unified Build Script")
    print("=================================")
    print()
    print(f"Version: {APP_VERSION_TAG}")
    print(f"Target: {args.target}")
    print(BUILD_COPYRIGHT)
    print(f"GitHub: {APP_GITHUB_URL}")
    print()
    print(COMMENTS)
    print("Original project: https://github.com/xtieume/PrinterOne")
    print()

    if args.target != "win7":
        print_win7_build_notice()

    if not validate_target_environment(args.target):
        return 1

    if not install_requirements(args.target):
        return 1

    try:
        clean_build()
    except Exception as error:
        print(f"[WARNING] Some files could not be cleaned: {error}")
        print("Trying to kill processes and continue...")
        kill_running_processes()
        time.sleep(1)

    print(f"\n[BUILD] Building {get_output_exe_name(args.target)}...")
    if not build_gui_exe(args.target):
        print("[ERROR] Failed to build GUI executable. Stopping build process.")
        return 1

    print("\n[VERIFY] Verifying build results...")
    if not check_gui_executable(args.target):
        print("[ERROR] Build verification failed. Executable is missing.")
        return 1

    print_build_summary(args.target)
    return 0


if __name__ == "__main__":
    sys.exit(main() or 0)
