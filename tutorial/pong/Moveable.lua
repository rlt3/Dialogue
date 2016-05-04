Coordinate = require ("Coordinate")

Moveable = Script("Moveable", function(max_speed, x, y)
    return {
        body = Coordinate.new(x, y),
        force = Coordinate.new(0, 0),
        magnitude = Coordinate.new(0, 0),
        max = max_speed,
        label_test = { x = x, y = y },
        array_test = { x, y }
    }
end)

--
-- Helper functions
-- 

function oscilate (m)
    local mag  = oscilate_magnitude(m.magnitude, m.force, m.max)
    local body = Coordinate.add(m.body, mag)
    return body, mag
end

function oscilate_magnitude (mag, force, max)
    mag.x = oscilate_ordinate(mag.x, force.x, max)
    mag.y = oscilate_ordinate(mag.y, force.y, max)
    return mag
end

function oscilate_ordinate (current, force, max)
    -- if there is a force and not max speed, add the force
    if force ~= 0 and math.abs(current) <= max then
        return current + force
    else
    -- else no force: bring current toward 0 from whichever dir (neg or pos)
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

--
-- The message handlers
-- 

function Moveable:update (dt)
    self.body, self.magnitude = oscilate(self)
    -- `actor' is a global var which is the actor this script is attached to
    actor:think{"position", self.body.x, self.body.y}
end

-- We `think' the position in the update handler above so that it triggers
-- this specific 'position' handler and others which may be in scripts attached
-- to this actor
function Moveable:position (dx, dy)
    self.body.x = dx
    self.body.y = dy
end

-- Move "up", "down", "left", "right"
function Moveable:move (state)
    self.force = Coordinate.from_state(state)
end

return Moveable
