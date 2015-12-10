Window = require("Window")
Graphics = {}
Graphics.__index = Graphics

function Graphics.new (x, y)
   local table = {}
   setmetatable(table, Graphics)
   table.window = Window.new(400, 600)
   table.coordinates = {x, y} or {0, 0}
   return table
end

function Graphics:draw ()
    self.window:clear()
    self.window:set_color(128, 128, 128, 255)
    self.window:draw(0, 0, 50, 50)
end

function Graphics:move (x, y)
    self.coordinates[1] = self.coordinates[1] + x
    self.coordinates[2] = self.coordinates[2] + y
end

function Graphics:update ()
    if self.window:per_second(1) then
        self.window:clear()
        self.window:set_color(128, 128, 128, 255)
        self.window:draw(self.coordinates[1], self.coordinates[2], 50, 50)
        self.window:render()
    end
end

return Graphics
