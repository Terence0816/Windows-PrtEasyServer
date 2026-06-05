#pragma once

#include "localization.h"
#include "printer_server.h"

class MainWindow {
public:
    MainWindow(HINSTANCE instance, bool autoStart);
    ~MainWindow();

    bool Create();
    void Show(int showCommand);
    HWND GetHwnd() const;

private:
    static constexpr UINT kMsgAppendLog = WM_APP + 1;
    static constexpr UINT kMsgTrayIcon = WM_APP + 2;
    static constexpr UINT kMsgHideToTray = WM_APP + 3;
    static constexpr UINT kMsgFinalizeUi = WM_APP + 4;

    enum ControlId {
        IdTabControl = 1001,
        IdPrinterCombo,
        IdPortEdit,
        IdAddUpdateButton,
        IdRemoveButton,
        IdRefreshPrintersButton,
        IdSaveButton,
        IdPrinterList,
        IdWebPortEdit,
        IdStartButton,
        IdStopButton,
        IdOpenWebButton,
        IdAddStartupButton,
        IdRemoveStartupButton,
        IdLogEdit,
        IdMinimizeToTrayCheck,
        IdLanguageCombo,
        IdApplyLanguageButton,
        IdAboutGithubLink,
        IdServerStatusStatic,
        IdStartupStatusStatic,
        IdStatusDetailsStatic,
        IdTrayShowWindow = 2001,
        IdTrayHideWindow,
        IdTrayStartServer,
        IdTrayStopServer,
        IdTrayQuit,
    };

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    void CreateControls();
    void CreateServerPageControls();
    void CreateSettingsPageControls();
    void ApplyFont(HWND control) const;
    void LayoutControls(int clientWidth, int clientHeight);
    void LayoutServerPage(int width, int height);
    void LayoutSettingsPage(int width, int height);
    void ShowActiveTab();
    void ApplyTranslations();
    void PopulateLanguageCombo();
    void UpdateTitle();

    void RefreshInstalledPrinters();
    void LoadConfigIntoUi();
    AppConfig CollectConfigFromUi(bool* ok, std::wstring* errorText) const;

    void UpdatePrinterListView(const std::vector<PrinterConfigEntry>& printers);
    std::vector<PrinterConfigEntry> ReadPrinterListView() const;
    int GetSelectedPrinterIndex() const;
    void LoadSelectedPrinterIntoEditor();
    void ResetPrinterEditor();
    int SuggestNextPrinterPort() const;
    void SetSuggestedNextPrinterPort();
    bool IsPortAlreadyUsed(int port, int ignoreRow) const;
    bool HasConfiguredPrinters() const;

    void AppendLog(const std::wstring& message);
    void PostLog(const std::wstring& message);
    void UpdateStatus();
    void UpdateStartupStatus();

    void OnAddOrUpdatePrinter();
    void OnRemovePrinter();
    void OnSaveConfig();
    void OnStartServer();
    void OnStopServer();
    void OnOpenWeb();
    void OnAddStartup();
    void OnRemoveStartup();
    void OnApplyLanguage();
    void RestartApplication(bool restoreServer);

    void AddTrayIcon();
    void RemoveTrayIcon();
    void UpdateTrayIcon();
    void ShowFromTray();
    void HideToTray(bool logAction);
    void ShowTrayMenu();
    void QuitApplication();

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    PrinterServer server_;
    LocalizationManager localizer_;
    bool autoStart_ = false;
    bool quitRequested_ = false;
    bool minimizeToTray_ = true;
    bool autoStartEvaluated_ = false;
    int editingRow_ = -1;
    int activeTab_ = 0;
    HFONT font_ = nullptr;
    HFONT statusFont_ = nullptr;
    HFONT linkFont_ = nullptr;
    HICON appIcon_ = nullptr;
    NOTIFYICONDATAW trayData_{};
    bool trayIconAdded_ = false;
    std::vector<std::wstring> languageCodes_;

    HWND tabControl_ = nullptr;
    HWND serverPage_ = nullptr;
    HWND settingsPage_ = nullptr;

    HWND configFrame_ = nullptr;
    HWND configHintStatic_ = nullptr;
    HWND printerCombo_ = nullptr;
    HWND printerLabel_ = nullptr;
    HWND portEdit_ = nullptr;
    HWND portLabel_ = nullptr;
    HWND addUpdateButton_ = nullptr;
    HWND removeButton_ = nullptr;
    HWND refreshPrintersButton_ = nullptr;
    HWND saveButton_ = nullptr;
    HWND printerList_ = nullptr;
    HWND webPortEdit_ = nullptr;
    HWND webPortLabel_ = nullptr;
    HWND webPortHintStatic_ = nullptr;

    HWND serverControlFrame_ = nullptr;
    HWND startButton_ = nullptr;
    HWND stopButton_ = nullptr;
    HWND openWebButton_ = nullptr;
    HWND serverStatusStatic_ = nullptr;
    HWND statusDetailsStatic_ = nullptr;

    HWND startupFrame_ = nullptr;
    HWND startupStatusStatic_ = nullptr;
    HWND addStartupButton_ = nullptr;
    HWND removeStartupButton_ = nullptr;

    HWND logFrame_ = nullptr;
    HWND logEdit_ = nullptr;

    HWND appSettingsFrame_ = nullptr;
    HWND minimizeToTrayCheck_ = nullptr;
    HWND languageFrame_ = nullptr;
    HWND languageLabel_ = nullptr;
    HWND languageCombo_ = nullptr;
    HWND applyLanguageButton_ = nullptr;
    HWND aboutFrame_ = nullptr;
    HWND aboutTitleStatic_ = nullptr;
    HWND aboutVersionStatic_ = nullptr;
    HWND aboutCopyrightStatic_ = nullptr;
    HWND aboutGithubLabelStatic_ = nullptr;
    HWND aboutGithubLink_ = nullptr;
    HWND aboutLegacyHeaderStatic_ = nullptr;
    HWND aboutLegacyUrlStatic_ = nullptr;
    HWND aboutLegacyCopyrightStatic_ = nullptr;
    HWND aboutSummaryStatic_ = nullptr;
};
