Window = require("Window")
Graphics = {}
Graphics.__index = Graphics

function Graphics.new (x, y)
   local table = {}
   setmetatable(table, Graphics)
   table.window = Window.new(400, 600)
   table.to_draw = {}
   return table
end

function Graphics:draw (definition)
    table.insert(self.to_draw, definition)
end

function Graphics:render ()
    self.window:clear()
    self.window:set_color(128, 128, 128, 255)
    for i, definition in ipairs(self.to_draw) do
        self.window:draw(unpack(definition))
    end
    self.window:render()
end

function Graphics:main ()
    --if self.window:per_second(1) then
    --    io.write("second\n")
    --end
    self:render()
end

return Graphics
