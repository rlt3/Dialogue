Coordinate = require ("Coordinate")
Moveable = {}
Moveable.__index = Moveable

function Moveable.new (max_speed, x, y)
   local table = {}
   setmetatable(table, Moveable)
   table.body = Coordinate.new(x, y)
   table.force = Coordinate.new(0, 0)
   table.magnitude = Coordinate.new(0, 0)
   table.max = max_speed
   return table
end

-- This is called 30 times per second. Since Moveable essentially controls the
-- position of the actor, we `think` to make sure to update the other scripts
function Moveable:update (dt)
    self:_oscilate()
    actor:think{"position", self.body.x, self.body.y}
end

-- Move "up", "down", etc
function Moveable:move (state)
    self.force = Coordinate.from_state(state)
end

-- When the actor `thinks` in the update handler above, it triggers this 
-- message handler.
function Moveable:position (dx, dy)
    self.body.x = dx
    self.body.y = dy
end

function Moveable:_oscilate ()
    self.magnitude.x = self:_oscilate_ordinate(self.force.x, self.magnitude.x);
    self.magnitude.y = self:_oscilate_ordinate(self.force.y, self.magnitude.y);
    self.body = Coordinate.add(self.body, self.magnitude);
end

function Moveable:_oscilate_ordinate (force, current)
    -- if there is a force and not max speed, add the force
    if force ~= 0 and math.abs(current) <= self.max then
        return current + force
    else
    -- else bring current toward 0 from whatever direction (neg or pos)
        if current == 0 then
            return current
        else
            if current < 0 then
                force = 1
            else
                force = -1
            end
            return current + force
        end
    end
end

return Moveable
