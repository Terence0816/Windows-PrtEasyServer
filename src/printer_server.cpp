#include "printer_server.h"

#include "localization.h"

#include <winreg.h>

namespace {

std::wstring SanitizeToken(const std::wstring& value) {
    std::wstring output;
    output.reserve(value.size());
    for (wchar_t ch : value) {
        if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z')) {
            output.push_back(ch);
        } else {
            output.push_back(L'_');
        }
    }
    output = Trim(output);
    return output.empty() ? L"HOST" : output;
}

std::wstring MakeTcpPortName(const std::wstring& hostName, int port) {
    return L"IP_" + SanitizeToken(hostName) + L"_" + std::to_wstring(port);
}

std::wstring GetWindowsDirectoryPath() {
    const std::wstring windowsDir = GetEnvVar(L"WINDIR");
    return windowsDir.empty() ? L"C:\\Windows" : windowsDir;
}

std::wstring NormalizePathForCompare(const std::wstring& path) {
    std::wstring normalized = path;
    std::replace(normalized.begin(), normalized.end(), L'/', L'\\');
    normalized = Trim(normalized);
    if (!normalized.empty() && normalized.back() == L'\\') {
        normalized.pop_back();
    }
    return ToLowerCopy(normalized);
}

bool PathStartsWith(const std::wstring& value, const std::wstring& prefix) {
    const std::wstring left = NormalizePathForCompare(value);
    const std::wstring right = NormalizePathForCompare(prefix);
    if (right.empty() || left.size() < right.size()) {
        return false;
    }
    if (left.compare(0, right.size(), right) != 0) {
        return false;
    }
    return left.size() == right.size() || left[right.size()] == L'\\';
}

std::wstring GetExecutablePath() {
    wchar_t buffer[MAX_PATH] = {};
    ::GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return buffer;
}

bool SendAll(SOCKET socketHandle, const char* data, std::size_t size) {
    std::size_t sent = 0;
    while (sent < size) {
        const int result = ::send(socketHandle, data + sent, static_cast<int>(size - sent), 0);
        if (result <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(result);
    }
    return true;
}

std::wstring GetRegistryStringValue(HKEY rootKey, const std::wstring& subKey, const std::wstring& valueName) {
    HKEY keyHandle = nullptr;
    if (::RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_READ, &keyHandle) != ERROR_SUCCESS) {
        return std::wstring();
    }

    DWORD type = 0;
    DWORD size = 0;
    std::wstring result;
    if (::RegQueryValueExW(keyHandle, valueName.c_str(), nullptr, &type, nullptr, &size) == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
        std::vector<wchar_t> buffer(size / sizeof(wchar_t) + 1, L'\0');
        if (::RegQueryValueExW(keyHandle, valueName.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(buffer.data()), &size) == ERROR_SUCCESS) {
            result.assign(buffer.data());
        }
    }

    ::RegCloseKey(keyHandle);
    return result;
}

std::vector<std::wstring> GetStartupValueNames() {
    return {kStartupValueName, L"PrtEasyServerCpp", L"PrinterOneManager"};
}

std::wstring GetDriverInfPathFromRegistry(const std::wstring& driverName, const std::wstring& environmentName) {
    const std::vector<std::wstring> environments = {
        environmentName,
        L"Windows x64",
        L"Windows NT x86"
    };
    const std::vector<std::wstring> versions = {
        L"Version-4",
        L"Version-3",
        L"Version-2"
    };

    for (const std::wstring& env : environments) {
        if (Trim(env).empty()) {
            continue;
        }
        for (const std::wstring& version : versions) {
            const std::wstring keyPath = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Environments\\" + env +
                                         L"\\Drivers\\" + version + L"\\" + driverName;
            std::wstring infPath = GetRegistryStringValue(HKEY_LOCAL_MACHINE, keyPath, L"InfPath");
            infPath = Trim(infPath);
            if (infPath.empty()) {
                continue;
            }
            if (infPath.find(L":\\") == std::wstring::npos) {
                infPath = JoinPath(JoinPath(GetEnvVar(L"WINDIR").empty() ? L"C:\\Windows" : GetEnvVar(L"WINDIR"), L"INF"), infPath);
            }
            if (FileExists(infPath)) {
                return infPath;
            }
        }
    }
    return std::wstring();
}

std::vector<std::wstring> SplitMultiSz(const wchar_t* multiString) {
    std::vector<std::wstring> values;
    if (multiString == nullptr) {
        return values;
    }

    const wchar_t* current = multiString;
    while (*current != L'\0') {
        std::wstring item = current;
        if (!Trim(item).empty()) {
            values.push_back(item);
        }
        current += item.size() + 1;
    }
    return values;
}

std::string MakeAsciiDownloadName(const std::wstring& name) {
    const std::string utf8 = WideToUtf8(name);
    std::string output;
    output.reserve(utf8.size());
    for (unsigned char ch : utf8) {
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' || ch == '-') {
            output.push_back(static_cast<char>(ch));
        } else {
            output.push_back('_');
        }
    }
    if (output.empty()) {
        output = "download.bin";
    }
    return output;
}

std::string BuildHttpHeaderLine(const std::string& key, const std::string& value) {
    return key + ": " + value + "\r\n";
}

std::string ReadHttpRequest(SOCKET socketHandle) {
    std::string request;
    request.reserve(4096);
    char buffer[2048] = {};
    while (request.find("\r\n\r\n") == std::string::npos && request.size() < 16384) {
        const int received = ::recv(socketHandle, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            break;
        }
        request.append(buffer, buffer + received);
    }
    return request;
}

std::string DetectRawDataDescription(const std::vector<unsigned char>& data) {
    if (data.empty()) {
        return "empty";
    }
    if (data.size() >= 2 && data[0] == 0x1B && data[1] == 0x25) {
        return "PCL/PJL";
    }
    if (data.size() >= 4 && data[0] == '%' && data[1] == 'P' && data[2] == 'D' && data[3] == 'F') {
        return "PDF";
    }
    if (data.size() >= 4 && data[0] == '%' && data[1] == '!' && data[2] == 'P' && data[3] == 'S') {
        return "PostScript";
    }
    if (!data.empty() && data[0] == 0x1B) {
        return "ESC/P";
    }
    return "RAW";
}

bool IsWindowsInfDirectory(const std::wstring& folderPath) {
    return NormalizePathForCompare(folderPath) == NormalizePathForCompare(JoinPath(GetWindowsDirectoryPath(), L"INF"));
}

bool IsDriverStoreDirectory(const std::wstring& folderPath) {
    const std::wstring driverStoreRoot = JoinPath(JoinPath(JoinPath(GetWindowsDirectoryPath(), L"System32"), L"DriverStore"), L"FileRepository");
    return PathStartsWith(folderPath, driverStoreRoot);
}

std::wstring GetDriverStoreRoot() {
    return JoinPath(JoinPath(JoinPath(GetWindowsDirectoryPath(), L"System32"), L"DriverStore"), L"FileRepository");
}

std::wstring ReadTextFileForSearch(const std::wstring& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return std::wstring();
    }

    std::string data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (data.empty()) {
        return std::wstring();
    }

    if (data.size() >= 2) {
        const unsigned char b0 = static_cast<unsigned char>(data[0]);
        const unsigned char b1 = static_cast<unsigned char>(data[1]);
        if (b0 == 0xFF && b1 == 0xFE) {
            return std::wstring(reinterpret_cast<const wchar_t*>(data.data() + 2),
                                reinterpret_cast<const wchar_t*>(data.data() + data.size()));
        }
    }

    if (data.size() >= 3 &&
        static_cast<unsigned char>(data[0]) == 0xEF &&
        static_cast<unsigned char>(data[1]) == 0xBB &&
        static_cast<unsigned char>(data[2]) == 0xBF) {
        return Utf8ToWide(data.substr(3));
    }

    const int utf8Len = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, data.c_str(), static_cast<int>(data.size()), nullptr, 0);
    if (utf8Len > 0) {
        std::wstring wide(static_cast<std::size_t>(utf8Len), L'\0');
        ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, data.c_str(), static_cast<int>(data.size()), &wide[0], utf8Len);
        return wide;
    }

    const int acpLen = ::MultiByteToWideChar(CP_ACP, 0, data.c_str(), static_cast<int>(data.size()), nullptr, 0);
    if (acpLen > 0) {
        std::wstring wide(static_cast<std::size_t>(acpLen), L'\0');
        ::MultiByteToWideChar(CP_ACP, 0, data.c_str(), static_cast<int>(data.size()), &wide[0], acpLen);
        return wide;
    }

    return std::wstring();
}

std::vector<std::wstring> BuildSearchTokens(const std::wstring& value) {
    std::vector<std::wstring> tokens;
    std::wstring current;

    for (wchar_t ch : ToLowerCopy(value)) {
        if ((ch >= L'a' && ch <= L'z') || (ch >= L'0' && ch <= L'9')) {
            current.push_back(ch);
        } else if (!current.empty()) {
            if (current.size() >= 3) {
                tokens.push_back(current);
            }
            current.clear();
        }
    }

    if (!current.empty() && current.size() >= 3) {
        tokens.push_back(current);
    }

    return tokens;
}

std::vector<std::wstring> BuildFileHintsFromPaths(const std::vector<std::wstring>& files) {
    std::vector<std::wstring> hints;
    for (const std::wstring& path : files) {
        const std::size_t sep = path.find_last_of(L"\\/");
        std::wstring base = sep == std::wstring::npos ? path : path.substr(sep + 1);
        base = ToLowerCopy(Trim(base));
        if (!base.empty() && std::find(hints.begin(), hints.end(), base) == hints.end()) {
            hints.push_back(base);
        }
    }
    return hints;
}

