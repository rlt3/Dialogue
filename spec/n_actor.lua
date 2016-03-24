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

    --it("can be created with a definition table of Script definitions", function()
    --    -- A Script definition is defined:
    --    --     { "module name" [, data0 [, data1 [, ... [, dataN]]]] }
    --    -- An Actor's definition table is a table of Script definitions.
    --    actor = Actor{ {"test-script"} }
    --    assert.is_equal(actor:id(), 0)
    --end)

    --it("can be created without any Script definitions", function()
    --    actor = Actor{}
    --    assert.is_equal(actor:id(), 0)
    --end)

    --it("will error if definition table isn't a table of tables", function()
    --    assert.has_error(function() 
    --        Actor{ {"valid"}, "bad" }
    --    end, "Failed to create script: `bad` isn't a table!")
    --end)

    --it("will error if a Script definition names no Script", function()
    --    assert.has_error(function() 
    --        Actor{ {} }
    --    end, "Failed to create script: invalid definition!")
    --end)

    --it("does not create an Actor instance if it has an invalid definition table", function()
    --    assert.has_error(function() 
    --        Actor{ {} }
    --    end, "Failed to create script: invalid definition!")

    --    actor = Actor{}
    --    assert.is_equal(actor:id(), 0)
    --end)

    it("does not keep the Actor's instance attached to the object", function()
        -- Tests that these objects are merely controllers for data which is
        -- contained inside the Company (Tree).
        actor = Actor{}
        assert.is_equal(actor:id(), 0)
        assert.is_equal(actor:parent():id(), -1)
        actor = nil

        actor = Actor{}
        assert.is_equal(actor:id(), 1)
        assert.is_equal(actor:parent():id(), 0)
        actor:remove()

        actor = Actor(0)
    end)

    --it("doesn't allow sending or probing if it isn't loaded", function()
    --    actor = Actor{ {"test-script"} }

    --    assert.has_error(function() 
    --        actor:probe(1, "private-member")
    --    end, "Cannot probe `private-member': not loaded!")
    --end)
end)
