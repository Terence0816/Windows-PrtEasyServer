#include "common.h"

#include <cstdarg>
#include <cwctype>

namespace {

bool CreateDirectoryDeep(const std::wstring& path) {
    if (path.empty() || DirectoryExists(path)) {
        return true;
    }

    const std::size_t sep = path.find_last_of(L"\\/");
    if (sep != std::wstring::npos) {
        const std::wstring parent = path.substr(0, sep);
        if (!parent.empty() && !DirectoryExists(parent) && !CreateDirectoryDeep(parent)) {
            return false;
        }
    }

    if (::CreateDirectoryW(path.c_str(), nullptr)) {
        return true;
    }

    return GetLastError() == ERROR_ALREADY_EXISTS;
}

void AppendFilesRecursive(const std::wstring& rootPath, std::vector<std::wstring>& files) {
    WIN32_FIND_DATAW data{};
    const std::wstring pattern = JoinPath(rootPath, L"*");
    HANDLE findHandle = ::FindFirstFileW(pattern.c_str(), &data);
    if (findHandle == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        const std::wstring name = data.cFileName;
        if (name == L"." || name == L"..") {
            continue;
        }

        const std::wstring fullPath = JoinPath(rootPath, name);
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            AppendFilesRecursive(fullPath, files);
        } else {
            files.push_back(fullPath);
        }
    } while (::FindNextFileW(findHandle, &data));

    ::FindClose(findHandle);
}

bool DeleteDirectoryDeep(const std::wstring& rootPath) {
    WIN32_FIND_DATAW data{};
    const std::wstring pattern = JoinPath(rootPath, L"*");
    HANDLE findHandle = ::FindFirstFileW(pattern.c_str(), &data);
    if (findHandle != INVALID_HANDLE_VALUE) {
        do {
            const std::wstring name = data.cFileName;
            if (name == L"." || name == L"..") {
                continue;
            }

            const std::wstring fullPath = JoinPath(rootPath, name);
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                DeleteDirectoryDeep(fullPath);
            } else {
                ::SetFileAttributesW(fullPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                ::DeleteFileW(fullPath.c_str());
            }
        } while (::FindNextFileW(findHandle, &data));

        ::FindClose(findHandle);
    }

    ::SetFileAttributesW(rootPath.c_str(), FILE_ATTRIBUTE_NORMAL);
    return ::RemoveDirectoryW(rootPath.c_str()) != FALSE;
}

}  // namespace

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return std::wstring();
    }

    const int length = ::MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0) {
        return std::wstring();
    }

    std::wstring output(static_cast<std::size_t>(length), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &output[0], length);
    return output;
}

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return std::string();
    }

    const int length = ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0) {
        return std::string();
    }

    std::string output(static_cast<std::size_t>(length), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &output[0], length, nullptr, nullptr);
    return output;
}

std::wstring Trim(const std::wstring& value) {
    const std::wstring whitespace = L" \t\r\n";
    const std::size_t start = value.find_first_not_of(whitespace);
    if (start == std::wstring::npos) {
        return std::wstring();
    }

    const std::size_t end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

std::string Trim(const std::string& value) {
    const std::string whitespace = " \t\r\n";
    const std::size_t start = value.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return std::string();
    }

    const std::size_t end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

std::wstring ToLowerCopy(const std::wstring& value) {
    std::wstring output = value;
    std::transform(output.begin(), output.end(), output.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return output;
}

std::wstring ReplaceAll(std::wstring text, const std::wstring& from, const std::wstring& to) {
    if (from.empty()) {
        return text;
    }

    std::size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::wstring::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
    return text;
}

std::string ReplaceAll(std::string text, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return text;
    }

    std::size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
    return text;
}

std::wstring JsonEscape(const std::wstring& value) {
    std::wstring output;
    output.reserve(value.size() + 8);
    for (wchar_t ch : value) {
        switch (ch) {
            case L'\\': output += L"\\\\"; break;
            case L'"': output += L"\\\""; break;
            case L'\r': output += L"\\r"; break;
            case L'\n': output += L"\\n"; break;
            case L'\t': output += L"\\t"; break;
            default:
                output.push_back(ch);
                break;
        }
    }
    return output;
}

std::string HtmlEscape(const std::string& value) {
    std::string output = value;
    output = ReplaceAll(output, "&", "&amp;");
    output = ReplaceAll(output, "<", "&lt;");
    output = ReplaceAll(output, ">", "&gt;");
    output = ReplaceAll(output, "\"", "&quot;");
    output = ReplaceAll(output, "'", "&#39;");
    return output;
}

std::string UrlEncode(const std::string& value) {
    std::ostringstream out;
    out << std::hex << std::uppercase;
    for (unsigned char ch : value) {
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
            out << static_cast<char>(ch);
        } else {
            out << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(ch);
        }
    }
    return out.str();
}

