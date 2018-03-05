function myprint(...)
    print('myprint', ...)
end
-----------------------------------------------------------------
LuaEditor={}
local function autocshow(editor, onlyfunc)
    local obj, startPos, endPos = editor:wordAt(editor.position - 2)
    local f,msg = obj and load('return '..obj, 'autocshow', 'bt', window)
    f = f and f()
    if type(f) ~='table' then return false end
    local list={}
    if onlyfunc then
        for k,_ in pairs(f) do
            if type(_) == 'function' then table.insert(list, k) end
        end
    else
        for k,_ in pairs(f) do table.insert(list, k) end
    end
    if #list > 0 then return editor:autocShow(table.concat(list, ' ')) end
    return false
end

-----------------------------------------------------------------
--内部相应控件的Notify事件
function LuaEditor:HandleNotify(code, ...)
    code = string.upper(code)
    if code == "CHARADDED" then
        local ch = select(1, ...)
        if ch == 46 then --'.'
            autocshow(self)
        elseif ch == 58 then --':'
            autocshow(self, true)
        elseif ch == 40 then -- '('
        end
        return true
    elseif code == "DWELLSTART" then
        local pos = select(1, ...)
        return self:tipShow(pos, "xxxx")
    elseif code == "DWELLEND" then
        return self:tipCancel()
    end
end

-----------------------------------------------------------------
--对应实例销毁时,清理工作,可能不被调用
function LuaEditor:HandleDetached()
    print("LuaEditor HandleDetached")
end

-----------------------------------------------------------------
function LuaEditor:HandleFocus(cmd, by_mouse_click, cancel)
    print("LuaEditor HandleFocus", cmd)
    return true
end
-----------------------------------------------------------------
function LuaEditor:HandleDropFiles(file1,...)
    print("LuaEditor HandleDropFiles", file1,...)
    return true
end
-----------------------------------------------------------------
--对应实例初始化
function LuaEditor:HandleAttached()
    local dir = self:url():match('file://(.*)')
    --配置scintilla,设置全局默认样式
    self:styleClearAll{
        fore=0xffffff,
        back=0x270009,
        font="Courier New",
        size=12
    }
    --设置其他样式
    self.style.braceLight = {
        fore=0xff,
        back=0x270009,
        font="Courier New",
        size=10
    }
    self.style {
        linenumber = {
            fore=0xff0000,
            back=0xffffff,
            font="Courier New",
            size=10,
        },
        --caretLineVisible = true,
        --caretLineBack = 0xb0ffff,
    }
    --设置折叠样式
    local plus_xpm=io.open(dir..'images/plus.xpm'):read('*a')
    local minus_xpm=io.open(dir..'images/minus.xpm'):read('*a')
    self.attribute.marker{
        folder={pixmap=plus_xpm},
        folderopen={pixmap=minus_xpm},
        folderend={pixmap=plus_xpm},
        folderopenmid={pixmap=minus_xpm},
        foldermidtail={define='tcornercurve', back=0xa0a0a0},
        foldersub={define='vline', back=0xa0a0a0,},
    }
    self.attribute.marker.foldersub.fore=0xff

    --设置边注样式
    self.attribute.margin{
        {type="number", width="_99"},
    }
    self.attribute.margin[2]={type='symbol', width=16, mask='folders'}
    self.attribute.margin[2].sensitive=true

    --设置其他属性
    self.attribute {
        tabWidth = 4,
        dwellTime= 300,
        language ='lua',
        codePage ='utf8',
        readOnly = false,
        autoFold = 'show click change',
    }
    --定义HTML语法
    self.lexer.hypertext={
        fold = '1',
        keyword={
            [[b body content head href html link meta
            name rel script strong title type xmlns]],
            "function",
            "sub"
        },
        style={
            tag ={fore=0xfff9,bold=true},
            tagEnd={fore=0xff},
            attribute={fore=0xfffb05,bold=true},
            value={fore=0xff},
            doubleString={fore=0x9595ff},
            singleString={fore=0x9595ff},
        }
    }
    --定义C++语法
    self.lexer.cpp={
        fold = '1',
        keyword={
            [[asm auto break case catch class const
            const_cast continue default delete do double
            dynamic_cast else enum explicit extern false for 
            friend goto if inline mutable
            namespace new operator private protected public
            register reinterpret_cast return signed
            sizeof static static_cast struct switch template
            this throw true try typedef typeid typename
            union unsigned using virtual volatile while
            html body head meta script title window document]],

            [[bool char float int long short void wchar_t
            b body content head href html link meta 
            name rel script strong title type xmlns]],
        },
        style={
            word ={fore=0xFF0000},
            word2 ={fore=0x800080, bold=true},
            string={fore=0x1515A3},
            character={fore=0x1515A3},
            preprocessor={fore=0x808080},
            comment={fore=0xff00},
            commentline={fore=0xff00},
            commentdoc={fore=0xff00},
        }
    }
    --定义LUA语法
    self.lexer.lua={
        fold = '1',
        keyword = {
            "and break do else elseif false for local if in nil not or repeat return then true until while self",
            "table.insert table.remove string.sub table.concat function require end",
        },
        style = {
            default={fore=0x0},
            word={fore=0xFFFF, bold=true},
            number={fore=0xFF},
            string={fore=0xFF00FF},
            character={fore=0xFF00FF},
            literalstring={fore=0xFF00FF},
            stringeol={fore=0xFF00FF},
            comment={fore=0x008000},
            commentline={fore=0x008000},
            word2={fore=0xF7FF00},
            operator= {fore=0x7FFF00},
        },
    }
end
