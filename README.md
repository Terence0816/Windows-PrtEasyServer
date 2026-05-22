# PrtEasyServer

Windows Network Print Server for local printers.

[![Release](https://img.shields.io/github/v/release/Terence0816/Windows-PrtEasyServer?label=Release&color=2d7d46)](https://github.com/Terence0816/Windows-PrtEasyServer/releases)
[![Downloads](https://img.shields.io/github/downloads/Terence0816/Windows-PrtEasyServer/total?label=Downloads&color=1f6feb)](https://github.com/Terence0816/Windows-PrtEasyServer/releases)
[![v1.1.0.0 Downloads](https://img.shields.io/github/downloads/Terence0816/Windows-PrtEasyServer/v1.1.0.0/total?label=v1.1.0.0%20Downloads&color=cf222e)](https://github.com/Terence0816/Windows-PrtEasyServer/releases/tag/v1.1.0.0)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE)

English | [繁體中文](#zh-tw)

PrtEasyServer turns a Windows local printer into a TCP/IP network printer server. It focuses on simple RAW 9100 sharing, avoids SMB printer sharing, and lets client PCs install printers through IP port setup without Windows file sharing credentials.

## What's New in v1.1.0.0

- The built-in web page can now download both the setup BAT and the matching driver package.
- If the BAT and driver ZIP stay in the same folder, the client PC can try automatic driver installation before falling back to manual setup.
- Updated Chinese and English web screenshots.
- Refined build metadata and Windows version information for the packaged EXE.

## Highlights

- Share one or more local printers at the same time
- RAW 9100 print server for standard TCP/IP printer port setup
- No SMB printer sharing, no network neighborhood password prompt
- Built-in web page for printer list, setup BAT download, and driver package download
- Chinese and English UI, web page, and installer messages
- Hostname-first printer connection flow for changing LAN IP environments
- Auto firewall rule check when app starts or services start
- Minimize to tray and startup support

## English Screenshots

### Desktop UI

![English Desktop UI](./assets/screenshots/ui-en.png)

### Web Page

![English Web Page](./assets/screenshots/web-en.png)

## Video Demo

Click the preview below to watch the YouTube demo:

[![Watch the PrtEasyServer demo on YouTube](https://img.youtube.com/vi/uwNWGIuaMrA/hqdefault.jpg)](https://youtu.be/uwNWGIuaMrA)

- YouTube: [https://youtu.be/uwNWGIuaMrA](https://youtu.be/uwNWGIuaMrA)

## Download

- Latest releases: [Releases](https://github.com/Terence0816/Windows-PrtEasyServer/releases)
- Official `v1.1.0.0` download: [PrtEasyServer.exe](https://github.com/Terence0816/Windows-PrtEasyServer/releases/download/v1.1.0.0/PrtEasyServer.exe)
- The release page also shows the current download count for the official build

## Typical Usage

1. Connect your printer to the Windows PC.
2. Select one or more printers in PrtEasyServer and assign ports such as `9100`, `9200`, or `9300`.
3. Start the server.
4. Open the built-in web page from another PC and download the setup BAT for the target printer.
5. Download the matching driver ZIP from the same web page and keep it in the same folder as the BAT.
6. Run the BAT on the client PC to create the TCP/IP port and try automatic driver installation.

## Search Keywords

Windows print server, network printer server, RAW 9100, TCP/IP printer sharing, USB printer sharing, printer host, no SMB printer sharing, IP printer setup, Windows local printer to network printer, printer driver package download, automatic printer installer

## Credits

This project is based on PrinterOne by xtieume.  
Original project: https://github.com/xtieume/PrinterOne

Original attribution:

- Original Copyright (c) 2025 xtieume@gmail.com
- This repository contains modifications, UI changes, multilingual support, web-based setup download, driver package download, automatic driver install flow, multi-printer support, and packaging updates by Terence0816

## License

This repository is released under the MIT License. See [LICENSE](./LICENSE).

---

<a id="zh-tw"></a>

# 繁體中文

PrtEasyServer 是一個 Windows 網路印表機伺服器，可將本機印表機轉成 TCP/IP 網路印表機。它主打簡單的 RAW 9100 分享方式，不需要 Windows 網芳、SMB 印表機分享或帳號密碼，讓用戶端可直接透過 IP 連接埠完成安裝。

## v1.1.0.0 更新重點

- 內建網頁現在可直接下載設定 BAT 與對應驅動程式 ZIP。
- 若 BAT 與驅動 ZIP 放在同一個資料夾，用戶端會先嘗試自動安裝驅動，再視情況回退到手動安裝。
- 已更新中文與英文的網頁畫面截圖。
- 已補強打包後 EXE 的版本資訊與 Windows 檔案內容欄位。

## 特色

- 可同時分享一台或多台本機印表機
- 使用 RAW 9100 提供標準 TCP/IP 印表機連接埠
- 不需 SMB 印表機分享，不會跳出網芳帳密問題
- 內建網頁可列出印表機、下載設定 BAT、下載驅動程式
- 支援繁體中文與英文介面、網頁與安裝訊息
- 以主機名稱優先連線，適合內網 IP 可能變動的環境
- 程式啟動與服務啟動時可自動檢查防火牆規則
- 支援最小化到系統匣與開機自動啟動

## 中文畫面

### 桌面介面

![繁體中文桌面介面](./assets/screenshots/ui-zh-tw.png)

### 網頁介面

![繁體中文網頁介面](./assets/screenshots/web-zh-tw.png)

## 影片示範

點擊下方縮圖可直接前往 YouTube 觀看示範：

[![觀看 PrtEasyServer YouTube 示範](https://img.youtube.com/vi/uwNWGIuaMrA/hqdefault.jpg)](https://youtu.be/uwNWGIuaMrA)

- YouTube 連結：[https://youtu.be/uwNWGIuaMrA](https://youtu.be/uwNWGIuaMrA)

## 下載

- 最新版本下載頁：[Releases](https://github.com/Terence0816/Windows-PrtEasyServer/releases)
- 正式版 `v1.1.0.0`：[PrtEasyServer.exe](https://github.com/Terence0816/Windows-PrtEasyServer/releases/download/v1.1.0.0/PrtEasyServer.exe)
- GitHub 發行頁右側也會顯示正式版下載次數

## 使用方式

1. 將印表機接到 Windows 主機。
2. 在 PrtEasyServer 選擇一台或多台印表機，設定如 `9100`、`9200`、`9300` 這類連接埠。
3. 啟動伺服器。
4. 在其他電腦開啟內建網頁，下載目標印表機的設定 BAT。
5. 再從同一頁下載對應的驅動程式 ZIP，並與 BAT 放在同一個資料夾。
6. 在用戶端執行 BAT，自動建立 TCP/IP 連接埠並嘗試安裝驅動與印表機。

## 搜尋關鍵字

Windows 網路印表機伺服器、RAW 9100、TCP/IP 印表機分享、USB 印表機分享、IP 印表機安裝、免 SMB 印表機分享、印表機驅動下載、自動安裝印表機

## 原作與致謝

本專案基於 PrinterOne 修改而成。  
原始專案：https://github.com/xtieume/PrinterOne

原始出處：

- Original Copyright (c) 2025 xtieume@gmail.com
- 本專案由 Terence0816 進行功能延伸與介面改良，包含多語系、網頁安裝流程、驅動程式下載、自動安裝流程、多印表機支援與打包資訊調整

## 授權

本專案採用 MIT License。詳見 [LICENSE](./LICENSE)。
