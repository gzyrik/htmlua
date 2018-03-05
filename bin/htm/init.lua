local C=require"htm.htmlua"
local U=require"htm.utils"
local bit=require"bit"
local rawlen = rawlen or table.maxn
local unpack = unpack or table.unpack
local __gc_setmetatable = require"htm.__gc"
local WidgetType={}
local L={
    work= print,
    trace = function(...) end,
    loop = U.DOC[[进入事件循环]](C.UILoop),
    post = U.DOC[[往线程发送异步信息
@param to   线程名字,主线程为'main'
@param msg  信息
@param wait 是否同步等待
]](function(to, msg, wait)
    C.PostTask(to, _G.__thread or 'main', wait, msg)
end),
    thread = U.DOC[[启动线程
--
例如:
    htm.thread(name, 'print(msg)'[, msg]);
子线程中,自动生成全局变量
    _G.__thread 当前线程名字
--
@param name 线程名字,须保证唯一性
@param code lua代码字符串
@param msg  传给code的消息
]](C.NewThread),
    widget = U.DOC[[设置或替换控件类型]](function(t)
        for k, v in pairs(t) do
            if not v.__method then v.__method = {} end
            if not v.__property then v.__property = {} end
            WidgetType[k] = v
        end
    end)
}
--------------------------------------------------------------------------------
--错误处理函数
C.OnError = function(msg,...)
    print(debug.traceback(string.format('%s ERROR: %s', os.date('%Y%m%d %H:%M:%S'), msg)), ...)
end
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--向前声明元表
local Window={ __doc = [[
当前窗口环境
静态函数 _OPEN, _HELP, _DOC, _THREAD, _CHANNEL
@property __chunk 内部函数集合
@property __env 父环境
@property __hwnd 内部的窗口句柄
@property window 指向自身的对象
@method E 类似jQuery的选择器
@property document 类似HTML DOM
]],
}
local Document={ __doc=[[
window.document 实例作为当前HTML DOM对象
@property root 根元素,即<html>节点元素
@method query, find
]],
}
local Element={attribute={},style={}}
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--弱引用所有元素对象
Element.objects = setmetatable({amount = 0}, {__mode = 'v'})
local __empty_table={__property={},__method={}}
--去掉API调用返回的首个状态值
local _ret = function(k, r, ...) assert(r==0, k..' failed') return ... end
--调用函数并去掉返回的首个状态值
local _api = function(f,k,...) return _ret(k, f[k](...)) end
-- HTML元素
-- 基本结构
--  element={__element,attribute={element},style={element}}
--  attribute,style反向引用自身
--  __element 是HELEMENT的C层指针,只允许由C层函数使用
--  __widget 内嵌控件指针,只能由__widgetype函数使用
--  __widgetype 内嵌控件类型
--  __prototype 行为表
local _element_new = U.DOC[[包装C层HELEMENT为元素对象.
@param helement 元素HELEMENT类型句柄,lightuserdata类型
@return element 元素对象
]](function(helement)
    assert(type(helement) == 'userdata')
    local element = Element.objects[helement]
    if element then return element end

    C.UseElement(helement)
    --组合__tostring,方便打印定位
    local name,r,id,etype, ctype,widget,wtype,prototype,func,wnd,events=""

    --获取自定义行为,延时定位
    r, prototype = C.Attribute(helement, '-prototype')
    --获取自定义的控件及其类型
    r, widget, wtype = C.GetWidget(helement) --获取自定义控件
    --元素类型
    r, id = C.GetAttributeByName(helement, 'id')
    r, ctype = C.ControlGetType(helement)
    r, etype = C.GetElementType(helement)
    --所在窗口
    r, wnd = C.GetElementHwnd(helement, true)
    assert(r ==0 and wnd)

    wnd = Window.objects[wnd]
    if prototype then
        name = prototype
        prototype = rawget(wnd, prototype)
        assert(type(prototype) == 'table', "Undefined prototype "..name)
    end
    if wtype then name = name .. '@'..wtype end
    if ctype then name = name..'!'..ctype end
    if etype then name = name..'^'..etype end
    if id then
        name = name..'#'..id
    else
        name = name..'&'..string.sub(tostring(helement),11)--跳过"userdata: "
    end

    element={
        __element = helement, __widget = widget, __tostring = name,
        __widgetype= WidgetType[wtype]  or __empty_table,
        __prototype= prototype or __empty_table
    }
    element.attribute = setmetatable({__element=element}, Element.attribute)
    element.style = setmetatable({__element=element}, Element.style)
    element.__context = setmetatable({self=element},{__index=wnd})
    Element.objects[helement] = __gc_setmetatable(element, Element) 
    Element.objects.amount = Element.objects.amount + 1
    L.trace('OnElementUsed', name, Element.objects.amount)

    func = element.__widgetype.UseElement
    if func then func(element) end
    func = element.__prototype.UseElement
    if func then func(element) end

    events = U.toEventMask(element.__prototype, 'Handle')
    if events ~= 0 then C.BindBehavior(element, events) end
    return element
end)
local function _element_new_(he, ...)
    if he then return _element_new(he), _element_new_(...) end
