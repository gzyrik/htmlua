#include <olebox.h>
#include "htmlua.h"
struct LuaWeb: public HTMLuaWnd,public OleBox::Stub
{
    HWND m_hWnd;
    virtual void Invalidate(HRGN hRGN, LPCRECT pRect, bool fErase){ 
        if (hRGN)
            ::InvalidateRgn(m_hWnd, hRGN, fErase);
        else
            ::InvalidateRect(m_hWnd, pRect, fErase);
    }
    virtual bool Callback(const wchar_t func[], const VARIANT* argv,  VARIANT& retva){ return false; }
    OleBox* m_web;
    LuaWeb():m_web(0){}
    ~LuaWeb()
    {
        //printf("WebView[%X] Destroy\n", m_hWnd);
        if (m_web) m_web->Destroy();
    }
    LRESULT OnWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (uMsg == WM_SIZE && m_web){
            RECT rt ={0, 0, LOWORD(lParam), HIWORD(lParam)};
            m_web->Resize(rt);
        }
        return HTMLuaWnd::OnWindowProc(hWnd, uMsg, wParam, lParam);
    }
    int	Init(lua_State*L)
    {
        bool embed = true;
        std::wstring cwd, url;
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
            url = el.attribute("-url", L"");
            if (url.empty()) url = el.get_value().get(L"");
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
        m_web = OleBox_WebBrowser(*this, cwd.c_str(), hwnd);
        if (!m_web) {
            delete this;
            return 0;
        }
        if (!url.empty() && !m_web->Invoke(L"navigate", url.c_str())) {
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
        const BOOL ret = ::IsWindow(m_hWnd) && m_web->Invoke(L"navigate", url.c_str());
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
        lua_pushinteger(L, script_call_olebox(m_web,params) ? HLDOM_OK : HLDOM_INVALID_HWND);
        return 1 + luaH_pushargv(L, 1, &params.retval);
    }
};

int luaopen_web(lua_State* L)
{
    static LuaClass<LuaWeb>::RegType methods[]={
#define _(x)  {#x, &LuaWeb::x}
        _(ControlSetValue),
        _(GetElementHwnd),
        _(XCallBehaviorMethod),
#undef _
        {0,0}
    };
    LuaClass<LuaWeb>::ClassTable(L, methods);
    return 1;
}
