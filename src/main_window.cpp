#include "main_window.h"

#include "app_constants.h"
#include "resource.h"

namespace {

constexpr wchar_t kPageWindowClassName[] = L"PrtEasyServerPageWindow";

LRESULT CALLBACK PageWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND parent = ::GetParent(hwnd);
    switch (message) {
        case WM_COMMAND:
        case WM_NOTIFY:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORLISTBOX:
            if (parent != nullptr) {
                return ::SendMessageW(parent, message, wParam, lParam);
            }
            return 0;
        default:
            break;
    }
    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

HWND CreateLabel(HWND parent, const wchar_t* text, DWORD style = 0) {
    return ::CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE | style, 0, 0, 10, 10, parent, nullptr, nullptr, nullptr);
}

HWND CreateGroupBox(HWND parent, const wchar_t* text) {
    return ::CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 0, 0, 10, 10, parent, nullptr, nullptr, nullptr);
}

void SetEditText(HWND edit, const std::wstring& text) {
    ::SetWindowTextW(edit, text.c_str());
}

std::wstring GetWindowTextString(HWND window) {
    const int length = ::GetWindowTextLengthW(window);
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    if (length > 0) {
        ::GetWindowTextW(window, &text[0], length + 1);
    }
    text.resize(static_cast<std::size_t>(length));
    return text;
}

int ParsePortValue(const std::wstring& text, int defaultValue = 0) {
    const std::wstring trimmed = Trim(text);
    if (trimmed.empty()) {
        return defaultValue;
    }
    const int value = _wtoi(trimmed.c_str());
    if (value <= 0 || value > 65535) {
        return defaultValue;
    }
    return value;
}

void SetListViewColumnText(HWND list, int index, const std::wstring& text) {
    LVCOLUMNW column{};
    column.mask = LVCF_TEXT;
    column.pszText = const_cast<LPWSTR>(text.c_str());
    ListView_SetColumn(list, index, &column);
}

std::wstring JoinIntegerList(const std::vector<int>& values) {
    std::wstring output;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            output += L", ";
        }
        output += std::to_wstring(values[i]);
    }
    return output;
}

std::wstring BuildAboutText(bool traditionalChinese) {
    if (traditionalChinese) {
        return L"PrtEasyServer - Windows 網路印表機伺服器\r\n"
               L"版本 2.0\r\n"
               L"Copyright (c) 2026 Terence0816\r\n"
               L"GitHub: https://github.com/Terence0816/Windows-PrtEasyServer\r\n\r\n"
               L"基於 PrinterOne 修改：\r\n"
               L"https://github.com/xtieume/PrinterOne\r\n"
               L"Original Copyright (c) 2025 xtieume@gmail.com\r\n\r\n"
               L"這是一個簡易的 TCP/IP 列印伺服器，可將本機印表機轉成網路 IP 印表機。\r\n"
               L"支援 RAW 9100 列印，不需 Windows 網芳、SMB 分享或帳號密碼。";
    }

    return L"PrtEasyServer - Windows Network Print Server\r\n"
           L"Version 2.0\r\n"
           L"Copyright (c) 2026 Terence0816\r\n"
           L"GitHub: https://github.com/Terence0816/Windows-PrtEasyServer\r\n\r\n"
           L"Based on PrinterOne:\r\n"
           L"https://github.com/xtieume/PrinterOne\r\n"
           L"Original Copyright (c) 2025 xtieume@gmail.com\r\n\r\n"
           L"This is a lightweight TCP/IP print server that turns a local printer into a network IP printer.\r\n"
           L"Supports RAW 9100 printing without Windows file sharing, SMB, or account passwords.";
}

std::wstring BuildLegacyAboutText(bool traditionalChinese) {
    if (traditionalChinese) {
        return L"PrtEasyServer - Windows 網路印表機伺服器\r\n"
               L"版本 2.0\r\n"
               L"Copyright (c) 2026 Terence0816\r\n"
               L"GitHub: https://github.com/Terence0816/Windows-PrtEasyServer\r\n\r\n"
               L"基於 PrinterOne 修改：\r\n"
               L"https://github.com/xtieume/PrinterOne\r\n"
               L"Original Copyright (c) 2025 xtieume@gmail.com\r\n\r\n"
               L"這是一個簡易的 TCP/IP 列印伺服器，可將本機印表機轉成網路 IP 印表機。\r\n"
               L"支援 RAW 9100 列印，不需 Windows 網芳、SMB 分享或帳號密碼。";
    }

    return L"PrtEasyServer - Windows Network Print Server\r\n"
           L"Version 2.0\r\n"
           L"Copyright (c) 2026 Terence0816\r\n"
           L"GitHub: https://github.com/Terence0816/Windows-PrtEasyServer\r\n\r\n"
           L"Based on PrinterOne:\r\n"
           L"https://github.com/xtieume/PrinterOne\r\n"
           L"Original Copyright (c) 2025 xtieume@gmail.com\r\n\r\n"
           L"This is a lightweight TCP/IP print server that turns a local Windows printer into an IP printer.\r\n"
           L"It supports RAW 9100 printing without Windows file sharing, SMB, or network credentials.";
}

std::wstring BuildLegacyAboutTextEscaped(bool traditionalChinese) {
    if (traditionalChinese) {
        return L"PrtEasyServer - Windows \u7db2\u8def\u5370\u8868\u6a5f\u4f3a\u670d\u5668\r\n"
               L"\u7248\u672c 2.0\r\n"
               L"Copyright (c) 2026 Terence0816\r\n"
               L"GitHub: https://github.com/Terence0816/Windows-PrtEasyServer\r\n\r\n"
               L"\u57fa\u65bc PrinterOne \u4fee\u6539\uff1a\r\n"
               L"https://github.com/xtieume/PrinterOne\r\n"
               L"Original Copyright (c) 2025 xtieume@gmail.com\r\n\r\n"
               L"\u9019\u662f\u4e00\u500b\u7c21\u6613\u7684 TCP/IP \u5217\u5370\u4f3a\u670d\u5668\uff0c\u53ef\u5c07\u672c\u6a5f\u5370\u8868\u6a5f\u8f49\u6210\u7db2\u8def IP \u5370\u8868\u6a5f\u3002\r\n"
               L"\u652f\u63f4 RAW 9100 \u5217\u5370\uff0c\u4e0d\u9700 Windows \u7db2\u82b3\u3001SMB \u5206\u4eab\u6216\u5e33\u865f\u5bc6\u78bc\u3002";
    }

    return L"PrtEasyServer - Windows Network Print Server\r\n"
           L"Version 2.0\r\n"
           L"Copyright (c) 2026 Terence0816\r\n"
           L"GitHub: https://github.com/Terence0816/Windows-PrtEasyServer\r\n\r\n"
           L"Based on PrinterOne:\r\n"
           L"https://github.com/xtieume/PrinterOne\r\n"
           L"Original Copyright (c) 2025 xtieume@gmail.com\r\n\r\n"
           L"This is a lightweight TCP/IP print server that turns a local Windows printer into an IP printer.\r\n"
           L"It supports RAW 9100 printing without Windows file sharing, SMB, or network credentials.";
}

bool RewriteLogWithRegex(const std::wstring& input, const wchar_t* pattern, const wchar_t* replacement, std::wstring* output) {
    const std::wregex regex(pattern);
    if (!std::regex_match(input, regex)) {
        return false;
    }
    *output = std::regex_replace(input, regex, replacement);
    return true;
}