end
--------------------------------------------------------------------------------
local _iselement=function(element) return type(element)=='table' and rawget(element, '__element') end 
Element.__gc = U.DOC[[元素对象回收时删除释放句柄
@return element 元素对象
]](function(element)
    local helement = _iselement(element)
    if not helement then return end
    Element.objects[helement] = nil
    Element.objects.amount = Element.objects.amount - 1
    L.trace('OnElementUnused', rawget(element, '__tostring'), Element.objects.amount)
    local f, g = rawget(element, '__prototype'), rawget(element, '__widgetype')
    f = f and f.UnuseElement
    g = g and g.UnuseElement
    if f then f(element) end
    if g then g(element) end
    C.UnuseElement(element)
end)
--------------------------------------------------------------------------------
--统一操作集成或元素的函数
local _element =function(element) return _iselement(element) and element or element[1] end
local _element_rawset=function(element, k, v)
    if _iselement(element) then
        rawset(element, k, v) 
    else
        for _, e in ipairs(element) do rawset(e, k, v) end
    end
end
local _element_env_ctx = function(e)
    local ctx = rawget(_element(e), '__context')
    return getmetatable(ctx).__index, ctx
end
local _element_newindex = function(e, k, v)
    local f = e.__widgetype.__property[k] or Element.__property[k]
    if f then 
        f(e, v)
    else
        rawset(e, k, v)
        local evt = string.match(k, '^on(%u%a*)$')
        evt = evt and U.toEventMask(evt)
        if  evt then
            local mask = type(v) == 'function' and evt or 0
            assert(mask ~= 0 or not v)
            C.BindBehavior(e, mask, evt)
        end
    end
end
--将c添加至变参末尾
local function _append_argv(c,...) 
    if select('#',...) > 0 then
        return select(1,...),_append_argv(c,select(2,...))
    else
        return c
    end
end
local __element_call = function(e, k, c, ...)
    local f = e.__widgetype[k]
    if f and c then
        return _ret(k, f(e, _append_argv(c, ...)))
    elseif f then
        return _ret(k, f(e, ...))
    else
        return _ret(k, c(e, ...))
    end
end
--------------------------------------------------------------------------------
local _element_call = U.DOC[[调用内嵌控件或标准元素方法
@param element 元素对象
@param k 方法名称
@return value 方法返回的次值或首值
]](function(element, k, ...)
    local c = C[k]
    if _iselement(element) then
        return __element_call(element, k, c,...)
    else
        local t={}
        for _, e in ipairs(element) do
            table.insert(t, _, __element_call(e, k, c,...))
        end
        return unpack(t)
    end
end)
--------------------------------------------------------------------------------
--样式元表
Element.style.__index = U.DOC[[返回样式值
@param s 样式对象
@param k 样式名称
@return value 样式值
]](function(s, k)
    return _element_call(s.__element, 'GetStyleAttribute', k)
end)
Element.style.__call = U.DOC[[批量设置样式
@param s 样式对象
@param t 样式表
支持如下调用
editor.style{
    default={
        font="Courier New",
        size=10,
        bold=true,
    },
    caretLineVisible = true,
    caretLineBack = 0xb0ffff,
}
]](function(s, styles)
    for k, v in pairs(styles) do
        _element_call(s.__element, 'SetStyleAttribute', k, v)
    end
end)
Element.style.__newindex = U.DOC[[设置样式值
@param s 样式对象
@param k 样式名称
@param v 样式值
@return result 0成功,反之错误
]](function(s, k, v)
    _element_call(s.__element, 'SetStyleAttribute', k, v)
    local onChange = rawget(s.__element, 'onStyleChange')
    if onChange then onChange(s.__element, k, v) end
    return r
end)
--------------------------------------------------------------------------------
--属性元表
Element.attribute.__len = function(a)
    local element = a.__element
    local r,n = C.GetAttributeCount(element)
    assert(r==0)
    return n
