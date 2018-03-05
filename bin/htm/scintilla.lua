local function loadIFace(dir, env)
    local enum,e,e_name, e_prefix={}
    local lexer,l,l_name,l_prefix={}
    local lexer2={}--重复
    local fo = {}
    ---------------------------------------------------------------------
    local fi = io.open(dir..'/Scintilla.iface', 'r')
    assert(fi,"open Scintilla.iface failed")
    for line in fi:lines() do
        if #line > 0 and line:byte(1) ~= 35 then --'#'
            local words = {}
            for w in line:gmatch("([%w_]+)") do table.insert(words, w) end
            local name = words[1]

            if name == 'cat' then     -- start a category
                table.insert(fo, string.rep('-', 80)..'\n--'..words[2])
            elseif name == 'fun' or name == 'get' or name == 'set' then
                table.insert(fo, 'SCI_'..string.upper(words[3])..'='..words[4])
            elseif name == 'evt' then -- an event
                table.insert(fo, 'SCN_'..string.upper(words[3])..'='..words[4])
            elseif name == 'enu' then -- associate an enumeration with a set of vals with a prefix
                e,e_name,e_prefix = {},words[2],words[3]
                enum[e_name]=e
                table.insert(fo, '-- enum '..words[2]..' '..words[3])
            elseif name == 'lex' then -- associate a lexer with the lexical classes it produces
                if l_prefix == words[4] then
                    local nick = lexer2[l_name] or {}
                    table.insert(nick, l_name)
                    lexer2[l_name]=nil
                    lexer[l_name]=nil
                    l_name = words[2]
                    lexer2[l_name] = nick
                else
                    l,l_name,l_prefix = {},words[2],words[4]
                end
                lexer[l_name]=l
                table.insert(fo, '-- lexer '..words[2]..' '..words[4])
            elseif name == 'val' then -- definition of a constant
                table.insert(fo, words[2]..'='..words[3])
                if e_prefix and string.sub(words[2],1,#e_prefix) == e_prefix then
                    e[string.sub(words[2], #e_prefix+1)] = words[3]
                elseif l_prefix and string.sub(words[2],1,#l_prefix) == l_prefix then
                    l[string.sub(words[2], #l_prefix+1)] = words[3]
                end
            end
        end
    end
    fi:close()
    ---------------------------------------------------------------------
    table.insert(fo, string.rep('-', 80)..'\n-- enumeration')
    table.insert(fo,'__ENUM={')
    for n, e in pairs(enum) do
        table.insert(fo, n..'={')
        for k,v in pairs(e) do
            table.insert(fo, '\t["'..k..'"]='..v..',')
        end
        table.insert(fo, '},')
    end
    table.insert(fo, '}')
    table.insert(fo, string.rep('-', 80)..'\n-- lexical')
    table.insert(fo, '__LEXER={')
    for n, l in pairs(lexer) do
        table.insert(fo, string.upper(n)..'={')
        for k,v in pairs(l) do
            table.insert(fo, '\t["'..k..'"]='..v..',')
        end
        table.insert(fo, '},')
    end
    table.insert(fo, '}')
    for n, l in pairs(lexer2) do
        for _,v in ipairs(l) do
            table.insert(fo, '__LEXER.'..string.upper(v)..'=__LEXER.'..string.upper(n))
        end
    end
    ---------------------------------------------------------------------
    fo = table.concat(fo,'\n')
    local iface={}
    local f, err=loadstring(fo)--, "Scintilla.iface",'bt', iface)
    if not f then
        fi = io.open(dir..'/scintilla.iface.lua', 'w+')
        fi:write(fo)
        fi:close()
        error(err)
    end
    f=setfenv(f, iface)
    f()
    return setmetatable(iface, {__index=env})
end
---------------------------------------------------------------------
local U=require"htm.utils"
local C=require"htm.htmlua".SciLexer
setfenv(1,loadIFace(U.mydir(), _G)) --加载常量环境
---------------------------------------------------------------------
local L=C
L.__method = {}
local PROP1 = function (set, get, p, r)
    return function(e, k, v)
        if v then
            assert(set, 'readonly property')
            if p then v = p(v,e) end
            C.SendMsg(e, set, k, v)
        else
            assert(get, 'writeonly property')
            v = C.SendMsg(e, get, k)
            if r then v = r(v,e) end
            return v
        end
    end
end
local PROP0 = function (set, get, p, r)
    return function(e, v)
        if v then
            assert(set, 'readonly property')
            if p then v = p(v,e) end
            C.SendMsg(e, set, v)
        else
            assert(get, 'writeonly property')
            v = C.SendMsg(e, get)
            if r then v = r(v,e) end
            return v
        end
    end
end

local SETP1 = function(set, p) return PROP1(set, nil, p) end
local GETP1 = function(get, p) return PROP1(nil, get, nil, p) end
local SETP0 = function(set, p) return PROP0(set, nil, p) end
local GETP0 = function(get, p) return PROP0(nil, get, nil, p) end
local DOC=U.DOC
local CLASS=U.CLASS
---------------------------------------------------------------------
local Marker=CLASS("标记")
Marker.assert=function(marker)
    local num = U.toenum(marker, __ENUM.MarkerOutline)
    assert(num and 0 <= num and num <= MARKER_MAX, "非法标记号'"..marker.."',有效范围[0,MARKER_MAX("..MARKER_MAX..")]")
    return num
end
Marker.__property = {
    pixmap= DOC[[设置标志对应对应的图形]](SETP1(SCI_MARKERDEFINEPIXMAP)),
    fore = DOC[[设置标志的前景色]](SETP1(SCI_MARKERSETFORE)),
    back = DOC[[设置标志的背景色]](SETP1(SCI_MARKERSETBACK)),
    define = DOC[[设置标志为内置图形]](SETP1(SCI_MARKERDEFINE, function(symbol)
        return U.toenum(symbol, __ENUM.MarkerSymbol)
    end)),
}
---------------------------------------------------------------------
local Margin=CLASS("边注")
Margin.assert = function(margin)
    assert(0<= margin and margin <= SC_MAX_MARGIN, "非法边注号'"..margin.."',有效范围[0,SC_MAX_MARGIN("..SC_MAX_MARGIN..")]")
    return margin
end
Margin.__property = {
    sensitive= DOC[[设置边注是否响应鼠标]](PROP1(SCI_SETMARGINSENSITIVEN, SCI_GETMARGINSENSITIVEN)),
    cursor   = DOC[[设置边注的鼠标]](PROP1(SCI_SETMARGINCURSORN, SCI_GETMARGINCURSORN)),
    text     = DOC[[设置边注的文本]](PROP1(SCI_MARGINSETTEXT, SCI_MARGINGETTEXT)),

    type = DOC[[边注类型, 类型可选值‘symbol','number','back','fore','text','rtext'
    ]](PROP1(SCI_SETMARGINTYPEN, SCI_GETMARGINTYPEN,function(type)
        return U.toenum(type, __ENUM.MarginType)
    end)),

    width = DOC[[设置边注宽度,若string则为字符串宽度
    ]](PROP1(SCI_SETMARGINWIDTHN, SCI_GETMARGINWIDTHN, function(width, e)
        return type(width) == 'string' and C.SendMsg(e, SCI_TEXTWIDTH, STYLE_LINENUMBER, width) or width
    end)),

    mask = DOC[[设置边注掩码,与行marker运算决定是否显示该标志,'folders'用于折叠
    ]](PROP1(SCI_SETMARGINMASKN, SCI_GETMARGINMASKN, function(mask)
        return mask == 'folders' and SC_MASK_FOLDERS or mask
    end)),
}
---------------------------------------------------------------------
--[[定制 文法样式
支持keyword,style,其他作为通常属性,例如:
element.lexer.lua={
        fold = '1',
        keyword = {
            "function if then and "
            "os string",
        },
        style = {
            word ={fore=0xFF0000},
            comment={fore=0xff00},
        },
}
其中 word,comment为样式ID,按照样式语法赋值
]]
local Lexer={}
Lexer.apply = function(element, name, props)
    local lang = __LEXER[string.upper(name)] --样式值名称与数值对应表
    assert(lang, name .. ' language not defined')
    for key, val in pairs(props) do
        if key == 'keyword' then
            for k, v in ipairs(val) do Lexer.SetKeyWords(element, k-1, v) end
        elseif key == 'style' then
            for k, v in pairs(val) do L.SetStyleAttribute(element, lang[string.upper(k)], v) end
        else
            Lexer.SetProperty(element, key, val)
        end
    end
    return 0
end
Lexer.__newindex = function(lexer, key, props)
    assert(type(props.keyword)=='table' and type(props.style)=='table', key .." 语法必须包含 'keyword' 和 'style' 的字义")
    local element = rawget(lexer, '__element')
    local lang = C.RecvMsg(element, SCI_GETLEXERLANGUAGE)
    element.attribute['scintilla_lexer_'..key] = table.tojson(props)
    --更新当前语法
    if lang == key then Lexer.apply(element, lang, props) end
end
Lexer.SetLanguage=DOC[[设置语法
]](function(element, language)
    C.SendMsg(element, SCI_SETLEXERLANGUAGE, 0, language)
    local json = element.attribute['scintilla_lexer_'..language]
    if json and #json > 0 then Lexer.apply(element, language, table.json(json)) end
    return 0
end)
Lexer.SetKeyWords = DOC[[设置词法分析器的关键字(必须先设置语法)
@param keywordSet  [0,KEYWORDSET_MAX(8)]
]](function(element, keywordSet, keyWords)
    assert(0<= keywordSet and keywordSet <= KEYWORDSET_MAX, "关键字号[0,KEYWORDSET_MAX(8)]")
    C.SendMsg(element, SCI_SETKEYWORDS, keywordSet, keyWords);
end)
Lexer.SetProperty = DOC[[设置词法分析器的可选属性(必须先设置语法)
]](function(element, key, val)
    return C.SendMsg(element, SCI_SETPROPERTY, key, val)
end)
---------------------------------------------------------------------
--其他属性
local __CODEPAGE_VALUES={
    UTF8=SC_CP_UTF8,
    GBK=936,
    BIG5=950,
    SJIS=932,
}
L.__attribute={
    language = DOC[[语法]](function(e, v)
        return v and Lexer.SetLanguage(e, v) or C.RecvMsg(e, SCI_GETLEXERLANGUAGE)
    end),
    autoFold = DOC[[]](PROP0(SCI_SETAUTOMATICFOLD, SCI_GETAUTOMATICFOLD, function(mask)
        return U.tomask(mask, __ENUM.AutomaticFold) end)),
    codePage = DOC[[]](PROP0(SCI_SETCODEPAGE, SCI_GETCODEPAGE, function(codepage)
        return U.toenum(codepage, __CODEPAGE_VALUES) end)),

    foldFlags = DOC[[设置自动折叠, 可选值'show' 'click' 'change'的组合
    ]](SETP0(SCI_SETFOLDFLAGS,function(flags) return U.tomask(flags, __ENUM.FoldFlag) end)),

    tabWidth = DOC[[tab键宽度]](PROP0(SCI_SETTABWIDTH, SCI_GETTABWIDTH)),
    dwellTime = DOC[['DWELLSTART'事件触发的鼠标停留时间]](PROP0(SCI_SETMOUSEDWELLTIME, SCI_GETMOUSEDWELLTIME)),
    readOnly = DOC[[]](PROP0(SCI_SETREADONLY, SCI_GETREADONLY)),
    margin = DOC[[批量设置边注属性]](function(e,v) assert(type(v)=='table') e.attribute.margin(v) end),
    marker = DOC[[批量设置标记属性]](function(e,v) assert(type(v)=='table') e.attribute.marker(v) end),
}
L.SetAttributeByName = function(element, key, val, super)
    local f = L.__attribute[key]
    if f then
        return 0, f(element, val)
    elseif super then
        return super(element, key, val) --表示未处理
    end
end
L.GetAttributeByName = function(element, key, super)
    local f = L.__attribute[key]
    if f then
        return 0, f(element)
    elseif super then
        return super(element, key)
    end
end
---------------------------------------------------------------------
L.styleSetUnderline= DOC[[设置样式的下划线
@param styleNumber 样式号[0,STYLE_MAX(255)]或预设名称
@param underline 是否下划线
]](function(element, styleNumber, underline)
    styleNumber = U.toenum(styleNumber, __ENUM.StylesCommon, 0, STYLE_MAX)
    C.SendMsg(element, SCI_STYLESETUNDERLINE, styleNumber, underline and 1 or 0)
end)
L.styleSetFont = DOC[[设置样式的字体
@param styleNumber 样式号[0,STYLE_MAX(255)]或预设名称
]](function(element, styleNumber, fontName)
    styleNumber = U.toenum(styleNumber, __ENUM.StylesCommon, 0, STYLE_MAX)
    C.SendMsg(element, SCI_STYLESETFONT, styleNumber, fontName)
end)
L.styleSetFore = DOC[[设置样式的前景色
@param styleNumber 样式号[0,STYLE_MAX(255)]或预设名称
]](function(element, styleNumber, color)
    styleNumber = U.toenum(styleNumber, __ENUM.StylesCommon, 0, STYLE_MAX)
    C.SendMsg(element, SCI_STYLESETFORE, styleNumber, color)
end)
L.styleSetBack = DOC[[设置样式的背景色
@param styleNumber 样式号[0,STYLE_MAX(255)]或预设名称
]](function(element, styleNumber, color)
    styleNumber = U.toenum(styleNumber, __ENUM.StylesCommon, 0, STYLE_MAX)
    C.SendMsg(element, SCI_STYLESETBACK, styleNumber, color)
end)
L.styleSetSize = DOC[[设置样式的字体大小
@param styleNumber 样式号[0,STYLE_MAX(255)]或预设名称
]](function(element, styleNumber, sizeInPoints)
    styleNumber = U.toenum(styleNumber, __ENUM.StylesCommon, 0, STYLE_MAX)
    C.SendMsg(element, SCI_STYLESETSIZE,  styleNumber, sizeInPoints)
end)
L.styleSetBold = DOC[[设置样式的粗体
@param styleNumber 样式号[0,STYLE_MAX(255)]或预设名称
]](function(element, styleNumber, bold)
    styleNumber = U.toenum(styleNumber, __ENUM.StylesCommon, 0, STYLE_MAX)
    C.SendMsg(element, SCI_STYLESETBOLD, styleNumber, bold and 1 or 0)
end)
L.styleSetItalic = DOC[[设置样式的斜体
@param styleNumber 样式号[0,STYLE_MAX(255)]或预设名称
]](function(element, styleNumber, italic)
    styleNumber = U.toenum(styleNumber, __ENUM.StylesCommon, 0, STYLE_MAX)
    C.SendMsg(element, SCI_STYLESETITALIC, styleNumber, italic and 1 or 0)
end)
L.styleSetWeight= DOC[[设置样式的字体粗细
bold 等价于设置weight为’normal' 或 'bold‘
@param styleNumber 样式号[0,STYLE_MAX(255)]或预设名称
@param weight 粗细,数值[1, 999]或预设名称
]](function(element, styleNumber, weight)
    styleNumber = U.toenum(styleNumber, __ENUM.StylesCommon, 0, STYLE_MAX)
    weight = U.toenum(weight, __ENUM.FontWeight, 1, 999)
    C.SendMsg(element, SCI_STYLESETWEIGHT, styleNumber, weight)
end)
L.styleSetVisible = DOC[[设置样式的可见性
@param styleNumber 样式号[0,STYLE_MAX(255)]或预设名称
@param visible     是否可见
]](function(element, styleNumber, visible)
    styleNumber = U.toenum(styleNumber, __ENUM.StylesCommon, 0, STYLE_MAX)
    C.SendMsg(element, SCI_STYLESETVISIBLE, styleNumber, visible and 1 or 0)
end)
L.styleSetCase = DOC[[设置样式的大小写
@param styleNumber 样式号[0,STYLE_MAX(255)]或预设名称
@param visible     是否可见
]](function(element, styleNumber, case)
    styleNumber = U.toenum(styleNumber, __ENUM.StylesCommon, 0, STYLE_MAX)
    case = U.toenum(case, __ENUM.CaseVisible, SC_CASE_MIXED, SC_CASE_CAMEL)
    C.SendMsg(element, SCI_STYLESETCASE, styleNumber, case)
end)
L.styleSetEolfilled = DOC[[设置样式的右边距是否按行尾背景填充
@param styleNumber 样式号[0,STYLE_MAX(255)]或预设名称
@param eolFilled     行尾背景填充
]](function(element, styleNumber, eolFilled)
    styleNumber = U.toenum(styleNumber, __ENUM.StylesCommon, 0, STYLE_MAX)
    C.SendMsg(element, SCI_STYLESETEOLFILLED, styleNumber, eolFilled and 1 or 0)
end)
L.styleSetHotspot = DOC[[设置样式的类似超链接感应
@param styleNumber 样式号[0,STYLE_MAX(255)]或预设名称
@param hotspot     是否感应
]](function(element, styleNumber, hotspot)
    styleNumber = U.toenum(styleNumber, __ENUM.StylesCommon, 0, STYLE_MAX)
    C.SendMsg(element, SCI_STYLESETHOTSPOT, styleNumber, hotspot and 1 or 0)
end)

--样式名与设置函数的对应表
local __STYLE_SETS={
    fore = L.styleSetFore,
    back = L.styleSetBack,
    font = L.styleSetFont,
    size = L.styleSetSize,
    bold = L.styleSetBold,
    underline = L.styleSetUnderline,
    italic = L.styleSetItalic,
    weight = L.styleSetWeight,
    visible = L.styleSetVisible,
    case = L.styleSetCase,
    eolfilled = L.styleSetEolfilled,
    hotspot = L.styleSetHotspot,
}
--[[设置样式属性
支持如下调用
editor.style.callTip = {
font="Courier New",
size=10,
bold=true,
}]]
L.SetStyleAttribute = function(element, key, val)
    if type(val) == 'table' then
        for k, v in pairs(val) do __STYLE_SETS[k](element, key, v) end
    elseif key == 'caretLineVisible' then
        C.SendMsg(element, SCI_SETCARETLINEVISIBLE, val)
    elseif key == 'caretLineBack' then
        C.SendMsg(element, SCI_SETCARETLINEBACK, val)
    end
    return 0
end
---------------------------------------------------------------------
--设置内部文本
L.SetElementInnerText = function(element, txt)
    C.SendMsg(element, SCI_SETTEXT,  0, txt or '')
    return 0
end 
L.ControlSetValue = L.SetElementInnerText
---------------------------------------------------------------------
--运行时属性(相对于attribute是可序列化的)
L.__property = {
    position  = DOC[[光标位置]](PROP0(SCI_SETCURRENTPOS, SCI_GETCURRENTPOS)),
    status    = DOC[[运行状态]](GETP0(SCI_GETSTATUS)),
    length    = DOC[[字节数]](GETP0(SCI_GETLENGTH)),
    modify    = DOC[[是否已修改]](GETP0(SCI_GETMODIFY)),
    zoom      = DOC[[-10到+20点范围内的缩放系数]](PROP0(SCI_SETZOOM, SCI_GETZOOM)),
}
L.__method.styleClearAll = DOC[[复位所有样式为default样式
@param[opt] styles 修改default样式
]](function(element, styles)
    if styles then L.SetStyleAttribute(element, STYLE_DEFAULT, styles) end
    return C.SendMsg(element, SCI_STYLECLEARALL)
end)
L.__method.styleResetDefault = DOC[[复位default样式
]](function(element)
    return C.SendMsg(element, SCI_STYLERESETDEFAULT)
end)
L.__method.autocShow = DOC[[弹出自动补全窗口
@param list 自动补全列表,以空格分割
@param[opt] entered 已经输入的位置
]](function(element, list, entered)
    return C.SendMsg(element, SCI_AUTOCSHOW, entered or 0, list)
end)

L.__method.tipShow = DOC[[弹出提示窗口
@param pos 字符位置
@param tip 提示文本
]](function(element, pos, tip)
    return C.SendMsg(element, SCI_CALLTIPSHOW, pos, tip);
end)

L.__method.tipCancel=DOC[[撤消提示窗口
]](function(element)
    return C.SendMsg(element, SCI_CALLTIPCANCEL);
end)
L.__method.wordAt=DOC[[获取单词
@param pos 单词位置
@param onlyWordCharacters 
@return 单词,起始位置,结束位置
]](function(element, pos, onlyWordCharacters)
    return C.GetTextRange(element, pos, onlyWordCharacters ~= nil)
end)
L.__method.textRange=DOC[[获取文本
@param startPos 起始位置
@param endPos   结束位置
@return 文本,起始位置,结束位置
]](function(element, startPos, endPos)
    return C.GetTextRange(element, startPos or 0, endPos or -1)
end)

L.__method.addText = DOC[[当前位置插入文件
@param str 字符串
]](function(element, str)
    C.SendMsg(element, SCI_ADDTEXT, #str, str)
end)
L.__method.appendText = DOC[[末尾添加文件
@param str 字符串
]](function(element, str)
    C.SendMsg(element, SCI_APPENDTEXT, #str, str)
end)
L.__method.insertText = DOC[[指定位置插入文件
@param pos 位置
@param str 字符串
]](function(element, pos, str)
    C.SendMsg(SCI_INSERTTEXT, pos, str)
end)
L.__method.gotoLine = DOC[[光标跳到指定行
@param line 行号,从0开始
]](function(element, line)
    C.SendMsg(SCI_GOTOLINE, line)
end)
L.__method.gotoPos = DOC[[光标跳到指定位置
@param pos 字符位置,从0开始
]](function(element, pos)
    C.SendMsg(SCI_GOTOPOS, line)
end)
L.__method.markerAdd = DOC[[添加行的标志
@param line 行号,从0开始
@param markerNumber 标志值[0,MARKER_MAX(31)]
]](function(element, line, markerNumber)
    markerNumber = Marker.assert(markerNumber)
    C.SendMsg(element, SCI_MARKERADD, line, markerNumber)
end)
L.__method.markerAddMask = DOC[[批量添加行的多个标志
@param line 行号,从0开始
@param markerMask 标志掩码
]](function(element, line, markerMask)
    C.SendMsg(element, SCI_MARKERADDSET, line, markerMask)
end)
local doc=[[Scintilla包含了一个“缩放因子”,
允许您使文档中的所有文本以一个点的步长变大或变小.
显示的点尺寸不会低于2,无论您设置什么缩放系数.
您可以设置-10到+20点范围内的缩放系数]]
L.__method.zoomIn = DOC("放大\n"..doc)(function(element) C.SendMsg(element, SCI_ZOOMIN) end)
L.__method.zoomOut= DOC("缩小\n"..doc)(function(element) C.SendMsg(element, SCI_ZOOMIN) end)

local UseElement = C.UseElement
L.UseElement = function(element)
    rawset(element, 'lexer', setmetatable({__element=element}, Lexer))
    rawset(element.attribute, 'marker', setmetatable({__element=element}, Marker))
    rawset(element.attribute, 'margin', setmetatable({__element=element}, Margin))
    return UseElement(element)
end
return L
