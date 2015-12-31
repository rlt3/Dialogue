__col = {}
__col.__index = __col

function __col:nth(n)
   return self[n]
end

function __col:each(f)
    for i = 1, #self do
        f(self[i])
    end
end

function Collection(table)
   setmetatable(table, __col)
   return table
end

return Collection
