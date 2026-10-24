// config.h : 常量、枚举、翻译表与数据表（纯只读，所有 .cpp 都能包含）
//

#pragma once

#include "framework.h"
#include "Resource.h"

// ---------------------------------------------------------------------------
// 布局常量（单位: DIP，96 DPI 下的像素）
// ---------------------------------------------------------------------------
inline constexpr float kW = 460.f;
inline constexpr float kH = 524.f;
inline constexpr float kPad = 14.f;
inline constexpr float kTop = 68.f;            // 内容区起始 y
inline constexpr float kCardH = 48.f;
inline constexpr float kGap = 8.f;
inline constexpr float kStatusH = 18.f;    // 底部状态栏高度
inline constexpr UINT  kTimerToast = 1;
inline constexpr UINT  kTimerApplyUpdate = 2;
inline constexpr int   kCmdTopmost = 1001;
inline constexpr int   kCmdAutostart = 1002;
inline constexpr int   kCmdAbout = 1003;
inline constexpr int   kCmdExit = 1004;
inline constexpr int   kCmdCheckUpdate = 1005;
inline constexpr int   kCmdTrayShow = 1006;
inline constexpr int   kCmdCleanData = 1007;
inline constexpr int   kCmdTheme = 2000;       // 2000 + ThemeMode
inline constexpr int   kCmdLang = 3000;        // 3000 + Lang

// 后台线程查到更新结果时 PostMessage 给主窗口
inline constexpr UINT  kMsgUpdateReady = WM_APP + 1;
// 后台同步 prompts\ 完成时 PostMessage 给主窗口；wParam = 新增/更新的文件数
inline constexpr UINT  kMsgSyncReady = WM_APP + 2;
// 后台下载更新完成时 PostMessage 给主窗口；wParam=1 成功 / 0 失败
inline constexpr UINT  kMsgUpdateDownloaded = WM_APP + 3;
// 托盘图标回调消息；lParam 低字记录鼠标事件（WM_LBUTTONUP / WM_RBUTTONUP / WM_CONTEXTMENU）
inline constexpr UINT  kMsgTray = WM_APP + 10;

// 更新检查相关 URL（仓库尚不存在，先用占位）
inline constexpr const wchar_t* kUpdateApiUrl = L"https://api.github.com/repos/freyrswift/UaStation/releases/latest";
inline constexpr const wchar_t* kUpdateReleaseUrl = L"https://github.com/freyrswift/UaStation/releases/latest";
inline constexpr const wchar_t* kUpdateDownloadUrl = L"https://github.com/freyrswift/UaStation/releases/latest/download/Ua.exe";

// prompts\ 同步用 GitHub Contents API（仓库尚不存在，占位）
inline constexpr const wchar_t* kPromptsApiHost = L"api.github.com";
inline constexpr const wchar_t* kPromptsApiPath = L"/repos/freyrswift/UaStation/contents/prompts";

// 配色。背景色取自系统：Win11 Mica 底色 #202020 / #F3F3F3，Win10 深浅色窗口底色 #1F1F1F / #F2F2F2
struct Palette
{
    bool dark;
    UINT32 bg, text, sub;
};

inline constexpr UINT32 kGreen = 0x2EBD59;     // 已注入的绿点
inline constexpr Palette kDark = { true, 0x202020, 0xFFFFFF, 0xC5C5C5 };
inline constexpr Palette kLight = { false, 0xF3F3F3, 0x1A1A1A, 0x5F5F5F };

enum ThemeMode { kThemeSystem = 0, kThemeDark = 1, kThemeLight = 2 };

// 语言
enum Lang { kLangCN = 0, kLangEN = 1 };

// Skill 分类
enum SkillCategory {
    kCatAll, kCatReverse, kCatGame, kCatKernel, kCatExploit,
    kCatCrack, kCatMobile, kCatPentest, kCatAi, kCatOther,
    kCatCount
};

// 当前语言（在 state.cpp 定义，T() 需要它）
extern int g_lang;

// prompts 同步完成时的 toast 文本覆盖（在 state.cpp 定义）。
// 为 nullptr 时 T(S_SYNC_UPDATED) 返回默认的 "提示词已更新"；非空时返回
// 格式化后的 "N 个提示词已更新"。缓冲区生命周期由 state.cpp 管理。
extern const wchar_t* g_syncToastOverride;

