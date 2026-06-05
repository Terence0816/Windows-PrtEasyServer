#include "main_window.h"
#include "app_constants.h"

#pragma comment(linker, \
    "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' " \
    "processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR commandLine, int showCommand) {
    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_LINK_CLASS;
    ::InitCommonControlsEx(&controls);

    const std::wstring args = commandLine ? commandLine : L"";
    const bool autoStart = args.find(L"--autostart") != std::wstring::npos;

    MainWindow window(instance, autoStart);
    if (!window.Create()) {
        ::MessageBoxW(nullptr, L"Failed to create the main window.", kAppName, MB_ICONERROR | MB_OK);
        return 1;
    }

    window.Show(showCommand);

    MSG msg{};
    while (::GetMessageW(&msg, nullptr, 0, 0) > 0) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
