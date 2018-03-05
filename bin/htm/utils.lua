local bit = require"bit"
local docstrings = setmetatable({}, {__mode = "kv"})
local function DOC(str)
    return function(obj) docstrings[obj] = str return obj end
end
local U={}
U.DOC = DOC[[定义对象文档并返回原始对象
@param doc 文档
@param obj 对象
@return obj
例如:
DOC"简介.
--参数 a 描述
"(function(a,...)
    ...
end)]](DOC)

U.HELP = DOC[[返回对象文档
@param obj 对象
@return doc 对应的文档字符串
]](function(obj)
    local f = type(obj) == 'table' and obj.__doc or getmetatable(obj)
    local t = type(f)
    if t == 'table' then
        f = f.__doc
        t = type(f)
    end
    if t == 'string' then return f end
    if t == 'function' then return f(obj) end
    return docstrings[obj]
end)

U.CLASS = function(name)
    local klass={assert=function(o) return o end, __property={}}
    local obj={
        __index = function(o, k)
            local f = klass.__property[k]
            assert(f, name.." invalid property "..k)
            return f(o.__element, o.__object)
        end,
        __newindex=function(o, k, v)
            local f = klass.__property[k]
            assert(f, name.." invalid property "..k)
            f(o.__element, o.__object, v)
        end
    }
    klass.__newindex=function(c, o, vals)
        c = c.__element
        o = klass.assert(o)
        local k,v = next(vals)
        while k do
            local f = klass.__property[k]
            assert(f, name.." invalid property "..k)
            f(c, o, v)
            k,v = next(vals,k)
        end
    end
    klass.__index = function(c, o)
        return setmetatable({__element=c.__element,__object=klass.assert(o)}, obj)
    end
    klass.__call = function(c, vals)
        local o,v = next(vals)
        while o do
            klass.__newindex(c, o, v)
            o,v=next(vals,o)
        end
    end
    return klass
end

U.toenum = DOC[[将字符串转为数值
@param val 字符常量或数值
@param map 字符常量与数值的对应
]](function(val, map)
    if map and type(val) == 'string' then
        val = map[string.upper(val)] or tonumber (val)
    end
    return val
end)