std::wstring LocalizeRuntimeLogLine(const std::wstring& message, bool traditionalChinese) {
    if (!traditionalChinese || message.empty()) {
        return message;
    }

    std::wstring prefix;
    std::wstring body = message;
    if (!body.empty() && body.front() == L'[') {
        const std::size_t split = body.find(L"] ");
        if (split != std::wstring::npos) {
            prefix = body.substr(0, split + 2);
            body = body.substr(split + 2);
        }
    }

    struct ExactTranslation {
        const wchar_t* english;
        const wchar_t* chinese;
    };

    static const ExactTranslation kExactTranslations[] = {
        {L"Firewall rule setup skipped because administrator privileges are required.",
         L"\u76ee\u524d\u4e0d\u662f\u7cfb\u7d71\u7ba1\u7406\u54e1\u8eab\u5206\uff0c\u7121\u6cd5\u81ea\u52d5\u5efa\u7acb\u9632\u706b\u7246\u898f\u5247\u3002"},
        {L"Server is already running.", L"\u4f3a\u670d\u5668\u5df2\u5728\u904b\u884c\u4e2d\u3002"},
        {L"No configured printer entries.", L"\u76ee\u524d\u6c92\u6709\u6709\u6548\u7684\u5370\u8868\u6a5f\u8a2d\u5b9a\u3002"},
        {L"Duplicate RAW ports found in configuration.", L"\u5370\u8868\u6a5f\u8a2d\u5b9a\u4e2d\u6709\u91cd\u8907\u7684 RAW \u9023\u63a5\u57e0\u3002"},
        {L"Web port conflicts with a printer RAW port.", L"\u7db2\u9801\u9023\u63a5\u57e0\u4e0d\u53ef\u8207\u5370\u8868\u6a5f RAW \u9023\u63a5\u57e0\u91cd\u8907\u3002"},
        {L"Failed to create RAW listener socket.", L"\u5efa\u7acb RAW \u76e3\u807d socket \u5931\u6557\u3002"},
        {L"Server started.", L"\u4f3a\u670d\u5668\u5df2\u555f\u52d5\u3002"},
        {L"Server stopped.", L"\u4f3a\u670d\u5668\u5df2\u505c\u6b62\u3002"},
    };

    for (const ExactTranslation& item : kExactTranslations) {
        if (body == item.english) {
            return prefix + item.chinese;
        }
    }

    std::wstring translated;
    if (RewriteLogWithRegex(body, LR"(^Firewall rule ready: (.+) \(TCP ([0-9]+)\)$)",
                            L"\u5df2\u5efa\u7acb\u898f\u5247\uff1a$1 (TCP $2)", &translated) ||
        RewriteLogWithRegex(body, LR"(^Firewall rule failed: (.+) \(TCP ([0-9]+)\) - (.+)$)",
                            L"\u9632\u706b\u7246\u898f\u5247\u5931\u6557\uff1a$1 (TCP $2) - $3", &translated) ||
        RewriteLogWithRegex(body, LR"(^Failed to bind RAW port ([0-9]+): (.+)$)",
                            L"\u7d81\u5b9a RAW \u9023\u63a5\u57e0 $1 \u5931\u6557\uff1a$2", &translated) ||
        RewriteLogWithRegex(body, LR"(^Failed to listen on RAW port ([0-9]+)\.$)",
                            L"RAW \u9023\u63a5\u57e0 $1 \u958b\u59cb\u76e3\u807d\u5931\u6557\u3002", &translated) ||
        RewriteLogWithRegex(body, LR"(^Listening on RAW ([0-9]+) for printer: (.+)$)",
                            L"\u5df2\u958b\u59cb\u76e3\u807d RAW $1\uff0c\u5370\u8868\u6a5f\uff1a$2", &translated) ||
        RewriteLogWithRegex(body, LR"(^Web setup page: (.+)$)",
                            L"\u7db2\u9801\u8a2d\u5b9a\u9801\uff1a$1", &translated) ||
        RewriteLogWithRegex(body, LR"(^Web server could not bind to port ([0-9]+)\.$)",
                            L"\u7db2\u9801\u4f3a\u670d\u5668\u7121\u6cd5\u7d81\u5b9a\u9023\u63a5\u57e0 $1\u3002", &translated) ||
        RewriteLogWithRegex(body, LR"(^Driver package ready: (.+)$)",
                            L"\u9a45\u52d5\u7a0b\u5f0f\u58d3\u7e2e\u6a94\u5df2\u5b8c\u6210\uff1a$1", &translated) ||
        RewriteLogWithRegex(body, LR"(^Driver package skipped for (.+): (.+)$)",
                            L"\u7565\u904e\u9a45\u52d5\u7a0b\u5f0f\u6253\u5305 $1\uff1a$2", &translated) ||
        RewriteLogWithRegex(body, LR"(^OpenPrinter failed for (.+): (.+)$)",
                            L"\u958b\u555f\u5370\u8868\u6a5f $1 \u5931\u6557\uff1a$2", &translated) ||
        RewriteLogWithRegex(body, LR"(^StartDocPrinter failed for (.+): (.+)$)",
                            L"\u958b\u59cb\u5217\u5370\u5de5\u4f5c $1 \u5931\u6557\uff1a$2", &translated) ||
        RewriteLogWithRegex(body, LR"(^StartPagePrinter failed for (.+): (.+)$)",
                            L"\u958b\u59cb\u5217\u5370\u9801\u9762 $1 \u5931\u6557\uff1a$2", &translated) ||
        RewriteLogWithRegex(body, LR"(^WritePrinter failed for (.+): (.+)$)",
                            L"\u5beb\u5165\u5370\u8868\u6a5f $1 \u5931\u6557\uff1a$2", &translated) ||
        RewriteLogWithRegex(body, LR"(^Sent ([0-9]+) bytes to printer: (.+)$)",
                            L"\u5df2\u50b3\u9001 $1 \u4f4d\u5143\u7d44\u5230\u5370\u8868\u6a5f\uff1a$2", &translated) ||
        RewriteLogWithRegex(body, LR"(^Client connected: (.+) -> (.+):([0-9]+)$)",
                            L"\u7528\u6236\u7aef\u5df2\u9023\u7dda\uff1a$1 -> $2:$3", &translated) ||
        RewriteLogWithRegex(body, LR"(^Connection closed with no data from (.+)\.$)",
                            L"\u4f86\u81ea $1 \u7684\u9023\u7dda\u672a\u50b3\u9001\u8cc7\u6599\u5c31\u5df2\u95dc\u9589\u3002", &translated) ||
        RewriteLogWithRegex(body, LR"(^Received ([0-9]+) bytes \((.+)\) for printer (.+)\.$)",
                            L"\u5df2\u63a5\u6536 $1 \u4f4d\u5143\u7d44\uff08$2\uff09\uff0c\u76ee\u6a19\u5370\u8868\u6a5f\uff1a$3\u3002", &translated)) {
        return prefix + translated;
    }

    return message;
}

HFONT CreateStatusFont(HFONT baseFont) {
    LOGFONTW lf{};
    if (baseFont != nullptr) {
        ::GetObjectW(baseFont, sizeof(lf), &lf);
    } else {
        lf.lfHeight = -26;
        wcscpy_s(lf.lfFaceName, L"Microsoft JhengHei");
    }
    lf.lfWeight = FW_BOLD;
    if (lf.lfHeight > -28) {
        lf.lfHeight = -28;
    }
    return ::CreateFontIndirectW(&lf);
}

HFONT CreateLinkFont(HFONT baseFont) {
    LOGFONTW lf{};
    if (baseFont != nullptr) {
        ::GetObjectW(baseFont, sizeof(lf), &lf);
    } else {
        lf.lfHeight = -16;
        wcscpy_s(lf.lfFaceName, L"Microsoft JhengHei");
    }
    lf.lfUnderline = TRUE;
    return ::CreateFontIndirectW(&lf);
}

