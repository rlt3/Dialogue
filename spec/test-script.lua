Test = Script("Test", function(s, n, t)
    return { 
        string = s, 
        numeral = n, 
        table = t, 
        last_author = nil 
    }
end)

function Test:increment_by (x, author)
    self.numeral = self.numeral + x
    self.last_author = author
end

function Test:name_is (str, author)
    self.string = str
    self.last_author = author
end

function Test:set_table (tab, author)
    self.table = tab
    self.last_author = author
end

function Test:proxy (tone, msg)
    actor[tone](actor, msg)
end

function Test:io (str)
    io.write(str)
end

return Test
