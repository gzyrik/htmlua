#include "htmlua.h"
#include "behavior_lua.h"
#define  WM_HTMLAYOUT_LOAD_URL (WM_USER+1)
template<typename T> class HtmlWnd : public HTMLuaWnd, public htmlayout::event_handler, public htmlayout::notification_handler<T>
{
private:
    LRESULT OnClose(HWND hWnd) { return ::DestroyWindow(hWnd); }
    LRESULT OnDestroy(HWND hWnd) { return 0; }
    virtual LRESULT OnWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        T* pThis = (T*)this;
        {
            BOOL bHandled = FALSE;
            LRESULT lResult = HTMLayoutProcND(hWnd,uMsg,wParam,lParam, &bHandled);
            if(bHandled)
                return lResult;
        }
        if (uMsg == WM_CREATE){
            htmlayout::attach_event_handler(hWnd, pThis);
            setup_callback(hWnd);
        } else if (uMsg == WM_HTMLAYOUT_LOAD_URL)
            return ::HTMLayoutLoadFile(hWnd, aux::a2w(m_url.c_str()));
        else if (uMsg ==  WM_CLOSE)
            return pThis->OnClose(hWnd); 
        else if (uMsg == WM_DESTROY)
            return pThis->OnDestroy(hWnd);
        return ::DefWindowProc(hWnd,uMsg,wParam,lParam);
    }
protected:
    HtmlWnd():event_handler(HANDLE_ALL){}
    std::string m_url;
    BOOL LoadHtml(HWND hWnd, LPCBYTE pb, DWORD nBytes)
    {
        ASSERT(::IsWindow(hWnd));
        return ::HTMLayoutLoadHtml(hWnd, pb, nBytes);
    }
    BOOL LoadUrl(HWND hWnd, LPCSTR url)
    {
        m_url = url;
        return ::PostMessageW(hWnd, WM_HTMLAYOUT_LOAD_URL, 0, 0);
    }
    BOOL LoadHtmlResource(LPCTSTR name)
    {   
        PBYTE pb = NULL;
        DWORD cb = 0;
        HMODULE hModule = AfxGetResourceHandle();
        HRSRC hrsrc = ::FindResource( hModule, name, MAKEINTRESOURCE(RT_HTML));

        if(!hrsrc) 
            return FALSE;

        HGLOBAL hgres = ::LoadResource(hModule, hrsrc);
        if(!hgres) return FALSE;

        pb = (PBYTE)::LockResource(hgres); if (!pb) return FALSE;
        cb = ::SizeofResource(hModule, hrsrc); if (!cb) return FALSE;

        return LoadHtml(pb,cb);
    }
};