int ScoreDriverInfText(const std::wstring& infTextLower, const std::wstring& driverName, const std::vector<std::wstring>& fileHints) {
    int score = 0;
    const std::wstring driverLower = ToLowerCopy(Trim(driverName));
    if (!driverLower.empty() && infTextLower.find(driverLower) != std::wstring::npos) {
        score += 100;
    } else {
        const std::vector<std::wstring> tokens = BuildSearchTokens(driverLower);
        int matchedTokens = 0;
        for (const std::wstring& token : tokens) {
            if (infTextLower.find(token) != std::wstring::npos) {
                ++matchedTokens;
            }
        }
        if (!tokens.empty()) {
            if (matchedTokens == static_cast<int>(tokens.size())) {
                score += matchedTokens * 10;
            } else if (matchedTokens >= std::max(2, static_cast<int>(tokens.size() / 2))) {
                score += matchedTokens * 4;
            }
        }
    }

    for (const std::wstring& hint : fileHints) {
        if (!hint.empty() && infTextLower.find(hint) != std::wstring::npos) {
            score += 35;
        }
    }

    return score;
}

bool FindBestDriverStorePackage(const std::wstring& driverName,
                                const std::vector<std::wstring>& fileHints,
                                std::wstring* bestFolder,
                                std::wstring* bestInfPath) {
    const std::wstring driverStoreRoot = GetDriverStoreRoot();
    if (!DirectoryExists(driverStoreRoot)) {
        return false;
    }

    int bestScore = 0;
    std::wstring chosenFolder;
    std::wstring chosenInfPath;

    const std::vector<std::wstring> files = EnumerateFilesRecursive(driverStoreRoot);
    for (const std::wstring& path : files) {
        const std::wstring lower = ToLowerCopy(path);
        if (lower.size() < 4 || lower.substr(lower.size() - 4) != L".inf") {
            continue;
        }

        const std::wstring text = ReadTextFileForSearch(path);
        if (text.empty()) {
            continue;
        }

        const int score = ScoreDriverInfText(ToLowerCopy(text), driverName, fileHints);
        if (score <= 0) {
            continue;
        }

        const std::wstring folder = path.substr(0, path.find_last_of(L"\\/"));
        if (score > bestScore ||
            (score == bestScore && folder.size() < chosenFolder.size()) ||
            (score == bestScore && chosenFolder.empty())) {
            bestScore = score;
            chosenFolder = folder;
            chosenInfPath = path;
        }
    }

    if (bestScore <= 0 || chosenFolder.empty()) {
        return false;
    }

    if (bestFolder) {
        *bestFolder = chosenFolder;
    }
    if (bestInfPath) {
        *bestInfPath = chosenInfPath;
    }
    return true;
}

}  // namespace

PrinterServer::PrinterServer(std::function<void(const std::wstring&)> logCallback)
    : logCallback_(std::move(logCallback)),
      config_(configStore_.Load()) {
    WSADATA wsaData{};
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);
}

PrinterServer::~PrinterServer() {
    Stop();
    ::WSACleanup();
}

AppConfig PrinterServer::GetConfigCopy() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return config_;
}

void PrinterServer::UpdateConfig(const AppConfig& config) {
    std::lock_guard<std::mutex> lock(configMutex_);
    config_ = config;
}

bool PrinterServer::SaveConfig() {
    std::lock_guard<std::mutex> lock(configMutex_);
    return configStore_.Save(config_);
}

std::wstring PrinterServer::GetConfigPath() const {
    return configStore_.GetConfigPath();
}

void PrinterServer::Log(const std::wstring& message) const {
    const std::wstring full = L"[" + CurrentTimestamp() + L"] " + message;
    if (logCallback_) {
        logCallback_(full);
    }
}

std::wstring PrinterServer::UiText(const std::wstring& english, const std::wstring& traditionalChinese) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    const std::wstring language = ToLowerCopy(Trim(config_.language));
    return (language == L"tw" || language == L"zh-tw") ? traditionalChinese : english;
}

std::vector<PrinterConfigEntry> PrinterServer::GetActivePrinters() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    std::vector<PrinterConfigEntry> active;
    for (const PrinterConfigEntry& entry : config_.printers) {
        PrinterConfigEntry cleaned = entry;
        cleaned.printerName = Trim(cleaned.printerName);
        if (!cleaned.printerName.empty() && cleaned.port > 0 && cleaned.port <= 65535) {
            active.push_back(cleaned);
        }
    }
    return active;
}

int PrinterServer::GetWebPort() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    if (config_.webPort <= 0 || config_.webPort > 65535) {
        return 80;
    }
    return config_.webPort;
}

std::vector<std::wstring> PrinterServer::EnumerateInstalledPrinters() const {
    DWORD needed = 0;
    DWORD returned = 0;
    ::EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, nullptr, 4, nullptr, 0, &needed, &returned);
    if (needed == 0) {
        return {};
    }

    std::vector<unsigned char> buffer(needed);
    if (!::EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, nullptr, 4, buffer.data(), needed, &needed, &returned)) {
        return {};
    }

    PRINTER_INFO_4W* printers = reinterpret_cast<PRINTER_INFO_4W*>(buffer.data());
    std::vector<std::wstring> names;
    for (DWORD i = 0; i < returned; ++i) {
        if (printers[i].pPrinterName != nullptr) {
            names.push_back(printers[i].pPrinterName);
        }
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return names;
}

PrinterDetails PrinterServer::QueryPrinterDetails(const std::wstring& printerName) const {
    PrinterDetails details;
    details.printerName = printerName;
    details.driverName = printerName;

    HANDLE printerHandle = nullptr;
    if (!::OpenPrinterW(const_cast<LPWSTR>(printerName.c_str()), &printerHandle, nullptr)) {
        return details;
    }

    DWORD needed = 0;
    ::GetPrinterW(printerHandle, 2, nullptr, 0, &needed);
    if (needed > 0) {
        std::vector<unsigned char> buffer(needed);
        if (::GetPrinterW(printerHandle, 2, buffer.data(), needed, &needed)) {
            PRINTER_INFO_2W* info = reinterpret_cast<PRINTER_INFO_2W*>(buffer.data());
            if (info->pPrinterName) {
                details.printerName = info->pPrinterName;
            }
            if (info->pDriverName) {
                details.driverName = info->pDriverName;
            }
            if (info->pPortName) {
                details.printerPortName = info->pPortName;
            }
        }
    }

    ::ClosePrinter(printerHandle);
    return details;
}

std::vector<WebPrinterEntry> PrinterServer::BuildWebEntries() const {
    std::vector<PrinterConfigEntry> printers = GetActivePrinters();
    const std::wstring hostName = GetComputerNameShort();
    const std::wstring hostIp = GetLocalIPv4Address();
    std::vector<WebPrinterEntry> entries;

    for (std::size_t i = 0; i < printers.size(); ++i) {
        const PrinterConfigEntry& printer = printers[i];
        const PrinterDetails details = QueryPrinterDetails(printer.printerName);

        WebPrinterEntry entry;
        entry.index = static_cast<int>(i + 1);
        entry.printerName = details.printerName.empty() ? printer.printerName : details.printerName;
        entry.driverName = details.driverName.empty() ? printer.printerName : details.driverName;
        entry.hostName = hostName;
        entry.hostIp = hostIp;
        entry.port = printer.port;
        entry.tcpPortName = MakeTcpPortName(hostName, printer.port);
        entry.driverArchiveName = SanitizeFileName(entry.driverName) + L".zip";
        entry.driverArchivePath = JoinPath(GetModuleDirectory(), entry.driverArchiveName);
        entries.push_back(entry);
    }

    return entries;
}

bool PrinterServer::EnsureFirewallPortRule(int port, const std::wstring& ruleName) const {
    if (port <= 0 || port > 65535) {
        return false;
    }

    std::wstring deleteCommand = L"netsh advfirewall firewall delete rule name=\"";
    deleteCommand += ruleName;
    deleteCommand += L"\" protocol=TCP localport=";
    deleteCommand += std::to_wstring(port);
    DWORD deleteExitCode = 0;
    RunHiddenProcess(deleteCommand, &deleteExitCode, nullptr);

    std::wstring command = L"netsh advfirewall firewall add rule name=\"";
    command += ruleName;
    command += L"\" dir=in action=allow protocol=TCP localport=";
    command += std::to_wstring(port);

    DWORD exitCode = 0;
    std::wstring output;
    const bool ok = RunHiddenProcess(command, &exitCode, &output) && exitCode == 0;
    if (ok) {
        Log(FormatString(UiText(L"Firewall rule ready: %s (TCP %d)",
                                L"已建立防火牆規則: %s (TCP %d)").c_str(),
                         ruleName.c_str(), port));
    } else {
        const std::wstring reason = Trim(output).empty() ? FormatLastErrorMessage() : Trim(output);
        Log(FormatString(UiText(L"Firewall rule failed: %s (TCP %d) - %s",
                                L"建立防火牆規則失敗: %s (TCP %d) - %s").c_str(),
                         ruleName.c_str(), port, reason.c_str()));
    }
    return ok;
}

void PrinterServer::EnsureFirewallRules() const {
    if (!IsRunningAsAdmin()) {
        Log(UiText(L"Firewall rule setup skipped because administrator privileges are required.",
                   L"略過建立防火牆規則，需系統管理員權限。"));
        return;
    }

    const std::vector<PrinterConfigEntry> printers = GetActivePrinters();
    for (const PrinterConfigEntry& printer : printers) {
        EnsureFirewallPortRule(printer.port, FormatString(L"%s RAW %d", kAppDisplayName, printer.port));
    }
    EnsureFirewallPortRule(GetWebPort(), FormatString(L"%s WEB %d", kAppDisplayName, GetWebPort()));
}

