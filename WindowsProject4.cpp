#include <windows.h>
#include <string>
#include <fstream>
#include <vector>
#include <ShlObj.h>  // 用于获取文档目录
#include <commctrl.h>  // 通用控件库，用于ComboBox
#include <codecvt>     // 新增头文件，用于编码转换

#pragma comment(lib, "comctl32.lib")  // 链接通用控件库

// 全局变量
HWND hButton = NULL;        // 开始按钮句柄
HWND hStatus = NULL;        // 状态标签句柄
HWND hComboBox = NULL;      // 路径选择下拉框句柄
HWND hBrowseBtn = NULL;     // 浏览按钮句柄
bool isMonitoring = false;  // 是否正在监测剪贴板
std::wstring lastContent;   // 上次检测到的剪贴板内容
UINT_PTR hTimer = NULL;     // 定时器句柄
std::wofstream outFile;     // 输出文件流
std::wstring filePath;      // 输出文件路径
std::vector<std::wstring> recentPaths;  // 最近使用的路径

// 初始化通用控件
void InitCommonControls() {
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    ::InitCommonControlsEx(&icc);
}

// 获取剪贴板文本内容（支持所有格式，保留所有字符）
std::wstring GetClipboardText(HWND hwnd) {
    std::wstring result;

    // 尝试各种文本格式，按优先级排列
    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        // Unicode文本格式 (首选)
        if (OpenClipboard(hwnd)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData) {
                wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
                if (pszText) {
                    result = pszText;
                    GlobalUnlock(hData);
                }
            }
            CloseClipboard();
        }
    }
    else if (IsClipboardFormatAvailable(CF_TEXT)) {
        // ANSI文本格式
        if (OpenClipboard(hwnd)) {
            HANDLE hData = GetClipboardData(CF_TEXT);
            if (hData) {
                char* pszText = static_cast<char*>(GlobalLock(hData));
                if (pszText) {
                    // 转换ANSI到Unicode
                    int len = MultiByteToWideChar(CP_ACP, 0, pszText, -1, NULL, 0);
                    if (len > 0) {
                        wchar_t* wideText = new wchar_t[len];
                        MultiByteToWideChar(CP_ACP, 0, pszText, -1, wideText, len);
                        result = wideText;
                        delete[] wideText;
                    }
                    GlobalUnlock(hData);
                }
            }
            CloseClipboard();
        }
    }
    else if (IsClipboardFormatAvailable(RegisterClipboardFormat(L"HTML Format"))) {
        // HTML格式
        if (OpenClipboard(hwnd)) {
            HANDLE hData = GetClipboardData(RegisterClipboardFormat(L"HTML Format"));
            if (hData) {
                char* pszHtml = static_cast<char*>(GlobalLock(hData));
                if (pszHtml) {
                    // 提取HTML中的纯文本内容
                    std::string html(pszHtml);
                    // 简单提取<body>标签内的内容
                    size_t startPos = html.find("<body>");
                    size_t endPos = html.find("</body>");
                    if (startPos != std::string::npos && endPos != std::string::npos) {
                        startPos += 6; // 跳过"<body>"
                        std::string bodyContent = html.substr(startPos, endPos - startPos);

                        // 简单去除HTML标签
                        std::string plainText;
                        bool inTag = false;
                        for (char c : bodyContent) {
                            if (c == '<') inTag = true;
                            else if (c == '>') inTag = false;
                            else if (!inTag) plainText += c;
                        }

                        // 转换为Unicode
                        int len = MultiByteToWideChar(CP_UTF8, 0, plainText.c_str(), -1, NULL, 0);
                        if (len > 0) {
                            wchar_t* wideText = new wchar_t[len];
                            MultiByteToWideChar(CP_UTF8, 0, plainText.c_str(), -1, wideText, len);
                            result = wideText;
                            delete[] wideText;
                        }
                    }
                    GlobalUnlock(hData);
                }
            }
            CloseClipboard();
        }
    }
    else if (IsClipboardFormatAvailable(RegisterClipboardFormat(L"Rich Text Format"))) {
        // RTF格式
        if (OpenClipboard(hwnd)) {
            HANDLE hData = GetClipboardData(RegisterClipboardFormat(L"Rich Text Format"));
            if (hData) {
                char* pszRtf = static_cast<char*>(GlobalLock(hData));
                if (pszRtf) {
                    // 简单提取RTF中的文本（注意：这是一个非常简化的实现）
                    std::string rtf(pszRtf);
                    std::string plainText;

                    // 简单的RTF文本提取逻辑
                    bool inText = false;
                    for (size_t i = 0; i < rtf.length(); i++) {
                        if (rtf[i] == '\\' && i + 1 < rtf.length() && rtf[i + 1] == ' ') {
                            // 跳过转义字符
                            i++;
                            continue;
                        }
                        if (rtf[i] == '{') {
                            inText = false;
                        }
                        else if (rtf[i] == '}') {
                            inText = true;
                        }
                        else if (inText && !iswspace(rtf[i])) {
                            plainText += rtf[i];
                        }
                    }

                    // 转换为Unicode
                    int len = MultiByteToWideChar(CP_ACP, 0, plainText.c_str(), -1, NULL, 0);
                    if (len > 0) {
                        wchar_t* wideText = new wchar_t[len];
                        MultiByteToWideChar(CP_ACP, 0, plainText.c_str(), -1, wideText, len);
                        result = wideText;
                        delete[] wideText;
                    }
                    GlobalUnlock(hData);
                }
            }
            CloseClipboard();
        }
    }

    // 保留所有字符，包括空格、换行和控制字符
    // 只去除首尾空白字符（可选，根据需求决定是否保留）
    size_t start = 0;
    while (start < result.length() && iswspace(result[start])) start++;

    size_t end = result.length();
    while (end > start && iswspace(result[end - 1])) end--;

    return result.substr(start, end - start);
}

