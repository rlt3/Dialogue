dialogue = Dialogue.new{
    { "Lead", {"draw", 100, 100} },
    {
        { 
            { {"draw", 2, 4} },
            {}
        }
    }
}

dialogue:children()[1]:send{"move", 2, 2}
