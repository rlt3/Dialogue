Dialogue = require 'Dialogue'

actor = Dialogue.Actor.new({ {"draw", 2, 4}, {"draw", 5, 6} })

children = actor:children{ 
    { {"weapon", "sword & board", "down"}, {"draw", 128, 256} },
    { {"weapon", "magic missile", "up"}, {"draw", 120, 250} },
    { {"weapon", "stupid bow", "up"}, {"draw", 115, 245} },
}
