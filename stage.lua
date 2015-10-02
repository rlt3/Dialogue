Dialogue = require 'Dialogue'

draw = Dialogue.Script{"draw", 400, 200}
update = Dialogue.Envelope{"update"}

hanks = Dialogue.Actor();
hanks:give{ "draw", 400, 200 }

-- An actor with two scripts.
-- Actor{ {"draw", 400, 200}, {"weapon", "longsword"} };
-- 
-- A dialogue with two child Actors which have two scripts.
-- Dialogue{
--     { },
--     { 
--         { {"draw", 400, 200}, {"weapon", "longsword"} },
--         { {"draw", 20, 40}, {"weapon", "knife"} }
--     }
-- };
