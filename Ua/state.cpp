// state.cpp : 全局运行时状态和用户设置
//

#include "state.h"
#include "inject.h"

#include <dwmapi.h>
#include <winhttp.h>
#include <shellapi.h>

#include <atomic>
#include <cstdio>
#include <map>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "winhttp.lib")

// 由 ui.cpp 提供：切换主题/语言后清空 SVG logo 缓存
void ResetLogos();

// ---------------------------------------------------------------------------
// 全局状态定义
// ---------------------------------------------------------------------------
HINSTANCE hInst = nullptr;
int g_tab = 0;
int g_hover = -1;
int g_lang = kLangCN;
bool g_topmost = false;
bool g_autostart = false;
bool g_menuOpen = false;
bool g_disclaimer = false;
bool g_skillMgr = false;
int g_skillScroll = 0;
bool g_aboutOpen = false;
float g_scale = 1.f;
int g_themeMode = kThemeSystem;
Palette P = kDark;
std::wstring g_toast;
bool g_micaActive = false;

int g_state[kItemCount] = {};
int g_dlg = -1;
std::vector<std::wstring> g_prompts;
int g_promptSel = -1;
int g_promptScroll = 0;

// 后台同步 prompts\ 时如发现版本变更，填入格式化后的 toast 文本并把
// g_syncToastOverride 指向它。缓冲区在整个进程生命期内有效。
static std::wstring g_syncToastBuffer;
const wchar_t* g_syncToastOverride = nullptr;

// ---------------------------------------------------------------------------
// 主题
// ---------------------------------------------------------------------------
bool SystemUsesLightTheme()
{
    DWORD light = 0, size = sizeof(light);
    RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &size);
    return light != 0;
}

bool IsWindows11()
{
    using RtlGetVersionFn = LONG(WINAPI*)(OSVERSIONINFOW*);
    auto fn = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
    OSVERSIONINFOW v{ sizeof(v) };
    return fn && fn(&v) == 0 && v.dwBuildNumber >= 22000;
}

bool IsMicaActive()
{
    return g_micaActive;
}

void ApplyTheme(HWND hWnd)
{
    bool light = g_themeMode == kThemeLight || (g_themeMode == kThemeSystem && SystemUsesLightTheme());
    P = light ? kLight : kDark;
    if (!IsWindows11())
        P.bg = light ? 0xF2F2F2 : 0x1F1F1F;

    BOOL dark = !light;     // 让系统菜单、阴影等也用对应的深浅色
    DwmSetWindowAttribute(hWnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));
    ResetLogos();
    InvalidateRect(hWnd, nullptr, FALSE);
}

void LoadThemeSetting()
{
    DWORD mode = kThemeSystem, size = sizeof(mode);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Ua", L"Theme", RRF_RT_REG_DWORD, nullptr, &mode, &size) == ERROR_SUCCESS &&
        mode <= kThemeLight)
        g_themeMode = (int)mode;
}

void LoadLangSetting()
{
    DWORD lang = kLangCN, size = sizeof(lang);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Ua", L"Lang", RRF_RT_REG_DWORD, nullptr, &lang, &size) == ERROR_SUCCESS &&
        lang <= kLangEN)
        g_lang = (int)lang;
}

void SetLang(HWND hWnd, int lang)
{
    g_lang = lang;
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Ua", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) == ERROR_SUCCESS)
    {
        DWORD v = (DWORD)lang;
        RegSetValueExW(key, L"Lang", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&v), sizeof(v));
        RegCloseKey(key);
    }
    InvalidateRect(hWnd, nullptr, FALSE);
}

void SetThemeMode(HWND hWnd, int mode)
{
    g_themeMode = mode;
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Ua", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) == ERROR_SUCCESS)
    {
        DWORD v = (DWORD)mode;
        RegSetValueExW(key, L"Theme", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&v), sizeof(v));
        RegCloseKey(key);
    }
    ApplyTheme(hWnd);
}

