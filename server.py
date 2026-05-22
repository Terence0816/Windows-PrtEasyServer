#!/usr/bin/env python3
"""
PrinterOne - Unified Server and GUI Application
A comprehensive TCP print server with integrated GUI management and test client functionality
"""

# Critical startup logging - Log everything from the very beginning
import os
import sys
import time
import json
import socket
import threading
import subprocess
import signal
import tempfile
import logging
import glob
import html
import base64
import ctypes
import locale
import shutil
import psutil
import winreg
import traceback
import re
import zipfile
import webbrowser
from datetime import datetime, timedelta
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, quote

def get_hidden_subprocess_kwargs():
    """Return Windows subprocess flags that suppress console windows."""
    if os.name != "nt":
        return {}

    startupinfo = subprocess.STARTUPINFO()
    startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startupinfo.wShowWindow = 0
    return {
        "startupinfo": startupinfo,
        "creationflags": getattr(subprocess, "CREATE_NO_WINDOW", 0),
    }

def get_app_directory():
    """Get the directory of the running script or built executable."""
    base_path = sys.executable if getattr(sys, "frozen", False) else __file__
    return os.path.dirname(os.path.abspath(base_path))

def get_config_file_path():
    """Always keep config.json next to the script or executable."""
    return os.path.join(get_app_directory(), "config.json")

# Setup early logging to capture startup issues
def setup_early_logging():
    """Disable file logging; the UI already shows runtime status."""
    return None

# Initialize early logging
startup_logger = setup_early_logging()

try:
    if startup_logger:
        startup_logger.info("Starting import phase...")
    
    # Import GUI modules
    try:
        if startup_logger:
            startup_logger.info("Importing GUI modules...")
        import tkinter as tk
        from tkinter import ttk
        if startup_logger:
            startup_logger.info("GUI modules imported successfully")
    except ImportError as e:
        if startup_logger:
            startup_logger.error(f"Failed to import GUI modules: {e}")
        raise
    
    # Import Windows-specific modules
    try:
        if startup_logger:
            startup_logger.info("Importing Windows print modules...")
        import win32print
        if startup_logger:
            startup_logger.info("Windows print modules imported successfully")
    except ImportError as e:
        if startup_logger:
            startup_logger.error(f"Failed to import Windows modules: {e}")
        raise
    
    # Import PDF generation
    try:
        if startup_logger:
            startup_logger.info("Importing PDF modules...")
        from reportlab.pdfgen import canvas
        from reportlab.lib.pagesizes import letter
        if startup_logger:
            startup_logger.info("PDF modules imported successfully")
    except ImportError as e:
        if startup_logger:
            startup_logger.error(f"Failed to import PDF modules: {e}")
        raise

except Exception as e:
    error_msg = f"CRITICAL: Import phase failed: {e}"
    if startup_logger:
        startup_logger.critical(error_msg)
        startup_logger.critical(f"Exception type: {type(e).__name__}")
        startup_logger.critical(f"Traceback: {traceback.format_exc()}")
    else:
        print(error_msg)
        print(f"Exception type: {type(e).__name__}")
        print(f"Traceback: {traceback.format_exc()}")
    sys.exit(1)

# System tray imports (optional)
try:
    import pystray
    from PIL import Image, ImageTk
    TRAY_AVAILABLE = True
except ImportError:
    TRAY_AVAILABLE = False
    print("pystray not available, system tray disabled")

# Global variables
SERVER_RUNNING = True
AUTO_START_MODE = False
APP_NAME = "PrtEasyServer"
APP_VERSION = "1.1.0.0"
APP_TITLE = "PrtEasyServer - Windows 網路印表機伺服器"
APP_REPO_NAME = "Windows-PrtEasyServer"
APP_GITHUB_URL = "https://github.com/Terence0816/Windows-PrtEasyServer"
APP_COPYRIGHT = "Copyright (c) 2026 Terence0816"
LEGACY_APP_NAME = "PrinterOne"
LEGACY_GITHUB_URL = "https://github.com/xtieume/PrinterOne"
LEGACY_COPYRIGHT = "Original Copyright (c) 2025 xtieume@gmail.com"
STARTUP_VALUE_NAME = "PrtEasyServer"
LEGACY_STARTUP_VALUE_NAME = "PrinterOneManager"
LANGUAGE_NAMES = {
    "zh-TW": {
        "native": "繁體中文",
        "english": "Traditional Chinese",
    },
    "en": {
        "native": "English",
        "english": "English",
    },
}
TRANSLATIONS = {
    "zh-TW": {
        "app_title": "PrtEasyServer - Windows 網路印表機伺服器",
        "tab_server": "伺服器管理",
        "tab_test": "測試工具",
        "tab_settings": "設定",
        "frame_config": "設定",
        "config_hint": "可新增多組印表機設定，啟動時會同時監聽所有已填入的連接埠。",
        "printer_label": "印表機{index}：",
        "port_label": "連接埠：",
        "web_port_label": "網頁連接埠：",
        "web_port_hint": "80 代表可直接在瀏覽器輸入伺服器 IP 開啟設定頁",
        "button_add": "新增一組",
        "button_remove": "刪除一組",
        "button_save": "儲存設定",
        "frame_server_control": "伺服器控制",
        "status_stopped": "[STOP] 伺服器已停止",
        "status_running": "[OK] 伺服器執行中",
        "button_start": "啟動伺服器",
        "button_stop": "停止伺服器",
        "frame_autostart": "開機自動啟動",
        "status_checking": "檢查中...",
        "button_add_startup": "加入",
        "button_remove_startup": "移除",
        "frame_server_logs": "伺服器紀錄",
        "frame_test_config": "測試設定",
        "label_host": "主機：",
        "button_test_connection": "測試連線",
        "frame_test_data": "測試資料",
        "button_send_test_data": "送出測試資料",
        "frame_test_logs": "測試紀錄",
        "test_log_intro": "測試紀錄區\n============\n\n按一下「測試連線」可檢查是否能連上伺服器。\n按一下「送出測試資料」可送出一筆測試列印工作。\n\n測試結果會顯示在這裡...\n",
        "frame_app_settings": "應用程式設定",
        "setting_minimize_to_tray": "關閉視窗時縮小到系統匣",
        "tray_unavailable": "（系統匣功能不可用，尚未安裝 pystray）",
        "frame_language": "語系",
        "language_label": "介面語系：",
        "button_apply_language": "套用語系",
        "frame_about": "關於",
        "about_text": "PrtEasyServer - Windows 網路印表機伺服器\n版本 1.1.0.0\nCopyright (c) 2026 Terence0816\nGitHub: https://github.com/Terence0816/Windows-PrtEasyServer\n\n基於 PrinterOne 修改：\nhttps://github.com/xtieume/PrinterOne\nOriginal Copyright (c) 2025 xtieume@gmail.com\n\n這是一個簡易的 TCP/IP 列印伺服器，可將本機印表機轉成網路 IP 印表機。\n支援 RAW 9100 列印，不需 Windows 網芳、SMB 分享或帳號密碼。\n\nThis project is based on PrinterOne by xtieume.\nOriginal project: https://github.com/xtieume/PrinterOne",
        "startup_enabled": "[OK] 已啟用開機自動啟動",
        "startup_disabled": "[STOP] 未啟用開機自動啟動",
        "server_info_started": "已啟動 {count} 組",
        "server_info_ip": "IP：{value}",
        "server_info_print_ports": "列印連接埠：{value}",
        "server_info_web": "設定網頁：{value}",
        "server_info_web_failed": "設定網頁：啟動失敗（{value}）",
        "error_web_port_number": "[ERROR] 網頁連接埠必須是有效的數字。",
        "error_web_port_range": "[ERROR] 網頁連接埠必須介於 1 到 65535。",
        "error_web_port_duplicate": "[ERROR] 網頁連接埠不可與任何印表機連接埠重複。",
        "info_firewall_check_failed": "[WARN] 啟動時檢查防火牆失敗：{error}",
        "info_settings_saved": "[OK] 設定已成功儲存！",
        "error_settings_save_failed": "[ERROR] 設定儲存失敗！",
        "warn_server_running": "[WARN] 伺服器已經在執行中！",
        "warn_no_printer_before_start": "[WARN] 請至少設定一組印表機後再啟動。",
        "info_settings_saved_before_start": "[OK] 設定已儲存",
        "error_settings_save_before_start_failed": "[ERROR] 啟動前儲存設定失敗！",
        "info_starting_servers": "[START] 正在啟動所有已設定的伺服器...",
        "error_server_start_failed": "[ERROR] 伺服器啟動失敗！",
        "warn_auto_start_no_printer": "[WARN] 自動啟動模式：尚未設定印表機，程式將在系統匣待命",
        "info_no_printer_no_autostart": "[INFO] 尚未設定印表機，未啟動伺服器",
        "info_auto_start_background": "[AUTO] 自動啟動模式：正在背景啟動伺服器，請查看系統匣...",
        "info_auto_start_configured": "[AUTO] 正在使用已設定的印表機自動啟動伺服器...",
        "error_auto_start": "[ERROR] 自動啟動時發生錯誤：{error}",
        "test_connecting": "[CONNECT] 正在測試連線到 {host}:{port}...",
        "test_connection_done": "[OK] 連線測試完成！",
        "test_connection_failed": "[ERROR] 連線測試失敗！",
        "test_pdf_convert": "[PDF] 正在將測試資料轉成 PDF，以供 PDF 印表機使用...",
        "test_pdf_convert_ok": "[OK] 測試資料已轉成 PDF（{size} 位元組）",
        "test_pdf_convert_failed": "[WARN] PDF 轉換失敗，改用原始資料",
        "test_pdf_convert_error": "[WARN] PDF 轉換發生錯誤：{error}",
        "test_sending_data": "[SEND] 正在送出測試資料到 {host}:{port}（{size} 位元組）",
        "test_send_ok": "[OK] 測試資料已成功送出！",
        "test_send_failed": "[ERROR] 測試資料送出失敗！",
        "language_switch_saving_failed": "[ERROR] 切換語系時儲存設定失敗！",
        "language_switch_restarting": "[INFO] 語系已切換，正在重新啟動伺服器...",
        "language_switch_restart_failed": "[ERROR] 語系切換後伺服器重新啟動失敗！",
        "language_switch_applied": "[OK] 語系已套用。",
        "language_option_zh-TW": "繁體中文",
        "language_option_en": "英文",
        "web_title": "PrtEasyServer 印表機服務",
        "web_eyebrow": "PRTEASYSERVER SERVICE",
        "web_heading": "網路印表機設定中心",
        "web_intro_1": "這台伺服器目前由 <strong>{host}</strong> 提供印表機分享服務。",
        "web_intro_2": "在其他電腦下載對應的設定檔後直接執行，即可快速建立印表機連接埠與印表機項目。",
        "web_meta_ip": "伺服器 IP：{value}",
        "web_meta_entry": "網頁入口：{value}",
        "web_meta_count": "共用印表機數量：{count}",
        "web_printer_badge": "印表機{index}",
        "web_driver": "驅動：",
        "web_raw_port": "Raw 連接埠：",
        "web_host": "安裝主機：",
        "web_target": "建議 TCP/IP 位址：",
        "web_download": "下載設定檔",
        "web_empty_title": "目前沒有可用的印表機設定",
        "web_empty_body": "請先回到 {app_name} 管理介面設定印表機，並啟動伺服器。",
        "web_footer": "提醒：下載的 BAT 會優先使用主機名稱連線；若名稱無法解析，會自動改用目前這台伺服器的 IP 位址 {ip}。",
        "bat_success_title": "安裝完成",
        "bat_success_message": "{printer} 印表機安裝完成",
        "bat_missing_title": "找不到印表機驅動",
        "bat_missing_message": "找不到驅動：{driver}`r`n`r`n已建立連接埠：{port}`r`n請手動新增印表機並選擇這個連接埠。",
    },
    "en": {
        "app_title": "PrtEasyServer - Windows Network Print Server",
        "tab_server": "Server",
        "tab_test": "Test Tools",
        "tab_settings": "Settings",
        "frame_config": "Configuration",
        "config_hint": "You can add multiple printer entries. The server will listen on every filled port at the same time.",
        "printer_label": "Printer {index}:",
        "port_label": "Port:",
        "web_port_label": "Web Port:",
        "web_port_hint": "Port 80 lets clients open the setup page by typing only the server IP",
        "button_add": "Add Row",
        "button_remove": "Remove Row",
        "button_save": "Save Settings",
        "frame_server_control": "Server Control",
        "status_stopped": "[STOP] Server stopped",
        "status_running": "[OK] Server running",
        "button_start": "Start Server",
        "button_stop": "Stop Server",
        "frame_autostart": "Windows Startup",
        "status_checking": "Checking...",
        "button_add_startup": "Add",
        "button_remove_startup": "Remove",
        "frame_server_logs": "Server Log",
        "frame_test_config": "Test Settings",
        "label_host": "Host:",
        "button_test_connection": "Test Connection",
        "frame_test_data": "Test Data",
        "button_send_test_data": "Send Test Data",
        "frame_test_logs": "Test Log",
        "test_log_intro": "Test Log\n========\n\nClick \"Test Connection\" to check whether the server can be reached.\nClick \"Send Test Data\" to send a sample print job.\n\nTest results will appear here...\n",
        "frame_app_settings": "Application Settings",
        "setting_minimize_to_tray": "Minimize to system tray when closing the window",
        "tray_unavailable": "(System tray support is unavailable because pystray is not installed)",
        "frame_language": "Language",
        "language_label": "Interface Language:",
        "button_apply_language": "Apply Language",
        "frame_about": "About",
        "about_text": "PrtEasyServer - Windows Network Print Server\nVersion 1.1.0.0\nCopyright (c) 2026 Terence0816\nGitHub: https://github.com/Terence0816/Windows-PrtEasyServer\n\nBased on PrinterOne:\nhttps://github.com/xtieume/PrinterOne\nOriginal Copyright (c) 2025 xtieume@gmail.com\n\nThis is a lightweight TCP/IP print server that turns local Windows printers into network IP printers.\nIt supports RAW 9100 printing without Windows network sharing, SMB, or account/password prompts.\n\nThis project is based on PrinterOne by xtieume.\nOriginal project: https://github.com/xtieume/PrinterOne",
        "startup_enabled": "[OK] Startup is enabled",
        "startup_disabled": "[STOP] Startup is disabled",
        "server_info_started": "{count} printer servers active",
        "server_info_ip": "IP: {value}",
        "server_info_print_ports": "Print Ports: {value}",
        "server_info_web": "Setup Page: {value}",
        "server_info_web_failed": "Setup Page: failed to start ({value})",
        "error_web_port_number": "[ERROR] Web port must be a valid number.",
        "error_web_port_range": "[ERROR] Web port must be between 1 and 65535.",
        "error_web_port_duplicate": "[ERROR] The web port cannot match any configured printer port.",
        "info_firewall_check_failed": "[WARN] Firewall check failed during startup: {error}",
        "info_settings_saved": "[OK] Settings saved successfully.",
        "error_settings_save_failed": "[ERROR] Failed to save settings.",
        "warn_server_running": "[WARN] The server is already running.",
        "warn_no_printer_before_start": "[WARN] Please configure at least one printer before starting the server.",
        "info_settings_saved_before_start": "[OK] Settings saved.",
        "error_settings_save_before_start_failed": "[ERROR] Failed to save settings before starting the server.",
        "info_starting_servers": "[START] Starting all configured printer servers...",
        "error_server_start_failed": "[ERROR] Failed to start the server.",
        "warn_auto_start_no_printer": "[WARN] Auto-start mode: no printer is configured, the app will stay idle in the tray.",
        "info_no_printer_no_autostart": "[INFO] No printer is configured, so the server was not started.",
        "info_auto_start_background": "[AUTO] Auto-start mode: starting the server in the background. Please check the tray icon.",
        "info_auto_start_configured": "[AUTO] Starting the server with the configured printer settings...",
        "error_auto_start": "[ERROR] Auto-start failed: {error}",
        "test_connecting": "[CONNECT] Testing connection to {host}:{port}...",
        "test_connection_done": "[OK] Connection test completed.",
        "test_connection_failed": "[ERROR] Connection test failed.",
        "test_pdf_convert": "[PDF] Converting test data to PDF for the PDF printer...",
        "test_pdf_convert_ok": "[OK] Test data converted to PDF ({size} bytes)",
        "test_pdf_convert_failed": "[WARN] PDF conversion failed. Falling back to raw data.",
        "test_pdf_convert_error": "[WARN] PDF conversion error: {error}",
        "test_sending_data": "[SEND] Sending test data to {host}:{port} ({size} bytes)",
        "test_send_ok": "[OK] Test data sent successfully.",
        "test_send_failed": "[ERROR] Failed to send test data.",
        "language_switch_saving_failed": "[ERROR] Failed to save settings while switching language.",
        "language_switch_restarting": "[INFO] Language applied. Restarting the server...",
        "language_switch_restart_failed": "[ERROR] Failed to restart the server after switching language.",
        "language_switch_applied": "[OK] Language applied.",
        "language_option_zh-TW": "Traditional Chinese",
        "language_option_en": "English",
        "web_title": "PrtEasyServer Printer Service",
        "web_eyebrow": "PRTEASYSERVER SERVICE",
        "web_heading": "Network Printer Setup Center",
        "web_intro_1": "This server is currently sharing printers through <strong>{host}</strong>.",
        "web_intro_2": "Download the matching setup file on another computer and run it to create the printer port and printer entry quickly.",
        "web_meta_ip": "Server IP: {value}",
        "web_meta_entry": "Web Entry: {value}",
        "web_meta_count": "Shared Printers: {count}",
        "web_printer_badge": "Printer {index}",
        "web_driver": "Driver:",
        "web_raw_port": "RAW Port:",
        "web_host": "Install Host:",
        "web_target": "Suggested TCP/IP Target:",
        "web_download": "Download Setup",
        "web_empty_title": "No printer configuration is available right now",
        "web_empty_body": "Please return to the {app_name} management UI, configure a printer, and start the server first.",
        "web_footer": "Note: the downloaded BAT file uses the hostname first. If the hostname cannot be resolved, it falls back to the current server IP address {ip}.",
        "bat_success_title": "Install Complete",
        "bat_success_message": "{printer} installation completed.",
        "bat_missing_title": "Printer Driver Not Found",
        "bat_missing_message": "Driver not found: {driver}`r`n`r`nCreated port: {port}`r`nPlease add the printer manually and select this port.",
    },
}

