// inject.h : 提示词注入、状态/备份管理、Skills 安装
//
// 状态和备份存在 %APPDATA%\Ua\，不在目标文件里嵌标记
//

#pragma once

#include "config.h"

#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// 路径工具
// ---------------------------------------------------------------------------
std::wstring HomeDir();
std::wstring AppDataDir();
bool Supported(int i);
bool ToolDetected(int i);           // AI 工具的主目录是否存在
std::wstring TargetPath(int i);
std::wstring PromptDir();
std::vector<std::wstring> ListPrompts();
std::wstring PromptPath(int i, std::wstring* shown);
void ExtractEmbeddedPrompts();      // 首次启动时从 exe 资源释放 prompts

// ---------------------------------------------------------------------------
// 文件读写
// ---------------------------------------------------------------------------
enum ReadResult { kReadOk, kReadMissing, kReadFail };
ReadResult ReadBytes(const std::wstring& path, std::string& out);
bool WriteAtomic(const std::wstring& path, const std::string& data);
bool IsUtf16(const std::string& s);
bool StripBom(std::string& s);

// ---------------------------------------------------------------------------
// 注入状态（每个 app 一个小文件 %APPDATA%\Ua\state\<key>.dat）
// ---------------------------------------------------------------------------
struct TargetState
{
    std::wstring backupFile;        // 备份文件的完整路径（首次注入时的备份）
    bool originalExisted = false;
    ULONGLONG installedAt = 0;
};

bool LoadState(int i, TargetState& ts);
bool SaveState(int i, const TargetState& ts);
bool DeleteState(int i);
bool HasState(int i);

// ---------------------------------------------------------------------------
// 备份
// ---------------------------------------------------------------------------
std::wstring MakeBackup(int i, bool fileExists);
bool RestoreBackup(int i, const TargetState& ts);

// ---------------------------------------------------------------------------
// 状态检测
// ---------------------------------------------------------------------------
int ScanState(int i);
void RefreshStates();

// ---------------------------------------------------------------------------
// Skills
// ---------------------------------------------------------------------------
bool HasSkillsDir(int i);
std::wstring SkillsTargetDir(int i);
int InstallSkills(int i);
int UninstallSkills(int i);

// 按 skill 名卸载：遍历所有支持 skills 的 AI 工具，移除对应子目录和 SKILL.md，
// 并更新每个工具的 skills state 文件（移除这个 skill 名）。返回卸载的工具数。
int UninstallSkillByName(const wchar_t* skillName);

// 用于 Skills 管理弹窗
struct SkillStatus
{
    const wchar_t* name;
    int installedCount;     // 装到了几个 AI 工具里
};
std::vector<SkillStatus> GetSkillStatuses();

// ---------------------------------------------------------------------------
// 注入 / 卸载
// ---------------------------------------------------------------------------
bool Install(int i, std::wstring& err);
bool Uninstall(int i, std::wstring& err);

// Marker 常量（inject.cpp 定义）：区间注入的起止标记
extern const char kMarkerBegin[];
extern const char kMarkerEnd[];
