_G.arg = {}
require 'busted.runner'()

describe("An Actor reference object", function()
    local a0 = nil
    local a1 = nil

    it("will error on creation if not given correct Script definitions", function()
        assert.has_error(function() 
            Actor{ {"good"}, "bad" }
        end, "Failed to create script: `bad` isn't a table!")

        assert.has_error(function() 
            Actor{ {"good"}, {}, {"another good"} }
        end, "Failed to create script: invalid definition!")
    end)

    it("can be created from a definition of Scripts", function()
        a0 = Actor{ {"draw", 200, 400} }
        assert.is_equal(a0:id(), 0)
    end)

    it("can be created by providing the id of an existing Actor", function()
        a0 = nil
        a0 = Actor(0)
        assert.is_equal(a0:id(), 0)
    end)

    it("won't error on creation if provided with a bad integer id", function()
        a1 = Actor(20)
    end)

    pending("can be given a name (string) to reference the Actor just like an id")
    pending("can be created by providing the name of an existing Actor")
end)