# Language overrides and helpers
APP_TITLE = f"{APP_NAME} - Windows Network Print Server"
SUPPORTED_LANGUAGES = ("zh-TW", "en")
LANGUAGE_NAMES = {
    "zh-TW": {
        "native": "繁體中文",
        "english": "Traditional Chinese",
    },
    "en": {
        "native": "English",
        "english": "English",
    },
}
TRANSLATIONS = {
    "zh-TW": {
        "app_title": "PrtEasyServer - Windows 網路印表機伺服器",
        "tab_server": "伺服器管理",
        "tab_test": "測試工具",
        "tab_settings": "設定",
        "frame_config": "設定",
        "config_hint": "可同時新增多組印表機設定，啟動後會一起監聽所有已填寫的連接埠。",
        "printer_label": "印表機{index}:",
        "port_label": "連接埠:",
        "web_port_label": "網頁連接埠:",
        "web_port_hint": "設為 80 時，其他電腦可直接輸入伺服器 IP 開啟設定頁面",
        "button_add": "新增一組",
        "button_remove": "刪除一組",
        "button_save": "儲存設定",
        "frame_server_control": "伺服器控制",
        "status_stopped": "[STOP] 伺服器已停止",
        "status_running": "[OK] 伺服器運行中",
        "button_start": "啟動伺服器",
        "button_stop": "停止伺服器",
        "frame_autostart": "開機自動啟動",
        "status_checking": "檢查中...",
        "button_add_startup": "加入",
        "button_remove_startup": "移除",
        "frame_server_logs": "伺服器記錄",
        "frame_test_config": "測試設定",
        "label_host": "主機:",
        "button_test_connection": "測試連線",
        "frame_test_data": "測試資料",
        "button_send_test_data": "傳送測試資料",
        "frame_test_logs": "測試記錄",
        "test_log_intro": "測試記錄\n============\n\n先輸入主機與連接埠來測試連線。\n也可以傳送測試資料，確認印表機與伺服器是否正常。\n\n等待測試操作...\n",
        "frame_app_settings": "應用程式設定",
        "setting_minimize_to_tray": "關閉視窗時縮小到系統匣",
        "tray_unavailable": "目前無法使用系統匣功能，因為找不到 pystray。",
        "frame_language": "語系",
        "language_label": "介面語言:",
        "button_apply_language": "套用語言",
        "frame_about": "關於",
        "about_text": "PrtEasyServer - Windows 網路印表機伺服器\n版本 1.1.0.0\nCopyright (c) 2026 Terence0816\nGitHub: https://github.com/Terence0816/Windows-PrtEasyServer\n\n基於 PrinterOne 修改：\nhttps://github.com/xtieume/PrinterOne\nOriginal Copyright (c) 2025 xtieume@gmail.com\n\n這是一個簡易的 TCP/IP 列印伺服器，可將本機印表機轉成網路 IP 印表機。\n支援 RAW 9100 列印，不需 Windows 網芳、SMB 分享或帳號密碼。\n\nThis project is based on PrinterOne by xtieume.\nOriginal project: https://github.com/xtieume/PrinterOne",
        "startup_enabled": "[OK] 已加入開機自動啟動",
        "startup_disabled": "[STOP] 未啟用開機自動啟動",
        "server_info_started": "已啟動 {count} 組",
        "server_info_ip": "IP: {value}",
        "server_info_print_ports": "列印連接埠: {value}",
        "server_info_web": "網頁位址: {value}",
        "server_info_web_failed": "網頁服務: 啟動失敗 ({value})",
        "error_port_range": "印表機{index} 的連接埠必須介於 1 到 65535。",
        "error_duplicate_ports": "[ERROR] 已設定的印表機不可使用重複連接埠。",
        "error_web_port_number": "[ERROR] 網頁連接埠必須是數字。",
        "error_web_port_range": "[ERROR] 網頁連接埠必須介於 1 到 65535。",
        "error_web_port_duplicate": "[ERROR] 網頁連接埠不可與印表機連接埠重複。",
        "info_printer_row_added": "[INFO] 已新增第 {count} 組印表機設定。",
        "info_printer_row_removed": "[INFO] 已刪除第 {count} 組印表機設定。",
        "info_printer_rows_reset": "[INFO] 已重設為單一印表機設定。",
        "info_firewall_check_failed": "[WARN] 啟動時檢查防火牆失敗: {error}",
        "info_settings_saved": "[OK] 設定已儲存。",
        "error_settings_save_failed": "[ERROR] 設定儲存失敗。",
        "warn_server_running": "[WARN] 伺服器已在運行中。",
        "warn_no_printer_before_start": "[WARN] 請先至少設定一台印表機再啟動伺服器。",
        "info_settings_saved_before_start": "[OK] 啟動前已先儲存設定。",
        "error_settings_save_before_start_failed": "[ERROR] 啟動前儲存設定失敗。",
        "info_starting_servers": "[START] 正在啟動所有已設定的伺服器...",
        "error_server_start_failed": "[ERROR] 伺服器啟動失敗。",
        "warn_auto_start_no_printer": "[WARN] 已啟用自動啟動，但目前沒有可用的印表機設定。",
        "info_no_printer_no_autostart": "[INFO] 目前沒有印表機設定，略過自動啟動。",
        "info_auto_start_background": "[AUTO] 開機自動啟動模式，正在背景啟動伺服器...",
        "info_auto_start_configured": "[AUTO] 偵測到已設定印表機，正在自動啟動伺服器...",
        "error_auto_start": "[ERROR] 自動啟動失敗: {error}",
        "test_connecting": "[CONNECT] 正在測試連線到 {host}:{port}...",
        "test_connection_done": "[OK] 連線測試成功。",
        "test_connection_failed": "[ERROR] 連線測試失敗。",
        "test_pdf_convert": "[PDF] 目標是 PDF 印表機，正在把測試資料轉成 PDF...",
        "test_pdf_convert_ok": "[OK] 已轉成 PDF，大小 {size} 位元組。",
        "test_pdf_convert_failed": "[WARN] PDF 轉換失敗，改送出原始資料。",
        "test_pdf_convert_error": "[WARN] PDF 轉換發生錯誤: {error}",
        "test_sending_data": "[SEND] 正在傳送測試資料到 {host}:{port}，大小 {size} 位元組。",
        "test_send_ok": "[OK] 測試資料已送出。",
        "test_send_failed": "[ERROR] 測試資料送出失敗。",
        "language_switch_saving_failed": "[ERROR] 儲存語系設定失敗。",
        "language_switch_restarting": "[INFO] 正在切換語言，將自動停止並重新啟動伺服器...",
        "language_switch_applied": "[OK] 語言已套用。",
        "language_option_zh-TW": "繁體中文",
        "language_option_en": "English",
        "web_title": "PrtEasyServer 印表機設定",
        "web_eyebrow": "PRTEASYSERVER SERVICE",
        "web_heading": "Windows 網路印表機伺服器",
        "web_intro_1": "這台伺服器目前使用 <strong>{host}</strong> 提供印表機安裝設定。",
        "web_intro_2": "請依照下方每台印表機的連接埠下載對應安裝檔，並在客戶端執行。",
        "web_meta_ip": "伺服器 IP: {value}",
        "web_meta_entry": "網頁入口: {value}",
        "web_meta_count": "可安裝印表機數量: {count}",
        "web_printer_badge": "印表機{index}",
        "web_driver": "驅動:",
        "web_raw_port": "Raw 連接埠:",
        "web_host": "主機名稱:",
        "web_target": "TCP/IP 目標:",
        "web_download": "下載設定檔",
        "web_empty_title": "目前尚未設定任何印表機",
        "web_empty_body": "請先回到 {app_name} 介面設定印表機後，再重新整理此頁面。",
        "web_footer": "注意：下載的 BAT 會使用主機名稱建立 TCP/IP 連接埠。如需連線此設定頁，可使用 {ip}。",
        "bat_success_title": "安裝完成",
        "bat_success_message": "{printer} 印表機安裝完成。",
        "bat_missing_title": "找不到印表機驅動",
        "bat_missing_message": "找不到驅動：{driver}`r`n`r`n已建立連接埠：{port}`r`n請手動新增印表機並選擇這個連接埠。",
        "client_socket_connected": "[OK] 連線已建立。",
        "client_sending_bytes": "[SEND] 正在傳送 {size} 位元組...",
        "client_data_sent": "[OK] 資料已送出。",
        "client_ping_only": "[OK] 已完成連線測試，未傳送資料。",
        "client_connection_refused": "[ERROR] 連線被拒絕，請確認 {host}:{port} 已啟動伺服器。",
        "client_timeout": "[ERROR] 連線逾時。",
        "client_unexpected_error": "[ERROR] 發生錯誤: {error}",
        "quit_app_exit": "[BYE] 正在關閉程式...",
        "quit_app_stopping_server": "[STOP] 正在停止伺服器...",
        "quit_app_tray_stopped": "[TRAY] 系統匣圖示已停止。",
        "quit_app_closed": "[OK] 程式已關閉。",
        "tray_show_window": "顯示視窗",
        "tray_hide_window": "隱藏視窗",
        "tray_quit": "結束程式",
        "firewall_trigger_launch": "程式啟動",
        "firewall_trigger_start": "啟動伺服器",
        "firewall_no_rules": "[FIREWALL] {trigger}: 沒有需要建立的防火牆規則。",
        "firewall_disabled": "[FIREWALL] Windows 防火牆目前關閉，略過規則建立。",
        "firewall_status_unknown": "[WARN] 無法確認 Windows 防火牆狀態: {details}",
        "firewall_admin_required": "[WARN] 目前不是系統管理員身分，無法自動建立防火牆規則。",
        "firewall_checking_enabled": "[FIREWALL] {trigger}: 正在檢查 Windows 防火牆規則 ({details})...",
        "firewall_creating_disabled": "[FIREWALL] {trigger}: Windows 防火牆未啟用，但仍嘗試建立 {app_name} 規則。",
        "firewall_rule_created": "[FIREWALL] 已建立規則: {name} (TCP {port})",
        "firewall_rule_updated": "[FIREWALL] 已更新規則: {name} (TCP {port})",
        "firewall_rule_exists": "[FIREWALL] 規則已存在: {name} (TCP {port})",
        "firewall_rule_failed": "[WARN] 防火牆規則失敗: {name} (TCP {port}) - {error}",
        "server_no_printer_config": "[!] 目前沒有任何已設定的印表機。",
        "server_duplicate_ports": "[!] 啟動失敗，印表機連接埠不可重複。",
        "server_web_port_conflict": "[!] 啟動失敗，網頁連接埠 {port} 不可與印表機連接埠重複。",
        "server_kill_port": "[KILL] 正在檢查連接埠 {port} 是否被占用...",
        "server_port_bound": "[OK] 連接埠 {port} 已開始監聽。",
        "server_printer_bound": "[PRINTER] 對應印表機: {printer}",
        "server_listen_address": "[CONNECT] 可由 {ip}:{port} 連線。",
        "server_multi_started": "[OK] 已啟動 {count} 組印表機伺服器。",
        "server_web_started": "[WEB] 網頁服務已啟動: {url}",
        "server_web_failed": "[WARN] 網頁服務啟動失敗: {error}",
        "server_start_exception": "[!] 伺服器啟動失敗: {error}",
        "server_stopped": "[DONE] 所有伺服器都已停止。",
        "server_accept_error": "[!] 監聽連線發生錯誤: {error}",
        "web_download_logged": "[WEB] 已提供印表機{index} 的 BAT 安裝檔。",
        "autostart_add_success": "已加入 Windows 開機自動啟動。",
        "autostart_add_failed": "加入開機自動啟動失敗: {error}",
        "autostart_remove_success": "已自 Windows 開機自動啟動移除。",
        "autostart_not_configured": "未設定開機自動啟動。",
        "autostart_remove_failed": "移除開機自動啟動失敗: {error}",
        "autostart_check_failed": "檢查開機自動啟動狀態時發生錯誤: {error}",
    },
    "en": {
        "app_title": "PrtEasyServer - Windows Network Print Server",
        "tab_server": "Server",
        "tab_test": "Test Tools",
        "tab_settings": "Settings",
        "frame_config": "Configuration",
        "config_hint": "You can add multiple printer entries, and the server will listen on every filled printer port at the same time.",
        "printer_label": "Printer {index}:",
        "port_label": "Port:",
        "web_port_label": "Web Port:",
        "web_port_hint": "Use port 80 if you want clients to open the setup page by typing only the server IP",
        "button_add": "Add Row",
        "button_remove": "Remove Row",
        "button_save": "Save Settings",
        "frame_server_control": "Server Control",
        "status_stopped": "[STOP] Server stopped",
        "status_running": "[OK] Server running",
        "button_start": "Start Server",
        "button_stop": "Stop Server",
        "frame_autostart": "Windows Startup",
        "status_checking": "Checking...",
        "button_add_startup": "Add",
        "button_remove_startup": "Remove",
        "frame_server_logs": "Server Log",
        "frame_test_config": "Test Settings",
        "label_host": "Host:",
        "button_test_connection": "Test Connection",
        "frame_test_data": "Test Data",
        "button_send_test_data": "Send Test Data",
        "frame_test_logs": "Test Log",
        "test_log_intro": "Test Log\n============\n\nEnter a host and port to test connectivity.\nYou can also send sample data to confirm that the printer and the server are working.\n\nWaiting for test actions...\n",
        "frame_app_settings": "Application Settings",
        "setting_minimize_to_tray": "Minimize to system tray when closing the window",
        "tray_unavailable": "System tray support is unavailable because pystray is missing.",
        "frame_language": "Language",
        "language_label": "Interface language:",
        "button_apply_language": "Apply Language",
        "frame_about": "About",
        "about_text": "PrtEasyServer - Windows Network Print Server\nVersion 1.1.0.0\nCopyright (c) 2026 Terence0816\nGitHub: https://github.com/Terence0816/Windows-PrtEasyServer\n\nBased on PrinterOne:\nhttps://github.com/xtieume/PrinterOne\nOriginal Copyright (c) 2025 xtieume@gmail.com\n\nThis is a lightweight TCP/IP print server that turns a local Windows printer into an IP printer.\nIt supports RAW 9100 printing without Windows file sharing, SMB, or network credentials.\n\nThis project is based on PrinterOne by xtieume.\nOriginal project: https://github.com/xtieume/PrinterOne",
        "startup_enabled": "[OK] Added to Windows startup",
        "startup_disabled": "[STOP] Windows startup disabled",
        "server_info_started": "Started {count} printer server(s)",
        "server_info_ip": "IP: {value}",
        "server_info_print_ports": "Print ports: {value}",
        "server_info_web": "Web page: {value}",
        "server_info_web_failed": "Web page: failed to start ({value})",
        "error_port_range": "Printer {index} port must be between 1 and 65535.",
        "error_duplicate_ports": "[ERROR] Configured printers cannot share the same port.",
        "error_web_port_number": "[ERROR] Web port must be a number.",
        "error_web_port_range": "[ERROR] Web port must be between 1 and 65535.",
        "error_web_port_duplicate": "[ERROR] The web port cannot reuse a printer port.",
        "info_printer_row_added": "[INFO] Added printer row {count}.",
        "info_printer_row_removed": "[INFO] Removed printer row {count}.",
        "info_printer_rows_reset": "[INFO] Reset back to a single printer row.",
        "info_firewall_check_failed": "[WARN] Startup firewall check failed: {error}",
        "info_settings_saved": "[OK] Settings saved.",
        "error_settings_save_failed": "[ERROR] Failed to save settings.",
        "warn_server_running": "[WARN] The server is already running.",
        "warn_no_printer_before_start": "[WARN] Configure at least one printer before starting the server.",
        "info_settings_saved_before_start": "[OK] Settings saved before start.",
        "error_settings_save_before_start_failed": "[ERROR] Failed to save settings before starting.",
        "info_starting_servers": "[START] Starting all configured servers...",
        "error_server_start_failed": "[ERROR] Failed to start the server.",
        "warn_auto_start_no_printer": "[WARN] Auto-start is enabled, but there are no usable printer settings yet.",
        "info_no_printer_no_autostart": "[INFO] No printer configuration was found, so auto-start was skipped.",
        "info_auto_start_background": "[AUTO] Startup mode detected. Starting the server in the background...",
        "info_auto_start_configured": "[AUTO] Configured printers detected. Starting the server automatically...",
        "error_auto_start": "[ERROR] Auto-start failed: {error}",
        "test_connecting": "[CONNECT] Testing connection to {host}:{port}...",
        "test_connection_done": "[OK] Connection test succeeded.",
        "test_connection_failed": "[ERROR] Connection test failed.",
        "test_pdf_convert": "[PDF] The target printer is a PDF printer. Converting the test data to PDF...",
        "test_pdf_convert_ok": "[OK] Test data converted to PDF ({size} bytes).",
        "test_pdf_convert_failed": "[WARN] PDF conversion failed. Sending the original test data instead.",
        "test_pdf_convert_error": "[WARN] PDF conversion error: {error}",
        "test_sending_data": "[SEND] Sending test data to {host}:{port} ({size} bytes)...",
        "test_send_ok": "[OK] Test data sent.",
        "test_send_failed": "[ERROR] Failed to send test data.",
        "language_switch_saving_failed": "[ERROR] Failed to save the language setting.",
        "language_switch_restarting": "[INFO] Applying the new language. The server will stop and restart automatically...",
        "language_switch_applied": "[OK] Language applied.",
        "language_option_zh-TW": "Traditional Chinese",
        "language_option_en": "English",
        "web_title": "PrtEasyServer Printer Setup",
        "web_eyebrow": "PRTEASYSERVER SERVICE",
        "web_heading": "Windows Network Print Server",
        "web_intro_1": "This server is currently publishing printer setup files from <strong>{host}</strong>.",
        "web_intro_2": "Download the matching setup file for the printer and port you want to add on the client device.",
        "web_meta_ip": "Server IP: {value}",
        "web_meta_entry": "Web entry: {value}",
        "web_meta_count": "Available printers: {count}",
        "web_printer_badge": "Printer {index}",
        "web_driver": "Driver:",
        "web_raw_port": "Raw port:",
        "web_host": "Host name:",
        "web_target": "TCP/IP target:",
        "web_download": "Download Setup",
        "web_empty_title": "No printers are configured yet",
        "web_empty_body": "Go back to {app_name}, configure at least one printer, and then refresh this page.",
        "web_footer": "Note: the downloaded BAT file creates the TCP/IP port by hostname. To open this page again, you can use {ip}.",
        "bat_success_title": "Install Complete",
        "bat_success_message": "{printer} installation completed.",
        "bat_missing_title": "Printer Driver Not Found",
        "bat_missing_message": "Driver not found: {driver}`r`n`r`nCreated port: {port}`r`nPlease add the printer manually and choose this port.",
        "client_socket_connected": "[OK] Connected.",
        "client_sending_bytes": "[SEND] Sending {size} bytes...",
        "client_data_sent": "[OK] Data sent.",
        "client_ping_only": "[OK] Connection test completed without sending data.",
        "client_connection_refused": "[ERROR] Connection refused. Make sure the server is running on {host}:{port}.",
        "client_timeout": "[ERROR] Connection timed out.",
        "client_unexpected_error": "[ERROR] Unexpected error: {error}",
        "quit_app_exit": "[BYE] Closing application...",
        "quit_app_stopping_server": "[STOP] Stopping server...",
        "quit_app_tray_stopped": "[TRAY] System tray icon stopped.",
        "quit_app_closed": "[OK] Application closed.",
        "tray_show_window": "Show Window",
        "tray_hide_window": "Hide Window",
        "tray_quit": "Quit",
        "firewall_trigger_launch": "App launch",
        "firewall_trigger_start": "Server start",
        "firewall_no_rules": "[FIREWALL] {trigger}: no firewall rules are needed.",
        "firewall_disabled": "[FIREWALL] Windows Firewall is disabled, so rule creation was skipped.",
        "firewall_status_unknown": "[WARN] Unable to confirm Windows Firewall status: {details}",
        "firewall_admin_required": "[WARN] Administrator rights are required to create firewall rules automatically.",
        "firewall_checking_enabled": "[FIREWALL] {trigger}: checking Windows Firewall rules ({details})...",
        "firewall_creating_disabled": "[FIREWALL] {trigger}: firewall is disabled, but {app_name} rules are still being prepared.",
        "firewall_rule_created": "[FIREWALL] Rule created: {name} (TCP {port})",
        "firewall_rule_updated": "[FIREWALL] Rule updated: {name} (TCP {port})",
        "firewall_rule_exists": "[FIREWALL] Rule already present: {name} (TCP {port})",
        "firewall_rule_failed": "[WARN] Firewall rule failed: {name} (TCP {port}) - {error}",
        "server_no_printer_config": "[!] No configured printers were found.",
        "server_duplicate_ports": "[!] Startup failed because printer ports must be unique.",
        "server_web_port_conflict": "[!] Startup failed because the web port {port} cannot match a printer port.",
        "server_kill_port": "[KILL] Checking whether port {port} is already in use...",
        "server_port_bound": "[OK] Port {port} is now listening.",
        "server_printer_bound": "[PRINTER] Bound printer: {printer}",
        "server_listen_address": "[CONNECT] Clients can connect to {ip}:{port}.",
        "server_multi_started": "[OK] Started {count} printer server(s).",
        "server_web_started": "[WEB] Web page started: {url}",
        "server_web_failed": "[WARN] Web page failed to start: {error}",
        "server_start_exception": "[!] Server startup failed: {error}",
        "server_stopped": "[DONE] All servers have been stopped.",
        "server_accept_error": "[!] Listener error: {error}",
        "web_download_logged": "[WEB] Served the BAT installer for printer {index}.",
        "autostart_add_success": "Added to Windows startup.",
        "autostart_add_failed": "Failed to add Windows startup entry: {error}",
        "autostart_remove_success": "Removed from Windows startup.",
        "autostart_not_configured": "Windows startup is not configured.",
        "autostart_remove_failed": "Failed to remove the Windows startup entry: {error}",
        "autostart_check_failed": "Failed to check Windows startup status: {error}",
    },
}

TRANSLATIONS["zh-TW"].update(
    {
        "web_intro_3": "設定檔及驅動程式建議放在同個目錄以便快速安裝。",
        "web_meta_ip": "伺服器主機：{value}",
        "web_download_driver": "下載驅動程式",
        "web_footer": "注意：下載的 BAT 會使用主機名稱建立 TCP/IP 連接埠。如需連線此設定頁，可使用 {ip}。",
        "web_footer_browser": "注意：下載檔案可能會被瀏覽器阻擋，可點「保留」繼續完成下載。",
        "web_footer_version": "伺服器目前版本：V{version}",
        "web_footer_latest": "查詢最新版本請前往：",
        "driver_prepare_background": "[DRIVER] 已在背景開始準備 {count} 個驅動程式壓縮檔。",
        "driver_package_exists": "[DRIVER] 已存在驅動程式壓縮檔：{file}",
        "driver_package_creating": "[DRIVER] 正在打包驅動程式：{driver} -> {file}",
        "driver_package_ready": "[DRIVER] 驅動程式壓縮檔已完成：{file}",
        "driver_package_failed": "[WARN] 驅動程式打包失敗：{driver} - {error}",
        "web_driver_download_logged": "[WEB] 已提供印表機{index} 的驅動程式壓縮檔：{file}",
        "web_driver_download_missing": "[WARN] 印表機{index} 的驅動程式壓縮檔目前無法提供：{error}",
    }
)

TRANSLATIONS["en"].update(
    {
        "web_intro_3": "Keep the setup file and the driver package in the same folder for faster installation.",
        "web_meta_ip": "Server Host: {value}",
        "web_download_driver": "Download Driver",
        "web_footer": "Note: the downloaded BAT file creates the TCP/IP port by hostname. To open this setup page again, you can use {ip}.",
        "web_footer_browser": "Note: your browser may block the downloaded file. Click \"Keep\" to continue the download.",
        "web_footer_version": "Current server version: V{version}",
        "web_footer_latest": "For the latest version, visit:",
        "driver_prepare_background": "[DRIVER] Background packaging started for {count} driver archive(s).",
        "driver_package_exists": "[DRIVER] Driver archive already exists: {file}",
        "driver_package_creating": "[DRIVER] Packaging driver: {driver} -> {file}",
        "driver_package_ready": "[DRIVER] Driver archive is ready: {file}",
        "driver_package_failed": "[WARN] Driver packaging failed: {driver} - {error}",
        "web_driver_download_logged": "[WEB] Served the driver archive for printer {index}: {file}",
        "web_driver_download_missing": "[WARN] The driver archive for printer {index} is not available yet: {error}",
    }
)

def normalize_language(language):
    """Normalize user/system language values into one of the supported codes."""
    if not language:
        return "en"

    value = str(language).strip().replace("_", "-").lower()
    if value in {"zh-tw", "zh-hk", "zh-mo", "zh-hant"}:
        return "zh-TW"
    if value.startswith("zh-") and "hant" in value:
        return "zh-TW"
    return "en"

def detect_default_language():
    """Pick zh-TW only for Traditional Chinese systems; otherwise default to English."""
    candidates = []

    try:
        language_id = ctypes.windll.kernel32.GetUserDefaultUILanguage()
        windows_locale = locale.windows_locale.get(language_id)
        if windows_locale:
            candidates.append(windows_locale)
    except Exception:
        pass

    try:
        system_locale = locale.getdefaultlocale()[0]
        if system_locale:
            candidates.append(system_locale)
    except Exception:
        pass

    for candidate in candidates:
        if normalize_language(candidate) == "zh-TW":
            return "zh-TW"

    return "en"

def translate_text(language, key, **kwargs):
    """Translate a UI key using the closest supported language."""
    normalized = normalize_language(language)
    bundle = TRANSLATIONS.get(normalized, TRANSLATIONS["en"])
    fallback_bundle = TRANSLATIONS["en"]
    text = bundle.get(key, fallback_bundle.get(key, key))
    return text.format(**kwargs) if kwargs else text

