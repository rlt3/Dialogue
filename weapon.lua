Weapon = {}
Weapon.__index = Weapon

function Weapon.new(weapon, direction)
   local table = {}
   setmetatable(table, Weapon)
   table.weapon = weapon or "dagger"
   table.direction = direction or "down"
   table.durability = 10
   return table
end

function Weapon:attack ()
    self.durability = self.durability - 1
end

function Weapon:update ()
    io.write("Swinging " .. self.weapon .. " " .. self.direction .. "\n");
end

return Weapon

