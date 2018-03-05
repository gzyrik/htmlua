local lfs=require'lfs'

--将目录对应为中文
local i18n={
    ['MSVS-start-page']= 'MSVS 起始页效果',
    ['horizontal-vertical-align']='水平/垂直对齐',
    ['drag-n-drop']='拖放',
    ['fixed-table']='table表格',
    ['animated-png']='PNG动画',
    ['@font-face']='自定义字体',
    ['@image-map']='自适应图像',
    ['border-radius']='圆角边框',
    ['csss!']='csss!脚本',
    ['css-plus']='CSS扩展',
    abs='绝对定位',
    back='背景',
    generic='入门',
    goodies='推荐',
    effects='特效',
    fisheye='鱼眼特效',
    svg='svg绘图',
    stress = '压力测试',
    printing= '打印',
    includes='include语法',
    optimizations='优化',
    grid='网格(grid)',
    frames='框架应用',
    flows='布局模式',
    richtext='richtext控件',
    editor='编辑器',
    tooltips='tooltip提示',
    menu='菜单',
    forms='窗口控件',
    communication='提交表单',
    behaviors='交互扩展(behaviors)',
    transitions='渐变效果(transition)',
    animations='动画',
    syntax='语法高亮',
    style='样式',
    margin='边注',
}

--扫描目录中的htm文件,返回HTML列表
local function _pathtml(rootpath)
    local htmls={}
    local i=1
    for entry in lfs.dir(rootpath) do
        if entry ~= '.' and entry ~= '..' then
            local path = rootpath ..'/'.. entry
            local attr,err = lfs.attributes(path)
            assert(type(attr) == 'table',err)

            if attr.mode == 'directory' then
                local subs= _pathtml(path)
                if subs and #subs > 0 then
                    subs = (i18n[entry] or entry) .. subs
                    table.insert(htmls, '<options .node>'..subs..'</options>')
                end
            else
                if string.match(entry,'^.*%.htm$') then
                    table.insert(htmls, i, '<option path="'..path..'">'..entry..'</option>')
                    i=i+1
                end
            end
        end
    end
    return table.concat(htmls, '\n');
end
local function pathtml(rootpath)
    return [[<widget .functree type='select'>]].._pathtml(rootpath)..[[</widget>]]
end

--获取HTML元素对象
local samples=document:query('#samples')
local onlines=document:query('#onlines')
local scintillas=document:query('#scintillas')
local flashes=document:query('#flashes')
local tests=document:query('#tests')
local prview=document:query('#prview') --效果预览控件
local editor=document:query('#editor') --编辑源码控件
local file
--选择文件项
local function onPreview(path)
    if path and path ~= file then 
        file = path
        prview.value = 'file://'..file
        editor.text = io.open(file):read('*a')
    end
end
--树型类的prototype
ftree = {}
function ftree:HandleBehaviorEvent(event)
    local item = self:query(':checked')
    if item then onPreview(item.attribute.path) end
end
--样例文件所在目录
local dir = samples:url():match('file://(.*)')
samples.innerHTML=pathtml(dir..'html_sample')
onlines.innerHTML=pathtml(dir..'html_online')
scintillas.innerHTML=pathtml(dir..'html_scintilla')
flashes.innerHTML=pathtml(dir..'html_flash')
tests.innerHTML=pathtml(dir..'html_tests')
--设置初始页面
file = dir..'html_sample/index.htm'
prview.value = 'file://'..file
editor.text = io.open(file):read('*a')
-----------------------------------------------------------------
--配置scintilla,设置全局默认样式
editor:styleClearAll{
    fore=0xffffff,
    back=0x270009,
    font="Courier New",
    size=12
}
--设置其他样式
editor.style.braceLight = {
    fore=0xff,
    back=0x270009,
    font="Courier New",
    size=10
}
editor.style {
    linenumber = {
        fore=0xff0000,
        back=0xffffff,
        font="Courier New",
        size=10,
    },
    --caretLineVisible = true,
    --caretLineBack = 0xb0ffff,
    tabWidth= 4,
}
local plus_xpm=io.open(dir..'images/plus.xpm'):read('*a')
local minus_xpm=io.open(dir..'images/minus.xpm'):read('*a')
--设置语法和折叠
editor.attribute {
    language = 'lua',
    codePage = 'utf8',
    autoFold = 'show click change',
    --定义边注
    margin = {
        {type="number", width="_99"},
        {type='symbol', width=16, mask='folders', sensitive=true},
    },
    --定义折叠样式
    marker = {
        folder={pixmap=plus_xpm},
        folderopen={pixmap=minus_xpm},
        folderend={pixmap=plus_xpm},
        folderopenmid={pixmap=minus_xpm},
        foldermidtail={define='tcornercurve', back=0xa0a0a0},
        foldersub={define='vline', back=0xa0a0a0,},
        foldertail={define='lcornercurve'},
    },
}
--定义HTML语法
editor.lexer.hypertext={
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
editor.lexer.cpp={
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
editor.lexer.lua={
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