class PrinterOneServer:
    """PrinterOne TCP Server"""
    
    def __init__(self, log_callback=None):
        try:
            if startup_logger:
                startup_logger.info("Initializing PrinterOneServer...")
            
            self.config = self.load_config()
            
            if startup_logger:
                startup_logger.info(f"Configuration loaded: {self.config}")
            
            self.server_socket = None
            self.server_thread = None
            self.listeners = []
            self.active_printers = []
            self.http_server = None
            self.http_thread = None
            self.web_running = False
            self.web_error = ""
            self.running = False
            self.log_callback = log_callback  # Callback function for logging to GUI
            
            if startup_logger:
                startup_logger.info("PrinterOneServer initialized successfully")
                
        except Exception as e:
            error_msg = f"Failed to initialize PrinterOneServer: {e}"
            if startup_logger:
                startup_logger.critical(error_msg)
                startup_logger.critical(f"Traceback: {traceback.format_exc()}")
            raise
    
    def log(self, message):
        """Log message to console and GUI if callback is set"""
        print(message)  # Always print to console
        if self.log_callback:
            # Remove the timestamp and brackets from message for GUI (GUI adds its own)
            clean_message = message
            if message.startswith("[") and "]" in message:
                # Extract message after first "]"
                bracket_end = message.find("]")
                if bracket_end != -1:
                    clean_message = message[bracket_end + 1:].strip()
            self.log_callback(clean_message)

    def default_port_for_index(self, index):
        """Return the default port for a printer row."""
        return 9100 + (index * 100)

    def normalize_printer_entry(self, entry, index):
        """Normalize a single printer configuration entry."""
        entry = entry if isinstance(entry, dict) else {}
        printer_name = str(entry.get("printer_name", entry.get("name", ""))).strip()

        try:
            port = int(entry.get("port", self.default_port_for_index(index)))
        except (TypeError, ValueError):
            port = self.default_port_for_index(index)

        return {
            "printer_name": printer_name,
            "port": port,
        }

    def get_normalized_printers(self, printers=None, legacy_name="", legacy_port=None):
        """Return normalized printer configuration rows."""
        normalized = []

        if isinstance(printers, list) and printers:
            for index, entry in enumerate(printers):
                normalized.append(self.normalize_printer_entry(entry, index))
        else:
            default_port = legacy_port if legacy_port is not None else self.default_port_for_index(0)
            normalized.append(
                self.normalize_printer_entry(
                    {"printer_name": legacy_name, "port": default_port},
                    0,
                )
            )

        if not normalized:
            normalized.append(self.normalize_printer_entry({}, 0))

        return normalized

    def sync_legacy_printer_fields(self):
        """Keep legacy single-printer fields aligned with the first printer row."""
        printers = self.config.get("printers", [])
        first_printer = printers[0] if printers else {"printer_name": "", "port": self.default_port_for_index(0)}
        self.config["printer_name"] = first_printer.get("printer_name", "")
        self.config["port"] = first_printer.get("port", self.default_port_for_index(0))

    def get_active_printer_configs(self):
        """Return configured printer rows that have a printer name."""
        printers = self.get_normalized_printers(
            self.config.get("printers"),
            self.config.get("printer_name", ""),
            self.config.get("port", self.default_port_for_index(0)),
        )
        return [printer for printer in printers if printer.get("printer_name", "").strip()]

    def find_printer_config_by_port(self, port):
        """Find a printer configuration by listening port."""
        for printer in self.get_normalized_printers(
            self.config.get("printers"),
            self.config.get("printer_name", ""),
            self.config.get("port", self.default_port_for_index(0)),
        ):
            if printer.get("port") == port:
                return printer
        return None

    def get_web_port(self):
        """Return the configured web port."""
        try:
            port = int(self.config.get("web_port", 80))
        except (TypeError, ValueError):
            port = 80

        if port <= 0 or port > 65535:
            return 80
        return port

    def get_language(self):
        """Return the currently configured UI language."""
        return normalize_language(self.config.get("language", detect_default_language()))

    def tr(self, key, **kwargs):
        """Translate a UI/logging key using the current configuration language."""
        return translate_text(self.get_language(), key, **kwargs)

    def is_running_as_admin(self):
        """Return whether the current process has administrator rights."""
        try:
            return bool(ctypes.windll.shell32.IsUserAnAdmin())
        except Exception:
            return False

    def run_powershell(self, script):
        """Run a PowerShell script and return the completed process."""
        return subprocess.run(
            [
                "powershell",
                "-NoProfile",
                "-ExecutionPolicy",
                "Bypass",
                "-Command",
                script,
            ],
            capture_output=True,
            text=True,
            errors="ignore",
            **get_hidden_subprocess_kwargs(),
        )

    def get_firewall_status(self):
        """Check whether Windows Firewall is enabled on any profile."""
        script = (
            "$enabledProfiles = Get-NetFirewallProfile | Where-Object { $_.Enabled -eq $true }; "
            "if ($enabledProfiles.Count -gt 0) { "
            "  'ON:' + (($enabledProfiles | Select-Object -ExpandProperty Name) -join ','); "
            "} else { "
            "  'OFF'; "
            "}"
        )

        try:
            result = self.run_powershell(script)
            output = (result.stdout or "").strip()
            if result.returncode == 0:
                if output.startswith("ON:"):
                    return True, output[3:] or "Domain,Private,Public"
                if output == "OFF":
                    return False, ""
            details = (result.stderr or output or f"returncode={result.returncode}").strip()
            return None, details
        except Exception as e:
            return None, str(e)

    def ensure_firewall_rule(self, display_name, port, description):
        """Ensure one inbound TCP firewall rule exists."""
        script = "\n".join(
            [
                f"$displayName = '{self.escape_ps_single_quote(display_name)}'",
                f"$description = '{self.escape_ps_single_quote(description)}'",
                f"$port = {int(port)}",
                "$existing = Get-NetFirewallRule -DisplayName $displayName -ErrorAction SilentlyContinue",
                "if ($existing) {",
                "    $existing | Set-NetFirewallRule -Enabled True -Action Allow -Profile Any | Out-Null",
                "    'UPDATED'",
                "} else {",
                "    New-NetFirewallRule -DisplayName $displayName -Direction Inbound -Action Allow -Protocol TCP -LocalPort $port -Profile Any -Description $description | Out-Null",
                "    'CREATED'",
                "}",
            ]
        )
        return self.run_powershell(script)

    def ensure_firewall_rules(self, printer_configs=None, trigger=""):
        """Detect firewall status and create inbound rules for configured ports."""
        printer_configs = printer_configs if printer_configs is not None else self.get_active_printer_configs()
        web_port = self.get_web_port()
        display_trigger = trigger or "檢查"

        rules = []
        seen_ports = set()
        for printer in printer_configs:
            port = int(printer.get("port", 0))
            printer_name = printer.get("printer_name", "").strip() or f"印表機 {port}"
            if port > 0 and port not in seen_ports:
                seen_ports.add(port)
                rules.append(
                    (
                        f"{APP_NAME} Print {port}",
                        port,
                        f"{APP_NAME} 印表機 {printer_name} 的 Raw TCP 連接埠 {port}",
                    )
                )

        if web_port > 0 and web_port not in seen_ports:
            rules.append(
                (
                    f"{APP_NAME} Web {web_port}",
                    web_port,
                    f"{APP_NAME} 內建設定網頁連接埠 {web_port}",
                )
            )

        if not rules:
            self.log(f"[FIREWALL] {display_trigger}：目前沒有可建立規則的連接埠。")
            return True

        enabled, details = self.get_firewall_status()
        if enabled is False:
            self.log("[FIREWALL] 偵測到 Windows 防火牆目前未啟用，略過自動開通。")
            return True

        if enabled is None:
            self.log(f"[WARN] 無法判斷 Windows 防火牆狀態：{details}")

        if not self.is_running_as_admin():
            self.log("[WARN] 偵測到需要設定 Windows 防火牆，但目前不是系統管理員身分，無法自動開通。")
            return False

        if enabled:
            self.log(f"[FIREWALL] {display_trigger}：已偵測到 Windows 防火牆啟用中（{details}），正在檢查通道...")
        else:
            self.log(f"[FIREWALL] {display_trigger}：正在檢查並建立 {APP_NAME} 防火牆規則...")

        all_ok = True
        for display_name, port, description in rules:
            try:
                result = self.ensure_firewall_rule(display_name, port, description)
                status = (result.stdout or "").strip().splitlines()
                status_text = status[-1].strip() if status else ""
                if result.returncode == 0 and status_text in {"CREATED", "UPDATED"}:
                    action_text = "已建立" if status_text == "CREATED" else "已更新"
                    self.log(f"[FIREWALL] {action_text}規則：{display_name}（TCP {port}）")
                elif result.returncode == 0:
                    self.log(f"[FIREWALL] 規則已確認：{display_name}（TCP {port}）")
                else:
                    error_text = (result.stderr or result.stdout or f"returncode={result.returncode}").strip()
                    self.log(f"[WARN] 建立防火牆規則失敗：{display_name}（TCP {port}） - {error_text}")
                    all_ok = False
            except Exception as e:
                self.log(f"[WARN] 建立防火牆規則時發生錯誤：{display_name}（TCP {port}） - {e}")
                all_ok = False

        return all_ok
    
    def load_config(self):
        """Load configuration from config.json"""
        default_language = detect_default_language()
        default_config = {
            "printers": [
                {
                    "printer_name": "",
                    "port": 9100,
                }
            ],
            "printer_name": "",
            "port": 9100,
            "use_pdf_conversion": True,
            "save_pdf_file": False,
            "auto_start": False,
            "service_name": APP_NAME,
            "service_description": f"{APP_NAME} - Windows network print server",
            "manual": False,
            "web_port": 80,
            "minimize_to_tray": True,
            "language": default_language,
        }

        self.config_path = get_config_file_path()

        try:
            if os.path.exists(self.config_path):
                with open(self.config_path, 'r', encoding='utf-8') as f:
                    config = json.load(f)

                for key, value in default_config.items():
                    if key not in config:
                        config[key] = value

                if config.get("service_name") in {"PrinterOne", "", None}:
                    config["service_name"] = APP_NAME
                if config.get("service_description") in {
                    "PrinterOne - Network print server for raw print data",
                    "",
                    None,
                }:
                    config["service_description"] = f"{APP_NAME} - Windows network print server"
                config["language"] = normalize_language(config.get("language", default_language))

                config["printers"] = self.get_normalized_printers(
                    config.get("printers"),
                    config.get("printer_name", ""),
                    config.get("port", self.default_port_for_index(0)),
                )
                self.config = config
                self.sync_legacy_printer_fields()

                return self.config
        except Exception as e:
            self.log(f"[!] 載入設定檔失敗：{e}")

        default_config["printers"] = self.get_normalized_printers(
            default_config.get("printers"),
            default_config.get("printer_name", ""),
            default_config.get("port", self.default_port_for_index(0)),
        )
        return default_config
    
    def save_config(
        self,
        printer_name=None,
        port=None,
        use_pdf_conversion=None,
        save_pdf_file=None,
        printers=None,
        web_port=None,
        language=None,
    ):
        """Save configuration to config.json"""
        try:
            current_printers = self.get_normalized_printers(
                self.config.get("printers"),
                self.config.get("printer_name", ""),
                self.config.get("port", self.default_port_for_index(0)),
            )

            if printers is not None:
                self.config["printers"] = self.get_normalized_printers(printers)
            else:
                if printer_name is not None:
                    current_printers[0]["printer_name"] = str(printer_name).strip()
                if port is not None:
                    try:
                        current_printers[0]["port"] = int(port)
                    except (TypeError, ValueError):
                        pass
                self.config["printers"] = self.get_normalized_printers(current_printers)

            if use_pdf_conversion is not None:
                self.config["use_pdf_conversion"] = use_pdf_conversion
            if save_pdf_file is not None:
                self.config["save_pdf_file"] = save_pdf_file
            if web_port is not None:
                self.config["web_port"] = int(web_port)
            if language is not None:
                self.config["language"] = normalize_language(language)
            
            self.config["manual"] = True
            self.sync_legacy_printer_fields()

            self.config_path = get_config_file_path()
            config_dir = os.path.dirname(self.config_path)
            if config_dir and not os.path.exists(config_dir):
                os.makedirs(config_dir, exist_ok=True)

            with open(self.config_path, 'w', encoding='utf-8') as f:
                json.dump(self.config, f, indent=4, ensure_ascii=False)

            self.log(f"[SAVE] 設定已儲存至 {self.config_path}")
            return True

        except Exception as e:
            self.log(f"[!] 儲存設定檔失敗：{e}")
            return False
    
    def list_printers(self):
        """List all available printers"""
        try:
            printers = []
            for printer in win32print.EnumPrinters(win32print.PRINTER_ENUM_LOCAL, None, 1):
                printers.append(printer[2])
            return printers
        except Exception as e:
            self.log(f"[!] 讀取印表機清單時發生錯誤：{e}")
            return []

    def get_printer_details(self, printer_name):
        """Get metadata for a local printer."""
        details = {
            "printer_name": printer_name,
            "driver_name": printer_name,
            "port_name": "",
        }

        if not printer_name:
            return details

        h_printer = None
        try:
            h_printer = win32print.OpenPrinter(printer_name)
            info = win32print.GetPrinter(h_printer, 2)
            details["printer_name"] = info.get("pPrinterName", printer_name) or printer_name
            details["driver_name"] = info.get("pDriverName", printer_name) or printer_name
            details["port_name"] = info.get("pPortName", "") or ""
        except Exception:
            pass
        finally:
            if h_printer:
                try:
                    win32print.ClosePrinter(h_printer)
                except Exception:
                    pass

        return details

    def get_server_host_name(self):
        """Resolve the best host name to share with clients."""
        candidates = []
        local_ip = self.get_local_ip()

        try:
            reverse_name = socket.gethostbyaddr(local_ip)[0]
            if reverse_name:
                candidates.append(reverse_name)
        except Exception:
            pass

        env_name = os.environ.get("COMPUTERNAME", "").strip()
        if env_name:
            candidates.append(env_name)

        socket_name = socket.gethostname().strip()
        if socket_name:
            candidates.append(socket_name)

        for candidate in candidates:
            short_name = candidate.split(".")[0].strip()
            if short_name and short_name != local_ip:
                return short_name

        return "localhost"

    def sanitize_port_name_component(self, value):
        """Sanitize a string for use in a Windows printer port name."""
        cleaned = re.sub(r"[^A-Za-z0-9]+", "_", str(value).strip().upper())
        cleaned = cleaned.strip("_")
        return cleaned or "PRINTERONE"

    def escape_ps_single_quote(self, value):
        """Escape a string for single-quoted PowerShell literals."""
        return str(value).replace("'", "''")

    def get_web_printer_entries(self):
        """Return active printers enriched with metadata for the web page."""
        printers = self.active_printers if self.active_printers else self.get_active_printer_configs()
        local_ip = self.get_local_ip()
        host_name = self.get_server_host_name()
        host_token = self.sanitize_port_name_component(host_name)
        entries = []

        for index, printer in enumerate(printers, 1):
            printer_name = printer.get("printer_name", "").strip()
            port = int(printer.get("port", self.default_port_for_index(index - 1)))
            details = self.get_printer_details(printer_name)
            driver_name = details.get("driver_name") or printer_name

            entries.append(
                {
                    "index": index,
                    "printer_name": printer_name,
                    "driver_name": driver_name,
                    "port": port,
                    "host_name": host_name,
                    "host_ip": local_ip,
                    "port_name": f"IP_{host_token}_{port}",
                }
            )

        return entries

    def build_installer_batch_content(self, printer_index):
        """Build the downloadable BAT installer for one printer."""
        entries = self.get_web_printer_entries()
        entry = next((item for item in entries if item["index"] == printer_index), None)
        if not entry:
            raise KeyError("printer_not_found")

        printer_name = self.escape_ps_single_quote(entry["printer_name"])
        driver_name = self.escape_ps_single_quote(entry["driver_name"])
        host_name = self.escape_ps_single_quote(entry["host_name"])
        host_ip = self.escape_ps_single_quote(entry["host_ip"])
        port_name = self.escape_ps_single_quote(entry["port_name"])
        port_number = int(entry["port"])

        powershell_script = "\n".join(
            [
                "$ErrorActionPreference = 'Stop'",
                "Add-Type -AssemblyName System.Windows.Forms",
                f"$printerName = '{printer_name}'",
                f"$driverName = '{driver_name}'",
                f"$portName = '{port_name}'",
                f"$hostName = '{host_name}'",
                f"$hostIp = '{host_ip}'",
                f"$portNumber = {port_number}",
                "$targetHost = $hostName",
                "try { [System.Net.Dns]::GetHostAddresses($hostName) | Out-Null } catch { $targetHost = $hostIp }",
                "if (-not (Get-PrinterPort -Name $portName -ErrorAction SilentlyContinue)) {",
                "    Add-PrinterPort -Name $portName -PrinterHostAddress $targetHost -PortNumber $portNumber",
                "}",
                "function Resolve-Driver($targetName) {",
                "    if ([string]::IsNullOrWhiteSpace($targetName)) {",
                "        return $null",
                "    }",
                "    $matched = Get-PrinterDriver -Name $targetName -ErrorAction SilentlyContinue",
                "    if (-not $matched) {",
                "        $matched = Get-PrinterDriver -ErrorAction SilentlyContinue |",
                "            Where-Object { $_.Name -eq $targetName -or $_.Name -like ($targetName + '*') -or $targetName -like ($_.Name + '*') } |",
                "            Select-Object -First 1",
                "    }",
                "    return $matched",
                "}",
                "$driverExists = Resolve-Driver $driverName",
                "if ($driverExists) {",
                "    $driverName = $driverExists.Name",
                "}",
                "if (-not $driverExists -and (Test-Path -LiteralPath $driverArchivePath)) {",
                "    try {",
                "        if (Test-Path -LiteralPath $driverTempRoot) {",
                "            Remove-Item -LiteralPath $driverTempRoot -Recurse -Force",
                "        }",
                "        Expand-Archive -LiteralPath $driverArchivePath -DestinationPath $driverTempRoot -Force",
                "        $infWildcard = Join-Path -Path $driverTempRoot -ChildPath '*.inf'",
                "        & pnputil.exe /add-driver $infWildcard /subdirs /install | Out-Null",
                "        Start-Sleep -Milliseconds 800",
                "        $driverExists = Resolve-Driver $driverName",
                "        if ($driverExists) {",
                "            $driverName = $driverExists.Name",
                "        }",
                "    } catch {",
                "    } finally {",
                "        if (Test-Path -LiteralPath $driverTempRoot) {",
                "            try { Remove-Item -LiteralPath $driverTempRoot -Recurse -Force } catch {}",
                "        }",
                "    }",
                "}",
                "$openPrintersFolder = {",
                "    try {",
                "        $shell = New-Object -ComObject Shell.Application",
                "        $shell.Open('shell:PrintersFolder')",
                "        Start-Sleep -Milliseconds 900",
                "        $wshell = New-Object -ComObject WScript.Shell",
                "        foreach ($title in @('印表機', '裝置和印表機', 'Devices and Printers', 'Printers')) {",
                "            if ($wshell.AppActivate($title)) {",
                "                Start-Sleep -Milliseconds 150",
                "                $wshell.SendKeys('^{HOME}')",
                "                Start-Sleep -Milliseconds 100",
                "                $wshell.SendKeys('{HOME}')",
                "                break",
                "            }",
                "        }",
                "    } catch {",
                "        Start-Process explorer.exe -ArgumentList 'shell:PrintersFolder'",
                "    }",
                "}",
                "if ($driverExists) {",
                "    if (-not (Get-Printer -Name $printerName -ErrorAction SilentlyContinue)) {",
                "        Add-Printer -Name $printerName -DriverName $driverName -PortName $portName",
                "    }",
                "    [System.Windows.Forms.MessageBox]::Show(",
                "        ($printerName + ' 印表機安裝完成'),",
                "        '安裝完成',",
                "        [System.Windows.Forms.MessageBoxButtons]::OK,",
                "        [System.Windows.Forms.MessageBoxIcon]::Information",
                "    ) | Out-Null",
                "    & $openPrintersFolder",
                "} else {",
                "    [System.Windows.Forms.MessageBox]::Show(",
                "        ('找不到驅動：' + $driverName + \"`r`n`r`n已建立連接埠：\" + $portName + \"`r`n請手動新增印表機並選擇這個連接埠。\"),",
                "        '找不到印表機驅動',",
                "        [System.Windows.Forms.MessageBoxButtons]::OK,",
                "        [System.Windows.Forms.MessageBoxIcon]::Warning",
                "    ) | Out-Null",
                "    Start-Process rundll32.exe -ArgumentList 'printui.dll,PrintUIEntry /il'",
                "}",
            ]
        )
        encoded_command = base64.b64encode(powershell_script.encode("utf-16le")).decode("ascii")
        encoded_chunks = [encoded_command[i:i + 240] for i in range(0, len(encoded_command), 240)]

        lines = [
            "@echo off",
            "setlocal",
            "",
            'set "B64FILE=%TEMP%\\PrtEasyServer_%RANDOM%_%RANDOM%.b64"',
            '> "%B64FILE%" (',
        ]
        lines.extend([f"echo {chunk}" for chunk in encoded_chunks])
        lines.extend(
            [
                ")",
                "",
                "powershell -NoProfile -ExecutionPolicy Bypass -Command \"$encoded = ((Get-Content -LiteralPath $env:B64FILE -Raw) -replace '\\s',''); $script = [Text.Encoding]::Unicode.GetString([Convert]::FromBase64String($encoded)); & ([scriptblock]::Create($script))\"",
                'set "ERR=%ERRORLEVEL%"',
                'del "%B64FILE%" >nul 2>nul',
                'if not "%ERR%"=="0" exit /b %ERR%',
                "",
                "endlocal",
                "exit /b 0",
                "",
            ]
        )

        safe_index = entry["index"]
        safe_port = entry["port"]
        safe_name = self.sanitize_port_name_component(entry["printer_name"])[:24]
        filename = f"{APP_NAME}_Setup_{safe_index}_{safe_name}_{safe_port}.bat"
        return "\r\n".join(lines), filename

    def render_web_page(self):
        """Render the built-in printer setup web page."""
        entries = self.get_web_printer_entries()
        host_name = self.get_server_host_name()
        local_ip = self.get_local_ip()
        web_port = self.get_web_port()

        cards = []
        for entry in entries:
            download_url = f"/download/{entry['index']}.bat"
            cards.append(
                f"""
                <section class="printer-card">
                    <div class="printer-label">印表機{entry['index']}</div>
                    <h2>{html.escape(entry['printer_name'])}</h2>
                    <p><strong>驅動：</strong>{html.escape(entry['driver_name'])}</p>
                    <p><strong>Raw 連接埠：</strong>{entry['port']}</p>
                    <p><strong>安裝主機：</strong>{html.escape(entry['host_name'])}</p>
                    <p><strong>建議 TCP/IP 位址：</strong>{html.escape(entry['host_name'])}</p>
                    <a class="download-button" href="{download_url}">下載設定檔</a>
                </section>
                """
            )

        if not cards:
            cards.append(
                """
                <section class="empty-state">
                    <h2>目前沒有可用的印表機設定</h2>
                    <p>請先回到 {APP_NAME} 管理介面設定印表機，並啟動伺服器。</p>
                </section>
                """
            )

        access_hint = local_ip if web_port == 80 else f"{local_ip}:{web_port}"

        return f"""<!DOCTYPE html>
<html lang="zh-Hant">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>{APP_NAME} 印表機服務</title>
    <style>
        :root {{
            color-scheme: light;
            --bg-top: #eff7ea;
            --bg-bottom: #f7fbff;
            --panel: rgba(255, 255, 255, 0.92);
            --line: #d7e1d4;
            --text: #173127;
            --muted: #567263;
            --accent: #2d8559;
            --accent-dark: #1d5d3c;
            --shadow: 0 18px 50px rgba(21, 54, 39, 0.12);
        }}
        * {{
            box-sizing: border-box;
        }}
        body {{
            margin: 0;
            font-family: "Microsoft JhengHei", "Segoe UI", sans-serif;
            color: var(--text);
            background:
                radial-gradient(circle at top left, rgba(180, 220, 170, 0.45), transparent 28%),
                linear-gradient(160deg, var(--bg-top), var(--bg-bottom));
            min-height: 100vh;
        }}
        .page {{
            width: min(1120px, calc(100% - 32px));
            margin: 0 auto;
            padding: 32px 0 40px;
        }}
        .hero {{
            background: linear-gradient(140deg, rgba(255,255,255,0.96), rgba(243, 250, 247, 0.92));
            border: 1px solid rgba(174, 198, 184, 0.65);
            border-radius: 28px;
            box-shadow: var(--shadow);
            padding: 28px 28px 24px;
            position: relative;
            overflow: hidden;
        }}
        .hero::after {{
            content: "";
            position: absolute;
            inset: auto -60px -70px auto;
            width: 200px;
            height: 200px;
            background: radial-gradient(circle, rgba(45, 133, 89, 0.18), transparent 68%);
        }}
        .eyebrow {{
            display: inline-flex;
            align-items: center;
            gap: 8px;
            padding: 8px 14px;
            border-radius: 999px;
            background: rgba(45, 133, 89, 0.10);
            color: var(--accent-dark);
            font-size: 13px;
            font-weight: 700;
            letter-spacing: 0.08em;
        }}
        h1 {{
            margin: 18px 0 10px;
            font-size: clamp(28px, 5vw, 44px);
            line-height: 1.08;
        }}
        .hero p {{
            margin: 6px 0;
            color: var(--muted);
            font-size: 16px;
            line-height: 1.7;
        }}
        .meta {{
            display: flex;
            flex-wrap: wrap;
            gap: 12px;
            margin-top: 20px;
        }}
        .meta-chip {{
            padding: 10px 14px;
            border-radius: 14px;
            border: 1px solid rgba(174, 198, 184, 0.7);
            background: rgba(255,255,255,0.85);
            font-size: 14px;
        }}
        .grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
            gap: 18px;
            margin-top: 24px;
        }}
        .printer-card, .empty-state {{
            background: var(--panel);
            border: 1px solid rgba(174, 198, 184, 0.72);
            border-radius: 22px;
            padding: 22px;
            box-shadow: 0 10px 24px rgba(24, 52, 39, 0.08);
        }}
        .printer-label {{
            display: inline-block;
            padding: 6px 11px;
            border-radius: 999px;
            background: rgba(45, 133, 89, 0.12);
            color: var(--accent-dark);
            font-size: 13px;
            font-weight: 700;
            margin-bottom: 14px;
        }}
        .printer-card h2 {{
            margin: 0 0 12px;
            font-size: 22px;
            line-height: 1.25;
        }}
        .printer-card p {{
            margin: 8px 0;
            color: var(--muted);
            line-height: 1.6;
        }}
        .download-button {{
            display: inline-flex;
            align-items: center;
            justify-content: center;
            margin-top: 18px;
            padding: 12px 18px;
            min-width: 160px;
            border-radius: 14px;
            background: linear-gradient(135deg, var(--accent), var(--accent-dark));
            color: #fff;
            text-decoration: none;
            font-weight: 700;
            box-shadow: 0 12px 24px rgba(45, 133, 89, 0.22);
        }}
        .download-button:hover {{
            filter: brightness(1.04);
        }}
        .footer-note {{
            margin-top: 18px;
            color: var(--muted);
            font-size: 14px;
            line-height: 1.7;
        }}
        @media (max-width: 640px) {{
            .page {{
                width: min(100% - 20px, 1120px);
                padding-top: 18px;
            }}
            .hero {{
                padding: 22px 18px 18px;
                border-radius: 22px;
            }}
            .printer-card, .empty-state {{
                padding: 18px;
            }}
        }}
    </style>
</head>
<body>
    <main class="page">
        <header class="hero">
            <span class="eyebrow">PRINTERONE SERVICE</span>
            <h1>網路印表機設定中心</h1>
            <p>這台伺服器目前由 <strong>{html.escape(host_name)}</strong> 提供印表機分享服務。</p>
            <p>在其他電腦下載對應的設定檔後直接執行，即可快速建立印表機連接埠與印表機項目。</p>
            <div class="meta">
                <div class="meta-chip">伺服器 IP：{html.escape(local_ip)}</div>
                <div class="meta-chip">網頁入口：{html.escape(access_hint)}</div>
                <div class="meta-chip">共用印表機數量：{len(entries)}</div>
            </div>
        </header>
        <section class="grid">
            {''.join(cards)}
        </section>
        <p class="footer-note">提醒：下載的 BAT 會優先使用主機名稱連線；若名稱無法解析，會自動改用目前這台伺服器的 IP 位址 {html.escape(local_ip)}。</p>
    </main>
</body>
</html>
"""

    def create_web_request_handler(self):
        """Create the HTTP handler bound to this app server."""
        owner = self

        class PrinterWebHandler(BaseHTTPRequestHandler):
            server_version = f"{APP_NAME}HTTP/1.0"

            def log_message(self, format, *args):
                return

            def do_GET(self):
                parsed = urlparse(self.path)
                if parsed.path in ("/", "/index.html"):
                    body = owner.render_web_page().encode("utf-8")
                    self.send_response(200)
                    self.send_header("Content-Type", "text/html; charset=utf-8")
                    self.send_header("Content-Length", str(len(body)))
                    self.end_headers()
                    self.wfile.write(body)
                    return

                if parsed.path.startswith("/download/") and parsed.path.endswith(".bat"):
                    name_part = parsed.path.rsplit("/", 1)[-1]
                    try:
                        printer_index = int(name_part.split(".", 1)[0])
                        batch_content, filename = owner.build_installer_batch_content(printer_index)
                    except (ValueError, KeyError):
                        self.send_error(404, "Printer not found")
                        return

                    body = batch_content.encode("ascii")
                    ascii_name = re.sub(r"[^A-Za-z0-9._-]+", "_", filename)
                    self.send_response(200)
                    self.send_header("Content-Type", "application/octet-stream")
                    self.send_header(
                        "Content-Disposition",
                        f"attachment; filename=\"{ascii_name}\"; filename*=UTF-8''{quote(filename)}",
                    )
                    self.send_header("Content-Length", str(len(body)))
                    self.end_headers()
                    self.wfile.write(body)
                    owner.log(f"[WEB] 已提供印表機{printer_index} 的 BAT 設定檔下載")
                    return

                self.send_error(404, "Not found")

        return PrinterWebHandler

    def start_web_server(self):
        """Start the built-in setup web server."""
        web_port = self.get_web_port()
        handler_class = self.create_web_request_handler()

        try:
            self.http_server = ThreadingHTTPServer(("0.0.0.0", web_port), handler_class)
            self.http_thread = threading.Thread(
                target=self.http_server.serve_forever,
                kwargs={"poll_interval": 0.5},
                daemon=True,
            )
            self.http_thread.start()
            self.web_running = True
            self.web_error = ""
            return True
        except Exception as e:
            self.http_server = None
            self.http_thread = None
            self.web_running = False
            self.web_error = str(e)
            return False

    def stop_web_server(self):
        """Stop the built-in setup web server."""
        if self.http_server:
            try:
                self.http_server.shutdown()
            except Exception:
                pass
            try:
                self.http_server.server_close()
            except Exception:
                pass

        self.http_server = None
        self.http_thread = None
        self.web_running = False
        self.web_error = ""
    
    def convert_raw_to_pdf(self, raw_data, save_file=False):
        """Convert raw data to PDF for testing with PDF printers (test client only)"""
        try:
            temp_pdf = tempfile.NamedTemporaryFile(delete=False, suffix='.pdf')
            temp_pdf_path = temp_pdf.name
            temp_pdf.close()
            
            self.log(f"[INFO] 正在建立 PDF：{temp_pdf_path}")
            
            c = canvas.Canvas(temp_pdf_path, pagesize=letter)
            data_format = self.analyze_raw_data(raw_data)
            
            # Add title
            c.setFont("Helvetica-Bold", 16)
            c.drawString(100, 750, f"{APP_NAME} - Test Print Job")
            
            # Add info
            c.setFont("Helvetica", 12)
            c.drawString(100, 720, f"Data length: {len(raw_data)} bytes")
            c.drawString(100, 700, f"Data format: {data_format}")
            c.drawString(100, 680, f"Received at: {time.strftime('%Y-%m-%d %H:%M:%S')}")
            
            y_position = 640
            
            # Try to extract and display readable text first
            try:
                text_content = self.extract_readable_text(raw_data)
                if text_content and len(text_content.strip()) > 0:
                    c.setFont("Helvetica-Bold", 14)
                    c.drawString(100, y_position, "Content:")
                    y_position -= 30
                    
                    # Display text content
                    c.setFont("Helvetica", 11)
                    lines = text_content.split('\n')
                    
                    for line in lines:
                        if y_position < 80:
                            c.showPage()  # New page
                            y_position = 750
                        
                        # Wrap long lines
                        if len(line) > 80:
                            for i in range(0, len(line), 80):
                                chunk = line[i:i+80]
                                c.drawString(100, y_position, chunk)
                                y_position -= 15
                                if y_position < 80:
                                    c.showPage()
                                    y_position = 750
                        else:
                            c.drawString(100, y_position, line)
                            y_position -= 15
                    
                    y_position -= 20
                else:
                    # If no readable text, show hex and ASCII as before
                    self.add_hex_dump_to_pdf(c, raw_data, y_position)
                    
            except Exception as e:
                self.log(f"[WARN] 擷取文字時發生錯誤，改用十六進位顯示：{e}")
                self.add_hex_dump_to_pdf(c, raw_data, y_position)
            
            c.save()
            
            # Read the PDF file
            with open(temp_pdf_path, 'rb') as f:
                pdf_data = f.read()
            
            # Clean up or save file
            if save_file:
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                saved_path = f"raw_data_{timestamp}.pdf"
                os.rename(temp_pdf_path, saved_path)
                self.log(f"[SAVE] PDF 已儲存為：{saved_path}")
            else:
                try:
                    os.unlink(temp_pdf_path)
                except:
                    pass
            
            return pdf_data
        except Exception as e:
            self.log(f"[!] PDF 轉換錯誤：{e}")
            return None
    
    def extract_readable_text(self, raw_data):
        """Extract readable text from raw data"""
        try:
            # Try UTF-8 first
            try:
                text = raw_data.decode('utf-8')
                # Clean up control characters but keep printable ones
                cleaned = ''.join(char if char.isprintable() or char in '\n\r\t' else ' ' for char in text)
                return cleaned.strip()
            except UnicodeDecodeError:
                pass
            
            # Try Windows-1252 (common in Windows printing)
            try:
                text = raw_data.decode('windows-1252', errors='ignore')
                cleaned = ''.join(char if char.isprintable() or char in '\n\r\t' else ' ' for char in text)
                return cleaned.strip()
            except:
                pass
            
            # Try ASCII with error handling
            try:
                text = raw_data.decode('ascii', errors='ignore')
                cleaned = ''.join(char if char.isprintable() or char in '\n\r\t' else ' ' for char in text)
                return cleaned.strip()
            except:
                pass
            
            return None
        except Exception as e:
            self.log(f"[WARN] 文字擷取錯誤：{e}")
            return None
    
    def add_hex_dump_to_pdf(self, canvas_obj, raw_data, start_y):
        """Add hex dump to PDF (fallback method)"""
        try:
            y_position = start_y
            
            # Add hex dump section
            canvas_obj.setFont("Helvetica-Bold", 12)
            canvas_obj.drawString(100, y_position, "Raw Data (Hex):")
            y_position -= 30
            
            # Add raw data as hex (first 2000 bytes)
            canvas_obj.setFont("Courier", 8)
            hex_data = raw_data[:2000].hex()
            
            # Split hex data into lines
            for i in range(0, len(hex_data), 80):
                if y_position < 50:
                    canvas_obj.showPage()
                    y_position = 750
                
                line = hex_data[i:i+80]
                formatted_line = ' '.join([line[j:j+2] for j in range(0, len(line), 2)])
                canvas_obj.drawString(100, y_position, formatted_line)
                y_position -= 12
            
            # Add ASCII representation
            y_position -= 20
            if y_position < 100:
                canvas_obj.showPage()
                y_position = 750
            
            canvas_obj.setFont("Helvetica-Bold", 12)
            canvas_obj.drawString(100, y_position, "ASCII representation:")
            y_position -= 20
            
            canvas_obj.setFont("Courier", 8)
            ascii_data = raw_data[:1000]
            ascii_line = ''
            for i, byte in enumerate(ascii_data):
                if 32 <= byte <= 126:
                    ascii_line += chr(byte)
                else:
                    ascii_line += '.'
                
                if (i + 1) % 80 == 0:
                    if y_position < 50:
                        canvas_obj.showPage()
                        y_position = 750
                    canvas_obj.drawString(100, y_position, ascii_line)
                    y_position -= 12
                    ascii_line = ''
            
            if ascii_line:
                canvas_obj.drawString(100, y_position, ascii_line)
                
        except Exception as e:
            self.log(f"[WARN] 十六進位輸出錯誤：{e}")
    
    def analyze_raw_data(self, data):
        """Analyze raw data to determine format"""
        if len(data) == 0:
            return "Empty data"
        
        # Check for common printer command formats
        if data.startswith(b'\x1b'):
            return "ESC/P (Epson)"
        elif data.startswith(b'\x1b%-12345X'):
            return "PCL (HP)"
        elif data.startswith(b'%!PS'):
            return "PostScript"
        elif data.startswith(b'\x02'):
            return "ZPL (Zebra)"
        elif b'PDF' in data[:100]:
            return "PDF document"
        elif b'Microsoft Office' in data or b'Word' in data or b'.docx' in data or b'.doc' in data:
            return "Microsoft Office document"
        elif b'%PDF' in data[:100]:
            return "PDF format"
        else:
            # Try to detect if it contains printable text
            try:
                decoded = data.decode('utf-8', errors='ignore')
                if len(decoded.strip()) > 0 and any(c.isprintable() and c not in '\r\n\t' for c in decoded[:200]):
                    return f"Text document ({len(data)} bytes)"
            except:
                pass
            
            return f"Binary/Unknown format ({len(data)} bytes)"
    
    def print_raw(self, data, printer_name):
        """Send raw data to printer"""
        try:
            self.log(f"[INFO] 正在開啟印表機：{printer_name}")
            hPrinter = win32print.OpenPrinter(printer_name)
            
            job_info = ("RAW Print Job", None, "RAW")
            hJob = win32print.StartDocPrinter(hPrinter, 1, job_info)
            win32print.StartPagePrinter(hPrinter)
            win32print.WritePrinter(hPrinter, data)
            win32print.EndPagePrinter(hPrinter)
            win32print.EndDocPrinter(hPrinter)
            win32print.ClosePrinter(hPrinter)
            
            self.log(f"[OK] 已成功列印 {len(data)} 位元組。")
            return True
        except Exception as e:
            self.log(f"[!] 列印錯誤：{e}")
            return False
    
    
    def handle_client(self, client_socket, address, printer_config):
        """Handle a client connection"""
        printer_name = printer_config.get("printer_name", "")
        port = printer_config.get("port", 0)
        target_label = f"{printer_name}（{port}）"

        self.log(f"[CONN] 用戶端已連線：{address} -> {target_label}")
        try:
            data = b""
            while True:
                chunk = client_socket.recv(4096)
                if not chunk:
                    break
                data += chunk
            
            if data:
                self.log(f"[DATA] 已從 {address} 接收 {len(data)} 位元組 -> {target_label}")
                self.log(f"[INFO] 資料格式：{self.analyze_raw_data(data)}")
                if printer_name:
                    self.print_raw(data, printer_name)
                else:
                    self.log("[!] 尚未設定印表機")
            else:
                self.log(f"[!] 未從 {address} 接收到資料 -> {target_label}")
                
        except Exception as e:
            self.log(f"[!] 處理用戶端 {address} 時發生錯誤：{e}")
        finally:
            client_socket.close()
            self.log(f"[CONN] 用戶端已中斷連線：{address} -> {target_label}")
    
    def kill_process_on_port(self, port):
        """Kill any process using the specified port"""
        try:
            # Use psutil instead of netstat to avoid snmpapi.dll dependency
            for conn in psutil.net_connections():
                if conn.laddr.port == port and conn.status == psutil.CONN_LISTEN:
                    try:
                        process = psutil.Process(conn.pid)
                        process.terminate()
                        self.log(f"[KILL] 已終止使用連接埠 {port} 的程序 {conn.pid}（{process.name()}）")
                        time.sleep(1)
                        
                        # Force kill if still running
                        if process.is_running():
                            process.kill()
                        self.log(f"[KILL] 已強制結束程序 {conn.pid}")
                    except (psutil.NoSuchProcess, psutil.AccessDenied) as e:
                        self.log(f"[!] 無法結束程序 {conn.pid}：{e}")
        except Exception as e:
            self.log(f"[!] 結束使用連接埠 {port} 的程序時發生錯誤：{e}")
    
    def get_local_ip(self):
        """Get the actual local IP address of the machine"""
        try:
            # Method 1: Try to connect to a remote server to determine local IP
            test_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            try:
                # Connect to Google DNS (doesn't actually send data)
                test_socket.connect(("8.8.8.8", 80))
                local_ip = test_socket.getsockname()[0]
                test_socket.close()
                
                # Validate that it's not a loopback address or VirtualBox
                if (not local_ip.startswith('127.') and 
                    not local_ip.startswith('192.168.56.') and  # VirtualBox Host-Only
                    not local_ip.startswith('169.254.')):       # APIPA
                    return local_ip
            except:
                test_socket.close()
            
            # Method 2: Use psutil to get network interfaces with better filtering
            try:
                import psutil
                interfaces_with_gw = []
                interfaces_without_gw = []
                
                # Get default gateways to identify primary interfaces
                gateways = psutil.net_if_stats()
                
                for interface_name, interface_addresses in psutil.net_if_addrs().items():
                    # Skip known virtual interfaces
                    if any(skip in interface_name.lower() for skip in [
                        'virtualbox', 'vmware', 'vbox', 'hyper-v', 'loopback', 
                        'bluetooth', 'isatap', 'teredo', 'tunnel'
                    ]):
                        continue
                    
                    for address in interface_addresses:
                        if address.family == socket.AF_INET:
                            ip = address.address
                            
                            # Skip loopback, APIPA, and VirtualBox IPs
                            if (ip.startswith('127.') or 
                                ip.startswith('169.254.') or
                                ip.startswith('192.168.56.')):  # VirtualBox Host-Only
                                continue
                            
                            # Check if this interface is up and running
                            try:
                                interface_stats = psutil.net_if_stats().get(interface_name)
                                if interface_stats and interface_stats.isup:
                                    # Prefer Wi-Fi and Ethernet over other interfaces
                                    if any(pref in interface_name.lower() for pref in ['wi-fi', 'wifi', 'ethernet', 'local area']):
                                        interfaces_with_gw.append((ip, interface_name))
                                    else:
                                        interfaces_without_gw.append((ip, interface_name))
                            except:
                                pass
                
                # Return the best interface
                if interfaces_with_gw:
                    # Prefer Wi-Fi over Ethernet if both available
                    for ip, name in interfaces_with_gw:
                        if 'wi-fi' in name.lower() or 'wifi' in name.lower():
                            return ip
                    # Otherwise return first good interface
                    return interfaces_with_gw[0][0]
                
                if interfaces_without_gw:
                    return interfaces_without_gw[0][0]
                    
            except Exception as e:
                if startup_logger:
                    startup_logger.warning(f"Method 2 failed: {e}")
            
            # Method 3: Fallback to hostname resolution
            try:
                hostname = socket.gethostname()
                local_ip = socket.gethostbyname(hostname)
                if (not local_ip.startswith('127.') and 
                    not local_ip.startswith('192.168.56.')):
                    return local_ip
            except:
                pass
            
            # Method 4: Last resort - return localhost
            return '127.0.0.1'
            
        except Exception as e:
            if startup_logger:
                startup_logger.error(f"Error getting local IP: {e}")
            return '127.0.0.1'
    
    def accept_loop(self, listener):
        """Accept connections for one printer listener."""
        server_socket = listener["socket"]
        printer_config = listener["printer_config"]

        while self.running:
            try:
                client_socket, address = server_socket.accept()
                client_thread = threading.Thread(
                    target=self.handle_client,
                    args=(client_socket, address, printer_config),
                    daemon=True,
                )
                client_thread.start()
            except socket.timeout:
                continue
            except OSError as e:
                if self.running:
                    self.log(f"[!] 接受用戶端連線時發生錯誤：{e}")
                break
            except Exception as e:
                if self.running:
                    self.log(f"[!] 接受用戶端連線時發生錯誤：{e}")
                break

    def start_server(self):
        """Start TCP print servers for all configured printers."""
        if self.running:
            self.log("[WARN] 伺服器已經在執行中！")
            return False

        printer_configs = self.get_active_printer_configs()
        if not printer_configs:
            self.log("[!] 尚未設定任何可啟動的印表機！")
            return False

        active_ports = [printer["port"] for printer in printer_configs]
        if len(active_ports) != len(set(active_ports)):
            self.log("[!] 啟動失敗：已設定的印表機中有重複的連接埠。")
            return False

        web_port = self.get_web_port()
        if web_port in active_ports:
            self.log(f"[!] 啟動失敗：網頁連接埠 {web_port} 與印表機連接埠重複。")
            return False

        self.ensure_firewall_rules(printer_configs=printer_configs, trigger="啟動伺服器")

        bound_listeners = []
        local_ip = self.get_local_ip()

        try:
            for printer_config in printer_configs:
                port = printer_config["port"]
                printer_name = printer_config["printer_name"]

                self.log(f"[KILL] 正在檢查使用連接埠 {port} 的程序...")
                self.kill_process_on_port(port)

                server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                server_socket.settimeout(1.0)
                server_socket.bind(('0.0.0.0', port))
                server_socket.listen(5)

                bound_listeners.append(
                    {
                        "socket": server_socket,
                        "printer_config": dict(printer_config),
                        "thread": None,
                    }
                )

                self.log(f"[OK] 伺服器已在連接埠 {port} 啟動")
                self.log(f"[PRINTER] 使用印表機：{printer_name}")
                self.log(f"[CONNECT] 其他電腦可連線到：{local_ip}:{port}")

            self.listeners = bound_listeners
            self.server_socket = self.listeners[0]["socket"] if self.listeners else None
            self.active_printers = [dict(item["printer_config"]) for item in self.listeners]
            self.running = True

            for listener in self.listeners:
                thread = threading.Thread(
                    target=self.accept_loop,
                    args=(listener,),
                    daemon=True,
                )
                listener["thread"] = thread
                thread.start()

            self.log(f"[OK] 已同時啟動 {len(self.active_printers)} 組印表機伺服器")

            if self.start_web_server():
                web_url = f"http://{local_ip}" if web_port == 80 else f"http://{local_ip}:{web_port}"
                self.log(f"[WEB] 網頁服務已啟動：{web_url}")
            else:
                self.log(f"[WARN] 網頁服務啟動失敗（連接埠 {web_port}）：{self.web_error}")

            return True

        except Exception as e:
            for listener in bound_listeners:
                try:
                    listener["socket"].close()
                except Exception:
                    pass

            self.listeners = []
            self.server_socket = None
            self.active_printers = []
            self.running = False
            self.log(f"[!] 伺服器錯誤：{e}")
            return False
    
    def stop_server(self):
        """Stop all TCP print servers."""
        was_running = self.running or bool(self.listeners)
        self.running = False

        self.stop_web_server()

        for listener in self.listeners:
            try:
                listener["socket"].close()
            except Exception:
                pass

        self.listeners = []
        self.server_socket = None
        self.active_printers = []

        if was_running:
            self.log("[DONE] 所有伺服器已停止")

