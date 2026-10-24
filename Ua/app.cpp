// app.cpp : wWinMain、WndProc 和窗口消息处理
//

#include "config.h"
#include "state.h"
#include "inject.h"
#include "ui.h"

#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <dwmapi.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")

// ---------------------------------------------------------------------------
// 托盘
// ---------------------------------------------------------------------------

// true 时下一次 WM_CLOSE 真正销毁窗口；默认关闭按钮只是隐藏到托盘
bool g_realExit = false;
static NOTIFYICONDATAW g_tray{};

static void AddTray(HWND hWnd)
{
    g_tray.cbSize = sizeof(g_tray);
    g_tray.hWnd = hWnd;
    g_tray.uID = 1;
    g_tray.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_tray.uCallbackMessage = kMsgTray;
    g_tray.hIcon = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_UA), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    wcsncpy_s(g_tray.szTip, T(S_TRAY_TOOLTIP), _TRUNCATE);
    Shell_NotifyIconW(NIM_ADD, &g_tray);
}

static void RemoveTray()
{
    Shell_NotifyIconW(NIM_DELETE, &g_tray);
    if (g_tray.hIcon) { DestroyIcon(g_tray.hIcon); g_tray.hIcon = nullptr; }
}

static void ShowMainWindow(HWND hWnd)
{
    ShowWindow(hWnd, SW_SHOW);
    SetForegroundWindow(hWnd);
}

static void ShowTrayMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;
    AppendMenuW(hMenu, MF_STRING, kCmdTrayShow, T(S_TRAY_SHOW));
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, kCmdExit, T(S_EXIT));
    // SetForegroundWindow 是经典 Win32 托盘菜单的要求，否则菜单不会在失焦时自动消失
    SetForegroundWindow(hWnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN,
        pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);
    if (cmd == kCmdTrayShow)
    {
        ShowMainWindow(hWnd);
    }
    else if (cmd == kCmdExit)
    {
        g_realExit = true;
        DestroyWindow(hWnd);
    }
}

// ---------------------------------------------------------------------------
// 窗口辅助
// ---------------------------------------------------------------------------

