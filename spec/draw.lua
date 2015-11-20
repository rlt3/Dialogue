Draw = {}
Draw.__index = Draw

function Draw.new(x, y)
   local table = {}
   setmetatable(table, Draw)
   table.coordinates = {x, y} or {0, 0}
   return table
end

function Draw:move (x, y)
    self.coordinates[1] = self.coordinates[1] + x
    self.coordinates[2] = self.coordinates[2] + y
end

function Draw:walk ()
    actor:think{"move", 2, 2}
end

function Draw:update ()
    io.write("Drawing at " .. self.coordinates[1] .. ", " .. self.coordinates[2] .. "\n");
end

return Draw
