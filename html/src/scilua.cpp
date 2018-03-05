#include "htmlua.h"
#include <Scintilla.h>
struct LuaScintilla : public HTMLuaWnd
{
    HWND m_hScintillaWnd;
    SciFnDirect m_pFnDirect;//直接调用功能函数
    WNDPROC m_oldWndProc;  
    sptr_t	    m_ptr;//上下文
    ~LuaScintilla()
    {
        //printf("Scintilla[%X] Destroy\n", m_hScintillaWnd);
    }
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        HTMLuaWnd* pThis = (HTMLuaWnd*)GetWindowLongPtr(hWnd,GWL_USERDATA);
        if(!pThis) return ::DefWindowProc(hWnd,uMsg,wParam,lParam);
        LRESULT ret = pThis->OnWindowProc(hWnd,uMsg,wParam,lParam);
        if (uMsg == WM_DESTROY) pThis->UnuseElement();
        return ret;
    }
    virtual LRESULT OnWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_DESTROY){
            SetWindowLongPtr(hWnd, GWL_USERDATA, 0);
            SetWindowLongPtrW(hWnd, GWL_WNDPROC, (LONG_PTR)m_oldWndProc);
        }
        else if (uMsg == WM_DROPFILES)
            OnDropFiles(::GetParent(hWnd), (HDROP)wParam);
        return m_oldWndProc(hWnd, uMsg, wParam, lParam);
    }
    virtual BOOL OnNotify(NMHDR* nm, HELEMENT he) 
    {
        SCNotification& scn = *(SCNotification*)nm;
        LUA_NMHDR param(nm->hwndFrom, nm->code, 0);
        switch(nm->code){
        case SCN_CHARADDED:
            param.name = "CHARADDED";
            param.argv.push_back(json::value(scn.ch));
            break;
        case SCN_DWELLSTART:
            param.name = "DWELLSTART"; goto dwell;
        case SCN_DWELLEND:
            param.name = "DWELLEND";
dwell:
            param.argv.push_back(json::value(scn.position));
            param.argv.push_back(json::value(scn.x));
            param.argv.push_back(json::value(scn.y));
            break;
        case SCN_ZOOM:
            param.name = "ZOOM"; break;
        case SCN_FOCUSIN:
            param.name = "FOCUSIN"; break;
        case SCN_FOCUSOUT:
            param.name = "FOCUSOUT"; break;
        default:
            return FALSE;
        }
        return HTMLuaWnd::OnNotify((NMHDR*)&param, he);
    }
    int	Init(lua_State*L)
    {
        bool embed = true;
        HWND hwnd = luaH_opthwnd(L, 1);
        HELEMENT he = luaH_optelement(L,2);
        if (hwnd && !::IsWindow(hwnd)){
            delete this;
            return 0;
        }
        if (he) {
            htmlayout::dom::element el(he);
            embed = (wcscmp(el.attribute("-overlap",L"false"), L"true") != 0);
        }

        m_hScintillaWnd = ::CreateWindowExW(0, L"Scintilla", L"",
            (hwnd && embed)  ? WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP | WS_CLIPCHILDREN
            : WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hwnd, (HMENU)LUA_NOTIFY_IDFROM, 0, 0);

        if(!m_hScintillaWnd){
            delete this;
            return 0;
        }

        m_pFnDirect = (SciFnDirect)::SendMessage(m_hScintillaWnd,SCI_GETDIRECTFUNCTION,0,0);
        m_ptr = (sptr_t)::SendMessage(m_hScintillaWnd,SCI_GETDIRECTPOINTER,0,0);

        SetWindowLongPtrW(m_hScintillaWnd, GWL_USERDATA, (LONG_PTR)(HTMLuaWnd*)this);
        m_oldWndProc = (WNDPROC)SetWindowLongPtrW(m_hScintillaWnd, GWL_WNDPROC,  (LONG_PTR)WindowProc);

        ::ShowWindow(m_hScintillaWnd, SW_SHOW);
        ::SetFocus(m_hScintillaWnd);

        lua_pushlightuserdata(L,m_hScintillaWnd);
        lua_pushlightuserdata(L,this);
        return 2;
    }
    //向控件发送消息
    sptr_t  SendEditor(unsigned int iMessage, uptr_t wParam = 0, sptr_t lParam = 0)
    {
        if(m_pFnDirect == NULL || m_ptr == NULL)
            return NULL;
        return (*m_pFnDirect)(m_ptr, iMessage, wParam, lParam);
    }

    //发送消息
    int  SendMsg(lua_State* L)
    {
        const unsigned iMsg = luaL_checkinteger(L, 1);
        uptr_t wParam = 0;
        uptr_t lParam = 0;
        switch(lua_type(L, 2)){
        case LUA_TNUMBER:
            wParam = (uptr_t)luaL_checkinteger(L, 2); break;
        case LUA_TBOOLEAN:
             wParam = (uptr_t)lua_toboolean(L, 2); break;
        default:
            if (!lua_isnoneornil(L,2))
                wParam = (uptr_t)luaL_checkstring(L, 2);
        }
        switch(lua_type(L, 3)){
        case LUA_TNUMBER:
            lParam = (uptr_t)luaL_checkinteger(L, 3);break;
        case LUA_TBOOLEAN:
            lParam = (uptr_t)lua_toboolean(L, 3);break;
        default:
            if (!lua_isnoneornil(L,3))
                lParam = (uptr_t)luaL_checkstring(L, 3);
        }
        lua_pushinteger(L, SendEditor(iMsg, wParam, lParam));
        return 1;
    }
    int RecvMsg(lua_State* L)
    {
        unsigned int iMsg = luaL_checkinteger(L, 1);
        int nLength = SendEditor(iMsg);
        char*  pText = new char[nLength+1];
        if(pText == NULL)
            return 0;
        memset(pText, 0, nLength + 1);
        nLength= SendEditor(iMsg, nLength+1, (sptr_t)pText);
        lua_pushlstring(L, pText, nLength);
        return 1;
    }

    //获得指定位置的字符串
    //- start end.  range text
    //- pos, bool.   word at pos
    int GetTextRange(lua_State* L)
    {
        TextRange tr;
        if (lua_type(L, 2) == LUA_TBOOLEAN)
        {
            const int pos = luaL_checkinteger(L, 1);
            int onlyWordCharacters = lua_toboolean(L, 2);
            tr.chrg.cpMin = SendEditor(SCI_WORDSTARTPOSITION, pos, onlyWordCharacters);
            tr.chrg.cpMax = SendEditor(SCI_WORDENDPOSITION, pos, onlyWordCharacters);
        }
        else
        {
            tr.chrg.cpMin	= luaL_checkinteger(L, 1);
            tr.chrg.cpMax   = luaL_checkinteger(L, 2);
            if(tr.chrg.cpMax < 0)
                tr.chrg.cpMax = SendEditor(SCI_GETTEXTLENGTH);
        }
        Sci_PositionCR nLen = tr.chrg.cpMax - tr.chrg.cpMin;
        if(nLen <= 0)
            return 0;

        char* pText = new char[nLen + 1];
        if(pText == NULL)
            return 0;
        tr.lpstrText = pText;
        memset(pText, 0, nLen + 1);
        nLen = SendEditor(SCI_GETTEXTRANGE, 0, (sptr_t)&tr);
        lua_pushlstring(L, pText, nLen);
        delete[] pText;

        lua_pushinteger(L, tr.chrg.cpMin);
        lua_pushinteger(L, tr.chrg.cpMax);
        return 3;
    }
    int GetElementHwnd(lua_State* L)
    {
        lua_pushinteger(L, ::IsWindow(m_hScintillaWnd) ? 0: 1);
        lua_pushlightuserdata(L, m_hScintillaWnd);
        return 2;
    }
};
int luaopen_scilua(lua_State* L)
{
    std::wstring fpath(aux::a2w(luaL_checkstring(L,2)));
    std::wstring::size_type pos = fpath.find_last_of(L"/\\");
    fpath = fpath.substr(0, pos+1) + L"SciLexer.dll";
    if (!LoadLibraryW(fpath.c_str()))
        luaL_error(L, "invalid %s", fpath.c_str());

    static LuaClass<LuaScintilla>::RegType methods[]={
#define _(x)  {#x, &LuaScintilla::x}
        _(SendMsg),
        _(RecvMsg),
        _(GetTextRange),
        _(GetElementHwnd),
#undef _
        {0,0}
    };
    LuaClass<LuaScintilla>::ClassTable(L, methods);
    return 1;
}
