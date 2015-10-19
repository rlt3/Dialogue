Draw = {}
Draw.__index = Draw

function Draw.new(coords)
   local table = {}
   setmetatable(table, Draw)
   table.coordinates = coords or {0, 0}
   return table
end

function Draw:update ()
    io.write("Drawing at " .. self.x .. ", " .. self.y .. "\n");
end

return Draw