// 翻译表：[langId][stringId]
enum Str {
    S_THEME_SYSTEM, S_THEME_DARK, S_THEME_LIGHT,
    S_TOPMOST, S_AUTOSTART, S_ABOUT, S_EXIT,
    S_TAB_EDITOR, S_TAB_TOOLS,
    S_THEME, S_LANG,
    S_LANG_CN, S_LANG_EN,
    S_DISCLAIMER_TITLE, S_DISCLAIMER_BODY, S_DISCLAIMER_BTN,
    S_NOT_SUPPORTED, S_TOOL_NOT_FOUND, S_INJECTED, S_FILE_HAS_CONTENT, S_NOT_INJECTED,
    S_LOCATION, S_OPEN, S_SELECT_PROMPT, S_NO_PROMPTS,
    S_WEBSITE, S_UNINSTALL, S_INJECT,
    S_INJECT_OK, S_UNINSTALL_OK, S_NO_LINK,
    S_SKILLS_TITLE, S_SKILLS_SUB, S_SKILLS_FOLDER, S_SKILLS_NEED_INJECT,
    S_SKILL_REMOVE_OK, S_SKILL_REMOVE_NONE,
    S_TOOLS_SKILLS, S_TOOLS_SKILLS_SUB, S_TOOLS_RELAY, S_TOOLS_RELAY_SUB,
    S_TOOLS_TUTORIAL, S_TOOLS_TUTORIAL_SUB, S_TOOLS_HELP, S_TOOLS_HELP_SUB,
    S_TOOLS_QQ, S_TOOLS_QQ_SUB,
    S_ABOUT_TEXT, S_ABOUT_LEGAL, S_VERSION,
    S_ERR_NO_PROMPT_SEL, S_ERR_READ_PROMPT, S_ERR_UTF16_PROMPT, S_ERR_EMPTY_PROMPT,
    S_ERR_READ_TARGET, S_ERR_SAVE_STATE, S_ERR_WRITE_TARGET,
    S_ERR_NO_STATE, S_ERR_RESTORE, S_ERR_AUTOSTART,
    S_ERR_OPEN_URL,
    S_GITHUB_LABEL, S_UPDATE_CHECK, S_UPDATE_NEW, S_UPDATE_LATEST, S_CHECK_UPDATE,
    S_ERR_PROMPTS_MISSING, S_WARN_PROMPTS_EMPTY,
    S_SKILL_SEARCH_PLACEHOLDER,
    S_CAT_ALL, S_CAT_REVERSE, S_CAT_GAME, S_CAT_KERNEL, S_CAT_EXPLOIT,
    S_CAT_CRACK, S_CAT_MOBILE, S_CAT_PENTEST, S_CAT_AI, S_CAT_OTHER,
    S_SYNC_CHECKING, S_SYNC_UPDATED, S_PROMPTS_UPDATED_COUNT,
    S_TRAY_SHOW, S_TRAY_TOOLTIP,
    S_UPDATE_DOWNLOADING, S_UPDATE_READY, S_UPDATE_BTN,
    S_INJECT_RESTART_HINT, S_UPDATE_MANUAL_HINT, S_WARN_FILE_MODIFIED, S_CLEAN_DATA, S_CLEAN_DATA_CONFIRM, S_CLEAN_DATA_OK,
    S_STATUS_INSTALLED, S_STATUS_SKILLS,
    S_CONFIRM_UNINSTALL,
    S_UPDATE_CONFIRM,
    S_SKILL_COUNT_FMT, S_SKILL_TOOL_COUNT_FMT, S_SKILL_NO_MATCH,
    S_COUNT
};

