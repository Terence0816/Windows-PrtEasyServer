#include "localization.h"

namespace {

using StringMap = std::map<std::wstring, std::wstring>;

StringMap BuildBaseEnglishStrings() {
    return {
        {L"app_title", L"PrtEasyServer - Windows Network Print Server"},
        {L"tab_server", L"Server"},
        {L"tab_test", L"Test Tools"},
        {L"tab_settings", L"Settings"},
        {L"frame_config", L"Printer Configuration"},
        {L"config_hint", L"You can add multiple printer entries. Each entry listens on its own RAW port."},
        {L"label_installed_printer", L"Installed printer"},
        {L"label_raw_port", L"RAW port"},
        {L"label_web_port", L"Web port"},
        {L"web_port_hint", L"When set to 80, other PCs can open the setup page directly by IP."},
        {L"button_add", L"Add"},
        {L"button_add_group", L"Add a row"},
        {L"button_update", L"Update"},
        {L"button_update_group", L"Update row"},
        {L"button_remove", L"Remove"},
        {L"button_remove_group", L"Remove row"},
        {L"button_refresh_printers", L"Refresh printers"},
        {L"button_save", L"Save settings"},
        {L"button_start", L"Start server"},
        {L"button_stop", L"Stop server"},
        {L"button_open_web", L"Open web page"},
        {L"button_add_startup", L"Add"},
        {L"button_remove_startup", L"Remove"},
        {L"column_printer", L"Printer"},
        {L"column_port", L"Port"},
        {L"frame_server_control", L"Server Control"},
        {L"frame_autostart", L"Windows Startup"},
        {L"frame_server_logs", L"Server Logs"},
        {L"status_running", L"[OK] Server is running"},
        {L"status_stopped", L"[STOP] Server is stopped"},
        {L"startup_enabled", L"[OK] Added to Windows startup"},
        {L"startup_disabled", L"[STOP] Windows startup disabled"},
        {L"status_web_url", L"Web page"},
        {L"status_ports", L"RAW ports"},
        {L"status_printer_count", L"Configured printers"},
        {L"frame_test_config", L"Connection Test"},
        {L"label_test_port", L"Test port"},
        {L"button_test_connection", L"Test connect"},
        {L"button_send_sample", L"Send sample"},
        {L"frame_test_logs", L"Test Logs"},
        {L"test_intro", L"Use localhost to verify the RAW listener for one configured port."},
        {L"frame_app_settings", L"Application Settings"},
        {L"setting_minimize_to_tray", L"Minimize to the system tray when closing the window"},
        {L"frame_language", L"Language"},
        {L"label_language", L"Interface language"},
        {L"button_apply_language", L"Apply language"},
        {L"language_option_tw", L"Traditional Chinese"},
        {L"language_option_en", L"English"},
        {L"frame_about", L"About"},
        {L"about_text", L"PrtEasyServer - Windows Network Print Server\r\nVersion 2.0.0\r\n\r\nC++ x64 edition for Windows 7 / 10 / 11.\r\nSupports RAW 9100 print sharing and a built-in setup web page."},
        {L"tray_show_window", L"Show Window"},
        {L"tray_hide_window", L"Hide Window"},
        {L"tray_quit", L"Quit"},
        {L"message_select_printer", L"Please select an installed printer first."},
        {L"message_invalid_raw_port", L"RAW port must be between 1 and 65535."},
        {L"message_invalid_web_port", L"Web port must be between 1 and 65535."},
        {L"message_duplicate_port", L"Each printer must use a different RAW port."},
        {L"message_save_failed", L"Failed to save config.json."},
        {L"message_start_server_first", L"Start the server first."},
        {L"message_invalid_test_port", L"Please enter a valid test port."},
        {L"message_language_save_failed", L"Failed to save the language setting."},
        {L"installer_success_title", L"Setup Complete"},
        {L"installer_success_message", L"Printer queue \"__PRINTER_NAME__\" is ready."},
        {L"installer_missing_title", L"Driver Not Found"},
        {L"installer_missing_message", L"The printer port was created, but the matching driver was not found. Please finish the Add Printer wizard manually."},
        {L"installer_queue_failed_title", L"Finish Setup Manually"},
        {L"installer_queue_failed_message", L"The driver package was found, but the printer queue could not be created automatically. The Add Printer window will open now."},
        {L"installer_error_extract_archive", L"Unable to extract the driver archive."},
        {L"installer_error_create_port", L"Failed to create printer port: "},
        {L"installer_extract_script_failed", L"Failed to extract embedded PowerShell script."},
        {L"installer_batch_failed", L"Installer failed. ErrorLevel=%ERR%"},
        {L"log_added_printer", L"Added printer entry: %s -> %d"},
        {L"log_updated_printer", L"Updated printer entry: %s -> %d"},
        {L"log_removed_printer", L"Removed printer entry: %s"},
        {L"log_saved_config", L"Saved configuration to: %s"},
        {L"log_start_done", L"Start request completed."},
        {L"log_start_failed", L"Server start failed."},
        {L"log_stop_done", L"Stop request completed."},
        {L"log_testing", L"Testing localhost:%d ..."},
        {L"log_test_ok", L"[TEST OK] %s"},
        {L"log_test_fail", L"[TEST FAIL] %s"},
        {L"log_language_restart", L"Language applied. Restarting the application..."},
        {L"log_auto_starting", L"Printer configuration found. Starting the server automatically..."},
        {L"log_no_printers_for_autostart", L"No configured printers were found, so auto-start was skipped."},
        {L"log_hidden_to_tray", L"Window hidden to the system tray."},
        {L"log_startup_added", L"Added to Windows startup."},
        {L"log_startup_removed", L"Removed from Windows startup."},
        {L"log_startup_add_failed", L"Failed to add the Windows startup entry: %s"},
        {L"log_startup_remove_failed", L"Failed to remove the Windows startup entry: %s"},
        {L"web_title", L"PrtEasyServer Printer Setup"},
        {L"web_eyebrow", L"PRTEASYSERVER SERVICE"},
        {L"web_heading", L"Windows Network Print Server"},
        {L"web_intro_1", L"This Windows PC is sharing local printers over RAW 9100 TCP/IP ports."},
        {L"web_intro_2", L"Open this page from a client PC, download the installer BAT, and run it with administrator rights when needed."},
        {L"web_intro_server_prefix", L"This server is currently using"},
        {L"web_intro_server_suffix", L"to provide printer installation settings."},
        {L"web_intro_download", L"Please download the matching setup file for each printer port below and run it on the client PC."},
        {L"web_intro_folder", L"Keep the setup file and driver package in the same folder for faster installation."},
        {L"web_manual_prefix", L"If you need manual TCP/IP setup, choose either:"},
        {L"web_manual_or", L"or"},
        {L"web_manual_recommend", L"(Recommended: use the host name so printing keeps working if the IP address changes.)"},
        {L"web_access_url", L"Web access"},
        {L"web_server_host", L"Server host"},
        {L"web_host_name", L"Host name"},
        {L"web_available_count", L"Available printer count"},
        {L"web_printer_badge", L"Printer %d"},
        {L"web_driver", L"Driver"},
        {L"web_raw_port", L"RAW port"},
        {L"web_host", L"Host"},
        {L"web_download_setup", L"Download setup file"},
        {L"web_download_driver", L"Download driver package"},
        {L"web_footer_note", L"Note: your browser may block downloads. Click \"Keep\" to continue."},
        {L"web_footer_version", L"Current server version:"},
        {L"web_footer_latest", L"For the latest version, visit:"},
        {L"web_empty_title", L"No printers configured"},
        {L"web_empty_body", L"Open the desktop app, add a printer entry, and start the server."},
    };
}

StringMap BuildBaseTraditionalChineseStrings() {
    return {
        {L"app_title", L"PrtEasyServer - Windows 網路印表機伺服器"},
        {L"tab_server", L"伺服器管理"},
        {L"tab_test", L"測試工具"},
        {L"tab_settings", L"設定"},
        {L"frame_config", L"印表機設定"},
        {L"config_hint", L"可同時新增多組印表機設定，啟動後會一起監聽所有已填寫的連接埠。"},
        {L"label_installed_printer", L"已安裝印表機"},
        {L"label_raw_port", L"連接埠"},
        {L"label_web_port", L"網頁連接埠"},
        {L"web_port_hint", L"設為 80 時，其他電腦可直接輸入伺服器 IP 開啟設定頁面。"},
        {L"button_add", L"新增"},
        {L"button_add_group", L"新增一組"},
        {L"button_update", L"更新"},
        {L"button_update_group", L"更新這組"},
        {L"button_remove", L"刪除"},
        {L"button_remove_group", L"刪除一組"},
        {L"button_refresh_printers", L"重新整理印表機"},
        {L"button_save", L"儲存設定"},
        {L"button_start", L"啟動伺服器"},
        {L"button_stop", L"停止伺服器"},
        {L"button_open_web", L"開啟網頁"},
        {L"button_add_startup", L"加入"},
        {L"button_remove_startup", L"移除"},
        {L"column_printer", L"印表機"},
        {L"column_port", L"連接埠"},
        {L"frame_server_control", L"伺服器控制"},
        {L"frame_autostart", L"開機自動啟動"},
        {L"frame_server_logs", L"伺服器日誌"},
        {L"status_running", L"[OK] 伺服器運行中"},
        {L"status_stopped", L"[STOP] 伺服器已停止"},
        {L"startup_enabled", L"[OK] 已加入開機自動啟動"},
        {L"startup_disabled", L"[STOP] 未啟用開機自動啟動"},
        {L"status_web_url", L"網頁設定"},
        {L"status_ports", L"RAW 連接埠"},
        {L"status_printer_count", L"已設定印表機"},
        {L"frame_test_config", L"連線測試"},
        {L"label_test_port", L"測試連接埠"},
        {L"button_test_connection", L"測試連線"},
        {L"button_send_sample", L"送出測試頁"},
        {L"frame_test_logs", L"測試日誌"},
        {L"test_intro", L"測試工具固定連到 localhost，用來驗證某個 RAW 監聽埠是否可用。"},
        {L"frame_app_settings", L"應用程式設定"},
        {L"setting_minimize_to_tray", L"關閉視窗時縮小到系統匣"},
        {L"frame_language", L"語系"},
        {L"label_language", L"介面語言"},
        {L"button_apply_language", L"套用語言"},
        {L"language_option_tw", L"繁體中文"},
        {L"language_option_en", L"English"},
        {L"frame_about", L"關於"},
        {L"about_text", L"PrtEasyServer - Windows 網路印表機伺服器\r\n版本 2.0.0\r\n\r\nC++ x64 版本，支援 Windows 7 / 10 / 11。\r\n提供 RAW 9100 列印分享與內建安裝設定網頁。"},
        {L"tray_show_window", L"顯示視窗"},
        {L"tray_hide_window", L"隱藏視窗"},
        {L"tray_quit", L"結束程式"},
        {L"message_select_printer", L"請先選擇已安裝的印表機。"},
        {L"message_invalid_raw_port", L"RAW 連接埠必須介於 1 到 65535 之間。"},
        {L"message_invalid_web_port", L"網頁連接埠必須介於 1 到 65535 之間。"},
        {L"message_duplicate_port", L"每台印表機都必須使用不同的 RAW 連接埠。"},
        {L"message_save_failed", L"儲存 config.json 失敗。"},
        {L"message_start_server_first", L"請先啟動伺服器。"},
        {L"message_invalid_test_port", L"請輸入正確的測試連接埠。"},
        {L"message_language_save_failed", L"儲存語言設定失敗。"},
        {L"installer_success_title", L"安裝完成"},
        {L"installer_success_message", L"印表機佇列「__PRINTER_NAME__」已可使用。"},
        {L"installer_missing_title", L"找不到驅動程式"},
        {L"installer_missing_message", L"已建立印表機連接埠，但找不到對應的驅動程式。請手動完成新增印表機精靈。"},
        {L"installer_queue_failed_title", L"請手動完成安裝"},
        {L"installer_queue_failed_message", L"已找到驅動程式套件，但無法自動建立印表機佇列。現在將開啟新增印表機視窗。"},
        {L"installer_error_extract_archive", L"無法解壓縮驅動程式壓縮檔。"},
        {L"installer_error_create_port", L"建立印表機連接埠失敗："},
        {L"installer_extract_script_failed", L"無法解開內嵌的 PowerShell 安裝腳本。"},
        {L"installer_batch_failed", L"安裝程式失敗。ErrorLevel=%ERR%"},
        {L"log_added_printer", L"已新增印表機設定: %s -> %d"},
        {L"log_updated_printer", L"已更新印表機設定: %s -> %d"},
        {L"log_removed_printer", L"已刪除印表機設定: %s"},
        {L"log_saved_config", L"已儲存設定檔: %s"},
        {L"log_start_done", L"已送出啟動伺服器要求。"},
        {L"log_start_failed", L"伺服器啟動失敗。"},
        {L"log_stop_done", L"已送出停止伺服器要求。"},
        {L"log_testing", L"正在測試 localhost:%d ..."},
        {L"log_test_ok", L"[測試成功] %s"},
        {L"log_test_fail", L"[測試失敗] %s"},
        {L"log_language_restart", L"語言已套用，正在重新啟動程式..."},
        {L"log_auto_starting", L"已找到印表機設定，正在自動啟動伺服器..."},
        {L"log_no_printers_for_autostart", L"目前沒有有效的印表機設定，已略過自動啟動。"},
        {L"log_hidden_to_tray", L"視窗已縮到系統匣。"},
        {L"log_startup_added", L"已加入 Windows 開機自動啟動。"},
        {L"log_startup_removed", L"已自 Windows 開機自動啟動移除。"},
        {L"log_startup_add_failed", L"加入 Windows 開機自動啟動失敗: %s"},
        {L"log_startup_remove_failed", L"移除 Windows 開機自動啟動失敗: %s"},
        {L"web_title", L"PrtEasyServer 印表機設定"},
        {L"web_eyebrow", L"PRTEASYSERVER SERVICE"},
        {L"web_heading", L"Windows 網路印表機伺服器"},
        {L"web_intro_1", L"這台 Windows 電腦目前透過 RAW 9100 TCP/IP 連接埠分享本機印表機。"},
        {L"web_intro_2", L"請在其他電腦開啟這個頁面，下載安裝 BAT，必要時以系統管理員身分執行。"},
        {L"web_intro_server_prefix", L"這台伺服器目前使用"},
        {L"web_intro_server_suffix", L"提供印表機安裝設定。"},
        {L"web_intro_download", L"請依照下方每台印表機的連接埠下載對應安裝檔，並在客戶端執行。"},
        {L"web_intro_folder", L"設定檔及驅動程式建議放在同個目錄以便快速安裝。"},
        {L"web_manual_prefix", L"如需手動 TCP/IP 設置請擇一選擇:"},
        {L"web_manual_or", L"或"},
        {L"web_manual_recommend", L"（建議使用 主機名稱 來設置，避免 IP 變動時無法連接）"},
        {L"web_access_url", L"網頁入口"},
        {L"web_server_host", L"伺服器主機"},
        {L"web_host_name", L"主機名稱"},
        {L"web_available_count", L"可安裝印表機數量"},
        {L"web_printer_badge", L"印表機%d"},
        {L"web_driver", L"驅動"},
        {L"web_raw_port", L"RAW 連接埠"},
        {L"web_host", L"主機"},
        {L"web_download_setup", L"下載設定檔"},
        {L"web_download_driver", L"下載驅動程式"},
        {L"web_footer_note", L"注意：下載檔案可能會被瀏覽器阻擋，可點「保留」繼續完成下載。"},
        {L"web_footer_version", L"伺服器目前版本："},
        {L"web_footer_latest", L"查詢最新版本請前往："},
        {L"web_empty_title", L"目前沒有印表機設定"},
        {L"web_empty_body", L"請先在桌面程式新增印表機設定並啟動伺服器。"},
    };
}

}  // namespace

