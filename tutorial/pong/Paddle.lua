Paddle = {}
Paddle.__index = Paddle

function Paddle.new (x, y)
   local table = {}
   setmetatable(table, Paddle)
   table.x = x
   table.y = y
   return table
end

-- Register our coordinates when we get the draw message
function Paddle:draw (author)
    actor:whisper(author, {"register", {self.x, self.y, 25, 125}})
end

-- Update our coordinates when our position changes
function Paddle:position (x, y)
    self.x = x
    self.y = y
end

return Paddle