inline const wchar_t* const kStr[][2] = {
    /* S_THEME_SYSTEM */    { L"\u8DDF\u968F\u7CFB\u7EDF",   L"System" },
    /* S_THEME_DARK */      { L"\u6DF1\u8272",       L"Dark" },
    /* S_THEME_LIGHT */     { L"\u6D45\u8272",       L"Light" },
    /* S_TOPMOST */         { L"\u7A97\u53E3\u7F6E\u9876",   L"Always on Top" },
    /* S_AUTOSTART */       { L"\u5F00\u673A\u81EA\u542F",   L"Start on Boot" },
    /* S_ABOUT */           { L"\u5173\u4E8E",       L"About" },
    /* S_EXIT */            { L"\u9000\u51FA",       L"Exit" },
    /* S_TAB_EDITOR */      { L"\u7F16\u8F91\u5668",     L"Editor" },
    /* S_TAB_TOOLS */       { L"\u5DE5\u5177",       L"Tools" },
    /* S_THEME */           { L"\u4E3B\u9898",       L"Theme" },
    /* S_LANG */            { L"\u8BED\u8A00",       L"Language" },
    /* S_LANG_CN */         { L"\u4E2D\u6587",       L"\u4E2D\u6587" },
    /* S_LANG_EN */         { L"English",    L"English" },
    /* S_DISCLAIMER_TITLE */{ L"\u514D\u8D23\u58F0\u660E",   L"Disclaimer" },
    /* S_DISCLAIMER_BODY */ { L"\u672C\u5DE5\u5177\u5B8C\u5168\u514D\u8D39\uFF0C\u4EC5\u4F9B\u5B66\u4E60\u548C\u6388\u6743\u5B89\u5168\u7814\u7A76\u4F7F\u7528\u3002\n\n1. \u7528\u6237\u5E94\u786E\u4FDD\u5728\u5408\u6CD5\u6388\u6743\u8303\u56F4\u5185\u4F7F\u7528\u672C\u5DE5\u5177\uFF0C\u5BF9\u4F7F\u7528\u672C\u5DE5\u5177\u6240\u4EA7\u751F\u7684\u4E00\u5207\u540E\u679C\u81EA\u884C\u627F\u62C5\u8D23\u4EFB\u3002\n\n2. \u672C\u5DE5\u5177\u4E0D\u6536\u53D6\u4EFB\u4F55\u8D39\u7528\u3002\u5982\u679C\u4F60\u662F\u82B1\u94B1\u8D2D\u4E70\u7684\uFF0C\u8BF4\u660E\u4F60\u88AB\u9A97\u4E86\uFF0C\u8BF7\u7ACB\u5373\u9000\u6B3E\u3002\n\n3. \u4F5C\u8005\u4E0D\u5BF9\u56E0\u4F7F\u7528\u672C\u5DE5\u5177\u5BFC\u81F4\u7684\u4EFB\u4F55\u76F4\u63A5\u6216\u95F4\u63A5\u635F\u5931\u627F\u62C5\u8D23\u4EFB\u3002\n\n4. \u7EE7\u7EED\u4F7F\u7528\u5373\u8868\u793A\u4F60\u5DF2\u9605\u8BFB\u5E76\u540C\u610F\u4EE5\u4E0A\u6761\u6B3E\u3002",
                              L"This tool is completely free and is intended solely for learning and authorized security research.\n\n"
                              L"1. Users must ensure they use this tool within legally authorized scope and bear full responsibility for any consequences.\n\n"
                              L"2. This tool is free of charge. If you paid for it, you were scammed \u2014 request a refund immediately.\n\n"
                              L"3. The author is not liable for any direct or indirect damages caused by using this tool.\n\n"
                              L"4. By continuing, you agree to the above terms." },
    /* S_DISCLAIMER_BTN */  { L"\u6211\u5DF2\u4E86\u89E3",   L"I Understand" },
    /* S_NOT_SUPPORTED (auto-install) */   { L"\u6682\u4E0D\u652F\u6301\u81EA\u52A8\u5B89\u88C5", L"Auto-install not supported" },
    /* S_TOOL_NOT_FOUND */  { L"\u672A\u68C0\u6D4B\u5230\u8BE5\u5DE5\u5177", L"Tool not detected" },
    /* S_INJECTED (installed) */        { L"\u5DF2\u5B89\u88C5",     L"Installed" },
    /* S_FILE_HAS_CONTENT */{ L"\u6587\u4EF6\u5DF2\u6709\u5185\u5BB9", L"File has content" },
    /* S_NOT_INJECTED (not installed) */    { L"\u672A\u5B89\u88C5",     L"Not installed" },
    /* S_LOCATION */        { L"\u4F4D\u7F6E",       L"Path" },
    /* S_OPEN */            { L"\u6253\u5F00",       L"Open" },
    /* S_SELECT_PROMPT */   { L"\u9009\u62E9\u63D0\u793A\u8BCD", L"Select Prompt" },
    /* S_NO_PROMPTS */      { L"prompts\\ \u6587\u4EF6\u5939\u91CC\u6CA1\u6709 .md \u6587\u4EF6", L"No .md files in prompts\\ folder" },
    /* S_WEBSITE */         { L"\u5B98\u7F51",       L"Site" },
    /* S_UNINSTALL */       { L"\u5378\u8F7D",       L"Remove" },
    /* S_INJECT */          { L"\u5B89\u88C5",       L"Install" },
    /* S_INJECT_OK */       { L" \u5B89\u88C5\u6210\u529F",  L" installed" },
    /* S_UNINSTALL_OK */    { L" \u5DF2\u5378\u8F7D",    L" removed" },
    /* S_NO_LINK */         { L"\u8FD8\u6CA1\u6709\u8BBE\u7F6E\u94FE\u63A5", L"No link configured" },
    /* S_SKILLS_TITLE */    { L"Skills \u7BA1\u7406", L"Skills Manager" },
    /* S_SKILLS_SUB */      { L"\u4E2A\u5185\u5D4C skill", L" embedded skills" },
    /* S_SKILLS_FOLDER */   { L"\u6253\u5F00 Skills \u6587\u4EF6\u5939", L"Open Skills Folder" },
    /* S_SKILLS_NEED_INJECT */ { L"\u8BF7\u5148\u5B89\u88C5\u63D0\u793A\u8BCD\u540E\u518D\u67E5\u770B", L"Please install a prompt first" },
    /* S_SKILL_REMOVE_OK */ { L"\u5DF2\u4ECE\u6240\u6709\u5DE5\u5177\u4E2D\u79FB\u9664", L"Removed from all tools" },
    /* S_SKILL_REMOVE_NONE */ { L"\u6CA1\u6709\u9700\u8981\u79FB\u9664\u7684", L"Nothing to remove" },
    /* S_TOOLS_SKILLS */    { L"Skills \u7BA1\u7406", L"Skills Manager" },
    /* S_TOOLS_SKILLS_SUB */{ L"\u7BA1\u7406\u5DF2\u5B89\u88C5\u7684 Skills", L"Manage installed skills" },
    /* S_TOOLS_RELAY */     { L"\u6EE1\u8840\u4E2D\u8F6C\u7AD9", L"API Relay" },
    /* S_TOOLS_RELAY_SUB */ { L"API \u4E2D\u8F6C\u670D\u52A1", L"API relay service" },
    /* S_TOOLS_TUTORIAL */  { L"\u4F7F\u7528\u6559\u7A0B", L"Tutorial" },
    /* S_TOOLS_TUTORIAL_SUB */ { L"\u4ECE\u5B89\u88C5\u5230\u4E0A\u624B", L"Getting started guide" },
    /* S_TOOLS_HELP */      { L"\u4E0D\u4F1A\u7528\u70B9\u8FD9\u91CC", L"Need Help?" },
    /* S_TOOLS_HELP_SUB */  { L"\u5E38\u89C1\u95EE\u9898\u4E0E\u6C42\u52A9", L"FAQ and support" },
    /* S_TOOLS_QQ */        { L"\u52A0\u5165QQ\u7FA4", L"Join QQ Group" },
    /* S_TOOLS_QQ_SUB */    { L"\u4EA4\u6D41\u4E0E\u53CD\u9988", L"Chat and feedback" },
    /* S_ABOUT_TEXT */      { L"Ua \u5DE5\u5177\u7BB1  \u7248\u672C ", L"Ua Toolbox  Version " },
    /* S_ABOUT_LEGAL */     { L"\u672C\u5DE5\u5177\u4EC5\u4F9B\u5B66\u4E60\u4E0E\u6388\u6743\u5B89\u5168\u7814\u7A76\u4F7F\u7528\u3002\u7528\u6237\u5E94\u5728\u5408\u6CD5\u6388\u6743\u8303\u56F4\u5185\u4F7F\u7528\uFF0C\u4F5C\u8005\u4E0D\u627F\u62C5\u4EFB\u4F55\u6CD5\u5F8B\u8D23\u4EFB\u3002\u672C\u5DE5\u5177\u5B8C\u5168\u514D\u8D39\u4E14\u5F00\u6E90\u3002", L"For educational and authorized security research only. Users are responsible for legal compliance. The author assumes no liability. This tool is free and open source." },
    /* S_VERSION */         { L"1.0.2", L"1.0.2" },
    /* S_ERR_NO_PROMPT_SEL */ { L"\u8BF7\u5148\u9009\u62E9\u4E00\u4E2A\u63D0\u793A\u8BCD", L"Please select a prompt first" },
    /* S_ERR_READ_PROMPT */ { L"\u8BFB\u53D6\u63D0\u793A\u8BCD\u6587\u4EF6\u5931\u8D25", L"Failed to read prompt file" },
    /* S_ERR_UTF16_PROMPT */{ L"\u63D0\u793A\u8BCD\u6587\u4EF6\u8BF7\u4FDD\u5B58\u4E3A UTF-8 \u7F16\u7801", L"Please save prompt file as UTF-8" },
    /* S_ERR_EMPTY_PROMPT */{ L"\u63D0\u793A\u8BCD\u6587\u4EF6\u662F\u7A7A\u7684", L"Prompt file is empty" },
    /* S_ERR_READ_TARGET */ { L"\u8BFB\u53D6\u76EE\u6807\u6587\u4EF6\u5931\u8D25", L"Failed to read target file" },
    /* S_ERR_SAVE_STATE */  { L"\u4FDD\u5B58\u72B6\u6001\u5931\u8D25", L"Failed to save state" },
    /* S_ERR_WRITE_TARGET */{ L"\u5199\u5165\u76EE\u6807\u6587\u4EF6\u5931\u8D25", L"Failed to write target file" },
    /* S_ERR_NO_STATE */    { L"\u8FD9\u4E2A\u5E94\u7528\u8FD8\u6CA1\u6709\u901A\u8FC7\u672C\u5DE5\u5177\u6CE8\u5165\u8FC7", L"This app has not been injected" },
    /* S_ERR_RESTORE */     { L"\u6062\u590D\u5907\u4EFD\u5931\u8D25", L"Failed to restore backup" },
    /* S_ERR_AUTOSTART */   { L"\u8BBE\u7F6E\u5F00\u673A\u81EA\u542F\u5931\u8D25", L"Failed to set autostart" },
    /* S_ERR_OPEN_URL */    { L"\u6253\u5F00\u94FE\u63A5\u5931\u8D25", L"Failed to open link" },
    /* S_GITHUB_LABEL */    { L"GitHub \u4ED3\u5E93", L"GitHub Repository" },
    /* S_UPDATE_CHECK */    { L"\u68C0\u67E5\u66F4\u65B0\u4E2D...", L"Checking for updates..." },
    /* S_UPDATE_NEW */      { L"\u53D1\u73B0\u65B0\u7248\u672C\uFF1A", L"New version available: " },
    /* S_UPDATE_LATEST */   { L"\u5DF2\u662F\u6700\u65B0\u7248\u672C", L"Already up to date" },
    /* S_CHECK_UPDATE */    { L"\u68C0\u67E5\u66F4\u65B0",     L"Check for Updates" },
    /* S_ERR_PROMPTS_MISSING */ { L"prompts \u6587\u4EF6\u5939\u4E22\u5931\uFF0C\u8BF7\u91CD\u65B0\u4E0B\u8F7D\u5B8C\u6574\u7248\u672C", L"prompts folder missing, please re-download" },
    /* S_WARN_PROMPTS_EMPTY */ { L"prompts \u6587\u4EF6\u5939\u662F\u7A7A\u7684", L"prompts folder is empty" },
    /* S_SKILL_SEARCH_PLACEHOLDER */ { L"\u641C\u7D22...",        L"Search..." },
    /* S_CAT_ALL */                  { L"\u5168\u90E8",           L"All" },
    /* S_CAT_REVERSE */              { L"\u9006\u5411",           L"Reverse" },
    /* S_CAT_GAME */                 { L"\u6E38\u620F",           L"Game" },
    /* S_CAT_KERNEL */               { L"\u5185\u6838",           L"Kernel" },
    /* S_CAT_EXPLOIT */              { L"\u6F0F\u6D1E",           L"Exploit" },
    /* S_CAT_CRACK */                { L"\u7834\u89E3",           L"Crack" },
    /* S_CAT_MOBILE */               { L"\u79FB\u52A8",           L"Mobile" },
    /* S_CAT_PENTEST */              { L"\u6E17\u900F",           L"Pentest" },
    /* S_CAT_AI */                   { L"AI",                     L"AI" },
    /* S_CAT_OTHER */                { L"\u5176\u4ED6",           L"Other" },
    /* S_SYNC_CHECKING */            { L"\u6B63\u5728\u540C\u6B65\u63D0\u793A\u8BCD...", L"Syncing prompts..." },
    /* S_SYNC_UPDATED */             { L"\u63D0\u793A\u8BCD\u5DF2\u66F4\u65B0",         L"Prompts updated" },
    /* S_PROMPTS_UPDATED_COUNT */    { L"%d \u4E2A\u63D0\u793A\u8BCD\u5DF2\u66F4\u65B0", L"%d prompts updated" },
    /* S_TRAY_SHOW */                { L"\u663E\u793A\u7A97\u53E3",                     L"Show Window" },
    /* S_TRAY_TOOLTIP */             { L"Ua \u5DE5\u5177\u7BB1",                     L"Ua Toolbox" },
    /* S_UPDATE_DOWNLOADING */       { L"\u6B63\u5728\u4E0B\u8F7D\u66F4\u65B0...",   L"Downloading update..." },
    /* S_UPDATE_READY */             { L"\u4E0B\u8F7D\u5B8C\u6210\uFF0C\u5373\u5C06\u91CD\u542F", L"Download complete, restarting..." },
    /* S_UPDATE_BTN */               { L"\u7ACB\u5373\u66F4\u65B0",                   L"Update Now" },
    /* S_INJECT_RESTART_HINT */ { L" \u5B89\u88C5\u6210\u529F\uFF0C\u8BF7\u91CD\u542F AI \u5DE5\u5177\u751F\u6548", L" installed, please restart the AI tool" },
    /* S_UPDATE_MANUAL_HINT */ { L"\u81EA\u52A8\u66F4\u65B0\u4E0B\u8F7D\u5931\u8D25\u3002\u662F\u5426\u6253\u5F00\u6D4F\u89C8\u5668\u624B\u52A8\u4E0B\u8F7D\u6700\u65B0\u7248\u672C\uFF1F", L"Auto-update download failed. Open browser to download the latest version?" },
    /* S_WARN_FILE_MODIFIED */ { L"\u8BE5\u6587\u4EF6\u5728\u5B89\u88C5\u540E\u88AB\u4FEE\u6539\u8FC7\uFF0C\u5378\u8F7D\u5C06\u6062\u590D\u5230\u5B89\u88C5\u524D\u7684\u7248\u672C\u3002\u786E\u5B9A\u7EE7\u7EED\uFF1F", L"This file was modified after installation. Uninstalling will restore the pre-install version. Continue?" },
    /* S_CLEAN_DATA */ { L"\u6E05\u7406\u6240\u6709\u6570\u636E", L"Clean All Data" },
    /* S_CLEAN_DATA_CONFIRM */ { L"\u5C06\u5220\u9664\u6240\u6709\u5907\u4EFD\u3001\u72B6\u6001\u548C\u8BBE\u7F6E\u3002\u786E\u5B9A\uFF1F", L"This will delete all backups, state and settings. Continue?" },
    /* S_CLEAN_DATA_OK */ { L"\u6570\u636E\u5DF2\u6E05\u7406", L"All data cleaned" },
    /* S_STATUS_INSTALLED */          { L"\u5DF2\u5B89\u88C5",                       L"installed" },
    /* S_STATUS_SKILLS */             { L"\u6280\u80FD",                             L"skills" },
    /* S_CONFIRM_UNINSTALL */         { L"\u786E\u8BA4\u5378\u8F7D\uFF1F\u8FD9\u5C06\u6062\u590D\u539F\u59CB\u914D\u7F6E\u6587\u4EF6\u3002", L"Confirm uninstall? This will restore the original config file." },
    /* S_UPDATE_CONFIRM */            { L"\u53D1\u73B0\u65B0\u7248\u672C %s\uFF08\u5F53\u524D %s\uFF09\uFF0C\u7ACB\u5373\u4E0B\u8F7D\u5E76\u66F4\u65B0\uFF1F", L"New version %s available (current %s). Download and update now?" },
    /* S_SKILL_COUNT_FMT */           { L"\u5171 %d / %d \u4E2A skill", L"%d of %d skills" },
    /* S_SKILL_TOOL_COUNT_FMT */      { L"%d \u4E2A\u5DE5\u5177", L"%d tools" },
    /* S_SKILL_NO_MATCH */            { L"\u6CA1\u6709\u5339\u914D\u7684 skill", L"No matching skills" },
};