U.tomask= DOC[[将字符串合并成掩码值
前缀:
+ 表示正掩码 = v
- 表示负掩码 = k
~ 表示取反码 x

后缀:
? 表示查询码 = y

注意
有后缀时,该单词严格模式,例如
hover  与 +hover 等价
hover？只表示查询码,
+hove? 即有正掩码又有查询码
~hove? 即有取反码又有查询码

@param val 多个字符常量或数值
@param map 字符常量与数值的对应
@param strict 是否严格模式
@return v,k,x,y 返回正掩码值,负掩码值,取反掩码值,查询掩码值
]](function(val, map)
    if type(val) == 'string' then
        local v, k, x, y = 0, 0, 0,0
        for p,w,s in string.gmatch(val, "([~+-]*)([%w_-]+)([?]*)") do
            local m = map[string.upper(w)]
            if p == '-' then
                k = bit.bor(k,m)
            elseif p == '~' then
                x = bit.bor(x,m)
            elseif p == '+' or s =='' then
                v = bit.bor(v,m)
            end
            if s == '?' then y = bit.bor(y, m) end
        end
        return v,k,x,y
    end
    assert(type(val) == 'number')
    return val,0,0,0
end)
--------------------------------------------------------------------------------
U.BEHAVIOR_EVENTS = DOC[[HTMLayout 中的事件数值值,对应为事件名]]({
    [0] = {'BUTTON_CLICK',          'onClick',      "click on button"},
    [1] = {'BUTTON_PRESS',          'onPress',      "mouse down or key down in button"},
    [2] = {'BUTTON_STATE_CHANGED',  'onChanged',    "checkbox/radio/slider changed its state/value"},
    [3] = {'EDIT_VALUE_CHANGING',   'onChanging',   "before text change"},
    [4] = {'EDIT_VALUE_CHANGED',    'onChanged',    "after text change"},
    [5] = {'SELECT_SELECTION_CHANGED','onSelect',   "selection in <select> changed"},
    [6] = {'SELECT_STATE_CHANGED',  'onChanged',    "node in select expanded/collapsed, heTarget is the node"},

    [7] = {'POPUP_REQUEST',     'onRequest', "request to show popup just received, here DOM of popup element can be modifed."},
    [8] = {'POPUP_READY',       'onReady',   "popup element has been measured and ready to be shown on screen, here you can use functions like ScrollToView."},
    [9] = {'POPUP_DISMISSED',   'onDismissed', "popup element is closed,here DOM of popup element can be modifed again - e.g. some items can be removed to free memory."},
    [0x13]= {'POPUP_DISMISSING', 'onDismissing',"popup is about to be closed"},

    [0xA] = {'MENU_ITEM_ACTIVE', 'onActive', "menu item activated by mouse hover or by keyboard"},
    [0xB] = {'MENU_ITEM_CLICK',  'onClick',  [[menu item click
    - BEHAVIOR_EVENT_PARAMS structure layout
    - BEHAVIOR_EVENT_PARAMS.cmd - MENU_ITEM_CLICK/MENU_ITEM_ACTIVE   
    - BEHAVIOR_EVENT_PARAMS.heTarget - the menu item, presumably <li> element
    - BEHAVIOR_EVENT_PARAMS.reason - BY_MOUSE_CLICK | BY_KEY_CLICK]]},

    [0xF] = {'CONTEXT_MENU_SETUP', 'onSetup',   "evt.he is a menu dom element that is about to be shown. You can disable/enable items in it."},
    [0x10]= {'CONTEXT_MENU_REQUEST','onRequest',[['right-click', BEHAVIOR_EVENT_PARAMS::he is current popup menu HELEMENT being processed or NULL.
    application can provide its own HELEMENT here (if it is NULL) or modify current menu element.]]},

    [0x11]= {'VISIUAL_STATUS_CHANGED', 'onVisiual',  "broadcast notification, sent to all elements of some container being shown or hidden"},
    [0x12]= {'DISABLED_STATUS_CHANGED','onDisabled', "broadcast notification, sent to all elements of some container that got new value of :disabled state"},

    -- "grey" event codes  - notfications from behaviors from this SDK 
    [0x80] = {'HYPERLINK_CLICK', 'onClick', "hyperlink click"},
    [0x81] = {'TABLE_HEADER_CLICK', 'onClick', [[click on some cell in table header,
    - target = the cell, 
    - reason = index of the cell (column number, 0..n)]]},

    [0x82] = {'TABLE_ROW_CLICK', 'onClick', [[click on data row in the table, target is the row
    - target = the row, 
    - reason = index of the row (fixed_rows..n)]]},
    [0x83] = {'TABLE_ROW_DBL_CLICK', 'onDblClick', [[mouse dbl click on data row in the table, target is the row
    - target = the row, 
    - reason = index of the row (fixed_rows..n)]]},

    [0x90] = {'ELEMENT_COLLAPSED', 'onCollapsed',"element was collapsed, so far only behavior:tabs is sending these two to the panels"},
    [0x91] = {'ELEMENT_EXPANDED',  'onExpanded', "element was expanded"},

    [0x92] = {'ACTIVATE_CHILD', 'onActivateChild',[[activate (select) child, 
    - used for example by accesskeys behaviors to send activation request, e.g. tab on behavior:tabs.
    - DO_SWITCH_TAB = ACTIVATE_CHILD, command to switch tab programmatically, handled by behavior:tabs 
    - use it as HTMLayoutPostEvent(tabsElementOrItsChild, DO_SWITCH_TAB, tabElementToShow, 0)]]},

    [0x93] = {'INIT_DATA_VIEW', 'onInitDataView', "request to virtual grid to initialize its view"},

    [0x94] = {'ROWS_DATA_REQUEST', 'onRowsDataRequest',[[request from virtual grid to data source behavior to fill data in the table
    -- parameters passed throug DATA_ROWS_PARAMS structure.]]},

    [0x95] = {'UI_STATE_CHANGED', 'onUIStateChanged',[[ui state changed, observers shall update their visual states.
    is sent for example by behavior:richtext when caret position/selection has changed.]]},

    [0x96] = {'FORM_SUBMIT', 'onSubmit',[[behavior:form detected submission event. BEHAVIOR_EVENT_PARAMS::data field contains data to be posted.
    - BEHAVIOR_EVENT_PARAMS::data is of type T_MAP in this case key/value pairs of data that is about 
    - to be submitted. You can modify the data or discard submission by returning TRUE from the handler.]]},

    [0x97] = {'FORM_RESET', 'onReset', [[behavior:form detected reset event (from button type=reset). BEHAVIOR_EVENT_PARAMS::data field contains data to be reset.
    - BEHAVIOR_EVENT_PARAMS::data is of type T_MAP in this case key/value pairs of data that is about 
    - to be rest. You can modify the data or discard reset by returning TRUE from the handler.]]},

    [0x98] = {'DOCUMENT_COMPLETE', 'onDocumentComplete',"behavior:frame have complete document."},

    [0x99] = {'HISTORY_PUSH', 'onHistoryPush',"behavior:history stuff"},
    [0x9A] = {'HISTORY_DROP', 'onHistoryDrop',""},                     
    [0x9B] = {'HISTORY_PRIOR','onHistoryPrior',""},
    [0x9C] = {'HISTORY_NEXT', 'onHistoryNext', ""},

    [0x9D] = {'HISTORY_STATE_CHANGED', 'onHistoryChanged', "behavior:history notification - history stack has changed"},

    [0x9E] = {'CLOSE_POPUP', 'onClosePopup',"close popup request"},
    [0x9F] = {'REQUEST_TOOLTIP', 'onRequestTooltip', "request tooltip, BEHAVIOR_EVENT_PARAMS.he <- is the tooltip element."},
    [0xA0] = {'ANIMATION', 'onAnimation', "animation started (reason=1) or ended(reason=0) on the element."},
    [0x100]= {'FIRST_APPLICATION_EVENT_CODE', "",[[
    - all custom event codes shall be greater
    - than this number. All codes below this will be used
    - solely by application - HTMLayout will not intrepret it 
    - and will do just dispatching.
    - To send event notifications with  these codes use
    - HTMLayoutSend/PostEvent API.]]},
})

