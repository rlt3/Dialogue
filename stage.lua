Dialogue = require 'Dialogue'

update = {"update"}
e = Dialogue.Envelope(update)

game = Dialogue.new{ { {"draw", 6, 8 } }, { { { {"draw", 400, 200} }, {} }, { { {"draw", 2, 4} }, {} } } }
