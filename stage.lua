Dialogue = require 'Dialogue'

update = {"update"}

tim = Dialogue.Actor{ {"draw", 0, 0 } }

tree = Dialogue.new{ 
    { {"draw", 0, 0 } }, 
    { 
        { { {"draw", 50, 100 }, {"weapon", "axe", "down" } }, {} }, 
        { { {"draw", 2, 4 }, {"weapon", "longsword", "up"} }, {} } 
    }
}
