// state.h : 全局运行时状态和用户设置
//

#pragma once

#include "config.h"

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// 全局状态变量（定义在 state.cpp）
// ---------------------------------------------------------------------------
extern HINSTANCE hInst;
extern int g_tab;
extern int g_hover;
extern bool g_topmost;
extern bool g_autostart;
extern bool g_menuOpen;
extern bool g_disclaimer;       // 免责声明弹窗
extern bool g_skillMgr;         // Skills 管理弹窗
extern int g_skillScroll;
extern bool g_aboutOpen;        // 关于弹窗
extern float g_scale;
extern int g_themeMode;
extern Palette P;
extern std::wstring g_toast;

// 注入对话框相关状态
extern int g_state[kItemCount];
extern int g_dlg;                           // 正在显示注入窗口的条目，-1 表示没有
extern std::vector<std::wstring> g_prompts; // 弹窗打开时列出的提示词文件名
extern int g_promptSel;                     // 选中的提示词序号
extern int g_promptScroll;                  // 列表滚动偏移（条目数）

// ---------------------------------------------------------------------------
// 主题（选择保存在 HKCU\Software\Ua\Theme）
// ---------------------------------------------------------------------------
bool SystemUsesLightTheme();
bool IsWindows11();
void ApplyTheme(HWND hWnd);
void LoadThemeSetting();
void SetThemeMode(HWND hWnd, int mode);

// Win11 Mica 效果开关：由 app.cpp 的 SetupFrame 在 Win11 上设置 DWMWA_SYSTEMBACKDROP_TYPE 成功后置 true
extern bool g_micaActive;
bool IsMicaActive();

// ---------------------------------------------------------------------------
// 语言
// ---------------------------------------------------------------------------
void LoadLangSetting();
void SetLang(HWND hWnd, int lang);

// ---------------------------------------------------------------------------
// 开机自启（HKCU\...\Run，不需要管理员权限）
// ---------------------------------------------------------------------------
bool AutostartEnabled();
bool SetAutostart(bool on);

// ---------------------------------------------------------------------------
// 免责声明接受状态
// ---------------------------------------------------------------------------
bool DisclaimerAccepted();
void AcceptDisclaimer();

// ---------------------------------------------------------------------------
// 自动更新检查
// g_updateChecked: 是否已完成一次检查（无论结果）
// g_updateVersion: 若有新版本，存储剥离 "v" 前缀后的版本号；无更新则为空
// 读写都走 g_updateMutex；也可用便捷函数 GetUpdateVersion / UpdateChecked
// ---------------------------------------------------------------------------
extern std::mutex g_updateMutex;
extern std::wstring g_updateVersion;
extern bool g_updateChecked;

std::wstring GetUpdateVersion();
bool UpdateChecked();
bool UpdateAvailable();
void CheckUpdates(HWND hWnd);
void DownloadUpdate(HWND hWnd);
void ApplyUpdate(HWND hWnd);

// 下载中标记（UI 可查询来禁止重复点击）
extern bool g_updateDownloading;

// 窗口已销毁标志：后台线程用它保护 PostMessage
extern std::atomic<bool> g_exiting;

// ---------------------------------------------------------------------------
// 后台同步 prompts\ 文件夹
// 启动时调用一次：拉 GitHub Contents API，有新 .md 就下载到 exe 旁边的
// prompts\ 目录。失败静默。完成后 PostMessage(kMsgSyncReady, updated, 0)。
// ---------------------------------------------------------------------------
void SyncPrompts(HWND hWnd);

// 从 .md 文件首行解析 <!-- Ua-Prompt-Version: X.Y.Z -->。
// 找不到返回空字符串。
std::wstring ParsePromptVersion(const std::string& content);
