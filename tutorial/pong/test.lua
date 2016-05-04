--_G['Script'] = require ("Script")
--Moveable = require("Moveable")
--
--_G['m'] = Moveable.new(10, 20, 30)
--
--m:move("left")
--
--function p ()
--    print("(" .. m.body.x .. ", " .. m.body.y .. ")")
--end

m = Actor{ {"Moveable", 10, 20, 30} }