end
Element.attribute.__call = U.DOC[[批量设置或返回所有属性值
@param a 属性对象
@return table 所有属性值
]](function(a, ...)
    local element = a.__element
    if select('#',...) == 0 then --返回所有属性值
        local r,n = C.GetAttributeCount(element)
        assert(r==0)
        local map={}
        for i=0,n-1 do
            local r, k, v = C.GetNthAttribute(element, i)
            assert(r==0)
            map[k] = v
        end
        --优先保留内嵌控件的属性
        local count, nth = element.__widgetype.GetAttributeCount, element.__widgetype.GetNthAttribute
        if count then
            r,n = count(element)
            assert(r==0)
            for i=0,n-1 do
                local r, k, v = nth(element, i)
                assert(r==0)
                map[k] = v
            end
        end
        return map
    else
        for k, v in pairs(...) do
            _element_call(element, 'SetAttributeByName', k, v)
        end
    end
end)
Element.attribute.__newindex = function(a, k, v)
    local element = a.__element
    if string.match(k, '^on%w+$') and type(v) == 'function' then
        --临时保存,加速事件回调
        _element_rawset(element, k, v)
        local wnd = _element_env_ctx(element)
        for i,f in ipairs(wnd.__chunk) do
            if f == v then v = i goto OK end
        end
        table.insert(wnd.__chunk, v)
        v = rawlen(wnd.__chunk)
        ::OK::
        L.trace('SaveEventHandle', k, v)
    end
    _element_call(element, 'SetAttributeByName', k, v)
end
Element.attribute.__index = function(a, k)
    local element = a.__element
    local r,v= string.byte(k,1)
    if r == 95 or r == 45 then -- '_','-'
        v = _element_call(element, 'Attribute', k)
    else
        v = _element_call(element, 'GetAttributeByName', k)
    end
    if not v or #v == 0 then
        return nil
    elseif string.match(k, '^on%w+$') then --事件处理函数,编译并保存
        local wnd, ctx = _element_env_ctx(element)
        local f = tonumber(v)
        if not f then
            v, f = loadstring(v,k)
            if v then
                v = setfenv(v, ctx)
                table.insert(wnd.__chunk, v)
                _element_call(element, 'SetAttributeByName', k, rawlen(wnd.__chunk))
            else
                return C.OnError(f)
            end
        else
            v = wnd.__chunk[f]
        end
        L.trace('ReadEventHandle', k, v)
        rawset(element, k, v) --临时保存,加速事件回调
    end
    return v
end
--------------------------------------------------------------------------------
--元素方法
local QUERY_HELP =[[类似于jQuery的元素查询器
--
element:query 为当前元素中查询
document:query 和 E 为全局查询
E1 返回首个元素
可组合使用如下几种方式:
    - 返回符合条件的元素表(也可能是单个元素,或nil)
        all  = E(selector)
    - 按条件查找首个元素,或nil
        first = E(selector, true)
    - 级连调用,返回符合条件的元素表(也可能是单个元素,或nil)
        all = E(selector1,E(selector2))
    - 将符合条件的元素,添加到元素表,并强制返回该表
        exist = {...}
        exist = E(selector, exist}
--
@param selector 查询条件
@param all 可选的输出表
--
@return all 返回所有元素或首个元素
--
@see http://terrainformatica.com/htmlayout/selectors.whtm
]] 
_element_query = U.DOC(QUERY_HELP)(function(element, selector, all)
    local force
    if all then
        if _iselement(all) then --实现E('*',E'html')情况
            all={all}
        elseif type(all) == 'table' then
            force= true
        end
    else
        all = {}
    end
    local idx,len,val = _api(C, 'CollectElements', element, selector, all)
    if idx > 0 then -- all is table
        for i=idx,len do all[i] = _element_new(all[i]) end
        if force or len > 1 then
            return setmetatable(all, Element)
        elseif len == 1 then
            return all[1]
        end
    elseif val then
        return _element_new(val)
    end
end)
local _element_method={}
_element_method.query = _element_query
_element_method.E = _element_query
_element_method.E1 = U.DOC(QUERY_HELP)(function(element, selector)
    return _element_query(element, selector, true)
end)

