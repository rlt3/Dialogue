_G.arg = {}
require 'busted.runner'()

--
-- This tests specific Actor object behavior and not behavior dealing with the
-- Company (tree). See `company.lua` for the tests on the Company.
--

describe("An Actor object", function()
    local actor

    after_each(function()
        if actor then
            actor:remove() 
            actor = nil
        end
    end)

    it("can be created with a definition table of Script definitions", function()
        -- A Script definition is defined:
        --     { "module name" [, data0 [, data1 [, ... [, dataN]]]] }
        -- An Actor's definition table is a table of Script definitions.
        actor = Actor{ {"test-script"} }
        assert.is_equal(actor:id(), 0)
    end)

    it("can be created without any Script definitions", function()
        actor = Actor{}
        assert.is_equal(actor:id(), 0)
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

    it("does not create an Actor instance if it has an invalid definition table", function()
        assert.has_error(function() 
            Actor{ {} }
        end, "Failed to create script: invalid definition!")

        actor = Actor{}
        assert.is_equal(actor:id(), 0)
    end)
end)
