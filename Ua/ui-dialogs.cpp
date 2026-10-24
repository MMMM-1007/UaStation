// ui-dialogs.cpp : 弹窗（设置菜单、注入对话框、Skills 管理、免责声明、关于）
//

#include "ui-dialogs.h"
#include <windowsx.h>

// ---------------------------------------------------------------------------
// 弹窗本地状态
// ---------------------------------------------------------------------------
// Skills 管理弹窗的筛选状态（搜索框 + 分类标签）
static std::wstring g_skillFilter;
static int g_skillCat = kCatAll;
static bool g_skillMgrPrev = false;
// 为了接收 WM_CHAR/WM_KEYDOWN 更新搜索框，用 SetWindowLongPtr 挂一个 WndProc 钩子
static WNDPROC g_oldWndProc = nullptr;

// ---------------------------------------------------------------------------
// 右上角按钮位置（ui-main.cpp 的 ButtonRect 本地定义；弹窗里也要用到）
// ---------------------------------------------------------------------------
static D2D1_RECT_F DlgButtonFromMain(int id)
{
    float right = kW - kPad - 4.f - (kHoverClose - id) * 32.f;
    return R(right - 30.f, kPad + 4.f, right, kPad + 32.f);
}

// ---------------------------------------------------------------------------
// 菜单面板
// ---------------------------------------------------------------------------
// 菜单面板右对齐到关闭按钮，紧贴在标签栏下方
D2D1_RECT_F MenuRect()
{
    float h = 12.f;
    for (const MenuEntry& e : kMenu)
        h += e.cmd ? kMenuItemH : kMenuSepH;
    float right = DlgButtonFromMain(kHoverClose).right;
    float top = kPad + 38.f;
    return R(right - kMenuW, top, right, top + h);
}

D2D1_RECT_F MenuItemRect(int i)
{
    D2D1_RECT_F m = MenuRect();
    float y = m.top + 6.f;
    for (int k = 0; k < i; ++k)
        y += kMenu[k].cmd ? kMenuItemH : kMenuSepH;
    return R(m.left + 6.f, y, m.right - 6.f, y + (kMenu[i].cmd ? kMenuItemH : kMenuSepH));
}

// ---------------------------------------------------------------------------
// 注入窗口
// ---------------------------------------------------------------------------
static float DlgH()
{
    int visible = std::min((int)g_prompts.size(), kDlgMaxVisible);
    if (visible < 1) visible = 1;
    float listTitleH = (g_prompts.size() <= 1) ? 0.f : 20.f;
    float listPadding = (g_prompts.size() <= 1) ? 10.f : 0.f;
    return kDlgTopH + kDlgPathH + listTitleH + visible * kDlgItemH + listPadding + kDlgBottomH;
}

D2D1_RECT_F DlgRect()
{
    float h = DlgH();
    float l = (kW - kDlgW) / 2, t = (kH - h) / 2 - 10;
    return R(l, t, l + kDlgW, t + h);
}

static float DlgListTop()
{
    float listTitleH = (g_prompts.size() <= 1) ? 0.f : 20.f;
    return DlgRect().top + kDlgTopH + kDlgPathH + listTitleH;
}
static float DlgListBottom()
{
    int visible = std::min((int)g_prompts.size(), kDlgMaxVisible);
    return DlgListTop() + (visible < 1 ? 1 : visible) * kDlgItemH;
}

D2D1_RECT_F DlgPromptRect(int vis)    // vis = 0-based visible index
{
    D2D1_RECT_F d = DlgRect();
    float y = DlgListTop() + vis * kDlgItemH;
    return R(d.left + 14, y, d.right - 14, y + kDlgItemH);
}

D2D1_RECT_F DlgButtonRect(int b)
{
    D2D1_RECT_F d = DlgRect();
    float by = d.bottom - 44, bh = 32, bw = 84;
    switch (b)
    {
    case kDlgClose:     return R(d.right - 38, d.top + 10, d.right - 10, d.top + 38);
    case kDlgFolder:    return R(d.right - 64, d.top + kDlgTopH + 2, d.right - 18, d.top + kDlgTopH + 22);
    case kDlgSite:      return R(d.left + 18, by, d.left + 18 + 72, by + bh);
    case kDlgUninstall: return R(d.right - 18 - 2 * bw - 8, by, d.right - 18 - bw - 8, by + bh);
    case kDlgInstall:   return R(d.right - 18 - bw, by, d.right - 18, by + bh);
    }
    return R(0, 0, 0, 0);
}