_element_method.url=U.DOC[[获取完整的URL
--
将相对地址转换为绝对地址
--
@param element 元素对象
@param relative 资源的相对地址
--
@return combined_url 返回资源的完整URL,例如'file://...'
]](function(element, url)
    local _, url = C.CombineURL(element, url or '')
    return url
end)
_element_method.prepend = U.DOC[[头部插入原始HTML字符串
--
头部的位置有两个选择
    - 元素标签内部的HTML开始处
    - 元素开始标签之前的,属于父元素的空隙处
--
@param element 元素对象
@param html HTML字符串
@param outer 插入的位置是否在元素开始标签之前
--
@return result 出错值
]](function(element, html, outer)
    return _element_call(element, 'SetElementHtml', outer, html, 1)
end)
_element_method.append = U.DOC[[尾部添加原始HTML字符串
--
尾部的位置有两个选择
    - 元素标签内部的HTML结束处
    - 元素结束标签之后的,属于父元素的空隙处
--
@param element 元素对象
@param html HTML字符串
@param outer 添加的位置是否在元素结束标签之后
--
@return result 出错值
]](function(element, html, outer)
    return _element_call(element, 'SetElementHtml', outer, html, 2)
end)
_element_method.clone = U.DOC[[克隆并插入子元素
--
从child深度复制出新元素对象,并插入到element中
    child = element:clone(template[,index])
--
@param element  父元素
@param template 原元素
@param index    插入位置,从1开始,省略或过大则添加至末尾
--
@return child   返回子元素对象
]](function(element, template, index)
    return _element_new_(_element_call(element, 'CloneElement', template, index))
end)

_element_method.insert = U.DOC[[创建并插入子元素
--
tag为HTML元素名,则创建并插入对应的子元素
    child = element:insert(tag, [text, [index] ])
--
@param element  元素对象
@param tag      元素类型,例如'div'
@param text     元素的文本值
@param index    插入位置,从1开始,省略或过大则添加至末尾
--
@return child   返回子元素对象
]](function(element, tag, text, index)
    return _element_new_(_element_call(element, 'InsertElement', tag, text, index))
end)
_element_method.update=U.DOC[[强制更新元素
--
指示需要更新元素
--
@param element 元素对象
@param flags 更新指示
--
@return result 出错值
--
@remarks
flags 默认"resize", 可取下列值的组合
    style(样式改变),       attribute(属性改变)
    inplace(尺寸没有改变), resize(尺寸需要改变)
    now(立即更新窗口)
]](function(element, flags)
    return C.UpdateElement(element, U.toUpdateFlags(flags or "resize"))
end)
_element_method.place=U.DOC[[设置或返回坐标
--
若xView,yView, width, height 为数值,则重新设置元素位置
    local result = element:place(xView,yView, width, height)
反之,返回元素的区域
    local left,top,right,bottom = element:place(area)
--
@param element 元素对象
@param area(xView) 区域掩码(或相对x坐标)
@param yView 相对y坐标
@param width 宽度
@param height 高度
@return left,top,right,bottom
--
@remarks
area 默认值"view border", 可取下列值组合
    root        -- or this flag if you want to get HTMLayout window relative coordinates,
                -- otherwise it will use nearest windowed container e.g. popup window
    self        -- "or" this flag if you want to get coordinates relative to the origin of element iself.

    container   -- position inside immediate container.
    view        -- position relative to view - HTMLayout window

    content     -- content (inner)  box
    padding     -- content + paddings
    border      -- content + paddings + border
    margin      -- content + paddings + border + margins 

    back        -- relative to content origin - location of background image (if it set no-repeat)
    fore        -- relative to content origin - location of foreground image (if it set no-repeat)

    scrollable  -- scroll_area - scrollable area in content box 
]](function(element, area, yView, width, height)
    if yview then
        return _api(C, 'MoveElement', element, area, yView, width, height)
    else
        return _api(C, 'GetElementLocation', element, U.toAreas(area or "view border"))
    end
end)
_element_method.state = U.DOC[[设置或返回元素状态
--
可组合使用如下几种方式:
    - 无参数时返回状态数值
        states = element:state()
    - 设置状态位
        result = element:state('+focus -current +0x2', [delayUpdate])
        前缀:  +表示设置(可省略),-表示删除, ~表示取反 
    - 查询状态位
        后缀:  ?表示查询
        result = element:state('focus? current')
--
@param element 元素对象
@param states 状态数值或枚举
@param delayUpdate 是否延时更新
--
@return result 无参数或查询时返回状态掩码
--
@remarks
有效的状态枚举如下,
            1           2           4           8
0x1         link        hover       active      focus
0x10        visited     current     checked     disabled
0x100       readonly    expaned     collapsed   incomplete
0x1000      animating   focusable   anchor      synthetic
0x10000     owns-popup  tabfocus    empty       busy
0x100000    drag-over   drop-target moving      copying
0x1000000   drag-source drop-marker pressed     popup
0x10000000  ltr         rtl

]](function(element, states, delayUpdate)
    if not states then
        return _api(C, 'GetElementState', element)
    else 
        return _element_call(element, 'SetElementState', not delayUpdate, U.toStateBits(states))
    end
end)
_element_method.destroy = U.DOC[[销毁元素
--
@param element 元素对象
--
@return result 出错值
]](function(element)
    return _api(C, 'DeleteElement', element)
end)
_element_method.xcall = U.DOC[[调用行为的扩展脚本
--
按如下次序挑选
    - 已绑定-prototype变量的'HandleScriptCall',
    - 控件的XCallBehaviorMethod
    - 标准C层函数使用绑定behavior的扩展脚本
--
@param element 元素对象
@param func 函数名
@param ... 其他参数
--
@return result,rval 出错值,脚本返回值
]](function(element, func, ...)
    local f = element.__prototype.HandleScriptCall or element.__widgetype.XCallBehaviorMethod or C.XCallBehaviorMethod
    return f(element, func, ...)
end)
--------------------------------------------------------------------------------
--运行时属性(相对于attribute是可序列化的)
Element.__property = {
    __hwnd= U.DOC[[只读的所属窗口句柄]](function(e,v)
        assert(v==nil, 'readonly property')
        return _element_call(e, 'GetElementHwnd')
    end),
    type  = U.DOC[[只读的节点类型,例如head,div,body]](function(e, v)
        assert(v==nil, 'readonly property')
        return _element_call(e, 'GetElementType')
    end),
    parent= U.DOC[[只读的父节点元素]](function(e,v)
        assert(v==nil, 'readonly property')
        return _element_new(_api(C, 'GetParentElement', e))
    end),
    index = U.DOC[[只读的在父节点中序号,从1开始]](function(e,v)
        assert(v==nil, 'readonly property')
        return _element_new(_api(C, 'GetElementIndex', e))
    end),
    value = U.DOC[[相应的内部值]](function(e,v)
        return _element_call(e, v and 'ControlSetValue' or 'ControlGetValue', v)
    end),
    text  = U.DOC[[内嵌文本]](function(e,v)
        return _element_call(e, v and 'SetElementInnerText' or 'GetElementInnerText', v)
    end),
    innerHTML= U.DOC[[开始和结束标签之间的HTML字符串]](function(e,v)
        return _element_call(e, v and 'SetElementHtml' or 'GetElementHtml', false, v)
    end),
    outerHTML= U.DOC[[包含标签的完整HTML字符串]](function(e,v)
        return _element_call(e, v and 'SetElementHtml' or 'GetElementHtml', true, v)
    end),
}
Element.__len = function(element)
    if _iselement(element) then
        return _api(C, 'GetChildrenCount', element)
    else
        return rawlen(element)
    end
