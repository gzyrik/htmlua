#include "htmlua.h"
#include "behavior_lua.h"
BOOL LuaBehavior::HandleBehaviorEvent (const LuaCallGruad& ctx, HELEMENT he, BEHAVIOR_EVENT_PARAMS& params)
{
    BOOL bHandled = FALSE;
    ctx.setup("HandleBehaviorEvent");
    lua_pushlightuserdata(ctx.L, he);
    if(he == params.heTarget)
        lua_pushvalue(ctx.L, -1);
    else
        lua_pushlightuserdata(ctx.L, params.heTarget);

    lua_pushinteger(ctx.L, params.cmd);
    lua_pushinteger(ctx.L, params.reason);
    luaH_pushargv(ctx.L, 1, &params.data);
    if(LUA_OK == lua_pcall(ctx.L, 5, 1, ctx.errorRef))
        bHandled = lua_toboolean(ctx.L, -1);
    lua_pop(ctx.L,1);
    return bHandled;
}
BOOL LuaBehavior::HandleScriptCall (const LuaCallGruad& ctx, HELEMENT he, XCALL_PARAMS& params ) 
{
    BOOL bHandled = FALSE;
    ctx.setup("HandleScriptCall");
    lua_pushlightuserdata(ctx.L, he);
    lua_pushstring(ctx.L, params.method_name);
    luaH_pushargv(ctx.L, params.argc, params.argv);
    if (LUA_OK != lua_pcall(ctx.L, params.argc+2, LUA_MULTRET, ctx.errorRef))
        lua_pop(ctx.L, 1);
    else {
        const int idx = ctx.retidx();
        if (idx < 0){
            luaH_toargv(ctx.L, &params.retval,idx, idx);
            lua_pop(ctx.L, -idx);
            bHandled = TRUE;
        }
    }
    return bHandled;
}
BOOL LuaBehavior::HandleMethodCall (const LuaCallGruad& ctx, HELEMENT he, METHOD_PARAMS& params)
{
    BOOL bHandled = FALSE;
    ctx.setup("HandleMethodCall");
    lua_pushlightuserdata(ctx.L, he);
    lua_pushinteger(ctx.L, params.methodID);
    lua_pushlightuserdata(ctx.L, &params);
    if (LUA_OK == lua_pcall(ctx.L, 3, 1, ctx.errorRef))
        bHandled = lua_toboolean(ctx.L, -1);
    lua_pop(ctx.L,1);
    return bHandled;
}
BOOL LuaBehavior::HandleExchange (const LuaCallGruad& ctx, HELEMENT he, EXCHANGE_PARAMS& params)
{
    BOOL bHandled = FALSE;
    ctx.setup("HandleExchange");
    lua_pushlightuserdata(ctx.L, he);
    if(he == params.target)
        lua_pushvalue(ctx.L, -1);
    else
        lua_pushlightuserdata(ctx.L, params.target);
    lua_pushinteger(ctx.L, params.cmd);
    lua_pushinteger(ctx.L, params.pos.x);
    lua_pushinteger(ctx.L, params.pos.y);
    lua_pushinteger(ctx.L, params.pos_view.x);
    lua_pushinteger(ctx.L, params.pos_view.y);
    lua_pushinteger(ctx.L, params.data_types);
    lua_pushinteger(ctx.L, params.drag_cmd);
    if (LUA_OK == lua_pcall(ctx.L, 9, 1, ctx.errorRef))
        bHandled = lua_toboolean(ctx.L, -1);
    lua_pop(ctx.L,1);
    return bHandled;
}
BOOL LuaBehavior::HandleDataArrived (const LuaCallGruad& ctx, HELEMENT he, DATA_ARRIVED_PARAMS& params)
{
    BOOL bHandled = FALSE;
    ctx.setup("HandleDataArrived");
    lua_pushlightuserdata(ctx.L, he);
    if(he == params.initiator)
        lua_pushvalue(ctx.L, -1);
    else
        lua_pushlightuserdata(ctx.L, params.initiator);
    lua_pushstring(ctx.L, aux::w2a(params.uri));
    lua_pushinteger(ctx.L, params.dataType);
    lua_pushinteger(ctx.L, params.status);
    lua_pushinteger(ctx.L, params.dataSize);
    lua_pushlightuserdata(ctx.L, (void*)params.data);
    if (LUA_OK == lua_pcall(ctx.L, 7, 1, ctx.errorRef))
        bHandled = lua_toboolean(ctx.L, -1);
    lua_pop(ctx.L,1);
    return bHandled;
}
BOOL LuaBehavior::HandleDraw (const LuaCallGruad& ctx, HELEMENT he, DRAW_PARAMS& params)
{
    BOOL bHandled = FALSE;
    ctx.setup("HandleDraw");
    lua_pushlightuserdata(ctx.L, he);
    lua_pushinteger(ctx.L, params.cmd);
    lua_pushlightuserdata(ctx.L, params.hdc);
    lua_pushinteger(ctx.L, params.area.left);
    lua_pushinteger(ctx.L, params.area.top);
    lua_pushinteger(ctx.L, params.area.right);
    lua_pushinteger(ctx.L, params.area.bottom);
    if (LUA_OK == lua_pcall(ctx.L, 7, 1, ctx.errorRef))
        bHandled = lua_toboolean(ctx.L, -1);
    lua_pop(ctx.L,1);
    return bHandled;
}
BOOL LuaBehavior::HandleScroll (const LuaCallGruad& ctx, HELEMENT he, SCROLL_PARAMS& params )
{
    BOOL bHandled = FALSE;
    ctx.setup("HandleScroll");
    lua_pushlightuserdata(ctx.L, he);
    if(he == params.target)
        lua_pushvalue(ctx.L, -1);
    else
        lua_pushlightuserdata(ctx.L, params.target);
    lua_pushinteger(ctx.L, params.cmd);
    lua_pushinteger(ctx.L, params.pos);
    lua_pushboolean(ctx.L, params.vertical);
    if (LUA_OK == lua_pcall(ctx.L, 5, 1, ctx.errorRef))
        bHandled = lua_toboolean(ctx.L, -1);
    lua_pop(ctx.L,1);
    return bHandled;
}
void LuaBehavior::HandleSize (const LuaCallGruad& ctx, HELEMENT he)
{
    assert(he);
    ctx.setup("HandleSize");
    lua_pushlightuserdata(ctx.L, he);
    if (LUA_OK != lua_pcall(ctx.L, 1, 0, ctx.errorRef))
        lua_pop(ctx.L,1);
}
BOOL LuaBehavior::HandleTimer (const LuaCallGruad& ctx, HELEMENT he,TIMER_PARAMS& params)
{
    BOOL bHandled = FALSE;
    ctx.setup("HandleTimer");
    lua_pushlightuserdata(ctx.L, he);
    lua_pushinteger(ctx.L, params.timerId);
    if (LUA_OK == lua_pcall(ctx.L, 2, 1, ctx.errorRef))
        bHandled = lua_toboolean(ctx.L, -1);
    lua_pop(ctx.L,1);
    return bHandled;
}
BOOL LuaBehavior::HandleFocus (const LuaCallGruad& ctx, HELEMENT he, FOCUS_PARAMS& params)
{ 
    BOOL bHandled = FALSE;
    ctx.setup("HandleFocus");
    lua_pushlightuserdata(ctx.L, he);
    if(he == params.target)
        lua_pushvalue(ctx.L, -1);
    else
        lua_pushlightuserdata(ctx.L, params.target);
    lua_pushinteger(ctx.L, params.cmd);
    lua_pushboolean(ctx.L, params.by_mouse_click);
    lua_pushboolean(ctx.L, params.cancel);
    if (LUA_OK == lua_pcall(ctx.L, 5, 1, ctx.errorRef))
        bHandled = lua_toboolean(ctx.L, -1);
    lua_pop(ctx.L,1);

    return bHandled;
}

