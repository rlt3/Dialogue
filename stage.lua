Dialogue.Post.init(2, 1024)
--actor = Dialogue.Actor.new{ {"draw", 2, 4} }
--Dialogue.Post.send(actor, 'send', 'update')

d = Dialogue.new{
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
