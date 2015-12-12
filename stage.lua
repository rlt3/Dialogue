dialogue = Dialogue.new { 
    { "Lead", {"Graphics"} },
    { 
        {
            { {"Paddle", 50, 50} },
            { }
        }
    } 
}

--dialogue:send{"register", {0, 0, 50, 50}}
dialogue:yell{"draw"}
