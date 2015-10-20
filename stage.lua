Dialogue = require 'Dialogue'

update = {"update"}

--tim = Dialogue.Actor{ {"draw", 0, 0 }, {"weapon", "sword", "left"} }
tim = Dialogue.Actor{ {"draw", 0, 0 } }

tree = Dialogue.new{ 
    { {"draw", 0, 0 } }, 
    { 
        { { {"weapon", "axe", "down" } }, {} }, 
        { { {"draw", 2, 4 }, {"weapon", "longsword", "up"} }, {} } 
    }
}

x = tim:scripts()[1]
y = tree:children()[1]:scripts()[1]