class TestClient:
    """Test client for the print server"""
    
    @staticmethod
    def test_connection(host='localhost', port=9100, test_data=None, log_callback=None):
        """Test connection to print server"""
        def log(message):
            print(message)  # Always print to console
            if log_callback:
                log_callback(message)
        
        try:
            client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client_socket.settimeout(5)
            
            log(f"[CONNECT] 正在連線到 {host}:{port}...")
            client_socket.connect((host, port))
            log("[OK] 連線成功！")
            
            # Send test data if provided
            if test_data is not None:
                log(f"[SEND] 正在送出 {len(test_data)} 位元組...")
                client_socket.send(test_data)
                log("[OK] 資料送出成功！")
            else:
                log("[OK] 僅測試連線，未送出資料")
            
            client_socket.close()
            return True
            
        except ConnectionRefusedError:
            log(f"[ERROR] 連線被拒絕。請確認 {host}:{port} 的伺服器是否已啟動。")
            return False
        except socket.timeout:
            log("[ERROR] 連線逾時。")
            return False
        except Exception as e:
            log(f"[ERROR] 發生錯誤：{e}")
            return False

class AutoStartManager:
    """Windows auto-start management"""
    
    @staticmethod  
    def find_manager_exe():
        """Find the GUI executable for auto-start."""
        # Check if running from exe (PyInstaller)
        if hasattr(sys, '_MEIPASS'):
            # Running from exe - use sys.executable which points to exe
            exe_path = os.path.abspath(sys.executable)
            # For exe files, we need to include parameters as part of the command
            return f'"{exe_path}" gui auto_start'
        else:
            # Running from Python script
            current_script = os.path.abspath(__file__)
            python_exe = os.path.abspath(sys.executable)
            return f'"{python_exe}" "{current_script}" gui auto_start'
    
    @staticmethod
    def add_to_startup():
        """Add PrinterOne Manager to Windows startup"""
        try:
            registry_path = AutoStartManager.find_manager_exe()
            
            # Add to Windows startup registry
            key = winreg.OpenKey(
                winreg.HKEY_CURRENT_USER,
                r"Software\Microsoft\Windows\CurrentVersion\Run",
                0,
                winreg.KEY_SET_VALUE
            )
            
            winreg.SetValueEx(key, STARTUP_VALUE_NAME, 0, winreg.REG_SZ, registry_path)
            winreg.CloseKey(key)
            
            return True, f"已將 {APP_NAME} 加入 Windows 開機自動啟動！"
            
        except Exception as e:
            return False, f"加入開機自動啟動時發生錯誤：{e}"
    
    @staticmethod
    def remove_from_startup():
        """Remove PrinterOne Manager from Windows startup"""
        try:
            key = winreg.OpenKey(
                winreg.HKEY_CURRENT_USER,
                r"Software\Microsoft\Windows\CurrentVersion\Run",
                0,
                winreg.KEY_SET_VALUE
            )
            
            removed = False
            for value_name in (STARTUP_VALUE_NAME, LEGACY_STARTUP_VALUE_NAME):
                try:
                    winreg.DeleteValue(key, value_name)
                    removed = True
                except FileNotFoundError:
                    pass
            winreg.CloseKey(key)
            
            if removed:
                return True, f"已將 {APP_NAME} 從 Windows 開機自動啟動移除！"
            return False, "未加入開機自動啟動"
            
        except Exception as e:
            return False, f"移除開機自動啟動時發生錯誤：{e}"
    
    @staticmethod
    def check_startup_status():
        """Check whether the app is in Windows startup."""
        try:
            key = winreg.OpenKey(
                winreg.HKEY_CURRENT_USER,
                r"Software\Microsoft\Windows\CurrentVersion\Run",
                0,
                winreg.KEY_READ
            )

            for value_name in (STARTUP_VALUE_NAME, LEGACY_STARTUP_VALUE_NAME):
                try:
                    value, _ = winreg.QueryValueEx(key, value_name)
                    winreg.CloseKey(key)
                    return True, value
                except FileNotFoundError:
                    continue

            winreg.CloseKey(key)
            return False, "未加入開機自動啟動"
                
        except Exception as e:
            return False, f"檢查開機自動啟動狀態時發生錯誤：{e}"

