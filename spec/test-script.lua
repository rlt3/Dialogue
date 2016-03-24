Test = {}
Test.__index = Test

function Test.new(string, numeral, table)
   local t = {}
   setmetatable(t, Test)
   t.string = string
   t.numeral = numeral
   t.table = table
   t.last_author = nil
   return t
end

function Test.increment (x, author)
    self.numeral = self.numeral + x
    self.last_author = author
end

function Test.name (str, author)
    self.string = str
    self.last_author = author
end

function Test.proxy (tone, msg)
    actor[tone](actor, msg)
end

function Test.io (str)
    io.write(str)
end

return Test