inline const wchar_t* T(Str s)
{
    // 后台同步发现版本变更时，S_SYNC_UPDATED 走格式化后的 "N 个提示词已更新"
    if (s == S_SYNC_UPDATED && g_syncToastOverride)
        return g_syncToastOverride;
    return kStr[s][g_lang];
}

// 字体：西文和数字用 Tahoma（Windows 自带），Tahoma 没有中文，中文交给微软雅黑 UI
inline constexpr const wchar_t* kFontFamily = L"Tahoma";
inline constexpr const wchar_t* kCjkFamily = L"Microsoft YaHei UI";
inline constexpr const wchar_t* kIconFamily = L"Segoe MDL2 Assets";

// ---------------------------------------------------------------------------
// 条目
// ---------------------------------------------------------------------------
enum Group { kGroupEditor, kGroupTools };

inline const wchar_t* const kTabs[] = { L"\u7F16\u8F91\u5668", L"\u5DE5\u5177" };
inline constexpr int kTabCount = ARRAYSIZE(kTabs);    // 第 i 个标签对应 Group(i)

struct Item
{
    const wchar_t* name;
    const wchar_t* sub;
    Group group;
    int logo;               // SVG 资源 ID；0 表示用 glyph
    bool framed;            // logo 自带方形底板，铺满图标位，不再加底色
    const wchar_t* glyph;   // Segoe MDL2 Assets 图标
    const wchar_t* url;     // 官网 / 点击后打开的链接；空字符串表示还没设置

