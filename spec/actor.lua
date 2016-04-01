_G.arg = {}
require 'busted.runner'()

--
-- This tests specific Actor object behavior and not behavior dealing with the
-- Company (tree). See `company.lua` for the tests on the Company.
--

-- NOTE: these tests require that auto-loading be off (`-l` flag to be passed)
-- so that we may test Actors in a loaded/unloaded state more easily than 
-- forcing Scripts to become unloaded by sending erroneous messages.

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

    it("doesn't allow sending or probing if it isn't loaded", function()
        actor = Actor{ {"test-script"} }

        assert.has_error(function() 
            actor:probe(1, "private-member")
        end, "Cannot probe `private-member': not loaded!")

        assert.has_error(function() 
            actor:send{"increment", 4}
        end, "Actor `0' has no loaded Scripts!")
    end)

    it("unloads the first Script which handles an erroneous message", function()
        actor = Actor{ {"test-script", "foo", 10, {}}, {"test-script", "bar", 5, {}} }
        actor:load()

        assert.has_error(function() 
            actor:send{"increment_by"}
        end, "attempt to perform arithmetic on local 'x' (a nil value)")

        assert.has_error(function() 
            actor:probe(1, "numeral")
        end, "Cannot probe `numeral': not loaded!")

        assert.is_equal(actor:probe(2, "numeral"), 5)
    end)

    it("can reload specific Scripts by id", function()
        actor = Actor{ {"test-script", "foo", 10, {}}, {"test-script", "bar", 5, {}} }
        actor:load()

        assert.has_error(function() 
            actor:send{"increment_by"}
        end, "attempt to perform arithmetic on local 'x' (a nil value)")

        assert.has_error(function() 
            actor:probe(1, "numeral")
        end, "Cannot probe `numeral': not loaded!")

        actor:load(1)
        assert.is_equal(actor:probe(1, "numeral"), 10)
        assert.is_equal(actor:probe(2, "numeral"), 5)
    end)

    it("uses the definition table given on creation for reloading", function()
        actor = Actor{ {"test-script", "foo", 10, {}} }
        actor:load()

        actor:send{"increment_by", 20}
        actor:send{"name_is", "jim"}

        assert.is_equal(actor:probe(1, "numeral"), 30)
        assert.is_equal(actor:probe(1, "string"), "jim")
        actor:load(1)
        assert.is_equal(actor:probe(1, "numeral"), 10)
        assert.is_equal(actor:probe(1, "string"), "foo")
    end)
end)
