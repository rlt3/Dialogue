-- If no second argument is passed to `Script` then the `new` function created
-- just returns an empty table with the 'Player' metatable attached.
Player = Script("Player")

-- Translate the input to a "move" message
function Player:input (direction)
    actor:think{"move", direction}
end

return Player
