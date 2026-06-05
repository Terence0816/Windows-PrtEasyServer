#pragma once

#include "app_constants.h"
#include "config.h"

struct PrinterDetails {
    std::wstring printerName;
    std::wstring driverName;
    std::wstring printerPortName;
};

struct WebPrinterEntry {
    int index = 0;
    std::wstring printerName;
    std::wstring driverName;
    std::wstring hostName;
    std::wstring hostIp;
    std::wstring tcpPortName;
    int port = 9100;
    std::wstring driverArchiveName;
    std::wstring driverArchivePath;
};

class PrinterServer {
public:
    explicit PrinterServer(std::function<void(const std::wstring&)> logCallback = nullptr);
    ~PrinterServer();

    AppConfig GetConfigCopy() const;
    void UpdateConfig(const AppConfig& config);
    bool SaveConfig();
    std::wstring GetConfigPath() const;

    bool Start();
    void Stop();
    bool IsRunning() const;
    std::wstring GetWebUrl() const;

    std::vector<std::wstring> EnumerateInstalledPrinters() const;
    PrinterDetails QueryPrinterDetails(const std::wstring& printerName) const;

    bool TestConnection(const std::wstring& host, int port, const std::string* payload, std::wstring* resultLog);

    bool AddToStartup(std::wstring* message) const;
    bool RemoveFromStartup(std::wstring* message) const;
    bool IsStartupEnabled(std::wstring* commandLine) const;

private:
    struct ListenerContext {
        SOCKET socketHandle = INVALID_SOCKET;
        PrinterConfigEntry printer;
        std::thread thread;
    };

    struct DriverManifestInfo {
        std::wstring driverName;
        std::wstring infPath;
        std::wstring archiveName;
        std::wstring sourceFolder;
        std::vector<std::wstring> files;
    };

    void Log(const std::wstring& message) const;
    std::wstring UiText(const std::wstring& english, const std::wstring& traditionalChinese) const;
    std::vector<PrinterConfigEntry> GetActivePrinters() const;
    int GetWebPort() const;
    std::vector<WebPrinterEntry> BuildWebEntries() const;
    std::wstring BuildInstallerBatchContent(const WebPrinterEntry& entry) const;
    std::string RenderWebPage() const;

    void RawAcceptLoop(ListenerContext* listener);
    void HandleRawClient(SOCKET clientSocket, sockaddr_in clientAddress, PrinterConfigEntry printer);
    bool PrintRaw(const std::vector<unsigned char>& data, const std::wstring& printerName) const;

    void HttpAcceptLoop();
    void HandleHttpClient(SOCKET clientSocket);
    void SendHttpResponse(SOCKET clientSocket,
                          int statusCode,
                          const std::string& statusText,
                          const std::string& contentType,
                          const std::string& body,
                          const std::vector<std::pair<std::string, std::string>>& extraHeaders = {}) const;
    void SendHttpFile(SOCKET clientSocket,
                      const std::wstring& path,
                      const std::string& contentType,
                      const std::string& downloadName) const;

    bool QueryDriverManifest(const WebPrinterEntry& entry, DriverManifestInfo* info, std::wstring* errorText) const;
    bool EnsureDriverArchive(const WebPrinterEntry& entry, std::wstring* archivePath, std::wstring* errorText);
    bool CreateDriverArchiveFromManifest(const DriverManifestInfo& info, const std::wstring& archivePath, std::wstring* errorText) const;
    bool CreateDriverArchiveFromFolder(const std::wstring& sourceFolder, const std::wstring& archivePath, std::wstring* errorText) const;
    std::wstring GetDriverArchivePath(const std::wstring& driverName) const;
    void StartDriverPackagingThread();
    void DriverPackagingWorker();

    bool EnsureFirewallPortRule(int port, const std::wstring& ruleName) const;
    void EnsureFirewallRules() const;

    ConfigStore configStore_;
    mutable std::mutex configMutex_;
    mutable std::mutex driverMutex_;
    std::function<void(const std::wstring&)> logCallback_;
    AppConfig config_;
    std::vector<std::unique_ptr<ListenerContext>> listeners_;
    SOCKET httpSocket_ = INVALID_SOCKET;
    std::thread httpThread_;
    std::thread driverThread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};
};
