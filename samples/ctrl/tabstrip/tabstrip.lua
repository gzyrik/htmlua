--tab页
TabStrip = {};

--msg
--切换tab时的消息
function TabStrip:onTabSwitch(new, old)
end

--删除tab时的消息
function TabStrip:onTabDelete(tab) 
end
--

function TabStrip:addTab(label, filename, icon_url)
    local tabs = self:query('widget.tabs')
    local tab =  tabs:query(string.format("option[value2='%s']", filename))
    local created = nil
    if tab == nil then 
        tab = tabs:insert('option', label, 0)
        tab.style{
            ["background-image"] = icon_url,
            ["background-repeat"] ="no-repeat",
            ["background-position"]="2px 60%",
        }
        tab.attribute.value2 = filename
        tab.attribute.title = filename
        created = true
    end

    self:switchTab(tab)
    self:HandleSize()
    return tab, created 
end


function TabStrip:switchTab(tab)
    local prevCurrent = self:query("widget.tabs>option:current");
    if prevCurrent then
        if  prevCurrent == tab  then return end
        prevCurrent:state('~current')
    end

    local ctl_close = self:query("widget.ctl.close")
    if not tab  then
        ctl_close.attribute.disabled = 'true'
    else
        tab:state('+current')
        ctl_close.attribute.disabled = nil 
        self:onTabSwitch(tab, prevCurrent); -- notify observers.
    end
end

function TabStrip:setCurrent(fn)
    local  tabs = self:query('widget.tabs');
    local  tab =  tabs:query(string.format("option[value2='%s']", fn));
    if tab == nil  then return end;
    self:switchTab(tab);
end
function TabStrip:closeTab(tab)
    local tabs = self:query("widget.tabs");
    local delindex = -1;
    for i = 1, #tabs do
        if tab == tabs[i] then
            self:onTabDelete(tab);
            delindex = i;
            tab:destroy();
            break;
        end
    end

    tabs:update();
    if #tabs > 0 then		
        delindex = math.min(#tabs, delindex);
        self:switchTab(tabs[delindex]);
        self:HandleSize();	
    else
        self:switchTab(nil);
        self:update();
    end
end

function TabStrip:changeLable(label)
    local  tab = self:query("widget.tabs>option:current");
    if tab == nil then return end;
    tab.innerHTML = label;
    tab.attribute.title = label;
end
-------------------------------------------------------------------------------------
--处理与元素绑定事件
--@param self 当前元素
function TabStrip:HandleAttached()	
    self.innerHTML = [[<widget.tabs ></widget>
    <widget.ctl.off-strip title='活动文件' style="display:none;">7<menu.popup /></widget>
    <widget.ctl.close disabled title='关闭当前页'>r</widget>
    ]];
end
--处理元素尺寸改变事件
--@param self 当前元素
function TabStrip:HandleSize()
    local combo = self:query("widget.off-strip"); --下拉按钮
    local menu = combo:query("menu"); --菜单列表
    local tabs = self:query("widget.tabs");

    --将tab移到下拉框中
    function moveToOffstrip(tab)
        if #menu == 0 then
            combo.style.display = 'block';
        end
        local h = menu:clone(tab, 1);
        h.tab = tab;
        tab.style.visibility = 'hidden';
    end

    --将tab从下拉表中移除
    function moveFromOffstrip(tab)
        for i = 1, #menu do
            local offtab = menu[i];		
            if offtab.tab == tab then			
                offtab:destroy();
                break;
            end
        end
        if #menu == 0 then			
            combo.style.display = 'none';
        end
    end

    local tabs_x,tabs_y,tabs_r,tabs_b = tabs:place()
    for i = 1, #tabs do
        local tab = tabs[i];
        local x,y,r,b = tab:place()
        if r > tabs_r and tab.style.visibility == 'visible' then
            moveToOffstrip(tab);			
        elseif r <= tabs_r and tab.style.visibility == 'hidden' then			
            tab.style.visibility = 'visible';
            moveFromOffstrip(tab);
        end
    end
    self:update();
end

--处理元素行为事件
--@param self 当前元素
--@param cmd 行为事件
--           .sinking 刚触发该事件
--           .handled 行为是否已处理
--           .target  行为的目标元素
--           .value   行为类型
--           .name    行为昵称
--@return  行为是否处理
function TabStrip:HandleBehaviorEvent(cmd, reason, data)
    --刚触发或已处理则直接返回
    if cmd.sinking or cmd.handled then return false end

    if cmd.value == 'BUTTON_CLICK' then --单击行为
        if cmd.target.type == "option" then --标签被单击
            self:switchTab(cmd.target)
            return true
        elseif cmd.target == self:query("widget.close") then --close按钮被单击
            local ctab = self:query("widget.tabs>option:current")
            self:closeTab( ctab ) 
            return true
        end
    elseif cmd.value == 'MENU_ITEM_CLICK' then --单击菜单项
        local menuItem = cmd.target
        local tab      = menuItem.tab
        if tab then
            tab.style.visibility = 'visible'
            tab.parent:insert(tab,0) -- move it as a first one		
            tab.parent:update()
            menuItem:destroy() 
            self:HandleSize()
            self:switchTab(tab)
            return true
        end
    end
    return false
end

