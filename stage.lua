Envelope = require 'Dialogue.Envelope'
Actor = require 'Dialogue.Actor'
Script = require 'Dialogue.Actor.Script'

draw = Script{"draw", 400, 200}

update = Envelope{"update"};
make = Envelope{"construct", 20, 40};

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