end
Element.__newindex = function(element, key, val)
    if _iselement(element) then
        return _element_newindex(element, key, val)
    else
        for _, e in ipairs(element) do _element_newindex(e, key, val) end
    end
end
Element.__index = function(element, key)
    if not _iselement(element) then
        if key == 'attribute' then
            return setmetatable({__element = element}, Element.attribute)
        elseif key == 'style' then
            return setmetatable({__element = element}, Element.style)
        end
        element = element[1]
    end
    if type(key) == 'number' then
        return _element_new(_api(C, 'GetNthChild', element, key))
    else --优先处理属性
        local f = element.__widgetype.__property[key] or Element.__property[key]
        if f then return f(element) end
        --标准方法,自定义行为,自定义控件行为
        return element.__prototype[key] or element.__widgetype.__method[key] or _element_method[key]
    end
end
Element.__doc = function(element)
    local doc={[[
HTML DOM的元素
内部样式表 element.style
属性表     element.attribute
--------------------------------------------------------------------------------
#property #  子元素个数的操作符
#property [] 返回子元素的操作符,-1表示末尾,1表示首个,0表示自身
]]}
    for k,v in pairs(Element.__property) do
        table.insert(doc, '#property '..k..'\t'..U.HELP(v))
    end
    for k,v in pairs(_element_method) do
        table.insert(doc, '--------------------------------------------------------------------------------')
        table.insert(doc, '#method '..k..'\t'..U.HELP(v))
    end
    if element.__widget then
        table.insert(doc, '--------------------------------------------------------------------------------')
        table.insert(doc,[[内嵌控件]])
        for k,v in pairs(element.__widgetype.__property) do
            table.insert(doc, '#property '..k..'\t'..U.HELP(v))
        end
        for k,v in pairs(element.__widgetype.__method) do
            table.insert(doc, '--------------------------------------------------------------------------------')
            table.insert(doc, '#method '..k..'\t'..U.HELP(v))
        end
    end
    return table.concat(doc, '\n')
