Dialogue.Post.init(2, 1024)
--actor = Dialogue.Actor.new{ {"draw", 2, 4} }

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
