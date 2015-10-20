_G.Dialogue = require("Dialogue")

describe("a Dialogue", function()
    local tree = Dialogue.new{ 
        { {"draw", 0, 0 } }, 
        { 
            { { {"draw", 50, 100 }, {"weapon", "axe", "down" } }, {} }, 
            { { {"draw", 2, 4 }, {"weapon", "longsword", "up"} }, {} } 
        }
    }

    it("is a tree of actors, each with differing amounts of scripts", function()
        assert.is.equal(#tree:scripts(), 1)
        assert.is.equal(#tree:children()[1]:scripts(), 2)
        assert.is.equal(#tree:children()[2]:scripts(), 2)
    end)

    it("allows for each actor to have localized state based on messages", function()
        tree:yell{"move", 40, 20}

        coords_one = tree:children()[1]:scripts()[1]:probe("coordinates")
        coords_two = tree:children()[2]:scripts()[1]:probe("coordinates")

        assert.is_equal(coords_one[1], 90)
        assert.is_equal(coords_one[2], 120)

        assert.is_equal(coords_two[1], 42)
        assert.is_equal(coords_two[2], 24)
    end)
end)
