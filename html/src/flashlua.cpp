#include "htmlua.h"
struct LuaFlash : public HTMLuaWnd, public OleBox::Stub
{
    HWND m_hWnd;
    virtual void Invalidate(HRGN hRGN, LPCRECT pRect, bool fErase){ 
        if (hRGN)
            ::InvalidateRgn(m_hWnd, hRGN, fErase);
        else
            ::InvalidateRect(m_hWnd, pRect, fErase);
    }
    virtual bool Callback(const wchar_t func[], const _variant_t* argv, _variant_t& retval){ return false; }

    OleBox* m_flash;
    LuaFlash():m_flash(0){}
    ~LuaFlash()
    {
        //printf("LuaFlash[%X] Destroy\n", m_hWnd);
        if (m_flash) m_flash->Destroy();
    }
    LRESULT OnWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (uMsg == WM_SIZE && m_flash){
            RECT rt ={0, 0, LOWORD(lParam), HIWORD(lParam)};
            m_flash->Resize(rt);
        }
        return HTMLuaWnd::OnWindowProc(hWnd, uMsg, wParam, lParam);
    }
    int	Init(lua_State*L)
    {
        bool embed = true;
        std::wstring cwd, movie;
        HWND hwnd = luaH_opthwnd(L, 1);
        HELEMENT he = luaH_optelement(L,2);
        if (hwnd && !::IsWindow(hwnd)){
            delete this;
            return 0;
        }
        if (he) {
            htmlayout::dom::element el(he);
            embed = (wcscmp(el.attribute("-overlap",L"false"), L"true") != 0);
            cwd = el.url(L"");
            movie = el.attribute("-movie", L"");
            if (movie.empty()) movie = el.get_value().get(L"");
        }
        else {
            wchar_t buf[1024];
            cwd = _wgetcwd(buf, sizeof(buf));
        }

        hwnd = CreateWnd(hwnd, embed);
        if (!hwnd) {
            delete this;
            return 0;
        }
        m_flash = OleBox_ShockwaveFlash(*this, cwd.c_str(), hwnd);
        if (!m_flash) {
            delete this;
            return 0;
        }
        if (!movie.empty() && !m_flash->Invoke(L"movie", movie.c_str())) {
            delete this;
            return 0;
        }
        m_hWnd = hwnd;
        lua_pushlightuserdata(L,hwnd);
        lua_pushlightuserdata(L,this);
        return 2;
    }
    int ControlSetValue(lua_State* L)
    {
        std::wstring url(aux::a2w(luaL_checkstring(L,1)));
        const BOOL ret = ::IsWindow(m_hWnd) && m_flash->Invoke(L"movie", url.c_str());
        lua_pushinteger(L, ret ? HLDOM_OK : HLDOM_INVALID_HWND);
        return 1;
    }
    int GetElementHwnd(lua_State* L)
    {
        lua_pushinteger(L, ::IsWindow(m_hWnd) ? HLDOM_OK: HLDOM_INVALID_HWND);
        lua_pushlightuserdata(L, m_hWnd);
        return 2;
    }
    int XCallBehaviorMethod(lua_State* L)
    {
        LuaXCallParam params(L);
        lua_pushinteger(L, script_call_olebox(m_flash, params) ? HLDOM_OK : HLDOM_INVALID_HWND);
        return 1 + luaH_pushargv(L, 1, &params.retval);
    }
};

int luaopen_flash(lua_State* L)
{
    static LuaClass<LuaFlash>::RegType methods[]={
#define _(x)  {#x, &LuaFlash::x}
        _(ControlSetValue),
        _(GetElementHwnd),
        _(XCallBehaviorMethod),
#undef _
        {0,0}
    };
    LuaClass<LuaFlash>::ClassTable(L, methods);
    return 1;
}
