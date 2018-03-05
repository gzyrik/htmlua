-----------------------------------------------------
--查找当前路径中lua
local SEP = package.config:sub(1,1)
local exedir = arg[0]
exedir = string.match(exedir, '(.+'..SEP..')[^'..SEP..']*$') or ''
package.path=exedir..'?.lua;'..exedir..'?'..SEP..'init.lua;'..package.path
package.cpath=exedir..'?.dll;'..package.cpath
-----------------------------------------------------
local htm=require"htm"
--htm.trace = print
local wnd = htm.open("file://!/../samples/"..(#arg > 0 and arg[#arg] or "samples")..".htm")
for i=1,#arg-1 do
    wnd._THREAD(arg[i], [[
    local htm=require"htm"
    htm.open("file://!/../samples/"..arg[0]..".htm")
    htm.loop()]])
    htm.post(arg[i],"fafaaf")
end
htm.loop()
