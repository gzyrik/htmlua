LuaEditor={}
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
    self.attribute.margin[1] = {type="number", width="_99"}
    self.attribute.margin[2]={type='symbol', width=16, mask='folders'}
    self.attribute.margin[2].sensitive=true

    --设置其他属性
    self.attribute {
        tabWidth = 4,
        dwellTime= 300,
        language ='markdown',
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
    --定义LUA语法
    self.lexer.markdown={
        fold = '1',
        keyword = {
            "param property method remarks and break do else elseif false for local if in nil not or repeat return then true until while self",
            "table.insert table.remove string.sub table.concat function require end",
        },
        style = {
            default={fore=0xFFFFFF},
            word={fore=0xFFFF, bold=true},
            number={fore=0xFF0F},
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