--状态名对应为 HTMLayout 中的状态值
local ELEMENT_STATE_BITS = 
{
    LINK       = 0x00000001,--STATE_LINK             = 0x00000001,
    HOVER      = 0x00000002,--STATE_HOVER            = 0x00000002,
    ACTIVE     = 0x00000004,--STATE_ACTIVE           = 0x00000004,
    FOCUS      = 0x00000008,--STATE_FOCUS            = 0x00000008,
    VISITED    = 0x00000010,--STATE_VISITED          = 0x00000010,
    CURRENT    = 0x00000020,--STATE_CURRENT          = 0x00000020,  // current (hot) item 
    CHECKED    = 0x00000040,--STATE_CHECKED          = 0x00000040,  // element is checked (or selected)
    DISABLED   = 0x00000080,--STATE_DISABLED         = 0x00000080,  // element is disabled
    READONLY   = 0x00000100,--STATE_READONLY         = 0x00000100,  // readonly input element 
    EXPANED    = 0x00000200,--STATE_EXPANDED         = 0x00000200,  // expanded state - nodes in tree view 
    COLLAPSED  = 0x00000400,--STATE_COLLAPSED        = 0x00000400,  // collapsed state - nodes in tree view - mutually exclusive with
    INCOMPLETE = 0x00000800,--STATE_INCOMPLETE       = 0x00000800,  // one of fore/back images requested but not delivered
    ANIMATING  = 0x00001000,--STATE_ANIMATING        = 0x00001000,  // is animating currently
    FOCUSABLE  = 0x00002000,--STATE_FOCUSABLE        = 0x00002000,  // will accept focus
    ANCHOR     = 0x00004000,--STATE_ANCHOR           = 0x00004000,  // anchor in selection (used with current in selects)
    SYNTHETIC  = 0x00008000,--STATE_SYNTHETIC        = 0x00008000,  // this is a synthetic element - don't emit it's head/tail
    ['OWNS-POPUP'] = 0x00010000,--STATE_OWNS_POPUP       = 0x00010000,  // this is a synthetic element - don't emit it's head/tail
    TABFOCUS   = 0x00020000,--STATE_TABFOCUS         = 0x00020000,  // focus gained by tab traversal
    EMPTY      = 0x00040000,--STATE_EMPTY            = 0x00040000,  // empty - element is empty (text.size() == 0 && subs.size() == 0) 
    --  if element has behavior attached then the behavior is responsible for the value of this flag.
    BUSY       = 0x00080000,--STATE_BUSY             = 0x00080000,  // busy, loading

    ['DRAG-OVER']  = 0x00100000,--STATE_DRAG_OVER        = 0x00100000,  // drag over the block that can accept it (so is current drop target). Flag is set for the drop target block
    ['DROP-TARGET']= 0x00200000,--STATE_DROP_TARGET      = 0x00200000,  // active drop target.
    MOVING     = 0x00400000,--STATE_MOVING           = 0x00400000,  // dragging/moving - the flag is set for the moving block.
    COPYING    = 0x00800000,--STATE_COPYING          = 0x00800000,  // dragging/copying - the flag is set for the copying block.
    ['DRAG-SOURCE']= 0x01000000,--STATE_DRAG_SOURCE      = 0x01000000,  // element that is a drag source.
    ['DROP-MARKER']= 0x02000000,--STATE_DROP_MARKER      = 0x02000000,  // element is drop marker

    PRESSED    = 0x04000000,--STATE_PRESSED          = 0x04000000,  // pressed - close to active but has wider life span - e.g. in MOUSE_UP it 
    --   IS still on, so behavior can check it in MOUSE_UP to discover CLICK condition.
    POPUP      = 0x08000000,--STATE_POPUP            = 0x08000000,  // this element is out of flow - popup 
    LTR        = 0x10000000,--STATE_IS_LTR           = 0x10000000,  // the element or one of its containers has dir=ltr declared
    RTL        = 0x20000000,--STATE_IS_RTL           = 0x20000000,  // the element or one of its containers has dir=rtl declared   
}
U.toStateBits = DOC[[将状态字符串转为正掩码,负掩码
]](function(states)
    return U.tomask(states, ELEMENT_STATE_BITS, true)
end)

