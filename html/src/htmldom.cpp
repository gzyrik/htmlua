#include "htmlua.h"
//=============================================================================
//元素API封装
//所有函数返回结果值HLDOM_RESULT

static int GetRootElement(lua_State* L)
{
    HELEMENT he;
    HWND hwnd = luaH_checkhwnd(L, 1);
    lua_pushinteger(L,
        HTMLayoutGetRootElement(hwnd, &he));
    lua_pushlightuserdata(L, he);
    return 2;
}
static int FindElement(lua_State* L)
{
    HELEMENT he;
    POINT pt;
    HWND hwnd = luaH_checkhwnd(L, 1);
    pt.x = luaL_checkinteger(L,2);
    pt.y = luaL_checkinteger(L,3);
    lua_pushinteger(L,
        HTMLayoutFindElement(hwnd, pt, &he));
    lua_pushlightuserdata(L, he);
    return 2;
}
static int UseElement(lua_State* L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    lua_pushinteger(L,
        HTMLayout_UseElement(he));
    return 1;
}
static int UnuseElement(lua_State* L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    lua_pushinteger(L,
        HTMLayout_UnuseElement(he));
    return 1;
}
static int MoveElement(lua_State* L)
{
    HLDOM_RESULT r;
    HELEMENT he = luaH_checkelement(L, 1);
    INT x = luaL_checkinteger(L, 2);
    INT y = luaL_checkinteger(L, 3);
    if (lua_gettop(L) >= 5){
        INT w = luaL_checkinteger(L, 4);
        INT h = luaL_checkinteger(L, 5);
        r = HTMLayoutMoveElementEx(he, x, y, w, h);
    }
    else
        r = HTMLayoutMoveElement(he, x, y);
    lua_pushinteger(L, r);
    return 1;
}
static int GetAttributeCount(lua_State* L)
{
    UINT count;
    HELEMENT he = luaH_checkelement(L, 1);
    lua_pushinteger(L,
        HTMLayoutGetAttributeCount(he, &count));
    lua_pushinteger(L, count);
    return 2;
}
//int,name,value(HELEMENT,int)
static int GetNthAttribute(lua_State* L)
{
    LPCSTR name;
    LPCWSTR value;
    HELEMENT he = luaH_checkelement(L, 1);
    UINT n = luaL_checkinteger(L, 2);
    lua_pushinteger(L,
        HTMLayoutGetNthAttribute(he, n, &name, &value));
    lua_pushstring(L, name);
    lua_pushstring(L, aux::w2a(value));
    return 3;
}
//int,string(HELEMENT,name)
static int GetAttributeByName(lua_State* L)
{
    LPCWSTR value;
    HELEMENT he = luaH_checkelement(L, 1);
    LPCSTR name = luaL_checkstring(L, 2);
    lua_pushinteger(L,
        HTMLayoutGetAttributeByName(he, name, &value));
    if (value && value[0])
        lua_pushstring(L, aux::w2a(value));
    else
        lua_pushnil(L);
    return 2;
}
static int Attribute(lua_State* L)
{
    HLDOM_RESULT hr;
    LPCWSTR value;
    HELEMENT he = luaH_checkelement(L, 1);
    const char* name = lua_tostring(L,2);
    if (name[0] == '-' || name[0] == '_') 
        name++;
    hr = HTMLayoutGetAttributeByName(he, name, &value);
    if (hr != HLDOM_OK || value == 0){
        std::string style;
        style.push_back('-');
        style.append(name);
        hr = HTMLayoutGetStyleAttribute(he, style.c_str(), &value);
    }
    lua_pushinteger(L, hr);
    if (value && value[0])
        lua_pushstring(L, aux::w2a(value));
    else
        lua_pushnil(L);
	return 2;
}
//int(HELEMENT, name, value)
static int SetAttributeByName(lua_State* L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    LPCSTR name = luaL_optstring(L, 2, 0);
    int r = HLDOM_INVALID_PARAMETER;
    if (lua_isnoneornil(L, 3))
        r = HTMLayoutSetAttributeByName(he, name, 0);
    else if (lua_isstring(L,3))
        r = HTMLayoutSetAttributeByName(he, name, aux::a2w(luaL_checkstring(L, 3)));
    lua_pushinteger(L, r);
    return 1;
}
static BOOL CALLBACK VisitElementCallback(HELEMENT he, LPVOID param)
{
    lua_State* L = (lua_State*)param;
    BOOL r = true;
    lua_pushvalue(L, 4);
    lua_pushlightuserdata(L, he);
    if(lua_pcall(L, 1, 1, 3) == LUA_OK)
        r = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return r;
}
//int(HELEMENT,string,string,string,function(HELEMENT), int)
static int VisitElements(lua_State* L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    LPCSTR tagName = luaL_optstring(L, 2, 0);
    LPCSTR attributeName =luaL_optstring(L, 3, 0);
    aux::a2w attributeValue(luaL_optstring(L, 4, 0));
    luaL_checktype(L, 5, LUA_TFUNCTION);
    DWORD  depth = luaL_checkinteger(L, 6);
    lua_pushinteger(L,
        HTMLayoutVisitElements(he, tagName, attributeName, 
            attributeValue, VisitElementCallback, L, depth));
    return 1;
}
//int(HELEMENT,string,function(HELEMENT))
static int SelectElements(lua_State* L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    aux::a2w CSS_selectors(luaL_checkstring(L, 2));
    luaL_checktype(L, 3, LUA_TFUNCTION);//OnError
    luaL_checktype(L, 4, LUA_TFUNCTION);//callback
    lua_pushinteger(L,
        HTMLayoutSelectElementsW(he, CSS_selectors, VisitElementCallback, L));
    return 1;
}
static BOOL CALLBACK CollectElementCallback(HELEMENT he, LPVOID arg)
{
    char** param = (char**)arg;
    param[0] = (char*)he;
    const int len = (int)++param[1];
    lua_State* L = (lua_State*)param[2];
    if (!L) return true;

    lua_pushlightuserdata(L, he);
    lua_rawseti(L, -2, len);
    return false;
}
//int(HELEMENT,string)
static int CollectElements(lua_State* L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    const char* selector = luaL_checkstring(L, 2);
    void* param[3] = {0,0,0};
    int idx = 0;
    if (lua_istable(L,-1)) {
        idx = lua_objlen(L, -1);
        param[1] = (void*)idx++;
        param[2] = L;
    }
    lua_pushinteger(L,
        HTMLayoutSelectElementsW(he, aux::a2w(selector), CollectElementCallback, param));
    lua_pushinteger(L, idx);//table start idx
    lua_pushinteger(L, (int)param[1]);//table len
    if (!param[0]) return 3;
    lua_pushlightuserdata(L, param[0]);//the last element
    return 4;
}
static int ControlSetValue(lua_State* L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    UINT t = CTL_UNKNOWN;
    HLDOM_RESULT r = HTMLayoutControlGetType(he, &t);
    if (r == HLDOM_OK) {
        json::value v;
        if (t == CTL_NO || t == CTL_UNKNOWN)
            return luaL_error(L, "invalid control type");
        else if(t == CTL_DATE || t == CTL_TIME)
            ;//v = StringToDate(PAS(L, 2));
        else if(lua_isboolean(L, 2))
            v = lua_toboolean(L, 2);
        else
            v = (const wchar_t*)aux::a2w(luaL_checkstring(L, 2));
        htmlayout::dom::element e(he);
        htmlayout::set_value(e, v);
    }
    lua_pushinteger(L, r);
    return 1;
}
//int,*,int(HELEMENT)
static int ControlGetValue(lua_State* L)
{
    htmlayout::dom::element e(luaH_checkelement(L, 1));
    json::value v = htmlayout::get_value(e);
    lua_pushinteger(L, HLDOM_OK);//TODO
    if(v.is_bool())
        lua_pushboolean(L, v.get(false));
    else if(v.is_int())
        lua_pushinteger(L, v.get(0));
    else if(v.is_float())
        lua_pushnumber(L, v.get(0.0f));
    else if (v.is_string())
        lua_pushstring(L, aux::w2a(v.to_string().c_str()));
    else//@TODO bin,T_DATE, T_FUNCTION,T_CURRENCY
        lua_pushnil(L);
    lua_pushinteger(L, v.t);
    return 3;
}
//int,int(HELEMENT)
static int ControlGetType(lua_State* L)
{
    static const char* ctl_types[] = {
        0,               ///< This dom element has no behavior at all.
        0,      ///< This dom element has behavior but its type is unknown.
        "EDIT",             ///< Single line edit box.
        "NUMERIC",          ///< Numeric input with optional spin buttons.
        "BUTTON",           ///< Command button.
        "CHECKBOX",         ///< CheckBox (button).
        "RADIO",            ///< OptionBox (button).
        "SELECT_SINGLE",    ///< Single select, ListBox or TreeView.
        "SELECT_MULTIPLE",  ///< Multiselectable select, ListBox or TreeView.
        "DD_SELECT",        ///< Dropdown single select.
        "TEXTAREA",         ///< Multiline TextBox.
        "HTMLAREA",         ///< WYSIWYG HTML editor.
        "PASSWORD",         ///< Password input element.
        "PROGRESS",         ///< Progress element.
        "SLIDER",           ///< Slider input element.  
        "DECIMAL",          ///< Decimal number input element.
        "CURRENCY",         ///< Currency input element.
        "SCROLLBAR",

        "HYPERLINK",

        "MENUBAR",
        "MENU",
        "MENUBUTTON",

        "CALENDAR",
        "DATE",
        "TIME",

        "FRAME",
        "FRAMESET",

        "GRAPHICS",
        "SPRITE",

        "LIST",
        "RICHTEXT",
        "TOOLTIP",

        "HIDDEN",
        "URL",            ///< URL input element.
        "TOOLBAR",

        "FORM",
    };
    UINT t=0;
    HELEMENT he = luaH_checkelement(L, 1);
    lua_pushinteger(L,
        HTMLayoutControlGetType(he, &t));
    if (ctl_types[t])
        lua_pushstring(L, ctl_types[t]);
    else
        lua_pushnil(L);
    return 2;
}
//int(HELEMENT,string,int)
static int SetElementHtml(lua_State* L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    BOOL outer = lua_toboolean(L, 2);
    aux::a2utf html(luaL_checkstring(L, 3));
    UINT flag = luaL_optinteger(L, 4, 0);
    if (outer) flag += 3; //SIH_* -> SOH_*
    lua_pushinteger(L,
        HTMLayoutSetElementHtml(he, html, html.length(), flag));
    return 1;
}
static void CALLBACK WriterCallbackB(LPCBYTE utf8, UINT utf8_length, LPVOID param)
{
    luaL_Buffer *b=(luaL_Buffer*)param;
    luaL_addlstring(b, (const char*)utf8, utf8_length);
}
//int,string(HELEMENT,bool)
static int GetElementHtml(lua_State* L)
{
    luaL_Buffer b;
    HELEMENT he = luaH_checkelement(L, 1);
    BOOL outer = lua_toboolean(L, 2);
    luaL_buffinit(L, &b);
    const HLDOM_RESULT r = 
        HTMLayoutGetElementHtmlCB(he, outer, WriterCallbackB, &b);
    if (r != HLDOM_OK) {
        lua_pushinteger(L, r);
        return 1;
    }
    luaL_pushresult(&b);
    lua_pushinteger(L, r);
    lua_insert(L,-2);
    return 2;
}
//int,int(HELEMENT)
static int GetChildrenCount(lua_State* L)
{
    UINT count;
    HELEMENT he = luaH_checkelement(L, 1);
    lua_pushinteger(L,
        HTMLayoutGetChildrenCount(he, &count));
    lua_pushinteger(L, count);
    return 2;
}
//int,HELEMENT(HELEMENT,int)
static int GetNthChild(lua_State* L)
{
    HLDOM_RESULT r;
    HELEMENT child = NULL;
    HELEMENT he = luaH_checkelement(L, 1);
    int n = luaL_checkinteger(L, 2);//-1表示末尾,1表示首个,0表示自身
    if (n == 0) {
        r = HLDOM_OK;
        child = he;
    }
    else {
        UINT count;
        r = HTMLayoutGetChildrenCount(he, &count);
        if (r == HLDOM_OK && count > 0){
            if (n < -(int)count) n = 0;
            else if (n > (int)count) n = count -1;
            else if (n < 0) n += count;
            else --n;
            r = HTMLayoutGetNthChild(he, n, &child);
        }
    }
    lua_pushinteger(L, r);
    lua_pushlightuserdata(L, child);
    return 2;
}
//int,int(HELEMENT)
static int GetElementIndex(lua_State* L)
{
    UINT index;
    HELEMENT he = luaH_checkelement(L, 1);
    lua_pushinteger(L,
        HTMLayoutGetElementIndex(he, &index));
    lua_pushinteger(L, index+1);//1表示首个
    return 2;
}
//int(HELEMENT)
static int DeleteElement(lua_State* L) 
{
    HELEMENT he = luaH_checkelement(L, 1);
    lua_pushinteger(L,
        HTMLayoutDeleteElement(he));
    return 1;
}
//int,HELEMENT(HELEMENT)
static int CloneElement(lua_State* L)
{
    HELEMENT hparent = luaH_checkelement(L, 1);
    HELEMENT he = luaH_checkelement(L, 2);
    HELEMENT he2 = 0;
    HLDOM_RESULT r = HTMLayoutCloneElement(he, &he2);
    if (r != HLDOM_OK || !he2) return 0;
    const int index = luaL_optinteger(L, 3, 0x7fffffff)-1;
    r = HTMLayoutInsertElement(he2, hparent, index);
    HTMLayout_UnuseElement(he2);
    lua_pushinteger(L, r);
    lua_pushlightuserdata(L, he2);
    return 2;
}
//int(HELEMENT,HELEMENT,int)
static int InsertElement(lua_State* L)
{
    HELEMENT hparent = luaH_checkelement(L, 1);
    LPCSTR tag = luaL_checkstring(L, 2);
    aux::a2w textOrNull(luaL_optstring(L,3,0));
    HELEMENT he = 0;
    HLDOM_RESULT r = HTMLayoutCreateElement(tag, textOrNull, &he);
    if (r != HLDOM_OK || !he) return 0;
    const int index = luaL_optinteger(L, 4, 0x7fffffff)-1;
    r = HTMLayoutInsertElement(he, hparent, index);
    HTMLayout_UnuseElement(he);
    lua_pushinteger(L, r);
    lua_pushlightuserdata(L, he);
    return 2;
}
//int(HELEMENT,HELEMENT,int)
static int ShowPopup(lua_State* L)
{
    HELEMENT hePopup = luaH_checkelement(L, 1);
    HELEMENT heAnchor = luaH_checkelement(L, 2);
    UINT placement = luaL_checkinteger(L, 3);
    lua_pushinteger(L,
        HTMLayoutShowPopup(hePopup, heAnchor, placement));
    return 1;
}
//int(HELEMENT,int,int,int)
static int ShowPopupAt(lua_State* L)
{
    POINT pos;
    HELEMENT hePopup = luaH_checkelement(L, 1);
    pos.x = luaL_checkinteger(L, 2);
    pos.y = luaL_checkinteger(L, 3);
    UINT mode = luaL_checkinteger(L, 4);
    lua_pushinteger(L,
        HTMLayoutShowPopupAt(hePopup, pos, mode));
    return 1;
}
//int(HELEMENT)
static int HidePopup(lua_State* L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    lua_pushinteger(L, HTMLayoutHidePopup(he));
    return 1;
}
//int,HELEMENT(string,string)
static int CreateElement(lua_State* L)
{
    HELEMENT he;
    LPCSTR tagname=luaL_checkstring(L, 1);
    aux::a2w textOrNull(luaL_optstring(L,2,0));
    lua_pushinteger(L,
        HTMLayoutCreateElement(tagname, textOrNull, &he));
    lua_pushlightuserdata(L, he);
    return 2;
}
//int(HELEMENT)
static int DetachElement(lua_State* L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    lua_pushinteger(L, HTMLayoutDetachElement(he));
    return 1;
}
//int(HELEMENT,bool)
static int ShowElementWnd(lua_State* L)
{
    HWND hwnd;
    HELEMENT he = luaH_checkelement(L, 1);
    BOOL bHide = lua_toboolean(L, 2);
    HLDOM_RESULT r = HTMLayoutGetElementHwnd(he,&hwnd, FALSE);
    if (r == HLDOM_OK){
        if (::IsWindow(hwnd)){
            ::ShowWindow(hwnd, bHide ? SW_HIDE: SW_SHOW);
        }
        else
            r = HLDOM_INVALID_HWND;
    }
    lua_pushinteger(L, r);
    return 1;
}
//int(HELEMENT)
static int RefreshElement(lua_State* L)
{
    RECT rc;
    HWND hwnd;
    HELEMENT he = luaH_checkelement(L, 1);
    HLDOM_RESULT r = HTMLayoutGetElementLocation(he,&rc, BORDER_BOX);
    if (r == HLDOM_OK)
        r = HTMLayoutGetElementHwnd(he,&hwnd, FALSE);
    if (r == HLDOM_OK)
        ::InvalidateRect(hwnd, &rc, FALSE);
    lua_pushinteger(L, r);
    return 1;
}
//int(HELEMENT,int)
static int UpdateElement(lua_State* L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    UINT flags = luaL_checkinteger(L, 2);
    lua_pushinteger(L,
        HTMLayoutUpdateElementEx(he,flags));
    return 1;
}
//int(HELEMENT,string,int,HELEMENT)
static int RequestElementData(lua_State*L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    aux::a2w url(luaL_checkstring(L,2));
    UINT dataType = luaL_checkinteger(L, 3);
    HELEMENT initiator = luaH_checkelement(L, 4);
    lua_pushinteger(L,
        HTMLayoutRequestElementData(he, url, dataType, initiator));
    return 1;
}
//int(HELEMENT,int);
static int ScrollToView(lua_State*L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    UINT flags = luaL_checkinteger(L, 2);
    lua_pushinteger(L,
        HTMLayoutScrollToView(he, flags));
    return 1;
}
//int(HELEMENT,x,y,bool)
static int SetScrollPos(lua_State*L)
{
    POINT pt;
    HELEMENT he = luaH_checkelement(L, 1);
    pt.x = luaL_checkinteger(L, 2);
    pt.y = luaL_checkinteger(L, 2);
    BOOL smooth = lua_toboolean(L, 4);

    lua_pushinteger(L,
        HTMLayoutSetScrollPos(he, pt, smooth));
    return 1;
}
//int,int*8(HELEMENT)
static int GetScrollInfo(lua_State*L)
{
    POINT pt;
    RECT rt;
    SIZE sz;
    HELEMENT he = luaH_checkelement(L, 1);
    lua_pushinteger(L,
        HTMLayoutGetScrollInfo(he, &pt, &rt, &sz));
    lua_pushinteger(L, pt.x);
    lua_pushinteger(L, pt.y);
    lua_pushinteger(L, sz.cx);
    lua_pushinteger(L, sz.cy);
    lua_pushinteger(L, rt.left);
    lua_pushinteger(L, rt.right);
    lua_pushinteger(L, rt.top);
    lua_pushinteger(L, rt.bottom);
    return 9;
}
//int,x,y,r,b(HELEMENT,int)
static int GetElementLocation(lua_State*L)
{
    RECT rect;
    HELEMENT he = luaH_checkelement(L, 1);
    UINT areas = luaL_checkinteger(L, 2);
    lua_pushinteger(L,
        HTMLayoutGetElementLocation(he, &rect, areas));
    lua_pushinteger(L, rect.left);
    lua_pushinteger(L, rect.top);
    lua_pushinteger(L, rect.right);
    lua_pushinteger(L, rect.bottom);
    return 5;
}
//int(HELEMENT)
static int SetCapture(lua_State*L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    lua_pushinteger(L,
        HTMLayoutSetCapture(he));
    return 1;
}
//()
static int ReleaseCapture(lua_State*L)
{
    ReleaseCapture();
    lua_pushinteger(L, HLDOM_OK);
    return 1;
}
//int,string(HELEMENT, string)
static int GetStyleAttribute(lua_State* L)
{
    LPCWSTR value;
    HELEMENT he = luaH_checkelement(L, 1);
    LPCSTR name = luaL_checkstring(L, 2);
    lua_pushinteger(L,
        HTMLayoutGetStyleAttribute(he, name, &value));
    lua_pushstring(L, aux::w2a(value));
    return 2;
}
//int(HELEMENT, string,string)
static int SetStyleAttribute(lua_State*L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    std::string name(luaL_checkstring(L, 2));
    for(size_t i=0;i<name.size(); ++i) {
        if (isupper(name[i])) {
            name[i] = tolower(name[i]);
            name.insert(i++, 1, '-');
        }
    }
    aux::a2w value(luaL_optstring(L, 3, 0));
    lua_pushinteger(L,
        HTMLayoutSetStyleAttribute(he, name.c_str(), value));
    return 1;
}
//int,HELEMENT(HELEMENT)
static int GetParentElement(lua_State*L)
{
    HELEMENT hparent;
    HELEMENT he=luaH_checkelement(L,1);
    lua_pushinteger(L,
        HTMLayoutGetParentElement(he, &hparent));
    lua_pushlightuserdata(L, hparent);
    return 2;
}
static void CALLBACK WriterCallbackW(LPCWSTR text, UINT text_length, LPVOID param )
{
    luaL_Buffer* b=(luaL_Buffer*)param;
    aux::w2a str(text, text_length);
    luaL_addlstring(b, (const char*)str, str.length());
}
//int,string(HELEMENT)
static int GetElementInnerText(lua_State*L)
{
    luaL_Buffer b;
    HELEMENT he=luaH_checkelement(L,1);
    luaL_buffinit(L, &b);
    const HLDOM_RESULT r =
        HTMLayoutGetElementInnerTextCB(he, WriterCallbackW, &b);
    if (r != HLDOM_OK) {
        lua_pushinteger(L, r);
        return 1;
    }
    luaL_pushresult(&b);
    lua_pushinteger(L, r);
    lua_insert(L,-2);
    return 2;
}
//int(HELEMENT,string)
static int SetElementInnerText(lua_State*L)
{
    HELEMENT he=luaH_checkelement(L, 1);
    aux::a2w txt(luaL_checkstring(L, 2));
    lua_pushinteger(L,
        HTMLayoutSetElementInnerText16(he, txt, txt.length()));
    return 1;
}
//int,string(HELEMENT)
static int GetElementType(lua_State*L)
{
    LPCSTR type;
    HELEMENT he=luaH_checkelement(L, 1);
    lua_pushinteger(L,
        HTMLayoutGetElementType(he, &type));
    if (type && type[0])
        lua_pushstring(L, type);
    else
        lua_pushnil(L);
    return 2;
}
//int,stirng(HELEMENT,string)
static int CombineURL(lua_State*L)
{
    wchar_t uri[2048];
    HELEMENT he = luaH_checkelement(L, 1);
    wcsncpy(uri,aux::a2w(luaL_checkstring(L, 2)),2048);
    lua_pushinteger(L,
        HTMLayoutCombineURL(he, uri, 2047));
    lua_pushstring(L, aux::w2a(uri));
    return 2;
}
//int,int(HELEMENT)
static int GetElementState(lua_State*L)
{
    UINT stateBits;
    HELEMENT he = luaH_checkelement(L, 1);
    lua_pushinteger(L,
        HTMLayoutGetElementState(he, &stateBits));
    lua_pushinteger(L, stateBits);
    return 2;
}
//int(HELEMENT,int,int,bool)
static int SetElementState(lua_State* L)
{
    UINT stats = 0;
    int r = HLDOM_OK;
    HELEMENT he = luaH_checkelement(L, 1);
    const BOOL updateView=lua_toboolean(L, 2);
    UINT stateBitsToSet=luaL_checkinteger(L, 3);
    UINT stateBitsToClear=luaL_checkinteger(L, 4);
    const UINT stateBitsToInverse=luaL_checkinteger(L, 5);
    const UINT stateBitsToQuery=luaL_checkinteger(L, 6);
    if (stateBitsToInverse){
        r = HTMLayoutGetElementState(he, &stats);
        if (r != HLDOM_OK) goto clean;
        stateBitsToSet   |= (stats ^ stateBitsToInverse)&stateBitsToInverse;
        stateBitsToClear |= stats & stateBitsToInverse;
    }
    stateBitsToClear &= ~stateBitsToSet;//同时存在时,设置优先
    if (stateBitsToSet || stateBitsToClear){
        r = HTMLayoutSetElementState(he,stateBitsToSet, stateBitsToClear, updateView);
        if (r != HLDOM_OK) goto clean;
        stats |= stateBitsToSet;
        stats &= ~stateBitsToClear;
    }
    if (stateBitsToQuery && !stateBitsToInverse)
        r = HTMLayoutGetElementState(he, &stats);
clean:
    lua_pushinteger(L, r);
    if (!stateBitsToQuery)
        return 1;
    lua_pushinteger(L, stateBitsToQuery&stats);
    return 2;
}
//int,HELEMENT(HELEMENT,string,int)
static int SelectParent(lua_State*L)
{
    HELEMENT heFound;
    HELEMENT  he = luaH_checkelement(L, 1);
    const char* selector=luaL_checkstring(L, 2);
    UINT      depth=luaL_checkinteger(L, 3);
    lua_pushinteger(L,
        HTMLayoutSelectParentW(he,  aux::a2w(selector), depth, &heFound));
    lua_pushlightuserdata(L,heFound);
    return 2;
}
//int(HELEMENT, int, int)
static int SetTimerEx(lua_State*L)
{
    HELEMENT he=luaH_checkelement(L, 1);
    UINT milliseconds=luaL_checkinteger(L, 2);
    UINT_PTR timerId=luaL_checkinteger(L, 3);
    lua_pushinteger(L,
        HTMLayoutSetTimerEx(he, milliseconds, timerId));
    return 1;
}
//int,HWND(HELEMENT, bool)
static int GetElementHwnd(lua_State* L)
{
    HWND hwnd;
    HELEMENT he = luaH_checkelement(L, 1);
    BOOL rootWindow = lua_toboolean(L,2);

    lua_pushinteger(L,
        HTMLayoutGetElementHwnd(he, &hwnd, rootWindow));
    lua_pushlightuserdata(L, hwnd);
    return 2;
}
//int(HELEMENT, int, HELEMENT, int)
static int PostEvent(lua_State* L)
{
    HELEMENT h = luaH_checkelement(L, 1);
    UINT appEventCode = luaL_checkinteger(L, 2);
    HELEMENT heSource = luaH_checkelement(L, 3);
    UINT_PTR reason= luaL_checkinteger(L, 4);
    lua_pushinteger(L, 
        HTMLayoutPostEvent(h,appEventCode, heSource, reason));
    return 1;
}
//int,bool(HELEMENT, int, HELEMENT, int)
static int SendEvent(lua_State* L)
{
    BOOL handled;
    HELEMENT h = luaH_checkelement(L, 1);
    UINT appEventCode = luaL_checkinteger(L, 2);
    HELEMENT heSource = luaH_checkelement(L, 3);
    UINT_PTR reason= luaL_checkinteger(L, 4);
    lua_pushinteger(L, 
        HTMLayoutSendEvent(h,appEventCode, heSource, reason, &handled));
    lua_pushboolean(L, handled);
    return 2;
}
//int(HELEMENT, HELEMENT)
static int SwapElements(lua_State* L)
{
    HELEMENT h1 = luaH_checkelement(L, 1);
    HELEMENT h2 = luaH_checkelement(L, 2);
    lua_pushinteger(L, HTMLayoutSwapElements(h1, h2));
    return 1;
}
static INT CALLBACK Comparator(HELEMENT h1, HELEMENT h2, LPVOID param)
{
    lua_State* L = (lua_State*)param;
    int ret = 0;
    lua_pushvalue(L, 5);
    lua_pushlightuserdata(L, h1);
    lua_pushlightuserdata(L, h2);
    if(lua_pcall(L, 2, 1, 4) == LUA_OK)
        ret = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return ret;
}
//int(HELEMENT, int, int, function)
static int SortElements(lua_State* L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    int firstIndex = luaL_checkinteger(L, 2);
    int lastIndex = luaL_checkinteger(L, 3);
    luaL_checktype(L, 4, LUA_TFUNCTION);//OnError
    luaL_checktype(L, 5, LUA_TFUNCTION);//callback
    lua_pushinteger(L, 
        HTMLayoutSortElements(he, firstIndex, lastIndex, Comparator, L));
    return 1;
}
static int DataReady(lua_State* L)
{
    HWND hwnd = luaH_checkhwnd(L, 1);
    aux::a2w uri(luaL_checkstring(L, 2));
    size_t len;
    const char* data = luaL_checklstring(L, 3, &len);
    lua_pushboolean(L,
        HTMLayoutDataReady(hwnd, uri, (LPBYTE)data, len));
    return 1;
}
static int XCallBehaviorMethod(lua_State* L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    LuaXCallParam params(L,2);
    lua_pushinteger(L, HTMLayoutCallBehaviorMethod(he, &params));
    return 1 + luaH_pushargv(L, 1, &params.retval);
}
static int GetWidget(lua_State* L)
{
    void* object;
    LPCWSTR type;
    HELEMENT he = luaH_checkelement(L, 1);
    lua_pushinteger(L,
        HTMLayoutGetObject(he, &object, &type));
    if (object && type && type[0]) {
        lua_pushlightuserdata(L, object);
        lua_pushstring(L, aux::w2a(type));
    }
    else {
        lua_pushnil(L);
        lua_pushnil(L);
    }
    return 3;
}
static int SetWidget(lua_State* L)
{
    HELEMENT he = luaH_checkelement(L, 1);
    lua_pushinteger(L,
        HTMLayoutSetObject(he, lua_touserdata(L,2), aux::a2w(luaL_checkstring(L, 3))));
    return 1;
}
//static int PackToTail(lua_State* L)
//{
//    const int top = lua_gettop(L);
//    const int num = lua_tointeger(L, 1);
//    for(int i=0;i<num;++i) lua_pushvalue(L, i+2);
//    return top-1;
//}
//static int UTF2ASCII(lua_State* L)
//{
//    aux::w2a str(aux::utf2w(lua_tostring(L,1)));
//    lua_pushlstring(L, str, str.length());
//    return 1;
//}
//=============================================================================
int luaopen_dom(lua_State* L)
{
    luaL_Reg htmlayout_lib[]={
#define _(x)  {#x, x}
        _(GetRootElement),
        _(FindElement),
        _(UseElement),
        _(UnuseElement),
        _(MoveElement),
        _(GetAttributeCount),
        _(GetNthAttribute),
        _(GetAttributeByName),
        _(SetAttributeByName),
		_(Attribute),
        _(VisitElements),
        _(SelectElements),
        _(CollectElements),
        _(ControlSetValue),
        _(ControlGetValue),
        _(ControlGetType),
        _(SetElementHtml),
        _(GetElementHtml),
        _(GetChildrenCount),
        _(GetNthChild),
        _(GetElementIndex),
        _(DeleteElement),
        _(InsertElement),
        _(ShowPopup),
        _(ShowPopupAt),
        _(HidePopup),
        _(CreateElement),
        _(CloneElement),
        _(DetachElement),
        _(ShowElementWnd),
        _(RefreshElement),
        _(UpdateElement),
        _(RequestElementData),
        _(ScrollToView),
        _(SetScrollPos),
        _(GetScrollInfo),
        _(GetElementLocation),
        _(SetCapture),
        _(ReleaseCapture),
        _(GetStyleAttribute),
        _(SetStyleAttribute),
        _(GetParentElement),
        _(GetElementInnerText),
        _(SetElementInnerText),
        _(GetElementType),
        _(CombineURL),
        _(GetElementState),
        _(SetElementState),
        _(SelectParent),
        _(SetTimerEx),
        _(GetElementHwnd),
        _(PostEvent),
        _(SendEvent),
        _(SwapElements),
        _(SortElements),
        _(DataReady),
        _(XCallBehaviorMethod),
        _(GetWidget),
        _(SetWidget),
        //_(PackToTail),
#undef _
        {0,0}
    };
    luaH_setfuncs(L, htmlayout_lib, 0);
    return 0;
} 
