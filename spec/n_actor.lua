_G.arg = {}
require 'busted.runner'()

--
-- This tests specific Actor object behavior and not behavior dealing with the
-- Company (tree). See `company.lua` for the tests on the Company.
--

describe("An Actor object", function()
    local actor

    after_each(function()
        Actor(0):remove()
    end)

    it("can be created with a definition table of Script definitions", function()
        actor = Actor{ {"test-script"} }
    end)

    it("can be created without any Script definitions", function()
        actor = Actor{}
    end)

    it("will error if definition table isn't a table of tables", function()
        assert.has_error(function() 
            Actor{ {"valid"}, "bad" }
        end, "Failed to create script: `bad` isn't a table!")
    end)

    it("will error if a Script definition names no Script", function()
        assert.has_error(function() 
            Actor{ {} }
        end, "Failed to create script: invalid definition!")
    end)

    -- an actor with bad scripts does not get automatically gc
    pending("keeps a bad Actor's lifetime 'alive'")
end)