LocalizationManager::LocalizationManager()
    : languageRoot_(JoinPath(GetModuleDirectory(), L"language")),
      currentLanguage_(L"en"),
      strings_(BuildDefaultEnglishStrings()),
      fallbackStrings_(BuildDefaultEnglishStrings()) {
}

bool LocalizationManager::Initialize(const std::wstring& requestedLanguage) {
    EnsureDefaultLanguageFiles();
    fallbackStrings_ = BuildDefaultEnglishStrings();
    const std::wstring requestedCode = NormalizeLanguageCode(requestedLanguage);

    if (requestedCode == L"tw") {
        currentLanguage_ = L"tw";
        strings_ = BuildDefaultTraditionalChineseStrings();
        LoadBundle(currentLanguage_, &strings_);
        return true;
    }

    if (requestedCode == L"en") {
        currentLanguage_ = L"en";
        strings_ = BuildDefaultEnglishStrings();
        LoadBundle(currentLanguage_, &strings_);
        return true;
    }

    StringMap customBundle = BuildDefaultEnglishStrings();
    if (LoadBundle(requestedCode, &customBundle)) {
        currentLanguage_ = requestedCode;
        strings_ = std::move(customBundle);
        return true;
    }

    currentLanguage_ = L"en";
    strings_ = BuildDefaultEnglishStrings();
    LoadBundle(currentLanguage_, &strings_);
    return true;
}

