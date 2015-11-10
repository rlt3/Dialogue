Dialogue = require 'Dialogue'

dialogue = Dialogue.new{
    { {"weapon", "Crown", "North"} },
    {
        { 
            { {"draw", 2, 4} },
            {}
        },
        { 
            { {"draw", 400, 200} },
            {
                { 
                    { {"weapon", "bullet", "south"} },
                    {}
                },
                { 
                    { {"weapon", "bomb", "south-east"} },
                    {}
                }
            }
        },
        { 
            { {"draw", 20, 6} },
            {}
        }
    }
}

a = dialogue:children()[1]
b = dialogue:children()[2]
c = b:children()[1]
d = b:children()[2]
e = dialogue:children()[3]

print(dialogue:scripts()[1]:probe("weapon"))
print(a:scripts()[1]:probe("coordinates"))
print(b:scripts()[1]:probe("coordinates"))
print(c:scripts()[1]:probe("weapon"))
print(d:scripts()[1]:probe("weapon"))
print(e:scripts()[1]:probe("coordinates"))

audience = b:audience("say")
print(#audience)
print(audience[1])
print(audience[2])
print(audience[3])
print(audience[4])

audience = b:audience("command")
print(#audience)
print(audience[1])
print(audience[2])

audience = b:audience("yell")
print(#audience)
print(audience[1])
print(audience[2])
print(audience[3])
print(audience[4])
print(audience[5])
print(audience[6])