// ---------------------------------------------------------------------------
// 开机自启
// ---------------------------------------------------------------------------
bool AutostartEnabled()
{
    return RegGetValueW(HKEY_CURRENT_USER, kRunKey, kRunValue, RRF_RT_REG_SZ, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
}

bool SetAutostart(bool on)
{
    if (!on)
    {
        LSTATUS st = RegDeleteKeyValueW(HKEY_CURRENT_USER, kRunKey, kRunValue);
        return st == ERROR_SUCCESS || st == ERROR_FILE_NOT_FOUND;
    }
    wchar_t exe[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, exe, MAX_PATH);
    if (n == 0 || n == MAX_PATH) return false;
    std::wstring cmd = L"\"" + std::wstring(exe) + L"\"";
    return RegSetKeyValueW(HKEY_CURRENT_USER, kRunKey, kRunValue, REG_SZ, cmd.c_str(),
        DWORD((cmd.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
}

// ---------------------------------------------------------------------------
// 免责声明接受状态
// ---------------------------------------------------------------------------
bool DisclaimerAccepted()
{
    DWORD v = 0, size = sizeof(v);
    return RegGetValueW(HKEY_CURRENT_USER, L"Software\\Ua", L"Accepted", RRF_RT_REG_DWORD, nullptr, &v, &size) == ERROR_SUCCESS && v;
}

void AcceptDisclaimer()
{
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Ua", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) == ERROR_SUCCESS)
    {
        DWORD v = 1;
        RegSetValueExW(key, L"Accepted", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&v), sizeof(v));
        RegCloseKey(key);
    }
}

// ---------------------------------------------------------------------------
// 自动更新检查
// ---------------------------------------------------------------------------
std::mutex g_updateMutex;
std::wstring g_updateVersion;
bool g_updateChecked = false;

// 同时只允许一个检查线程在跑
static std::atomic<bool> g_updateRunning{ false };

// 窗口已销毁标志：后台线程用它保护 PostMessage
std::atomic<bool> g_exiting{ false };

// 下载更新状态
bool g_updateDownloading = false;
static std::atomic<bool> g_downloadRunning{ false };

std::wstring GetUpdateVersion()
{
    std::lock_guard<std::mutex> lock(g_updateMutex);
    return g_updateVersion;
}

bool UpdateChecked()
{
    std::lock_guard<std::mutex> lock(g_updateMutex);
    return g_updateChecked;
}

bool UpdateAvailable()
{
    std::lock_guard<std::mutex> lock(g_updateMutex);
    return !g_updateVersion.empty();
}

// 从 JSON 响应里抠出 "tag_name":"vX.Y.Z" 的值，去掉可能的 'v' 前缀
static std::wstring ParseTagName(const std::string& json)
{
    const std::string key = "\"tag_name\"";
    size_t p = json.find(key);
    if (p == std::string::npos) return L"";
    p += key.size();
    while (p < json.size() && (json[p] == ' ' || json[p] == '\t' || json[p] == ':'))
        ++p;
    if (p >= json.size() || json[p] != '"') return L"";
    ++p;
    size_t end = json.find('"', p);
    if (end == std::string::npos) return L"";
    std::string tag = json.substr(p, end - p);
    if (!tag.empty() && (tag[0] == 'v' || tag[0] == 'V')) tag.erase(0, 1);
    // tag_name 只含 ASCII 版本字符，直接扩宽
    std::wstring out(tag.begin(), tag.end());
    return out;
}

// 用 WinHTTP 做一次 HTTPS GET，返回响应体（UTF-8 原始字节）；失败返回空串。
// headers 可以是 nullptr（不加额外请求头）。
// maxBytes 用来防止异常响应撑爆内存，超过就返回空串。
static std::string HttpsGet(const wchar_t* host, const wchar_t* path,
                            const wchar_t* headers, size_t maxBytes)
{
    HINTERNET hSession = WinHttpOpen(L"UaToolbox/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return {};

    WinHttpSetTimeouts(hSession, 5000, 5000, 5000, 5000);

    HINTERNET hConnect = WinHttpConnect(hSession, host,
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return {}; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return {}; }

    std::string body;
    BOOL ok = WinHttpSendRequest(hRequest,
        headers, headers ? (DWORD)-1 : 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr);

    if (ok)
    {
        DWORD status = 0, size = sizeof(status);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
        if (status == 200)
        {
            DWORD avail = 0;
            while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0)
            {
                size_t off = body.size();
                body.resize(off + avail);
                DWORD read = 0;
                if (!WinHttpReadData(hRequest, body.data() + off, avail, &read))
                {
                    body.clear();
                    break;
                }
                body.resize(off + read);
                if (body.size() > maxBytes) { body.clear(); break; }
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return body;
}

// 用 WinHTTP 抓 GitHub Releases API，返回原始 JSON（UTF-8）；失败返回空串
static std::string FetchLatestReleaseJson()
{
    return HttpsGet(L"api.github.com", L"/repos/freyrswift/UaStation/releases/latest",
        L"Accept: application/vnd.github+json\r\n", 512 * 1024);
}

void CheckUpdates(HWND hWnd)
{
    bool expected = false;
    if (!g_updateRunning.compare_exchange_strong(expected, true))
        return;     // 已经在跑

    std::thread([hWnd]()
        {
            std::string json = FetchLatestReleaseJson();
            std::wstring tag = ParseTagName(json);

            {
                std::lock_guard<std::mutex> lock(g_updateMutex);
                g_updateChecked = true;
                // tag 为空表示拉取失败；拿到 tag 后和内嵌版本比较
                if (!tag.empty() && tag != T(S_VERSION))
                    g_updateVersion = tag;
                else
                    g_updateVersion.clear();
            }

            g_updateRunning = false;
            if (hWnd && !g_exiting) PostMessageW(hWnd, kMsgUpdateReady, 0, 0);
        }).detach();
}

// ---------------------------------------------------------------------------
// 下载更新 exe 到 %TEMP%\Ua-update.exe
// ---------------------------------------------------------------------------

// wstring → string 辅助（用 WideCharToMultiByte）
static std::string Narrow(const std::wstring& ws)
{
    if (ws.empty()) return {};
    int n = WideCharToMultiByte(CP_ACP, 0, ws.c_str(), (int)ws.size(),
                                nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string s((size_t)n, '\0');
    WideCharToMultiByte(CP_ACP, 0, ws.c_str(), (int)ws.size(),
                        s.data(), n, nullptr, nullptr);
    return s;
}

// 用 WinHTTP 下载文件到磁盘。支持 302 重定向（WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS）。
// 成功返回 true。
static bool HttpsDownloadFile(const wchar_t* url, const wchar_t* destPath)
{
    // 解析 URL：跳过 "https://"，分离 host 和 path
    const wchar_t* p = url;
    if (wcsncmp(p, L"https://", 8) == 0) p += 8;
    const wchar_t* slash = wcschr(p, L'/');
    if (!slash) return false;
    std::wstring host(p, slash);
    std::wstring path(slash);

    HINTERNET hSession = WinHttpOpen(L"UaToolbox/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    WinHttpSetTimeouts(hSession, 10000, 10000, 30000, 60000);

    // 确保自动跟随重定向（GitHub releases 会 302）
    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hSession, WINHTTP_OPTION_REDIRECT_POLICY,
                     &redirectPolicy, sizeof(redirectPolicy));

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    BOOL ok = WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr);

    bool success = false;
    if (ok)
    {
        DWORD status = 0, size = sizeof(status);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
        if (status == 200)
        {
            HANDLE hFile = CreateFileW(destPath, GENERIC_WRITE, 0, nullptr,
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                success = true;
                DWORD avail = 0;
                while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0)
                {
                    std::vector<char> buf(avail);
                    DWORD read = 0;
                    if (!WinHttpReadData(hRequest, buf.data(), avail, &read))
                    {
                        success = false;
                        break;
                    }
                    DWORD written = 0;
                    if (!WriteFile(hFile, buf.data(), read, &written, nullptr) || written != read)
                    {
                        success = false;
                        break;
                    }
                }
                CloseHandle(hFile);
                if (!success) DeleteFileW(destPath);
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return success;
}

void DownloadUpdate(HWND hWnd)
{
    bool expected = false;
    if (!g_downloadRunning.compare_exchange_strong(expected, true))
        return;     // 已经在跑

    g_updateDownloading = true;

    std::thread([hWnd]()
        {
            wchar_t tmp[MAX_PATH];
            GetTempPathW(MAX_PATH, tmp);
            std::wstring dest = std::wstring(tmp) + L"Ua-update.exe";

            bool ok = HttpsDownloadFile(kUpdateDownloadUrl, dest.c_str());

            g_updateDownloading = false;
            g_downloadRunning = false;
            if (hWnd && !g_exiting) PostMessageW(hWnd, kMsgUpdateDownloaded, ok ? 1 : 0, 0);
        }).detach();
}

void ApplyUpdate(HWND hWnd)
{
    wchar_t exe[MAX_PATH], tmp[MAX_PATH];
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    GetTempPathW(MAX_PATH, tmp);

    std::wstring newExe = std::wstring(tmp) + L"Ua-update.exe";
    std::wstring bat = std::wstring(tmp) + L"ua-update.bat";

    // 写批处理：等旧进程退出 -> 覆盖 -> 启动新版 -> 删自己
    std::string script;
    script += "@echo off\r\n";
    script += "timeout /t 2 /nobreak >nul\r\n";
    script += "copy /y \"" + Narrow(newExe) + "\" \"" + Narrow(std::wstring(exe)) + "\"\r\n";
    script += "start \"\" \"" + Narrow(std::wstring(exe)) + "\"\r\n";
    script += "del \"%~f0\"\r\n";

    WriteAtomic(bat, script);

    // 启动 bat（隐藏窗口），然后自己退出
    ShellExecuteW(nullptr, L"open", bat.c_str(), nullptr, nullptr, SW_HIDE);

    // 设置真正退出标记并销毁窗口
    extern bool g_realExit;
    g_realExit = true;
    DestroyWindow(hWnd);
}

// ---------------------------------------------------------------------------
// 后台同步 prompts\ 文件夹（从 GitHub 拉新模板，静默失败）
// ---------------------------------------------------------------------------
static std::atomic<bool> g_syncRunning{ false };

// 把 https://host/some/path 拆成 host 和 path 两段（不处理端口和 query）
static bool SplitHttpsUrl(const std::string& url, std::wstring& host, std::wstring& path)
{
    const std::string prefix = "https://";
    if (url.compare(0, prefix.size(), prefix) != 0) return false;
    size_t slash = url.find('/', prefix.size());
    if (slash == std::string::npos) return false;
    std::string h = url.substr(prefix.size(), slash - prefix.size());
    std::string p = url.substr(slash);
    if (h.empty() || p.empty()) return false;
    // 这些字段都是 ASCII URL，直接扩宽
    host.assign(h.begin(), h.end());
    path.assign(p.begin(), p.end());
    return true;
}

// 从 json[from..] 开始找 "key":"value"，返回 value。next 回传扫描到的位置。
// 失败返回空串，next 保持原值。
static std::string JsonFindString(const std::string& json, size_t from,
                                  const char* key, size_t& next)
{
    std::string pat = "\"";
    pat += key;
    pat += "\"";
    size_t p = json.find(pat, from);
    if (p == std::string::npos) { next = from; return {}; }
    p += pat.size();
    while (p < json.size() && (json[p] == ' ' || json[p] == '\t' || json[p] == ':'))
        ++p;
    if (p >= json.size() || json[p] != '"') { next = p; return {}; }
    ++p;
    size_t end = p;
    // 简单处理转义：遇到 \ 跳过下一字节
    while (end < json.size() && json[end] != '"')
    {
        if (json[end] == '\\' && end + 1 < json.size()) end += 2;
        else ++end;
    }
    if (end >= json.size()) { next = end; return {}; }
    std::string v = json.substr(p, end - p);
    next = end + 1;
    return v;
}

struct RemoteFile { std::string name, sha, downloadUrl; };

// Contents API 返回的数组里每项的字段顺序固定：name -> path -> sha -> size ->
// url -> html_url -> git_url -> download_url -> type -> _links。
// 顺序线性扫 name/sha/download_url 三个字段即可。
static std::vector<RemoteFile> ParseContents(const std::string& json)
{
    std::vector<RemoteFile> out;
    size_t p = 0;
    while (p < json.size())
    {
        size_t next;
        std::string name = JsonFindString(json, p, "name", next);
        if (name.empty()) break;
        p = next;
        std::string sha = JsonFindString(json, p, "sha", next);
        if (sha.empty()) break;
        p = next;
        std::string dl = JsonFindString(json, p, "download_url", next);
        if (dl.empty()) break;
        p = next;
        out.push_back({ name, sha, dl });
    }
    return out;
}

// 读 sha 缓存：%APPDATA%\Ua\prompts-sync.json，格式 "name=sha\n"
static std::map<std::string, std::string> LoadSyncCache(const std::wstring& path)
{
    std::map<std::string, std::string> cache;
    std::string raw;
    if (ReadBytes(path, raw) != kReadOk) return cache;
    size_t p = 0;
    while (p < raw.size())
    {
        size_t nl = raw.find('\n', p);
        if (nl == std::string::npos) nl = raw.size();
        std::string line = raw.substr(p, nl - p);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t eq = line.find('=');
        if (eq != std::string::npos && eq > 0)
            cache[line.substr(0, eq)] = line.substr(eq + 1);
        p = nl + 1;
    }
    return cache;
}

static bool SaveSyncCache(const std::wstring& path,
                          const std::map<std::string, std::string>& cache)
{
    std::string data;
    for (const auto& [name, sha] : cache)
    {
        data += name;
        data += '=';
        data += sha;
        data += '\n';
    }
    return WriteAtomic(path, data);
}

// 从 .md 文件首行解析 <!-- Ua-Prompt-Version: X.Y.Z -->。
// 只扫描第一行；兼容 LF 和 CRLF。版本号只允许 ASCII，直接扩宽成 wstring。
std::wstring ParsePromptVersion(const std::string& content)
{
    size_t end = content.find('\n');
    if (end == std::string::npos) end = content.size();
    size_t lineEnd = end;
    if (lineEnd > 0 && content[lineEnd - 1] == '\r') --lineEnd;

    const std::string prefix = "<!-- Ua-Prompt-Version:";
    const std::string suffix = "-->";
    // 必须整行能完整匹配前缀和后缀
    size_t p = content.find(prefix);
    if (p == std::string::npos || p >= lineEnd) return L"";
    p += prefix.size();
    if (p > lineEnd) return L"";
    size_t q = content.find(suffix, p);
    if (q == std::string::npos || q > lineEnd) return L"";
    // 去掉版本号前后的空白
    while (p < q && (content[p] == ' ' || content[p] == '\t')) ++p;
    size_t qTrim = q;
    while (qTrim > p && (content[qTrim - 1] == ' ' || content[qTrim - 1] == '\t')) --qTrim;
    if (qTrim <= p) return L"";
    std::string v = content.substr(p, qTrim - p);
    return std::wstring(v.begin(), v.end());
}

// 把一段 UTF-16 文本追加到 %APPDATA%\Ua\prompts-updates.txt（UTF-8，CRLF，带 BOM）。
// 失败静默，这是日志不关心。
static void AppendPromptsUpdateLog(const std::wstring& text)
{
    if (text.empty()) return;
    std::wstring path = AppDataDir() + L"\\prompts-updates.txt";

    int n = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(),
                                 nullptr, 0, nullptr, nullptr);
    if (n <= 0) return;
    std::string utf8((size_t)n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(),
                        utf8.data(), n, nullptr, nullptr);

    HANDLE h = CreateFileW(path.c_str(), FILE_APPEND_DATA,
        FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;

    // 新建文件时写入 UTF-8 BOM，方便 Notepad 识别编码
    LARGE_INTEGER sz{};
    if (GetFileSizeEx(h, &sz) && sz.QuadPart == 0)
    {
        const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
        DWORD w = 0;
        WriteFile(h, bom, 3, &w, nullptr);
    }
    DWORD written = 0;
    WriteFile(h, utf8.data(), (DWORD)utf8.size(), &written, nullptr);
    CloseHandle(h);
}

// 文件名得安全：只允许 ASCII 字母/数字/点/连字符/下划线，必须以 .md 结尾
static bool IsSafeMdName(const std::string& name)
{
    if (name.size() < 4 || name.size() > 128) return false;
    // 后缀 .md（忽略大小写）
    const char* ext = name.c_str() + name.size() - 3;
    if (ext[0] != '.' || (ext[1] != 'm' && ext[1] != 'M') || (ext[2] != 'd' && ext[2] != 'D'))
        return false;
    for (char c : name)
    {
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_';
        if (!ok) return false;
    }
    if (name[0] == '.') return false;              // 不允许 .md / .foo.md 这样的隐藏文件
    if (name.find("..") != std::string::npos) return false;
    return true;
}

// 返回版本标记发生变化的 .md 文件数（不是单纯的下载数）。
// 过程中：下载前读本地旧版本，下载后读新版本，不同就记一行到更新日志。
static int DoSyncPrompts()
{
    // 1. 拉目录列表
    std::string listJson = HttpsGet(kPromptsApiHost, kPromptsApiPath,
        L"Accept: application/vnd.github+json\r\n", 2 * 1024 * 1024);
    if (listJson.empty()) return 0;

    // 2. 解析
    std::vector<RemoteFile> remote = ParseContents(listJson);
    if (remote.empty()) return 0;

    // 3. 读本地 sha 缓存
    std::wstring cachePath = AppDataDir() + L"\\prompts-sync.json";
    auto cache = LoadSyncCache(cachePath);

    // 4. 下载变化过的 .md，记录版本变化
    std::wstring promptDir = PromptDir();
    int downloaded = 0;
    int versionChanges = 0;
    std::wstring changeLog;     // 累积 "<name>  OLD -> NEW" 行，稍后一次性写文件

    for (const RemoteFile& f : remote)
    {
        if (!IsSafeMdName(f.name)) continue;
        auto it = cache.find(f.name);
        if (it != cache.end() && it->second == f.sha) continue;     // 本地已是最新

        std::wstring host, path;
        if (!SplitHttpsUrl(f.downloadUrl, host, path)) continue;
        std::string body = HttpsGet(host.c_str(), path.c_str(), nullptr, 2 * 1024 * 1024);
        if (body.empty()) continue;

        // 文件名是 ASCII，直接扩宽
        std::wstring wname(f.name.begin(), f.name.end());
        std::wstring target = promptDir + L"\\" + wname;

        // 下载完成、落盘之前先读旧版本
        std::string oldContent;
        std::wstring oldVersion;
        if (ReadBytes(target, oldContent) == kReadOk)
            oldVersion = ParsePromptVersion(oldContent);
        std::wstring newVersion = ParsePromptVersion(body);

        if (!WriteAtomic(target, body)) continue;

        cache[f.name] = f.sha;
        ++downloaded;

        // 版本号发生变化（含首次下载和版本号消失）才算一次 UI 可见更新
        if (oldVersion != newVersion)
        {
            changeLog += wname;
            changeLog += L"  ";
            changeLog += oldVersion.empty() ? std::wstring(L"(new)") : oldVersion;
            changeLog += L" → ";
            changeLog += newVersion.empty() ? std::wstring(L"(none)") : newVersion;
            changeLog += L"\r\n";
            ++versionChanges;
        }
    }

    // 5. 持久化缓存
    if (downloaded > 0) SaveSyncCache(cachePath, cache);

    // 6. 版本变化写入 %APPDATA%\Ua\prompts-updates.txt（带时间戳，追加模式）
    if (versionChanges > 0)
    {
        SYSTEMTIME st{};
        GetLocalTime(&st);
        wchar_t ts[64];
        swprintf_s(ts, 64, L"[%04d-%02d-%02d %02d:%02d:%02d]\r\n",
                   st.wYear, st.wMonth, st.wDay,
                   st.wHour, st.wMinute, st.wSecond);
        std::wstring full = ts;
        full += changeLog;
        full += L"\r\n";
        AppendPromptsUpdateLog(full);
    }

    return versionChanges;
}

void SyncPrompts(HWND hWnd)
{
    bool expected = false;
    if (!g_syncRunning.compare_exchange_strong(expected, true))
        return;     // 已经在跑

    std::thread([hWnd]()
        {
            int versionChanges = DoSyncPrompts();
            // 发现版本变更时填充 toast 覆盖：PostMessage 之前写好，UI 线程读到才生效
            if (versionChanges > 0)
            {
                wchar_t buf[128];
                swprintf_s(buf, 128, kStr[S_PROMPTS_UPDATED_COUNT][g_lang], versionChanges);
                g_syncToastBuffer = buf;
                g_syncToastOverride = g_syncToastBuffer.c_str();
            }
            g_syncRunning = false;
            if (hWnd && !g_exiting) PostMessageW(hWnd, kMsgSyncReady, (WPARAM)versionChanges, 0);
        }).detach();
}
