Collection = {}
Collection.__index = Collection

function Collection.new(table)
   setmetatable(table, Collection)
   return table
end

function Collection:nth(n)
   return self[n]
end

function Collection:each(f)
    for i = 1, #self do
        f(self[i])
    end
end

return Collection
