Coordinate = require ("Coordinate")

Moveable = {}
Moveable.__index = Moveable

function Moveable.new (max_speed, x, y)
   local table = {}
   setmetatable(table, Moveable)
   table.body = Coordinate.new(x, y)
   table.force = Coordinate.new(0.0, 0.0)
   table.magnitude = Coordinate.new(0.0, 0.0)
   table.max = max_speed
   return table
end

function Moveable:update (dt)
    self:oscilate()
    actor:say{"position", self.body.x, self.body.y}
end

function Moveable:input (state)
    self.force = Coordinate.from_state(state)
end

function Moveable:stop ()
    self.force.x = 0
    self.force.y = 0
end

function Moveable:position (dx, dy)
    self.body.x = dx
    self.body.y = dy
end

function Moveable:oscilate ()
    self.magnitude.x = self:oscilate_ordinate(self.force.x, self.magnitude.x);
    self.magnitude.y = self:oscilate_ordinate(self.force.y, self.magnitude.y);
    self.body = Coordinate.add(self.body, self.magnitude);
end

function Moveable:oscilate_ordinate (force, current)
    -- if there is a force and not max speed, add the force
    if force ~= 0.0 and math.abs(current) <= self.max then
        return current + force
    else
    -- else bring current toward 0 from whatever direction (neg or pos)
        if current == 0.0 then
            return current
        else
            if current < 0.0 then
                force = 1.0 
            else
                force = -1.0
            end
            return current + force
        end
    end
end

return Moveable