struct LuaHtmlView : public HtmlWnd<LuaHtmlView> 
{
    HWND m_hWnd;
    ~LuaHtmlView()
    {
        //printf("HtmlView[%X] Destroy\n", m_hWnd);
    }
    //////////////////////////////////////////////////////////////////////////
    //htmlayout::notification_handle implement
    virtual LRESULT on_dialog_created(NMHDR*) { return 0;  }
    virtual LRESULT on_dialog_close( LPNMHL_DIALOG_CLOSE_RQ pnmld) { return 0; }
    virtual LRESULT on_attach_behavior( LPNMHL_ATTACH_BEHAVIOR lpab ) { htmlayout::behavior::handle(lpab); return 0; }
    virtual LRESULT on_document_complete(){
        LuaCallGruad ctx("OnDocumentComplete");
        assert(ctx);
        HELEMENT he = 0;
        HTMLayoutGetRootElement(m_hWnd, &he);
        lua_pushlightuserdata(ctx.L, he);
        lua_pushlightuserdata(ctx.L, m_hWnd);
        lua_pushstring(ctx.L, m_url.c_str());
        if (LUA_OK != lua_pcall(ctx.L, 3, 0, ctx.errorRef))
            lua_pop(ctx.L,1);
        return 0;
    }
    virtual LRESULT on_document_loaded() {
        LuaCallGruad ctx("OnDocumentLoaded");
        assert(ctx);
        HELEMENT he = 0;

        HTMLayoutGetRootElement(m_hWnd, &he);
        HTMLayoutSetObject(he, this, L"HtmlView");//根元素绑定窗口控件

        lua_pushlightuserdata(ctx.L, he);
        lua_pushlightuserdata(ctx.L, m_hWnd);
        lua_pushstring(ctx.L, m_url.c_str());
        if (LUA_OK != lua_pcall(ctx.L, 3, 0, ctx.errorRef))
            lua_pop(ctx.L,1);
        return 0;
    }
    virtual LRESULT on_create_control(LPNMHL_CREATE_CONTROL nm) {
        LuaCallGruad ctx("OnCreateControl");
        assert(ctx);
        lua_pushlightuserdata(ctx.L, nm->helement);
        lua_pushlightuserdata(ctx.L, nm->inHwndParent);
        if (lua_pcall(ctx.L, 2, 1, ctx.errorRef) == LUA_OK)
            nm->outControlHwnd = (HWND)lua_touserdata(ctx.L, -1);
        lua_pop(ctx.L,1);
        return 0;
    }
    virtual LRESULT on_control_created(LPNMHL_CREATE_CONTROL nm) {
        LuaCallGruad ctx("OnControlCreated");
        assert(ctx);
        lua_pushlightuserdata(ctx.L, nm->helement);
        lua_pushlightuserdata(ctx.L, nm->inHwndParent);
        lua_pushlightuserdata(ctx.L, nm->outControlHwnd);
        if (LUA_OK != lua_pcall(ctx.L, 3, 0, ctx.errorRef))
            lua_pop(ctx.L, 1);
        return 0;
    }
    virtual LRESULT on_destroy_control(LPNMHL_DESTROY_CONTROL nm) { 
        LuaCallGruad ctx("OnDestroyControl");
        assert(ctx);
        lua_pushlightuserdata(ctx.L, nm->helement);
        lua_pushlightuserdata(ctx.L, nm->inoutControlHwnd);
        if (LUA_OK == lua_pcall(ctx.L, 2, 1, ctx.errorRef))
            nm->inoutControlHwnd = luaH_checkhwnd(ctx.L, -1);
        lua_pop(ctx.L, 1);
        return 0;
    }
    virtual LRESULT on_load_data(LPNMHL_LOAD_DATA pnmld) {
        int ret = LOAD_OK;
        PBYTE pb; DWORD cb;
        if(htmlayout::load_resource_data(pnmld->uri, pb, cb)){
            ::HTMLayoutDataReady(pnmld->hdr.hwndFrom, pnmld->uri, pb,  cb);
            return ret;
        }
        LuaCallGruad ctx("OnLoadData");
        assert(ctx);
        lua_pushinteger(ctx.L, pnmld->dataType);
        lua_pushstring(ctx.L, aux::w2a(pnmld->uri));
        lua_pushlightuserdata(ctx.L, pnmld->principal);
        if (LUA_OK == lua_pcall(ctx.L, 3, 1, ctx.errorRef))
            ret = lua_toboolean(ctx.L, -1) ? LOAD_OK:LOAD_DISCARD;
        lua_pop(ctx.L, 1);
        return ret;
    }
    virtual LRESULT on_data_loaded(LPNMHL_DATA_LOADED pnmld) {
        LuaCallGruad ctx("OnDataLoaded");
        assert(ctx);
        lua_pushinteger(ctx.L, pnmld->dataType);
        lua_pushstring(ctx.L, aux::w2a(pnmld->uri));
        lua_pushinteger(ctx.L, pnmld->status);
        lua_pushlstring(ctx.L, (const char*)pnmld->data, pnmld->dataSize);
        if (LUA_OK != lua_pcall(ctx.L, 4, 0, ctx.errorRef))
            lua_pop(ctx.L, 1);
        return 0;
    }
    //////////////////////////////////////////////////////////////////////////
    virtual BOOL handle_event (HELEMENT he, BEHAVIOR_EVENT_PARAMS& params)
    {
        if (!he) HTMLayoutGetRootElement(m_hWnd, &he);
        return LuaBehavior::HandleBehaviorEvent(LuaCallGruad(), he, params); 
    }
    virtual BOOL handle_script_call (HELEMENT he, XCALL_PARAMS& params) 
    {
        if (!he) HTMLayoutGetRootElement(m_hWnd, &he);
        return LuaBehavior::HandleScriptCall(LuaCallGruad(), he, params);
    }
    //////////////////////////////////////////////////////////////////////////
    //window event
    LRESULT OnClose(HWND hWnd) {
        LuaCallGruad ctx("OnWindowClose");
        lua_pushlightuserdata(ctx.L, hWnd);
        const bool destroy = LUA_OK == lua_pcall(ctx.L, 1, 1, ctx.errorRef) && lua_toboolean(ctx.L, -1);
        lua_pop(ctx.L,1);
        if (destroy) DestroyWindow(hWnd);
        return 0L;
    }
    LRESULT OnDestroy(HWND hWnd) {
        LuaCallGruad ctx("OnWindowDestroy");
        lua_pushlightuserdata(ctx.L, hWnd);
        if (LUA_OK == lua_pcall(ctx.L, 1, 1, ctx.errorRef) && lua_toboolean(ctx.L, -1))
            PostQuitMessage(0);
        lua_pop(ctx.L,1);
        return 0L;
    }
    //////////////////////////////////////////////////////////////////////////
    int Init(lua_State* L)
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
        hwnd = CreateWnd(hwnd, embed);
        if (!hwnd) {
            delete this;
            return 0;
        }
        m_hWnd = hwnd;
        lua_pushlightuserdata(L,hwnd);
        lua_pushlightuserdata(L,this);
        return 2;
    }
    //设置窗口标题
    int SetElementInnerText(lua_State*L)
    {
        const BOOL ret = ::IsWindow(m_hWnd) && ::SetWindowTextW(m_hWnd, aux::a2w(luaL_checkstring(L, 1)));
        lua_pushinteger(L, ret ? HLDOM_OK : HLDOM_INVALID_HWND);
        return 1;
    }
    int ControlSetValue(lua_State* L)
    {
        const BOOL ret = ::IsWindow(m_hWnd) && LoadUrl(m_hWnd, luaL_checkstring(L,1));
        lua_pushinteger(L, ret ? HLDOM_OK : HLDOM_INVALID_HWND);
        return 1;
    }
    int GetElementHwnd(lua_State* L)
    {
        lua_pushinteger(L, ::IsWindow(m_hWnd) ? HLDOM_OK: HLDOM_INVALID_HWND);
        lua_pushlightuserdata(L, m_hWnd);
        return 2;
    }
    //设置窗口图标
    int SetElementInnerImage(lua_State* L)
    {
        if (!::IsWindow(m_hWnd))
            return 0;
        const char* fpath = luaL_checkstring(L,1);
        if (strstr(fpath,"file://") == fpath)
            fpath += 7;
        else
            return 0;

        HICON hIcon = (HICON)::LoadImageW(0, aux::a2w(fpath),
            IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
        if(hIcon == NULL)
            return 0;

        ::SendMessageW(m_hWnd, WM_SETICON, luaL_optinteger(L, 2, 0), (LPARAM)hIcon);
        lua_pushinteger(L, HLDOM_OK);
        return 1;
    }
};
int luaopen_view(lua_State* L)
{
    static LuaClass<LuaHtmlView>::RegType methods[]={
#define _(x)  {#x, &LuaHtmlView::x}
        _(ControlSetValue),
        _(GetElementHwnd),
        _(SetElementInnerText),
        _(SetElementInnerImage),
#undef _
        {0,0}
    };
    LuaClass<LuaHtmlView>::ClassTable(L, methods);
    return 1;
}