bool DlgEnabled(int b)
{
    if (g_dlg < 0) return false;
    switch (b)
    {
    case kDlgSite:      return kItems[g_dlg].url[0] != 0;
    case kDlgInstall:   return Supported(g_dlg) && ToolDetected(g_dlg) && g_promptSel >= 0;
    case kDlgUninstall: return g_state[g_dlg] == kInstalled;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 免责声明窗口
// ---------------------------------------------------------------------------
static D2D1_RECT_F DiscRect()
{
    float l = (kW - kDiscW) / 2, t = (kH - kDiscH) / 2;
    return R(l, t, l + kDiscW, t + kDiscH);
}

D2D1_RECT_F DiscAcceptRect()
{
    D2D1_RECT_F d = DiscRect();
    return R(d.left + (kDiscW - 120) / 2, d.bottom - 50, d.left + (kDiscW + 120) / 2, d.bottom - 18);
}

// ---------------------------------------------------------------------------
// 关于窗口
// ---------------------------------------------------------------------------
D2D1_RECT_F AboutRect()
{
    float w = 340.f, h = 320.f;
    float l = (kW - w) / 2, t = (kH - h) / 2;
    return R(l, t, l + w, t + h);
}

D2D1_RECT_F AboutCloseRect()
{
    D2D1_RECT_F d = AboutRect();
    return R(d.right - 38, d.top + 10, d.right - 10, d.top + 38);
}

D2D1_RECT_F AboutGithubRect()
{
    D2D1_RECT_F d = AboutRect();
    float by = d.bottom - 48;
    return R(d.left + (340.f - 160) / 2, by, d.left + (340.f + 160) / 2, by + 32);
}

// 关于卡片里显示更新状态的那一行（位于版本号正下方）
D2D1_RECT_F AboutUpdateRect()
{
    D2D1_RECT_F d = AboutRect();
    return R(d.left + 20, d.top + 90, d.right - 20, d.top + 108);
}

// "立即更新" 按钮（紧挨更新状态行下方）
D2D1_RECT_F AboutUpdateBtnRect()
{
    D2D1_RECT_F d = AboutRect();
    float bw = 90.f, bh = 26.f;
    float cx = (d.left + d.right) / 2;
    return R(cx - bw / 2, d.top + 108, cx + bw / 2, d.top + 108 + bh);
}

// ---------------------------------------------------------------------------
// Skills 管理弹窗
// ---------------------------------------------------------------------------
// 按行自动折行排布分类标签。返回总高度。
static constexpr float kCatPillH = 22.f;
static constexpr float kCatPillGap = 4.f;
static constexpr float kCatPillPad = 10.f;

static void CatPillRects(float x0, float y0, float maxW, D2D1_RECT_F out[kCatCount], float& totalH)
{
    float x = x0, y = y0;
    for (int i = 0; i < kCatCount; ++i)
    {
        float w = MeasureText(T((Str)(S_CAT_ALL + i)), g_fTab.Get()) + 2 * kCatPillPad;
        if (x > x0 && x + w > x0 + maxW)
        {
            x = x0;
            y += kCatPillH + kCatPillGap;
        }
        out[i] = R(x, y, x + w, y + kCatPillH);
        x += w + kCatPillGap;
    }
    totalH = y + kCatPillH - y0;
}

static float SkillCatHeight()
{
    D2D1_RECT_F tmp[kCatCount];
    float h = 0.f;
    CatPillRects(0.f, 0.f, kSkillDlgW - 28.f, tmp, h);
    return h;
}

// 过滤后的 skill 下标（指向 kSkills[]）
std::vector<int> FilteredSkills()
{
    std::vector<int> out;
    std::wstring flt = g_skillFilter;
    for (wchar_t& c : flt) c = (wchar_t)towlower(c);
    for (int i = 0; i < kSkillCount; ++i)
    {
        if (g_skillCat != kCatAll && kSkillCategories[i] != g_skillCat) continue;
        if (!flt.empty())
        {
            std::wstring name = kSkills[i].dirName;
            for (wchar_t& c : name) c = (wchar_t)towlower(c);
            if (name.find(flt) == std::wstring::npos) continue;
        }
        out.push_back(i);
    }
    return out;
}

static float SkillDlgH()
{
    int visible = std::min(kSkillCount, kSkillMaxVisible);
    return 60.f + SkillCatHeight() + 8.f + 32.f + 8.f + visible * kSkillItemH + 52.f;
}

D2D1_RECT_F SkillDlgRect()
{
    float h = SkillDlgH();
    float l = (kW - kSkillDlgW) / 2, t = (kH - h) / 2;
    return R(l, t, l + kSkillDlgW, t + h);
}

D2D1_RECT_F SkillCloseRect()
{
    D2D1_RECT_F d = SkillDlgRect();
    return R(d.right - 38, d.top + 10, d.right - 10, d.top + 38);
}

D2D1_RECT_F SkillFolderRect()
{
    D2D1_RECT_F d = SkillDlgRect();
    float by = d.bottom - 44;
    return R(d.left + (kSkillDlgW - 140) / 2, by, d.left + (kSkillDlgW + 140) / 2, by + 32);
}

// 分类标签在弹窗中的实际位置
void SkillCatPills(D2D1_RECT_F out[kCatCount])
{
    D2D1_RECT_F d = SkillDlgRect();
    float h = 0.f;
    CatPillRects(d.left + 14, d.top + 60.f, kSkillDlgW - 28.f, out, h);
}

// 搜索框 rect
D2D1_RECT_F SkillSearchRect()
{
    D2D1_RECT_F d = SkillDlgRect();
    float top = d.top + 60.f + SkillCatHeight() + 8.f;
    return R(d.left + 14, top, d.right - 14, top + 32.f);
}

// 列表起始位置
static float SkillListTop()
{
    D2D1_RECT_F d = SkillDlgRect();
    return d.top + 60.f + SkillCatHeight() + 8.f + 32.f + 8.f;
}

D2D1_RECT_F SkillItemRect(int vis)
{
    D2D1_RECT_F d = SkillDlgRect();
    float y = SkillListTop() + vis * kSkillItemH;
    return R(d.left + 14, y, d.right - 14, y + kSkillItemH);
}

// ✕ 按钮：条目最右侧，正方形，不增加行高
D2D1_RECT_F SkillRemoveRect(int vis)
{
    D2D1_RECT_F ir = SkillItemRect(vis);
    float cy = (ir.top + ir.bottom) / 2;
    return R(ir.right - 24, cy - 10, ir.right - 4, cy + 10);
}

// ---------------------------------------------------------------------------
// ui-main.cpp 需要调用 DrawIcon 画应用 logo（放在 ui-main.cpp 里了），
// 为了避免循环依赖，这里前向声明
// ---------------------------------------------------------------------------
void DrawIcon(int i, D2D1_RECT_F tile);

// ---------------------------------------------------------------------------
// 菜单绘制
// ---------------------------------------------------------------------------
static bool MenuChecked(int cmd)
{
    if (cmd >= kCmdTheme && cmd <= kCmdTheme + kThemeLight) return cmd - kCmdTheme == g_themeMode;
    if (cmd >= kCmdLang && cmd <= kCmdLang + kLangEN) return cmd - kCmdLang == g_lang;
    if (cmd == kCmdTopmost) return g_topmost;
    if (cmd == kCmdAutostart) return g_autostart;
    return false;
}

void DrawMenu()
{
    D2D1_RECT_F m = MenuRect();

    // 简单的投影：往下偏移几层半透明圆角矩形
    for (int k = 3; k >= 1; --k)
        Fill(R(m.left - k, m.top + k, m.right + k, m.bottom + 2 * k), C(0x000000, P.dark ? 0.10f : 0.04f), 8.f + k);
    Fill(m, C(P.dark ? 0x2C2C2C : 0xFFFFFF), 8.f);
    Stroke(m, Ov(0.14f), 8.f);

    for (int i = 0; i < kMenuCount; ++i)
    {
        const MenuEntry& e = kMenu[i];
        D2D1_RECT_F r = MenuItemRect(i);
        if (!e.cmd)
        {
            float y = (r.top + r.bottom) / 2;
            g_brush->SetColor(Ov(0.12f));
            g_rt->DrawLine(D2D1::Point2F(r.left + 4, y), D2D1::Point2F(r.right - 4, y), g_brush.Get(), 1.f);
            continue;
        }

        bool hot = g_hover == kHoverMenu + i;
        if (hot) Fill(r, e.cmd == kCmdExit ? C(0xC42B1C) : Ov(0.08f), 5.f);
        D2D1_COLOR_F fg = hot && e.cmd == kCmdExit ? C(0xFFFFFF) : C(P.text);

        Text(e.glyph, g_fGlyph.Get(), R(r.left + 4, r.top, r.left + 32, r.bottom), fg, DWRITE_TEXT_ALIGNMENT_CENTER);
        Text(T(e.textId), g_fTab.Get(), R(r.left + 38, r.top, r.right - 28, r.bottom), fg);
        if (MenuChecked(e.cmd))
            Text(L"", g_fIcon.Get(), R(r.right - 28, r.top, r.right - 6, r.bottom), fg, DWRITE_TEXT_ALIGNMENT_CENTER);  // CheckMark
    }
}

// ---------------------------------------------------------------------------
// 注入对话框
// ---------------------------------------------------------------------------
static void DlgButton(int b, const wchar_t* label, bool primary)
{
    D2D1_RECT_F r = DlgButtonRect(b);
    bool on = DlgEnabled(b), hot = on && g_hover == kHoverDlg + b;
    if (!on)
    {
        Fill(r, Ov(0.04f), 6.f);
        Text(label, g_fTab.Get(), r, C(P.sub, 0.45f), DWRITE_TEXT_ALIGNMENT_CENTER);
        return;
    }
    if (primary)
    {
        // Win11 强调色按钮：深色主题 #60CDFF + 黑字，浅色主题 #005FB8 + 白字
        Fill(r, C(P.dark ? 0x60CDFF : 0x005FB8, hot ? 0.88f : 1.f), 6.f);
        Text(label, g_fTabSel.Get(), r, C(P.dark ? 0x000000 : 0xFFFFFF), DWRITE_TEXT_ALIGNMENT_CENTER);
        return;
    }
    Fill(r, Ov(hot ? 0.12f : 0.07f), 6.f);
    Stroke(r, Ov(0.12f), 6.f);
    Text(label, g_fTab.Get(), r, C(P.text), DWRITE_TEXT_ALIGNMENT_CENTER);
}

// 提示词文件名 → 分类描述（中英双语）
static const wchar_t* PromptDescription(const std::wstring& filename)
{
    // 综合提示词（破甲 + 路由表 + 场景化规范）
    if (_wcsicmp(filename.c_str(), L"Ua.md") == 0)
        return g_lang == kLangEN ? L"Expert · Security + Dev + Writing" : L"综合 · 安全研究+开发+写作";

    // 未知文件：显示去掉 .md 的文件名（兜底）
    static thread_local std::wstring fallback;
    fallback = filename;
    if (fallback.size() > 3 && _wcsicmp(fallback.c_str() + fallback.size() - 3, L".md") == 0)
        fallback.resize(fallback.size() - 3);
    return fallback.c_str();
}

void DrawDialog()
{
    const Item& it = kItems[g_dlg];
    D2D1_SIZE_F sz = g_rt->GetSize();
    g_brush->SetColor(C(0x000000, P.dark ? 0.45f : 0.25f));
    g_rt->FillRectangle(R(0, 0, sz.width, sz.height), g_brush.Get());

    D2D1_RECT_F d = DlgRect();
    Fill(d, C(P.dark ? 0x2C2C2C : 0xFFFFFF), 10.f);
    Stroke(d, Ov(0.14f), 10.f);

    // 标题：logo + 名称 + 状态
    D2D1_RECT_F tile = R(d.left + 18, d.top + 14, d.left + 48, d.top + 44);
    DrawIcon(g_dlg, tile);
    Text(it.name, g_fTitle.Get(), R(tile.right + 10, d.top + 14, d.right - 44, d.top + 30), C(P.text));

    const wchar_t* status;
    UINT32 dot;
    if (!Supported(g_dlg))                       { status = T(S_NOT_SUPPORTED); dot = P.sub; }
    else if (!ToolDetected(g_dlg))               { status = T(S_TOOL_NOT_FOUND); dot = 0xE8A33D; }
    else if (g_state[g_dlg] == kInstalled)       { status = T(S_INJECTED);           dot = kGreen; }
    else if (g_state[g_dlg] == kActiveNoState)   { status = T(S_FILE_HAS_CONTENT);     dot = 0xE8A33D; }
    else                                         { status = T(S_NOT_INJECTED);           dot = P.sub; }
    g_brush->SetColor(C(dot));
    g_rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(tile.right + 14, d.top + 40), 3.5f, 3.5f), g_brush.Get());
    Text(status, g_fLabel.Get(), R(tile.right + 22, d.top + 33, d.right - 18, d.top + 48), C(P.sub));

    bool close = g_hover == kHoverDlg + kDlgClose;
    D2D1_RECT_F cr = DlgButtonRect(kDlgClose);
    if (close) Fill(cr, Ov(0.08f), 6.f);
    Text(L"", g_fIcon.Get(), cr, C(close ? P.text : P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);

    // 位置 + 打开按钮
    float py = d.top + kDlgTopH;
    Text(T(S_LOCATION), g_fLabel.Get(), R(d.left + 18, py + 4, d.left + 58, py + 20), C(P.sub));
    std::wstring where = Supported(g_dlg)
        ? std::wstring(L"~\\") + it.target
        : std::wstring(it.note ? it.note : L"");
    Text(where, g_fLabel.Get(), R(d.left + 58, py + 4, d.right - 70, py + 20), C(P.text));
    D2D1_RECT_F fr = DlgButtonRect(kDlgFolder);
    bool fhot = g_hover == kHoverDlg + kDlgFolder;
    if (fhot) Fill(fr, Ov(0.08f), 5.f);
    Text(T(S_OPEN), g_fTab.Get(), fr, C(P.dark ? 0x60CDFF : 0x005FB8), DWRITE_TEXT_ALIGNMENT_CENTER);

    // 分隔线
    g_brush->SetColor(Ov(0.10f));
    g_rt->DrawLine(D2D1::Point2F(d.left + 14, py + 28), D2D1::Point2F(d.right - 14, py + 28), g_brush.Get(), 1.f);

    // 提示词列表标题（只有多个提示词时显示，单个时省掉）
    if (g_prompts.size() > 1)
        Text(T(S_SELECT_PROMPT), g_fLabel.Get(), R(d.left + 18, py + 32, d.right - 18, py + 48), C(P.sub));

    // 提示词列表
    int visible = std::min((int)g_prompts.size(), kDlgMaxVisible);
    if (g_prompts.empty())
    {
        Text(T(S_NO_PROMPTS), g_fLabel.Get(),
            R(d.left + 18, DlgListTop() + 12, d.right - 18, DlgListTop() + 36), C(P.sub));
    }
    else
    {
        g_rt->PushAxisAlignedClip(R(d.left, DlgListTop(), d.right, DlgListBottom()), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        for (int v = 0; v < visible; ++v)
        {
            int idx = g_promptScroll + v;
            if (idx >= (int)g_prompts.size()) break;
            D2D1_RECT_F pr = DlgPromptRect(v);
            bool sel = idx == g_promptSel;
            bool hot = g_hover == kHoverPrompt + idx;

            if (sel) Fill(pr, Ov(0.12f), 6.f);
            else if (hot) Fill(pr, Ov(0.06f), 6.f);

            D2D1_ELLIPSE circle = D2D1::Ellipse(D2D1::Point2F(pr.left + 14, (pr.top + pr.bottom) / 2), 7.f, 7.f);
            if (sel)
            {
                g_brush->SetColor(C(P.dark ? 0x60CDFF : 0x005FB8));
                g_rt->FillEllipse(circle, g_brush.Get());
                g_brush->SetColor(C(P.dark ? 0x2C2C2C : 0xFFFFFF));
                g_rt->FillEllipse(D2D1::Ellipse(circle.point, 3.f, 3.f), g_brush.Get());
            }
            else
            {
                g_brush->SetColor(Ov(0.25f));
                g_rt->DrawEllipse(circle, g_brush.Get(), 1.5f);
            }

            Text(g_prompts[idx], g_fTitle.Get(), R(pr.left + 30, pr.top + 6, pr.right - 8, pr.top + 24), C(P.text));
            Text(PromptDescription(g_prompts[idx]), g_fLabel.Get(), R(pr.left + 30, pr.top + 26, pr.right - 8, pr.top + 42), C(P.sub));
        }
        g_rt->PopAxisAlignedClip();

        if ((int)g_prompts.size() > kDlgMaxVisible)
        {
            if (g_promptScroll > 0)
                Text(L"", g_fIcon.Get(), R(d.right - 36, DlgListTop() - 2, d.right - 14, DlgListTop() + 14), C(P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);
            if (g_promptScroll + kDlgMaxVisible < (int)g_prompts.size())
                Text(L"", g_fIcon.Get(), R(d.right - 36, DlgListBottom() - 14, d.right - 14, DlgListBottom() + 2), C(P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);
        }
    }

    g_brush->SetColor(Ov(0.10f));
    g_rt->DrawLine(D2D1::Point2F(d.left, d.bottom - kDlgBottomH), D2D1::Point2F(d.right, d.bottom - kDlgBottomH), g_brush.Get(), 1.f);

    DlgButton(kDlgSite, T(S_WEBSITE), false);
    DlgButton(kDlgUninstall, T(S_UNINSTALL), false);
    DlgButton(kDlgInstall, T(S_INJECT), true);
}

// ---------------------------------------------------------------------------
// 免责声明
// ---------------------------------------------------------------------------
void DrawDisclaimer()
{
    D2D1_SIZE_F sz = g_rt->GetSize();
    g_brush->SetColor(C(0x000000, P.dark ? 0.60f : 0.40f));
    g_rt->FillRectangle(R(0, 0, sz.width, sz.height), g_brush.Get());

    D2D1_RECT_F d = DiscRect();
    Fill(d, C(P.dark ? 0x2C2C2C : 0xFFFFFF), 12.f);
    Stroke(d, Ov(0.14f), 12.f);

    Text(L"", g_fGlyph.Get(), R(d.left + (kDiscW - 24) / 2, d.top + 16, d.left + (kDiscW + 24) / 2, d.top + 44),
        C(P.dark ? 0x60CDFF : 0x005FB8), DWRITE_TEXT_ALIGNMENT_CENTER);   // Shield

    Text(T(S_DISCLAIMER_TITLE), g_fTitle.Get(), R(d.left + 20, d.top + 48, d.right - 20, d.top + 66), C(P.text), DWRITE_TEXT_ALIGNMENT_CENTER);

    Text(T(S_DISCLAIMER_BODY), g_fWrap.Get(), R(d.left + 24, d.top + 74, d.right - 24, d.bottom - 60), C(P.sub));

    D2D1_RECT_F btn = DiscAcceptRect();
    bool hot = g_hover == kHoverAccept;
    Fill(btn, C(P.dark ? 0x60CDFF : 0x005FB8, hot ? 0.88f : 1.f), 6.f);
    Text(T(S_DISCLAIMER_BTN), g_fTabSel.Get(), btn, C(P.dark ? 0x000000 : 0xFFFFFF), DWRITE_TEXT_ALIGNMENT_CENTER);
}

// ---------------------------------------------------------------------------
// 关于
// ---------------------------------------------------------------------------
void DrawAbout()
{
    D2D1_SIZE_F sz = g_rt->GetSize();
    g_brush->SetColor(C(0x000000, P.dark ? 0.50f : 0.30f));
    g_rt->FillRectangle(R(0, 0, sz.width, sz.height), g_brush.Get());

    D2D1_RECT_F d = AboutRect();
    Fill(d, C(P.dark ? 0x2C2C2C : 0xFFFFFF), 12.f);
    Stroke(d, Ov(0.14f), 12.f);

    // Logo SVG (Azure shield), bigger and more distinctive
    ID2D1SvgDocument* uaLogo = LoadSvgById(IDR_LOGO_UA);
    if (uaLogo)
    {
        float logoSize = 40.f;
        DrawSvgSized(uaLogo, R(d.left + (340.f - logoSize) / 2, d.top + 10,
            d.left + (340.f + logoSize) / 2, d.top + 10 + logoSize), 18.f);
    }
    else
    {
        Text(L"", g_fGlyph.Get(), R(d.left + (340.f - 24) / 2, d.top + 18, d.left + (340.f + 24) / 2, d.top + 44),
            C(P.dark ? 0x60CDFF : 0x005FB8), DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    // Title
    Text(L"Ua Toolbox", g_fTitle.Get(), R(d.left + 20, d.top + 50, d.right - 20, d.top + 68), C(P.text), DWRITE_TEXT_ALIGNMENT_CENTER);

    // Version
    std::wstring ver = std::wstring(T(S_ABOUT_TEXT)) + T(S_VERSION);
    Text(ver, g_fLabel.Get(), R(d.left + 20, d.top + 70, d.right - 20, d.top + 86), C(P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);

    // Update status（版本号正下方）
    // 已检查且有新版本：蓝色、可点击打开 release 页
    // 已检查且无新版本：灰色 "已是最新版本"
    // 还没检查完：灰色 "检查更新中..."
    D2D1_RECT_F ur = AboutUpdateRect();
    bool checked = UpdateChecked();
    std::wstring newVer = GetUpdateVersion();
    if (!newVer.empty())
    {
        bool upHot = g_hover == kHoverAboutUpdate;
        UINT32 accent = P.dark ? 0x60CDFF : 0x005FB8;
        std::wstring line = std::wstring(T(S_UPDATE_NEW)) + newVer;
        Text(line, g_fLabel.Get(), ur, C(accent, upHot ? 0.80f : 1.f), DWRITE_TEXT_ALIGNMENT_CENTER);

        // "立即更新" 按钮
        D2D1_RECT_F ub = AboutUpdateBtnRect();
        bool ubHot = g_hover == kHoverAboutUpdate2;
        bool downloading = g_updateDownloading;
        if (downloading)
        {
            Fill(ub, Ov(0.04f), 6.f);
            Text(T(S_UPDATE_DOWNLOADING), g_fTab.Get(), ub, C(P.sub, 0.65f), DWRITE_TEXT_ALIGNMENT_CENTER);
        }
        else
        {
            Fill(ub, C(accent, ubHot ? 0.88f : 1.f), 6.f);
            Text(T(S_UPDATE_BTN), g_fTabSel.Get(), ub, C(P.dark ? 0x000000 : 0xFFFFFF), DWRITE_TEXT_ALIGNMENT_CENTER);
        }
    }
    else
    {
        Text(checked ? T(S_UPDATE_LATEST) : T(S_UPDATE_CHECK),
            g_fLabel.Get(), ur, C(P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    // Separator
    g_brush->SetColor(Ov(0.10f));
    g_rt->DrawLine(D2D1::Point2F(d.left + 20, d.top + 140), D2D1::Point2F(d.right - 20, d.top + 140), g_brush.Get(), 1.f);

    // Legal text
    Text(T(S_ABOUT_LEGAL), g_fWrap.Get(), R(d.left + 24, d.top + 148, d.right - 24, d.bottom - 60), C(P.sub));

    // GitHub button
    D2D1_RECT_F gh = AboutGithubRect();
    bool ghHot = g_hover == kHoverAboutGithub;
    Fill(gh, Ov(ghHot ? 0.12f : 0.07f), 6.f);
    Stroke(gh, Ov(0.12f), 6.f);
    Text(L"", g_fIcon.Get(), R(gh.left + 10, gh.top, gh.left + 40, gh.bottom), C(P.text), DWRITE_TEXT_ALIGNMENT_CENTER);
    Text(T(S_GITHUB_LABEL), g_fTab.Get(), R(gh.left + 36, gh.top, gh.right - 10, gh.bottom), C(P.text), DWRITE_TEXT_ALIGNMENT_CENTER);

    // Close button
    bool hot = g_hover == kHoverAboutClose;
    D2D1_RECT_F cr = AboutCloseRect();
    if (hot) Fill(cr, Ov(0.08f), 6.f);
    Text(L"", g_fIcon.Get(), cr, C(hot ? P.text : P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);
}

// ---------------------------------------------------------------------------
// Skills 管理弹窗
// ---------------------------------------------------------------------------
void DrawSkillMgr()
{
    auto statuses = GetSkillStatuses();
    std::vector<int> filtered = FilteredSkills();
    int filteredCount = (int)filtered.size();
    int maxScroll = std::max(0, filteredCount - kSkillMaxVisible);
    if (g_skillScroll > maxScroll) g_skillScroll = maxScroll;
    if (g_skillScroll < 0) g_skillScroll = 0;

    D2D1_SIZE_F sz = g_rt->GetSize();
    g_brush->SetColor(C(0x000000, P.dark ? 0.45f : 0.25f));
    g_rt->FillRectangle(R(0, 0, sz.width, sz.height), g_brush.Get());

    D2D1_RECT_F d = SkillDlgRect();
    Fill(d, C(P.dark ? 0x2C2C2C : 0xFFFFFF), 10.f);
    Stroke(d, Ov(0.14f), 10.f);

    Text(T(S_SKILLS_TITLE), g_fTitle.Get(), R(d.left + 18, d.top + 16, d.right - 44, d.top + 34), C(P.text));
    wchar_t sub[64];
    swprintf_s(sub, 64, T(S_SKILL_COUNT_FMT), filteredCount, kSkillCount);
    Text(sub, g_fLabel.Get(), R(d.left + 18, d.top + 36, d.right - 44, d.top + 50), C(P.sub));

    bool closeHot = g_hover == kHoverSkillClose;
    D2D1_RECT_F cr = SkillCloseRect();
    if (closeHot) Fill(cr, Ov(0.08f), 6.f);
    Text(L"", g_fIcon.Get(), cr, C(closeHot ? P.text : P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);

    // 分类标签 pills
    D2D1_RECT_F pills[kCatCount];
    SkillCatPills(pills);
    for (int ci = 0; ci < kCatCount; ++ci)
    {
        bool selCat = (ci == g_skillCat);
        bool hotCat = g_hover == kHoverSkillCat + ci;
        if (selCat)
        {
            Fill(pills[ci], C(P.dark ? 0x60CDFF : 0x005FB8, hotCat ? 0.88f : 1.f), 11.f);
            Text(T((Str)(S_CAT_ALL + ci)), g_fTab.Get(), pills[ci],
                C(P.dark ? 0x000000 : 0xFFFFFF), DWRITE_TEXT_ALIGNMENT_CENTER);
        }
        else
        {
            Fill(pills[ci], Ov(hotCat ? 0.12f : 0.06f), 11.f);
            Stroke(pills[ci], Ov(0.12f), 11.f);
            Text(T((Str)(S_CAT_ALL + ci)), g_fTab.Get(), pills[ci], C(P.text), DWRITE_TEXT_ALIGNMENT_CENTER);
        }
    }

    // 搜索框
    D2D1_RECT_F sr = SkillSearchRect();
    bool searchHot = g_hover == kHoverSkillSearch;
    Fill(sr, Ov(0.06f), 6.f);
    Stroke(sr, Ov(searchHot ? 0.22f : 0.14f), 6.f);
    Text(L"", g_fIcon.Get(), R(sr.left + 6, sr.top, sr.left + 28, sr.bottom), C(P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);
    if (g_skillFilter.empty())
    {
        Text(T(S_SKILL_SEARCH_PLACEHOLDER), g_fTab.Get(),
            R(sr.left + 30, sr.top, sr.right - 10, sr.bottom), C(P.sub, 0.65f));
    }
    else
    {
        bool caret = (GetTickCount64() / 500) % 2 == 0;
        std::wstring shown = g_skillFilter + (caret ? L"|" : L" ");
        Text(shown, g_fTab.Get(),
            R(sr.left + 30, sr.top, sr.right - 10, sr.bottom), C(P.text));
    }

    // skill 列表
    int visible = std::min(filteredCount - g_skillScroll, kSkillMaxVisible);
    if (visible < 0) visible = 0;
    float listTop = SkillListTop();
    float listBottom = listTop + kSkillMaxVisible * kSkillItemH;
    g_rt->PushAxisAlignedClip(R(d.left, listTop, d.right, listBottom), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    for (int v = 0; v < visible; ++v)
    {
        int idxFilt = g_skillScroll + v;
        if (idxFilt >= filteredCount) break;
        int idx = filtered[idxFilt];
        D2D1_RECT_F ir = SkillItemRect(v);
        bool hot = g_hover == kHoverSkillMgr + idx;
        if (hot) Fill(ir, Ov(0.06f), 4.f);

        // 绿点表示已安装到至少一个工具
        UINT32 dot = statuses[idx].installedCount > 0 ? kGreen : P.sub;
        float cy = (ir.top + ir.bottom) / 2;
        g_brush->SetColor(C(dot));
        g_rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(ir.left + 8, cy), 3.5f, 3.5f), g_brush.Get());

        Text(statuses[idx].name, g_fLabel.Get(), R(ir.left + 20, ir.top + 2, ir.right - 86, ir.bottom - 2), C(P.text));

        if (statuses[idx].installedCount > 0)
        {
            // 已装到几个工具（右侧中段，右边再留 26px 给 ✕ 按钮）
            wchar_t cnt[32];
            swprintf_s(cnt, 32, T(S_SKILL_TOOL_COUNT_FMT), statuses[idx].installedCount);
            Text(cnt, g_fLabel.Get(), R(ir.right - 86, ir.top + 2, ir.right - 30, ir.bottom - 2), C(P.sub), DWRITE_TEXT_ALIGNMENT_TRAILING);

            // ✕ 卸载按钮
            D2D1_RECT_F xr = SkillRemoveRect(v);
            bool xHot = g_hover == kHoverSkillRemove + idx;
            if (xHot) Fill(xr, Ov(0.12f), 4.f);
            Text(L"", g_fIcon.Get(), xr, C(xHot ? P.text : P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);  // Cancel
        }
    }
    g_rt->PopAxisAlignedClip();

    // 空列表提示
    if (filteredCount == 0)
    {
        Text(T(S_SKILL_NO_MATCH), g_fLabel.Get(),
            R(d.left + 18, listTop + 20, d.right - 18, listTop + 40),
            C(P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    // 滚动提示
    if (filteredCount > kSkillMaxVisible)
    {
        if (g_skillScroll > 0)
            Text(L"", g_fIcon.Get(), R(d.right - 36, listTop - 2, d.right - 14, listTop + 14), C(P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);
        if (g_skillScroll + kSkillMaxVisible < filteredCount)
            Text(L"", g_fIcon.Get(), R(d.right - 36, listBottom - 14, d.right - 14, listBottom + 2), C(P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    // 底部分隔线 + 按钮
    g_brush->SetColor(Ov(0.10f));
    g_rt->DrawLine(D2D1::Point2F(d.left, d.bottom - 52.f), D2D1::Point2F(d.right, d.bottom - 52.f), g_brush.Get(), 1.f);

    D2D1_RECT_F btn = SkillFolderRect();
    bool btnHot = g_hover == kHoverSkillFolder;
    Fill(btn, Ov(btnHot ? 0.12f : 0.07f), 6.f);
    Stroke(btn, Ov(0.12f), 6.f);
    Text(T(S_SKILLS_FOLDER), g_fTab.Get(), btn, C(P.text), DWRITE_TEXT_ALIGNMENT_CENTER);
}

// ---------------------------------------------------------------------------
// Skills 管理弹窗需要接收 WM_CHAR / WM_KEYDOWN 更新搜索框（文本输入必须经 WndProc），
// 以及点击分类标签改变 g_skillCat。用 SetWindowLongPtr 挂一个 WndProc 钩子完成这些。
// About "立即更新" 按钮已经走 HitTest + WM_LBUTTONUP 正常路径，不再在这里拦。
// ---------------------------------------------------------------------------
static LRESULT CALLBACK HookWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (g_skillMgr)
    {
        if (msg == WM_CHAR)
        {
            // ESC(27) / Enter(13) / Tab(9) / Backspace(8) 等控制字符交给原 WndProc
            if (wp >= 32 && wp < 0x10000)
            {
                g_skillFilter.push_back((wchar_t)wp);
                g_skillScroll = 0;
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }
        }
        else if (msg == WM_KEYDOWN && wp == VK_BACK)
        {
            if (!g_skillFilter.empty())
            {
                g_skillFilter.pop_back();
                g_skillScroll = 0;
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            return 0;
        }
        else if (msg == WM_LBUTTONUP)
        {
            // app.cpp 不知道分类标签，这里拦截
            if (g_hover >= kHoverSkillCat && g_hover < kHoverSkillCat + kCatCount)
            {
                g_skillCat = g_hover - kHoverSkillCat;
                g_skillScroll = 0;
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }
            // 搜索框自身的点击不消费（也不需要特殊处理，让 app.cpp 走原路径）
        }
    }
    return CallWindowProc(g_oldWndProc, hWnd, msg, wp, lp);
}

void EnsureHook(HWND hWnd)
{
    if (g_oldWndProc) return;
    g_oldWndProc = (WNDPROC)SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)HookWndProc);
}

// Skills 管理弹窗刚被打开时，清空筛选条件。
// Render 每帧调用；用 g_skillMgrPrev 做边沿检测。
void MaybeResetSkillFilter()
{
    if (g_skillMgr && !g_skillMgrPrev)
    {
        g_skillFilter.clear();
        g_skillCat = kCatAll;
        g_skillScroll = 0;
    }
    g_skillMgrPrev = g_skillMgr;
}