    // 以下只有 AI 条目才填。key 非空时点击弹出注入窗口，否则直接打开 url
    const wchar_t* key = nullptr;       // 提示词文件名：prompts\<key>.md
    const wchar_t* target = nullptr;    // 全局提示词文件，相对用户目录；为空表示不支持自动注入
    const wchar_t* note = nullptr;      // 不支持时显示的原因

    // skills 目录（相对用户目录），如 ".codex\\skills"；为空则该工具不支持 skills
    const wchar_t* skillsDir = nullptr;
};

// 内嵌的 skill 文件（从 seep-kit 复制，RCDATA 资源）
struct SkillDef
{
    int resId;
    const wchar_t* dirName;     // 安装到 skills 目录下的文件夹名
};

inline const SkillDef kSkills[] = {
    { IDR_SKILL_AI_LLM_SECURITY, L"ai-llm-security" },
    { IDR_SKILL_APK_REVERSE, L"apk-reverse" },
    { IDR_SKILL_APKSEEP, L"apkseep" },
    { IDR_SKILL_CHEAT_ENGINE, L"cheat-engine" },
    { IDR_SKILL_CLIENT_LICENSE_VALIDATION_BYPASS, L"client-license-validation-bypass" },
    { IDR_SKILL_DMA_ATTACK, L"dma-attack" },
    { IDR_SKILL_EXPLOIT_DEVELOPMENT, L"exploit-development" },
    { IDR_SKILL_IDA_REVERSE, L"ida-reverse" },
    { IDR_SKILL_MALWARE_ANALYSIS, L"malware-analysis" },
    { IDR_SKILL_MCP_JS_REVERSE_PLAYBOOK, L"mcp-js-reverse-playbook" },
    { IDR_SKILL_MOBILE_SECURITY, L"mobile-security" },
    { IDR_SKILL_PROCESS_INJECTION_DETECT, L"process-injection-detect" },
    { IDR_SKILL_RADARE2, L"radare2" },
    { IDR_SKILL_RE_ANGR, L"re-angr" },
    { IDR_SKILL_RE_ANTI_CHEAT, L"re-anti-cheat" },
    { IDR_SKILL_RE_CONSOLE, L"re-console" },
    { IDR_SKILL_RE_CRASH_TRIAGE, L"re-crash-triage" },
    { IDR_SKILL_RE_DEOBFUSCATE, L"re-deobfuscate" },
    { IDR_SKILL_RE_DRM, L"re-drm" },
    { IDR_SKILL_RED_TEAM_OPS, L"red-team-ops" },
    { IDR_SKILL_RE_EBPF, L"re-ebpf" },
    { IDR_SKILL_RE_EVASION, L"re-evasion" },
    { IDR_SKILL_RE_EXPLOIT, L"re-exploit" },
    { IDR_SKILL_RE_FRIDA, L"re-frida" },
    { IDR_SKILL_RE_FUZZING, L"re-fuzzing" },
    { IDR_SKILL_RE_GAME, L"re-game" },
    { IDR_SKILL_RE_HYPERVISOR, L"re-hypervisor" },
    { IDR_SKILL_RE_KERNEL, L"re-kernel" },
    { IDR_SKILL_RE_KEYGEN, L"re-keygen" },
    { IDR_SKILL_RE_LICENSE, L"re-license" },
    { IDR_SKILL_RE_PACKER_ID, L"re-packer-id" },
    { IDR_SKILL_RE_PATCHING, L"re-patching" },
    { IDR_SKILL_RE_PWN, L"re-pwn" },
    { IDR_SKILL_RE_SHELLCODE, L"re-shellcode" },
    { IDR_SKILL_RE_UNPACK, L"re-unpack" },
    { IDR_SKILL_REVERSE_ENGINEERING, L"reverse-engineering" },
    { IDR_SKILL_SOFTSEEP, L"softseep" },
    { IDR_SKILL_THREAT_HUNTING, L"threat-hunting" },
    { IDR_SKILL_UNITY_GAME_RESEARCH, L"unity-game-research" },
    { IDR_SKILL_WEB_SECURITY, L"web-security" },
};
inline constexpr int kSkillCount = ARRAYSIZE(kSkills);

