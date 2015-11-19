Dialogue = require("Dialogue")

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


mailbox = dialogue:mailbox()
b = dialogue:children()[2]
--o = b:children()[1]:scripts()[1]
--t = b:children()[2]:scripts()[1]