class PrinterOneGUI:
    """Integrated GUI for PrinterOne"""
    
    def __init__(self, root):
        self.init_logger = None
        try:
            if self.init_logger:
                self.init_logger.info("=== PrinterOneGUI Initialization Started ===")
                self.init_logger.info(f"Tkinter root object: {root}")
            
            self.root = root
            self.root.title(APP_TITLE)
            self.root.geometry("1200x700")
            self.root.resizable(True, True)
            
            if self.init_logger:
                self.init_logger.info("Basic root window configuration completed")
            
            # Initialize server with log callback
            if self.init_logger:
                self.init_logger.info("Initializing PrinterOneServer...")
            
            self.server = PrinterOneServer(log_callback=self.log_message)
            
            if self.init_logger:
                self.init_logger.info("PrinterOneServer initialized successfully")
            
            # GUI variables
            if self.init_logger:
                self.init_logger.info("Setting up GUI variables...")
            
            self.available_printers = self.server.list_printers()
            self.printer_rows = []
            self.printer_rows_frame = None
            self.remove_printer_button = None
            self.test_host_var = tk.StringVar(value="localhost")
            self.web_port_var = tk.IntVar(value=self.server.get_web_port())
            initial_printers = self.server.get_normalized_printers(
                self.server.config.get("printers"),
                self.server.config.get("printer_name", ""),
                self.server.config.get("port", self.server.default_port_for_index(0)),
            )
            self.load_printer_rows(initial_printers)
            self.test_port_var = tk.IntVar(value=initial_printers[0]["port"] if initial_printers else 9100)
            
            # System tray variables
            self.tray_icon = None
            self.minimize_to_tray = self.server.config.get("minimize_to_tray", True)
            self.minimize_to_tray_var = tk.BooleanVar(value=self.minimize_to_tray)
            
            if self.init_logger:
                self.init_logger.info("GUI variables setup completed")
            
            # Setup logging
            if self.init_logger:
                self.init_logger.info("Setting up application logging...")
            
            self.logger = self.setup_logging()
            
            if self.init_logger:
                self.init_logger.info("Application logging setup completed")
            
            # Set window icon
            if self.init_logger:
                self.init_logger.info("Setting window icon...")
            
            self.set_window_icon()
            
            if self.init_logger:
                self.init_logger.info("Window icon setup completed")
            
            # Create GUI
            if self.init_logger:
                self.init_logger.info("Creating GUI widgets...")
            
            self.create_widgets()
            
            if self.init_logger:
                self.init_logger.info("GUI widgets creation completed")

            self.root.after(400, self.check_firewall_on_launch)
            
            # Update status
            if self.init_logger:
                self.init_logger.info("Updating initial status...")
            
            self.update_status()
            
            if self.init_logger:
                self.init_logger.info("Initial status update completed")
            
            # Bind window events
            if self.init_logger:
                self.init_logger.info("Binding window events...")
            
            self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
            
            if self.init_logger:
                self.init_logger.info("Window events binding completed")
            
            # Start status update thread
            if self.init_logger:
                self.init_logger.info("Starting status update thread...")
            
            self.start_status_thread()
            
            if self.init_logger:
                self.init_logger.info("Status update thread started")
            
            # Setup system tray
            if self.init_logger:
                self.init_logger.info(f"Setting up system tray (TRAY_AVAILABLE: {TRAY_AVAILABLE})...")
            
            if TRAY_AVAILABLE:
                self.setup_tray()
                if self.init_logger:
                    self.init_logger.info("System tray setup completed")
            else:
                if self.init_logger:
                    self.init_logger.info("System tray not available, skipping")
            
            # Auto-start server if configured
            if self.init_logger:
                self.init_logger.info(f"Checking auto-start configuration (AUTO_START_MODE: {AUTO_START_MODE})...")
            
            if AUTO_START_MODE:
                if self.init_logger:
                    self.init_logger.info("Auto-start mode detected, hiding window to system tray")
                # Hide window to system tray in auto-start mode
                if TRAY_AVAILABLE:
                    self.root.after(100, self.hide_window)
                if self.init_logger:
                    self.init_logger.info("Scheduling server start in 2 seconds")
                self.root.after(2000, self.auto_start_server)
            else:
                # Auto-start server if printer is configured
                if self.server.get_active_printer_configs():
                    if self.init_logger:
                        self.init_logger.info("Configured printers found, scheduling auto-start in 1 second")
                    self.log_message("已設定印表機，正在自動啟動伺服器...")
                    self.root.after(1000, self.auto_start_server)
                else:
                    if self.init_logger:
                        self.init_logger.info("No printer configured, server will not auto-start")
            
            if self.init_logger:
                self.init_logger.info("=== PrinterOneGUI Initialization Completed Successfully ===")
                
        except Exception as e:
            error_msg = f"Error during GUI initialization: {e}"
            print(error_msg)
            if self.init_logger:
                self.init_logger.critical(error_msg)
                self.init_logger.critical(f"Exception type: {type(e).__name__}")
                import traceback
                self.init_logger.critical(f"Traceback: {traceback.format_exc()}")
            
            # Re-raise to maintain original behavior
            raise
    
    def get_resource_path(self, filename):
        """Get absolute path to resource"""
        try:
            if hasattr(sys, '_MEIPASS'):
                return os.path.join(sys._MEIPASS, filename)
            return os.path.join(os.path.dirname(os.path.abspath(__file__)), filename)
        except Exception:
            return os.path.abspath(filename)
    
    def set_window_icon(self):
        """Set window icon"""
        try:
            icon_png = self.get_resource_path("printer.png") 
            if os.path.exists(icon_png):
                img = Image.open(icon_png)
                img = img.resize((32, 32), Image.Resampling.LANCZOS)
                photo = ImageTk.PhotoImage(img)
                self.root.iconphoto(True, photo)
        except Exception as e:
            print(f"Error setting window icon: {e}")

    def create_printer_row_state(self, printer_name="", port=None):
        """Create Tk variables for one printer row."""
        index = len(self.printer_rows)
        default_port = self.server.default_port_for_index(index)
        return {
            "printer_var": tk.StringVar(value=printer_name),
            "port_var": tk.IntVar(value=port if port is not None else default_port),
        }

    def load_printer_rows(self, printers):
        """Load printer rows from configuration."""
        self.printer_rows = []
        normalized = self.server.get_normalized_printers(printers)
        for index, printer in enumerate(normalized):
            self.printer_rows.append(
                {
                    "printer_var": tk.StringVar(value=printer.get("printer_name", "")),
                    "port_var": tk.IntVar(
                        value=printer.get("port", self.server.default_port_for_index(index))
                    ),
                }
            )

    def render_printer_rows(self):
        """Render printer rows in the configuration area."""
        if not self.printer_rows_frame:
            return

        for child in self.printer_rows_frame.winfo_children():
            child.destroy()

        self.printer_rows_frame.columnconfigure(1, weight=1)

        for index, row in enumerate(self.printer_rows):
            ttk.Label(
                self.printer_rows_frame,
                text=f"印表機{index + 1}：",
            ).grid(row=index, column=0, sticky=tk.W, pady=(0, 8))

            printer_combo = ttk.Combobox(
                self.printer_rows_frame,
                textvariable=row["printer_var"],
                values=self.available_printers,
                width=42,
            )
            printer_combo.grid(row=index, column=1, sticky=tk.EW, padx=(6, 12), pady=(0, 8))

            ttk.Label(
                self.printer_rows_frame,
                text="連接埠：",
            ).grid(row=index, column=2, sticky=tk.W, pady=(0, 8))

            ttk.Entry(
                self.printer_rows_frame,
                textvariable=row["port_var"],
                width=10,
            ).grid(row=index, column=3, sticky=tk.W, pady=(0, 8))

        if self.remove_printer_button:
            self.remove_printer_button.config(
                state="normal" if len(self.printer_rows) > 1 else "disabled"
            )

    def add_printer_row(self):
        """Add one more printer row to the UI."""
        self.printer_rows.append(self.create_printer_row_state())
        self.render_printer_rows()
        self.log_message(f"[INFO] 已新增印表機{len(self.printer_rows)}的設定列")

    def remove_printer_row(self):
        """Remove the last printer row from the UI."""
        if len(self.printer_rows) > 1:
            removed_index = len(self.printer_rows)
            self.printer_rows.pop()
            self.render_printer_rows()
            self.log_message(f"[INFO] 已刪除印表機{removed_index}的設定列")
            return

        self.printer_rows = [self.create_printer_row_state("", self.server.default_port_for_index(0))]
        self.render_printer_rows()
        self.log_message("[INFO] 已清空最後一組印表機設定")

    def collect_printer_rows(self):
        """Collect printer rows from the UI and validate them."""
        collected = []

        try:
            for index, row in enumerate(self.printer_rows):
                printer_name = row["printer_var"].get().strip()
                port = int(row["port_var"].get())

                if port <= 0 or port > 65535:
                    raise ValueError(f"印表機{index + 1} 的連接埠必須介於 1 到 65535。")

                collected.append(
                    {
                        "printer_name": printer_name,
                        "port": port,
                    }
                )
        except (ValueError, tk.TclError) as e:
            self.log_message(f"[ERROR] {e}")
            return None

        active_ports = [item["port"] for item in collected if item["printer_name"]]
        if len(active_ports) != len(set(active_ports)):
            self.log_message("[ERROR] 已設定的印表機中有重複的連接埠，請先修正。")
            return None

        return collected

    def get_persisted_printer_rows(self, printers):
        """Drop blank trailing rows so accidental additions do not persist forever."""
        active_printers = [dict(item) for item in printers if item.get("printer_name", "").strip()]
        if active_printers:
            return active_printers
        if printers:
            return [dict(printers[0])]
        return [{"printer_name": "", "port": self.server.default_port_for_index(0)}]

    def collect_server_configuration(self):
        """Collect and validate printer rows plus web port settings."""
        printers = self.collect_printer_rows()
        if printers is None:
            return None, None

        try:
            web_port = int(self.web_port_var.get())
        except (ValueError, tk.TclError):
            self.log_message("[ERROR] 網頁連接埠必須是有效的數字。")
            return None, None

        if web_port <= 0 or web_port > 65535:
            self.log_message("[ERROR] 網頁連接埠必須介於 1 到 65535。")
            return None, None

        active_ports = [item["port"] for item in printers if item.get("printer_name")]
        if web_port in active_ports:
            self.log_message("[ERROR] 網頁連接埠不可與任何印表機連接埠重複。")
            return None, None

        return printers, web_port
    
    def create_widgets(self):
        """Create GUI widgets"""
        # Main notebook for tabs
        notebook = ttk.Notebook(self.root)
        notebook.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Server Management Tab
        server_frame = ttk.Frame(notebook)
        notebook.add(server_frame, text="伺服器管理")
        self.create_server_tab(server_frame)
        
        # Test Client Tab
        test_frame = ttk.Frame(notebook)
        notebook.add(test_frame, text="測試工具")
        self.create_test_tab(test_frame)
        
        # Settings Tab
        settings_frame = ttk.Frame(notebook)
        notebook.add(settings_frame, text="設定")
        self.create_settings_tab(settings_frame)
    
    def create_server_tab(self, parent):
        """Create server management tab"""
        # Top section - Configuration and Control
        top_frame = ttk.Frame(parent)
        top_frame.pack(fill=tk.X, padx=10, pady=10)
        
        # Configuration frame
        config_frame = ttk.LabelFrame(top_frame, text="設定", padding="10")
        config_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 5))

        ttk.Label(
            config_frame,
            text="可新增多組印表機設定，啟動時會同時監聽所有已填入的連接埠。",
        ).pack(anchor=tk.W, pady=(0, 8))

        self.printer_rows_frame = ttk.Frame(config_frame)
        self.printer_rows_frame.pack(fill=tk.X)
        self.render_printer_rows()

        web_port_frame = ttk.Frame(config_frame)
        web_port_frame.pack(fill=tk.X, pady=(0, 4))

        ttk.Label(web_port_frame, text="網頁連接埠：").pack(side=tk.LEFT)
        ttk.Entry(web_port_frame, textvariable=self.web_port_var, width=10).pack(side=tk.LEFT)
        ttk.Label(
            web_port_frame,
            text="80 代表可直接在瀏覽器輸入伺服器 IP 開啟設定頁",
            foreground="gray",
        ).pack(side=tk.LEFT, padx=(10, 0))

        config_button_frame = ttk.Frame(config_frame)
        config_button_frame.pack(fill=tk.X, pady=(6, 0))

        ttk.Button(
            config_button_frame,
            text="新增一組",
            command=self.add_printer_row,
            width=12,
        ).pack(side=tk.LEFT)

        self.remove_printer_button = ttk.Button(
            config_button_frame,
            text="刪除一組",
            command=self.remove_printer_row,
            width=12,
        )
        self.remove_printer_button.pack(side=tk.LEFT, padx=(8, 0))

        ttk.Button(
            config_button_frame,
            text="儲存設定",
            command=self.save_configuration,
            width=12,
        ).pack(side=tk.LEFT, padx=(8, 0))
        
        # Control frame
        control_frame = ttk.LabelFrame(top_frame, text="伺服器控制", padding="10")
        control_frame.pack(side=tk.RIGHT, fill=tk.BOTH, padx=(5, 0))
        
        # Server status
        self.server_status_label = ttk.Label(control_frame, text="[STOP] 伺服器已停止", 
                                           font=("Arial", 12, "bold"))
        self.server_status_label.pack(pady=10)
        
        self.server_info_label = ttk.Label(
            control_frame,
            text="",
            font=("Arial", 9),
            justify=tk.LEFT,
            wraplength=280,
        )
        self.server_info_label.pack(pady=5)
        
        # Control buttons
        button_frame = ttk.Frame(control_frame)
        button_frame.pack(pady=10)
        
        self.start_button = ttk.Button(button_frame, text="啟動伺服器", 
                                      command=self.start_server, width=15)
        self.start_button.pack(side=tk.LEFT, padx=5)
        
        self.stop_button = ttk.Button(button_frame, text="停止伺服器", 
                                     command=self.stop_server, width=15, state="disabled")
        self.stop_button.pack(side=tk.LEFT, padx=5)
        
        # Auto-start section
        autostart_frame = ttk.LabelFrame(control_frame, text="開機自動啟動", padding="10")
        autostart_frame.pack(fill=tk.X, pady=(10, 0))
        
        self.autostart_status_label = ttk.Label(autostart_frame, text="檢查中...")
        self.autostart_status_label.pack()
        
        autostart_button_frame = ttk.Frame(autostart_frame)
        autostart_button_frame.pack(pady=5)
        
        self.add_autostart_button = ttk.Button(autostart_button_frame, text="加入", 
                                              command=self.add_to_startup, width=12)
        self.add_autostart_button.pack(side=tk.LEFT, padx=2)
        
        self.remove_autostart_button = ttk.Button(autostart_button_frame, text="移除", 
                                                 command=self.remove_from_startup, width=12)
        self.remove_autostart_button.pack(side=tk.LEFT, padx=2)
        
        # Log section
        log_frame = ttk.LabelFrame(parent, text="伺服器紀錄", padding="10")
        log_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        
        # Create text widget with scrollbar
        log_text_frame = ttk.Frame(log_frame)
        log_text_frame.pack(fill=tk.BOTH, expand=True)
        
        self.log_text = tk.Text(log_text_frame, height=15, font=("Consolas", 9))
        log_scrollbar = ttk.Scrollbar(log_text_frame, orient="vertical", command=self.log_text.yview)
        self.log_text.configure(yscrollcommand=log_scrollbar.set)
        
        self.log_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        log_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
    
    def create_test_tab(self, parent):
        """Create test client tab"""
        # Test configuration frame
        test_config_frame = ttk.LabelFrame(parent, text="測試設定", padding="10")
        test_config_frame.pack(fill=tk.X, padx=10, pady=10)
        
        config_grid = ttk.Frame(test_config_frame)
        config_grid.pack(fill=tk.X)
        
        # Host/Server input
        ttk.Label(config_grid, text="主機：").grid(row=0, column=0, sticky=tk.W, pady=5)
        host_entry = ttk.Entry(config_grid, textvariable=self.test_host_var, width=20)
        host_entry.grid(row=0, column=1, sticky=tk.W, padx=(10, 20), pady=5)
        
        # Port input
        ttk.Label(config_grid, text="連接埠：").grid(row=0, column=2, sticky=tk.W, pady=5)
        port_entry = ttk.Entry(config_grid, textvariable=self.test_port_var, width=10)
        port_entry.grid(row=0, column=3, sticky=tk.W, padx=(10, 0), pady=5)
        
        # Test button
        ttk.Button(config_grid, text="測試連線", 
                  command=self.test_connection, width=15).grid(row=0, column=4, padx=(20, 0), pady=5)
        
        # Test data options
        test_data_frame = ttk.LabelFrame(parent, text="測試資料", padding="10")
        test_data_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        
        # Test data button
        button_frame = ttk.Frame(test_data_frame)
        button_frame.pack(fill=tk.X, pady=(0, 10))
        
        ttk.Button(button_frame, text="送出測試資料", 
                  command=lambda: self.send_test_data("test")).pack(side=tk.LEFT, padx=5)
        
        # Test log area
        log_frame = ttk.LabelFrame(test_data_frame, text="測試紀錄", padding="10")
        log_frame.pack(fill=tk.BOTH, expand=True, pady=(10, 0))
        
        # Test log text widget with scrollbar
        test_log_container = ttk.Frame(log_frame)
        test_log_container.pack(fill=tk.BOTH, expand=True)
        
        self.test_log_text = tk.Text(test_log_container, height=8, font=("Consolas", 9), wrap=tk.WORD)
        test_log_scrollbar = ttk.Scrollbar(test_log_container, orient="vertical", command=self.test_log_text.yview)
        self.test_log_text.configure(yscrollcommand=test_log_scrollbar.set)
        
        self.test_log_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        test_log_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Initialize with instruction text
        instruction_text = """測試紀錄區
============

按一下「測試連線」可檢查是否能連上伺服器。
按一下「送出測試資料」可送出一筆測試列印工作。

測試結果會顯示在這裡...
"""
        
        self.test_log_text.insert("1.0", instruction_text)
    
    def create_settings_tab(self, parent):
        """Create settings tab"""
        # Application settings
        app_frame = ttk.LabelFrame(parent, text="應用程式設定", padding="10")
        app_frame.pack(fill=tk.X, padx=10, pady=10)
        
        # Minimize to tray option
        minimize_check = ttk.Checkbutton(app_frame, 
                                       text="關閉視窗時縮小到系統匣",
                                       variable=self.minimize_to_tray_var,
                                       command=self.on_minimize_option_changed)
        minimize_check.pack(anchor=tk.W, pady=5)
        
        if not TRAY_AVAILABLE:
            minimize_check.config(state="disabled")
            ttk.Label(app_frame, text="（系統匣功能不可用，尚未安裝 pystray）",
                     font=("Arial", 8), foreground="gray").pack(anchor=tk.W)
        
        # About section
        about_frame = ttk.LabelFrame(parent, text="關於", padding="10")
        about_frame.pack(fill=tk.X, padx=10, pady=(0, 10))
        
        about_text = """PrtEasyServer - Windows 網路印表機伺服器
版本 1.1.0.0
Copyright (c) 2026 Terence0816
GitHub: https://github.com/Terence0816/Windows-PrtEasyServer

基於 PrinterOne 修改：
https://github.com/xtieume/PrinterOne
Original Copyright (c) 2025 xtieume@gmail.com

這是一個簡易的 TCP/IP 列印伺服器，可將本機印表機轉成網路 IP 印表機。
支援 RAW 9100 列印，不需 Windows 網芳、SMB 分享或帳號密碼。

This project is based on PrinterOne by xtieume.
Original project: https://github.com/xtieume/PrinterOne"""
        
        ttk.Label(about_frame, text=about_text, justify=tk.LEFT, font=("Arial", 9)).pack(anchor=tk.W)
    
    def log_test_message(self, message):
        """Add message to test log"""
        timestamp = time.strftime("%H:%M:%S")
        log_entry = f"[{timestamp}] {message}\n"
        
        # Add to test log
        self.test_log_text.insert(tk.END, log_entry)  
        self.test_log_text.see(tk.END)
        self.test_log_text.update_idletasks()
        
        # Limit test log size
        if int(self.test_log_text.index('end-1c').split('.')[0]) > 100:
            self.test_log_text.delete('1.0', '10.0')
    
    def log_message(self, message):
        """Add message to log"""
        timestamp = time.strftime("%H:%M:%S")
        log_entry = f"[{timestamp}] {message}\n"
        
        # Add to GUI log
        self.log_text.insert(tk.END, log_entry)  
        self.log_text.see(tk.END)
        self.log_text.update_idletasks()
        
        # Also log to file
        if self.logger:
            self.logger.info(message)
        
        # Limit GUI log size
        if int(self.log_text.index('end-1c').split('.')[0]) > 1000:
            self.log_text.delete('1.0', '100.0')

    def check_firewall_on_launch(self):
        """Check firewall rules shortly after the GUI starts."""
        def worker():
            try:
                self.server.ensure_firewall_rules(trigger="程式啟動")
            except Exception as e:
                self.root.after(0, lambda: self.log_message(f"[WARN] 啟動時檢查防火牆失敗：{e}"))

        threading.Thread(target=worker, daemon=True).start()
    
    def save_configuration(self):
        """Save the current configuration"""
        printers, web_port = self.collect_server_configuration()
        if printers is None:
            return

        persisted_printers = self.get_persisted_printer_rows(printers)

        if self.server.save_config(printers=persisted_printers, web_port=web_port):
            self.load_printer_rows(self.server.config.get("printers", persisted_printers))
            self.render_printer_rows()
            self.log_message("[OK] 設定已成功儲存！")
            if persisted_printers:
                self.test_port_var.set(persisted_printers[0]["port"])
        else:
            self.log_message("[ERROR] 設定儲存失敗！")
    
    def start_server(self):
        """Start the print server"""
        if self.server.running:
            self.log_message("[WARN] 伺服器已經在執行中！")
            return

        printers, web_port = self.collect_server_configuration()
        if printers is None:
            return

        if not any(item["printer_name"] for item in printers):
            self.log_message("[WARN] 請至少設定一組印表機後再啟動。")
            return

        persisted_printers = self.get_persisted_printer_rows(printers)

        if self.server.save_config(printers=persisted_printers, web_port=web_port):
            self.load_printer_rows(self.server.config.get("printers", persisted_printers))
            self.render_printer_rows()
            self.log_message("[OK] 設定已儲存")
        else:
            self.log_message("[ERROR] 啟動前儲存設定失敗！")
            return

        self.log_message("[START] 正在啟動所有已設定的伺服器...")
        started = self.server.start_server()
        if not started:
            self.log_message("[ERROR] 伺服器啟動失敗！")
        self.update_server_status()
    
    def stop_server(self):
        """Stop the print server"""
        self.server.stop_server()
        self.update_server_status()
    
    def auto_start_server(self):
        """Auto-start server when launched from startup or when printer is configured"""
        try:
            printers = self.server.get_active_printer_configs()

            if not printers:
                if AUTO_START_MODE:
                    self.log_message("[WARN] 自動啟動模式：尚未設定印表機，程式將在系統匣待命")
                else:
                    self.log_message("[INFO] 尚未設定印表機，未啟動伺服器")
                return
            
            if AUTO_START_MODE:
                self.log_message("[AUTO] 自動啟動模式：正在背景啟動伺服器，請查看系統匣...")
            else:
                self.log_message("[AUTO] 正在使用已設定的印表機自動啟動伺服器...")
            
            # Start server automatically
            self.start_server()
        except Exception as e:
            self.log_message(f"[ERROR] 自動啟動時發生錯誤：{e}")
    
    def update_status(self):
        """Update all status displays"""
        self.update_server_status()
        self.update_autostart_status()
    
    def update_server_status(self):
        """Update server status display"""
        if self.server.running:
            self.server_status_label.config(text="[OK] 伺服器執行中", foreground="green")
            self.start_button.config(state="disabled")
            self.stop_button.config(state="normal")

            ports = ", ".join(str(item["port"]) for item in self.server.active_printers)
            printer_count = len(self.server.active_printers)
            web_port = self.server.get_web_port()
            try:
                local_ip = self.server.get_local_ip()
                web_url = f"http://{local_ip}" if web_port == 80 else f"http://{local_ip}:{web_port}"
                info_lines = [
                    f"已啟動 {printer_count} 組",
                    f"IP：{local_ip}",
                    f"列印連接埠：{ports}",
                ]
                if self.server.web_running:
                    info_lines.append(f"設定網頁：{web_url}")
                elif self.server.web_error:
                    info_lines.append(f"設定網頁：啟動失敗（{self.server.web_error}）")
                info_text = "\n".join(info_lines)
            except:
                info_text = f"已啟動 {printer_count} 組\n連接埠：{ports}"

            self.server_info_label.config(text=info_text)
        else:
            self.server_status_label.config(text="[STOP] 伺服器已停止", foreground="red")
            self.start_button.config(state="normal")
            self.stop_button.config(state="disabled")
            self.server_info_label.config(text="")
    
    def update_autostart_status(self):
        """Update auto-start status"""
        is_in_startup, path_or_error = AutoStartManager.check_startup_status()
        
        if is_in_startup:
            self.autostart_status_label.config(text="[OK] 已啟用開機自動啟動", foreground="green")
            self.add_autostart_button.config(state="disabled")
            self.remove_autostart_button.config(state="normal")
        else:
            self.autostart_status_label.config(text="[STOP] 未啟用開機自動啟動", foreground="red")
            self.add_autostart_button.config(state="normal")
            self.remove_autostart_button.config(state="disabled")
    
    def add_to_startup(self):
        """Add to Windows startup"""
        success, message = AutoStartManager.add_to_startup()
        if success:
            self.log_message(f"[OK] {message}")
        else:
            self.log_message(f"[ERROR] {message}")
        self.update_autostart_status()
    
    def remove_from_startup(self):
        """Remove from Windows startup"""
        success, message = AutoStartManager.remove_from_startup()
        if success:
            self.log_message(f"[OK] {message}")
        else:
            self.log_message(f"[ERROR] {message}")
        self.update_autostart_status()
    
    def test_connection(self):
        """Test connection to server (ping only)"""
        host = self.test_host_var.get()
        port = self.test_port_var.get()
        
        self.log_test_message(f"[CONNECT] 正在測試連線到 {host}:{port}...")
        
        def run_test():
            # Only test connection, don't send any data
            success = TestClient.test_connection(host, port, test_data=None, log_callback=self.log_test_message)
            if success:
                self.root.after(0, lambda: self.log_test_message("[OK] 連線測試完成！"))
            else:
                self.root.after(0, lambda: self.log_test_message("[ERROR] 連線測試失敗！"))
        
        threading.Thread(target=run_test, daemon=True).start()
    
    def send_test_data(self, data_type):
        """Send test data to server"""
        host = self.test_host_var.get()
        port = self.test_port_var.get()
        
        # Prepare test data (default test data)
        test_data = b"""PrtEasyServer Test Data
====================

This is a test print job sent from PrtEasyServer test client.
Date: """ + time.strftime("%Y-%m-%d %H:%M:%S").encode() + b"""

Test content:
- Line 1: Testing printer functionality
- Line 2: Checking data transmission
- Line 3: Verifying print server operation
- Line 4: Testing raw data handling
- Line 5: End of test data

If you can see this printed output, the PrtEasyServer server is working correctly!
"""
        
        # Check if target printer is PDF printer and convert data if needed
        target_printer = self.server.find_printer_config_by_port(port)
        printer_name = target_printer.get("printer_name", "") if target_printer else ""
        use_pdf_conversion = self.server.config.get("use_pdf_conversion", True)
        
        if printer_name == "Microsoft Print to PDF" and use_pdf_conversion:
            self.log_test_message("[PDF] 正在將測試資料轉成 PDF，以供 PDF 印表機使用...")
            try:
                pdf_data = self.server.convert_raw_to_pdf(test_data, save_file=False)
                if pdf_data:
                    test_data = pdf_data
                    self.log_test_message(f"[OK] 測試資料已轉成 PDF（{len(test_data)} 位元組）")
                else:
                    self.log_test_message("[WARN] PDF 轉換失敗，改用原始資料")
            except Exception as e:
                self.log_test_message(f"[WARN] PDF 轉換發生錯誤：{e}")
        
        self.log_test_message(f"[SEND] 正在送出測試資料到 {host}:{port}（{len(test_data)} 位元組）")
        
        def run_test():
            success = TestClient.test_connection(host, port, test_data, log_callback=self.log_test_message)
            if success:
                self.root.after(0, lambda: self.log_test_message("[OK] 測試資料已成功送出！"))
            else:
                self.root.after(0, lambda: self.log_test_message("[ERROR] 測試資料送出失敗！"))
        
        threading.Thread(target=run_test, daemon=True).start()
    
    def start_status_thread(self):
        """Start thread to periodically update status"""
        def status_updater():
            while True:
                try:
                    self.root.after(0, self.update_server_status) 
                    time.sleep(2)
                except:
                    break
        
        threading.Thread(target=status_updater, daemon=True).start()
    
    def on_closing(self):
        """Handle window closing"""
        if TRAY_AVAILABLE and self.tray_icon and self.minimize_to_tray:
            self.hide_window()
        else:
            self.quit_app()
    
    def on_minimize_option_changed(self):
        """Handle minimize to tray option change"""
        self.minimize_to_tray = self.minimize_to_tray_var.get()
        # Save to config
        self.server.config["minimize_to_tray"] = self.minimize_to_tray
        self.server.save_config()
    
    def quit_app(self):
        """Quit the application"""
        try:
            self.log_message("[BYE] 正在關閉程式...")
            
            # Stop server
            if self.server.running:
                self.log_message("[STOP] 正在停止伺服器...")
                self.server.stop_server()
                time.sleep(1)
            
            # Stop tray icon
            if TRAY_AVAILABLE and self.tray_icon:
                try:
                    self.tray_icon.stop()
                    self.log_message("[TRAY] 系統匣圖示已停止")
                except:
                    pass
            
            self.log_message("[OK] 應用程式已關閉")
            self.root.quit()
            self.root.destroy()
            sys.exit(0)
        except Exception as e:
            print(f"Error quitting app: {e}")
            sys.exit(0)
    
    def setup_tray(self):
        """Setup system tray icon"""
        if not TRAY_AVAILABLE:
            return
        
        try:
            # Load icon - try multiple paths
            tray_image = None
            
            # Try bundled resource first
            icon_path = self.get_resource_path("printer.png")
            try:
                tray_image = Image.open(icon_path)
            except Exception:
                pass
            
            # If bundled resource fails, try direct path
            if tray_image is None:
                try:
                    direct_path = "printer.png"
                    tray_image = Image.open(direct_path)
                except Exception:
                    pass
            
            # If all fails, create a default icon
            if tray_image is None:
                tray_image = Image.new('RGB', (64, 64), color='blue')
            
            # Create menu
            menu = pystray.Menu(
                pystray.MenuItem("顯示視窗", self.show_window, default=True),
                pystray.MenuItem("隱藏視窗", self.hide_window),
                pystray.Menu.SEPARATOR,
                pystray.MenuItem("啟動伺服器", self.start_server_tray),
                pystray.MenuItem("停止伺服器", self.stop_server_tray),
                pystray.Menu.SEPARATOR,
                pystray.MenuItem("結束程式", self.quit_app)
            )
            
            self.tray_icon = pystray.Icon(APP_NAME, tray_image, APP_NAME, menu)
            threading.Thread(target=self.tray_icon.run, daemon=True).start()
            
        except Exception as e:
            print(f"Error setting up tray: {e}")
    
    def show_window(self, icon=None, item=None):
        """Show the main window"""
        self.root.deiconify()
        self.root.lift()
        self.root.focus_force()
    
    def hide_window(self, icon=None, item=None):
        """Hide window to tray"""
        self.root.withdraw()
    
    def start_server_tray(self, icon=None, item=None):
        """Start server from tray"""
        self.root.after(0, self.start_server)
    
    def stop_server_tray(self, icon=None, item=None):
        """Stop server from tray"""
        self.root.after(0, self.stop_server)
    
    def setup_logging(self):
        """Disable file logging; the GUI already shows live messages."""
        return None
    
    def cleanup_old_logs(self, logs_dir, days_to_keep=30):
        """No-op because file logging is disabled."""
        return