end
--可打印的字符串
Element.__tostring = function(element) return element.__tostring end
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
Document.query = U.DOC(QUERY_HELP)(function(document,selector, all)
    assert(getmetatable(document) == Document)
    local r, root = C.GetRootElement(document.window.__hwnd)
    assert(r==0 and root)
    return _element_query(root, selector, all)
end)
Document.find = U.DOC[[根据坐标查找元素
@param document 文档对象
@param x 坐标
@param y 坐标
@return element 返回指定坐标的元素,失败返回nil
]](function(document, x, y)
    assert(getmetatable(document) == Document)
    local he = _api(C, 'FindElement', document.window, x, y)
    if he then return _element_new(he) end
    return nil
end)

Document.__index = function(doc, key)
    if key == 'query' then
        return Document.query
    elseif key == 'find' then
        return Document.find
    elseif key == 'root' then
        local r, root = C.GetRootElement(doc.window.__hwnd)
        assert(r==0 and root)
        return _element_new(root)
    end
end
--------------------------------------------------------------------------------
--window实例作为当前HTML的环境_ENV, window.__env 为父环境
--强引用所有窗口对象,在C.onClose中删除
--------------------------------------------------------------------------------
Window.objects = {amount = 0}
local _window_new = U.DOC[[包装C层指针为元素对象.
@param hwnd 窗口指针,lightuserdata类型
@param env  继承的LUA环境
@return window 窗口对象
]](function(hwnd, env)
    assert(hwnd)
    local window = Window.objects[hwnd]
    if not window then
        window ={__env= env or _G, __hwnd= hwnd, __chunk={}}
        local win_query = U.DOC(QUERY_HELP)(function(selector, all) 
            local r, root = C.GetRootElement(hwnd)
            assert(r==0 and root)
            return _element_query(root, selector, all)
        end)
        window.E = win_query;
        window.E1 = U.DOC(QUERY_HELP)(function(selector) return win_query(selector, true); end)
        window.window = window
        window.document=setmetatable({window=window}, Document)
        Window.objects[hwnd] = window
        Window.objects.amount = Window.objects.amount + 1
        L.trace('OnWindowCreated', hwnd, Window.objects.amount)
        setmetatable(window, Window)
    elseif env and env ~= window.__env then
        window.__env = env
    end
    return window
end)

local _window_open = U.DOC[[新建HTML窗口
@param url 网页地址,!替换为当前脚本所在绝对目录
@param parent 父窗口HWND
@param env  继承的LUA环境
@return window 窗口对象
]](function(url, parent, env)
    if string.find(url, '!') then url = string.gsub(url, '!', U.mydir(2)) end
    local wnd, widget = C.HtmlView.CreateElement(parent)
    wnd = _window_new(wnd, env or _G)
    C.HtmlView.ControlSetValue(widget, url)
    return wnd
end)
Window.__newindex = function(w, k, v)
    if k == '_WORK'  and type(v) == 'function' then
        L.work = v
    else
        rawset(w, k, v)
    end
end
Window.__index = function(w, k)
    assert(getmetatable(w) == Window)
    if k == '_OPEN' then
        return _window_open
    elseif k == '_HELP' then
        return U.HELP
    elseif k == '_DOC' then
        return U.DOC
    elseif k == '_THREAD' then
        return L.thread
    elseif k == '_ANSI' then
        return C.ANSI
    else
        return w.__env[k]
    end
end
--------------------------------------------------------------------------------
--分离URL
local function parse_url(url) 
    local _, _, protocol, pkg, path, param = string.find(url, '([%w]+)://([^/]+)/([^?]+)(.*)')
    protocol = string.lower(protocol or '')
    return protocol, pkg, path, param
end
--------------------------------------------------------------------------------
--加载脚本
local function loadScript(env, src, ...)
    local f = string.match(src, '^file://(.*)')
    local f, msg = f and loadfile(f) or loadstring(src,'<script>')
    if f then
        xpcall(setfenv(f, env), C.OnError, ...)
    else
        C.OnError(msg)
    end
