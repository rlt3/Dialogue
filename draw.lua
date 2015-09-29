Draw = {}
Draw.__index = Draw

function Draw.new()
   local table = {}
   setmetatable(table, Draw)
   table.x = 0
   table.y = 0
   return table
end

function Draw:construct (x, y)
    self.x = x
    self.y = y
end

function Draw:update ()
    io.write("Drawing at " .. self.x .. ", " .. self.y .. "\n");
end

return Draw