std::wstring SanitizeFileName(const std::wstring& value) {
    std::wstring output;
    output.reserve(value.size());
    for (wchar_t ch : value) {
        switch (ch) {
            case L'<':
            case L'>':
            case L':':
            case L'"':
            case L'/':
            case L'\\':
            case L'|':
            case L'?':
            case L'*':
                output.push_back(L'_');
                break;
            default:
                output.push_back(ch);
                break;
        }
    }
    output = Trim(output);
    if (output.empty()) {
        return L"Printer";
    }
    return output;
}

std::wstring GetModuleDirectory() {
    wchar_t pathBuffer[MAX_PATH] = {};
    ::GetModuleFileNameW(nullptr, pathBuffer, MAX_PATH);
    std::wstring modulePath = pathBuffer;
    const std::size_t sep = modulePath.find_last_of(L"\\/");
    if (sep == std::wstring::npos) {
        return modulePath;
    }
    return modulePath.substr(0, sep);
}

std::wstring JoinPath(const std::wstring& left, const std::wstring& right) {
    if (left.empty()) {
        return right;
    }
    if (right.empty()) {
        return left;
    }
    if (left.back() == L'\\' || left.back() == L'/') {
        return left + right;
    }
    return left + L"\\" + right;
}

bool FileExists(const std::wstring& path) {
    DWORD attrs = ::GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool DirectoryExists(const std::wstring& path) {
    DWORD attrs = ::GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool EnsureDirectory(const std::wstring& path) {
    return CreateDirectoryDeep(path);
}

bool DeleteDirectoryTree(const std::wstring& path) {
    if (!DirectoryExists(path)) {
        return true;
    }
    return DeleteDirectoryDeep(path);
}

bool CopyFileOverwrite(const std::wstring& sourcePath, const std::wstring& destPath) {
    const std::size_t sep = destPath.find_last_of(L"\\/");
    if (sep != std::wstring::npos) {
        EnsureDirectory(destPath.substr(0, sep));
    }
    return ::CopyFileW(sourcePath.c_str(), destPath.c_str(), FALSE) != FALSE;
}

std::vector<std::wstring> EnumerateFilesRecursive(const std::wstring& rootPath) {
    std::vector<std::wstring> files;
    if (!DirectoryExists(rootPath)) {
        return files;
    }
    AppendFilesRecursive(rootPath, files);
    return files;
}

std::wstring ReadUtf8File(const std::wstring& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return std::wstring();
    }

    std::string data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0xEF && static_cast<unsigned char>(data[1]) == 0xBB && static_cast<unsigned char>(data[2]) == 0xBF) {
        data.erase(0, 3);
    }
    return Utf8ToWide(data);
}

bool WriteUtf8File(const std::wstring& path, const std::string& content, bool withBom) {
    const std::size_t sep = path.find_last_of(L"\\/");
    if (sep != std::wstring::npos) {
        EnsureDirectory(path.substr(0, sep));
    }

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    if (withBom) {
        const unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
        output.write(reinterpret_cast<const char*>(bom), 3);
    }
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    return output.good();
}

