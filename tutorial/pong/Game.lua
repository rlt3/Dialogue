Window = require("Window")

Game = Script("Game", function()
-- No need to set actor-level variables because no other scripts need this 
-- information. This table is exactly like private scope in typical a OOP 
-- class. No other scripts can access this information.
    return {
        window = Window.new(400, 600),
        to_draw = {}
    }
end)

function handle_input (window)
    event_type, data = window:get_event()

    if event_type == "key_down" then
        if data.key == "w" then
            actor:yell{"input", "up"}
        elseif data.key == "a" then
            actor:yell{"input", "left"}
        elseif data.key == "s" then
            actor:yell{"input", "down"}
        elseif data.key == "d" then
            actor:yell{"input", "right"}
        end
    end

    if event_type == "key_up" then
        actor:yell{"input", "stop"}
    end
end

function render (window, to_draw)
    window:clear()
    window:set_color(128, 128, 128, 255)
    for author, body in pairs(to_draw) do
        window:draw(body.x, body.y, body.w, body.h)
    end
    window:render()
end

-- Register the definitions by the actor's id
function Game:register (body, author)
    self.to_draw[author] = body
end

-- The main `loop` of the game purely by recursive message
function Game:main ()
    if self.window:per_second(30) then
        actor:command{"update"}
    end

    handle_input(self.window)
    actor:command{"draw"}
    render(self.window, self.to_draw)
    
    -- recursive :^)
    actor:think{"main"}
end

return Game