local UPDATE_ELEMENT_FLAGS =
{
    STYLE  = 0x20,  --reset styles - this may require if you have styles dependent from attributes,
    ATTRIBUTE = 0x10,  --use these flags after SetAttribute then. RESET_STYLE_THIS is faster than RESET_STYLE_DEEP.
    INPLACE = 0x0001, --use this flag if you do not expect any dimensional changes - this is faster than REMEASURE
    RESIZE = 0x0002, --use this flag if changes of some attributes/content may cause change of dimensions of the element  
    NOW = 0X8000,      --invoke ::UpdateWindow function after applying changes
}
U.toUpdateFlags = DOC[[将更新状态转为掩码
]](function(flags)
    return U.tomask(flags, UPDATE_ELEMENT_FLAGS)
end)

local ELEMENT_AREAS =
{
    ROOT= 0X01,       -- or this flag if you want to get HTMLayout window relative coordinates, otherwise it will use nearest windowed container e.g. popup window.
    SELF= 0X02,       -- "or" this flag if you want to get coordinates relative to the origin of element iself.
    CONTAINER= 0x03,  -- position inside immediate container.
    VIEW= 0X04,       -- position relative to view - HTMLayout window

    CONTENT = 0x00,   -- content (inner)  box
    PADDING = 0x10,   -- content + paddings
    BORDER  = 0x20,   -- content + paddings + border
    MARGIN  = 0x30,   -- content + paddings + border + margins 

    BACK = 0X40, -- relative to content origin - location of background image (if it set no-repeat)
    FORE = 0X50, -- relative to content origin - location of foreground image (if it set no-repeat)

    SCROLLABLE = 0x60, -- scroll_area - scrollable area in content box 
}

U.toAreas = DOC[[将区域转为掩码
]](function(flags)
    return U.tomask(flags, ELEMENT_AREAS)
end)