void RefreshComboBoxChrome(HWND combo) {
    if (combo == nullptr) {
        return;
    }

    ::SetWindowPos(combo, nullptr, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    ::RedrawWindow(combo, nullptr, nullptr,
                   RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

    COMBOBOXINFO info{};
    info.cbSize = sizeof(info);
    if (::GetComboBoxInfo(combo, &info)) {
        if (info.hwndItem != nullptr) {
            ::RedrawWindow(info.hwndItem, nullptr, nullptr,
                           RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
        }
        if (info.hwndList != nullptr) {
            ::RedrawWindow(info.hwndList, nullptr, nullptr,
                           RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
        }
    }
}

}  // namespace

MainWindow::MainWindow(HINSTANCE instance, bool autoStart)
    : instance_(instance),
      server_([this](const std::wstring& message) { PostLog(message); }),
      autoStart_(autoStart) {
    font_ = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
    statusFont_ = CreateStatusFont(font_);
    linkFont_ = CreateLinkFont(font_);
    const AppConfig config = server_.GetConfigCopy();
    localizer_.Initialize(config.language);
    minimizeToTray_ = config.minimizeToTray;
}

MainWindow::~MainWindow() {
    RemoveTrayIcon();
    server_.Stop();
    if (statusFont_ != nullptr) {
        ::DeleteObject(statusFont_);
        statusFont_ = nullptr;
    }
    if (linkFont_ != nullptr) {
        ::DeleteObject(linkFont_);
        linkFont_ = nullptr;
    }
}

bool MainWindow::Create() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.hInstance = instance_;
    wc.lpfnWndProc = MainWindow::WindowProc;
    wc.lpszClassName = L"PrtEasyServerMainWindow";
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hIcon = ::LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hIconSm = wc.hIcon;

    if (!::RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    WNDCLASSEXW pageClass{};
    pageClass.cbSize = sizeof(pageClass);
    pageClass.hInstance = instance_;
    pageClass.lpfnWndProc = PageWindowProc;
    pageClass.lpszClassName = kPageWindowClassName;
    pageClass.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    pageClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    if (!::RegisterClassExW(&pageClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    hwnd_ = ::CreateWindowExW(
        0,
        wc.lpszClassName,
        localizer_.Get(L"app_title").c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1120,
        820,
        nullptr,
        nullptr,
        instance_,
        this);

    return hwnd_ != nullptr;
}

void MainWindow::Show(int showCommand) {
    if (autoStart_ && minimizeToTray_) {
        ::ShowWindow(hwnd_, SW_HIDE);
    } else if (autoStart_) {
        ::ShowWindow(hwnd_, SW_SHOWMINIMIZED);
    } else {
        ::ShowWindow(hwnd_, showCommand);
    }
    ::UpdateWindow(hwnd_);
    ::PostMessageW(hwnd_, kMsgFinalizeUi, 0, 0);
}

HWND MainWindow::GetHwnd() const {
    return hwnd_;
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    MainWindow* self = nullptr;
    if (message == WM_NCCREATE) {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<MainWindow*>(create->lpCreateParams);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self != nullptr) {
        return self->HandleMessage(message, wParam, lParam);
    }

    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            CreateControls();
            AddTrayIcon();
            RefreshInstalledPrinters();
            LoadConfigIntoUi();
            UpdateStatus();

            if (!autoStartEvaluated_) {
                autoStartEvaluated_ = true;
                if (HasConfiguredPrinters()) {
                    AppendLog(localizer_.Get(L"log_auto_starting"));
                    ::PostMessageW(hwnd_, WM_COMMAND, MAKEWPARAM(IdStartButton, BN_CLICKED), reinterpret_cast<LPARAM>(startButton_));
                    if (autoStart_ && minimizeToTray_) {
                        ::PostMessageW(hwnd_, kMsgHideToTray, 0, 0);
                    }
                } else {
                    AppendLog(localizer_.Get(L"log_no_printers_for_autostart"));
                }
            }
            return 0;
        }
        case WM_SIZE: {
            RECT rc{};
            ::GetClientRect(hwnd_, &rc);
            LayoutControls(rc.right - rc.left, rc.bottom - rc.top);
            RefreshComboBoxChrome(printerCombo_);
            RefreshComboBoxChrome(languageCombo_);
            return 0;
        }
        case kMsgFinalizeUi: {
            RECT rc{};
            ::GetClientRect(hwnd_, &rc);
            LayoutControls(rc.right - rc.left, rc.bottom - rc.top);
            ShowActiveTab();
            RefreshInstalledPrinters();
            LoadConfigIntoUi();
            RefreshComboBoxChrome(printerCombo_);
            RefreshComboBoxChrome(languageCombo_);
            ::InvalidateRect(serverPage_, nullptr, TRUE);
            ::UpdateWindow(serverPage_);
            ::InvalidateRect(settingsPage_, nullptr, TRUE);
            return 0;
        }
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IdAddUpdateButton:
                    OnAddOrUpdatePrinter();
                    break;
                case IdRemoveButton:
                    OnRemovePrinter();
                    break;
                case IdRefreshPrintersButton:
                    RefreshInstalledPrinters();
                    break;
                case IdSaveButton:
                    OnSaveConfig();
                    break;
                case IdStartButton:
                    OnStartServer();
                    break;
                case IdStopButton:
                    OnStopServer();
                    break;
                case IdOpenWebButton:
                    OnOpenWeb();
                    break;
                case IdAddStartupButton:
                    OnAddStartup();
                    break;
                case IdRemoveStartupButton:
                    OnRemoveStartup();
                    break;
                case IdApplyLanguageButton:
                    OnApplyLanguage();
                    break;
                case IdAboutGithubLink:
                    if (HIWORD(wParam) == STN_CLICKED) {
                        OpenUrlInBrowser(L"https://github.com/Terence0816/Windows-PrtEasyServer");
                    }
                    break;
                case IdMinimizeToTrayCheck:
                    minimizeToTray_ = (::SendMessageW(minimizeToTrayCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    break;
                default:
                    break;
            }
            return 0;
        }
        case WM_NOTIFY: {
            LPNMHDR header = reinterpret_cast<LPNMHDR>(lParam);
            if (header && header->idFrom == IdPrinterList && header->code == LVN_ITEMCHANGED) {
                LoadSelectedPrinterIntoEditor();
            } else if (header && header->idFrom == IdTabControl && header->code == TCN_SELCHANGE) {
                activeTab_ = TabCtrl_GetCurSel(tabControl_);
                if (activeTab_ < 0) {
                    activeTab_ = 0;
                }
                ShowActiveTab();
            }
            return 0;
        }
        case WM_CTLCOLORSTATIC: {
            HDC dc = reinterpret_cast<HDC>(wParam);
            HWND control = reinterpret_cast<HWND>(lParam);
            HWND parent = ::GetParent(control);
            if (control == aboutGithubLink_) {
                ::SetBkMode(dc, TRANSPARENT);
                ::SetBkColor(dc, ::GetSysColor(COLOR_BTNFACE));
                ::SetTextColor(dc, RGB(0, 102, 204));
                return reinterpret_cast<LRESULT>(::GetSysColorBrush(COLOR_BTNFACE));
            }
            if (control == logEdit_ || parent == printerCombo_ || parent == languageCombo_) {
                ::SetBkMode(dc, OPAQUE);
                ::SetBkColor(dc, ::GetSysColor(COLOR_WINDOW));
                ::SetTextColor(dc, RGB(0, 0, 0));
                return reinterpret_cast<LRESULT>(::GetSysColorBrush(COLOR_WINDOW));
            }
            ::SetBkMode(dc, TRANSPARENT);
            ::SetBkColor(dc, ::GetSysColor(COLOR_BTNFACE));
            if (control == serverStatusStatic_) {
                ::SetTextColor(dc, server_.IsRunning() ? RGB(0, 96, 0) : RGB(210, 0, 0));
            } else if (control == startupStatusStatic_) {
                std::wstring value;
                ::SetTextColor(dc, server_.IsStartupEnabled(&value) ? RGB(0, 96, 0) : RGB(210, 0, 0));
            } else {
                ::SetTextColor(dc, RGB(0, 0, 0));
            }
            return reinterpret_cast<LRESULT>(::GetSysColorBrush(COLOR_BTNFACE));
        }
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            HDC dc = reinterpret_cast<HDC>(wParam);
            ::SetTextColor(dc, RGB(0, 0, 0));
            ::SetBkColor(dc, ::GetSysColor(COLOR_WINDOW));
            return reinterpret_cast<LRESULT>(::GetSysColorBrush(COLOR_WINDOW));
        }
        case WM_CTLCOLORBTN:
            return reinterpret_cast<LRESULT>(::GetSysColorBrush(COLOR_BTNFACE));
        case WM_SETCURSOR:
            if (reinterpret_cast<HWND>(wParam) == aboutGithubLink_) {
                HCURSOR hand = ::LoadCursorW(nullptr, IDC_HAND);
                if (hand != nullptr) {
                    ::SetCursor(hand);
                    return TRUE;
                }
            }
            break;
        case kMsgAppendLog: {
            std::unique_ptr<std::wstring> text(reinterpret_cast<std::wstring*>(lParam));
            if (text) {
                AppendLog(*text);
            }
            return 0;
        }
        case kMsgTrayIcon: {
            const UINT eventCode = LOWORD(static_cast<DWORD>(lParam));
            if (eventCode == WM_LBUTTONDBLCLK || eventCode == NIN_SELECT || eventCode == NIN_KEYSELECT) {
                ShowFromTray();
            } else if (eventCode == WM_RBUTTONUP || eventCode == WM_CONTEXTMENU) {
                ShowTrayMenu();
            }
            return 0;
        }
        case kMsgHideToTray:
            HideToTray(false);
            return 0;
        case WM_CLOSE:
            if (!quitRequested_ && minimizeToTray_) {
                HideToTray(true);
                return 0;
            }
            RemoveTrayIcon();
            server_.Stop();
            ::DestroyWindow(hwnd_);
            return 0;
        case WM_DESTROY:
            RemoveTrayIcon();
            ::PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return ::DefWindowProcW(hwnd_, message, wParam, lParam);
}

void MainWindow::ApplyFont(HWND control) const {
    if (control != nullptr && font_ != nullptr) {
        ::SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    }
}

void MainWindow::CreateControls() {
    appIcon_ = ::LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON));
    if (appIcon_ == nullptr) {
        appIcon_ = ::LoadIconW(nullptr, IDI_APPLICATION);
    }

    tabControl_ = ::CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP,
                                    0, 0, 10, 10, hwnd_, reinterpret_cast<HMENU>(IdTabControl), instance_, nullptr);
    ApplyFont(tabControl_);

    TCITEMW item{};
    item.mask = TCIF_TEXT;
    item.pszText = const_cast<LPWSTR>(L"");
    TabCtrl_InsertItem(tabControl_, 0, &item);
    TabCtrl_InsertItem(tabControl_, 1, &item);

    serverPage_ = ::CreateWindowExW(0, kPageWindowClassName, L"", WS_CHILD | WS_VISIBLE,
                                    0, 0, 10, 10, hwnd_, nullptr, instance_, nullptr);
    settingsPage_ = ::CreateWindowExW(0, kPageWindowClassName, L"", WS_CHILD,
                                      0, 0, 10, 10, hwnd_, nullptr, instance_, nullptr);

    CreateServerPageControls();
    CreateSettingsPageControls();
    ApplyTranslations();
    ShowActiveTab();
}

