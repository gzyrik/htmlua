#pragma once
#include "htmlayout.h"
#include "htmlayout_dom.hpp"
#include <lua.hpp>
#ifndef LUA_OK
#define LUA_OK 0
#endif
#include <olebox.h>
bool script_call_olebox(OleBox* obox, XCALL_PARAMS& params);
//实用函数
#define luaH_checkelement(L, idx) (HELEMENT)luaH_checkptr(L, idx, "__element")
#define luaH_checkhwnd(L, idx) (HWND)luaH_checkptr(L, idx, "__hwnd",false,true)
#define luaH_checkobject(L, idx) luaH_checkptr(L, idx, "__widget")

#define luaH_optelement(L, idx) (HELEMENT)luaH_checkptr(L, idx, "__element", true)
#define luaH_opthwnd(L, idx) (HWND)luaH_checkptr(L, idx, "__hwnd", true, true)
#define luaH_optobject(L, idx) (HWND)luaH_checkptr(L, idx, "__widget", true)
void luaH_setfuncs(lua_State *L, const luaL_Reg *l, int nup);
#define LUA_NOTIFY_IDFROM 3557
struct LUA_NMHDR : NMHDR /**< Default WM_NOTIFY header */
{
    LUA_NMHDR(HWND hwndFrom, UINT code, LPCSTR func): name(func) {
        this->hwndFrom = hwndFrom;
        this->idFrom = LUA_NOTIFY_IDFROM;
        this->code = code;
    }
    LPCSTR  name;
    std::vector<json::value> argv;
    json::value retval;
};
inline void* luaH_checkptr(lua_State* L, int idx, const char* field, bool opt=false, bool metable=false)
{
    void* p = 0;
    int type = lua_type(L, idx);
    if (type == LUA_TLIGHTUSERDATA)
        p = lua_touserdata(L, idx);
    else if (type == LUA_TTABLE) {
        if (metable)
            lua_getfield(L, idx, field);
        else {
            lua_pushstring(L, field);
            lua_rawget(L, idx);
        }
        if (lua_type(L, -1) == LUA_TLIGHTUSERDATA)
            p = lua_touserdata(L, -1);
        lua_pop(L,1);
    }
    if (!p && !opt)
        luaL_error(L, "invalid %s at %d", field, idx);
    return p;
}
inline json::value* luaH_toargv(lua_State* L, json::value* argv, int begin=1, int last = -1)
{
    const int size = lua_gettop(L) + 1;
    if (begin < 0) begin += size;
    if (last < 0) last += size;
    for(int i=begin;i<=last;++i) 
    {
        const int type = lua_type(L, i);
        json::value& value = argv[i-begin];
        switch(type)
        {
        default:
        case LUA_TLIGHTUSERDATA:
        case LUA_TTABLE:
        case LUA_TFUNCTION:
        case LUA_TUSERDATA:
        case LUA_TTHREAD:
        case LUA_TNONE:
        case LUA_TNIL:
            value.clear();
            break;
        case LUA_TBOOLEAN:
            value = lua_toboolean(L, i) !=0;
            break;
        case LUA_TNUMBER:
            value = lua_tonumber(L, i);
            break;
        case LUA_TSTRING:
            value = (const wchar_t*)aux::a2w(lua_tostring(L, i));
            break;
        }
    }
    return argv;
}
inline int luaH_pushargv(lua_State*L, int argc, json::value* argv)
{
    for(int i=0;i<argc;++i){
        if(argv[i].is_bool())
            lua_pushboolean(L,argv[i].get(false));
        else if (argv[i].is_int())
            lua_pushinteger(L, argv[i].get(0));
        else if (argv[i].is_float())
            lua_pushnumber(L, argv[i].get(0.0));
        else if (argv[i].is_string())
            lua_pushstring(L, aux::w2a(argv[i].get_chars()));
        else
            lua_pushnil(L);
    }
    return argc;
}
struct LuaXCallParam : public XCALL_PARAMS
{
    std::vector<json::value> vec;
    LuaXCallParam(lua_State*L, const int start=1):
        XCALL_PARAMS(luaL_checkstring(L,start)),vec(lua_gettop(L)-start)
    {
        if (this->vec.size() > 0){
            this->argv = luaH_toargv(L, &this->vec[0], start+1);
            this->argc = this->vec.size();
        }
        lua_pop(L, 1+(int)this->vec.size());
    }
};
inline HLDOM_RESULT HTMLayoutSetObject(HELEMENT he, void* object, LPCWSTR type)
{
    wchar_t buffer[38];
    _ultow_s((size_t)object, buffer, 16);
    HLDOM_RESULT ret = HTMLayoutSetAttributeByName(he, "-customwidget", buffer);
    ret |= HTMLayoutSetAttributeByName(he, "type", type);
    return ret;
}
inline HLDOM_RESULT HTMLayoutGetObject(HELEMENT he, LPVOID* object, LPCWSTR* type)
{
    LPCWSTR value;
    HLDOM_RESULT ret = HTMLayoutGetAttributeByName(he, "-customwidget", &value);
    *object = (ret == HLDOM_OK && value != 0) ? (void*)(size_t)wcstoul(value, 0, 16) : 0;
    ret |= HTMLayoutGetAttributeByName(he, "type", type);
    return ret;
}
//保持lua运行环境堆栈的个数，防止
//数据无序增长
struct LuaGruad
{
    lua_State* L;
    const int top;
    LuaGruad(lua_State* l=0) :L(l ? l : luaState()),top(lua_gettop(L)){}
    ~LuaGruad()
    {
        assert(lua_gettop(L) == top);
        lua_settop(L, top);
    }
    //current thread lua_State
    static lua_State* luaState();
};
struct LuaCallGruad : public LuaGruad
{
    int tableRef;
    mutable int errorRef;
    mutable const char* fname;
    LuaCallGruad(lua_State* l=0, int table=0):LuaGruad(l),tableRef(table),errorRef(0), fname(0){}
    LuaCallGruad(const char* cb, lua_State* l=0, int table=0):LuaGruad(l),tableRef(table),errorRef(0),fname(0){ setup(cb); }
    operator bool() const { return lua_isfunction(L, errorRef+1); }
    ~LuaCallGruad() {
        if (errorRef) lua_pop(L, 1);//remove errorRef
        const int t = lua_gettop(L);
        if (t != top)
            luaL_error(L,"%s stack error: from %d to %d ", fname, top, t);
    }
    void setup(const char* cb) const
    {
        fname = cb;
        lua_settop(L, top);
        lua_rawgeti(L, LUA_REGISTRYINDEX, tableRef ? tableRef : libRef());
        if (!lua_istable(L, -1))
            luaL_error(L, "uninitialized");
        errorRef = lua_gettop(L);
        lua_getfield(L, -1, cb);
        if (!lua_isfunction(L, -1))
            luaL_error(L, "invalid %s function", cb);
        lua_getfield(L, -2, "OnError");
        if (!lua_isfunction(L, -1))
            luaL_error(L, "not find OnError");
        lua_replace(L, errorRef);
    }
    //返回值位置
    int retidx() const {
        int n = lua_gettop(L);
        return errorRef ? errorRef - n : top - n;
    }
    //current thread lib ref
    static int libRef();
};
//=============================================================================
class HTMLuaWnd
{
    static HWND HTMLuaCreateWnd(HWND parent, bool embed, HTMLuaWnd* pThis);
protected:
    HWND CreateWnd(HWND parent, bool embed){ return HTMLuaCreateWnd(parent, embed, this); }
    virtual int Init(lua_State* L) = 0;
protected:
    int  m_refCount;
public:
    virtual LRESULT OnWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    { return ::DefWindowProc(hWnd,uMsg,wParam,lParam);}
    virtual BOOL OnNotify(NMHDR* nm, HELEMENT he);
    virtual void OnDropFiles(HWND hWnd, HDROP hDrop);
    HTMLuaWnd():m_refCount(0) {}
    virtual ~HTMLuaWnd(){}
    int CreateElement(lua_State* L) { return Init(L); }
    int UseElement() { return m_refCount++;}
    int UnuseElement() {
        int ref = m_refCount;
        if (--m_refCount < 0) delete this;
        return ref;
    }
};
//类方法封装
template <typename T> struct LuaClass
{
    typedef int (T::*mfp)(lua_State *L);
    typedef struct { const char *name; mfp func; } RegType;
    static int UseElement(lua_State *L) {
        T*obj = (T*)(luaH_checkobject(L, 1));
        if (!obj) return 0;
        lua_pushinteger(L, obj->UseElement());
        return 1;
    }
    static int UnuseElement(lua_State *L) {
        T*obj = (T*)(luaH_checkobject(L, 1));
        if (!obj) return 0;
        lua_pushinteger(L, obj->UnuseElement());
        return 1;
    }
    static int CreateElement(lua_State *L) {
        T* p = new T();
        return p->CreateElement(L);
    }
    static int dispatch(lua_State *L) {
        T *obj = (T*)luaH_checkobject(L, 1);
        if (!obj) luaL_error(L,"null");
        lua_remove(L, 1);
        RegType* l = (RegType*)lua_touserdata(L, lua_upvalueindex(1));
        return (obj->*(l->func))(L);
    }
    static int delete_T(lua_State *L) {
        T*obj = (T*)(luaH_checkobject(L, 1));
        if (obj) delete obj;
        return 0;
    }

    static void ClassTable(lua_State* L, RegType methods[])
    {
        lua_newtable(L);
        for (RegType *l = methods; l->name; l++) {
            lua_pushlightuserdata(L, l);
            lua_pushcclosure(L, dispatch, 1);
            lua_setfield(L, -2, l->name);
        }
        lua_pushcfunction(L, CreateElement);
        lua_setfield(L, -2, "CreateElement");
        lua_pushcfunction(L, UseElement);
        lua_setfield(L, -2, "UseElement");
        lua_pushcfunction(L, UnuseElement);
        lua_setfield(L, -2, "UnuseElement");
    }
};