// 浏览文件夹对话框
std::wstring BrowseFolder(HWND hwnd) {
    wchar_t folderPath[MAX_PATH] = { 0 };
    BROWSEINFO bi = { 0 };
    bi.hwndOwner = hwnd;
    bi.lpszTitle = L"选择保存位置";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != NULL) {
        SHGetPathFromIDList(pidl, folderPath);
        CoTaskMemFree(pidl);
        return std::wstring(folderPath);
    }
    return L"";
}

// 更新ComboBox中的路径列表
void UpdatePathComboBox() {
    SendMessage(hComboBox, CB_RESETCONTENT, 0, 0);
    for (const auto& path : recentPaths) {
        SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)path.c_str());
    }
    if (!recentPaths.empty()) {
        SendMessage(hComboBox, CB_SETCURSEL, 0, 0);
    }
}

// 添加新路径到最近使用列表
void AddPathToRecent(const std::wstring& path) {
    // 检查是否已存在
    for (auto it = recentPaths.begin(); it != recentPaths.end(); ++it) {
        if (*it == path) {
            recentPaths.erase(it);
            break;
        }
    }

    // 添加到列表开头
    recentPaths.insert(recentPaths.begin(), path);

    // 限制列表长度
    if (recentPaths.size() > 5) {
        recentPaths.resize(5);
    }

    UpdatePathComboBox();
}

// 写入内容到文件
void WriteToFile(const std::wstring& content) {
    if (outFile.is_open()) {
        outFile << content << L"\n";
        outFile.flush();
    }
}