void MainWindow::CreateServerPageControls() {
    configFrame_ = CreateGroupBox(serverPage_, L"");
    configHintStatic_ = CreateLabel(serverPage_, L"", SS_LEFT);
    printerLabel_ = CreateLabel(serverPage_, L"");
    printerCombo_ = ::CreateWindowExW(WS_EX_CLIENTEDGE, WC_COMBOBOXW, L"",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_NOINTEGRALHEIGHT | WS_VSCROLL,
                                      0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdPrinterCombo), instance_, nullptr);
    portLabel_ = CreateLabel(serverPage_, L"");
    portEdit_ = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                  0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdPortEdit), instance_, nullptr);
    addUpdateButton_ = ::CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                         0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdAddUpdateButton), instance_, nullptr);
    removeButton_ = ::CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                      0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdRemoveButton), instance_, nullptr);
    refreshPrintersButton_ = ::CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                               0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdRefreshPrintersButton), instance_, nullptr);
    saveButton_ = ::CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                    0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdSaveButton), instance_, nullptr);

    printerList_ = ::CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                     0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdPrinterList), instance_, nullptr);
    ListView_SetExtendedListViewStyle(printerList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    LVCOLUMNW col{};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    col.cx = 380;
    col.pszText = const_cast<LPWSTR>(L"Printer");
    ListView_InsertColumn(printerList_, 0, &col);
    col.cx = 110;
    col.pszText = const_cast<LPWSTR>(L"Port");
    ListView_InsertColumn(printerList_, 1, &col);

    webPortLabel_ = CreateLabel(serverPage_, L"");
    webPortEdit_ = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"80", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                     0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdWebPortEdit), instance_, nullptr);
    webPortHintStatic_ = CreateLabel(serverPage_, L"");

    serverControlFrame_ = CreateGroupBox(serverPage_, L"");
    serverStatusStatic_ = ::CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                                            0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdServerStatusStatic), instance_, nullptr);
    statusDetailsStatic_ = ::CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                             0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdStatusDetailsStatic), instance_, nullptr);
    startButton_ = ::CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                     0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdStartButton), instance_, nullptr);
    stopButton_ = ::CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                    0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdStopButton), instance_, nullptr);
    openWebButton_ = ::CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                       0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdOpenWebButton), instance_, nullptr);

    startupFrame_ = CreateGroupBox(serverPage_, L"");
    startupStatusStatic_ = ::CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                                             0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdStartupStatusStatic), instance_, nullptr);
    addStartupButton_ = ::CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                          0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdAddStartupButton), instance_, nullptr);
    removeStartupButton_ = ::CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                             0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdRemoveStartupButton), instance_, nullptr);

    logFrame_ = CreateGroupBox(serverPage_, L"");
    logEdit_ = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                                 0, 0, 10, 10, serverPage_, reinterpret_cast<HMENU>(IdLogEdit), instance_, nullptr);

    for (HWND control : {configFrame_, configHintStatic_, printerLabel_, printerCombo_, portLabel_, portEdit_, addUpdateButton_, removeButton_,
                         refreshPrintersButton_, saveButton_, printerList_, webPortLabel_, webPortEdit_, webPortHintStatic_,
                         serverControlFrame_, statusDetailsStatic_, startButton_, stopButton_, openWebButton_, startupFrame_,
                         addStartupButton_, removeStartupButton_, logFrame_, logEdit_}) {
        ApplyFont(control);
    }

    if (statusFont_ != nullptr) {
        ::SendMessageW(serverStatusStatic_, WM_SETFONT, reinterpret_cast<WPARAM>(statusFont_), TRUE);
    } else {
        ApplyFont(serverStatusStatic_);
    }
    ApplyFont(startupStatusStatic_);
}

void MainWindow::CreateSettingsPageControls() {
    appSettingsFrame_ = CreateGroupBox(settingsPage_, L"");
    minimizeToTrayCheck_ = ::CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                             0, 0, 10, 10, settingsPage_, reinterpret_cast<HMENU>(IdMinimizeToTrayCheck), instance_, nullptr);

    languageFrame_ = CreateGroupBox(settingsPage_, L"");
    languageLabel_ = CreateLabel(settingsPage_, L"");
    languageCombo_ = ::CreateWindowExW(WS_EX_CLIENTEDGE, WC_COMBOBOXW, L"",
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_NOINTEGRALHEIGHT | WS_VSCROLL,
                                       0, 0, 10, 10, settingsPage_, reinterpret_cast<HMENU>(IdLanguageCombo), instance_, nullptr);
    applyLanguageButton_ = ::CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                             0, 0, 10, 10, settingsPage_, reinterpret_cast<HMENU>(IdApplyLanguageButton), instance_, nullptr);

    aboutFrame_ = CreateGroupBox(settingsPage_, L"");
    aboutTitleStatic_ = CreateLabel(settingsPage_, L"");
    aboutVersionStatic_ = CreateLabel(settingsPage_, L"");
    aboutCopyrightStatic_ = CreateLabel(settingsPage_, L"");
    aboutGithubLabelStatic_ = CreateLabel(settingsPage_, L"");
    aboutGithubLink_ = ::CreateWindowExW(0, L"STATIC", L"",
                                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | SS_NOTIFY,
                                         0, 0, 10, 10, settingsPage_, reinterpret_cast<HMENU>(IdAboutGithubLink), instance_, nullptr);
    aboutLegacyHeaderStatic_ = CreateLabel(settingsPage_, L"");
    aboutLegacyUrlStatic_ = CreateLabel(settingsPage_, L"");
    aboutLegacyCopyrightStatic_ = CreateLabel(settingsPage_, L"");
    aboutSummaryStatic_ = CreateLabel(settingsPage_, L"", SS_LEFT);

    for (HWND control : {appSettingsFrame_, minimizeToTrayCheck_, languageFrame_, languageLabel_, languageCombo_, applyLanguageButton_,
                         aboutFrame_, aboutTitleStatic_, aboutVersionStatic_, aboutCopyrightStatic_, aboutGithubLabelStatic_,
                         aboutGithubLink_, aboutLegacyHeaderStatic_, aboutLegacyUrlStatic_, aboutLegacyCopyrightStatic_, aboutSummaryStatic_}) {
        ApplyFont(control);
    }
    if (linkFont_ != nullptr) {
        ::SendMessageW(aboutGithubLink_, WM_SETFONT, reinterpret_cast<WPARAM>(linkFont_), TRUE);
    }

    ::SendMessageW(printerCombo_, CB_SETMINVISIBLE, 12, 0);
    ::SendMessageW(printerCombo_, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), 22);
    ::SendMessageW(printerCombo_, CB_SETITEMHEIGHT, 0, 18);
    ::SendMessageW(printerCombo_, CB_SETDROPPEDWIDTH, 360, 0);
    ::SendMessageW(languageCombo_, CB_SETMINVISIBLE, 6, 0);
    ::SendMessageW(languageCombo_, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), 22);
    ::SendMessageW(languageCombo_, CB_SETITEMHEIGHT, 0, 18);
    ::SendMessageW(languageCombo_, CB_SETDROPPEDWIDTH, 220, 0);
}

