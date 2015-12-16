Coordinate = require ("Coordinate")
Paddle = {}
Paddle.__index = Paddle

function Paddle.new (x, y)
   local table = {}
   setmetatable(table, Paddle)
   table.x = x
   table.y = y
   return table
end

function Paddle:draw (author)
    actor:whisper(author, {"register", {self.x, self.y, 50, 50}})
end

function Paddle:position (x, y)
    self.x = x
    self.y = y
end

return Paddle
