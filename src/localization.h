#pragma once

#include "common.h"

#include <map>

class LocalizationManager {
public:
    LocalizationManager();

    bool Initialize(const std::wstring& requestedLanguage);

    std::wstring Get(const std::wstring& key) const;
    std::wstring GetLanguageFilePath(const std::wstring& code) const;
    const std::wstring& GetCurrentLanguage() const;
    std::vector<std::wstring> GetAvailableLanguages() const;

    static std::wstring NormalizeLanguageCode(const std::wstring& value);
    static std::wstring DetectDefaultLanguage();
    static std::vector<std::wstring> GetSupportedLanguages();

private:
    using StringMap = std::map<std::wstring, std::wstring>;

    void EnsureDefaultLanguageFiles() const;
    bool LoadBundle(const std::wstring& code, StringMap* target) const;

    static StringMap BuildDefaultEnglishStrings();
    static StringMap BuildDefaultTraditionalChineseStrings();
    static std::wstring SerializeLanguageFile(const StringMap& bundle);
    static std::wstring EscapeFileValue(const std::wstring& value);
    static std::wstring UnescapeFileValue(const std::wstring& value);
    static const StringMap& GetBuiltInBundle(const std::wstring& code);

    std::wstring languageRoot_;
    std::wstring currentLanguage_;
    StringMap strings_;
    StringMap fallbackStrings_;
};