std::wstring LocalizationManager::Get(const std::wstring& key) const {
    const auto it = strings_.find(key);
    if (it != strings_.end()) {
        return it->second;
    }
    const auto fallback = fallbackStrings_.find(key);
    if (fallback != fallbackStrings_.end()) {
        return fallback->second;
    }
    return key;
}

std::wstring LocalizationManager::GetLanguageFilePath(const std::wstring& code) const {
    return JoinPath(languageRoot_, NormalizeLanguageCode(code) + L".txt");
}

const std::wstring& LocalizationManager::GetCurrentLanguage() const {
    return currentLanguage_;
}

std::vector<std::wstring> LocalizationManager::GetAvailableLanguages() const {
    EnsureDefaultLanguageFiles();

    std::vector<std::wstring> languages = GetSupportedLanguages();
    const std::wstring pattern = JoinPath(languageRoot_, L"*.txt");

    WIN32_FIND_DATAW data{};
    HANDLE findHandle = ::FindFirstFileW(pattern.c_str(), &data);
    if (findHandle == INVALID_HANDLE_VALUE) {
        return languages;
    }

    do {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            continue;
        }

        std::wstring fileName = data.cFileName;
        const std::size_t dot = fileName.find_last_of(L'.');
        if (dot == std::wstring::npos) {
            continue;
        }

        fileName = fileName.substr(0, dot);
        const std::wstring code = NormalizeLanguageCode(fileName);
        if (code.empty()) {
            continue;
        }
        if (std::find(languages.begin(), languages.end(), code) == languages.end()) {
            languages.push_back(code);
        }
    } while (::FindNextFileW(findHandle, &data));

    ::FindClose(findHandle);

    if (languages.size() > 2) {
        std::sort(languages.begin() + 2, languages.end());
    }

    return languages;
}

