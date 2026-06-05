#include "config.h"

#include "localization.h"

ConfigStore::ConfigStore()
    : configPath_(JoinPath(GetModuleDirectory(), L"config.json")) {
}

const std::wstring& ConfigStore::GetConfigPath() const {
    return configPath_;
}

std::wstring ConfigStore::EscapeRegex(const std::wstring& input) {
    std::wstring output;
    output.reserve(input.size() * 2);
    for (wchar_t ch : input) {
        switch (ch) {
            case L'.':
            case L'^':
            case L'$':
            case L'|':
            case L'(':
            case L')':
            case L'[':
            case L']':
            case L'{':
            case L'}':
            case L'*':
            case L'+':
            case L'?':
            case L'\\':
                output.push_back(L'\\');
                output.push_back(ch);
                break;
            default:
                output.push_back(ch);
                break;
        }
    }
    return output;
}

int ConfigStore::ParseIntValue(const std::wstring& json, const std::wstring& key, int defaultValue) {
    const std::wregex pattern(L"\"" + EscapeRegex(key) + L"\"\\s*:\\s*(-?\\d+)", std::regex::icase);
    std::wsmatch match;
    if (std::regex_search(json, match, pattern) && match.size() > 1) {
        return _wtoi(match[1].str().c_str());
    }
    return defaultValue;
}

bool ConfigStore::ParseBoolValue(const std::wstring& json, const std::wstring& key, bool defaultValue) {
    const std::wregex pattern(L"\"" + EscapeRegex(key) + L"\"\\s*:\\s*(true|false|1|0)", std::regex::icase);
    std::wsmatch match;
    if (std::regex_search(json, match, pattern) && match.size() > 1) {
        const std::wstring value = ToLowerCopy(Trim(match[1].str()));
        return value == L"true" || value == L"1";
    }
    return defaultValue;
}

std::wstring ConfigStore::ParseStringValue(const std::wstring& json, const std::wstring& key, const std::wstring& defaultValue) {
    const std::wregex pattern(L"\"" + EscapeRegex(key) + L"\"\\s*:\\s*\"((?:\\\\.|[^\"])*)\"", std::regex::icase);
    std::wsmatch match;
    if (std::regex_search(json, match, pattern) && match.size() > 1) {
        std::wstring value = match[1].str();
        value = ReplaceAll(value, L"\\r", L"\r");
        value = ReplaceAll(value, L"\\n", L"\n");
        value = ReplaceAll(value, L"\\t", L"\t");
        value = ReplaceAll(value, L"\\\"", L"\"");
        value = ReplaceAll(value, L"\\\\", L"\\");
        return value;
    }
    return defaultValue;
}

AppConfig ConfigStore::Load() const {
    AppConfig config;
    config.language = LocalizationManager::DetectDefaultLanguage();

    if (!FileExists(configPath_)) {
        return config;
    }

    const std::wstring json = ReadUtf8File(configPath_);
    if (json.empty()) {
        return config;
    }

    config.webPort = ParseIntValue(json, L"web_port", 80);
    if (config.webPort <= 0 || config.webPort > 65535) {
        config.webPort = 80;
    }

    config.language = LocalizationManager::NormalizeLanguageCode(
        ParseStringValue(json, L"language", config.language));
    if (Trim(config.language).empty()) {
        config.language = LocalizationManager::DetectDefaultLanguage();
    }
    config.minimizeToTray = ParseBoolValue(json, L"minimize_to_tray", true);

    std::vector<PrinterConfigEntry> printers;
    const std::wregex printerPattern(
        LR"PRINTER(\{[^{}]*"printer_name"\s*:\s*"((?:\\.|[^"])*)"\s*,\s*"port"\s*:\s*(\d+)[^{}]*\})PRINTER",
        std::regex::icase);
    auto begin = std::wsregex_iterator(json.begin(), json.end(), printerPattern);
    auto end = std::wsregex_iterator();
    for (auto it = begin; it != end; ++it) {
        PrinterConfigEntry entry;
        entry.printerName = (*it)[1].str();
        entry.printerName = ReplaceAll(entry.printerName, L"\\\"", L"\"");
        entry.printerName = ReplaceAll(entry.printerName, L"\\\\", L"\\");
        entry.port = _wtoi((*it)[2].str().c_str());
        if (entry.port <= 0) {
            entry.port = 9100;
        }
        printers.push_back(entry);
    }

    if (printers.empty()) {
        PrinterConfigEntry legacy;
        legacy.printerName = ParseStringValue(json, L"printer_name", L"");
        legacy.port = ParseIntValue(json, L"port", 9100);
        if (legacy.port <= 0) {
            legacy.port = 9100;
        }
        if (!Trim(legacy.printerName).empty()) {
            printers.push_back(legacy);
        }
    }

    config.printers = printers;
    return config;
}

bool ConfigStore::Save(const AppConfig& config) const {
    std::wostringstream output;
    output << L"{\n";
    output << L"  \"printers\": [\n";
    for (std::size_t i = 0; i < config.printers.size(); ++i) {
        const PrinterConfigEntry& entry = config.printers[i];
        output << L"    {\n";
        output << L"      \"printer_name\": \"" << JsonEscape(entry.printerName) << L"\",\n";
        output << L"      \"port\": " << entry.port << L"\n";
        output << L"    }";
        if (i + 1 < config.printers.size()) {
            output << L",";
        }
        output << L"\n";
    }
    output << L"  ],\n";

    const PrinterConfigEntry first = config.printers.empty() ? PrinterConfigEntry() : config.printers.front();
    output << L"  \"printer_name\": \"" << JsonEscape(first.printerName) << L"\",\n";
    output << L"  \"port\": " << first.port << L",\n";
    output << L"  \"web_port\": " << config.webPort << L",\n";
    output << L"  \"language\": \"" << JsonEscape(LocalizationManager::NormalizeLanguageCode(config.language)) << L"\",\n";
    output << L"  \"minimize_to_tray\": " << (config.minimizeToTray ? L"true" : L"false") << L"\n";
    output << L"}\n";

    return WriteUtf8File(configPath_, WideToUtf8(output.str()), false);
}