class PrinterOneServer(PrinterOneServer):
    """Localized server behavior and web/BAT output."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.driver_package_lock = threading.Lock()
        self.driver_package_thread = None

    def ensure_firewall_rules(self, printer_configs=None, trigger=""):
        printer_configs = printer_configs if printer_configs is not None else self.get_active_printer_configs()
        web_port = self.get_web_port()
        display_trigger = trigger or self.tr("firewall_trigger_launch")

        rules = []
        seen_ports = set()
        for printer in printer_configs:
            port = int(printer.get("port", 0))
            printer_name = printer.get("printer_name", "").strip() or f"Printer {port}"
            if port > 0 and port not in seen_ports:
                seen_ports.add(port)
                rules.append(
                    (
                        f"{APP_NAME} Print {port}",
                        port,
                        f"{APP_NAME} printer {printer_name} raw TCP port {port}",
                    )
                )

        if web_port > 0 and web_port not in seen_ports:
            rules.append(
                (
                    f"{APP_NAME} Web {web_port}",
                    web_port,
                    f"{APP_NAME} setup web port {web_port}",
                )
            )

        if not rules:
            self.log(self.tr("firewall_no_rules", trigger=display_trigger))
            return True

        enabled, details = self.get_firewall_status()
        if enabled is False:
            self.log(self.tr("firewall_disabled"))
            return True

        if enabled is None:
            self.log(self.tr("firewall_status_unknown", details=details))

        if not self.is_running_as_admin():
            self.log(self.tr("firewall_admin_required"))
            return False

        if enabled:
            self.log(self.tr("firewall_checking_enabled", trigger=display_trigger, details=details))
        else:
            self.log(self.tr("firewall_creating_disabled", trigger=display_trigger, app_name=APP_NAME))

        all_ok = True
        for display_name, port, description in rules:
            try:
                result = self.ensure_firewall_rule(display_name, port, description)
                status = (result.stdout or "").strip().splitlines()
                status_text = status[-1].strip() if status else ""
                if result.returncode == 0 and status_text in {"CREATED", "UPDATED"}:
                    key = "firewall_rule_created" if status_text == "CREATED" else "firewall_rule_updated"
                    self.log(self.tr(key, name=display_name, port=port))
                elif result.returncode == 0:
                    self.log(self.tr("firewall_rule_exists", name=display_name, port=port))
                else:
                    error_text = (result.stderr or result.stdout or f"returncode={result.returncode}").strip()
                    self.log(self.tr("firewall_rule_failed", name=display_name, port=port, error=error_text))
                    all_ok = False
            except Exception as e:
                self.log(self.tr("firewall_rule_failed", name=display_name, port=port, error=e))
                all_ok = False

        return all_ok

    def sanitize_driver_archive_name(self, driver_name):
        """Return a filesystem-safe driver ZIP name while preserving readability."""
        cleaned = re.sub(r'[<>:"/\\|?*]+', " ", str(driver_name or "").strip())
        cleaned = cleaned.replace("\0", " ")
        cleaned = re.sub(r"\s+", " ", cleaned).strip(" .")
        return cleaned or "Printer Driver"

    def get_driver_archive_filename(self, driver_name):
        """Return the ZIP filename for a packaged printer driver."""
        return f"{self.sanitize_driver_archive_name(driver_name)}.zip"

    def get_driver_archive_path(self, driver_name):
        """Return the absolute ZIP path in the application directory."""
        return os.path.join(get_app_directory(), self.get_driver_archive_filename(driver_name))

    def get_web_printer_entries(self):
        entries = super().get_web_printer_entries()
        for entry in entries:
            archive_name = self.get_driver_archive_filename(entry.get("driver_name", ""))
            archive_path = os.path.join(get_app_directory(), archive_name)
            entry["driver_archive_name"] = archive_name
            entry["driver_archive_path"] = archive_path
            entry["driver_archive_ready"] = os.path.exists(archive_path)
        return entries

    def find_driver_package_info(self, driver_name):
        """Resolve the installed printer driver and its INF path."""
        escaped_name = self.escape_ps_single_quote(driver_name)
        script = "\n".join(
            [
                "$ErrorActionPreference = 'Stop'",
                f"$driverName = '{escaped_name}'",
                "$driver = Get-PrinterDriver -Name $driverName -ErrorAction SilentlyContinue",
                "if (-not $driver) {",
                "    $driver = Get-PrinterDriver -ErrorAction SilentlyContinue |",
                "        Where-Object { $_.Name -eq $driverName -or $_.Name -like ($driverName + '*') -or $driverName -like ($_.Name + '*') } |",
                "        Select-Object -First 1",
                "}",
                "if ($driver) {",
                "    $resolvedName = $driver.Name",
                "    $infPath = $null",
                "    try { $infPath = $driver.InfPath } catch {}",
                "    if (-not $infPath) {",
                "        $cimDriver = Get-CimInstance Win32_PrinterDriver -ErrorAction SilentlyContinue |",
                "            Where-Object { $_.Name -eq $resolvedName } |",
                "            Select-Object -First 1",
                "        if ($cimDriver -and $cimDriver.InfName) {",
                "            $infPath = $cimDriver.InfName",
                "        }",
                "    }",
                "    [pscustomobject]@{ Name = $resolvedName; InfPath = [string]$infPath } | ConvertTo-Json -Compress",
                "}",
            ]
        )

        result = self.run_powershell(script)
        output = (result.stdout or "").strip()
        if result.returncode != 0:
            error_text = (result.stderr or output or f"returncode={result.returncode}").strip()
            return None, error_text
        if not output:
            return None, "Driver information was not found."

        try:
            payload = json.loads(output.splitlines()[-1])
        except json.JSONDecodeError:
            return None, output

        resolved_name = str(payload.get("Name") or driver_name).strip() or driver_name
        inf_path = str(payload.get("InfPath") or "").strip()
        inf_name = os.path.basename(inf_path)
        source_folder = ""
        if inf_path and os.path.isabs(inf_path) and os.path.exists(inf_path):
            source_folder = os.path.dirname(inf_path)
        if not inf_name:
            return None, "The installed printer driver does not expose an INF path."

        return {
            "driver_name": resolved_name,
            "inf_name": inf_name,
            "inf_path": inf_path,
            "source_folder": source_folder,
        }, ""

    def _ensure_driver_archive_for_entry_unlocked(self, entry):
        driver_name = str(entry.get("driver_name", "")).strip()
        archive_name = entry.get("driver_archive_name") or self.get_driver_archive_filename(driver_name)
        archive_path = entry.get("driver_archive_path") or os.path.join(get_app_directory(), archive_name)

        if os.path.exists(archive_path):
            return archive_path, archive_name, "exists"

        driver_info, error_text = self.find_driver_package_info(driver_name)
        if not driver_info:
            return None, archive_name, error_text or "Driver information was not found."

        temp_zip_path = f"{archive_path}.part"
        try:
            source_folder = driver_info.get("source_folder", "")
            exported_root = ""
            if source_folder and os.path.isdir(source_folder):
                exported_root = source_folder
            else:
                with tempfile.TemporaryDirectory(prefix=f"{APP_NAME}_driver_") as temp_root:
                    export_dir = os.path.join(temp_root, "export")
                    os.makedirs(export_dir, exist_ok=True)

                    export_result = subprocess.run(
                        ["pnputil.exe", "/export-driver", driver_info["inf_name"], export_dir],
                        capture_output=True,
                        text=True,
                        errors="ignore",
                        **get_hidden_subprocess_kwargs(),
                    )
                    if export_result.returncode != 0:
                        error_text = (
                            export_result.stderr
                            or export_result.stdout
                            or f"returncode={export_result.returncode}"
                        ).strip()
                        return None, archive_name, error_text

                    exported_root = export_dir
                    exported_files = []
                    for root, _, filenames in os.walk(exported_root):
                        for filename in filenames:
                            full_path = os.path.join(root, filename)
                            relative_path = os.path.relpath(full_path, exported_root)
                            exported_files.append((full_path, relative_path))

                    if not exported_files:
                        return None, archive_name, "No driver files were exported."

                    with zipfile.ZipFile(temp_zip_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
                        for full_path, relative_path in exported_files:
                            archive.write(full_path, relative_path)

                    os.replace(temp_zip_path, archive_path)
                    return archive_path, archive_name, "created"

            exported_files = []
            for root, _, filenames in os.walk(exported_root):
                for filename in filenames:
                    full_path = os.path.join(root, filename)
                    relative_path = os.path.relpath(full_path, exported_root)
                    exported_files.append((full_path, relative_path))

            if not exported_files:
                return None, archive_name, "No driver files were exported."

            with zipfile.ZipFile(temp_zip_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
                for full_path, relative_path in exported_files:
                    archive.write(full_path, relative_path)

            os.replace(temp_zip_path, archive_path)
            return archive_path, archive_name, "created"
        except Exception as e:
            return None, archive_name, str(e)
        finally:
            if os.path.exists(temp_zip_path):
                try:
                    os.remove(temp_zip_path)
                except OSError:
                    pass

    def ensure_driver_archive_for_entry(self, entry):
        with self.driver_package_lock:
            return self._ensure_driver_archive_for_entry_unlocked(entry)

    def ensure_driver_archive_for_index(self, printer_index):
        entry = next((item for item in self.get_web_printer_entries() if item["index"] == printer_index), None)
        if not entry:
            raise KeyError("printer_not_found")

        archive_path, archive_name, status = self.ensure_driver_archive_for_entry(entry)
        if archive_path and os.path.exists(archive_path):
            return archive_path, archive_name

        raise FileNotFoundError(status or "Driver archive is unavailable.")

    def prepare_driver_archives(self, entries=None):
        entries = list(entries or self.get_web_printer_entries())
        if not entries:
            return

        with self.driver_package_lock:
            seen_archives = set()
            for entry in entries:
                archive_name = entry.get("driver_archive_name") or self.get_driver_archive_filename(entry.get("driver_name", ""))
                archive_key = archive_name.lower()
                if archive_key in seen_archives:
                    continue
                seen_archives.add(archive_key)

                archive_path = entry.get("driver_archive_path") or os.path.join(get_app_directory(), archive_name)
                if os.path.exists(archive_path):
                    self.log(self.tr("driver_package_exists", file=archive_name))
                    continue

                self.log(self.tr("driver_package_creating", driver=entry.get("driver_name", ""), file=archive_name))
                built_path, _, status = self._ensure_driver_archive_for_entry_unlocked(entry)
                if built_path and os.path.exists(built_path):
                    self.log(self.tr("driver_package_ready", file=archive_name))
                else:
                    self.log(self.tr("driver_package_failed", driver=entry.get("driver_name", ""), error=status))

    def start_driver_archive_packaging(self):
        entries = self.get_web_printer_entries()
        unique_entries = []
        seen_archives = set()
        for entry in entries:
            archive_name = entry.get("driver_archive_name") or self.get_driver_archive_filename(entry.get("driver_name", ""))
            archive_key = archive_name.lower()
            if archive_key in seen_archives:
                continue
            seen_archives.add(archive_key)
            unique_entries.append(entry)

        if not unique_entries:
            return

        if self.driver_package_thread and self.driver_package_thread.is_alive():
            return

        self.log(self.tr("driver_prepare_background", count=len(unique_entries)))
        self.driver_package_thread = threading.Thread(
            target=self.prepare_driver_archives,
            args=(unique_entries,),
            daemon=True,
            name=f"{APP_NAME}DriverPackager",
        )
        self.driver_package_thread.start()

    def build_installer_batch_content(self, printer_index):
        entries = self.get_web_printer_entries()
        entry = next((item for item in entries if item["index"] == printer_index), None)
        if not entry:
            raise KeyError("printer_not_found")

        printer_name = entry["printer_name"]
        driver_name = entry["driver_name"]
        host_name = entry["host_name"]
        port_name = entry["port_name"]
        port_number = int(entry["port"])
        driver_archive_name = entry["driver_archive_name"]
        driver_info, _ = self.find_driver_package_info(driver_name)
        driver_inf_name = ""
        if driver_info:
            driver_inf_name = str(driver_info.get("inf_name") or "").strip()

        success_title = self.tr("bat_success_title")
        success_message = self.tr("bat_success_message", printer=printer_name).replace("`r`n", "\r\n")
        missing_title = self.tr("bat_missing_title")
        missing_message = self.tr("bat_missing_message", driver=driver_name, port=port_name).replace("`r`n", "\r\n")

        powershell_script = "\n".join(
            [
                "$ErrorActionPreference = 'Stop'",
                "$ProgressPreference = 'SilentlyContinue'",
                "Add-Type -AssemblyName System.Windows.Forms",
                f"$printerName = '{self.escape_ps_single_quote(printer_name)}'",
                f"$driverName = '{self.escape_ps_single_quote(driver_name)}'",
                f"$portName = '{self.escape_ps_single_quote(port_name)}'",
                f"$hostName = '{self.escape_ps_single_quote(host_name)}'",
                f"$portNumber = {port_number}",
                f"$driverArchiveName = '{self.escape_ps_single_quote(driver_archive_name)}'",
                f"$driverInfName = '{self.escape_ps_single_quote(driver_inf_name)}'",
                f"$successTitle = '{self.escape_ps_single_quote(success_title)}'",
                f"$successMessage = '{self.escape_ps_single_quote(success_message)}'",
                f"$missingTitle = '{self.escape_ps_single_quote(missing_title)}'",
                f"$missingMessage = '{self.escape_ps_single_quote(missing_message)}'",
                "$scriptDir = $env:SCRIPT_DIR",
                "if ([string]::IsNullOrWhiteSpace($scriptDir)) {",
                "    $scriptDir = (Get-Location).Path",
                "}",
                "$driverArchivePath = Join-Path -Path $scriptDir -ChildPath $driverArchiveName",
                "$driverTempRoot = Join-Path -Path $env:TEMP -ChildPath ('PrtEasyServer_' + [System.IO.Path]::GetFileNameWithoutExtension($driverArchiveName))",
                "if (-not (Get-PrinterPort -Name $portName -ErrorAction SilentlyContinue)) {",
                "    Add-PrinterPort -Name $portName -PrinterHostAddress $hostName -PortNumber $portNumber",
                "}",
                "function Resolve-Driver($targetName) {",
                "    if ([string]::IsNullOrWhiteSpace($targetName)) {",
                "        return $null",
                "    }",
                "    $matched = Get-PrinterDriver -Name $targetName -ErrorAction SilentlyContinue",
                "    if (-not $matched) {",
                "        $matched = Get-PrinterDriver -ErrorAction SilentlyContinue |",
                "            Where-Object { $_.Name -eq $targetName -or $_.Name -like ($targetName + '*') -or $targetName -like ($_.Name + '*') } |",
                "            Select-Object -First 1",
                "    }",
                "    return $matched",
                "}",
                "function Install-DriverPackage($rootPath, $expectedInfName, $modelName) {",
                "    $candidateInfs = New-Object System.Collections.Generic.List[string]",
                "    if (-not [string]::IsNullOrWhiteSpace($expectedInfName)) {",
                "        Get-ChildItem -LiteralPath $rootPath -Filter $expectedInfName -Recurse -File -ErrorAction SilentlyContinue |",
                "            ForEach-Object { if (-not $candidateInfs.Contains($_.FullName)) { [void]$candidateInfs.Add($_.FullName) } }",
                "    }",
                "    Get-ChildItem -LiteralPath $rootPath -Filter '*.inf' -Recurse -File -ErrorAction SilentlyContinue |",
                "        ForEach-Object { if (-not $candidateInfs.Contains($_.FullName)) { [void]$candidateInfs.Add($_.FullName) } }",
                "    foreach ($infPath in $candidateInfs) {",
                "        try {",
                "            $printUiArgs = @(",
                "                'printui.dll,PrintUIEntry',",
                "                '/ia',",
                "                ('/m \"{0}\"' -f $modelName),",
                "                ('/f \"{0}\"' -f $infPath)",
                "            )",
                "            $installProcess = Start-Process -FilePath 'rundll32.exe' -ArgumentList $printUiArgs -PassThru -Wait -WindowStyle Hidden",
                "            Start-Sleep -Seconds 1",
                "            $resolved = Resolve-Driver $modelName",
                "            if ($resolved) {",
                "                return $resolved",
                "            }",
                "        } catch {",
                "        }",
                "        try {",
                "            [void](Start-Process -FilePath 'pnputil.exe' -ArgumentList @('/add-driver', $infPath, '/install') -PassThru -Wait -WindowStyle Hidden)",
                "            Start-Sleep -Seconds 1",
                "            $resolved = Resolve-Driver $modelName",
                "            if ($resolved) {",
                "                return $resolved",
                "            }",
                "        } catch {",
                "        }",
                "    }",
                "    return $null",
                "}",
                "$driverExists = Resolve-Driver $driverName",
                "if ($driverExists) {",
                "    $driverName = $driverExists.Name",
                "}",
                "if (-not $driverExists -and (Test-Path -LiteralPath $driverArchivePath)) {",
                "    try {",
                "        if (Test-Path -LiteralPath $driverTempRoot) {",
                "            Remove-Item -LiteralPath $driverTempRoot -Recurse -Force",
                "        }",
                "        Expand-Archive -LiteralPath $driverArchivePath -DestinationPath $driverTempRoot -Force",
                "        $driverExists = Install-DriverPackage $driverTempRoot $driverInfName $driverName",
                "        if ($driverExists) {",
                "            $driverName = $driverExists.Name",
                "        }",
                "    } catch {",
                "    } finally {",
                "        if (Test-Path -LiteralPath $driverTempRoot) {",
                "            try { Remove-Item -LiteralPath $driverTempRoot -Recurse -Force } catch {}",
                "        }",
                "    }",
                "}",
                "$openPrintersFolder = {",
                "    try {",
                "        $shell = New-Object -ComObject Shell.Application",
                "        $shell.Open('shell:PrintersFolder')",
                "        Start-Sleep -Milliseconds 900",
                "        $wshell = New-Object -ComObject WScript.Shell",
                "        foreach ($title in @('印表機', '裝置和印表機', 'Devices and Printers', 'Printers')) {",
                "            if ($wshell.AppActivate($title)) {",
                "                Start-Sleep -Milliseconds 150",
                "                $wshell.SendKeys('^{HOME}')",
                "                Start-Sleep -Milliseconds 100",
                "                $wshell.SendKeys('{HOME}')",
                "                break",
                "            }",
                "        }",
                "    } catch {",
                "        Start-Process explorer.exe -ArgumentList 'shell:PrintersFolder'",
                "    }",
                "}",
                "if ($driverExists) {",
                "    if (-not (Get-Printer -Name $printerName -ErrorAction SilentlyContinue)) {",
                "        Add-Printer -Name $printerName -DriverName $driverName -PortName $portName",
                "    }",
                "    [System.Windows.Forms.MessageBox]::Show(",
                "        $successMessage,",
                "        $successTitle,",
                "        [System.Windows.Forms.MessageBoxButtons]::OK,",
                "        [System.Windows.Forms.MessageBoxIcon]::Information",
                "    ) | Out-Null",
                "    & $openPrintersFolder",
                "} else {",
                "    [System.Windows.Forms.MessageBox]::Show(",
                "        $missingMessage,",
                "        $missingTitle,",
                "        [System.Windows.Forms.MessageBoxButtons]::OK,",
                "        [System.Windows.Forms.MessageBoxIcon]::Warning",
                "    ) | Out-Null",
                "    Start-Process rundll32.exe -ArgumentList 'printui.dll,PrintUIEntry /il'",
                "}",
            ]
        )
        encoded_command = base64.b64encode(powershell_script.encode("utf-16le")).decode("ascii")
        encoded_chunks = [encoded_command[i:i + 240] for i in range(0, len(encoded_command), 240)]

        lines = [
            "@echo off",
            "rem PrtEasyServer installer batch - script_dir mode",
            "setlocal",
            "",
            'set "SCRIPT_DIR=%~dp0"',
            'set "B64FILE=%TEMP%\\PrtEasyServer_%RANDOM%_%RANDOM%.b64"',
            '> "%B64FILE%" (',
        ]
        lines.extend([f"echo {chunk}" for chunk in encoded_chunks])
        lines.extend(
            [
                ")",
                "",
                "powershell -NoProfile -ExecutionPolicy Bypass -Command \"$encoded = ((Get-Content -LiteralPath $env:B64FILE -Raw) -replace '\\s',''); $script = [Text.Encoding]::Unicode.GetString([Convert]::FromBase64String($encoded)); & ([scriptblock]::Create($script))\"",
                'set "ERR=%ERRORLEVEL%"',
                'del "%B64FILE%" >nul 2>nul',
                'if not "%ERR%"=="0" exit /b %ERR%',
                "",
                "endlocal",
                "exit /b 0",
                "",
            ]
        )

        safe_name = self.sanitize_port_name_component(printer_name)[:24]
        filename = f"{APP_NAME}_Setup_{entry['index']}_{safe_name}_{port_number}.bat"
        return "\r\n".join(lines), filename

    def render_web_page(self):
        entries = self.get_web_printer_entries()
        host_name = self.get_server_host_name()
        local_ip = self.get_local_ip()
        web_port = self.get_web_port()
        html_lang = "zh-Hant" if self.get_language() == "zh-TW" else "en"
        font_stack = '"Microsoft JhengHei", "Segoe UI", sans-serif' if self.get_language() == "zh-TW" else '"Segoe UI", Arial, sans-serif'

        cards = []
        for entry in entries:
            download_url = f"/download/{entry['index']}.bat"
            driver_url = f"/driver/{entry['index']}.zip"
            cards.append(
                f"""
                <section class="printer-card">
                    <div class="printer-label">{self.tr('web_printer_badge', index=entry['index'])}</div>
                    <h2>{html.escape(entry['printer_name'])}</h2>
                    <p><strong>{html.escape(self.tr('web_raw_port'))}</strong> {entry['port']}</p>
                    <p><strong>{html.escape(self.tr('web_host'))}</strong> {html.escape(entry['host_name'])}</p>
                    <p><strong>{html.escape(self.tr('web_target'))}</strong> {html.escape(entry['host_name'])}</p>
                    <div class="button-row">
                        <a class="download-button" href="{download_url}">{html.escape(self.tr('web_download'))}</a>
                        <a class="download-button secondary" href="{driver_url}">{html.escape(self.tr('web_download_driver'))}</a>
                    </div>
                </section>
                """
            )

        if not cards:
            cards.append(
                f"""
                <section class="empty-state">
                    <h2>{html.escape(self.tr('web_empty_title'))}</h2>
                    <p>{html.escape(self.tr('web_empty_body', app_name=APP_NAME))}</p>
                </section>
                """
            )

        access_hint = local_ip if web_port == 80 else f"{local_ip}:{web_port}"

        return f"""<!DOCTYPE html>
