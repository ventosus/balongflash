#!/usr/bin/env lua

local input = arg[1]
local output = arg[2]

local f = io.input(input)
local s = f:read('*a')
f:close()

local cyrillic = ''
for i = 0x410, 0x44f do
	cyrillic = cyrillic .. utf8.char(i)
end

--print(cyrillic)

local function trans(o)
	local p = io.popen('trans -brief ' .. o)
	local q = p:read('*l')
	p:close()

	return q
end

r = s:gsub(string.format('([%s]+)', cyrillic), function(o)
	print(o)

	return trans(o)
end)

f = io.output(output)
f:write(r)
f:close()
