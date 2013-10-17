local function isPrime(number)
	if number < 2 then return false end

	for i=2, math.sqrt(number) do
		if number % i == 0 then return false end
	end

	return true
end

for i=1,100000 do
	if isPrime(i) then
		io.write(i)
		io.write('\n')
	end
end