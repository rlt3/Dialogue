_G.arg = {}
require 'busted.runner'()

describe("The Company of Actors", function()
    local a0
    local a1
    local a2
    local a3
    local a4
    local bad_actor

    it("creates Actors and returns actor objects which hold an id", function()
        a0 = Actor({})
        a1 = a0:child{}
        a2 = a1:child{}
        a3 = a1:child{}

        assert.is_equal(a0:id(), 0)
        assert.is_equal(a1:id(), 1)
        assert.is_equal(a2:id(), 2)
        assert.is_equal(a3:id(), 3)
    end)

    it("expects actor objects to be an id with a metatable attached", function()
        -- "create" a bad actor object for testing
        bad_actor = setmetatable( { 20 }, getmetatable(a0))
        assert.is_equal(getmetatable(a2), getmetatable(bad_actor))
    end)

    it("allows creation of children explicitly or implicitly", function()
        -- explicit creation
        assert.is_equal(Actor({}, a0):id(), 4)
        -- implicit creation
        a4 = a0:child{}
        assert.is_equal(a4:id(), 5)
    end)

    it("will error if creating a child of a bad parent", function()
        assert.has_error(function() 
            bad_actor:child{} 
        end, "Invalid parent `20` for new actor!")

        assert.has_error(function() 
            Actor({}, bad_actor)
        end, "Invalid parent `20` for new actor!")
    end)

    it("can delete any actor by id", function()
        a3:delete()
        a3 = nil
    end)

    it("will error if delete is called for an invalid actor", function()
        assert.has_error(function() 
            bad_actor:delete() 
        end, "Cannot delete invalid reference `20`!")
    end)

    it("garbage collects garbage actors when creating new ones", function()
        -- since we deleted a3 a while back, it's id (or it's 'space') will be
        -- free. a3 will now be the child of a0 whereas before it was a child
        -- of a1
        a3 = a0:child{}
        assert.is_equal(a3:id(), 3)
    end)

    it("doesn't garbage collect still-referenced actors", function()
        -- a1 now has 1 child (a2). deleting a1 will mark a1 and its
        -- descendents as garbage. Before that, we reference (lock) a2 so it
        -- won't be cleaned up when another Actor is created
        assert.is_true(a2:lock())
        a1:delete()

        -- 1 was the next unused spot
        assert.is_equal(a4:child{}:id(), 1)

        -- Even though 2 was the next unused it was locked, so the new actor
        -- was given the next unused id: 6
        assert.is_equal(a4:child{}:id(), 6)

        -- unlocking the reference to 2 frees it to be garbage collected
        assert.is_true(a2:unlock())
        assert.is_equal(a0:child{}:id(), 2)
    end)

    it("allows benching of actors which may be erroring often", function()
        a4:bench()
    end)

    it("doesn't allow benching of bad ids", function()
        assert.has_error(function() 
            bad_actor:bench()
        end, "Cannot bench invalid reference `20`!")
    end)

    it("doesn't cleanup benched ids", function()
        assert.is_equal(a0:child{}:id(), 7)
    end)

    it("allows benched actors to be new parents", function()
        assert.is_equal(a4:child{}:id(), 8)
    end)

    it("lets benched actors rejoin the tree", function()
        a4:join()
    end)

    it("doesn't allow joining of bad actors", function()
        assert.has_error(function() 
            bad_actor:join()
        end, "Cannot join `20` -- id invalid!")
    end)

    it("doesn't allow joining of non-benched actors", function()
        assert.has_error(function() 
            a0:join()
        end, "Cannot join `0` -- not benched!")
    end)

    it("doesn't allow joining of benched actors with bad parents", function()
        local a8 = setmetatable( { 8 }, getmetatable(a0))
        a8:bench()
        a4:delete()
        assert.has_error(function() 
            a8:join()
        end, "Cannot join `8` -- bad parent!")
    end)
end)
