Draw = {}
Draw.__index = Draw

function Draw.new(x, y)
   local table = {}
   setmetatable(table, Draw)
   table.coordinates = {}
   table.coordinates.x = x or 0
   table.coordinates.y = y or 0
   return table
end

function Draw:move (x, y)
    self.coordinates.x = self.coordinates.x + x
    self.coordinates.y = self.coordinates.y + y
end

function Draw:update ()
    io.write("Drawing at " .. self.coordinates.x .. ", " .. self.coordinates.y .. "\n");
end

return Draw
