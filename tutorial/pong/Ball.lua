Coordinate = require ("Coordinate")
Ball = {}
Ball.__index = Ball

function Ball.new (x, y)
   local table = {}
   setmetatable(table, Ball)
   table.x = x
   table.y = y
   return table
end

function Ball:draw (author)
    actor:whisper(author, {"register", {self.x, self.y, 10, 10}})
end

function Ball:position (x, y)
    self.x = x
    self.y = y
end

return Ball