void MainWindow::LayoutControls(int clientWidth, int clientHeight) {
    const int margin = 10;
    ::MoveWindow(tabControl_, margin, margin, clientWidth - margin * 2, clientHeight - margin * 2, TRUE);

    RECT pageRect{};
    ::GetWindowRect(tabControl_, &pageRect);
    ::MapWindowPoints(HWND_DESKTOP, hwnd_, reinterpret_cast<LPPOINT>(&pageRect), 2);
    TabCtrl_AdjustRect(tabControl_, FALSE, &pageRect);

    const int pageWidth = pageRect.right - pageRect.left;
    const int pageHeight = pageRect.bottom - pageRect.top;
    ::MoveWindow(serverPage_, pageRect.left, pageRect.top, pageWidth, pageHeight, TRUE);
    ::MoveWindow(settingsPage_, pageRect.left, pageRect.top, pageWidth, pageHeight, TRUE);

    LayoutServerPage(pageWidth, pageHeight);
    LayoutSettingsPage(pageWidth, pageHeight);
}

void MainWindow::LayoutServerPage(int width, int height) {
    const int margin = 12;
    const int gap = 14;
    const int rightWidth = 340;
    const int leftWidth = width - rightWidth - gap;
    const int topHeight = 420;

    ::MoveWindow(configFrame_, 0, 0, leftWidth, topHeight, TRUE);
    ::MoveWindow(configHintStatic_, margin, 28, leftWidth - margin * 2, 34, TRUE);
    ::MoveWindow(printerLabel_, margin, 72, 150, 20, TRUE);
    ::MoveWindow(printerCombo_, margin, 96, 360, 260, TRUE);
    ::MoveWindow(portLabel_, margin + 370, 72, 100, 20, TRUE);
    ::MoveWindow(portEdit_, margin + 370, 96, 110, 24, TRUE);
    ::MoveWindow(addUpdateButton_, margin + 494, 94, 100, 28, TRUE);

    ::MoveWindow(removeButton_, margin, 132, 90, 28, TRUE);
    ::MoveWindow(refreshPrintersButton_, margin + 100, 132, 150, 28, TRUE);
    ::MoveWindow(saveButton_, margin + 260, 132, 110, 28, TRUE);

    ::MoveWindow(printerList_, margin, 172, leftWidth - margin * 2, 190, TRUE);
    ::MoveWindow(webPortLabel_, margin, 374, 110, 20, TRUE);
    ::MoveWindow(webPortEdit_, margin + 120, 370, 90, 24, TRUE);
    ::MoveWindow(webPortHintStatic_, margin + 220, 372, leftWidth - margin * 2 - 220, 22, TRUE);

    ::MoveWindow(serverControlFrame_, leftWidth + gap, 0, rightWidth, 250, TRUE);
    const int controlLeft = leftWidth + gap + margin;
    ::MoveWindow(serverStatusStatic_, controlLeft, 34, rightWidth - margin * 2, 38, TRUE);
    ::MoveWindow(statusDetailsStatic_, controlLeft, 84, rightWidth - margin * 2, 90, TRUE);
    ::MoveWindow(startButton_, controlLeft, 186, 118, 30, TRUE);
    ::MoveWindow(stopButton_, controlLeft + 130, 186, 118, 30, TRUE);
    ::MoveWindow(openWebButton_, controlLeft, 222, 140, 30, TRUE);

    ::MoveWindow(startupFrame_, leftWidth + gap, 262, rightWidth, 158, TRUE);
    ::MoveWindow(startupStatusStatic_, controlLeft, 296, rightWidth - margin * 2, 24, TRUE);
    ::MoveWindow(addStartupButton_, controlLeft, 342, 100, 30, TRUE);
    ::MoveWindow(removeStartupButton_, controlLeft + 110, 342, 100, 30, TRUE);

    const int logTop = topHeight + gap;
    const int logHeight = height - logTop;
    ::MoveWindow(logFrame_, 0, logTop, width, logHeight, TRUE);
    ::MoveWindow(logEdit_, margin, logTop + 28, width - margin * 2, logHeight - 40, TRUE);
}

void MainWindow::LayoutSettingsPage(int width, int height) {
    const int margin = 12;
    ::MoveWindow(appSettingsFrame_, 0, 0, width, 90, TRUE);
    ::MoveWindow(minimizeToTrayCheck_, margin, 34, width - margin * 2, 24, TRUE);

    ::MoveWindow(languageFrame_, 0, 100, width, 90, TRUE);
    ::MoveWindow(languageLabel_, margin, 134, 150, 20, TRUE);
    ::MoveWindow(languageCombo_, margin + 160, 130, 220, 140, TRUE);
    ::MoveWindow(applyLanguageButton_, margin + 392, 128, 120, 30, TRUE);

    const int aboutTop = 200;
    const int aboutFrameHeight = std::min(340, std::max(250, height - aboutTop - 12));
    ::MoveWindow(aboutFrame_, 0, aboutTop, width, aboutFrameHeight, TRUE);
    const int aboutLeft = margin + 6;
    const int aboutWidth = width - aboutLeft - margin;
    ::MoveWindow(aboutTitleStatic_, aboutLeft, aboutTop + 30, aboutWidth, 22, TRUE);
    ::MoveWindow(aboutVersionStatic_, aboutLeft, aboutTop + 54, aboutWidth, 20, TRUE);
    ::MoveWindow(aboutCopyrightStatic_, aboutLeft, aboutTop + 76, aboutWidth, 20, TRUE);
    ::MoveWindow(aboutGithubLabelStatic_, aboutLeft, aboutTop + 98, 56, 20, TRUE);
    ::MoveWindow(aboutGithubLink_, aboutLeft + 58, aboutTop + 96, aboutWidth - 60, 24, TRUE);
    ::MoveWindow(aboutLegacyHeaderStatic_, aboutLeft, aboutTop + 136, aboutWidth, 20, TRUE);
    ::MoveWindow(aboutLegacyUrlStatic_, aboutLeft, aboutTop + 158, aboutWidth, 20, TRUE);
    ::MoveWindow(aboutLegacyCopyrightStatic_, aboutLeft, aboutTop + 180, aboutWidth, 20, TRUE);
    ::MoveWindow(aboutSummaryStatic_, aboutLeft, aboutTop + 226, aboutWidth, 48, TRUE);
}

void MainWindow::ShowActiveTab() {
    ::ShowWindow(serverPage_, activeTab_ == 0 ? SW_SHOW : SW_HIDE);
    ::ShowWindow(settingsPage_, activeTab_ == 1 ? SW_SHOW : SW_HIDE);
}