// 圆角和系统阴影
static void SetupFrame(HWND hWnd)
{
    DWORD corner = 2;   // DWMWCP_ROUND，仅 Win11 生效
    DwmSetWindowAttribute(hWnd, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */, &corner, sizeof(corner));

    // Win11 22H2+ 请求 Mica 系统背景效果。DWM 调用成功后置 g_micaActive，
    // ui-main.cpp 的 Render 会据此跳过不透明背景填充，让 Mica 真正可见
    if (IsWindows11())
    {
        DWORD backdrop = 2;   // DWMSBT_MAINWINDOW (Mica)
        if (SUCCEEDED(DwmSetWindowAttribute(hWnd, 38 /* DWMWA_SYSTEMBACKDROP_TYPE */, &backdrop, sizeof(backdrop))))
            g_micaActive = true;
    }

    // 窗口带 WS_CAPTION 但去掉了非客户区（见 WM_NCCALCSIZE）。
    // Mica 需要边框延伸到整个客户区（全 -1）才会被 DWM 作为系统背景绘制；
    // 非 Mica 模式只需延伸 1px，DWM 就会画系统阴影
    MARGINS m = g_micaActive ? MARGINS{ -1, -1, -1, -1 } : MARGINS{ 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(hWnd, &m);
}

// 放到屏幕正中央
static void PlaceWindow(HWND hWnd)
{
    g_scale = GetDpiForWindow(hWnd) / 96.f;
    int w = (int)std::lround(kW * g_scale), h = (int)std::lround(kH * g_scale);
    RECT wa;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    int cx = (wa.left + wa.right - w) / 2;
    int cy = (wa.top + wa.bottom - h) / 2;
    SetWindowPos(hWnd, nullptr, cx, cy, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

static void ShowToast(HWND hWnd, const std::wstring& text)
{
    g_toast = text;
    SetTimer(hWnd, kTimerToast, 2500, nullptr);
    InvalidateRect(hWnd, nullptr, FALSE);
}

static void OpenUrl(HWND hWnd, const wchar_t* url)
{
    if ((INT_PTR)ShellExecuteW(hWnd, L"open", url, nullptr, nullptr, SW_SHOWNORMAL) <= 32)
        ShowToast(hWnd, T(S_ERR_OPEN_URL));
}

static void SetDialog(HWND hWnd, int i)
{
    g_dlg = i;
    g_prompts.clear();
    g_promptSel = -1;
    g_promptScroll = 0;
    if (i >= 0)
    {
        g_state[i] = ScanState(i);
        g_prompts = ListPrompts();
        // 自动选中：Ua.md（唯一的综合提示词）；否则 default.md；否则第一个
        for (int j = 0; j < (int)g_prompts.size(); ++j)
        {
            if (_wcsicmp(g_prompts[j].c_str(), L"Ua.md") == 0) { g_promptSel = j; break; }
            if (_wcsicmp(g_prompts[j].c_str(), L"default.md") == 0 && g_promptSel < 0) g_promptSel = j;
        }
        if (g_promptSel < 0 && !g_prompts.empty()) g_promptSel = 0;
    }
    InvalidateRect(hWnd, nullptr, FALSE);
}

static void DialogCommand(HWND hWnd, int b)
{
    int i = g_dlg;
    if (i < 0 || !DlgEnabled(b)) return;
    std::wstring err;
    switch (b)
    {
    case kDlgClose:
        SetDialog(hWnd, -1);
        break;
    case kDlgSite:
        OpenUrl(hWnd, kItems[i].url);
        break;
    case kDlgFolder:
        {
            std::wstring dir = PromptDir();
            SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);
            ShellExecuteW(hWnd, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        break;
    case kDlgInstall:
    case kDlgUninstall:
        {
            if (b == kDlgUninstall)
            {
                // 普通卸载确认
                int result = MessageBoxW(hWnd, T(S_CONFIRM_UNINSTALL), L"Ua", MB_YESNO | MB_ICONQUESTION);
                if (result != IDYES) return;

                // 检查文件里是否还有 Ua 的注入标记
                TargetState ts;
                if (LoadState(i, ts))
                {
                    std::wstring path = TargetPath(i);
                    std::string cur;
                    if (!path.empty() && ReadBytes(path, cur) == kReadOk)
                    {
                        // marker 注入：检查 UA-INJECT 标记是否还在
                        bool hasUaMarker = cur.find(kMarkerBegin) != std::string::npos
                                        && cur.find(kMarkerEnd) != std::string::npos;
                        // 兼容旧版全文覆盖：中/英文激活词
                        if (!hasUaMarker)
                        {
                            const char markerCN[] = "Ua \xE5\xB7\xB2\xE7\xBB\x8F\xE5\x87\x86\xE5\xA4\x87\xE5\xA5\xBD";
                            const char markerEN[] = "Ua is ready";
                            hasUaMarker = cur.find(markerCN) != std::string::npos
                                       || cur.find(markerEN) != std::string::npos;
                        }
                        if (!hasUaMarker)
                        {
                            int r2 = MessageBoxW(hWnd, T(S_WARN_FILE_MODIFIED), L"Ua", MB_YESNO | MB_ICONWARNING);
                            if (r2 != IDYES) return;
                        }
                    }
                }
            }
            bool ok = b == kDlgInstall ? Install(i, err) : Uninstall(i, err);
            g_state[i] = ScanState(i);
            if (ok)
            {
                SetDialog(hWnd, -1);
                ShowToast(hWnd, std::wstring(kItems[i].name) + (b == kDlgInstall ? T(S_INJECT_RESTART_HINT) : T(S_UNINSTALL_OK)));
            }
            else
            {
                ShowToast(hWnd, err);
            }
        }
        break;
    }
}

static void Activate(HWND hWnd, int i)
{
    const Item& it = kItems[i];
    if (it.key)
    {
        SetDialog(hWnd, i);
        return;
    }
        // Skills 管理
    if (wcscmp(it.name, L"Skills \u7BA1\u7406") == 0)
    {
        // 检查是否有工具已注入
        bool anyInjected = false;
        for (int j = 0; j < kItemCount; ++j)
            if (kItems[j].key && g_state[j] == kInstalled) { anyInjected = true; break; }
        if (!anyInjected)
        {
            ShowToast(hWnd, T(S_SKILLS_NEED_INJECT));
            return;
        }
        g_skillMgr = true;
        g_skillScroll = 0;
        InvalidateRect(hWnd, nullptr, FALSE);
        return;
    }
    if (!it.url[0])
    {
        ShowToast(hWnd, std::wstring(L"\u201C") + it.name + L"\u201D\u8FD8\u6CA1\u6709\u8BBE\u7F6E\u94FE\u63A5");
        return;
    }
    OpenUrl(hWnd, it.url);
}

static void SetMenuOpen(HWND hWnd, bool open)
{
    if (open == g_menuOpen) return;
    g_menuOpen = open;
    if (open) g_autostart = AutostartEnabled();     // 用户可能在任务管理器里改过
    InvalidateRect(hWnd, nullptr, FALSE);
}

static void RunCommand(HWND hWnd, int cmd)
{
    if (cmd >= kCmdTheme && cmd <= kCmdTheme + kThemeLight)
    {
        SetThemeMode(hWnd, cmd - kCmdTheme);
    }
    else if (cmd >= kCmdLang && cmd <= kCmdLang + kLangEN)
    {
        SetLang(hWnd, cmd - kCmdLang);
    }
    else if (cmd == kCmdTopmost)
    {
        g_topmost = !g_topmost;
        SetWindowPos(hWnd, g_topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    else if (cmd == kCmdAutostart)
    {
        if (SetAutostart(!g_autostart))
            g_autostart = !g_autostart;
        else
            ShowToast(hWnd, T(S_ERR_AUTOSTART));
    }
    else if (cmd == kCmdAbout)
    {
        g_aboutOpen = true;
        InvalidateRect(hWnd, nullptr, FALSE);
    }
    else if (cmd == kCmdCheckUpdate)
    {
        // 清掉旧结果再查，确保 About 卡片马上回到 "检查更新中..." 状态
        {
            std::lock_guard<std::mutex> lock(g_updateMutex);
            g_updateChecked = false;
            g_updateVersion.clear();
        }
        CheckUpdates(hWnd);
        ShowToast(hWnd, T(S_UPDATE_CHECK));
    }
    else if (cmd == kCmdCleanData)
    {
        int r = MessageBoxW(hWnd, T(S_CLEAN_DATA_CONFIRM), L"Ua", MB_YESNO | MB_ICONWARNING);
        if (r == IDYES)
        {
            // 删除 %APPDATA%\Ua 下的所有内容（备份、state、同步缓存等）
            std::wstring dir = AppDataDir();
            if (!dir.empty())
            {
                // 用 SHFileOperation 递归删除
                std::wstring from = dir + L'\0';     // 双 null 终止
                SHFILEOPSTRUCTW op{};
                op.wFunc = FO_DELETE;
                op.pFrom = from.c_str();
                op.fFlags = FOF_NO_UI | FOF_NOCONFIRMATION | FOF_SILENT;
                SHFileOperationW(&op);
            }
            // 也清掉注册表 HKCU\Software\Ua
            RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Ua");
            // 重新加载状态
            RefreshStates();
            InvalidateRect(hWnd, nullptr, FALSE);
            ShowToast(hWnd, T(S_CLEAN_DATA_OK));
        }
    }
    else if (cmd == kCmdExit)
    {
        g_realExit = true;
        DestroyWindow(hWnd);
    }
}

static void SetHover(HWND hWnd, int hover)
{
    if (hover == g_hover) return;
    g_hover = hover;
    InvalidateRect(hWnd, nullptr, FALSE);
}

// ---------------------------------------------------------------------------
// WndProc
// ---------------------------------------------------------------------------
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_NCCALCSIZE:
        if (wParam) return 0;               // 整个窗口都是客户区，不要系统标题栏和边框
        break;
    case WM_NCHITTEST:
        {
            POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hWnd, &p);
            // 标签、按钮和卡片可点击，其余区域可拖动窗口；菜单打开时整个窗口都要接收点击，用来关闭菜单
            return g_disclaimer || g_aboutOpen || g_skillMgr || g_menuOpen || g_dlg >= 0 || HitTest(p.x / g_scale, p.y / g_scale) >= 0 ? HTCLIENT : HTCAPTION;
        }
    case WM_NCLBUTTONDBLCLK:
        if (wParam == HTCAPTION) return 0;     // 禁止双击最大化
        break;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT && ((g_hover >= kHoverCard && g_hover < kHoverMenu) ||
            (g_hover >= kHoverDlg && g_hover < kHoverDlg + kDlgButtonCount && DlgEnabled(g_hover - kHoverDlg)) ||
            g_hover == kHoverAboutGithub || g_hover == kHoverAboutUpdate || g_hover == kHoverAboutUpdate2))
        {
            SetCursor(LoadCursor(nullptr, IDC_HAND));
            return TRUE;
        }
        break;
    case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hWnd, 0 };
            TrackMouseEvent(&tme);
            SetHover(hWnd, HitTest(GET_X_LPARAM(lParam) / g_scale, GET_Y_LPARAM(lParam) / g_scale));
        }
        return 0;
    case WM_MOUSELEAVE:
        SetHover(hWnd, -1);
        return 0;
    case WM_LBUTTONUP:
        {
            int hit = HitTest(GET_X_LPARAM(lParam) / g_scale, GET_Y_LPARAM(lParam) / g_scale);
            if (g_disclaimer)
            {
                if (hit == kHoverAccept)
                {
                    g_disclaimer = false;
                    InvalidateRect(hWnd, nullptr, FALSE);
                    // 免责声明确认后才检查更新和同步
                    CheckUpdates(hWnd);
                    SyncPrompts(hWnd);
                }
                return 0;
            }
            if (g_aboutOpen)
            {
                if (hit == kHoverAboutClose || hit == kHoverDlgOut)
                {
                    g_aboutOpen = false;
                    InvalidateRect(hWnd, nullptr, FALSE);
                }
                else if (hit == kHoverAboutGithub)
                {
                    OpenUrl(hWnd, L"https://github.com/freyrswift/UaStation");
                }
                else if (hit == kHoverAboutUpdate)
                {
                    OpenUrl(hWnd, kUpdateReleaseUrl);
                }
                else if (hit == kHoverAboutUpdate2 && !g_updateDownloading)
                {
                    ShowToast(hWnd, T(S_UPDATE_DOWNLOADING));
                    DownloadUpdate(hWnd);
                }
                return 0;
            }
            if (g_skillMgr)
            {
                if (hit == kHoverDlgOut || hit == kHoverSkillClose)
                {
                    g_skillMgr = false;
                    InvalidateRect(hWnd, nullptr, FALSE);
                }
                else if (hit == kHoverSkillFolder)
                {
                    // 打开第一个有 skills 目录的 AI 工具的 skills 文件夹
                    for (int j = 0; j < kItemCount; ++j)
                    {
                        if (!HasSkillsDir(j)) continue;
                        std::wstring dir = SkillsTargetDir(j);
                        if (!dir.empty())
                        {
                            SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);
                            ShellExecuteW(hWnd, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                            break;
                        }
                    }
                }
                else if (hit >= kHoverSkillRemove && hit < kHoverSkillRemove + kSkillCount)
                {
                    int sk = hit - kHoverSkillRemove;
                    int removed = UninstallSkillByName(kSkills[sk].dirName);
                    ShowToast(hWnd, removed > 0 ? T(S_SKILL_REMOVE_OK) : T(S_SKILL_REMOVE_NONE));
                    InvalidateRect(hWnd, nullptr, FALSE);
                }
                SetHover(hWnd, HitTest(GET_X_LPARAM(lParam) / g_scale, GET_Y_LPARAM(lParam) / g_scale));
                return 0;
            }
            if (g_dlg >= 0)
            {
                // 注入窗口打开时：点按钮执行，点窗口外关闭，其余点击忽略
                if (hit == kHoverDlgOut) SetDialog(hWnd, -1);
                else if (hit >= kHoverPrompt)
                {
                    int sel = hit - kHoverPrompt;
                    if (sel >= 0 && sel < (int)g_prompts.size()) g_promptSel = sel;
                    InvalidateRect(hWnd, nullptr, FALSE);
                }
                else if (hit >= kHoverDlg && hit < kHoverDlg + kDlgButtonCount) DialogCommand(hWnd, hit - kHoverDlg);
                SetHover(hWnd, HitTest(GET_X_LPARAM(lParam) / g_scale, GET_Y_LPARAM(lParam) / g_scale));
                return 0;
            }
            if (g_menuOpen && hit != kHoverGear)
            {
                // 菜单打开时：点菜单项执行命令，点菜单外关闭菜单，点击不再传给下面的卡片
                if (hit == kHoverMenuBg) return 0;
                SetMenuOpen(hWnd, false);
                if (hit >= kHoverMenu && hit < kHoverMenu + kMenuCount)
                    RunCommand(hWnd, kMenu[hit - kHoverMenu].cmd);
                SetHover(hWnd, HitTest(GET_X_LPARAM(lParam) / g_scale, GET_Y_LPARAM(lParam) / g_scale));
                return 0;
            }
            if (hit == kHoverGear)
            {
                SetMenuOpen(hWnd, !g_menuOpen);
            }
            else if (hit == kHoverMin)
            {
                SetHover(hWnd, -1);
                ShowWindow(hWnd, SW_MINIMIZE);
            }
            else if (hit == kHoverClose)
            {
                SetHover(hWnd, -1);
                ShowWindow(hWnd, SW_HIDE);
            }
            else if (hit >= kHoverCard)
            {
                Activate(hWnd, hit - kHoverCard);
            }
            else if (hit >= 0 && hit != g_tab)
            {
                g_tab = hit;
                InvalidateRect(hWnd, nullptr, FALSE);
            }
        }
        return 0;
    case WM_CONTEXTMENU:
    case WM_NCRBUTTONUP:
        if (g_dlg < 0) SetMenuOpen(hWnd, true);    // 右键任意位置也能打开设置菜单
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            if (g_disclaimer) { /* 免责声明不能 ESC 关 */ }
            else if (g_aboutOpen) { g_aboutOpen = false; InvalidateRect(hWnd, nullptr, FALSE); }
            else if (g_skillMgr) { g_skillMgr = false; InvalidateRect(hWnd, nullptr, FALSE); }
            else if (g_dlg >= 0) SetDialog(hWnd, -1);
            else if (g_menuOpen) SetMenuOpen(hWnd, false);
            else ShowWindow(hWnd, SW_HIDE);
        }
        return 0;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            SetMenuOpen(hWnd, false);
            g_skillMgr = false;
        }
        else
        {
            RefreshStates();            // 用户可能在外面手动改过提示词文件
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        break;
    case WM_SETTINGCHANGE:
        // 用户在系统设置里切换深浅色时会广播 "ImmersiveColorSet"
        if (g_themeMode == kThemeSystem && lParam && wcscmp(reinterpret_cast<const wchar_t*>(lParam), L"ImmersiveColorSet") == 0)
            ApplyTheme(hWnd);
        break;
    case WM_TIMER:
        if (wParam == kTimerToast)
        {
            KillTimer(hWnd, kTimerToast);
            g_toast.clear();
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        else if (wParam == kTimerApplyUpdate)
        {
            KillTimer(hWnd, kTimerApplyUpdate);
            ApplyUpdate(hWnd);
        }
        return 0;
    case kMsgUpdateReady:
        // 后台更新检查完成：有新版本时弹窗询问，用户同意再下载（避免悄悄替换 exe）
        InvalidateRect(hWnd, nullptr, FALSE);
        if (UpdateAvailable())
        {
            std::wstring newVer = GetUpdateVersion();
            wchar_t msg[256];
            swprintf_s(msg, 256, T(S_UPDATE_CONFIRM), newVer.c_str(), T(S_VERSION));
            if (MessageBoxW(hWnd, msg, L"Ua", MB_YESNO | MB_ICONINFORMATION) == IDYES)
            {
                ShowToast(hWnd, T(S_UPDATE_DOWNLOADING));
                DownloadUpdate(hWnd);
            }
        }
        return 0;
    case kMsgUpdateDownloaded:
        // 后台下载更新完成；wParam=1 成功 / 0 失败
        if (wParam)
        {
            ShowToast(hWnd, T(S_UPDATE_READY));
            SetTimer(hWnd, kTimerApplyUpdate, 2000, nullptr);
        }
        else
        {
            // 下载失败：弹窗询问是否打开 GitHub Releases 手动下载
            int r = MessageBoxW(hWnd, T(S_UPDATE_MANUAL_HINT), L"Ua",
                MB_YESNO | MB_ICONINFORMATION);
            if (r == IDYES)
                OpenUrl(hWnd, kUpdateReleaseUrl);
        }
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    case kMsgSyncReady:
        // 后台 prompts 同步完成；wParam 为更新文件数，>0 时弹 toast
        if (wParam > 0) ShowToast(hWnd, T(S_SYNC_UPDATED));
        return 0;
    case WM_SIZE:
        ResizeTarget(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_DPICHANGED:
        {
            g_scale = HIWORD(wParam) / 96.f;
            SetTargetDpi(96.f * g_scale);
            const RECT* r = reinterpret_cast<const RECT*>(lParam);
            SetWindowPos(hWnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    case WM_MOUSEWHEEL:
        if (g_skillMgr && kSkillCount > kSkillMaxVisible)
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int maxScroll = kSkillCount - kSkillMaxVisible;
            g_skillScroll = std::clamp(g_skillScroll - (delta > 0 ? 1 : -1), 0, maxScroll);
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        else if (g_dlg >= 0 && (int)g_prompts.size() > kDlgMaxVisible)
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int maxScroll = (int)g_prompts.size() - kDlgMaxVisible;
            g_promptScroll = std::clamp(g_promptScroll - (delta > 0 ? 1 : -1), 0, maxScroll);
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            Render(hWnd);
            EndPaint(hWnd, &ps);
        }
        return 0;
    case WM_CLOSE:
        // 系统关闭/Alt+F4 都走这里：没有真正退出标记时隐藏到托盘，程序继续运行
        if (!g_realExit)
        {
            ShowWindow(hWnd, SW_HIDE);
            return 0;
        }
        break;
    case kMsgTray:
        switch (LOWORD(lParam))
        {
        case WM_LBUTTONUP:
            ShowMainWindow(hWnd);
            break;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowTrayMenu(hWnd);
            break;
        }
        return 0;
    case WM_DESTROY:
        g_exiting = true;
        RemoveTray();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// ---------------------------------------------------------------------------
// wWinMain
// ---------------------------------------------------------------------------
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    hInst = hInstance;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // 单实例：如果已经有一个在跑，激活它的窗口，自己退出
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"UaToolbox_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        HWND existing = FindWindowW(L"UaToolbox", nullptr);
        if (existing)
        {
            ShowWindow(existing, SW_SHOW);
            SetForegroundWindow(existing);
        }
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    // \u63D0\u524D\u52A0\u8F7D\u8BED\u8A00\u8BBE\u7F6E\uFF0C\u8BA9\u4E0B\u9762\u7684 MessageBox \u7528\u6B63\u786E\u8BED\u8A00\u663E\u793A
    LoadLangSetting();

    // \u9996\u6B21\u542F\u52A8\u65F6\u4ECE exe \u8D44\u6E90\u91CA\u653E\u5185\u5D4C\u7684\u63D0\u793A\u8BCD\u5230 %APPDATA%/Ua/prompts/
    ExtractEmbeddedPrompts();

    // \u91CA\u653E\u540E\u5982\u679C\u4ECD\u4E3A\u7A7A\uFF0C\u63D0\u793A\u4F46\u7EE7\u7EED\u8FD0\u884C\uFF08Skills \u7BA1\u7406\u7B49\u529F\u80FD\u4ECD\u53EF\u7528\uFF09
    bool promptsEmpty = ListPrompts().empty();

    if (!CreateFactories())
    {
        MessageBoxW(nullptr, L"\u521D\u59CB\u5316 Direct2D / DirectWrite \u5931\u8D25\u3002", L"AI \u5DE5\u5177\u7BB1", MB_ICONERROR);
        return 1;
    }

    WNDCLASSEXW wcex{ sizeof(WNDCLASSEXW) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_UA));
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"UaToolbox";
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowExW(WS_EX_APPWINDOW, wcex.lpszClassName, L"Ua \u5DE5\u5177\u7BB1",
        WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX, 0, 0, 100, 100, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
        return 1;
    LoadThemeSetting();
    ApplyTheme(hWnd);

    g_autostart = AutostartEnabled();
    RefreshStates();
    SetupFrame(hWnd);
    PlaceWindow(hWnd);

    // 启动即添加托盘图标，窗口可见与否都保留
    AddTray(hWnd);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // prompts\ 文件夹存在但没有 .md：提示用户但继续运行（Skills 管理等功能仍可用）
    if (promptsEmpty)
        MessageBoxW(hWnd, T(S_WARN_PROMPTS_EMPTY), L"Ua", MB_ICONWARNING);

    // 每次启动都弹免责声明；更新检查在用户点"我已了解"后触发
    g_disclaimer = true;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