// 每个 skill 的分类（下标与 kSkills[] 一一对应）
inline constexpr SkillCategory kSkillCategories[] = {
    kCatAi,       // ai-llm-security
    kCatMobile,   // apk-reverse
    kCatMobile,   // apkseep
    kCatReverse,  // cheat-engine
    kCatCrack,    // client-license-validation-bypass
    kCatGame,     // dma-attack
    kCatExploit,  // exploit-development
    kCatReverse,  // ida-reverse
    kCatPentest,  // malware-analysis
    kCatMobile,   // mcp-js-reverse-playbook
    kCatMobile,   // mobile-security
    kCatOther,    // process-injection-detect
    kCatReverse,  // radare2
    kCatOther,    // re-angr
    kCatGame,     // re-anti-cheat
    kCatGame,     // re-console
    kCatExploit,  // re-crash-triage
    kCatCrack,    // re-deobfuscate
    kCatCrack,    // re-drm
    kCatPentest,  // red-team-ops
    kCatKernel,   // re-ebpf
    kCatPentest,  // re-evasion
    kCatExploit,  // re-exploit
    kCatPentest,  // re-frida
    kCatExploit,  // re-fuzzing
    kCatGame,     // re-game
    kCatKernel,   // re-hypervisor
    kCatKernel,   // re-kernel
    kCatCrack,    // re-keygen
    kCatCrack,    // re-license
    kCatCrack,    // re-packer-id
    kCatCrack,    // re-patching
    kCatExploit,  // re-pwn
    kCatExploit,  // re-shellcode
    kCatCrack,    // re-unpack
    kCatReverse,  // reverse-engineering
    kCatReverse,  // softseep
    kCatPentest,  // threat-hunting
    kCatGame,     // unity-game-research
    kCatPentest,  // web-security
};
static_assert(ARRAYSIZE(kSkillCategories) == kSkillCount, "kSkillCategories must match kSkills");

