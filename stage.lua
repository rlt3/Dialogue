Dialogue = require 'Dialogue'

actor = Dialogue.Actor.new{}
actor:give{"weapon", "scimitar", "north"}
actor:scripts{ {"weapon", "scimitar", "north"}, {"draw", 2, 4} }
