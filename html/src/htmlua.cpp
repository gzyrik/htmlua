#include "htmlua.h"
#include "behavior_lua.h"
#include <map>
#include <set>
#include <process.h>
#pragma comment(lib, "comsupp.lib")
namespace{
struct ThreadParam
{
    HANDLE hEvent;
    const char* source;
    const char* name;
    const char* msg;
    const char* code;
};
struct THREADNAME_INFO 
{
    DWORD dwType;     // must be 0x1000
    LPCSTR szName;    // pointer to name (in user addr space)
    DWORD dwThreadID; // thread ID (-1 = caller thread)
    DWORD dwFlags;    // reserved for future use, must be zero
};
void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = szThreadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;

    __try
    {
        RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD),
            (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
}
struct LuaTask : public htmlayout::gui_task
{
    const std::string from;
    const std::string code;
    HANDLE hEvent;
    LuaTask( const std::string& f, const std::string& c, HANDLE h=0):from(f),code(c),hEvent(h){}
    virtual void exec()
    {
        LuaCallGruad ctx("OnThreadTask");
        lua_pushstring(ctx.L, code.c_str());
        lua_pushstring(ctx.L, from.c_str());
        if (LUA_OK != lua_pcall(ctx.L, 2, 0, ctx.errorRef))
            lua_pop(ctx.L, 1);
    }
    virtual void release() {
        if (hEvent)
            ::SetEvent(hEvent);
        else
            delete this;
    }
};
typedef std::map<HELEMENT, std::set<UINT> > ElemCodesMap;//排除集合
typedef std::map<HWND, ElemCodesMap> NotifyMap;//窗口->排除集合
typedef std::map<std::string, DWORD> ThreadMap;//名字->线程ID
}
static NotifyMap _notify;
static int HandleNofity(NMHDR* pNotify)
{
    int count=0;
    NotifyMap::iterator hwnd_iter = _notify.find(pNotify->hwndFrom);
    if (hwnd_iter != _notify.end()) {
        ElemCodesMap& elems = hwnd_iter->second;
        for(ElemCodesMap::iterator iter = elems.begin(); iter != elems.end();++iter) {
            std::set<UINT>& codes = iter->second;
            if (codes.find(pNotify->code) == codes.end()){
                HTMLuaWnd* pFromWnd = (HTMLuaWnd*)GetWindowLongPtr(pNotify->hwndFrom,GWL_USERDATA);
                if (!pFromWnd || !pFromWnd->OnNotify(pNotify, iter->first))
                    codes.insert(pNotify->code);//没有处理,后续排除
                else
                    ++count;
            }
        }
    }
    return count;
}
static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HTMLuaWnd* pThis;
    if(uMsg == WM_CREATE) {
        pThis = (HTMLuaWnd*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hWnd,GWL_USERDATA,(LONG_PTR)pThis);
    }
    else {
        pThis = (HTMLuaWnd*)GetWindowLongPtr(hWnd,GWL_USERDATA);
        if(!pThis) return ::DefWindowProc(hWnd,uMsg,wParam,lParam);
    }
    if (uMsg == WM_NOTIFY) {
        NMHDR* pNotify = (NMHDR *)(lParam);
        if(pNotify->idFrom == LUA_NOTIFY_IDFROM)
            HandleNofity((NMHDR*)lParam);
    }
    else if (uMsg == WM_DROPFILES){
        pThis->OnDropFiles(hWnd,(HDROP)wParam);
    }
    else if (uMsg == WM_DESTROY) {
        SetWindowLongPtr(hWnd, GWL_USERDATA, 0);
        NotifyMap::const_iterator hwnd_iter = _notify.find(hWnd);
        if (hwnd_iter != _notify.end()){
            const ElemCodesMap& elems = hwnd_iter->second;
            for(ElemCodesMap::const_iterator iter = elems.begin(); iter != elems.end();++iter)
                HTMLayout_UnuseElement(iter->first);
            _notify.erase(hwnd_iter);
        }
    }
    LRESULT ret = pThis->OnWindowProc(hWnd,uMsg,wParam,lParam);
    if (uMsg == WM_DESTROY) pThis->UnuseElement();
    return ret;
}
BOOL HTMLuaWnd::OnNotify(NMHDR* nm, HELEMENT he)
{
    BOOL bHandled = FALSE;
    LUA_NMHDR* lnm = static_cast<LUA_NMHDR*>(nm);
    LuaCallGruad ctx("OnNotify");
    lua_pushlightuserdata(ctx.L, he);
    lua_pushstring(ctx.L, lnm->name);
    if (lnm->argv.size() > 0) luaH_pushargv(ctx.L, lnm->argv.size(), &lnm->argv[0]);
    if (LUA_OK != lua_pcall(ctx.L,  lnm->argv.size()+2, LUA_MULTRET, ctx.errorRef))
        lua_pop(ctx.L, 1);
    else {
        const int idx = ctx.retidx();
        if (idx < 0){
            luaH_toargv(ctx.L, &lnm->retval,idx, idx);
            lua_pop(ctx.L, -idx);
            bHandled = TRUE;
        }
    }
    return bHandled;
}
void HTMLuaWnd::OnDropFiles(HWND hWnd, HDROP hDrop)
{
    HELEMENT he = 0;
    POINT pt;
    ::DragQueryPoint(hDrop, &pt);
    if (HTMLayoutFindElement(hWnd, pt, &he) == HLDOM_OK && he != 0)
    {
        wchar_t fname[MAX_PATH];
        const UINT iFileCount = ::DragQueryFile(hDrop, -1, 0, 0);
        if (iFileCount > 0) {
            LuaCallGruad ctx("OnDropFiles");
            lua_pushlightuserdata(ctx.L, he);
            for(UINT i=0;i<iFileCount;++i){
                ::DragQueryFileW(hDrop, i,fname, MAX_PATH);
                lua_pushstring(ctx.L,aux::w2a(fname));
            }
            if (LUA_OK == lua_pcall(ctx.L, iFileCount+1, 1, ctx.errorRef)){
                if (lua_toboolean(ctx.L, -1))
                    ::DragFinish(hDrop);
            }
            lua_pop(ctx.L, 1);
        }
    }
}
enum {
    HADNLE_LUA_NOTIFY= 0x1000000,
    HADNLE_LUA_INIT  = 0x2000000,
    HADNLE_LUA_DROPFILES  = 0x4000000,
    HANDLE_LUA_MASK  = 0xF000000,
};
//int,string(HELEMENT,mask)
static int BindBehavior(lua_State* L)
{
    static LuaBehavior _luaBehavior;
    LPCWSTR value = 0;
    HLDOM_RESULT r = HLDOM_OK;
    HELEMENT he = luaH_checkelement(L, 1);
    const UINT mask = (UINT)luaL_checkinteger(L, 2);
    r = HTMLayoutGetAttributeByName(he, "--subscription--", &value);
    const UINT subscription = (r == HLDOM_OK && value != 0) ? wcstoul(value, 0, 16) : 0;
    const UINT changed = (mask ^ subscription) & (UINT)luaL_optinteger(L, 3, -1);
    if (changed & HADNLE_LUA_DROPFILES) {
        HWND hwndFrom = luaH_checkhwnd(L, 1);
        ::DragAcceptFiles(hwndFrom, mask & HADNLE_LUA_DROPFILES);
    }
    if (changed & HADNLE_LUA_NOTIFY){
        if (mask & HADNLE_LUA_NOTIFY) {
            HWND hwndFrom = luaH_checkhwnd(L, 1);
            if (_notify[hwndFrom].insert(ElemCodesMap::value_type(he, std::set<UINT>())).second)
                r = HTMLayout_UseElement(he);
        }
        else {
            NotifyMap::iterator iter = _notify.begin();
            while (iter != _notify.end()) {
                NotifyMap::iterator cur = iter++;
                if (cur->second.erase(he) > 0){
                    HTMLayout_UnuseElement(he);
                    if (!cur->second.size())
                        _notify.erase(cur);
                }
            }
        }
    }
    if (changed & HANDLE_ALL) {
        UINT newsubscription = mask & HANDLE_ALL;
        if (subscription & HANDLE_ALL){
            htmlayout::detach_event_handler(he, &_luaBehavior);
            newsubscription |= DISABLE_INITIALIZATION;//attach again, above attached event
        }
        if (newsubscription & HANDLE_ALL) {
            r = htmlayout::attach_event_handler(he, &_luaBehavior, newsubscription);
        }
    }
    else if (changed & HADNLE_LUA_INIT) {
        if (mask & HADNLE_LUA_INIT)
            LuaBehavior::HandleAttached(LuaCallGruad(L), he);
        else
            LuaBehavior::HandleDetached(LuaCallGruad(L), he);
    }
    if (changed) {
        wchar_t buffer[38];
        _ultow_s((size_t)mask, buffer, 16);
        r = HTMLayoutSetAttributeByName(he, "--subscription--", buffer);
    }
    lua_pushinteger(L, r);
    return 1;
}
static int UILoop(lua_State* L)
{
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)!= 0) { 
        htmlayout::queue::execute();
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
    }
    return 0;
}
static int ANSI(lua_State* L)
{
    lua_pushstring(L, aux::utf2a(luaL_checkstring(L,1)));
    return 1;
}
static ThreadMap _threads;
static unsigned __stdcall threadfunc(void * p)
{
    htmlayout::queue queue;
    std::string name;
    lua_State* L;
    {
        std::string code, source, msg;
        {
            ThreadParam* param = (ThreadParam*)p;
            code = param->code;
            source = param->source;
            name = param->name;
            msg = param->msg;
            L = luaL_newstate();
            ::SetEvent(param->hEvent);
        }
        lua_pushstring(L, name.c_str());
        lua_setglobal(L,"__thread");
        luaL_openlibs(L);  /* open standard libraries */
        SetThreadName(-1,name.c_str());
        if (luaL_loadbuffer(L, code.c_str(), code.size(), source.c_str()) != LUA_OK){
            const char* err = lua_tostring(L,-1);
            printf("ERRPR %s\n", err);
            return 0;
        }
        lua_pushstring(L, msg.c_str());
    }
    if(LUA_OK != lua_pcall(L, 1, 0, 0))
    {
        const char* err = lua_tostring(L,-1);
        printf("ERROR %s\n", err);
    }
    _threads.erase(name);
    lua_close(L);
    return 0;
}
//int(func,name)
static int NewThread(lua_State* L)
{
    const char* name = luaL_checkstring(L,1);
    const char* code = luaL_checkstring(L,2);
    if (!name || !name[0] || _threads.find(name) != _threads.end())
        luaL_error(L,"null or duplicate thread name");

    lua_Debug ar;
    {
        if (!lua_getstack(L, 1, &ar))
            luaL_error(L,"lua_getstack failed");
        if (!lua_getinfo(L,"S", &ar))
            luaL_error(L,"lua_getinfo failed");
    }

    ThreadParam param;
    param.code = code;
    param.source = ar.source;
    param.name = name;
    param.msg = luaL_optstring(L, 3,"");
    param.hEvent = ::CreateEventW(NULL,FALSE,FALSE,NULL);
    unsigned int threadId;
    HANDLE hThread = (HANDLE)::_beginthreadex( NULL, 0, &threadfunc, (LPVOID)&param, 0, &threadId);
    _threads[name] = threadId;
    ::WaitForSingleObject(param.hEvent, INFINITE);
    ::CloseHandle(param.hEvent);
    ::CloseHandle(hThread);
    return 0;
}
static int PostTask(lua_State* L)
{
    const char* to = luaL_checkstring(L,1);
    ThreadMap::iterator iter = _threads.find(to);
    if (iter == _threads.end())
        return 0;
    const char* from = luaL_checkstring(L,2);
    bool sync = lua_toboolean(L,3) != 0;
    const char* code = luaL_checkstring(L,4);
    if (sync) {
        HANDLE hEvent = ::CreateEventW(NULL,FALSE,FALSE,NULL);
        LuaTask t(from, code, hEvent);
        htmlayout::queue::push(&t, iter->second);
        ::WaitForSingleObject(hEvent, INFINITE);
    }
    else
        htmlayout::queue::push(new LuaTask(from, code), iter->second);
    return 0;
}
static ATOM RegisterWndClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(wcex); 

    wcex.style        = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc  = ::WindowProc;
    wcex.cbClsExtra   = 0;
    wcex.cbWndExtra   = 0;
    wcex.hInstance    = hInstance;
    wcex.hIcon      = LoadIcon(hInstance, (LPCTSTR)IDI_APPLICATION);
    wcex.hCursor    = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)COLOR_WINDOW;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName  = L"HtmlView";
    wcex.hIconSm    = LoadIcon(hInstance, (LPCTSTR)IDI_APPLICATION);
    return RegisterClassExW(&wcex);
}
HWND HTMLuaWnd::HTMLuaCreateWnd(HWND parent, bool embed, HTMLuaWnd* pThis)
{
    static ATOM _classAtom = 0;
    if (!_classAtom)
        _classAtom= RegisterWndClass(0);
    HWND hWnd = ::CreateWindowExW(0, L"HtmlView", L"",
        (parent && embed)? WS_CHILDWINDOW : WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, parent, NULL, 0, pThis);
    ShowWindow(hWnd, SW_SHOW);
    return hWnd;
}
int luaopen_dom(lua_State* L);
int luaopen_view(lua_State* L);
int luaopen_scilua(lua_State* L);
int luaopen_flash(lua_State* L);
int luaopen_web(lua_State* L);
int luaopen_json_decode(lua_State* L);
bool script_call_olebox(OleBox* obox, XCALL_PARAMS& params){
    if (!obox) return false;
    return true;
}
static DWORD _tlsLibRef = TlsAlloc();
static DWORD _tlsLuaState = TlsAlloc();
int LuaCallGruad::libRef() { return (int)TlsGetValue(_tlsLibRef);}
lua_State* LuaGruad::luaState() { return (lua_State*)TlsGetValue(_tlsLuaState);}
static wchar_t _path[MAX_PATH];
BOOL WINAPI
DllMain                 (HINSTANCE      hinstDLL,
                         DWORD          fdwReason,
                         LPVOID         lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        ::GetModuleFileNameW((HMODULE)hinstDLL, _path, MAX_PATH);
        _threads["main"] = ::GetCurrentThreadId();
        break;
    case DLL_PROCESS_DETACH:
        break;
    default:
        ;/* do nothing */
    }
    return TRUE;
}
void luaH_setfuncs(lua_State *L, const luaL_Reg *l, int nup)
{
    luaL_checkstack(L, nup, "too many upvalues");
    for (; l->name; l++) {
        int i;
        for (i = 0; i < nup; i++)  /* Copy upvalues to the top. */
            lua_pushvalue(L, -nup);
        lua_pushcclosure(L, l->func, nup);
        lua_setfield(L, -(nup + 2), l->name);
    }
    lua_pop(L, nup);  /* Remove upvalues. */
}
extern "C" __declspec(dllexport) int
luaopen_htm_htmlua(lua_State* L)
{
    OleInitialize(NULL); // required for D&D operations
#if LUA_VERSION_NUM < 503
    lua_pushstring(L, aux::w2a(_path));
#endif
/*
    std::wstring fpath(aux::a2w(luaL_checkstring(L,2)));
    std::wstring::size_type pos = fpath.find_last_of(L"/\\");

    std::wstring dll = fpath.substr(0, pos+1) + L"htmlayout.dll";
    if (!LoadLibraryW(dll.c_str()))
        luaL_error(L, "invalid %s", dll.c_str());

    dll = fpath.substr(0, pos+1) + L"olebox.dll";
    if (!LoadLibraryW(dll.c_str()))
        luaL_error(L, "invalid %s", dll.c_str());
*/
    static htmlayout::debug_output_console _console;
    lua_newtable(L);
    lua_pushvalue(L, -1);
    int libRef = luaL_ref(L, LUA_REGISTRYINDEX);
    if(!TlsSetValue(_tlsLibRef, (LPVOID)libRef) || !TlsSetValue(_tlsLuaState, (LPVOID)L))
        luaL_error(L, "%s", "TlsSetValue error");

    luaopen_dom(L);
    if (luaopen_json_decode(L) == 1)
        lua_setfield(L, -2, "JsonTable");
    if (luaopen_view(L) == 1)
        lua_setfield(L, -2, "HtmlView");
    if (luaopen_scilua(L) == 1)
        lua_setfield(L, -2, "SciLexer");
    if (luaopen_flash(L) == 1)
        lua_setfield(L, -2, "Flash");
    if (luaopen_web(L) == 1)
        lua_setfield(L, -2, "Web");
    luaL_Reg libs[]={
#define _(x)  {#x, x}
        _(BindBehavior),
        _(UILoop),
        _(NewThread),
        _(ANSI),
        _(PostTask),
#undef _
        {0,0}
    };
    luaH_setfuncs(L, libs, 0);
    static htmlayout::queue queue;
    return 1;
}