local EVENT_GROUPS =
{
    Mouse = 0x0001,              -- mouse events 

    --键盘事件
    --参数格式(element, cmd, int code, state)
    --cmd 事件
    --           .target  行为的目标元素
    --           .type    行为类型,"DOWN","UP","CHAR"
    --state 表 {CTRL=, SHIFT=, ALT=}
    Key = 0x0002,                -- key events

    --焦点事件
    --参数格式(element, cmd, bool by_mouse_click, bool cancel)
    --cmd 事件
    --           .target  行为的目标元素
    --           .type    行为类型,"LOST","GOT"
    Focus = 0x0004,              -- focus events, if this flag is set it also means that element it attached to is focusable 

    Scroll = 0x0008,             -- scroll events 

    --定时器触发事件
    --参数格式(element,int timerId)
    Timer = 0x0010,              -- timer event 

    --尺寸改变事件
    --参数格式(element)
    Size = 0x0020,               -- size changed event 

    --元素绘制时调用
    --参数格式(element, enum cmd, HDC, int left, int top, int right, int bottom)
    --enum cmd可选值"BACKGROUND","CONTENT","FOREGROUND"
    Draw = 0x0040,               -- drawing request (event)

    --参数格式(element, initiator, string uri, int dataType, int status, int dataSize, void* data)
    DataArrived = 0x080,         -- requested data () has been delivered

    --由Behavior触发的事件,例BUTTON_CLICK, VALUE_CHANGED
    --参数格式(element, cmd, int reason, data)
    --cmd 事件
    --           .sinking 刚触发该事件
    --           .handled 行为是否已处理
    --           .target  行为的目标元素
    --           .type    行为类型 @see BEHAVIOR_EVENTS
    --           .name    行为昵称 @see BEHAVIOR_EVENTS
    --           .__doc   行为的相应文档 @see BEHAVIOR_EVENTS
    BehaviorEvent = 0x0100,      --[[secondary, synthetic events: 
    BUTTON_CLICK, HYPERLINK_CLICK, etc., 
    a.k.a. notifications from intrinsic behaviors]]
    
    --自定义的方法调用
    --参数格式(element, int fun, void* param)
    MethodCall = 0x0200,         -- behavior specific methods

    --自定义的脚本函数调用
    --参数格式(element, string fun, ...)
    ScriptCall = 0x0200,

    Exchange   = 0x1000,         -- system drag-n-drop
    Gesture    = 0x2000,         -- touch input events


    --处理控件的Notify事件
    --参数格式(element, string name, ...)
    Notify   = 0x1000000,

    --额外的初始化
    --参数格式(element)
    Attached = 0x2000000,

    --额外的清理工作,可能不被调用
    --参数格式: (element)
    Detached = 0x2000000,

    --处理拖曳文件
    --参数格式: (element, file1,...)
    DropFiles= 0x4000000,
}

U.toEventMask = DOC[[统计原型表的事件处理掩码
原型中事件处理函数必须以Handle开头, 实例的事件处理函数必须以on开头.
HandleMouse,HandleKey,HandleFocus,HandleScroll,
HandleTimer,HandleSize,HandleDraw,HandleDataArrived,
HandleBehaviorEvent,HandleMethodCall,HandleExchange,HandleGesture,
扩展的LUA事件
HandleNotify,HandleAttached,HandleDetached
]](function(prototype, prefix)
    if not prefix then return EVENT_GROUPS[prototype] end
    local mask = 0
    local k,v = next(EVENT_GROUPS);
    while k do
        if prototype[prefix..k] then mask = bit.bor(mask, v) end
        k,v=next(EVENT_GROUPS, k)
    end
    return mask
end)

U.mydir = DOC[[返回当前脚本所在目录
]](function(depth)
    local SEP = package.config:sub(1,1)
    local pwd = (SEP == '\\') and 'cd' or 'pwd'
    local path,n = debug.getinfo((depth or 1)+1, 'S').source
    --print('debug.getinfo', path)

    path, n= string.gsub(string.sub(path, 2), '[\\/]', SEP)
    if n > 0 then
        path = string.match(path, '^([^'..SEP..'].*'..SEP..').*$')
        --print('cd', path)
        n = io.popen('cd '..path..'&& '..pwd)
    else
        n = io.popen(pwd)
    end
    path = n:read'*l'
    n:close()

    --print('mydir', path)
    path = string.gsub(path, '\\','/')
    return path
end)
-------------------------------------------------------------------------------
--扩展标准table库
--table.json   将JSON串转表
--table.tojson 将表转JSON串
local table = require "table"
table.json = require("htm.htmlua").JsonTable
local function serialize(tbl)  
    local tmp = {}  
    local k,v = next(tbl)
    while k do
        local k_type = type(k)  
        local v_type = type(v)  
        local key = (k_type == "string" and "\"" .. k .. "\":") or (k_type == "number" and "")  
        local value = (v_type == "table" and serialize(v))  
        or (v_type == "boolean" and tostring(v))  
        or (v_type == "string" and "\"" .. v .. "\"")  
        or (v_type == "number" and v)  
        tmp[#tmp + 1] = key and value and tostring(key) .. tostring(value) or nil  
        k,v = next(tbl, k)
    end  
    if #tbl == 0 then  
        return "{" .. table.concat(tmp, ",") .. "}"  
    else  
        return "[" .. table.concat(tmp, ",") .. "]"  
    end  
end
function table.tojson(t)
    assert(type(t) == "table")  
    return serialize(t)
end

return U
