Body = Script("Body", function(width, height, x, y)
    return { 
-- We are defining what the body looks like for the entire system.  Everything
-- that gets drawn will use this body structure. This is *exactly* like 
-- designing function's contract before its implementation, except here it is
-- the data which needs to be explicitly designed.
        body = {
            w = width,
            h = height,
            x = x,
            y = y
        }
    }
end)

function Body:body (body)
    self.body = body
end

-- We assume that the author of the draw message is going to handle the
-- drawing. So we register our body (dimensions & location) with that author.
function Body:draw (author)
    author:send{"register", body}
end

return Body
