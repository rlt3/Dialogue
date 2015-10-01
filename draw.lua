Draw = {}
Draw.__index = Draw

function Draw.new(x, y)
   local table = {}
   setmetatable(table, Draw)
   table.x = x or 0
   table.y = y or 0
   return table
end

function Draw:update ()
    io.write("Drawing at " .. self.x .. ", " .. self.y .. "\n");
end

return Draw