end
--加载<script>元素中的脚本
local function loadHtmlScript(env, e, ...)   
    local url = e.attribute['src'] --加载src指定的文件内容
    if url then
        url = e:url(url)
        if url then
            loadScript(env, url, ...)
        else
            C.OnError(url)
        end
    end
    url = e.text --加载内容
    if url then loadScript(env, url, ...) end
end

local function loadFrameScript(env, e, ...)
    local root = e:query('html')	
    if root then env = _window_new(root.__hwnd, env) end

    for _, v in ipairs(e:query("script[type='text/lua']",{})) do
        loadHtmlScript(env, v, ...)
    end
end
--------------------------------------------------------------------------------
--窗口事件处理
--------------------------------------------------------------------------------
C.OnWindowClose = function(hwnd)
    return true 
end
C.OnWindowDestroy = function(hwnd)
    if not Window.objects[hwnd] then return end
    Window.objects[hwnd]=nil
    Window.objects.amount = Window.objects.amount - 1
    L.trace('OnWindowDestroy', hwnd, Window.objects.amount)
    return Window.objects.amount == 0
end
--------------------------------------------------------------------------------
--元素事件处理
--BUBBLING= 0,// bubbling (emersion) phase
--SINKING = 0x08000, // capture
--HANDLED = 0x10000, // event already processed
local EVENTS_META  = {
    __newindex = function() error("cmd can't modify") end,
    __doc = function(m)
        local v = bit.band(m.__cmd,0x7FFF)-- ~0x18000
        if m.type == 'Focus' then
            return "focus TODO"
        elseif m.type == 'Key' then
            return "key TODO"
        elseif m.type == 'BehaviorEvent' then
            return BEHAVIOR_EVENTS[v][3]
        end
    end,
    __tostring = function(m)
        local str = m.type
        if bit.band(m.__cmd, 0x08000) ~= 0 then str=str..'[SINKING]' end
        if bit.band(m.__cmd, 0x10000) ~= 0 then str=str..'[HANDLED]' end
        return str..': '..m.value..' >'..string.gsub(tostring(m.__target),'userdata','element')
    end,
    __index=function(m, k)
        k = string.upper(k)
        if k == 'TARGET'      then return _element_new(m.__target)
        elseif k == 'SINKING' then return bit.band(m.__cmd, 0x08000) ~= 0
        elseif k == 'HANDLED' then return bit.band(m.__cmd, 0x10000) ~= 0
        end
        local v = bit.band(m.__cmd, 0x7FFF)
        if m.type == 'Focus' then
            if k == 'LOST'    then return v == 0
            elseif k == 'GOT' then return v == 1
            elseif k =='VALUE'then return v == 0 and "LOST" or "GOT"
            end
        elseif m.type == 'Key' then
            if k == 'DOWN'     then return v == 0
            elseif k == 'UP'   then return v == 1
            elseif k == 'CHAR' then return v == 2
            elseif k == 'VALUE' then 
                local t = {"DOWN","UP","CHAR"}
                return t[v+1]
            end
        elseif m.type == 'BehaviorEvent' then
            local info = U.BEHAVIOR_EVENTS[v]
            if k == 'VALUE' then return info[1]
            elseif k == 'NAME' then return info[2]
            else return k == info[1]
            end
        end
    end
}
--------------------------------------------------------------------------------
C.HandleMouse = function(...) return false end

C.HandleAttached = function(source)
    source = _element_new(source)
    local f= source.__prototype.HandleAttached
    if f then f(source) end
end
C.HandleDetached = function(source)
    source = _element_new(source)
    local f= source.__prototype.HandleDetached
    if f then f(source) end
end
--CSS!脚本的全局函数
C.HandleScriptCall = function(he, cmd, ...)
    local r, wnd = C.GetElementHwnd(he, true)
    assert(r == 0 and wnd)
    wnd = Window.objects[wnd]
    r = wnd[cmd]
    if type(r) == 'function' then r = r(...) end
    return r;
