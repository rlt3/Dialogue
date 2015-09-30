E = require 'Dialogue.Envelope'
S = require 'Dialogue.Script'

script = S{"draw", 400, 200}

up = E{"update"};
cons = E{"construct", 20, 40};

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