void MainWindow::ApplyTranslations() {
    UpdateTitle();

    TCITEMW item{};
    item.mask = TCIF_TEXT;
    std::wstring serverTab = localizer_.Get(L"tab_server");
    item.pszText = const_cast<LPWSTR>(serverTab.c_str());
    TabCtrl_SetItem(tabControl_, 0, &item);
    std::wstring settingsTab = localizer_.Get(L"tab_settings");
    item.pszText = const_cast<LPWSTR>(settingsTab.c_str());
    TabCtrl_SetItem(tabControl_, 1, &item);

    ::SetWindowTextW(configFrame_, localizer_.Get(L"frame_config").c_str());
    ::SetWindowTextW(configHintStatic_, localizer_.Get(L"config_hint").c_str());
    ::SetWindowTextW(printerLabel_, localizer_.Get(L"label_installed_printer").c_str());
    ::SetWindowTextW(portLabel_, localizer_.Get(L"label_raw_port").c_str());
    ::SetWindowTextW(addUpdateButton_, (editingRow_ >= 0 ? localizer_.Get(L"button_update_group") : localizer_.Get(L"button_add_group")).c_str());
    ::SetWindowTextW(removeButton_, localizer_.Get(L"button_remove_group").c_str());
    ::SetWindowTextW(refreshPrintersButton_, localizer_.Get(L"button_refresh_printers").c_str());
    ::SetWindowTextW(saveButton_, localizer_.Get(L"button_save").c_str());
    ::SetWindowTextW(webPortLabel_, localizer_.Get(L"label_web_port").c_str());
    ::SetWindowTextW(webPortHintStatic_, localizer_.Get(L"web_port_hint").c_str());
    ::SetWindowTextW(serverControlFrame_, localizer_.Get(L"frame_server_control").c_str());
    ::SetWindowTextW(startButton_, localizer_.Get(L"button_start").c_str());
    ::SetWindowTextW(stopButton_, localizer_.Get(L"button_stop").c_str());
    ::SetWindowTextW(openWebButton_, localizer_.Get(L"button_open_web").c_str());
    ::SetWindowTextW(startupFrame_, localizer_.Get(L"frame_autostart").c_str());
    ::SetWindowTextW(addStartupButton_, localizer_.Get(L"button_add_startup").c_str());
    ::SetWindowTextW(removeStartupButton_, localizer_.Get(L"button_remove_startup").c_str());
    ::SetWindowTextW(logFrame_, localizer_.Get(L"frame_server_logs").c_str());
    SetListViewColumnText(printerList_, 0, localizer_.Get(L"column_printer"));
    SetListViewColumnText(printerList_, 1, localizer_.Get(L"column_port"));

    ::SetWindowTextW(appSettingsFrame_, localizer_.Get(L"frame_app_settings").c_str());
    ::SetWindowTextW(minimizeToTrayCheck_, localizer_.Get(L"setting_minimize_to_tray").c_str());
    ::SetWindowTextW(languageFrame_, localizer_.Get(L"frame_language").c_str());
    ::SetWindowTextW(languageLabel_, localizer_.Get(L"label_language").c_str());
    ::SetWindowTextW(applyLanguageButton_, localizer_.Get(L"button_apply_language").c_str());
    ::SetWindowTextW(aboutFrame_, localizer_.Get(L"frame_about").c_str());

    const bool traditionalChinese = (localizer_.GetCurrentLanguage() == L"tw");
    ::SetWindowTextW(aboutTitleStatic_, traditionalChinese
        ? L"PrtEasyServer - Windows \u7db2\u8def\u5370\u8868\u6a5f\u4f3a\u670d\u5668"
        : L"PrtEasyServer - Windows Network Print Server");
    ::SetWindowTextW(aboutVersionStatic_, traditionalChinese ? L"\u7248\u672c 2.0" : L"Version 2.0");
    ::SetWindowTextW(aboutCopyrightStatic_, L"Copyright (c) 2026 Terence0816");
    ::SetWindowTextW(aboutGithubLabelStatic_, L"GitHub:");
    ::SetWindowTextW(aboutGithubLink_, L"https://github.com/Terence0816/Windows-PrtEasyServer");
    ::SetWindowTextW(aboutLegacyHeaderStatic_, traditionalChinese ? L"\u57fa\u65bc PrinterOne \u4fee\u6539\uff1a" : L"Based on PrinterOne:");
    ::SetWindowTextW(aboutLegacyUrlStatic_, L"https://github.com/xtieume/PrinterOne");
    ::SetWindowTextW(aboutLegacyCopyrightStatic_, L"Original Copyright (c) 2025 xtieume@gmail.com");
    ::SetWindowTextW(aboutSummaryStatic_, traditionalChinese
        ? L"\u9019\u662f\u4e00\u500b\u7c21\u6613\u7684 TCP/IP \u5217\u5370\u4f3a\u670d\u5668\uff0c\u53ef\u5c07\u672c\u6a5f\u5370\u8868\u6a5f\u8f49\u6210\u7db2\u8def IP \u5370\u8868\u6a5f\u3002\r\n\u652f\u63f4 RAW 9100 \u5217\u5370\uff0c\u4e0d\u9700 Windows \u7db2\u82b3\u3001SMB \u5206\u4eab\u6216\u5e33\u865f\u5bc6\u78bc\u3002"
        : L"This is a lightweight TCP/IP print server that turns a local Windows printer into an IP printer.\r\nIt supports RAW 9100 printing without Windows file sharing, SMB, or network credentials.");

    PopulateLanguageCombo();
}