std::wstring LocalizationManager::NormalizeLanguageCode(const std::wstring& value) {
    std::wstring normalized = ToLowerCopy(Trim(ReplaceAll(value, L"_", L"-")));
    std::wstring safe;
    safe.reserve(normalized.size());
    for (wchar_t ch : normalized) {
        if ((ch >= L'a' && ch <= L'z') || (ch >= L'0' && ch <= L'9') || ch == L'-') {
            safe.push_back(ch);
        }
    }
    normalized = safe;
    if (normalized.empty()) {
        return L"en";
    }
    if (normalized == L"tw" || normalized == L"zh-tw" || normalized == L"zh-hk" || normalized == L"zh-mo" || normalized == L"zh-hant") {
        return L"tw";
    }
    if (normalized.find(L"zh-") == 0 && normalized.find(L"hant") != std::wstring::npos) {
        return L"tw";
    }
    if (normalized == L"en" || normalized == L"en-us" || normalized == L"en-gb") {
        return L"en";
    }
    return normalized;
}

std::wstring LocalizationManager::DetectDefaultLanguage() {
    wchar_t localeName[LOCALE_NAME_MAX_LENGTH] = {};
    if (::GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) > 0) {
        if (NormalizeLanguageCode(localeName) == L"tw") {
            return L"tw";
        }
    }

    const LANGID languageId = ::GetUserDefaultUILanguage();
    switch (PRIMARYLANGID(languageId)) {
        case LANG_CHINESE:
            switch (SUBLANGID(languageId)) {
                case SUBLANG_CHINESE_TRADITIONAL:
                case SUBLANG_CHINESE_HONGKONG:
                case SUBLANG_CHINESE_MACAU:
                    return L"tw";
                default:
                    break;
            }
            break;
        default:
            break;
    }

    return L"en";
}

