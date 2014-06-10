local function maths()
  return (-10 + (2 + 2) * 20)
end

local function jit_test(m, t)
	local ret = 0

	for i=1,10000000 do
		ret = ret + m + t + maths()
	end

	return ret
end

local function float_maths()
  return 4.0 + 4.0
end

local function jit_float_test()
  return 8.4 + float_maths()
end

print(jit_test(4, 4))
print(jit_float_test())