bool PrinterServer::Start() {
    if (running_) {
        Log(UiText(L"Server is already running.", L"伺服器已在運行中。"));
        return false;
    }

    const std::vector<PrinterConfigEntry> printers = GetActivePrinters();
    if (printers.empty()) {
        Log(UiText(L"No configured printer entries.", L"目前沒有有效的印表機設定。"));
        return false;
    }

    std::vector<int> ports;
    ports.reserve(printers.size());
    for (const PrinterConfigEntry& printer : printers) {
        if (std::find(ports.begin(), ports.end(), printer.port) != ports.end()) {
            Log(UiText(L"Duplicate RAW ports found in configuration.",
                       L"印表機設定中的 RAW 連接埠重複。"));
            return false;
        }
        ports.push_back(printer.port);
    }

    const int webPort = GetWebPort();
    if (std::find(ports.begin(), ports.end(), webPort) != ports.end()) {
        Log(UiText(L"Web port conflicts with a printer RAW port.",
                   L"網頁連接埠不可與印表機 RAW 連接埠重複。"));
        return false;
    }

    EnsureFirewallRules();

    std::vector<std::unique_ptr<ListenerContext>> newListeners;
    auto cleanupNewListeners = [&newListeners]() {
        for (std::unique_ptr<ListenerContext>& listener : newListeners) {
            if (listener->socketHandle != INVALID_SOCKET) {
                ::closesocket(listener->socketHandle);
                listener->socketHandle = INVALID_SOCKET;
            }
        }
        newListeners.clear();
    };
    for (const PrinterConfigEntry& printer : printers) {
        std::unique_ptr<ListenerContext> listener(new ListenerContext());
        listener->printer = printer;
        listener->socketHandle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listener->socketHandle == INVALID_SOCKET) {
            Log(UiText(L"Failed to create RAW listener socket.",
                       L"建立 RAW 監聽 socket 失敗。"));
            cleanupNewListeners();
            return false;
        }

        BOOL reuse = TRUE;
        ::setsockopt(listener->socketHandle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(static_cast<u_short>(printer.port));

        if (::bind(listener->socketHandle, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            Log(FormatString(UiText(L"Failed to bind RAW port %d: %s",
                                    L"綁定 RAW 連接埠 %d 失敗: %s").c_str(),
                             printer.port, FormatLastErrorMessage().c_str()));
            ::closesocket(listener->socketHandle);
            cleanupNewListeners();
            return false;
        }

        if (::listen(listener->socketHandle, SOMAXCONN) != 0) {
            Log(FormatString(UiText(L"Failed to listen on RAW port %d.",
                                    L"無法在 RAW 連接埠 %d 上開始監聽。").c_str(),
                             printer.port));
            ::closesocket(listener->socketHandle);
            cleanupNewListeners();
            return false;
        }

        newListeners.push_back(std::move(listener));
    }

    listeners_ = std::move(newListeners);
    stopRequested_ = false;
    running_ = true;

    for (std::unique_ptr<ListenerContext>& listener : listeners_) {
        listener->thread = std::thread(&PrinterServer::RawAcceptLoop, this, listener.get());
        Log(FormatString(UiText(L"Listening on RAW %d for printer: %s",
                                L"已開始監聽 RAW %d，印表機: %s").c_str(),
                         listener->printer.port, listener->printer.printerName.c_str()));
    }

    httpSocket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (httpSocket_ != INVALID_SOCKET) {
        BOOL reuse = TRUE;
        ::setsockopt(httpSocket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

        sockaddr_in httpAddr{};
        httpAddr.sin_family = AF_INET;
        httpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        httpAddr.sin_port = htons(static_cast<u_short>(webPort));
        if (::bind(httpSocket_, reinterpret_cast<sockaddr*>(&httpAddr), sizeof(httpAddr)) == 0 && ::listen(httpSocket_, SOMAXCONN) == 0) {
            httpThread_ = std::thread(&PrinterServer::HttpAcceptLoop, this);
            Log(FormatString(UiText(L"Web setup page: %s",
                                    L"網頁設定頁面: %s").c_str(),
                             GetWebUrl().c_str()));
        } else {
            Log(FormatString(UiText(L"Web server could not bind to port %d.",
                                    L"網頁伺服器無法綁定到連接埠 %d。").c_str(),
                             webPort));
            ::closesocket(httpSocket_);
            httpSocket_ = INVALID_SOCKET;
        }
    }

    Log(UiText(L"Server started.", L"伺服器已啟動。"));
    StartDriverPackagingThread();
    return true;
}

void PrinterServer::Stop() {
    const bool wasRunning = running_ || !listeners_.empty() || httpSocket_ != INVALID_SOCKET;
    stopRequested_ = true;
    running_ = false;

    if (httpSocket_ != INVALID_SOCKET) {
        ::closesocket(httpSocket_);
        httpSocket_ = INVALID_SOCKET;
    }
    if (httpThread_.joinable()) {
        httpThread_.join();
    }

    for (std::unique_ptr<ListenerContext>& listener : listeners_) {
        if (listener->socketHandle != INVALID_SOCKET) {
            ::closesocket(listener->socketHandle);
            listener->socketHandle = INVALID_SOCKET;
        }
    }
    for (std::unique_ptr<ListenerContext>& listener : listeners_) {
        if (listener->thread.joinable()) {
            listener->thread.join();
        }
    }
    listeners_.clear();

    if (driverThread_.joinable()) {
        driverThread_.join();
    }

    if (wasRunning) {
        Log(UiText(L"Server stopped.", L"伺服器已停止。"));
    }
}

bool PrinterServer::IsRunning() const {
    return running_;
}

std::wstring PrinterServer::GetWebUrl() const {
    const std::wstring ip = GetLocalIPv4Address();
    const int port = GetWebPort();
    if (port == 80) {
        return L"http://" + ip;
    }
    return L"http://" + ip + L":" + std::to_wstring(port);
}

bool PrinterServer::PrintRaw(const std::vector<unsigned char>& data, const std::wstring& printerName) const {
    HANDLE printerHandle = nullptr;
    if (!::OpenPrinterW(const_cast<LPWSTR>(printerName.c_str()), &printerHandle, nullptr)) {
        Log(FormatString(UiText(L"OpenPrinter failed for %s: %s",
                                L"\u958b\u555f\u5370\u8868\u6a5f %s \u5931\u6557\uff1a%s").c_str(),
                         printerName.c_str(), FormatLastErrorMessage().c_str()));
        return false;
    }

    DOC_INFO_1W docInfo{};
    docInfo.pDocName = const_cast<LPWSTR>(L"PrtEasyServer RAW Job");
    docInfo.pDatatype = const_cast<LPWSTR>(L"RAW");

    DWORD jobId = ::StartDocPrinterW(printerHandle, 1, reinterpret_cast<LPBYTE>(&docInfo));
    if (jobId == 0) {
        Log(FormatString(UiText(L"StartDocPrinter failed for %s: %s",
                                L"\u958b\u59cb\u5217\u5370\u5de5\u4f5c %s \u5931\u6557\uff1a%s").c_str(),
                         printerName.c_str(), FormatLastErrorMessage().c_str()));
        ::ClosePrinter(printerHandle);
        return false;
    }

    if (!::StartPagePrinter(printerHandle)) {
        Log(FormatString(UiText(L"StartPagePrinter failed for %s: %s",
                                L"\u958b\u59cb\u5217\u5370\u9801\u9762 %s \u5931\u6557\uff1a%s").c_str(),
                         printerName.c_str(), FormatLastErrorMessage().c_str()));
        ::EndDocPrinter(printerHandle);
        ::ClosePrinter(printerHandle);
        return false;
    }

    DWORD written = 0;
    const BOOL ok = ::WritePrinter(printerHandle,
                                   const_cast<unsigned char*>(data.data()),
                                   static_cast<DWORD>(data.size()),
                                   &written);

    ::EndPagePrinter(printerHandle);
    ::EndDocPrinter(printerHandle);
    ::ClosePrinter(printerHandle);

    if (!ok) {
        Log(FormatString(UiText(L"WritePrinter failed for %s: %s",
                                L"\u5beb\u5165\u5370\u8868\u6a5f %s \u5931\u6557\uff1a%s").c_str(),
                         printerName.c_str(), FormatLastErrorMessage().c_str()));
        return false;
    }

    Log(FormatString(UiText(L"Sent %lu bytes to printer: %s",
                            L"\u5df2\u50b3\u9001 %lu \u4f4d\u5143\u7d44\u5230\u5370\u8868\u6a5f\uff1a%s").c_str(),
                     written, printerName.c_str()));
    return true;
}

void PrinterServer::RawAcceptLoop(ListenerContext* listener) {
    while (!stopRequested_) {
        sockaddr_in clientAddr{};
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = ::accept(listener->socketHandle, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSocket == INVALID_SOCKET) {
            if (!stopRequested_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        std::thread(&PrinterServer::HandleRawClient, this, clientSocket, clientAddr, listener->printer).detach();
    }
}

void PrinterServer::HandleRawClient(SOCKET clientSocket, sockaddr_in clientAddress, PrinterConfigEntry printer) {
    char addressBuffer[64] = {};
    ::InetNtopA(AF_INET, &clientAddress.sin_addr, addressBuffer, sizeof(addressBuffer));
    const std::wstring clientLabel = Utf8ToWide(addressBuffer);
    Log(FormatString(UiText(L"Client connected: %s -> %s:%d",
                            L"\u7528\u6236\u7aef\u5df2\u9023\u7dda\uff1a%s -> %s:%d").c_str(),
                     clientLabel.c_str(),
                     printer.printerName.c_str(),
                     printer.port));

    std::vector<unsigned char> data;
    data.reserve(65536);
    unsigned char buffer[8192] = {};
    while (true) {
        const int received = ::recv(clientSocket, reinterpret_cast<char*>(buffer), sizeof(buffer), 0);
        if (received <= 0) {
            break;
        }
        data.insert(data.end(), buffer, buffer + received);
    }

    ::closesocket(clientSocket);

    if (data.empty()) {
        Log(FormatString(UiText(L"Connection closed with no data from %s.",
                                L"\u4f86\u81ea %s \u7684\u9023\u7dda\u672a\u50b3\u9001\u8cc7\u6599\u5c31\u5df2\u95dc\u9589\u3002").c_str(),
                         clientLabel.c_str()));
        return;
    }

    Log(FormatString(UiText(L"Received %u bytes (%S) for printer %s.",
                            L"\u5df2\u63a5\u6536 %u \u4f4d\u5143\u7d44\uff08%S\uff09\uff0c\u76ee\u6a19\u5370\u8868\u6a5f\uff1a%s\u3002").c_str(),
                     static_cast<unsigned int>(data.size()),
                     DetectRawDataDescription(data).c_str(),
                     printer.printerName.c_str()));

    PrintRaw(data, printer.printerName);
}

void PrinterServer::SendHttpResponse(SOCKET clientSocket,
                                     int statusCode,
                                     const std::string& statusText,
                                     const std::string& contentType,
                                     const std::string& body,
                                     const std::vector<std::pair<std::string, std::string>>& extraHeaders) const {
    std::ostringstream headers;
    headers << "HTTP/1.1 " << statusCode << ' ' << statusText << "\r\n";
    headers << BuildHttpHeaderLine("Content-Type", contentType);
    headers << BuildHttpHeaderLine("Content-Length", std::to_string(body.size()));
    headers << BuildHttpHeaderLine("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    headers << BuildHttpHeaderLine("Pragma", "no-cache");
    headers << BuildHttpHeaderLine("Expires", "0");
    for (const auto& header : extraHeaders) {
        headers << BuildHttpHeaderLine(header.first, header.second);
    }
    headers << "\r\n";
    const std::string headerText = headers.str();
    SendAll(clientSocket, headerText.data(), headerText.size());
    SendAll(clientSocket, body.data(), body.size());
}

void PrinterServer::SendHttpFile(SOCKET clientSocket,
                                 const std::wstring& path,
                                 const std::string& contentType,
                                 const std::string& downloadName) const {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        SendHttpResponse(clientSocket, 404, "Not Found", "text/plain; charset=utf-8", "Not found");
        return;
    }

    std::string data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    std::vector<std::pair<std::string, std::string>> headers;
    headers.push_back({"Content-Disposition", "attachment; filename=\"" + MakeAsciiDownloadName(Utf8ToWide(downloadName)) +
                                              "\"; filename*=UTF-8''" + UrlEncode(downloadName)});
    SendHttpResponse(clientSocket, 200, "OK", contentType, data, headers);
}

void PrinterServer::HttpAcceptLoop() {
    while (!stopRequested_ && httpSocket_ != INVALID_SOCKET) {
        sockaddr_in clientAddr{};
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = ::accept(httpSocket_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSocket == INVALID_SOCKET) {
            if (!stopRequested_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }
        std::thread(&PrinterServer::HandleHttpClient, this, clientSocket).detach();
    }
}

void PrinterServer::HandleHttpClient(SOCKET clientSocket) {
    const std::string request = ReadHttpRequest(clientSocket);
    if (request.empty()) {
        ::closesocket(clientSocket);
        return;
    }

    std::istringstream stream(request);
    std::string method;
    std::string path;
    std::string version;
    stream >> method >> path >> version;

    if (method != "GET") {
        SendHttpResponse(clientSocket, 405, "Method Not Allowed", "text/plain; charset=utf-8", "Only GET is supported");
        ::closesocket(clientSocket);
        return;
    }

    const std::vector<WebPrinterEntry> entries = BuildWebEntries();

    if (path == "/" || path == "/index.html") {
        const std::string body = RenderWebPage();
        SendHttpResponse(clientSocket, 200, "OK", "text/html; charset=utf-8", body);
        ::closesocket(clientSocket);
        return;
    }

    if (path.rfind("/download/", 0) == 0 && path.size() > 14) {
        const std::string token = path.substr(10);
        const std::size_t dot = token.find('.');
        const int index = std::atoi((dot == std::string::npos ? token : token.substr(0, dot)).c_str());
        for (const WebPrinterEntry& entry : entries) {
            if (entry.index == index) {
                const std::wstring batchName = SanitizeFileName(entry.printerName) + L"_" + std::to_wstring(entry.port) + L".bat";
                const std::string body = WideToUtf8(BuildInstallerBatchContent(entry));
                std::vector<std::pair<std::string, std::string>> headers;
                headers.push_back({"Content-Disposition", "attachment; filename=\"" + MakeAsciiDownloadName(batchName) +
                                                      "\"; filename*=UTF-8''" + UrlEncode(WideToUtf8(batchName))});
                SendHttpResponse(clientSocket, 200, "OK", "application/octet-stream", body, headers);
                ::closesocket(clientSocket);
                return;
            }
        }
    }

    if (path.rfind("/driver/", 0) == 0 && path.size() > 11) {
        const std::string token = path.substr(8);
        const std::size_t dot = token.find('.');
        const int index = std::atoi((dot == std::string::npos ? token : token.substr(0, dot)).c_str());
        for (const WebPrinterEntry& entry : entries) {
            if (entry.index == index) {
                std::wstring archivePath;
                std::wstring errorText;
                if (EnsureDriverArchive(entry, &archivePath, &errorText) && FileExists(archivePath)) {
                    SendHttpFile(clientSocket, archivePath, "application/zip", WideToUtf8(entry.driverArchiveName));
                } else {
                    SendHttpResponse(clientSocket, 404, "Not Found", "text/plain; charset=utf-8", WideToUtf8(errorText.empty() ? L"Driver package unavailable." : errorText));
                }
                ::closesocket(clientSocket);
                return;
            }
        }
    }

    SendHttpResponse(clientSocket, 404, "Not Found", "text/plain; charset=utf-8", "Not found");
    ::closesocket(clientSocket);
}

bool PrinterServer::QueryDriverManifest(const WebPrinterEntry& entry, DriverManifestInfo* info, std::wstring* errorText) const {
    if (info == nullptr) {
        return false;
    }

    HANDLE printerHandle = nullptr;
    if (!::OpenPrinterW(const_cast<LPWSTR>(entry.printerName.c_str()), &printerHandle, nullptr)) {
        if (errorText) {
            *errorText = L"OpenPrinter failed while reading driver details.";
        }
        return false;
    }

    DWORD needed = 0;
    ::GetPrinterDriverW(printerHandle, nullptr, 6, nullptr, 0, &needed);
    if (needed == 0) {
        ::ClosePrinter(printerHandle);
        if (errorText) {
            *errorText = L"GetPrinterDriver could not report the required buffer size.";
        }
        return false;
    }

    std::vector<unsigned char> buffer(needed);
    if (!::GetPrinterDriverW(printerHandle, nullptr, 6, buffer.data(), needed, &needed)) {
        ::ClosePrinter(printerHandle);
        if (errorText) {
            *errorText = L"GetPrinterDriver failed.";
        }
        return false;
    }

    DRIVER_INFO_6W* driver = reinterpret_cast<DRIVER_INFO_6W*>(buffer.data());
    DriverManifestInfo manifest;
    manifest.driverName = driver->pName ? driver->pName : entry.driverName;
    manifest.archiveName = entry.driverArchiveName;
    manifest.infPath = GetDriverInfPathFromRegistry(manifest.driverName, driver->pEnvironment ? driver->pEnvironment : L"Windows x64");

    if (manifest.infPath.empty()) {
        ::ClosePrinter(printerHandle);
        if (errorText) {
            *errorText = L"INF path for the printer driver was not found in the registry.";
        }
        return false;
    }

    manifest.sourceFolder = manifest.infPath.substr(0, manifest.infPath.find_last_of(L"\\/"));

    std::vector<std::wstring> files;
    auto addFile = [&files](const std::wstring& path) {
        const std::wstring trimmed = Trim(path);
        if (trimmed.empty() || !FileExists(trimmed)) {
            return;
        }
        if (std::find(files.begin(), files.end(), trimmed) == files.end()) {
            files.push_back(trimmed);
        }
    };

    addFile(manifest.infPath);
    addFile(driver->pDriverPath ? driver->pDriverPath : L"");
    addFile(driver->pDataFile ? driver->pDataFile : L"");
    addFile(driver->pConfigFile ? driver->pConfigFile : L"");
    addFile(driver->pHelpFile ? driver->pHelpFile : L"");
    for (const std::wstring& dependent : SplitMultiSz(driver->pDependentFiles)) {
        addFile(dependent);
    }

    ::ClosePrinter(printerHandle);

    if (files.empty()) {
        if (errorText) {
            *errorText = L"No driver files were collected for packaging.";
        }
        return false;
    }

    manifest.files = files;

    if (!IsDriverStoreDirectory(manifest.sourceFolder)) {
        const std::vector<std::wstring> fileHints = BuildFileHintsFromPaths(manifest.files);
        std::wstring driverStoreFolder;
        std::wstring driverStoreInfPath;
        if (FindBestDriverStorePackage(manifest.driverName, fileHints, &driverStoreFolder, &driverStoreInfPath)) {
            manifest.sourceFolder = driverStoreFolder;
            manifest.infPath = driverStoreInfPath;
        }
    }

    *info = manifest;
    return true;
}

std::wstring PrinterServer::GetDriverArchivePath(const std::wstring& driverName) const {
    return JoinPath(GetModuleDirectory(), SanitizeFileName(driverName) + L".zip");
}

bool PrinterServer::CreateDriverArchiveFromManifest(const DriverManifestInfo& info, const std::wstring& archivePath, std::wstring* errorText) const {
    const std::wstring tempRoot = JoinPath(GetEnvVar(L"TEMP").empty() ? L"C:\\Windows\\Temp" : GetEnvVar(L"TEMP"),
                                           L"PrtEasyServer_" + SanitizeToken(info.driverName) + L"_" + std::to_wstring(::GetTickCount()));
    const std::wstring tempFiles = JoinPath(tempRoot, L"files");
    EnsureDirectory(tempFiles);

    int duplicateIndex = 1;
    for (const std::wstring& sourcePath : info.files) {
        std::wstring fileName = sourcePath.substr(sourcePath.find_last_of(L"\\/") + 1);
        std::wstring destPath = JoinPath(tempFiles, fileName);
        while (FileExists(destPath)) {
            const std::size_t dot = fileName.find_last_of(L'.');
            if (dot == std::wstring::npos) {
                destPath = JoinPath(tempFiles, fileName + L"_" + std::to_wstring(duplicateIndex++));
            } else {
                destPath = JoinPath(tempFiles,
                                    fileName.substr(0, dot) + L"_" + std::to_wstring(duplicateIndex++) + fileName.substr(dot));
            }
        }
        if (!CopyFileOverwrite(sourcePath, destPath)) {
            if (errorText) {
                *errorText = L"CopyFile failed while preparing the driver package.";
            }
            DeleteDirectoryTree(tempRoot);
            return false;
        }
    }

    DeleteFileW(archivePath.c_str());

    std::wstring zipScript =
        L"$zipPath = " + QuoteForPowerShell(archivePath) + L"\n"
        L"$sourcePath = " + QuoteForPowerShell(tempFiles) + L"\n"
        L"if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }\n"
        L"[System.IO.File]::WriteAllBytes($zipPath, [byte[]](80,75,5,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))\n"
        L"$shell = New-Object -ComObject Shell.Application\n"
        L"$zipNs = $shell.NameSpace($zipPath)\n"
        L"$srcNs = $shell.NameSpace($sourcePath)\n"
        L"if (($null -eq $zipNs) -or ($null -eq $srcNs)) { throw 'Shell.Application ZIP namespace unavailable.' }\n"
        L"$expected = (Get-ChildItem -LiteralPath $sourcePath -File | Measure-Object).Count\n"
        L"$zipNs.CopyHere($srcNs.Items(), 16)\n"
        L"for ($i = 0; $i -lt 80; $i++) {\n"
        L"    Start-Sleep -Milliseconds 500\n"
        L"    if ($zipNs.Items().Count -ge $expected) { break }\n"
        L"}\n";

    DWORD exitCode = 0;
    std::wstring output;
    const bool ran = RunPowerShellScript(zipScript, &exitCode, &output);

    DeleteDirectoryTree(tempRoot);

    if (!ran || exitCode != 0 || !FileExists(archivePath)) {
        if (errorText) {
            *errorText = output.empty() ? L"PowerShell ZIP creation failed." : output;
        }
        return false;
    }
    return true;
}

bool PrinterServer::CreateDriverArchiveFromFolder(const std::wstring& sourceFolder, const std::wstring& archivePath, std::wstring* errorText) const {
    if (!DirectoryExists(sourceFolder)) {
        if (errorText) {
            *errorText = L"Driver source folder does not exist.";
        }
        return false;
    }

    const std::vector<std::wstring> files = EnumerateFilesRecursive(sourceFolder);
    if (files.empty()) {
        if (errorText) {
            *errorText = L"No files were found in the driver source folder.";
        }
        return false;
    }

    const std::wstring tempRoot = JoinPath(GetEnvVar(L"TEMP").empty() ? L"C:\\Windows\\Temp" : GetEnvVar(L"TEMP"),
                                           L"PrtEasyServer_pkg_" + std::to_wstring(::GetTickCount()));
    const std::wstring tempFiles = JoinPath(tempRoot, L"files");
    EnsureDirectory(tempFiles);

    for (const std::wstring& sourcePath : files) {
        std::wstring relativePath = sourcePath.substr(sourceFolder.size());
        while (!relativePath.empty() && (relativePath.front() == L'\\' || relativePath.front() == L'/')) {
            relativePath.erase(relativePath.begin());
        }
        if (relativePath.empty()) {
            continue;
        }

        const std::wstring destPath = JoinPath(tempFiles, relativePath);
        if (!CopyFileOverwrite(sourcePath, destPath)) {
            if (errorText) {
                *errorText = L"CopyFile failed while copying the full driver package folder.";
            }
            DeleteDirectoryTree(tempRoot);
            return false;
        }
    }

    DeleteFileW(archivePath.c_str());
    std::wstring zipScript =
        L"$zipPath = " + QuoteForPowerShell(archivePath) + L"\n"
        L"$sourcePath = " + QuoteForPowerShell(tempFiles) + L"\n"
        L"if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }\n"
        L"[System.IO.File]::WriteAllBytes($zipPath, [byte[]](80,75,5,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))\n"
        L"$shell = New-Object -ComObject Shell.Application\n"
        L"$zipNs = $shell.NameSpace($zipPath)\n"
        L"$srcNs = $shell.NameSpace($sourcePath)\n"
        L"if (($null -eq $zipNs) -or ($null -eq $srcNs)) { throw 'Shell.Application ZIP namespace unavailable.' }\n"
        L"$expected = (Get-ChildItem -LiteralPath $sourcePath -Recurse -File | Measure-Object).Count\n"
        L"$zipNs.CopyHere($srcNs.Items(), 16)\n"
        L"for ($i = 0; $i -lt 160; $i++) {\n"
        L"    Start-Sleep -Milliseconds 500\n"
        L"    if ((Get-ChildItem -LiteralPath $zipPath -ErrorAction SilentlyContinue | Measure-Object).Count -gt 0) {\n"
        L"        if ($zipNs.Items().Count -ge (Get-ChildItem -LiteralPath $sourcePath | Measure-Object).Count) { break }\n"
        L"    }\n"
        L"}\n";

    DWORD exitCode = 0;
    std::wstring output;
    const bool ran = RunPowerShellScript(zipScript, &exitCode, &output);

    DeleteDirectoryTree(tempRoot);

    if (!ran || exitCode != 0 || !FileExists(archivePath)) {
        if (errorText) {
            *errorText = output.empty() ? L"PowerShell ZIP creation failed for the driver folder." : output;
        }
        return false;
    }
    return true;
}

bool PrinterServer::EnsureDriverArchive(const WebPrinterEntry& entry, std::wstring* archivePath, std::wstring* errorText) {
    std::lock_guard<std::mutex> lock(driverMutex_);
    const std::wstring finalPath = entry.driverArchivePath.empty() ? GetDriverArchivePath(entry.driverName) : entry.driverArchivePath;
    if (archivePath) {
        *archivePath = finalPath;
    }
    if (FileExists(finalPath)) {
        return true;
    }

    DriverManifestInfo info;
    std::wstring queryError;
    if (!QueryDriverManifest(entry, &info, &queryError)) {
        if (errorText) {
            *errorText = queryError;
        }
        return false;
    }

    std::wstring createError;
    const bool preferFullFolder = !info.sourceFolder.empty() &&
                                  DirectoryExists(info.sourceFolder) &&
                                  !IsWindowsInfDirectory(info.sourceFolder);

    bool ok = false;
    if (preferFullFolder) {
        ok = CreateDriverArchiveFromFolder(info.sourceFolder, finalPath, &createError);
        if (!ok && IsDriverStoreDirectory(info.sourceFolder)) {
            // DriverStore folder should have been enough; fall back only as last resort.
            std::wstring manifestError;
            if (CreateDriverArchiveFromManifest(info, finalPath, &manifestError)) {
                ok = true;
            } else if (createError.empty()) {
                createError = manifestError;
            }
        }
    } else {
        ok = CreateDriverArchiveFromManifest(info, finalPath, &createError);
    }

    if (!ok) {
        if (errorText) {
            *errorText = createError;
        }
        return false;
    }
    return true;
}

void PrinterServer::StartDriverPackagingThread() {
    if (driverThread_.joinable()) {
        driverThread_.join();
    }
    driverThread_ = std::thread(&PrinterServer::DriverPackagingWorker, this);
}

void PrinterServer::DriverPackagingWorker() {
    std::vector<WebPrinterEntry> entries = BuildWebEntries();
    std::vector<std::wstring> seen;
    for (const WebPrinterEntry& entry : entries) {
        if (stopRequested_) {
            return;
        }
        if (std::find(seen.begin(), seen.end(), entry.driverArchiveName) != seen.end()) {
            continue;
        }
        seen.push_back(entry.driverArchiveName);

        std::wstring archivePath;
        std::wstring errorText;
        if (EnsureDriverArchive(entry, &archivePath, &errorText)) {
            Log(FormatString(UiText(L"Driver package ready: %s",
                                    L"驅動封裝完成: %s").c_str(),
                             archivePath.c_str()));
        } else {
            Log(FormatString(UiText(L"Driver package skipped for %s: %s",
                                    L"略過驅動封裝 %s: %s").c_str(),
                             entry.driverName.c_str(), errorText.c_str()));
        }
    }
}

std::wstring PrinterServer::BuildInstallerBatchContent(const WebPrinterEntry& entry) const {
    const AppConfig config = GetConfigCopy();
    LocalizationManager localizer;
    localizer.Initialize(config.language);

    const auto escapePsLiteral = [](const std::wstring& value) {
        return ReplaceAll(value, L"'", L"''");
    };

    DriverManifestInfo manifest;
    std::wstring manifestError;
    std::wstring infName;
    if (QueryDriverManifest(entry, &manifest, &manifestError)) {
        infName = manifest.infPath.substr(manifest.infPath.find_last_of(L"\\/") + 1);
    }

    std::wstring psScriptTemplate;
    psScriptTemplate.reserve(24000);
    psScriptTemplate += LR"PS(
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
Add-Type -AssemblyName System.Windows.Forms
$printerName = '__PRINTER_NAME__'
$requestedDriverName = '__DRIVER_NAME__'
$driverName = '__DRIVER_NAME__'
$portName = '__PORT_NAME__'
$hostName = '__HOST_NAME__'
$hostIp = '__HOST_IP__'
$portNumber = __PORT_NUMBER__
$driverArchiveName = '__DRIVER_ARCHIVE_NAME__'
$driverInfName = '__DRIVER_INF_NAME__'
$successTitle = '__INSTALLER_SUCCESS_TITLE__'
$successMessage = '__INSTALLER_SUCCESS_MESSAGE__'
$missingTitle = '__INSTALLER_MISSING_TITLE__'
$missingMessage = '__INSTALLER_MISSING_MESSAGE__'
$queueFailedTitle = '__INSTALLER_QUEUE_FAILED_TITLE__'
$queueFailedMessage = '__INSTALLER_QUEUE_FAILED_MESSAGE__'
$extractArchiveError = '__INSTALLER_ERROR_EXTRACT_ARCHIVE__'
$createPortError = '__INSTALLER_ERROR_CREATE_PORT__'
$scriptDir = $env:SCRIPT_DIR
if (($null -eq $scriptDir) -or ([string]$scriptDir).Trim() -eq '') {
    $scriptDir = (Get-Location).Path
}
$driverArchivePath = Join-Path -Path $scriptDir -ChildPath $driverArchiveName
$driverTempRoot = Join-Path -Path $env:TEMP -ChildPath ('PrtEasyServer_' + [System.IO.Path]::GetFileNameWithoutExtension($driverArchiveName))
function Test-Command($name) {
    return [bool](Get-Command -Name $name -ErrorAction SilentlyContinue)
}
function Escape-WqlValue($value) {
    if ($null -eq $value) { return '' }
    return ([string]$value).Replace("'", "''")
}
function Get-PrintingAdminScript($scriptName) {
    $baseFolder = Join-Path -Path $env:WINDIR -ChildPath 'System32/Printing_Admin_Scripts'
    $candidates = @()
    foreach ($language in @($PSUICulture, $PSCulture, 'en-US')) {
        if (($null -ne $language) -and ([string]$language).Trim() -ne '') {
            $candidates += (Join-Path -Path $baseFolder -ChildPath ($language + '/' + $scriptName))
        }
    }
    $candidates += (Join-Path -Path $baseFolder -ChildPath $scriptName)
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }
    return $null
}
$targetHost = $hostName
try { [System.Net.Dns]::GetHostAddresses($hostName) | Out-Null } catch { $targetHost = $hostIp }
function Get-InstalledPrinterPort($targetPortName) {
    if (Test-Command 'Get-PrinterPort') {
        return Get-PrinterPort -Name $targetPortName -ErrorAction SilentlyContinue
    }
    $escaped = Escape-WqlValue $targetPortName
    return Get-WmiObject -Class Win32_TCPIPPrinterPort -Filter ('Name=''' + $escaped + '''') -ErrorAction SilentlyContinue
}
function Ensure-PrinterPort($targetPortName, $targetHostName, $targetPortNumber) {
    if (Get-InstalledPrinterPort $targetPortName) {
        return $true
    }
    if (Test-Command 'Add-PrinterPort') {
        Add-PrinterPort -Name $targetPortName -PrinterHostAddress $targetHostName -PortNumber $targetPortNumber
        return [bool](Get-InstalledPrinterPort $targetPortName)
    }
    $prnport = Get-PrintingAdminScript 'prnport.vbs'
    if (-not $prnport) {
        return $false
    }
    $portArgs = @('//nologo', $prnport, '-a', '-r', $targetPortName, '-h', $targetHostName, '-o', 'raw', '-n', [string]$targetPortNumber)
    $portProcess = Start-Process -FilePath 'cscript.exe' -ArgumentList $portArgs -PassThru -Wait -WindowStyle Hidden
    if ($portProcess.ExitCode -eq 0) { Start-Sleep -Milliseconds 500 }
    return [bool](Get-InstalledPrinterPort $targetPortName)
}
function Get-InstalledDriver($targetName) {
    if (($null -eq $targetName) -or ([string]$targetName).Trim() -eq '') { return $null }
    if (Test-Command 'Get-PrinterDriver') {
        $matched = Get-PrinterDriver -Name $targetName -ErrorAction SilentlyContinue
        if (-not $matched) {
            $matched = Get-PrinterDriver -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -eq $targetName -or $_.Name -like ($targetName + '*') -or $targetName -like ($_.Name + '*') } |
                Select-Object -First 1
        }
        if ($matched) { return $matched }
    }
    $drivers = Get-WmiObject -Class Win32_PrinterDriver -ErrorAction SilentlyContinue
    if (-not $drivers) { return $null }
    return $drivers |
        Where-Object { $_.Name -eq $targetName -or $_.Name -like ($targetName + '*') -or $targetName -like ($_.Name + '*') } |
        Select-Object -First 1
}
)PS";
    psScriptTemplate += LR"PS(
function Get-InstalledPrinter($targetPrinterName) {
    if (Test-Command 'Get-Printer') {
        return Get-Printer -Name $targetPrinterName -ErrorAction SilentlyContinue
    }
    $escaped = Escape-WqlValue $targetPrinterName
    return Get-WmiObject -Class Win32_Printer -Filter ('Name=''' + $escaped + '''') -ErrorAction SilentlyContinue
}
function Expand-ZipCompat($zipPath, $destinationPath) {
    if (Test-Path -LiteralPath $destinationPath) {
        Remove-Item -LiteralPath $destinationPath -Recurse -Force
    }
    New-Item -ItemType Directory -Path $destinationPath -Force | Out-Null
    if (Test-Command 'Expand-Archive') {
        Expand-Archive -LiteralPath $zipPath -DestinationPath $destinationPath -Force
        return
    }
    try {
        Add-Type -AssemblyName System.IO.Compression.FileSystem -ErrorAction Stop
        [System.IO.Compression.ZipFile]::ExtractToDirectory($zipPath, $destinationPath)
        return
    } catch {
    }
    $shell = New-Object -ComObject Shell.Application
    $archiveNamespace = $shell.NameSpace($zipPath)
    $destinationNamespace = $shell.NameSpace($destinationPath)
    if ($archiveNamespace -and $destinationNamespace) {
        $destinationNamespace.CopyHere($archiveNamespace.Items(), 16)
        Start-Sleep -Seconds 2
        return
    }
    throw $extractArchiveError
}
function Install-DriverFromInf($infPath, $modelName) {
    $printUiArgs = @(
        'printui.dll,PrintUIEntry',
        '/ia',
        ('/m "{0}"' -f $modelName),
        ('/f "{0}"' -f $infPath),
        '/q'
    )
    $installProcess = Start-Process -FilePath 'rundll32.exe' -ArgumentList $printUiArgs -PassThru -Wait -WindowStyle Hidden
    if ($installProcess.ExitCode -eq 0) { Start-Sleep -Milliseconds 800 }
    return Get-InstalledDriver $modelName
}
function Install-PrinterFromInf($infPath, $modelName, $targetPrinterName, $targetPortName) {
    $printUiArgs = @(
        'printui.dll,PrintUIEntry',
        '/if',
        ('/b "{0}"' -f $targetPrinterName),
        ('/f "{0}"' -f $infPath),
        ('/r "{0}"' -f $targetPortName),
        ('/m "{0}"' -f $modelName),
        '/z',
        '/q'
    )
    $installProcess = Start-Process -FilePath 'rundll32.exe' -ArgumentList $printUiArgs -PassThru -Wait -WindowStyle Hidden
    if ($installProcess.ExitCode -eq 0) { Start-Sleep -Milliseconds 1200 }
    return Get-InstalledPrinter $targetPrinterName
}
function Install-DriverPackage($rootPath, $expectedInfName, $modelName) {
    $candidateInfs = @()
    if (($null -ne $expectedInfName) -and ([string]$expectedInfName).Trim() -ne '') {
        foreach ($item in (Get-ChildItem -LiteralPath $rootPath -Filter $expectedInfName -Recurse -ErrorAction SilentlyContinue | Where-Object { -not $_.PSIsContainer })) {
            if ($candidateInfs -notcontains $item.FullName) { $candidateInfs += $item.FullName }
        }
    }
    foreach ($item in (Get-ChildItem -LiteralPath $rootPath -Filter '*.inf' -Recurse -ErrorAction SilentlyContinue | Where-Object { -not $_.PSIsContainer })) {
        if ($candidateInfs -notcontains $item.FullName) { $candidateInfs += $item.FullName }
    }
    foreach ($infPath in $candidateInfs) {
        try {
            $resolved = Install-DriverFromInf $infPath $modelName
            if ($resolved) { return (New-Object PSObject -Property @{ Name = $resolved.Name; InfPath = $infPath }) }
        } catch {
        }
    }
    return $null
}
)PS";
    psScriptTemplate += LR"PS(
function Install-PrinterPackage($rootPath, $expectedInfName, $modelNames, $targetPrinterName, $targetPortName) {
    $candidateInfs = @()
    if (($null -ne $expectedInfName) -and ([string]$expectedInfName).Trim() -ne '') {
        foreach ($item in (Get-ChildItem -LiteralPath $rootPath -Filter $expectedInfName -Recurse -ErrorAction SilentlyContinue | Where-Object { -not $_.PSIsContainer })) {
            if ($candidateInfs -notcontains $item.FullName) { $candidateInfs += $item.FullName }
        }
    }
    foreach ($item in (Get-ChildItem -LiteralPath $rootPath -Filter '*.inf' -Recurse -ErrorAction SilentlyContinue | Where-Object { -not $_.PSIsContainer })) {
        if ($candidateInfs -notcontains $item.FullName) { $candidateInfs += $item.FullName }
    }
    foreach ($infPath in $candidateInfs) {
        foreach ($modelName in $modelNames) {
            if (($null -eq $modelName) -or ([string]$modelName).Trim() -eq '') { continue }
            try {
                $installedPrinter = Install-PrinterFromInf $infPath $modelName $targetPrinterName $targetPortName
                if ($installedPrinter) { return $installedPrinter }
            } catch {
            }
        }
    }
    return $null
}
function Ensure-PrinterQueue($targetPrinterName, $targetDriverName, $targetPortName) {
    if (Get-InstalledPrinter $targetPrinterName) { return $true }
    if (Test-Command 'Add-Printer') {
        Add-Printer -Name $targetPrinterName -DriverName $targetDriverName -PortName $targetPortName
        return [bool](Get-InstalledPrinter $targetPrinterName)
    }
    $prnmngr = Get-PrintingAdminScript 'prnmngr.vbs'
    if (-not $prnmngr) { return $false }
    $printerArgs = @('//nologo', $prnmngr, '-a', '-p', $targetPrinterName, '-m', $targetDriverName, '-r', $targetPortName)
    $printerProcess = Start-Process -FilePath 'cscript.exe' -ArgumentList $printerArgs -PassThru -Wait -WindowStyle Hidden
    if ($printerProcess.ExitCode -eq 0) { Start-Sleep -Milliseconds 800 }
    return [bool](Get-InstalledPrinter $targetPrinterName)
}
if (-not (Ensure-PrinterPort $portName $targetHost $portNumber)) {
    throw ($createPortError + $portName)
}
$packageExpanded = $false
$driverExists = Get-InstalledDriver $driverName
if ($driverExists) { $driverName = $driverExists.Name }
if ((-not $driverExists) -and (Test-Path -LiteralPath $driverArchivePath)) {
    try {
        Expand-ZipCompat $driverArchivePath $driverTempRoot
        $packageExpanded = $true
        $installedDriver = Install-DriverPackage $driverTempRoot $driverInfName $requestedDriverName
        if ($installedDriver) {
            $driverExists = Get-InstalledDriver $installedDriver.Name
            if ($driverExists) { $driverName = $driverExists.Name }
        }
    } catch {
    }
}
$queueCreated = $false
if ($driverExists) {
    $queueCreated = Ensure-PrinterQueue $printerName $driverName $portName
}
)PS";
    psScriptTemplate += LR"PS(
if ((-not $queueCreated) -and (Test-Path -LiteralPath $driverArchivePath)) {
    if (-not $packageExpanded) {
        try {
            Expand-ZipCompat $driverArchivePath $driverTempRoot
            $packageExpanded = $true
        } catch {
        }
    }
}
if ((-not $queueCreated) -and $packageExpanded) {
    $modelNames = @()
    foreach ($candidateName in @($driverName, $requestedDriverName)) {
        if (($null -ne $candidateName) -and ([string]$candidateName).Trim() -ne '' -and ($modelNames -notcontains $candidateName)) {
            $modelNames += $candidateName
        }
    }
    try {
        $installedPrinter = Install-PrinterPackage $driverTempRoot $driverInfName $modelNames $printerName $portName
        if ($installedPrinter) { $queueCreated = $true }
    } catch {
    }
}
if (Test-Path -LiteralPath $driverTempRoot) {
    try { Remove-Item -LiteralPath $driverTempRoot -Recurse -Force } catch {}
}
$openPrintersFolder = {
    try {
        $shell = New-Object -ComObject Shell.Application
        $shell.Open('shell:PrintersFolder')
    } catch {
        Start-Process explorer.exe -ArgumentList 'shell:PrintersFolder'
    }
}
$printerInstalled = Get-InstalledPrinter $printerName
if ((-not $printerInstalled) -and $queueCreated) {
    $printerInstalled = Get-InstalledPrinter $printerName
}
if ($printerInstalled) {
    [System.Windows.Forms.MessageBox]::Show(
        $successMessage,
        $successTitle,
        [System.Windows.Forms.MessageBoxButtons]::OK,
        [System.Windows.Forms.MessageBoxIcon]::Information
    ) | Out-Null
    & $openPrintersFolder
} else {
    $manualTitle = $missingTitle
    $manualMessage = $missingMessage
    if ($driverExists) {
        $manualTitle = $queueFailedTitle
        $manualMessage = $queueFailedMessage
    }
    [System.Windows.Forms.MessageBox]::Show(
        $manualMessage,
        $manualTitle,
        [System.Windows.Forms.MessageBoxButtons]::OK,
        [System.Windows.Forms.MessageBoxIcon]::Warning
    ) | Out-Null
    Start-Process rundll32.exe -ArgumentList 'printui.dll,PrintUIEntry /il'
}
)PS";

    std::wstring psScript = psScriptTemplate;
    psScript = ReplaceAll(psScript, L"__INSTALLER_SUCCESS_TITLE__", escapePsLiteral(localizer.Get(L"installer_success_title")));
    psScript = ReplaceAll(psScript, L"__INSTALLER_SUCCESS_MESSAGE__", escapePsLiteral(localizer.Get(L"installer_success_message")));
    psScript = ReplaceAll(psScript, L"__INSTALLER_MISSING_TITLE__", escapePsLiteral(localizer.Get(L"installer_missing_title")));
    psScript = ReplaceAll(psScript, L"__INSTALLER_MISSING_MESSAGE__", escapePsLiteral(localizer.Get(L"installer_missing_message")));
    psScript = ReplaceAll(psScript, L"__INSTALLER_QUEUE_FAILED_TITLE__", escapePsLiteral(localizer.Get(L"installer_queue_failed_title")));
    psScript = ReplaceAll(psScript, L"__INSTALLER_QUEUE_FAILED_MESSAGE__", escapePsLiteral(localizer.Get(L"installer_queue_failed_message")));
    psScript = ReplaceAll(psScript, L"__INSTALLER_ERROR_EXTRACT_ARCHIVE__", escapePsLiteral(localizer.Get(L"installer_error_extract_archive")));
    psScript = ReplaceAll(psScript, L"__INSTALLER_ERROR_CREATE_PORT__", escapePsLiteral(localizer.Get(L"installer_error_create_port")));
    psScript = ReplaceAll(psScript, L"__PRINTER_NAME__", ReplaceAll(entry.printerName, L"'", L"''"));
    psScript = ReplaceAll(psScript, L"__DRIVER_NAME__", ReplaceAll(entry.driverName, L"'", L"''"));
    psScript = ReplaceAll(psScript, L"__PORT_NAME__", ReplaceAll(entry.tcpPortName, L"'", L"''"));
    psScript = ReplaceAll(psScript, L"__HOST_NAME__", ReplaceAll(entry.hostName, L"'", L"''"));
    psScript = ReplaceAll(psScript, L"__HOST_IP__", ReplaceAll(entry.hostIp, L"'", L"''"));
    psScript = ReplaceAll(psScript, L"__PORT_NUMBER__", std::to_wstring(entry.port));
    psScript = ReplaceAll(psScript, L"__DRIVER_ARCHIVE_NAME__", ReplaceAll(entry.driverArchiveName, L"'", L"''"));
    psScript = ReplaceAll(psScript, L"__DRIVER_INF_NAME__", ReplaceAll(infName, L"'", L"''"));

    std::wostringstream batch;
    batch << L"@echo off\r\n";
    batch << L"rem " << kAppDisplayName << L" readable installer\r\n";
    batch << L"setlocal\r\n\r\n";
    batch << L"set \"SCRIPT_DIR=%~dp0\"\r\n";
    batch << L"set \"SELF=%~f0\"\r\n";
    batch << L"set \"PS1OUT=%TEMP%\\PrtEasyServer_Setup_%RANDOM%_%RANDOM%.ps1\"\r\n\r\n";
    batch << L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"$prefix = '#PS1# '; $lines = [System.IO.File]::ReadAllLines($env:SELF, [System.Text.Encoding]::UTF8); $ps = foreach ($line in $lines) { if ($line.StartsWith($prefix)) { $line.Substring($prefix.Length) } }; $utf8Bom = New-Object System.Text.UTF8Encoding -ArgumentList $true; [System.IO.File]::WriteAllLines($env:PS1OUT, [string[]]$ps, $utf8Bom)\"\r\n";
    batch << L"if errorlevel 1 (\r\n";
    batch << L"    echo " << localizer.Get(L"installer_extract_script_failed") << L"\r\n";
    batch << L"    pause\r\n";
    batch << L"    exit /b 1\r\n";
    batch << L")\r\n\r\n";
    batch << L"powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"%PS1OUT%\"\r\n";
    batch << L"set \"ERR=%ERRORLEVEL%\"\r\n\r\n";
    batch << L"del \"%PS1OUT%\" >nul 2>nul\r\n\r\n";
    batch << L"if not \"%ERR%\"==\"0\" (\r\n";
    batch << L"    echo " << localizer.Get(L"installer_batch_failed") << L"\r\n";
    batch << L"    pause\r\n";
    batch << L"    exit /b %ERR%\r\n";
    batch << L")\r\n\r\n";
    batch << L"endlocal\r\n";
    batch << L"exit /b 0\r\n\r\n";
    batch << L"# POWERSHELL_START - plain UTF-8 PowerShell lines follow\r\n";

    std::wistringstream lines(psScript);
    std::wstring line;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == L'\r') {
            line.pop_back();
        }
        batch << L"#PS1# " << line << L"\r\n";
    }

    return batch.str();
}

std::string PrinterServer::RenderWebPage() const {
    const AppConfig config = GetConfigCopy();
    LocalizationManager localizer;
    localizer.Initialize(config.language);

    const std::vector<WebPrinterEntry> entries = BuildWebEntries();
    const bool traditionalChinese = (localizer.GetCurrentLanguage() == L"tw");
    const std::string htmlLang = traditionalChinese ? "zh-Hant" : "en";
    const std::wstring hostNameWide = GetComputerNameShort();
    const std::wstring hostIpWide = GetLocalIPv4Address();
    const int webPort = GetWebPort();
    const std::wstring webEntryWide = webPort == 80 ? hostIpWide : (hostIpWide + L":" + std::to_wstring(webPort));
    const std::string hostName = HtmlEscape(WideToUtf8(hostNameWide));
    const std::string hostIp = HtmlEscape(WideToUtf8(hostIpWide));
    const std::string webEntry = HtmlEscape(WideToUtf8(webEntryWide));
    const std::string pageTitle = HtmlEscape(WideToUtf8(localizer.Get(L"web_title")));
    const std::string eyebrow = HtmlEscape(WideToUtf8(localizer.Get(L"web_eyebrow")));
    const std::string heading = HtmlEscape(WideToUtf8(localizer.Get(L"web_heading")));
    const std::string introServerPrefix = HtmlEscape(WideToUtf8(localizer.Get(L"web_intro_server_prefix")));
    const std::string introServerSuffix = HtmlEscape(WideToUtf8(localizer.Get(L"web_intro_server_suffix")));
    const std::string introDownload = HtmlEscape(WideToUtf8(localizer.Get(L"web_intro_download")));
    const std::string introFolder = HtmlEscape(WideToUtf8(localizer.Get(L"web_intro_folder")));
    const std::string manualPrefix = HtmlEscape(WideToUtf8(localizer.Get(L"web_manual_prefix")));
    const std::string manualOr = HtmlEscape(WideToUtf8(localizer.Get(L"web_manual_or")));
    const std::string manualRecommend = HtmlEscape(WideToUtf8(localizer.Get(L"web_manual_recommend")));
    const std::string accessLabel = HtmlEscape(WideToUtf8(localizer.Get(L"web_access_url")));
    const std::string serverHostLabel = HtmlEscape(WideToUtf8(localizer.Get(L"web_server_host")));
    const std::string countLabel = HtmlEscape(WideToUtf8(localizer.Get(L"web_available_count")));
    const std::string rawPortLabel = HtmlEscape(WideToUtf8(localizer.Get(L"web_raw_port")));
    const std::string downloadSetupLabel = HtmlEscape(WideToUtf8(localizer.Get(L"web_download_setup")));
    const std::string downloadDriverLabel = HtmlEscape(WideToUtf8(localizer.Get(L"web_download_driver")));
    const std::string footerNote = HtmlEscape(WideToUtf8(localizer.Get(L"web_footer_note")));
    const std::string footerVersion = HtmlEscape(WideToUtf8(localizer.Get(L"web_footer_version")));
    const std::string footerLatest = HtmlEscape(WideToUtf8(localizer.Get(L"web_footer_latest")));
    const std::string emptyTitle = HtmlEscape(WideToUtf8(localizer.Get(L"web_empty_title")));
    const std::string emptyBody = HtmlEscape(WideToUtf8(localizer.Get(L"web_empty_body")));
    const std::string versionText = HtmlEscape(WideToUtf8(std::wstring(L"V") + kAppVersion));
    const std::string githubUrl = "https://github.com/Terence0816/Windows-PrtEasyServer";

    std::ostringstream cards;
    if (entries.empty()) {
        cards << "<section class=\"card\"><h2>" << emptyTitle << "</h2><p>" << emptyBody << "</p></section>";
    } else {
        for (const WebPrinterEntry& entry : entries) {
            const std::wstring badgeText = FormatString(localizer.Get(L"web_printer_badge").c_str(), entry.index);
            cards << "<section class=\"card\">";
            cards << "<div class=\"badge\">" << HtmlEscape(WideToUtf8(badgeText)) << "</div>";
            cards << "<h2>" << HtmlEscape(WideToUtf8(entry.printerName)) << "</h2>";
            cards << "<p class=\"raw-line\"><strong>" << rawPortLabel << ":</strong> <span class=\"pill-inline\">" << entry.port << "</span></p>";
            cards << "<div class=\"actions\">";
            cards << "<a class=\"button-link\" href=\"/download/" << entry.index << ".bat\">" << downloadSetupLabel << "</a>";
            cards << "<a class=\"button-link secondary\" href=\"/driver/" << entry.index << ".zip\">" << downloadDriverLabel << "</a>";
            cards << "</div></section>";
        }
    }

    std::ostringstream html;
    html << "<!DOCTYPE html><html lang=\"" << htmlLang << "\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html << "<title>" << pageTitle << "</title>";
    html << "<style>"
            "body{margin:0;font-family:'Segoe UI',Arial,sans-serif;color:#173127;background:linear-gradient(150deg,#eff7ea,#f7fbff);}"
            ".page{max-width:1120px;margin:0 auto;padding:16px 0 28px;}"
            ".hero,.card{background:rgba(255,255,255,.94);border:1px solid #d7e1d4;border-radius:24px;box-shadow:0 16px 40px rgba(21,54,39,.12);}"
            ".hero{padding:28px 30px 26px;margin-bottom:26px;}"
            ".hero h1{margin:18px 0 18px;font-size:58px;line-height:1.08;letter-spacing:.01em;}"
            ".hero p{margin:0 0 12px;color:#527060;line-height:1.9;font-size:16px;}"
            ".pill-inline,.summary-pill,.badge{display:inline-flex;align-items:center;padding:6px 14px;border-radius:999px;background:rgba(45,133,89,.10);border:1px solid #cfe0d2;color:#14533d;}"
            ".badge{font-weight:700;margin-bottom:10px;}"
            ".summary-row{display:flex;flex-wrap:wrap;gap:10px;margin-top:18px;}"
            ".summary-pill{background:rgba(255,255,255,.92);box-shadow:0 6px 14px rgba(21,54,39,.08);color:#234c39;}"
            ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(320px,1fr));gap:18px;}"
            ".card{padding:22px 22px 20px;}"
            ".card h2{margin:8px 0 14px;font-size:26px;line-height:1.3;}"
            ".card p{margin:0;color:#234c39;font-size:16px;line-height:1.8;}"
            ".raw-line{margin-top:8px;}"
            "a{color:#0f6c8b;}"
            ".button-link{display:flex;justify-content:center;align-items:center;text-decoration:none;font-weight:700;padding:14px 16px;border-radius:14px;background:#267848;color:#fff;box-sizing:border-box;}"
            ".button-link.secondary{background:#4ca067;}"
            ".actions{display:grid;gap:10px;margin-top:16px;}"
            ".footer{padding:18px 4px 0;color:#4f6d5e;font-size:15px;line-height:1.9;}"
            ".footer p{margin:0 0 4px;}"
            ".footer a{color:#0f6c8b;text-decoration:underline;font-weight:600;}"
            "@media (max-width:720px){.page{padding:12px 12px 24px;}.hero{padding:22px 20px;}.hero h1{font-size:40px;}.grid{grid-template-columns:1fr;}}"
            "</style></head><body><main class=\"page\">";
    html << "<section class=\"hero\"><div class=\"badge\">" << eyebrow << "</div><h1>" << heading << "</h1>";
    html << "<p>" << introServerPrefix << " <span class=\"pill-inline\">" << hostName << "</span> " << introServerSuffix << "</p>";
    html << "<p>" << introDownload << "</p>";
    html << "<p>" << introFolder << "</p>";
    html << "<p>" << manualPrefix << " <span class=\"pill-inline\">" << hostName << "</span> " << manualOr << " <span class=\"pill-inline\">" << hostIp << "</span></p>";
    html << "<p>" << manualRecommend << "</p>";
    html << "<div class=\"summary-row\">";
    html << "<span class=\"summary-pill\">" << serverHostLabel << ": " << hostName << "</span>";
    html << "<span class=\"summary-pill\">" << accessLabel << ": " << webEntry << "</span>";
    html << "<span class=\"summary-pill\">" << countLabel << ": " << entries.size() << "</span>";
    html << "</div>";
    html << "</section>";
    html << "<section class=\"grid\">" << cards.str() << "</section>";
    html << "<section class=\"footer\">";
    html << "<p>" << footerNote << "</p>";
    html << "<p>" << footerVersion << " " << versionText << " " << footerLatest << " <a href=\"" << githubUrl << "\">" << githubUrl << "</a></p>";
    html << "</section></main></body></html>";
    return html.str();
}

bool PrinterServer::TestConnection(const std::wstring& host, int port, const std::string* payload, std::wstring* resultLog) {
    addrinfoW hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfoW* results = nullptr;
    const std::wstring portText = std::to_wstring(port);
    if (::GetAddrInfoW(host.c_str(), portText.c_str(), &hints, &results) != 0) {
        if (resultLog) {
            *resultLog = L"DNS or address resolution failed.";
        }
        return false;
    }

    SOCKET client = INVALID_SOCKET;
    bool connected = false;
    for (addrinfoW* item = results; item != nullptr; item = item->ai_next) {
        client = ::socket(item->ai_family, item->ai_socktype, item->ai_protocol);
        if (client == INVALID_SOCKET) {
            continue;
        }
        if (::connect(client, item->ai_addr, static_cast<int>(item->ai_addrlen)) == 0) {
            connected = true;
            break;
        }
        ::closesocket(client);
        client = INVALID_SOCKET;
    }
    ::FreeAddrInfoW(results);

    if (!connected || client == INVALID_SOCKET) {
        if (resultLog) {
            *resultLog = L"Connection failed.";
        }
        return false;
    }

    if (payload != nullptr && !payload->empty()) {
        if (!SendAll(client, payload->data(), payload->size())) {
            ::closesocket(client);
            if (resultLog) {
                *resultLog = L"Connected, but sending data failed.";
            }
            return false;
        }
        if (resultLog) {
            *resultLog = FormatString(L"Connected and sent %u bytes.", static_cast<unsigned int>(payload->size()));
        }
    } else if (resultLog) {
        *resultLog = L"TCP connection successful.";
    }

    ::closesocket(client);
    return true;
}

bool PrinterServer::AddToStartup(std::wstring* message) const {
    HKEY keyHandle = nullptr;
    const LONG status = ::RegOpenKeyExW(HKEY_CURRENT_USER,
                                        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                                        0,
                                        KEY_SET_VALUE,
                                        &keyHandle);
    if (status != ERROR_SUCCESS) {
        if (message) {
            *message = L"Unable to open the Run registry key.";
        }
        return false;
    }

    const std::wstring command = L"\"" + GetExecutablePath() + L"\" --autostart";
    for (const std::wstring& valueName : GetStartupValueNames()) {
        if (valueName != kStartupValueName) {
            ::RegDeleteValueW(keyHandle, valueName.c_str());
        }
    }

    const LONG writeStatus = ::RegSetValueExW(keyHandle,
                                              kStartupValueName,
                                              0,
                                              REG_SZ,
                                              reinterpret_cast<const BYTE*>(command.c_str()),
                                              static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    ::RegCloseKey(keyHandle);

    if (writeStatus == ERROR_SUCCESS) {
        if (message) {
            *message = L"Startup entry added.";
        }
        return true;
    }

    if (message) {
        *message = L"Failed to save the startup entry.";
    }
    return false;
}

bool PrinterServer::RemoveFromStartup(std::wstring* message) const {
    HKEY keyHandle = nullptr;
    const LONG status = ::RegOpenKeyExW(HKEY_CURRENT_USER,
                                        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                                        0,
                                        KEY_SET_VALUE,
                                        &keyHandle);
    if (status != ERROR_SUCCESS) {
        if (message) {
            *message = L"Unable to open the Run registry key.";
        }
        return false;
    }

    LONG deleteStatus = ERROR_FILE_NOT_FOUND;
    for (const std::wstring& valueName : GetStartupValueNames()) {
        const LONG result = ::RegDeleteValueW(keyHandle, valueName.c_str());
        if (result == ERROR_SUCCESS) {
            deleteStatus = ERROR_SUCCESS;
        } else if (deleteStatus == ERROR_FILE_NOT_FOUND) {
            deleteStatus = result;
        }
    }
    ::RegCloseKey(keyHandle);

    if (deleteStatus == ERROR_SUCCESS || deleteStatus == ERROR_FILE_NOT_FOUND) {
        if (message) {
            *message = L"Startup entry removed.";
        }
        return true;
    }

    if (message) {
        *message = L"Failed to remove the startup entry.";
    }
    return false;
}

bool PrinterServer::IsStartupEnabled(std::wstring* commandLine) const {
    for (const std::wstring& valueName : GetStartupValueNames()) {
        const std::wstring value = GetRegistryStringValue(HKEY_CURRENT_USER,
                                                          L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                                                          valueName);
        if (!Trim(value).empty()) {
            if (commandLine) {
                *commandLine = value;
            }
            return true;
        }
    }
    if (commandLine) {
        commandLine->clear();
    }
    return false;
}