std::vector<std::wstring> LocalizationManager::GetSupportedLanguages() {
    return {L"tw", L"en"};
}

void LocalizationManager::EnsureDefaultLanguageFiles() const {
    EnsureDirectory(languageRoot_);

    for (const std::wstring& code : GetSupportedLanguages()) {
        const std::wstring path = GetLanguageFilePath(code);
        StringMap merged = GetBuiltInBundle(code);
        if (FileExists(path)) {
            LoadBundle(code, &merged);
        }
        WriteUtf8File(path, WideToUtf8(SerializeLanguageFile(merged)), true);
    }

    const std::wstring pattern = JoinPath(languageRoot_, L"*.txt");
    WIN32_FIND_DATAW data{};
    HANDLE findHandle = ::FindFirstFileW(pattern.c_str(), &data);
    if (findHandle == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            continue;
        }

        const std::wstring path = JoinPath(languageRoot_, data.cFileName);
        const std::wstring raw = ReadUtf8File(path);
        if (raw.find(L"\n") != std::wstring::npos && raw.find(L"\r\n") == std::wstring::npos) {
            const std::wstring normalized = ReplaceAll(raw, L"\n", L"\r\n");
            WriteUtf8File(path, WideToUtf8(normalized), true);
        }
    } while (::FindNextFileW(findHandle, &data));

    ::FindClose(findHandle);
}