inline const Item kItems[] = {
    // 全局提示词位置（按官方文档 + gpt6-Astra seats.py 交叉核对）
    // 检测逻辑在 inject.cpp ToolDetected()：检查目标路径的父目录是否存在
    { L"Claude",           L"", kGroupEditor, IDR_LOGO_CLAUDE,    false, nullptr, L"https://docs.anthropic.com/en/docs/claude-code/overview",
        L"claude",    L".claude\\CLAUDE.md", nullptr, L".claude\\skills" },
    { L"DeepSeek Harness", L"", kGroupEditor, IDR_LOGO_DEEPSEEK,  false, nullptr, L"",
        L"deepseek",  L".dsh\\AGENTS.md", nullptr, L".dsh\\skills" },
    { L"Skills \u7BA1\u7406",      L"\u7BA1\u7406\u5DF2\u5B89\u88C5\u7684 Skills", kGroupTools, 0, false, L"\uE8FD", L"" },     // BulletedList
    { L"\u6EE1\u8840\u4E2D\u8F6C\u7AD9",       L"API \u4E2D\u8F6C\u670D\u52A1",    kGroupTools,     0, false, L"\uE774", L"https://utlra.cc/" },     // Globe
    { L"\u4F7F\u7528\u6559\u7A0B",         L"\u4ECE\u5B89\u88C5\u5230\u4E0A\u624B",    kGroupTools,     0, false, L"\uE82D", L"https://github.com/freyrswift/UaStation#usage" },     // Dictionary
    { L"\u4E0D\u4F1A\u7528\u70B9\u8FD9\u91CC",     L"\u5E38\u89C1\u95EE\u9898\u4E0E\u6C42\u52A9",  kGroupTools,     0, false, L"\uE897", L"https://qm.qq.com/q/3XsJIxaZde" },
    { L"\u52A0\u5165QQ\u7FA4",         L"\u4EA4\u6D41\u4E0E\u53CD\u9988",      kGroupTools,     0, false, L"\uE8BD", L"https://qm.qq.com/q/3XsJIxaZde" },     // Message
};
inline constexpr int kItemCount = ARRAYSIZE(kItems);

