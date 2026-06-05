#pragma once

#include "common.h"

struct PrinterConfigEntry {
    std::wstring printerName;
    int port = 9100;
};

struct AppConfig {
    std::vector<PrinterConfigEntry> printers;
    int webPort = 80;
    std::wstring language = L"en";
    bool minimizeToTray = true;
};

class ConfigStore {
public:
    ConfigStore();

    const std::wstring& GetConfigPath() const;
    AppConfig Load() const;
    bool Save(const AppConfig& config) const;

private:
    std::wstring configPath_;

    static std::wstring EscapeRegex(const std::wstring& input);
    static int ParseIntValue(const std::wstring& json, const std::wstring& key, int defaultValue);
    static bool ParseBoolValue(const std::wstring& json, const std::wstring& key, bool defaultValue);
    static std::wstring ParseStringValue(const std::wstring& json, const std::wstring& key, const std::wstring& defaultValue);
};