<html lang="{html_lang}">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>{html.escape(self.tr('web_title'))}</title>
    <style>
        :root {{
            color-scheme: light;
            --bg-top: #eff7ea;
            --bg-bottom: #f7fbff;
            --panel: rgba(255, 255, 255, 0.92);
            --line: #d7e1d4;
            --text: #173127;
            --muted: #567263;
            --accent: #2d8559;
            --accent-dark: #1d5d3c;
            --shadow: 0 18px 50px rgba(21, 54, 39, 0.12);
        }}
        * {{ box-sizing: border-box; }}
        body {{
            margin: 0;
            font-family: {font_stack};
            color: var(--text);
            background:
                radial-gradient(circle at top left, rgba(180, 220, 170, 0.45), transparent 28%),
                linear-gradient(160deg, var(--bg-top), var(--bg-bottom));
            min-height: 100vh;
        }}
        .page {{ width: min(1120px, calc(100vw - 32px)); margin: 0 auto; padding: 32px 0 48px; }}
        .hero {{
            background: var(--panel);
            border: 1px solid var(--line);
            border-radius: 24px;
            box-shadow: var(--shadow);
            padding: 28px 30px;
            margin-bottom: 24px;
        }}
        .eyebrow {{
            display: inline-flex;
            padding: 6px 12px;
            border-radius: 999px;
            background: rgba(45, 133, 89, 0.12);
            color: var(--accent-dark);
            font-size: 12px;
            font-weight: 700;
            letter-spacing: 0.08em;
        }}
        h1 {{ margin: 14px 0 12px; font-size: clamp(30px, 5vw, 48px); }}
        .hero p {{ margin: 0 0 10px; color: var(--muted); line-height: 1.6; }}
        .meta {{ display: flex; flex-wrap: wrap; gap: 10px; margin-top: 18px; }}
        .meta-chip {{
            padding: 9px 14px;
            border-radius: 999px;
            background: #ffffff;
            border: 1px solid var(--line);
            color: var(--accent-dark);
            font-size: 14px;
        }}
        .grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 18px;
        }}
        .printer-card, .empty-state {{
            background: var(--panel);
            border: 1px solid var(--line);
            border-radius: 22px;
            box-shadow: var(--shadow);
            padding: 22px;
        }}
        .printer-label {{
            display: inline-flex;
            padding: 6px 12px;
            border-radius: 999px;
            background: rgba(45, 133, 89, 0.12);
            color: var(--accent-dark);
            font-size: 12px;
            font-weight: 700;
            margin-bottom: 12px;
        }}
        .printer-card h2, .empty-state h2 {{ margin: 0 0 10px; font-size: 22px; }}
        .printer-card p, .empty-state p {{ margin: 8px 0; color: var(--muted); line-height: 1.55; }}
        .button-row {{
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
            margin-top: 14px;
        }}
        .download-button {{
            display: inline-flex;
            align-items: center;
            justify-content: center;
            padding: 12px 18px;
            border-radius: 14px;
            background: linear-gradient(135deg, var(--accent), var(--accent-dark));
            color: #ffffff;
            font-weight: 700;
            text-decoration: none;
            flex: 1 1 190px;
        }}
        .download-button.secondary {{
            background: linear-gradient(135deg, #58a86f, #3f8f60);
            color: #ffffff;
        }}
        .download-button:hover {{
            filter: brightness(1.03);
        }}
        .download-button.secondary:hover {{
            filter: brightness(1.08);
        }}
        .footer-note {{
            margin: 20px 4px 0;
            color: var(--muted);
            font-size: 14px;
            line-height: 1.6;
        }}
        .footer-note + .footer-note {{
            margin-top: 6px;
        }}
        .footer-note a {{
            color: var(--accent-dark);
            font-weight: 700;
            text-decoration: underline;
            text-underline-offset: 2px;
        }}
        @media (max-width: 640px) {{
            .page {{ width: min(100vw - 20px, 1120px); padding: 16px 0 32px; }}
            .hero {{ padding: 22px 18px; }}
            .printer-card, .empty-state {{ padding: 18px; }}
        }}
    </style>
</head>
<body>
    <main class="page">
        <header class="hero">
            <span class="eyebrow">{html.escape(self.tr('web_eyebrow'))}</span>
            <h1>{html.escape(self.tr('web_heading'))}</h1>
            <p>{self.tr('web_intro_1', host=html.escape(host_name))}</p>
            <p>{html.escape(self.tr('web_intro_2'))}</p>
            <p>{html.escape(self.tr('web_intro_3'))}</p>
            <div class="meta">
                <div class="meta-chip">{html.escape(self.tr('web_meta_ip', value=host_name))}</div>
                <div class="meta-chip">{html.escape(self.tr('web_meta_entry', value=access_hint))}</div>
                <div class="meta-chip">{html.escape(self.tr('web_meta_count', count=len(entries)))}</div>
            </div>
        </header>
        <section class="grid">
            {''.join(cards)}
        </section>
        <p class="footer-note">{html.escape(self.tr('web_footer_browser'))} {html.escape(self.tr('web_footer_version', version=APP_VERSION))} {html.escape(self.tr('web_footer_latest'))}<a href="{html.escape(APP_GITHUB_URL, quote=True)}" target="_blank" rel="noopener noreferrer">{html.escape(APP_GITHUB_URL)}</a></p>
    </main>
</body>
</html>
"""

    def create_web_request_handler(self):
        owner = self

        class PrinterWebHandler(BaseHTTPRequestHandler):
            server_version = f"{APP_NAME}HTTP/1.0"

            def log_message(self, format, *args):
                return

            def do_GET(self):
                parsed = urlparse(self.path)
                if parsed.path in ("/", "/index.html"):
                    body = owner.render_web_page().encode("utf-8")
                    self.send_response(200)
                    self.send_header("Content-Type", "text/html; charset=utf-8")
                    self.send_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0")
                    self.send_header("Pragma", "no-cache")
                    self.send_header("Expires", "0")
                    self.send_header("Content-Length", str(len(body)))
                    self.end_headers()
                    self.wfile.write(body)
                    return

                if parsed.path.startswith("/download/") and parsed.path.endswith(".bat"):
                    name_part = parsed.path.rsplit("/", 1)[-1]
                    try:
                        printer_index = int(name_part.split(".", 1)[0])
                        batch_content, filename = owner.build_installer_batch_content(printer_index)
                    except (ValueError, KeyError):
                        self.send_error(404, "Printer not found")
                        return

                    body = batch_content.encode("ascii")
                    ascii_name = re.sub(r"[^A-Za-z0-9._-]+", "_", filename)
                    self.send_response(200)
                    self.send_header("Content-Type", "application/octet-stream")
                    self.send_header(
                        "Content-Disposition",
                        f"attachment; filename=\"{ascii_name}\"; filename*=UTF-8''{quote(filename)}",
                    )
                    self.send_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0")
                    self.send_header("Pragma", "no-cache")
                    self.send_header("Expires", "0")
                    self.send_header("Content-Length", str(len(body)))
                    self.end_headers()
                    self.wfile.write(body)
                    owner.log(owner.tr("web_download_logged", index=printer_index))
                    return

                if parsed.path.startswith("/driver/") and parsed.path.endswith(".zip"):
                    name_part = parsed.path.rsplit("/", 1)[-1]
                    index_token = name_part.split(".", 1)[0]
                    try:
                        printer_index = int(index_token)
                        archive_path, filename = owner.ensure_driver_archive_for_index(printer_index)
                    except (ValueError, KeyError, FileNotFoundError) as e:
                        owner.log(owner.tr("web_driver_download_missing", index=index_token, error=e))
                        self.send_error(404, "Driver package not found")
                        return

                    ascii_name = re.sub(r"[^A-Za-z0-9._-]+", "_", filename)
                    file_size = os.path.getsize(archive_path)
                    self.send_response(200)
                    self.send_header("Content-Type", "application/zip")
                    self.send_header(
                        "Content-Disposition",
                        f"attachment; filename=\"{ascii_name}\"; filename*=UTF-8''{quote(filename)}",
                    )
                    self.send_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0")
                    self.send_header("Pragma", "no-cache")
                    self.send_header("Expires", "0")
                    self.send_header("Content-Length", str(file_size))
                    self.end_headers()
                    with open(archive_path, "rb") as file_handle:
                        shutil.copyfileobj(file_handle, self.wfile)
                    owner.log(owner.tr("web_driver_download_logged", index=printer_index, file=filename))
                    return

                self.send_error(404, "Not found")

        return PrinterWebHandler

    def accept_loop(self, listener):
        server_socket = listener["socket"]
        printer_config = listener["printer_config"]

        while self.running:
            try:
                client_socket, address = server_socket.accept()
                client_thread = threading.Thread(
                    target=self.handle_client,
                    args=(client_socket, address, printer_config),
                    daemon=True,
                )
                client_thread.start()
            except socket.timeout:
                continue
            except OSError as e:
                if self.running:
                    self.log(self.tr("server_accept_error", error=e))
                break
            except Exception as e:
                if self.running:
                    self.log(self.tr("server_accept_error", error=e))
                break

    def start_server(self):
        if self.running:
            self.log(self.tr("warn_server_running"))
            return False

        printer_configs = self.get_active_printer_configs()
        if not printer_configs:
            self.log(self.tr("server_no_printer_config"))
            return False

        active_ports = [printer["port"] for printer in printer_configs]
        if len(active_ports) != len(set(active_ports)):
            self.log(self.tr("server_duplicate_ports"))
            return False

        web_port = self.get_web_port()
        if web_port in active_ports:
            self.log(self.tr("server_web_port_conflict", port=web_port))
            return False

        self.ensure_firewall_rules(printer_configs=printer_configs, trigger=self.tr("firewall_trigger_start"))

        bound_listeners = []
        local_ip = self.get_local_ip()

        try:
            for printer_config in printer_configs:
                port = printer_config["port"]
                printer_name = printer_config["printer_name"]

                self.log(self.tr("server_kill_port", port=port))
                self.kill_process_on_port(port)

                server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                server_socket.settimeout(1.0)
                server_socket.bind(("0.0.0.0", port))
                server_socket.listen(5)

                bound_listeners.append(
                    {
                        "socket": server_socket,
                        "printer_config": dict(printer_config),
                        "thread": None,
                    }
                )

                self.log(self.tr("server_port_bound", port=port))
                self.log(self.tr("server_printer_bound", printer=printer_name))
                self.log(self.tr("server_listen_address", ip=local_ip, port=port))

            self.listeners = bound_listeners
            self.server_socket = self.listeners[0]["socket"] if self.listeners else None
            self.active_printers = [dict(item["printer_config"]) for item in self.listeners]
            self.running = True

            for listener in self.listeners:
                thread = threading.Thread(
                    target=self.accept_loop,
                    args=(listener,),
                    daemon=True,
                )
                listener["thread"] = thread
                thread.start()

            self.log(self.tr("server_multi_started", count=len(self.active_printers)))
            self.start_driver_archive_packaging()

            if self.start_web_server():
                web_url = f"http://{local_ip}" if web_port == 80 else f"http://{local_ip}:{web_port}"
                self.log(self.tr("server_web_started", url=web_url))
            else:
                self.log(self.tr("server_web_failed", error=self.web_error))

            return True
        except Exception as e:
            for listener in bound_listeners:
                try:
                    listener["socket"].close()
                except Exception:
                    pass

            self.listeners = []
            self.server_socket = None
            self.active_printers = []
            self.running = False
            self.log(self.tr("server_start_exception", error=e))
            return False

    def stop_server(self):
        was_running = self.running or bool(self.listeners)
        self.running = False

        self.stop_web_server()

        for listener in self.listeners:
            try:
                listener["socket"].close()
            except Exception:
                pass

        self.listeners = []
        self.server_socket = None
        self.active_printers = []

        if was_running:
            self.log(self.tr("server_stopped"))


class TestClient(TestClient):
    """Localized test client output."""

    @staticmethod
    def test_connection(host="localhost", port=9100, test_data=None, log_callback=None, language="en"):
        def tr(key, **kwargs):
            return translate_text(language, key, **kwargs)

        def log(message):
            print(message)
            if log_callback:
                log_callback(message)

        try:
            client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client_socket.settimeout(5)

            log(tr("test_connecting", host=host, port=port))
            client_socket.connect((host, port))
            log(tr("client_socket_connected"))

            if test_data is not None:
                log(tr("client_sending_bytes", size=len(test_data)))
                client_socket.send(test_data)
                log(tr("client_data_sent"))
            else:
                log(tr("client_ping_only"))

            client_socket.close()
            return True
        except ConnectionRefusedError:
            log(tr("client_connection_refused", host=host, port=port))
            return False
        except socket.timeout:
            log(tr("client_timeout"))
            return False
        except Exception as e:
            log(tr("client_unexpected_error", error=e))
            return False


class AutoStartManager(AutoStartManager):
    """Localized Windows auto-start management."""

    @staticmethod
    def add_to_startup(language=None):
        try:
            registry_path = AutoStartManager.find_manager_exe()
            key = winreg.OpenKey(
                winreg.HKEY_CURRENT_USER,
                r"Software\Microsoft\Windows\CurrentVersion\Run",
                0,
                winreg.KEY_SET_VALUE,
            )
            winreg.SetValueEx(key, STARTUP_VALUE_NAME, 0, winreg.REG_SZ, registry_path)
            winreg.CloseKey(key)
            return True, translate_text(language, "autostart_add_success")
        except Exception as e:
            return False, translate_text(language, "autostart_add_failed", error=e)

    @staticmethod
    def remove_from_startup(language=None):
        try:
            key = winreg.OpenKey(
                winreg.HKEY_CURRENT_USER,
                r"Software\Microsoft\Windows\CurrentVersion\Run",
                0,
                winreg.KEY_SET_VALUE,
            )

            removed = False
            for value_name in (STARTUP_VALUE_NAME, LEGACY_STARTUP_VALUE_NAME):
                try:
                    winreg.DeleteValue(key, value_name)
                    removed = True
                except FileNotFoundError:
                    pass
            winreg.CloseKey(key)

            if removed:
                return True, translate_text(language, "autostart_remove_success")
            return False, translate_text(language, "autostart_not_configured")
        except Exception as e:
            return False, translate_text(language, "autostart_remove_failed", error=e)

    @staticmethod
    def check_startup_status(language=None):
        try:
            key = winreg.OpenKey(
                winreg.HKEY_CURRENT_USER,
                r"Software\Microsoft\Windows\CurrentVersion\Run",
                0,
                winreg.KEY_READ,
            )

            for value_name in (STARTUP_VALUE_NAME, LEGACY_STARTUP_VALUE_NAME):
                try:
                    value, _ = winreg.QueryValueEx(key, value_name)
                    winreg.CloseKey(key)
                    return True, value
                except FileNotFoundError:
                    continue

            winreg.CloseKey(key)
            return False, translate_text(language, "autostart_not_configured")
        except Exception as e:
            return False, translate_text(language, "autostart_check_failed", error=e)


class PrinterOneGUI(PrinterOneGUI):
    """Localized GUI with persisted language switching."""

    def __init__(self, root):
        self.root = root
        self.server = None
        self.logger = None
        self.log_text = None
        self.test_log_text = None
        self.notebook = None
        self.tray_icon = None
        self.printer_rows_frame = None
        self.remove_printer_button = None
        self.server_status_label = None
        self.server_info_label = None
        self.start_button = None
        self.stop_button = None
        self.autostart_status_label = None
        self.add_autostart_button = None
        self.remove_autostart_button = None
        self.language_combobox = None
        self.language_display_var = tk.StringVar()
        self.language_code_by_label = {}
        self.pending_log_entries = []
        self.pending_test_log_entries = []

        self.root.geometry("1200x700")
        self.root.resizable(True, True)

        self.server = PrinterOneServer(log_callback=self.log_message)
        self.current_language = self.server.get_language()
        self.root.title(self.tr("app_title"))

        self.available_printers = self.server.list_printers()
        self.printer_rows = []
        self.test_host_var = tk.StringVar(value="localhost")
        self.web_port_var = tk.IntVar(value=self.server.get_web_port())
        initial_printers = self.server.get_normalized_printers(
            self.server.config.get("printers"),
            self.server.config.get("printer_name", ""),
            self.server.config.get("port", self.server.default_port_for_index(0)),
        )
        self.load_printer_rows(initial_printers)
        self.test_port_var = tk.IntVar(value=initial_printers[0]["port"] if initial_printers else 9100)

        self.minimize_to_tray = self.server.config.get("minimize_to_tray", True)
        self.minimize_to_tray_var = tk.BooleanVar(value=self.minimize_to_tray)
        self.language_var = tk.StringVar(value=self.current_language)

        self.logger = self.setup_logging()
        self.set_window_icon()
        self.create_widgets()
        self.flush_queued_logs()
        self.root.after(400, self.check_firewall_on_launch)
        self.update_status()
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.start_status_thread()

        if TRAY_AVAILABLE:
            self.setup_tray()

        if AUTO_START_MODE:
            if TRAY_AVAILABLE:
                self.root.after(100, self.hide_window)
            self.root.after(2000, self.auto_start_server)
        elif self.server.get_active_printer_configs():
            self.root.after(1000, self.auto_start_server)

    def tr(self, key, **kwargs):
        return translate_text(self.current_language, key, **kwargs)

    def open_external_url(self, url):
        try:
            webbrowser.open_new_tab(url)
        except Exception:
            pass

    def populate_about_text_widget(self, widget, text):
        widget.config(state=tk.NORMAL)
        widget.delete("1.0", tk.END)

        url_pattern = re.compile(r"https?://[^\s]+")
        cursor = 0
        link_index = 0

        for match in url_pattern.finditer(text):
            start, end = match.span()
            if start > cursor:
                widget.insert(tk.END, text[cursor:start])

            url = match.group(0)
            tag_name = f"about_link_{link_index}"
            widget.insert(tk.END, url, tag_name)
            widget.tag_configure(tag_name, foreground="#0b63c9", underline=True)
            widget.tag_bind(tag_name, "<Button-1>", lambda _event, link=url: self.open_external_url(link))
            widget.tag_bind(tag_name, "<Enter>", lambda _event: widget.config(cursor="hand2"))
            widget.tag_bind(tag_name, "<Leave>", lambda _event: widget.config(cursor="arrow"))
            cursor = end
            link_index += 1

        if cursor < len(text):
            widget.insert(tk.END, text[cursor:])

        line_count = max(10, text.count("\n") + 1)
        widget.config(height=line_count, state=tk.DISABLED)

    def create_about_text_widget(self, parent):
        background = self.root.tk.call("ttk::style", "lookup", "TFrame", "-background") or self.root.cget("bg") or "SystemButtonFace"
        about_widget = tk.Text(
            parent,
            wrap=tk.WORD,
            font=("Arial", 9),
            borderwidth=0,
            highlightthickness=0,
            relief=tk.FLAT,
            background=background,
            cursor="arrow",
        )
        self.populate_about_text_widget(about_widget, self.tr("about_text"))
        return about_widget

    def render_printer_rows(self):
        if not self.printer_rows_frame:
            return

        for child in self.printer_rows_frame.winfo_children():
            child.destroy()

        self.printer_rows_frame.columnconfigure(1, weight=1)

        for index, row in enumerate(self.printer_rows):
            ttk.Label(self.printer_rows_frame, text=self.tr("printer_label", index=index + 1)).grid(
                row=index, column=0, sticky=tk.W, pady=(0, 8)
            )

            printer_combo = ttk.Combobox(
                self.printer_rows_frame,
                textvariable=row["printer_var"],
                values=self.available_printers,
                width=42,
            )
            printer_combo.grid(row=index, column=1, sticky=tk.EW, padx=(6, 12), pady=(0, 8))

            ttk.Label(self.printer_rows_frame, text=self.tr("port_label")).grid(
                row=index, column=2, sticky=tk.W, pady=(0, 8)
            )
            ttk.Entry(self.printer_rows_frame, textvariable=row["port_var"], width=10).grid(
                row=index, column=3, sticky=tk.W, pady=(0, 8)
            )

        if self.remove_printer_button:
            self.remove_printer_button.config(state="normal" if len(self.printer_rows) > 1 else "disabled")

    def add_printer_row(self):
        self.printer_rows.append(self.create_printer_row_state())
        self.render_printer_rows()
        self.log_message(self.tr("info_printer_row_added", count=len(self.printer_rows)))

    def remove_printer_row(self):
        if len(self.printer_rows) > 1:
            removed_index = len(self.printer_rows)
            self.printer_rows.pop()
            self.render_printer_rows()
            self.log_message(self.tr("info_printer_row_removed", count=removed_index))
            return

        self.printer_rows = [self.create_printer_row_state("", self.server.default_port_for_index(0))]
        self.render_printer_rows()
        self.log_message(self.tr("info_printer_rows_reset"))

    def collect_printer_rows(self):
        collected = []
        try:
            for index, row in enumerate(self.printer_rows):
                printer_name = row["printer_var"].get().strip()
                port = int(row["port_var"].get())

                if port <= 0 or port > 65535:
                    raise ValueError(self.tr("error_port_range", index=index + 1))

                collected.append({"printer_name": printer_name, "port": port})
        except (ValueError, tk.TclError) as e:
            self.log_message(f"[ERROR] {e}")
            return None

        active_ports = [item["port"] for item in collected if item["printer_name"]]
        if len(active_ports) != len(set(active_ports)):
            self.log_message(self.tr("error_duplicate_ports"))
            return None

        return collected

    def collect_server_configuration(self):
        printers = self.collect_printer_rows()
        if printers is None:
            return None, None

        try:
            web_port = int(self.web_port_var.get())
        except (ValueError, tk.TclError):
            self.log_message(self.tr("error_web_port_number"))
            return None, None

        if web_port <= 0 or web_port > 65535:
            self.log_message(self.tr("error_web_port_range"))
            return None, None

        active_ports = [item["port"] for item in printers if item.get("printer_name")]
        if web_port in active_ports:
            self.log_message(self.tr("error_web_port_duplicate"))
            return None, None

        return printers, web_port

    def create_widgets(self):
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        server_frame = ttk.Frame(self.notebook)
        self.notebook.add(server_frame, text=self.tr("tab_server"))
        self.create_server_tab(server_frame)

        test_frame = ttk.Frame(self.notebook)
        self.notebook.add(test_frame, text=self.tr("tab_test"))
        self.create_test_tab(test_frame)

        settings_frame = ttk.Frame(self.notebook)
        self.notebook.add(settings_frame, text=self.tr("tab_settings"))
        self.create_settings_tab(settings_frame)

    def create_server_tab(self, parent):
        top_frame = ttk.Frame(parent)
        top_frame.pack(fill=tk.X, padx=10, pady=10)

        config_frame = ttk.LabelFrame(top_frame, text=self.tr("frame_config"), padding="10")
        config_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 5))
        ttk.Label(config_frame, text=self.tr("config_hint")).pack(anchor=tk.W, pady=(0, 8))

        self.printer_rows_frame = ttk.Frame(config_frame)
        self.printer_rows_frame.pack(fill=tk.X)
        self.render_printer_rows()

        web_port_frame = ttk.Frame(config_frame)
        web_port_frame.pack(fill=tk.X, pady=(0, 4))
        ttk.Label(web_port_frame, text=self.tr("web_port_label")).pack(side=tk.LEFT)
        ttk.Entry(web_port_frame, textvariable=self.web_port_var, width=10).pack(side=tk.LEFT)
        ttk.Label(web_port_frame, text=self.tr("web_port_hint"), foreground="gray").pack(side=tk.LEFT, padx=(10, 0))

        config_button_frame = ttk.Frame(config_frame)
        config_button_frame.pack(fill=tk.X, pady=(6, 0))
        ttk.Button(config_button_frame, text=self.tr("button_add"), command=self.add_printer_row, width=12).pack(side=tk.LEFT)

        self.remove_printer_button = ttk.Button(
            config_button_frame,
            text=self.tr("button_remove"),
            command=self.remove_printer_row,
            width=12,
        )
        self.remove_printer_button.pack(side=tk.LEFT, padx=(8, 0))

        ttk.Button(config_button_frame, text=self.tr("button_save"), command=self.save_configuration, width=12).pack(
            side=tk.LEFT, padx=(8, 0)
        )

        control_frame = ttk.LabelFrame(top_frame, text=self.tr("frame_server_control"), padding="10")
        control_frame.pack(side=tk.RIGHT, fill=tk.BOTH, padx=(5, 0))

        self.server_status_label = ttk.Label(control_frame, text=self.tr("status_stopped"), font=("Arial", 12, "bold"))
        self.server_status_label.pack(pady=10)
        self.server_info_label = ttk.Label(control_frame, text="", font=("Arial", 9), justify=tk.LEFT, wraplength=280)
        self.server_info_label.pack(pady=5)

        button_frame = ttk.Frame(control_frame)
        button_frame.pack(pady=10)
        self.start_button = ttk.Button(button_frame, text=self.tr("button_start"), command=self.start_server, width=15)
        self.start_button.pack(side=tk.LEFT, padx=5)
        self.stop_button = ttk.Button(
            button_frame,
            text=self.tr("button_stop"),
            command=self.stop_server,
            width=15,
            state="disabled",
        )
        self.stop_button.pack(side=tk.LEFT, padx=5)

        autostart_frame = ttk.LabelFrame(control_frame, text=self.tr("frame_autostart"), padding="10")
        autostart_frame.pack(fill=tk.X, pady=(10, 0))
        self.autostart_status_label = ttk.Label(autostart_frame, text=self.tr("status_checking"))
        self.autostart_status_label.pack()

        autostart_button_frame = ttk.Frame(autostart_frame)
        autostart_button_frame.pack(pady=5)
        self.add_autostart_button = ttk.Button(
            autostart_button_frame,
            text=self.tr("button_add_startup"),
            command=self.add_to_startup,
            width=12,
        )
        self.add_autostart_button.pack(side=tk.LEFT, padx=2)
        self.remove_autostart_button = ttk.Button(
            autostart_button_frame,
            text=self.tr("button_remove_startup"),
            command=self.remove_from_startup,
            width=12,
        )
        self.remove_autostart_button.pack(side=tk.LEFT, padx=2)

        log_frame = ttk.LabelFrame(parent, text=self.tr("frame_server_logs"), padding="10")
        log_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        log_text_frame = ttk.Frame(log_frame)
        log_text_frame.pack(fill=tk.BOTH, expand=True)
        self.log_text = tk.Text(log_text_frame, height=15, font=("Consolas", 9))
        log_scrollbar = ttk.Scrollbar(log_text_frame, orient="vertical", command=self.log_text.yview)
        self.log_text.configure(yscrollcommand=log_scrollbar.set)
        self.log_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        log_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

    def create_test_tab(self, parent):
        test_config_frame = ttk.LabelFrame(parent, text=self.tr("frame_test_config"), padding="10")
        test_config_frame.pack(fill=tk.X, padx=10, pady=10)
        config_grid = ttk.Frame(test_config_frame)
        config_grid.pack(fill=tk.X)

        ttk.Label(config_grid, text=self.tr("label_host")).grid(row=0, column=0, sticky=tk.W, pady=5)
        ttk.Entry(config_grid, textvariable=self.test_host_var, width=20).grid(row=0, column=1, sticky=tk.W, padx=(10, 20), pady=5)
        ttk.Label(config_grid, text=self.tr("port_label")).grid(row=0, column=2, sticky=tk.W, pady=5)
        ttk.Entry(config_grid, textvariable=self.test_port_var, width=10).grid(row=0, column=3, sticky=tk.W, padx=(10, 0), pady=5)
        ttk.Button(config_grid, text=self.tr("button_test_connection"), command=self.test_connection, width=15).grid(
            row=0, column=4, padx=(20, 0), pady=5
        )

        test_data_frame = ttk.LabelFrame(parent, text=self.tr("frame_test_data"), padding="10")
        test_data_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        button_frame = ttk.Frame(test_data_frame)
        button_frame.pack(fill=tk.X, pady=(0, 10))
        ttk.Button(button_frame, text=self.tr("button_send_test_data"), command=lambda: self.send_test_data("test")).pack(
            side=tk.LEFT, padx=5
        )

        log_frame = ttk.LabelFrame(test_data_frame, text=self.tr("frame_test_logs"), padding="10")
        log_frame.pack(fill=tk.BOTH, expand=True, pady=(10, 0))
        test_log_container = ttk.Frame(log_frame)
        test_log_container.pack(fill=tk.BOTH, expand=True)
        self.test_log_text = tk.Text(test_log_container, height=8, font=("Consolas", 9), wrap=tk.WORD)
        test_log_scrollbar = ttk.Scrollbar(test_log_container, orient="vertical", command=self.test_log_text.yview)
        self.test_log_text.configure(yscrollcommand=test_log_scrollbar.set)
        self.test_log_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        test_log_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.test_log_text.insert("1.0", self.tr("test_log_intro"))

    def create_settings_tab(self, parent):
        app_frame = ttk.LabelFrame(parent, text=self.tr("frame_app_settings"), padding="10")
        app_frame.pack(fill=tk.X, padx=10, pady=10)

        minimize_check = ttk.Checkbutton(
            app_frame,
            text=self.tr("setting_minimize_to_tray"),
            variable=self.minimize_to_tray_var,
            command=self.on_minimize_option_changed,
        )
        minimize_check.pack(anchor=tk.W, pady=5)
        if not TRAY_AVAILABLE:
            minimize_check.config(state="disabled")
            ttk.Label(app_frame, text=self.tr("tray_unavailable"), font=("Arial", 8), foreground="gray").pack(anchor=tk.W)

        language_frame = ttk.LabelFrame(parent, text=self.tr("frame_language"), padding="10")
        language_frame.pack(fill=tk.X, padx=10, pady=(0, 10))
        language_row = ttk.Frame(language_frame)
        language_row.pack(fill=tk.X)
        ttk.Label(language_row, text=self.tr("language_label")).pack(side=tk.LEFT)
        self.language_combobox = ttk.Combobox(language_row, textvariable=self.language_display_var, state="readonly", width=24)
        self.language_combobox.pack(side=tk.LEFT, padx=(10, 10))
        self.populate_language_combobox()
        ttk.Button(language_row, text=self.tr("button_apply_language"), command=self.apply_language_change, width=14).pack(side=tk.LEFT)

        about_frame = ttk.LabelFrame(parent, text=self.tr("frame_about"), padding="10")
        about_frame.pack(fill=tk.X, padx=10, pady=(0, 10))
        self.create_about_text_widget(about_frame).pack(fill=tk.X, anchor=tk.W)

    def populate_language_combobox(self):
        options = []
        self.language_code_by_label = {}
        for code in SUPPORTED_LANGUAGES:
            label = self.tr(f"language_option_{code}")
            options.append(label)
            self.language_code_by_label[label] = code
        if self.language_combobox:
            self.language_combobox["values"] = options
        self.language_display_var.set(self.tr(f"language_option_{normalize_language(self.language_var.get())}"))

    def rebuild_ui(self):
        existing_log = self.log_text.get("1.0", "end-1c") if self.log_text and self.log_text.winfo_exists() else ""
        existing_test_log = self.test_log_text.get("1.0", "end-1c") if self.test_log_text and self.test_log_text.winfo_exists() else ""

        for child in self.root.winfo_children():
            child.destroy()

        self.log_text = None
        self.test_log_text = None
        self.printer_rows_frame = None
        self.remove_printer_button = None
        self.server_status_label = None
        self.server_info_label = None
        self.start_button = None
        self.stop_button = None
        self.autostart_status_label = None
        self.add_autostart_button = None
        self.remove_autostart_button = None
        self.language_combobox = None

        self.root.title(self.tr("app_title"))
        self.create_widgets()
        if existing_log:
            self.log_text.insert("1.0", existing_log)
            self.log_text.see(tk.END)
        if existing_test_log:
            self.test_log_text.delete("1.0", tk.END)
            self.test_log_text.insert("1.0", existing_test_log)
            self.test_log_text.see(tk.END)
        self.flush_queued_logs()
        self.update_status()
        if TRAY_AVAILABLE:
            self.setup_tray()

    def apply_language_change(self):
        new_language = normalize_language(self.language_code_by_label.get(self.language_display_var.get(), self.current_language))
        if new_language == self.current_language:
            return

        was_running = self.server.running
        old_language = self.current_language
        if not self.server.save_config(language=new_language):
            self.log_message(translate_text(old_language, "language_switch_saving_failed"))
            self.populate_language_combobox()
            return

        if was_running:
            self.log_message(self.tr("language_switch_restarting"))
            self.server.stop_server()

        self.current_language = new_language
        self.language_var.set(new_language)
        self.server.config["language"] = new_language
        self.rebuild_ui()

        if was_running:
            self.start_server()

        self.log_message(self.tr("language_switch_applied"))

    def log_test_message(self, message):
        timestamp = time.strftime("%H:%M:%S")
        log_entry = f"[{timestamp}] {message}\n"
        if self.test_log_text and self.test_log_text.winfo_exists():
            self.test_log_text.insert(tk.END, log_entry)
            self.test_log_text.see(tk.END)
            self.test_log_text.update_idletasks()
            if int(self.test_log_text.index("end-1c").split(".")[0]) > 100:
                self.test_log_text.delete("1.0", "10.0")
        else:
            self.pending_test_log_entries.append(log_entry)

    def log_message(self, message):
        timestamp = time.strftime("%H:%M:%S")
        log_entry = f"[{timestamp}] {message}\n"
        if self.log_text and self.log_text.winfo_exists():
            self.log_text.insert(tk.END, log_entry)
            self.log_text.see(tk.END)
            self.log_text.update_idletasks()
            if int(self.log_text.index("end-1c").split(".")[0]) > 1000:
                self.log_text.delete("1.0", "100.0")
        else:
            self.pending_log_entries.append(log_entry)

        if self.logger:
            self.logger.info(message)

    def flush_queued_logs(self):
        if self.log_text and self.pending_log_entries:
            for entry in self.pending_log_entries:
                self.log_text.insert(tk.END, entry)
            self.log_text.see(tk.END)
            self.pending_log_entries.clear()

        if self.test_log_text and self.pending_test_log_entries:
            for entry in self.pending_test_log_entries:
                self.test_log_text.insert(tk.END, entry)
            self.test_log_text.see(tk.END)
            self.pending_test_log_entries.clear()

    def check_firewall_on_launch(self):
        def worker():
            try:
                self.server.ensure_firewall_rules(trigger=self.tr("firewall_trigger_launch"))
            except Exception as e:
                self.root.after(0, lambda: self.log_message(self.tr("info_firewall_check_failed", error=e)))

        threading.Thread(target=worker, daemon=True).start()

    def save_configuration(self):
        printers, web_port = self.collect_server_configuration()
        if printers is None:
            return

        persisted_printers = self.get_persisted_printer_rows(printers)
        if self.server.save_config(printers=persisted_printers, web_port=web_port, language=self.current_language):
            self.load_printer_rows(self.server.config.get("printers", persisted_printers))
            self.render_printer_rows()
            self.log_message(self.tr("info_settings_saved"))
            if persisted_printers:
                self.test_port_var.set(persisted_printers[0]["port"])
        else:
            self.log_message(self.tr("error_settings_save_failed"))

    def start_server(self):
        if self.server.running:
            self.log_message(self.tr("warn_server_running"))
            return

        printers, web_port = self.collect_server_configuration()
        if printers is None:
            return

        if not any(item["printer_name"] for item in printers):
            self.log_message(self.tr("warn_no_printer_before_start"))
            return

        persisted_printers = self.get_persisted_printer_rows(printers)
        if self.server.save_config(printers=persisted_printers, web_port=web_port, language=self.current_language):
            self.load_printer_rows(self.server.config.get("printers", persisted_printers))
            self.render_printer_rows()
            self.log_message(self.tr("info_settings_saved_before_start"))
        else:
            self.log_message(self.tr("error_settings_save_before_start_failed"))
            return

        self.log_message(self.tr("info_starting_servers"))
        if not self.server.start_server():
            self.log_message(self.tr("error_server_start_failed"))
        self.update_server_status()

    def stop_server(self):
        self.server.stop_server()
        self.update_server_status()

    def auto_start_server(self):
        try:
            printers = self.server.get_active_printer_configs()
            if not printers:
                if AUTO_START_MODE:
                    self.log_message(self.tr("warn_auto_start_no_printer"))
                else:
                    self.log_message(self.tr("info_no_printer_no_autostart"))
                return

            if AUTO_START_MODE:
                self.log_message(self.tr("info_auto_start_background"))
            else:
                self.log_message(self.tr("info_auto_start_configured"))

            self.start_server()
        except Exception as e:
            self.log_message(self.tr("error_auto_start", error=e))

    def update_status(self):
        self.update_server_status()
        self.update_autostart_status()

    def update_server_status(self):
        if not self.server_status_label:
            return

        if self.server.running:
            self.server_status_label.config(text=self.tr("status_running"), foreground="green")
            self.start_button.config(state="disabled")
            self.stop_button.config(state="normal")

            ports = ", ".join(str(item["port"]) for item in self.server.active_printers)
            printer_count = len(self.server.active_printers)
            web_port = self.server.get_web_port()
            try:
                local_ip = self.server.get_local_ip()
                web_url = f"http://{local_ip}" if web_port == 80 else f"http://{local_ip}:{web_port}"
                info_lines = [
                    self.tr("server_info_started", count=printer_count),
                    self.tr("server_info_ip", value=local_ip),
                    self.tr("server_info_print_ports", value=ports),
                ]
                if self.server.web_running:
                    info_lines.append(self.tr("server_info_web", value=web_url))
                elif self.server.web_error:
                    info_lines.append(self.tr("server_info_web_failed", value=self.server.web_error))
                info_text = "\n".join(info_lines)
            except Exception:
                info_text = "\n".join(
                    [
                        self.tr("server_info_started", count=printer_count),
                        self.tr("server_info_print_ports", value=ports),
                    ]
                )

            self.server_info_label.config(text=info_text)
        else:
            self.server_status_label.config(text=self.tr("status_stopped"), foreground="red")
            self.start_button.config(state="normal")
            self.stop_button.config(state="disabled")
            self.server_info_label.config(text="")

    def update_autostart_status(self):
        if not self.autostart_status_label:
            return

        is_in_startup, _ = AutoStartManager.check_startup_status(self.current_language)
        if is_in_startup:
            self.autostart_status_label.config(text=self.tr("startup_enabled"), foreground="green")
            self.add_autostart_button.config(state="disabled")
            self.remove_autostart_button.config(state="normal")
        else:
            self.autostart_status_label.config(text=self.tr("startup_disabled"), foreground="red")
            self.add_autostart_button.config(state="normal")
            self.remove_autostart_button.config(state="disabled")

    def add_to_startup(self):
        success, message = AutoStartManager.add_to_startup(self.current_language)
        self.log_message(f"[OK] {message}" if success else f"[ERROR] {message}")
        self.update_autostart_status()

    def remove_from_startup(self):
        success, message = AutoStartManager.remove_from_startup(self.current_language)
        self.log_message(f"[OK] {message}" if success else f"[ERROR] {message}")
        self.update_autostart_status()

    def test_connection(self):
        host = self.test_host_var.get().strip() or "localhost"
        port = int(self.test_port_var.get())
        self.log_test_message(self.tr("test_connecting", host=host, port=port))

        def run_test():
            success = TestClient.test_connection(
                host,
                port,
                test_data=None,
                log_callback=self.log_test_message,
                language=self.current_language,
            )
            if success:
                self.root.after(0, lambda: self.log_test_message(self.tr("test_connection_done")))
            else:
                self.root.after(0, lambda: self.log_test_message(self.tr("test_connection_failed")))

        threading.Thread(target=run_test, daemon=True).start()

    def send_test_data(self, data_type):
        host = self.test_host_var.get().strip() or "localhost"
        port = int(self.test_port_var.get())

        test_data = b"""PrtEasyServer Test Data
====================

This is a test print job sent from PrtEasyServer test client.
Date: """ + time.strftime("%Y-%m-%d %H:%M:%S").encode() + b"""

Test content:
- Line 1: Testing printer functionality
- Line 2: Checking data transmission
- Line 3: Verifying print server operation
- Line 4: Testing raw data handling
- Line 5: End of test data

If you can see this printed output, the PrtEasyServer server is working correctly!
"""

        target_printer = self.server.find_printer_config_by_port(port)
        printer_name = target_printer.get("printer_name", "") if target_printer else ""
        use_pdf_conversion = self.server.config.get("use_pdf_conversion", True)

        if printer_name == "Microsoft Print to PDF" and use_pdf_conversion:
            self.log_test_message(self.tr("test_pdf_convert"))
            try:
                pdf_data = self.server.convert_raw_to_pdf(test_data, save_file=False)
                if pdf_data:
                    test_data = pdf_data
                    self.log_test_message(self.tr("test_pdf_convert_ok", size=len(test_data)))
                else:
                    self.log_test_message(self.tr("test_pdf_convert_failed"))
            except Exception as e:
                self.log_test_message(self.tr("test_pdf_convert_error", error=e))

        self.log_test_message(self.tr("test_sending_data", host=host, port=port, size=len(test_data)))

        def run_test():
            success = TestClient.test_connection(
                host,
                port,
                test_data=test_data,
                log_callback=self.log_test_message,
                language=self.current_language,
            )
            if success:
                self.root.after(0, lambda: self.log_test_message(self.tr("test_send_ok")))
            else:
                self.root.after(0, lambda: self.log_test_message(self.tr("test_send_failed")))

        threading.Thread(target=run_test, daemon=True).start()

    def on_minimize_option_changed(self):
        self.minimize_to_tray = self.minimize_to_tray_var.get()
        self.server.config["minimize_to_tray"] = self.minimize_to_tray
        self.server.save_config(language=self.current_language)

    def quit_app(self):
        try:
            self.log_message(self.tr("quit_app_exit"))
            if self.server.running:
                self.log_message(self.tr("quit_app_stopping_server"))
                self.server.stop_server()
                time.sleep(1)

            if TRAY_AVAILABLE and self.tray_icon:
                try:
                    self.tray_icon.stop()
                    self.log_message(self.tr("quit_app_tray_stopped"))
                except Exception:
                    pass

            self.log_message(self.tr("quit_app_closed"))
            self.root.quit()
            self.root.destroy()
            sys.exit(0)
        except Exception:
            sys.exit(0)

    def setup_tray(self):
        if not TRAY_AVAILABLE:
            return

        try:
            if self.tray_icon:
                try:
                    self.tray_icon.stop()
                except Exception:
                    pass

            tray_image = None
            icon_path = self.get_resource_path("printer.png")
            try:
                tray_image = Image.open(icon_path)
            except Exception:
                pass

            if tray_image is None:
                try:
                    tray_image = Image.open("printer.png")
                except Exception:
                    pass

            if tray_image is None:
                tray_image = Image.new("RGB", (64, 64), color="blue")

            menu = pystray.Menu(
                pystray.MenuItem(self.tr("tray_show_window"), self.show_window, default=True),
                pystray.MenuItem(self.tr("tray_hide_window"), self.hide_window),
                pystray.Menu.SEPARATOR,
                pystray.MenuItem(self.tr("button_start"), self.start_server_tray),
                pystray.MenuItem(self.tr("button_stop"), self.stop_server_tray),
                pystray.Menu.SEPARATOR,
                pystray.MenuItem(self.tr("tray_quit"), self.quit_app),
            )

            self.tray_icon = pystray.Icon(APP_NAME, tray_image, APP_NAME, menu)
            threading.Thread(target=self.tray_icon.run, daemon=True).start()
        except Exception as e:
            print(f"Error setting up tray: {e}")

    def setup_logging(self):
        return None

    def cleanup_old_logs(self, logs_dir, days_to_keep=30):
        return


def signal_handler(signum, frame):
    """Handle Ctrl+C signal"""
    global SERVER_RUNNING
    print(f"\n[STOP] Received signal {signum}, stopping...")
    SERVER_RUNNING = False
    sys.exit(0)

def run_console_mode():
    """Run in console mode (command line interface)"""
    # Set up signal handler
    signal.signal(signal.SIGINT, signal_handler)
    
    print(APP_TITLE)
    print("=" * 35)
    print()
    
    server = PrinterOneServer()
    server.ensure_firewall_rules(trigger="程式啟動")
    
    # Check if configuration exists
    config = server.config
    if not server.get_active_printer_configs():
        print("尚未設定印表機，請先完成設定：")
        printers = server.list_printers()
        
        if not printers:
            print("[!] 找不到任何印表機！")
            return
        
        print(f"找到 {len(printers)} 台印表機：")
        for i, printer in enumerate(printers, 1):
            print(f"  {i}. {printer}")
        
        while True:
            try:
                selection = input(f"\n請選擇印表機（1-{len(printers)}）：")
                printer_index = int(selection) - 1
                if 0 <= printer_index < len(printers):
                    selected_printer = printers[printer_index]
                    break
                else:
                    print("[!] 選擇無效。")
            except ValueError:
                print("[!] 請輸入有效的數字。")
        
        port_input = input("請輸入連接埠（預設：9100）：")
        port = int(port_input) if port_input.strip() else 9100
        
        server.save_config(printer_name=selected_printer, port=port)
        print(f"[OK] 設定已儲存：{selected_printer}，連接埠 {port}")
    
    # Start server
    active_printers = server.get_active_printer_configs()
    print("正在啟動以下印表機設定：")
    for index, printer in enumerate(active_printers, 1):
        print(f"  {index}. {printer['printer_name']} -> {printer['port']}")
    print("按 Ctrl+C 可停止")
    print()
    
    try:
        if server.start_server():
            while server.running:
                time.sleep(1)
    except KeyboardInterrupt:
        print("\n[STOP] 使用者已停止伺服器")
        server.stop_server()
    except Exception as e:
        print(f"[!] 伺服器錯誤：{e}")

def run_gui_mode():
    """Run in GUI mode"""
    global AUTO_START_MODE
    
    gui_logger = None
    try:
        if gui_logger:
            gui_logger.info("=== GUI Mode Starting ===")
            gui_logger.info(f"Process ID: {os.getpid()}")
            gui_logger.info(f"Arguments: {sys.argv}")
        
        # Check if this is an auto-start instance (works for both script and exe)
        AUTO_START_MODE = 'auto_start' in sys.argv
        
        if gui_logger:
            gui_logger.info(f"Auto-start mode: {AUTO_START_MODE}")
        
        # Kill existing GUI instances
        killed_count = 0
        try:
            if gui_logger:
                gui_logger.info("Checking for existing GUI instances...")
            
            current_pid = os.getpid()
            for proc in psutil.process_iter(['pid', 'name', 'cmdline']):
                try:
                    if proc.info['pid'] == current_pid:
                        continue
                    
                    process_name = proc.info['name'].lower()
                    cmdline = ' '.join(proc.info['cmdline']) if proc.info['cmdline'] else ''
                    
                    # Kill other GUI instances
                    if ((process_name == 'python.exe' and 'server.py' in cmdline and 'gui' in cmdline) or
                        process_name in {f'{APP_NAME.lower()}.exe', 'printerone.exe'}):
                        if gui_logger:
                            gui_logger.info(f"Killing existing instance: {process_name} (PID: {proc.info['pid']})")
                        proc.terminate()
                        try:
                            proc.wait(timeout=3)
                        except psutil.TimeoutExpired:
                            proc.kill()
                        killed_count += 1
                except (psutil.NoSuchProcess, psutil.AccessDenied):
                    pass
        except Exception as e:
            error_msg = f"Error killing existing instances: {e}"
            print(error_msg)
            if gui_logger:
                gui_logger.error(error_msg)
        
        if killed_count > 0:
            info_msg = f"Killed {killed_count} existing GUI instance(s)"
            print(info_msg)
            if gui_logger:
                gui_logger.info(info_msg)
            time.sleep(1)
        elif gui_logger:
            gui_logger.info("No existing GUI instances found")
        
        # Create and run GUI
        if gui_logger:
            gui_logger.info("Creating Tkinter root window...")
        
        root = tk.Tk()
        
        if gui_logger:
            gui_logger.info("Tkinter root window created successfully")
            gui_logger.info("Initializing PrinterOneGUI...")
        
        try:
            app = PrinterOneGUI(root)
            if gui_logger:
                gui_logger.info("PrinterOneGUI initialized successfully")
                gui_logger.info("Starting Tkinter mainloop...")
            
            root.mainloop()
            
            if gui_logger:
                gui_logger.info("Tkinter mainloop completed normally")
                
        except Exception as e:
            error_msg = f"GUI error: {e}"
            print(error_msg)
            if gui_logger:
                gui_logger.critical(error_msg)
                gui_logger.critical(f"Exception type: {type(e).__name__}")
                import traceback
                gui_logger.critical(f"Traceback: {traceback.format_exc()}")
            
            import traceback
            traceback.print_exc()
            
            # Re-raise for proper error handling
            raise
            
    except Exception as e:
        error_msg = f"Critical GUI startup error: {e}"
        print(error_msg)
        if gui_logger:
            gui_logger.critical(error_msg)
            gui_logger.critical(f"Exception type: {type(e).__name__}")
            import traceback
            gui_logger.critical(f"Traceback: {traceback.format_exc()}")
        
        # Re-raise the exception to maintain original behavior
        raise

def run_test_mode():
    """Run test client"""
    print(f"{APP_NAME} - 測試工具")
    print("=" * 24)
    print()
    
    # Get server details
    host = input("伺服器主機（預設：localhost）：").strip() or "localhost"
    port_input = input("伺服器連接埠（預設：9100）：").strip()
    port = int(port_input) if port_input else 9100
    
    # Test connection
    TestClient.test_connection(host, port)

def show_help():
    """Show help information"""
    print(APP_TITLE)
    print("=" * 35)
    print()
    print("使用方式：")
    print("  python server.py                - 以命令列模式執行")
    print("  python server.py gui            - 以圖形介面執行")
    print("  python server.py gui auto_start - 以自動啟動模式執行圖形介面")
    print("  python server.py test           - 執行測試工具")
    print("  python server.py --help         - 顯示這份說明")
    print()
    print("功能特色：")
    print("  • 支援原始列印資料的 TCP 列印伺服器")
    print("  • 可為 PDF 印表機自動轉換 PDF")
    print("  • 內建圖形化管理介面")
    print("  • 內建測試工具")
    print("  • 支援 Windows 開機自動啟動")
    print("  • 支援系統匣操作")
    print()
    print(APP_COPYRIGHT)
    print(f"GitHub: {APP_GITHUB_URL}")

def setup_startup_logging():
    """Disable file logging; runtime status stays in the app only."""
    return None

def main():
    """Main function"""
    # Setup startup logging first
    startup_logger = setup_startup_logging()
    
    try:
        if startup_logger:
            startup_logger.info("=== PrinterOne Application Started ===")
            startup_logger.info(f"Python version: {sys.version}")
            startup_logger.info(f"Command line arguments: {sys.argv}")
            startup_logger.info(f"Working directory: {os.getcwd()}")
            startup_logger.info(f"Executable path: {sys.executable}")
            
            # Log if running from exe
            if hasattr(sys, '_MEIPASS'):
                startup_logger.info(f"Running from PyInstaller exe: {sys.executable}")
                startup_logger.info(f"Bundle dir: {sys._MEIPASS}")
            else:
                startup_logger.info("Running from Python script")
        
        if len(sys.argv) > 1:
            command = sys.argv[1].lower()
            
            if startup_logger:
                startup_logger.info(f"Command mode: {command}")
            
            if command in ['--help', '-h', 'help']:
                if startup_logger:
                    startup_logger.info("Showing help")
                show_help()
            elif command == 'gui':
                if startup_logger:
                    startup_logger.info("Starting GUI mode")
                run_gui_mode()
            elif command == 'test':
                if startup_logger:
                    startup_logger.info("Starting test mode")
                run_test_mode()
            else:
                if startup_logger:
                    startup_logger.error(f"Unknown command: {command}")
                print(f"Unknown command: {command}")
                show_help()
        else:
            if startup_logger:
                startup_logger.info("Default mode - attempting GUI")
            # Default to GUI mode if available, otherwise console
            try:
                run_gui_mode()
            except ImportError as e:
                if startup_logger:
                    startup_logger.error(f"GUI dependencies not available: {e}")
                    startup_logger.info("Falling back to console mode")
                print(f"GUI dependencies not available: {e}")
                print("正在切換為命令列模式...")
                run_console_mode()
        
        if startup_logger:
            startup_logger.info("Application completed successfully")
            
    except Exception as e:
        error_msg = f"Critical startup error: {e}"
        print(error_msg)
        if startup_logger:
            startup_logger.critical(error_msg)
            startup_logger.critical(f"Exception type: {type(e).__name__}")
            import traceback
            startup_logger.critical(f"Traceback: {traceback.format_exc()}")
        
        # Re-raise the exception to maintain original behavior
        raise

if __name__ == "__main__":
    try:
        if startup_logger:
            startup_logger.info("=== Main execution started ===")
            startup_logger.info(f"OS called application: {sys.executable}")
            startup_logger.info(f"Arguments passed: {sys.argv}")
            startup_logger.info(f"Environment: {dict(os.environ)}")
        
        main()
        
        if startup_logger:
            startup_logger.info("=== Main execution completed successfully ===")
            
    except Exception as e:
        error_msg = f"CRITICAL APPLICATION FAILURE: {e}"
        print(error_msg)
        
        if startup_logger:
            startup_logger.critical("=== CRITICAL APPLICATION FAILURE ===")
            startup_logger.critical(error_msg)
            startup_logger.critical(f"Exception type: {type(e).__name__}")
            startup_logger.critical(f"Full traceback: {traceback.format_exc()}")
            startup_logger.critical("=== END OF CRITICAL FAILURE LOG ===")
        else:
            print(f"Exception type: {type(e).__name__}")
            print(f"Full traceback: {traceback.format_exc()}")
        
        # Keep console open for debugging
        try:
            input("按 Enter 結束...")
        except:
            pass
            
        sys.exit(1)