void MainWindow::PopulateLanguageCombo() {
    languageCodes_.clear();
    ::SendMessageW(languageCombo_, CB_RESETCONTENT, 0, 0);

    for (const std::wstring& code : localizer_.GetAvailableLanguages()) {
        std::wstring label;
        if (code == L"tw") {
            label = localizer_.Get(L"language_option_tw");
        } else if (code == L"en") {
            label = localizer_.Get(L"language_option_en");
        } else {
            label = code;
        }
        const LRESULT index = ::SendMessageW(languageCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        if (index != CB_ERR) {
            languageCodes_.push_back(code);
        }
    }

    const std::wstring current = localizer_.GetCurrentLanguage();
    for (std::size_t i = 0; i < languageCodes_.size(); ++i) {
        if (languageCodes_[i] == current) {
            ::SendMessageW(languageCombo_, CB_SETCURSEL, static_cast<WPARAM>(i), 0);
            return;
        }
    }

    if (!languageCodes_.empty()) {
        ::SendMessageW(languageCombo_, CB_SETCURSEL, 0, 0);
    }
    RefreshComboBoxChrome(languageCombo_);
}

void MainWindow::UpdateTitle() {
    const std::wstring title = localizer_.Get(L"app_title");
    ::SetWindowTextW(hwnd_, title.c_str());
    UpdateTrayIcon();
}

void MainWindow::RefreshInstalledPrinters() {
    const std::wstring currentSelection = GetWindowTextString(printerCombo_);
    ::SendMessageW(printerCombo_, CB_RESETCONTENT, 0, 0);
    const std::vector<std::wstring> printers = server_.EnumerateInstalledPrinters();
    for (const std::wstring& printer : printers) {
        ::SendMessageW(printerCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(printer.c_str()));
    }

    bool restoredSelection = false;
    if (!currentSelection.empty()) {
        const LRESULT index = ::SendMessageW(printerCombo_, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(currentSelection.c_str()));
        if (index != CB_ERR) {
            ::SendMessageW(printerCombo_, CB_SETCURSEL, index, 0);
            restoredSelection = true;
        }
    }

    if (!restoredSelection && ::SendMessageW(printerCombo_, CB_GETCOUNT, 0, 0) > 0) {
        ::SendMessageW(printerCombo_, CB_SETCURSEL, 0, 0);
    }
    RefreshComboBoxChrome(printerCombo_);
}

void MainWindow::LoadConfigIntoUi() {
    const AppConfig config = server_.GetConfigCopy();
    minimizeToTray_ = config.minimizeToTray;
    ::SendMessageW(minimizeToTrayCheck_, BM_SETCHECK, minimizeToTray_ ? BST_CHECKED : BST_UNCHECKED, 0);
    UpdatePrinterListView(config.printers);
    SetEditText(webPortEdit_, std::to_wstring(config.webPort));
    SetSuggestedNextPrinterPort();
    if (!config.printers.empty()) {
        const PrinterConfigEntry& first = config.printers.front();
        if (!first.printerName.empty()) {
            const LRESULT index = ::SendMessageW(printerCombo_, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(first.printerName.c_str()));
            if (index != CB_ERR) {
                ::SendMessageW(printerCombo_, CB_SETCURSEL, index, 0);
            }
        }
    }
    RefreshComboBoxChrome(printerCombo_);
    RefreshComboBoxChrome(languageCombo_);
}

AppConfig MainWindow::CollectConfigFromUi(bool* ok, std::wstring* errorText) const {
    AppConfig config;
    config.printers = ReadPrinterListView();
    config.webPort = ParsePortValue(GetWindowTextString(webPortEdit_), 80);
    config.language = localizer_.GetCurrentLanguage();
    config.minimizeToTray = (::SendMessageW(minimizeToTrayCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED);

    if (config.webPort <= 0 || config.webPort > 65535) {
        if (ok) {
            *ok = false;
        }
        if (errorText) {
            *errorText = localizer_.Get(L"message_invalid_web_port");
        }
        return config;
    }

    std::vector<int> ports;
    for (const PrinterConfigEntry& printer : config.printers) {
        if (std::find(ports.begin(), ports.end(), printer.port) != ports.end()) {
            if (ok) {
                *ok = false;
            }
            if (errorText) {
                *errorText = localizer_.Get(L"message_duplicate_port");
            }
            return config;
        }
        ports.push_back(printer.port);
    }

    if (ok) {
        *ok = true;
    }
    return config;
}

void MainWindow::UpdatePrinterListView(const std::vector<PrinterConfigEntry>& printers) {
    ListView_DeleteAllItems(printerList_);
    int rowIndex = 0;
    for (const PrinterConfigEntry& printer : printers) {
        if (Trim(printer.printerName).empty() || printer.port <= 0) {
            continue;
        }
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = rowIndex;
        item.pszText = const_cast<LPWSTR>(printer.printerName.c_str());
        ListView_InsertItem(printerList_, &item);
        std::wstring portText = std::to_wstring(printer.port);
        ListView_SetItemText(printerList_, rowIndex, 1, const_cast<LPWSTR>(portText.c_str()));
        ++rowIndex;
    }
}

std::vector<PrinterConfigEntry> MainWindow::ReadPrinterListView() const {
    std::vector<PrinterConfigEntry> printers;
    const int count = ListView_GetItemCount(printerList_);
    wchar_t buffer[512] = {};
    for (int i = 0; i < count; ++i) {
        PrinterConfigEntry entry;
        ListView_GetItemText(printerList_, i, 0, buffer, _countof(buffer));
        entry.printerName = Trim(buffer);
        ListView_GetItemText(printerList_, i, 1, buffer, _countof(buffer));
        entry.port = ParsePortValue(buffer, 0);
        if (!entry.printerName.empty() && entry.port > 0) {
            printers.push_back(entry);
        }
    }
    return printers;
}

int MainWindow::GetSelectedPrinterIndex() const {
    return ListView_GetNextItem(printerList_, -1, LVNI_SELECTED);
}

void MainWindow::LoadSelectedPrinterIntoEditor() {
    const int index = GetSelectedPrinterIndex();
    if (index < 0) {
        ResetPrinterEditor();
        return;
    }

    wchar_t buffer[512] = {};
    ListView_GetItemText(printerList_, index, 0, buffer, _countof(buffer));
    const std::wstring printerName = buffer;
    ListView_GetItemText(printerList_, index, 1, buffer, _countof(buffer));
    const std::wstring portText = buffer;

    const LRESULT comboIndex = ::SendMessageW(printerCombo_, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(printerName.c_str()));
    if (comboIndex != CB_ERR) {
        ::SendMessageW(printerCombo_, CB_SETCURSEL, comboIndex, 0);
    }
    SetEditText(portEdit_, portText);
    editingRow_ = index;
    ::SetWindowTextW(addUpdateButton_, localizer_.Get(L"button_update_group").c_str());
}

void MainWindow::ResetPrinterEditor() {
    editingRow_ = -1;
    ::SetWindowTextW(addUpdateButton_, localizer_.Get(L"button_add_group").c_str());
    if (::SendMessageW(printerCombo_, CB_GETCOUNT, 0, 0) > 0) {
        ::SendMessageW(printerCombo_, CB_SETCURSEL, 0, 0);
    }
    SetSuggestedNextPrinterPort();
}

int MainWindow::SuggestNextPrinterPort() const {
    const int count = static_cast<int>(ReadPrinterListView().size());
    return 9100 + count * 100;
}

void MainWindow::SetSuggestedNextPrinterPort() {
    if (editingRow_ >= 0) {
        return;
    }
    SetEditText(portEdit_, std::to_wstring(SuggestNextPrinterPort()));
}

bool MainWindow::IsPortAlreadyUsed(int port, int ignoreRow) const {
    const int count = ListView_GetItemCount(printerList_);
    wchar_t buffer[64] = {};
    for (int i = 0; i < count; ++i) {
        if (i == ignoreRow) {
            continue;
        }
        ListView_GetItemText(printerList_, i, 1, buffer, _countof(buffer));
        if (ParsePortValue(buffer, 0) == port) {
            return true;
        }
    }
    return false;
}

bool MainWindow::HasConfiguredPrinters() const {
    return !ReadPrinterListView().empty();
}

void MainWindow::AppendLog(const std::wstring& message) {
    const std::wstring localized = LocalizeRuntimeLogLine(message, localizer_.GetCurrentLanguage() == L"tw");
    const std::wstring line = localized + L"\r\n";
    const int length = ::GetWindowTextLengthW(logEdit_);
    ::SendMessageW(logEdit_, EM_SETSEL, length, length);
    ::SendMessageW(logEdit_, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(line.c_str()));
    ::SendMessageW(logEdit_, EM_SCROLLCARET, 0, 0);
    UpdateStatus();
}

void MainWindow::PostLog(const std::wstring& message) {
    if (hwnd_ == nullptr) {
        return;
    }
    std::wstring* text = new std::wstring(message);
    ::PostMessageW(hwnd_, kMsgAppendLog, 0, reinterpret_cast<LPARAM>(text));
}

void MainWindow::UpdateStatus() {
    const std::wstring statusText = server_.IsRunning() ? localizer_.Get(L"status_running") : localizer_.Get(L"status_stopped");
    ::SetWindowTextW(serverStatusStatic_, statusText.c_str());

    std::wstring startupCommand;
    const bool startupEnabled = server_.IsStartupEnabled(&startupCommand);
    const std::wstring startupText = startupEnabled ? localizer_.Get(L"startup_enabled") : localizer_.Get(L"startup_disabled");
    ::SetWindowTextW(startupStatusStatic_, startupText.c_str());

    const std::vector<PrinterConfigEntry> printers = ReadPrinterListView();
    std::vector<int> ports;
    for (const PrinterConfigEntry& printer : printers) {
        ports.push_back(printer.port);
    }

    std::wstring details = localizer_.Get(L"status_printer_count") + L": " + std::to_wstring(printers.size());
    if (!ports.empty()) {
        details += L"\r\n" + localizer_.Get(L"status_ports") + L": " + JoinIntegerList(ports);
    }
    if (server_.IsRunning()) {
        details += L"\r\n" + localizer_.Get(L"status_web_url") + L": " + server_.GetWebUrl();
    }
    ::SetWindowTextW(statusDetailsStatic_, details.c_str());

    ::EnableWindow(startButton_, server_.IsRunning() ? FALSE : TRUE);
    ::EnableWindow(stopButton_, server_.IsRunning() ? TRUE : FALSE);
    ::EnableWindow(openWebButton_, server_.IsRunning() ? TRUE : FALSE);
    ::EnableWindow(addStartupButton_, startupEnabled ? FALSE : TRUE);
    ::EnableWindow(removeStartupButton_, startupEnabled ? TRUE : FALSE);

    UpdateTrayIcon();
    ::InvalidateRect(serverStatusStatic_, nullptr, TRUE);
    ::InvalidateRect(startupStatusStatic_, nullptr, TRUE);
}

void MainWindow::UpdateStartupStatus() {
    UpdateStatus();
}

void MainWindow::OnAddOrUpdatePrinter() {
    wchar_t printerBuffer[512] = {};
    const LRESULT selection = ::SendMessageW(printerCombo_, CB_GETCURSEL, 0, 0);
    if (selection != CB_ERR) {
        ::SendMessageW(printerCombo_, CB_GETLBTEXT, selection, reinterpret_cast<LPARAM>(printerBuffer));
    }

    const std::wstring printerName = Trim(printerBuffer);
    const int port = ParsePortValue(GetWindowTextString(portEdit_), 0);
    if (printerName.empty()) {
        ::MessageBoxW(hwnd_, localizer_.Get(L"message_select_printer").c_str(), localizer_.Get(L"app_title").c_str(), MB_ICONWARNING | MB_OK);
        return;
    }
    if (port <= 0 || port > 65535) {
        ::MessageBoxW(hwnd_, localizer_.Get(L"message_invalid_raw_port").c_str(), localizer_.Get(L"app_title").c_str(), MB_ICONWARNING | MB_OK);
        return;
    }
    if (IsPortAlreadyUsed(port, editingRow_ >= 0 ? editingRow_ : -1)) {
        ::MessageBoxW(hwnd_, localizer_.Get(L"message_duplicate_port").c_str(), localizer_.Get(L"app_title").c_str(), MB_ICONWARNING | MB_OK);
        return;
    }

    if (editingRow_ >= 0) {
        ListView_SetItemText(printerList_, editingRow_, 0, const_cast<LPWSTR>(printerName.c_str()));
        std::wstring portText = std::to_wstring(port);
        ListView_SetItemText(printerList_, editingRow_, 1, const_cast<LPWSTR>(portText.c_str()));
        const std::wstring pattern = localizer_.Get(L"log_updated_printer");
        AppendLog(FormatString(pattern.c_str(), printerName.c_str(), port));
    } else {
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = ListView_GetItemCount(printerList_);
        item.pszText = const_cast<LPWSTR>(printerName.c_str());
        const int row = ListView_InsertItem(printerList_, &item);
        std::wstring portText = std::to_wstring(port);
        ListView_SetItemText(printerList_, row, 1, const_cast<LPWSTR>(portText.c_str()));
        const std::wstring pattern = localizer_.Get(L"log_added_printer");
        AppendLog(FormatString(pattern.c_str(), printerName.c_str(), port));
    }

    ResetPrinterEditor();
}

void MainWindow::OnRemovePrinter() {
    const int index = GetSelectedPrinterIndex();
    if (index < 0) {
        return;
    }
    wchar_t buffer[512] = {};
    ListView_GetItemText(printerList_, index, 0, buffer, _countof(buffer));
    const std::wstring removedPrinterName = buffer;
    ListView_DeleteItem(printerList_, index);
    ResetPrinterEditor();
    const std::wstring pattern = localizer_.Get(L"log_removed_printer");
    AppendLog(FormatString(pattern.c_str(), removedPrinterName.c_str()));
}

void MainWindow::OnSaveConfig() {
    bool ok = false;
    std::wstring errorText;
    AppConfig config = CollectConfigFromUi(&ok, &errorText);
    if (!ok) {
        ::MessageBoxW(hwnd_, errorText.c_str(), localizer_.Get(L"app_title").c_str(), MB_ICONWARNING | MB_OK);
        return;
    }

    server_.UpdateConfig(config);
    if (!server_.SaveConfig()) {
        ::MessageBoxW(hwnd_, localizer_.Get(L"message_save_failed").c_str(), localizer_.Get(L"app_title").c_str(), MB_ICONERROR | MB_OK);
        return;
    }
    const std::wstring pattern = localizer_.Get(L"log_saved_config");
    AppendLog(FormatString(pattern.c_str(), server_.GetConfigPath().c_str()));
    UpdateStatus();
}

void MainWindow::OnStartServer() {
    bool ok = false;
    std::wstring errorText;
    AppConfig config = CollectConfigFromUi(&ok, &errorText);
    if (!ok) {
        ::MessageBoxW(hwnd_, errorText.c_str(), localizer_.Get(L"app_title").c_str(), MB_ICONWARNING | MB_OK);
        return;
    }
    server_.UpdateConfig(config);
    if (!server_.SaveConfig()) {
        ::MessageBoxW(hwnd_, localizer_.Get(L"message_save_failed").c_str(), localizer_.Get(L"app_title").c_str(), MB_ICONERROR | MB_OK);
        return;
    }
    if (server_.Start()) {
        AppendLog(localizer_.Get(L"log_start_done"));
    } else {
        AppendLog(localizer_.Get(L"log_start_failed"));
    }
    UpdateStatus();
}

void MainWindow::OnStopServer() {
    server_.Stop();
    AppendLog(localizer_.Get(L"log_stop_done"));
    UpdateStatus();
}

void MainWindow::OnOpenWeb() {
    if (!server_.IsRunning()) {
        ::MessageBoxW(hwnd_, localizer_.Get(L"message_start_server_first").c_str(), localizer_.Get(L"app_title").c_str(), MB_ICONINFORMATION | MB_OK);
        return;
    }
    OpenUrlInBrowser(server_.GetWebUrl());
}

void MainWindow::OnAddStartup() {
    std::wstring message;
    if (server_.AddToStartup(&message)) {
        AppendLog(localizer_.Get(L"log_startup_added"));
    } else {
        const std::wstring pattern = localizer_.Get(L"log_startup_add_failed");
        const std::wstring full = FormatString(pattern.c_str(), message.c_str());
        AppendLog(full);
        ::MessageBoxW(hwnd_, full.c_str(), localizer_.Get(L"app_title").c_str(), MB_ICONWARNING | MB_OK);
    }
    UpdateStartupStatus();
}

void MainWindow::OnRemoveStartup() {
    std::wstring message;
    if (server_.RemoveFromStartup(&message)) {
        AppendLog(localizer_.Get(L"log_startup_removed"));
    } else {
        const std::wstring pattern = localizer_.Get(L"log_startup_remove_failed");
        const std::wstring full = FormatString(pattern.c_str(), message.c_str());
        AppendLog(full);
        ::MessageBoxW(hwnd_, full.c_str(), localizer_.Get(L"app_title").c_str(), MB_ICONWARNING | MB_OK);
    }
    UpdateStartupStatus();
}

void MainWindow::OnApplyLanguage() {
    const LRESULT selection = ::SendMessageW(languageCombo_, CB_GETCURSEL, 0, 0);
    if (selection == CB_ERR || selection < 0 || static_cast<std::size_t>(selection) >= languageCodes_.size()) {
        return;
    }

    const std::wstring newLanguage = languageCodes_[static_cast<std::size_t>(selection)];
    if (newLanguage == localizer_.GetCurrentLanguage()) {
        return;
    }

    bool ok = false;
    std::wstring ignoredError;
    AppConfig config = CollectConfigFromUi(&ok, &ignoredError);
    if (!ok) {
        config = server_.GetConfigCopy();
    }
    config.language = newLanguage;
    config.minimizeToTray = (::SendMessageW(minimizeToTrayCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED);
    server_.UpdateConfig(config);
    if (!server_.SaveConfig()) {
        ::MessageBoxW(hwnd_, localizer_.Get(L"message_language_save_failed").c_str(), localizer_.Get(L"app_title").c_str(), MB_ICONERROR | MB_OK);
        return;
    }

    AppendLog(localizer_.Get(L"log_language_restart"));
    RestartApplication(server_.IsRunning());
}

void MainWindow::RestartApplication(bool restoreServer) {
    std::wstring commandLine = L"\"" + JoinPath(GetModuleDirectory(), L"PrtEasyServer.exe") + L"\"";
    if (restoreServer) {
        commandLine += L" --autostart";
    }

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    std::vector<wchar_t> buffer(commandLine.begin(), commandLine.end());
    buffer.push_back(L'\0');
    if (::CreateProcessW(nullptr, buffer.data(), nullptr, nullptr, FALSE, 0, nullptr, GetModuleDirectory().c_str(), &startupInfo, &processInfo)) {
        ::CloseHandle(processInfo.hThread);
        ::CloseHandle(processInfo.hProcess);
    }

    QuitApplication();
}

void MainWindow::AddTrayIcon() {
    if (trayIconAdded_) {
        return;
    }

    trayData_ = {};
    trayData_.cbSize = sizeof(trayData_);
    trayData_.hWnd = hwnd_;
    trayData_.uID = 1;
    trayData_.uCallbackMessage = kMsgTrayIcon;
    trayData_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    trayData_.hIcon = appIcon_;
    std::wstring tip = localizer_.Get(L"app_title");
    wcsncpy_s(trayData_.szTip, tip.c_str(), _TRUNCATE);
    if (::Shell_NotifyIconW(NIM_ADD, &trayData_)) {
        trayIconAdded_ = true;
        trayData_.uVersion = NOTIFYICON_VERSION_4;
        ::Shell_NotifyIconW(NIM_SETVERSION, &trayData_);
    }
}

void MainWindow::RemoveTrayIcon() {
    if (trayIconAdded_) {
        ::Shell_NotifyIconW(NIM_DELETE, &trayData_);
        trayIconAdded_ = false;
    }
}

void MainWindow::UpdateTrayIcon() {
    if (!trayIconAdded_) {
        return;
    }

    trayData_.uFlags = NIF_ICON | NIF_TIP;
    trayData_.hIcon = appIcon_;
    std::wstring tip = localizer_.Get(L"app_title");
    tip += server_.IsRunning() ? L" [ON]" : L" [OFF]";
    wcsncpy_s(trayData_.szTip, tip.c_str(), _TRUNCATE);
    ::Shell_NotifyIconW(NIM_MODIFY, &trayData_);
}

void MainWindow::ShowFromTray() {
    ::ShowWindow(hwnd_, SW_SHOW);
    ::ShowWindow(hwnd_, SW_RESTORE);
    ::SetForegroundWindow(hwnd_);
}

void MainWindow::HideToTray(bool logAction) {
    ::ShowWindow(hwnd_, SW_HIDE);
    if (logAction) {
        AppendLog(localizer_.Get(L"log_hidden_to_tray"));
    }
}

void MainWindow::ShowTrayMenu() {
    HMENU menu = ::CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }

    ::AppendMenuW(menu, MF_STRING, IdTrayShowWindow, localizer_.Get(L"tray_show_window").c_str());
    ::AppendMenuW(menu, MF_STRING, IdTrayHideWindow, localizer_.Get(L"tray_hide_window").c_str());
    ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(menu, MF_STRING | (server_.IsRunning() ? MF_GRAYED : 0), IdTrayStartServer, localizer_.Get(L"button_start").c_str());
    ::AppendMenuW(menu, MF_STRING | (!server_.IsRunning() ? MF_GRAYED : 0), IdTrayStopServer, localizer_.Get(L"button_stop").c_str());
    ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(menu, MF_STRING, IdTrayQuit, localizer_.Get(L"tray_quit").c_str());

    POINT pt{};
    ::GetCursorPos(&pt);
    ::SetForegroundWindow(hwnd_);
    const UINT command = ::TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd_, nullptr);
    ::DestroyMenu(menu);
    ::PostMessageW(hwnd_, WM_NULL, 0, 0);

    switch (command) {
        case IdTrayShowWindow:
            ShowFromTray();
            break;
        case IdTrayHideWindow:
            HideToTray(false);
            break;
        case IdTrayStartServer:
            OnStartServer();
            break;
        case IdTrayStopServer:
            OnStopServer();
            break;
        case IdTrayQuit:
            QuitApplication();
            break;
        default:
            break;
    }
}

void MainWindow::QuitApplication() {
    quitRequested_ = true;
    ::PostMessageW(hwnd_, WM_CLOSE, 0, 0);
}
