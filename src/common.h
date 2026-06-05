#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <winspool.h>
#include <iphlpapi.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <memory>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Winspool.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Ole32.lib")

std::wstring Utf8ToWide(const std::string& text);
std::string WideToUtf8(const std::wstring& text);
std::wstring Trim(const std::wstring& value);
std::string Trim(const std::string& value);
std::wstring ToLowerCopy(const std::wstring& value);
std::wstring ReplaceAll(std::wstring text, const std::wstring& from, const std::wstring& to);
std::string ReplaceAll(std::string text, const std::string& from, const std::string& to);
std::wstring JsonEscape(const std::wstring& value);
std::string HtmlEscape(const std::string& value);
std::string UrlEncode(const std::string& value);
std::wstring SanitizeFileName(const std::wstring& value);
std::wstring GetModuleDirectory();
std::wstring JoinPath(const std::wstring& left, const std::wstring& right);
bool FileExists(const std::wstring& path);
bool DirectoryExists(const std::wstring& path);
bool EnsureDirectory(const std::wstring& path);
bool DeleteDirectoryTree(const std::wstring& path);
bool CopyFileOverwrite(const std::wstring& sourcePath, const std::wstring& destPath);
std::vector<std::wstring> EnumerateFilesRecursive(const std::wstring& rootPath);
std::wstring ReadUtf8File(const std::wstring& path);
bool WriteUtf8File(const std::wstring& path, const std::string& content, bool withBom = false);
std::wstring CurrentTimestamp();
std::wstring FormatErrorMessage(DWORD errorCode);
std::wstring FormatLastErrorMessage();
std::wstring FormatString(const wchar_t* format, ...);
bool RunHiddenProcess(const std::wstring& commandLine, DWORD* exitCode = nullptr, std::wstring* capturedStdout = nullptr);
bool RunPowerShellScript(const std::wstring& script, DWORD* exitCode = nullptr, std::wstring* capturedStdout = nullptr);
bool IsRunningAsAdmin();
std::wstring GetEnvVar(const wchar_t* name);
std::wstring GetComputerNameShort();
std::wstring GetLocalIPv4Address();
bool OpenUrlInBrowser(const std::wstring& url);
std::wstring QuoteForPowerShell(const std::wstring& value);
