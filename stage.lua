Dialogue = require 'Dialogue'

update = {"update"}

tim_t = { {"draw", 2, 4} }

hanks = Dialogue.Actor{ {"draw", 400, 200} }

-- A dialogue with two child Actors which have two scripts.
-- Dialogue{
--     { },
--     { 
--         { {"draw", 400, 200}, {"weapon", "longsword"} },
--         { {"draw", 20, 40}, {"weapon", "knife"} }
--     }
-- };
