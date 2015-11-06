Dialogue = require 'Dialogue'

--actor = Dialogue.Actor.new({ {"draw", 2, 4}, {"draw", 5, 6} })
--
--children = actor:children{ 
--    { {"weapon", "sword & board", "down"}, {"draw", 128, 256} },
--    { {"weapon", "magic missile", "up"}, {"draw", 120, 250} },
--    { {"weapon", "stupid bow", "up"}, {"draw", 115, 245} },
--}
--
--mailbox = Dialogue.Mailbox.new(8):pause()
--
--envelope = Dialogue.Mailbox.Envelope.new(mailbox, actor, "yell", {"update"})

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

b = dialogue:children()[2]

audience = b:audience("yell")
