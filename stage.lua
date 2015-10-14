Dialogue = require 'Dialogue'

update = {"update"}
game = Dialogue.new{ { {"draw", 6, 8 } }, { { { {"draw", 400, 200} }, {} }, { { {"draw", 2, 4} }, {} } } }

tim = Dialogue.Actor{ {"draw", 6, 8} }