// 窗口过程函数
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        // 初始化通用控件
        InitCommonControls();

        // 创建开始按钮
        hButton = CreateWindow(
            L"BUTTON", L"开始监测剪贴板",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            300, 250, 200, 50, hwnd, (HMENU)1, NULL, NULL
        );

        // 创建状态标签
        hStatus = CreateWindow(
            L"STATIC", L"点击开始按钮启动监测",
            WS_VISIBLE | WS_CHILD,
            300, 320, 200, 25, hwnd, (HMENU)2, NULL, NULL
        );

        // 创建路径选择标签
        CreateWindow(
            L"STATIC", L"保存路径:",
            WS_VISIBLE | WS_CHILD,
            100, 100, 80, 25, hwnd, (HMENU)3, NULL, NULL
        );

        // 创建路径选择ComboBox
        hComboBox = CreateWindow(
            WC_COMBOBOX, L"",
            WS_VISIBLE | WS_CHILD | CBS_DROPDOWN,
            200, 100, 400, 200, hwnd, (HMENU)4, NULL, NULL
        );

        // 创建浏览按钮
        hBrowseBtn = CreateWindow(
            L"BUTTON", L"浏览...",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            620, 100, 80, 25, hwnd, (HMENU)5, NULL, NULL
        );

        // 初始化文件路径
        wchar_t docPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, 0, docPath))) {
            filePath = std::wstring(docPath) + L"\\clipboard_log.txt";
            recentPaths.push_back(std::wstring(docPath));
        }
        else {
            filePath = L"clipboard_log.txt";
            recentPaths.push_back(L".");
        }

        // 更新路径选择框
        UpdatePathComboBox();

        // 创建定时器(300毫秒间隔，提高响应速度)
        hTimer = SetTimer(hwnd, 1, 300, NULL);
        break;
    }

    case WM_COMMAND: {
        if (LOWORD(wParam) == 1 && HIWORD(wParam) == BN_CLICKED) {
            // 开始/停止按钮点击事件
            isMonitoring = !isMonitoring;

            // 更新按钮文本和状态标签
            SetWindowText(hButton, isMonitoring ? L"停止监测" : L"开始监测");
            SetWindowText(hStatus, isMonitoring ? L"正在监测剪贴板..." : L"监测已停止");

            // 如果开始监测，打开文件
            if (isMonitoring) {
                // 获取当前选择的路径
                int index = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
                if (index >= 0) {
                    wchar_t path[MAX_PATH];
                    SendMessage(hComboBox, CB_GETLBTEXT, index, (LPARAM)path);
                    filePath = std::wstring(path) + L"\\clipboard_log.txt";
                }

                // 打开文件并设置为UTF-16编码
                outFile.imbue(std::locale(std::locale(), new std::codecvt_utf16<wchar_t, 0x10FFFF, std::codecvt_mode::little_endian>()));
                outFile.open(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
                if (!outFile.is_open()) {
                    MessageBox(hwnd, L"无法创建日志文件!", L"错误", MB_ICONERROR);
                    isMonitoring = false;
                    SetWindowText(hButton, L"开始监测");
                    SetWindowText(hStatus, L"文件打开失败，监测未启动");
                }
                else {
                    // 写入UTF-16 BOM标记
                    unsigned char bom[2] = { 0xFF, 0xFE };
                    outFile.write(reinterpret_cast<const wchar_t*>(bom), 1);

                    lastContent = GetClipboardText(hwnd);
                }
            }
            else {
                // 如果停止监测，关闭文件
                if (outFile.is_open()) {
                    outFile.close();
                }
            }
        }
        else if (LOWORD(wParam) == 5 && HIWORD(wParam) == BN_CLICKED) {
            // 浏览按钮点击事件
            std::wstring selectedPath = BrowseFolder(hwnd);
            if (!selectedPath.empty()) {
                AddPathToRecent(selectedPath);
            }
        }
        else if (LOWORD(wParam) == 4 && HIWORD(wParam) == CBN_SELCHANGE) {
            // 路径选择框变化事件
            int index = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
            if (index >= 0) {
                wchar_t path[MAX_PATH];
                SendMessage(hComboBox, CB_GETLBTEXT, index, (LPARAM)path);
                filePath = std::wstring(path) + L"\\clipboard_log.txt";

                // 如果正在监测，重新打开文件
                if (isMonitoring) {
                    if (outFile.is_open()) {
                        outFile.close();
                    }

                    // 重新打开文件并设置为UTF-16编码
                    outFile.imbue(std::locale(std::locale(), new std::codecvt_utf16<wchar_t, 0x10FFFF, std::codecvt_mode::little_endian>()));
                    outFile.open(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
                    if (!outFile.is_open()) {
                        MessageBox(hwnd, L"无法创建日志文件!", L"错误", MB_ICONERROR);
                        isMonitoring = false;
                        SetWindowText(hButton, L"开始监测");
                        SetWindowText(hStatus, L"文件打开失败，监测已停止");
                    }
                    else {
                        // 写入UTF-16 BOM标记
                        unsigned char bom[2] = { 0xFF, 0xFE };
                        outFile.write(reinterpret_cast<const wchar_t*>(bom), 1);
                    }
                }
            }
        }
        break;
    }

    case WM_TIMER: {
        if (isMonitoring) {
            std::wstring current = GetClipboardText(hwnd);

            // 检测到新内容
            if (current != lastContent && !current.empty()) {
                WriteToFile(current);
                lastContent = current;
            }
        }
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        // 设置字体和颜色
        HFONT hFont = CreateFont(
            16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei"
        );
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        // 绘制标题
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        DrawTextW(hdc, L"剪贴板内容监测工具", -1, &rect, DT_CENTER | DT_TOP | DT_SINGLELINE);

        // 绘制文件路径信息
        rect.top += 30;
        std::wstring filePathInfo = std::wstring(L"当前日志文件: ") + filePath;
        DrawTextW(hdc, filePathInfo.c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_SINGLELINE);

        // 绘制状态信息
        rect.top += 30;
        std::wstring statusText = isMonitoring ? std::wstring(L"运行中") : std::wstring(L"已停止");
        std::wstring statusInfo = std::wstring(L"监测状态: ") + statusText;
        DrawTextW(hdc, statusInfo.c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_SINGLELINE);

        // 恢复字体
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY: {
        // 清理资源
        if (hTimer) {
            KillTimer(hwnd, hTimer);
            hTimer = NULL;
        }

        if (outFile.is_open()) {
            outFile.close();
        }

        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"ClipboardMonitorClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"剪贴板内容监测工具",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}