require("Script")

Script("Foo", function(m, n)
    return { x = m, y = n }
end)

function Foo:test ()
    io.write("x: " .. self.x .. " y: " .. self.y .. "\n")
end

return Foo