bool LocalizationManager::LoadBundle(const std::wstring& code, StringMap* target) const {
    if (target == nullptr) {
        return false;
    }

    const std::wstring path = GetLanguageFilePath(code);
    if (!FileExists(path)) {
        return false;
    }

    std::wstringstream stream(ReadUtf8File(path));
    std::wstring line;
    std::size_t applied = 0;
    while (std::getline(stream, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == L'#' || line[0] == L';') {
            continue;
        }
        const std::size_t eq = line.find(L'=');
        if (eq == std::wstring::npos) {
            continue;
        }
        const std::wstring key = Trim(line.substr(0, eq));
        const std::wstring value = UnescapeFileValue(line.substr(eq + 1));
        if (!key.empty()) {
            (*target)[key] = value;
            ++applied;
        }
    }
    return applied > 0;
}

LocalizationManager::StringMap LocalizationManager::BuildDefaultEnglishStrings() {
    return BuildBaseEnglishStrings();
}

LocalizationManager::StringMap LocalizationManager::BuildDefaultTraditionalChineseStrings() {
    return BuildBaseTraditionalChineseStrings();
}

std::wstring LocalizationManager::SerializeLanguageFile(const StringMap& bundle) {
    std::wostringstream output;
    output << L"# PrtEasyServer language file\r\n";
    output << L"# Edit the values after '=' and restart the app.\r\n\r\n";
    for (const auto& entry : bundle) {
        output << entry.first << L"=" << EscapeFileValue(entry.second) << L"\r\n";
    }
    return output.str();
}

std::wstring LocalizationManager::EscapeFileValue(const std::wstring& value) {
    std::wstring output = value;
    output = ReplaceAll(output, L"\\", L"\\\\");
    output = ReplaceAll(output, L"\r", L"\\r");
    output = ReplaceAll(output, L"\n", L"\\n");
    output = ReplaceAll(output, L"\t", L"\\t");
    return output;
}

std::wstring LocalizationManager::UnescapeFileValue(const std::wstring& value) {
    std::wstring output = value;
    output = ReplaceAll(output, L"\\r", L"\r");
    output = ReplaceAll(output, L"\\n", L"\n");
    output = ReplaceAll(output, L"\\t", L"\t");
    output = ReplaceAll(output, L"\\\\", L"\\");
    return output;
}

const LocalizationManager::StringMap& LocalizationManager::GetBuiltInBundle(const std::wstring& code) {
    static const StringMap english = BuildDefaultEnglishStrings();
    static const StringMap traditionalChinese = BuildDefaultTraditionalChineseStrings();
    return NormalizeLanguageCode(code) == L"tw" ? traditionalChinese : english;
}