// 悬停目标：0..kTabCount-1 为标签，kHoverGear..kHoverClose 为右上角按钮，kHoverCard + i 为第 i 个条目
inline constexpr int kHoverGear = 100;
inline constexpr int kHoverMin = 101;
inline constexpr int kHoverClose = 102;
inline constexpr int kHoverCard = 200;
inline constexpr int kHoverMenu = 300;         // kHoverMenu + i 为设置菜单第 i 项
inline constexpr int kHoverMenuBg = 399;       // 菜单面板内的空白处（分隔线等）
inline constexpr int kHoverDlg = 400;          // 注入窗口里的按钮，见 DlgButton
inline constexpr int kHoverDlgBg = 498;        // 注入窗口面板内的空白处
inline constexpr int kHoverDlgOut = 499;       // 注入窗口外（点击关闭）
inline constexpr int kHoverPrompt = 500;       // kHoverPrompt + i 为提示词列表第 i 项
inline constexpr int kHoverAccept = 600;       // 免责声明"我已了解"按钮
inline constexpr int kHoverSkillMgr = 700;     // 700 + i 为 skill 列表第 i 项
inline constexpr int kHoverSkillRemove = 740; // 740 + i 为第 i 个 skill 的 ✕ 卸载按钮
inline constexpr int kHoverSkillCat = 780;    // 780 + cat 为分类标签
inline constexpr int kHoverSkillSearch = 790; // 搜索框区域
inline constexpr int kHoverSkillClose = 798;
inline constexpr int kHoverSkillFolder = 799;
inline constexpr int kHoverAboutClose = 801;
inline constexpr int kHoverAboutGithub = 802;
inline constexpr int kHoverAboutUpdate = 803;
inline constexpr int kHoverAboutUpdate2 = 804;  // "立即更新" 按钮
inline constexpr int kSkillMaxVisible = 10;

enum DlgButton { kDlgClose, kDlgSite, kDlgUninstall, kDlgInstall, kDlgFolder, kDlgButtonCount };
inline constexpr float kDlgW = 380.f;
inline constexpr float kDlgTopH = 66.f;       // logo + 名称 + 状态
inline constexpr float kDlgPathH = 50.f;      // 位置 + 打开
inline constexpr float kDlgItemH = 48.f;      // 每个提示词条目
inline constexpr float kDlgBottomH = 52.f;    // 按钮行
inline constexpr int   kDlgMaxVisible = 5;    // 最多同时显示的提示词条目数

// 设置菜单（点齿轮弹出，画在窗口内，和界面同一套样式）。cmd 为 0 表示分隔线
struct MenuEntry
{
    const wchar_t* glyph;   // Segoe MDL2 Assets 图标
    Str textId;
    int cmd;
};

inline const MenuEntry kMenu[] = {
    { L"\uE770", S_THEME_SYSTEM, kCmdTheme + 0 },
    { L"\uE708", S_THEME_DARK,   kCmdTheme + 1 },
    { L"\uE706", S_THEME_LIGHT,  kCmdTheme + 2 },
    { nullptr,   S_COUNT,        0 },
    { L"\uE774", S_LANG_CN,      kCmdLang + 0 },
    { L"\uE774", S_LANG_EN,      kCmdLang + 1 },
    { nullptr,   S_COUNT,        0 },
    { L"\uE718", S_TOPMOST,      kCmdTopmost },
    { L"\uE768", S_AUTOSTART,    kCmdAutostart },
    { nullptr,   S_COUNT,        0 },
    { L"\uE117", S_CHECK_UPDATE, kCmdCheckUpdate },
    { L"\uE74D", S_CLEAN_DATA,   kCmdCleanData },
    { L"\uE946", S_ABOUT,        kCmdAbout },
    { L"\uE7E8", S_EXIT,         kCmdExit },
};
inline constexpr int kMenuCount = ARRAYSIZE(kMenu);
inline constexpr float kMenuW = 184.f;
inline constexpr float kMenuItemH = 32.f;
inline constexpr float kMenuSepH = 9.f;

inline constexpr const wchar_t* kRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
inline constexpr const wchar_t* kRunValue = L"UaToolbox";

// ---------------------------------------------------------------------------
// 注入相关常量
// ---------------------------------------------------------------------------
inline constexpr size_t kMaxFile = 4 * 1024 * 1024;
inline constexpr int kMaxBackups = 200;

enum InjectState { kNotInstalled = 0, kInstalled = 1, kActiveNoState = 2 };

// 免责声明窗口尺寸
inline constexpr float kDiscW = 380.f;
inline constexpr float kDiscH = 340.f;

// Skills 管理弹窗尺寸
inline constexpr float kSkillDlgW = 400.f;
inline constexpr float kSkillItemH = 28.f;
