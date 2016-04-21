Coordinate = {}
Coordinate.__index = Coordinate

function Coordinate.new (x, y)
   local pair = {}
   pair.x = x or 0
   pair.y = y or 0
   return pair
end

function Coordinate.add (l, r)
    return Coordinate.new(l.x + r.x, l.y + r.y)
end

function Coordinate.scale (c, scale)
    return Coordinate.new(c.x * scale, c.y * scale)
end

function Coordinate.from_state (state)
    if state == "down" then
        return Coordinate.new(0, 1)
    elseif state == "right" then
        return Coordinate.new(1, 0)
    elseif state == "up" then
        return Coordinate.new(0, -1)
    elseif state == "left" then
        return Coordinate.new(-1, 0)
    else
        return Coordinate.new(0, 0)
    end
end

return Coordinate
