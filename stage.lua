Dialogue.Post.init(2, 1024)

head = Dialogue.new{
    { {"weapon", "Crown", "North"} },
    {
        { 
            { "Lead", {"draw", 2, 4} },
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

a = head:children()[1]
b = head:children()[2]
c = b:children()[1]
d = b:children()[2]
e = head:children()[3]

os.execute("sleep " .. tonumber(1.0))

print(Dialogue.Post.__obj)
b:whisper(e, "bump")
os.execute("sleep " .. tonumber(0.5))
