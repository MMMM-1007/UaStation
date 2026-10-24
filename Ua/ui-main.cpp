// ui-main.cpp : 主界面布局、卡片绘制、命中测试与 Render 入口
//

#include "ui-draw.h"
#include "ui-dialogs.h"

// ---------------------------------------------------------------------------
// 布局：标签栏、右上角按钮、条目卡片
// ---------------------------------------------------------------------------

// 右上角按钮：设置 / 最小化 / 关闭，id 为 kHoverGear..kHoverClose
static D2D1_RECT_F ButtonRect(int id)
{
    float right = kW - kPad - 4.f - (kHoverClose - id) * 32.f;
    return R(right - 30.f, kPad + 4.f, right, kPad + 32.f);
}

static D2D1_RECT_F GearRect() { return ButtonRect(kHoverGear); }

static D2D1_RECT_F TabRect(int i)
{
    float left = kPad + 4.f, right = GearRect().left - 6.f;
    float w = (right - left) / kTabCount;
    return R(left + i * w, kPad + 4.f, left + (i + 1) * w, kPad + 32.f);
}

struct Slot
{
    D2D1_RECT_F rect;
    int item;
};

// 当前标签对应分组的条目，两列排列
static void Layout(std::vector<Slot>& slots)
{
    float y = kTop;
    float cw = (kW - 2 * kPad - kGap) / 2;
    int col = 0;
    for (int i = 0; i < kItemCount; ++i)
    {
        if (kItems[i].group != Group(g_tab)) continue;
        float x = kPad + col * (cw + kGap);
        slots.push_back({ R(x, y, x + cw, y + kCardH), i });
        if (++col == 2) { col = 0; y += kCardH + kGap; }
    }
}

static bool In(D2D1_RECT_F r, float x, float y) { return x >= r.left && x < r.right && y >= r.top && y < r.bottom; }

// ---------------------------------------------------------------------------
// 命中测试
// ---------------------------------------------------------------------------
int HitTest(float x, float y)
{
    if (g_disclaimer)
    {
        if (In(DiscAcceptRect(), x, y)) return kHoverAccept;
        return -2;  // block everything else
    }
    if (g_aboutOpen)
    {
        if (In(AboutCloseRect(), x, y)) return kHoverAboutClose;
        if (!In(AboutRect(), x, y)) return kHoverDlgOut;
        if (In(AboutGithubRect(), x, y)) return kHoverAboutGithub;
        // 更新链接只有在真的发现新版本时才可点击
        if (UpdateAvailable() && In(AboutUpdateRect(), x, y)) return kHoverAboutUpdate;
        // "立即更新" 按钮：发现新版本且没在下载时可点击
        if (UpdateAvailable() && !g_updateDownloading && In(AboutUpdateBtnRect(), x, y)) return kHoverAboutUpdate2;
        return kHoverDlgBg;
    }
    if (g_skillMgr)
    {
        D2D1_RECT_F d = SkillDlgRect();
        if (!In(d, x, y)) return kHoverDlgOut;
        if (In(SkillCloseRect(), x, y)) return kHoverSkillClose;
        if (In(SkillFolderRect(), x, y)) return kHoverSkillFolder;
        // 分类标签
        D2D1_RECT_F pills[kCatCount];
        SkillCatPills(pills);
        for (int i = 0; i < kCatCount; ++i)
            if (In(pills[i], x, y)) return kHoverSkillCat + i;
        // 搜索框（点一下不干啥，键盘输入由钩子处理）
        if (In(SkillSearchRect(), x, y)) return kHoverSkillSearch;
        // 过滤后的 skill 列表
        std::vector<int> filtered = FilteredSkills();
        int filteredCount = (int)filtered.size();
        int maxScroll = std::max(0, filteredCount - kSkillMaxVisible);
        int startIdx = std::min(std::max(0, g_skillScroll), maxScroll);
        int visible = std::min(filteredCount - startIdx, kSkillMaxVisible);
        for (int v = 0; v < visible; ++v)
        {
            int idxFilt = startIdx + v;
            if (idxFilt >= filteredCount) break;
            int idx = filtered[idxFilt];
            if (!In(SkillItemRect(v), x, y)) continue;
            if (In(SkillRemoveRect(v), x, y)) return kHoverSkillRemove + idx;
            return kHoverSkillMgr + idx;
        }
        return kHoverDlgBg;
    }
    if (g_dlg >= 0)
    {
        D2D1_RECT_F d = DlgRect();
        if (!In(d, x, y)) return kHoverDlgOut;
        for (int b = 0; b < kDlgButtonCount; ++b)
            if (In(DlgButtonRect(b), x, y)) return kHoverDlg + b;
        // 提示词列表
        int visible = std::min((int)g_prompts.size(), kDlgMaxVisible);
        for (int v = 0; v < visible; ++v)
            if (In(DlgPromptRect(v), x, y)) return kHoverPrompt + g_promptScroll + v;
        return kHoverDlgBg;
    }
    if (g_menuOpen && In(MenuRect(), x, y))
    {
        for (int i = 0; i < kMenuCount; ++i)
            if (kMenu[i].cmd && In(MenuItemRect(i), x, y)) return kHoverMenu + i;
        return kHoverMenuBg;
    }
    for (int i = 0; i < kTabCount; ++i)
        if (In(TabRect(i), x, y)) return i;
    for (int id = kHoverGear; id <= kHoverClose; ++id)
        if (In(ButtonRect(id), x, y)) return id;
    std::vector<Slot> slots;
    Layout(slots);
    for (const Slot& s : slots)
        if (In(s.rect, x, y)) return kHoverCard + s.item;
    return -1;
}