end
local HandleEvent = function(event, source, target, cmd, ...)
    local on,f,behavior
    if event == 'BehaviorEvent' then
        behavior = bit.band(cmd, 0x7FFF)
        local info = U.BEHAVIOR_EVENTS[behavior]
        if not info then return false end
        on = info[2]
    else
        on = 'on'..event
    end
    source = _element_new(source)
    --BUBBLING
    if on and (not cmd or bit.band(cmd,0x18000) == 0) then
        f = rawget(source,on) or source.attribute[on]
        if f then cmd = nil end
    end
    if not f then f = source.__prototype['Handle'..event] end
    if type(f) == 'function' then
        if not cmd then return xpcall(f, C.OnError, source, ...) end
        local ret, handled = xpcall(f, C.OnError, source,
        setmetatable({__target=target, __cmd=cmd, type=event}, EVENTS_META), ...)
        if ret then return handled end
    else
        if f then C.OnError(event, '\n--', f, tostring(source)) end
        -- HYPERLINK_CLICK 阻止内置处理,修复hyperlink::try_to_load处理(方式与内置相同)
        if behavior == 0x80 then return true end
    end
    return false
end
C.HandleFocus = function(source, target, cmd, ...)
    return HandleEvent('Focus', source, target, cmd, ...)
end
C.HandleKey = function(source, target, cmd, ...)
    return HandleEvent('Key', source, target, cmd, ...)
end
C.HandleSize = function(source, ...)
    return HandleEvent('Size', source, nil, nil, ...) 
end
C.HandleBehaviorEvent = function(source, target, cmd, ...)
    return HandleEvent('BehaviorEvent', source, target, cmd, ...)
end
--------------------------------------------------------------------------------
--内部通知
--------------------------------------------------------------------------------
--当页面加载完成之后调用
--root  根元素指针
--...   _window_open()中加载时额外的参数
C.OnDocumentLoaded = function(root, hwnd, url, ...)
    L.trace('OnDocLoaded', url)
    local env = _window_new(hwnd)
    root = _element_new(root)
    assert(root.__hwnd == hwnd)
    local head = root:query("head")

    if head then
        --设置标题
        local title = head:query("title")
        if title then root.text = title.text end
        --设置程序图标
        local icon = head:query("link[rel='icon']")
        if icon then _element_call(root,'SetElementInnerImage', root:url(icon.attribute.href)) end
    end

    --执行脚本
    for _, v in ipairs(root:query("script[type='text/lua']",{})) do
        loadHtmlScript(env, v, ...)
    end

    for _, v in ipairs(root:query('frame', {})) do
        loadFrameScript(env, v, ...)
    end

    --加载所有原型元素
    root:query('[prototype]',{});
end
C.OnDocumentComplete = function(he, hwnd, url, ...)
    L.trace('OnDocComplete', url)
    do
        local root = _element_new(he)
        local f = root.onReady
        if type(f) == 'function' then
            xpcall(f, C.OnError, root, url, ...)
        end
    end
    collectgarbage()
end
--创建页面控件
C.OnCreateControl = function(helement, hparent)
    local r, etype = C.GetAttributeByName(helement, 'type');
    assert(r == 0)
    local wclass = WidgetType[etype]
    if not wclass then return nil end

    local hwnd,widget = wclass.CreateElement(hparent, helement)
    L.trace('OnCreateCtrl', helement, etype, hwnd)
    r = C.SetWidget(helement, widget, etype)
    assert(r == 0)
    return hwnd
end
--完成页面控件创建
C.OnControlCreated = function(helement, hparent, hwnd)
    local r, etype = C.GetAttributeByName(helement, 'type');
    assert(r == 0)
    L.trace('OnCtrlCreated', etype, hwnd)
end
--销毁页面控件
C.OnDestroyControl=function(helement, hwnd) 
    local r, etype = C.GetAttributeByName(helement, 'type');
    assert(r == 0)
    L.trace('onDestroyCtrl', etype, hwnd)
    return hwnd
end
--通知
C.OnNotify = function(element, ...)
    return HandleEvent('Notify', element, nil, nil, ...)
end
--加载数据
C.OnLoadData = function(dataType, uri, principal) 
    L.trace('onLoadData', uri) 
    --C.DataReady(hwnd, uri, "data...")
    return true --OK, or discard
end
--完成数据加载
C.OnDataLoaded= function(dataType, uri, status, data)
    L.trace('onDataLoaded', uri)
end
--接收文件拖动
C.OnDropFiles= function(element, ...)
    return HandleEvent('DropFiles', element, nil, nil, ...)
end
C.OnThreadTask = function(code,from)
    L.work(code,from)
end
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
L.open = _window_open
--注册内置控件
L.widget{
    HtmlView=C.HtmlView,
    Flash=C.Flash,
    Web=C.Web,
    Scintilla=require"htm.scintilla"
}
return L
