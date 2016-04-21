Window = require("Window")

-- `Script` affects the global state. It looks at the first argument to know 
-- which table & metatable to use. Here it creates a global table named 
-- Graphics. It then creates a `new` function as a field for that table.
--
-- The `new` function accepts any number of arguments and passes them to the 
-- anonymous function which is the second argument of `Script`. `new` then
-- accepts the table output of the anonymous function, attaches the correct
-- metatable to it and return its.
Script("Graphics", function(w, h)
    return { window = Window.new(w, h), to_draw = {} }
end)

-- Register the definitions by the actor's id
function Graphics:register (definition, author)
    self.to_draw[author] = definition
end

-- The main `loop` of the game purely by recursive message
function Graphics:main ()
    if self.window:per_second(30) then
        actor:command{"update"}
    end

    self:_handle_input()
    actor:command{"draw"}
    self:_render()
    
    -- recursive :^)
    actor:think{"main"}
end

-- Actually draw the definitions registered
function Graphics:_render (r, g, b, a)
    -- rbga are optional
    self.window:clear(r, g, b, a)
    self.window:set_color(128, 128, 128, 255)
    for author, definition in pairs(self.to_draw) do
        self.window:draw(unpack(definition))
    end
    self.window:render()
end

-- Send the input messages to actors in the game
function Graphics:_handle_input ()
    event_type, data = self.window:get_event()

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
        actor:yell{"input", "no_key"}
    end
end

return Graphics