std::wstring CurrentTimestamp() {
    SYSTEMTIME st{};
    ::GetLocalTime(&st);
    wchar_t buffer[64] = {};
    swprintf_s(buffer, L"%04u-%02u-%02u %02u:%02u:%02u",
               st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buffer;
}

std::wstring FormatErrorMessage(DWORD errorCode) {
    LPWSTR messageBuffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length = ::FormatMessageW(flags, nullptr, errorCode, 0, reinterpret_cast<LPWSTR>(&messageBuffer), 0, nullptr);
    if (length == 0 || messageBuffer == nullptr) {
        return FormatString(L"error=%lu", errorCode);
    }

    std::wstring message(messageBuffer, length);
    ::LocalFree(messageBuffer);
    return Trim(message);
}

std::wstring FormatLastErrorMessage() {
    return FormatErrorMessage(::GetLastError());
}

std::wstring FormatString(const wchar_t* format, ...) {
    wchar_t buffer[2048] = {};
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(buffer, _countof(buffer), _TRUNCATE, format, args);
    va_end(args);
    return buffer;
}

bool RunHiddenProcess(const std::wstring& commandLine, DWORD* exitCode, std::wstring* capturedStdout) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    const bool capture = capturedStdout != nullptr;
    if (capture) {
        if (!::CreatePipe(&readPipe, &writePipe, &sa, 0)) {
            return false;
        }
        ::SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    if (capture) {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = writePipe;
        si.hStdError = writePipe;
    }

    PROCESS_INFORMATION pi{};
    std::wstring mutableCommandLine = commandLine;
    const BOOL created = ::CreateProcessW(
        nullptr,
        &mutableCommandLine[0],
        nullptr,
        nullptr,
        capture ? TRUE : FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi);

    if (capture && writePipe) {
        ::CloseHandle(writePipe);
        writePipe = nullptr;
    }

    if (!created) {
        if (readPipe) {
            ::CloseHandle(readPipe);
        }
        return false;
    }

    std::string stdoutBuffer;
    if (capture && readPipe) {
        char temp[4096] = {};
        DWORD bytesRead = 0;
        while (::ReadFile(readPipe, temp, sizeof(temp), &bytesRead, nullptr) && bytesRead > 0) {
            stdoutBuffer.append(temp, temp + bytesRead);
        }
        ::CloseHandle(readPipe);
        readPipe = nullptr;
    }

    ::WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD code = 0;
    ::GetExitCodeProcess(pi.hProcess, &code);
    if (exitCode) {
        *exitCode = code;
    }

    if (capturedStdout) {
        *capturedStdout = Utf8ToWide(stdoutBuffer);
        if (capturedStdout->empty()) {
            const int wideLength = ::MultiByteToWideChar(CP_ACP, 0, stdoutBuffer.c_str(), static_cast<int>(stdoutBuffer.size()), nullptr, 0);
            if (wideLength > 0) {
                std::wstring fallback(static_cast<std::size_t>(wideLength), L'\0');
                ::MultiByteToWideChar(CP_ACP, 0, stdoutBuffer.c_str(), static_cast<int>(stdoutBuffer.size()), &fallback[0], wideLength);
                *capturedStdout = fallback;
            }
        }
    }

    ::CloseHandle(pi.hThread);
    ::CloseHandle(pi.hProcess);
    return true;
}

bool RunPowerShellScript(const std::wstring& script, DWORD* exitCode, std::wstring* capturedStdout) {
    wchar_t tempPath[MAX_PATH] = {};
    if (::GetTempPathW(_countof(tempPath), tempPath) == 0) {
        return false;
    }

    wchar_t tempFile[MAX_PATH] = {};
    if (::GetTempFileNameW(tempPath, L"psc", 0, tempFile) == 0) {
        return false;
    }

    const std::wstring scriptPath = std::wstring(tempFile) + L".ps1";
    ::MoveFileExW(tempFile, scriptPath.c_str(), MOVEFILE_REPLACE_EXISTING);

    if (!WriteUtf8File(scriptPath, WideToUtf8(script), true)) {
        ::DeleteFileW(scriptPath.c_str());
        return false;
    }

    const std::wstring command = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" + scriptPath + L"\"";
    const bool result = RunHiddenProcess(command, exitCode, capturedStdout);
    ::DeleteFileW(scriptPath.c_str());
    return result;
}

bool IsRunningAsAdmin() {
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
    PSID adminGroup = nullptr;
    BOOL isMember = FALSE;

    if (!::AllocateAndInitializeSid(&authority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0, 0, 0, 0, 0, 0,
                                    &adminGroup)) {
        return false;
    }

    const BOOL ok = ::CheckTokenMembership(nullptr, adminGroup, &isMember);
    ::FreeSid(adminGroup);
    return ok != FALSE && isMember != FALSE;
}

std::wstring GetEnvVar(const wchar_t* name) {
    const DWORD size = ::GetEnvironmentVariableW(name, nullptr, 0);
    if (size == 0) {
        return std::wstring();
    }
    std::wstring value(static_cast<std::size_t>(size), L'\0');
    ::GetEnvironmentVariableW(name, &value[0], size);
    if (!value.empty() && value.back() == L'\0') {
        value.pop_back();
    }
    return value;
}

std::wstring GetComputerNameShort() {
    wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD size = _countof(buffer);
    if (::GetComputerNameW(buffer, &size)) {
        return buffer;
    }
    return L"localhost";
}

std::wstring GetLocalIPv4Address() {
    SOCKET sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock != INVALID_SOCKET) {
        sockaddr_in remote{};
        remote.sin_family = AF_INET;
        remote.sin_port = htons(80);
        ::InetPtonA(AF_INET, "8.8.8.8", &remote.sin_addr);

        if (::connect(sock, reinterpret_cast<sockaddr*>(&remote), sizeof(remote)) == 0) {
            sockaddr_in local{};
            int localSize = sizeof(local);
            if (::getsockname(sock, reinterpret_cast<sockaddr*>(&local), &localSize) == 0) {
                wchar_t addressBuffer[64] = {};
                if (::InetNtopW(AF_INET, &local.sin_addr, addressBuffer, _countof(addressBuffer)) != nullptr) {
                    ::closesocket(sock);
                    return addressBuffer;
                }
            }
        }
        ::closesocket(sock);
    }

    ULONG family = AF_INET;
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG size = 0;
    ::GetAdaptersAddresses(family, flags, nullptr, nullptr, &size);
    std::vector<unsigned char> buffer(size);
    IP_ADAPTER_ADDRESSES* addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    if (::GetAdaptersAddresses(family, flags, nullptr, addresses, &size) == NO_ERROR) {
        for (IP_ADAPTER_ADDRESSES* adapter = addresses; adapter != nullptr; adapter = adapter->Next) {
            if (adapter->OperStatus != IfOperStatusUp) {
                continue;
            }
            if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
                continue;
            }

            for (IP_ADAPTER_UNICAST_ADDRESS* unicast = adapter->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next) {
                if (unicast->Address.lpSockaddr && unicast->Address.lpSockaddr->sa_family == AF_INET) {
                    wchar_t addressBuffer[64] = {};
                    DWORD bufferLength = _countof(addressBuffer);
                    if (::WSAAddressToStringW(unicast->Address.lpSockaddr,
                                              static_cast<DWORD>(unicast->Address.iSockaddrLength),
                                              nullptr,
                                              addressBuffer,
                                              &bufferLength) == 0) {
                        const std::wstring candidate = addressBuffer;
                        if (candidate.rfind(L"169.254.", 0) != 0 && candidate.rfind(L"127.", 0) != 0) {
                            return candidate;
                        }
                    }
                }
            }
        }
    }

    return L"127.0.0.1";
}

bool OpenUrlInBrowser(const std::wstring& url) {
    HINSTANCE result = ::ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(result) > 32;
}

std::wstring QuoteForPowerShell(const std::wstring& value) {
    std::wstring escaped = ReplaceAll(value, L"'", L"''");
    return L"'" + escaped + L"'";
}
