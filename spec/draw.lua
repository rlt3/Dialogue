Draw = {}
Draw.__index = Draw

function Draw.new(x, y)
   local table = {}
   setmetatable(table, Draw)
   table.follow = nil
   table.coordinates = {x, y} or {0, 0}
   return table
end

function Draw:move (x, y, author)
    self.coordinates[1] = self.coordinates[1] + x
    self.coordinates[2] = self.coordinates[2] + y
end

function Draw:bump (author)
    self:move(-1, -1)
    actor:whisper(author, {"move", -1, -1})
end

function Draw:watch (author)
    self.follow = author
end

function Draw:update ()
    io.write("Drawing at " .. self.coordinates[1] .. ", " .. self.coordinates[2] .. "\n");
end

return Draw
