Window = require("Window")
Graphics = {}
Graphics.__index = Graphics

function Graphics.new ()
   local table = {}
   setmetatable(table, Graphics)
   table.window = Window.new(400, 600)
   table.to_draw = {}
   return table
end

function Graphics:destroy ()
    self.window:quit()
end

-- Register a definition to draw
function Graphics:register (definition, author)
    self.to_draw[author:__tostring()] = definition
end

-- Actually draw the definitions given
function Graphics:render (r, g, b, a)
    self.window:clear(r, g, b, a)
    self.window:set_color(128, 128, 128, 255)
    for author, definition in pairs(self.to_draw) do
        self.window:draw(unpack(definition))
    end
    self.window:render()
end

function Graphics:handle_event ()
    event_type, data = self.window:get_event()

    if event_type == "key_down" then
        if data.key == "quit" then exit() end
        
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

function Graphics:main ()
    if self.window:per_second(30) then
        actor:yell{"update"}
    end
    self:handle_event()
    actor:command{"draw"}
    self:render()
end

return Graphics
