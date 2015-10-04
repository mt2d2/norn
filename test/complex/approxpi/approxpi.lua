function term(k)
  local ret = 4.0 / (2 * k + 1)
  if k % 2 == 0.0 then
    return ret
  else
    return -ret
  end
end

function pi(n)
  local f = 0.0
  for i=0,n do
    f = f + term(i)
  end
  return f
end

print(pi(1e9))