BOOL LuaBehavior::HandleKey (const LuaCallGruad& ctx, HELEMENT he, KEY_PARAMS& params)
{ 
    BOOL bHandled = FALSE;
    ctx.setup("HandleKey");
    lua_pushlightuserdata(ctx.L, he);

    if(he == params.target)
        lua_pushvalue(ctx.L, -1);
    else
        lua_pushlightuserdata(ctx.L, params.target);

    lua_pushinteger(ctx.L, params.cmd);
    lua_pushinteger(ctx.L, params.key_code);
    {
        lua_createtable(ctx.L, 0, 3);
        if (params.alt_state&CONTROL_KEY_PRESSED) {
            lua_pushboolean(ctx.L, true);
            lua_setfield(ctx.L, -1, "CTRL");
        }
        if (params.alt_state&SHIFT_KEY_PRESSED) {
            lua_pushboolean(ctx.L, true);
            lua_setfield(ctx.L, -1, "SHIFT");
        }
        if (params.alt_state&ALT_KEY_PRESSED) {
            lua_pushboolean(ctx.L, true);
            lua_setfield(ctx.L, -1, "ALT");
        }
    }
    if (LUA_OK == lua_pcall(ctx.L, 5, 1, ctx.errorRef))
        bHandled = lua_toboolean(ctx.L, -1);
    lua_pop(ctx.L, 1);
    return bHandled;
}
BOOL LuaBehavior::HandleMouse (const LuaCallGruad& ctx, HELEMENT he, MOUSE_PARAMS& params)
{
    BOOL bHandled = FALSE;
    ctx.setup("HandleMouse");
    const int top = lua_gettop(ctx.L) - 1;
    lua_pushlightuserdata(ctx.L, he);
    if(he == params.target)
        lua_pushvalue(ctx.L, -1);
    else
        lua_pushlightuserdata(ctx.L, params.target);
    lua_pushinteger(ctx.L, params.cmd);
    lua_pushinteger(ctx.L, params.pos.x);
    lua_pushinteger(ctx.L, params.pos.y);
    lua_pushinteger(ctx.L, params.pos_document.x);
    lua_pushinteger(ctx.L, params.pos_document.y);
    lua_pushinteger(ctx.L, params.button_state);
    lua_pushinteger(ctx.L, params.alt_state);
    lua_pushinteger(ctx.L, params.cursor_type);
    lua_pushboolean(ctx.L, params.is_on_icon);
    lua_pushinteger(ctx.L, params.dragging_mode);
    if(params.dragging == NULL)
        lua_pushnil(ctx.L);
    else
        lua_pushlightuserdata(ctx.L, params.dragging);

    if (LUA_OK == lua_pcall(ctx.L, 13, LUA_MULTRET, ctx.errorRef)){
        int n = top - lua_gettop(ctx.L);
        bHandled = lua_toboolean(ctx.L, n++);
        if (n < 0)
            params.cursor_type = luaL_checkinteger(ctx.L, n++);
        if (n < 0)
            params.is_on_icon  = lua_toboolean(ctx.L, n++);
        if (n < 0)
            params.dragging_mode = luaL_checkinteger(ctx.L, n++);
        if (n < 0)
            params.dragging = luaH_checkelement(ctx.L, n++);
    }
    lua_settop(ctx.L, top);
    return bHandled;
}
void LuaBehavior::HandleDetached(const LuaCallGruad& ctx, HELEMENT he)
{
    ctx.setup("HandleDetached");
    lua_pushlightuserdata(ctx.L, he);
    lua_pcall(ctx.L, 1, 0, ctx.errorRef);
} 
void LuaBehavior::HandleAttached(const LuaCallGruad& ctx, HELEMENT he)
{
    ctx.setup("HandleAttached");
    lua_pushlightuserdata(ctx.L, he);
    lua_pcall(ctx.L, 1, 0, ctx.errorRef);
}


