Body = Script("Body", function(width, height, x, y)
-- We setting variables on the actor think of it like a regular OOP protected
-- property. When doing this, that variable will be available to all other
-- scripts on that actor. This allows us to set the definition of body for any
-- other scripts which need that definition
    actor.body = {
        w = width,
        h = height,
        x = x,
        y = y
    }
-- No need for private variables in this script, just return an empty table
    return {}
end)

-- We assume that the author of the draw message is going to handle the
-- drawing. So we register our body (dimensions & location) with that author.
function Body:draw (author)
    actor:whisper(author, {"register", actor.body})
end

return Body