// ---------------------------------------------------------------------------
// 条目卡片绘制（标签页主体内容）
// ---------------------------------------------------------------------------
static void DrawTabs()
{
    D2D1_RECT_F bar = R(kPad, kPad, kW - kPad, kPad + 36.f);
    Fill(bar, Ov(0.06f), 8.f);

    for (int i = 0; i < kTabCount; ++i)
    {
        D2D1_RECT_F r = TabRect(i);
        if (i == g_tab)
        {
            Fill(R(r.left + 2, r.top, r.right - 2, r.bottom), Ov(0.11f), 6.f);
            Text(T(i == 0 ? S_TAB_EDITOR : S_TAB_TOOLS), g_fTabSel.Get(), r, C(P.text), DWRITE_TEXT_ALIGNMENT_CENTER);
        }
        else
        {
            if (i == g_hover) Fill(R(r.left + 2, r.top, r.right - 2, r.bottom), Ov(0.05f), 6.f);
            Text(T(i == 0 ? S_TAB_EDITOR : S_TAB_TOOLS), g_fTab.Get(), r, C(i == g_hover ? P.text : P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);
        }
    }

    const wchar_t* glyphs[] = { L"", L"", L"" };     // Setting / ChromeMinimize / ChromeClose
    for (int id = kHoverGear; id <= kHoverClose; ++id)
    {
        D2D1_RECT_F r = ButtonRect(id);
        bool hot = g_hover == id || (id == kHoverGear && g_menuOpen);
        bool closeHot = hot && id == kHoverClose;
        if (hot) Fill(r, closeHot ? C(0xC42B1C) : Ov(0.08f), 6.f);
        Text(glyphs[id - kHoverGear], g_fIcon.Get(), r, C(closeHot ? 0xFFFFFF : hot ? P.text : P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    g_brush->SetColor(Ov(0.10f));
    g_rt->DrawLine(D2D1::Point2F(kPad, kTop - 8.f), D2D1::Point2F(kW - kPad, kTop - 8.f), g_brush.Get(), 1.f);
}

// 需要暴露给 ui-dialogs.cpp 的 DrawDialog 使用（画弹窗标题的小 logo）
void DrawIcon(int i, D2D1_RECT_F tile)
{
    const Item& it = kItems[i];
    ID2D1SvgDocument* doc = it.logo ? Logo(i) : nullptr;

    if (doc && it.framed)
    {
        DrawSvg(doc, tile);
        return;
    }

    Fill(tile, Ov(0.08f), 6.f);
    if (doc)
    {
        float inset = (tile.right - tile.left) * 0.16f;
        DrawSvg(doc, R(tile.left + inset, tile.top + inset, tile.right - inset, tile.bottom - inset));
    }
    else if (it.glyph)
    {
        Text(it.glyph, g_fGlyph.Get(), tile, C(P.text), DWRITE_TEXT_ALIGNMENT_CENTER);
    }
    else
    {
        Text(std::wstring(1, it.name[0]), g_fLetter.Get(), tile, C(P.text), DWRITE_TEXT_ALIGNMENT_CENTER);
    }
}

static const wchar_t* ItemName(int i)
{
    if (kItems[i].group == kGroupTools)
    {
        if (wcscmp(kItems[i].name, L"Skills 管理") == 0) return T(S_TOOLS_SKILLS);
        if (wcscmp(kItems[i].name, L"满血中转站") == 0) return T(S_TOOLS_RELAY);
        if (wcscmp(kItems[i].name, L"使用教程") == 0) return T(S_TOOLS_TUTORIAL);
        if (wcscmp(kItems[i].name, L"不会用点这里") == 0) return T(S_TOOLS_HELP);
        if (wcscmp(kItems[i].name, L"加入QQ群") == 0) return T(S_TOOLS_QQ);
    }
    return kItems[i].name;
}

static const wchar_t* ItemSub(int i)
{
    if (kItems[i].group == kGroupTools)
    {
        if (wcscmp(kItems[i].name, L"Skills 管理") == 0) return T(S_TOOLS_SKILLS_SUB);
        if (wcscmp(kItems[i].name, L"满血中转站") == 0) return T(S_TOOLS_RELAY_SUB);
        if (wcscmp(kItems[i].name, L"使用教程") == 0) return T(S_TOOLS_TUTORIAL_SUB);
        if (wcscmp(kItems[i].name, L"不会用点这里") == 0) return T(S_TOOLS_HELP_SUB);
        if (wcscmp(kItems[i].name, L"加入QQ群") == 0) return T(S_TOOLS_QQ_SUB);
    }
    return kItems[i].sub;
}

static void DrawCard(const Slot& s)
{
    const Item& it = kItems[s.item];
    D2D1_RECT_F r = s.rect;
    bool hot = g_hover == kHoverCard + s.item;

    Fill(r, Ov(hot ? 0.10f : 0.05f), 8.f);
    Stroke(r, Ov(hot ? 0.18f : 0.12f), 8.f);

    float cy = (r.top + r.bottom) / 2;
    D2D1_RECT_F tile = R(r.left + 10, cy - 14, r.left + 38, cy + 14);
    DrawIcon(s.item, tile);

    // 已注入：logo 右下角一个绿点，外面套一圈底色把它和 logo 隔开
    if (it.key && g_state[s.item] == kInstalled)
    {
        D2D1_ELLIPSE ring = D2D1::Ellipse(D2D1::Point2F(tile.right - 1, tile.bottom - 1), 6.f, 6.f);
        g_brush->SetColor(C(P.bg));
        g_rt->FillEllipse(ring, g_brush.Get());
        g_brush->SetColor(Ov(hot ? 0.10f : 0.05f));
        g_rt->FillEllipse(ring, g_brush.Get());
        g_brush->SetColor(C(kGreen));
        g_rt->FillEllipse(D2D1::Ellipse(ring.point, 4.5f, 4.5f), g_brush.Get());
    }

    float tx = tile.right + 10, tr = r.right - 26;
    if (it.sub[0])
    {
        Text(ItemName(s.item), g_fTitle.Get(), R(tx, cy - 16, tr, cy + 1), C(P.text));
        Text(ItemSub(s.item), g_fLabel.Get(), R(tx, cy + 2, tr, cy + 17), C(P.sub));
    }
    else
    {
        Text(ItemName(s.item), g_fTitle.Get(), R(tx, cy - 8, tr, cy + 10), C(P.text));
    }
    Text(L"", g_fIcon.Get(), R(tr, r.top, r.right - 8, r.bottom), C(hot ? P.text : P.sub), DWRITE_TEXT_ALIGNMENT_CENTER);   // ChevronRight
}

// ---------------------------------------------------------------------------
// 渲染入口
// ---------------------------------------------------------------------------
void Render(HWND hWnd)
{
    if (!CreateTarget(hWnd)) return;
    EnsureHook(hWnd);

    // 每次打开 Skills 管理弹窗时清空筛选，恢复到"全部"
    MaybeResetSkillFilter();

    g_rt->BeginDraw();
    g_rt->SetTransform(D2D1::Matrix3x2F::Identity());
    g_rt->Clear(D2D1::ColorF(0, 0, 0, 0));

    D2D1_SIZE_F sz = g_rt->GetSize();
    // Mica 模式下窗口背景由 DWM 绘制，这里保持透明；否则用主题色填不透明底
    if (!IsMicaActive())
    {
        g_brush->SetColor(C(P.bg));
        g_rt->FillRectangle(R(0, 0, sz.width, sz.height), g_brush.Get());
    }
    g_brush->SetColor(Ov(0.14f));
    g_rt->DrawRectangle(R(0.5f, 0.5f, sz.width - 0.5f, sz.height - 0.5f), g_brush.Get(), 1.f);

    DrawTabs();

    std::vector<Slot> slots;
    Layout(slots);
    for (const Slot& s : slots)
        DrawCard(s);

    if (g_menuOpen)
        DrawMenu();
    if (g_dlg >= 0)
        DrawDialog();
    if (g_skillMgr)
        DrawSkillMgr();
    if (g_aboutOpen)
        DrawAbout();
    if (g_disclaimer)
        DrawDisclaimer();

    // 底部状态栏
    {
        D2D1_RECT_F bar = R(0, kH - kStatusH, kW, kH);
        Fill(bar, Ov(0.04f), 0.f);
        wchar_t buf[64];
        _snwprintf_s(buf, _TRUNCATE, L"v%s", T(S_VERSION));
        Text(buf, g_fLabel.Get(), R(kPad, kH - kStatusH, kW - kPad, kH), C(P.sub), DWRITE_TEXT_ALIGNMENT_TRAILING);
    }

    if (!g_toast.empty())
    {
        D2D1_RECT_F t = R(kPad + 40, kH - kPad - 40, kW - kPad - 40, kH - kPad - 8);
        Fill(t, P.dark ? C(0x2C2C2C, 0.98f) : C(0xFFFFFF, 0.98f), 16.f);
        Stroke(t, Ov(0.18f), 16.f);
        Text(g_toast, g_fTab.Get(), t, C(P.text), DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    if (g_rt->EndDraw() == D2DERR_RECREATE_TARGET)
    {
        g_brush.Reset();
        g_rt.Reset();
    }
}
