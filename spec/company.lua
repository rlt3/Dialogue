_G.arg = {}
require 'busted.runner'()

describe("The Company", function()
    local a0, a1, a2, a3, a4, a5

    setup(function()
        a0 = Actor{}
        a1 = a0:child{}

        a2 = a0:child{}
        a3 = a2:child{}
        a4 = a2:child{}

        a5 = a0:child{}

        --     0
        --   / | \
        --  1  2  5
        --    / \
        --   3   4
    end)

    teardown(function()
        a0:remove()
    end)

    it("is a tree of Actors", function()
        assert.are_same(a0:children(), {1, 2, 5})
        assert.are_same(a1:children(), {})
        assert.are_same(a2:children(), {3, 4})
        assert.are_same(a3:children(), {})
        assert.are_same(a4:children(), {})
        assert.are_same(a5:children(), {})
    end)

    it("handles Actors by integer ids", function()
        assert.are_same(Actor(5), a5)
    end)

    it("doesn't handle lifetimes through the Lua actor objects", function()
        a2 = nil
        assert.is_equal(a3:parent():id(), 2)
        assert.is_equal(a4:parent():id(), 2)
        a2 = Actor(2)
    end)

    it("lets actors be deleted through the method `remove`", function()
        a5:remove()
        assert.are_same(a0:children(), {1, 2})
    end)

    it("removes the descendents recursively of a deleted node", function()
        assert.is_equal(a3:parent():id(), 2)
        assert.is_equal(a4:parent():id(), 2)
        a2:remove()
        assert.are_same(a0:children(), {1})
    end)

    it("considers ids from removed (and non-existing) actors to be invalid", function()
        --assert.has_error(function() 
        --    a3:parent():id()
        --end, "Cannot get parent of invalid Actor `3`!")

        --assert.has_error(function() 
        --    a2:child{}
        --end, "Failed to create actor: invalid parent id `2`!")

        --assert.has_error(function() 
        --    a2:send{}
        --end, "Actor id `2` is an invalid reference!")

        --assert.has_error(function() 
        --    a2:load()
        --end, "Actor id `2` is an invalid reference!")

        --assert.has_error(function() 
        --    a2:probe()
        --end, "Actor id `2` is an invalid reference!")

        --assert.has_error(function() 
        --    a2:remove()
        --end, "Cannot delete invalid reference `2`!")

        --assert.has_error(function() 
        --    a2:bench()
        --end, "Cannot bench invalid reference `2`!")

        --assert.has_error(function() 
        --    a2:join()
        --end, "Cannot join `2`: id invalid!")

        ----assert.has_error(function() 
        ----    a2:async("send", {})
        ----end, "Starting async method `nil` failed: invalid Actor id `2`!")

        ---- Fail silently right now
        --a2:children()
        --a2:audience("foo")
    end)

    it("it recycles garbage ids when creating assigning new ones", function()
        a2 = a0:child{}
        assert.are_same(a0:children(), {1, 2})
        a2:child{}
        a2:child{}
        assert.are_same(a2:children(), {3, 4})
        a0:child{}
        assert.are_same(a0:children(), {1, 2, 5})
    end)

    --pending("can handle deletion of the root")

    --it("creates Actors and returns actor objects which hold an id", function()
    --    local a0 = Actor({})
    --    assert.is_equal(a0:id(), 0)
    --end)

    --it("lets Actors be accessed by their id", function()
    --    local a0 = Actor(0)
    --    assert.is_equal(#a0:children(), 0)
    --    assert.is_equal(a0:id(), 0)
    --    assert.is_equal(a0:parent():id(), -1)
    --end)

    --it("creates Actor objects with just an id", function()
    --    local a37 = Actor(37)
    --    assert.is_equal(a37:id(), 37)
    --end)

    --it("allows creation of children explicitly or implicitly", function()
    --    -- explicit creation
    --    local a0 = Actor(0)
    --    assert.is_equal(Actor({}, a0):id(), 1)
    --    assert.is_equal(Actor(1):parent():id(), 0)
    --    -- implicit creation
    --    local a2 = a0:child{}
    --    assert.is_equal(a2:id(), 2)
    --    assert.is_equal(a2:parent():id(), 0)
    --    assert.is_equal(#a0:children(), 2)
    --end)

    --it("will error if creating a child attached to a bad parent", function()
    --    local a37 = Actor(37)

    --    assert.has_error(function() 
    --        a37:child{} 
    --    end, "Failed to create actor: invalid parent id `37`!")

    --    assert.has_error(function() 
    --        Actor({}, a37)
    --    end, "Failed to create actor: invalid parent id `37`!")
    --end)

    --it("can delete any actor by id", function()
    --    -- undeleted children from `creation of children` above
    --    Actor(1):delete()
    --    Actor(2):delete()
    --    assert.is_equal(#Actor(0):children(), 0)
    --end)

    --it("will error if delete is called for an invalid actor", function()
    --    local a44 = Actor(44)
    --    assert.has_error(function() 
    --        a44:delete() 
    --    end, "Cannot delete invalid reference `44`!")
    --end)

    --it("deletes all descendents of an Actor when that Actor is deleted", function()
    --    local a1 = Actor(0):child{}
    --    assert.is_equal(a1:id(), 1)
    --    assert.is_equal(a1:child{}:id(), 2)
    --    assert.is_equal(a1:child{}:id(), 3)

    --    assert.is_equal(Actor(1):parent():id(), 0)
    --    assert.is_equal(Actor(2):parent():id(), 1)
    --    assert.is_equal(Actor(3):parent():id(), 1)

    --    a1:delete()

    --    assert.has_error(function() 
    --        Actor(1):child{}
    --    end, "Failed to create actor: invalid parent id `1`!")
    --    assert.has_error(function() 
    --        Actor(2):child{}
    --    end, "Failed to create actor: invalid parent id `2`!")
    --    assert.has_error(function() 
    --        Actor(3):child{}
    --    end, "Failed to create actor: invalid parent id `3`!")
    --end)

    --it("garbage collects garbage actors when creating new ones", function()
    --    -- id of new Actor will be 1 even though 1 existed before
    --    local new_actor = Actor(0):child{}
    --    assert.is_equal(new_actor:id(), 1)
    --    assert.is_equal(new_actor:parent():id(), 0)
    --    new_actor:delete()
    --end)

    --it("doesn't garbage collect still-referenced actors", function()
    --    local a0 = Actor(0)
    --    local a1 = a0:child{}
    --    local a2 = a1:child{}

    --    assert.is_equal(a1:id(), 1)
    --    assert.is_equal(a2:id(), 2)

    --    assert.is_equal(a1:parent():id(), 0)
    --    assert.is_equal(a2:parent():id(), 1)

    --    -- lock a2 (same mechanism as reference) and delete its parent
    --    assert.is_true(a2:lock())
    --    a1:delete()

    --    -- id is `1` because a1 was deleted above and `1` became free
    --    assert.is_equal(a0:child{}:id(), 1)

    --    -- but id here is `3`. skips `2` because it is still ref'd
    --    assert.is_equal(a0:child{}:id(), 3)

    --    assert.is_true(a2:unlock())
    --    assert.is_equal(a0:child{}:id(), 2)

    --    Actor(1):delete()
    --    Actor(2):delete()
    --    Actor(3):delete()
    --end)

    --it("allows benching and joining of Actors from the Company tree", function()
    --    local a1 = Actor(0):child{}
    --    assert.is_equal(a1:id(), 1)
    --    a1:bench()
    --    assert.is_equal(#Actor(0):children(), 0)
    --    a1:join()
    --    assert.is_equal(a1:parent():id(), 0)
    --    assert.is_equal(#Actor(0):children(), 1)
    --    a1:delete()
    --end)

    --it("doesn't allow benching of bad ids", function()
    --    assert.has_error(function() 
    --        Actor(44):bench()
    --    end, "Cannot bench invalid reference `44`!")
    --end)

    --it("doesn't cleanup benched ids", function()
    --    local a1 = Actor(0):child{}
    --    assert.is_equal(a1:id(), 1)
    --    assert.is_equal(a1:parent():id(), 0)
    --    a1:bench()
    --    -- doesn't take id `1` because benching isn't deletion
    --    assert.is_equal(Actor(0):child{}:id(), 2)
    --    a1:join()
    --    a1:delete()
    --    Actor(2):delete()
    --end)

    --it("doesn't allow joining of bad actors", function()
    --    assert.has_error(function() 
    --        Actor(44):join()
    --    end, "Cannot join `44`: id invalid!")
    --end)

    --it("doesn't allow joining of non-benched actors", function()
    --    local a1 = Actor(0):child{}
    --    assert.is_equal(a1:parent():id(), 0)
    --    assert.has_error(function() 
    --        a1:join()
    --    end, "Cannot join `1`: not benched!")
    --    a1:delete()
    --end)

    --it("doesn't allow joining of benched actors with bad parents", function()
    --    local a0 = Actor(0)
    --    local a1 = a0:child{}
    --    local a2 = a1:child{}
    --    assert.is_equal(a1:id(), 1)
    --    assert.is_equal(a2:id(), 2)
    --    assert.is_equal(a1:parent():id(), 0)
    --    assert.is_equal(a2:parent():id(), 1)

    --    a2:bench()
    --    a1:delete()

    --    assert.has_error(function() 
    --        a2:join()
    --    end, "Cannot join `2`: bad parent!")

    --    a2:delete()
    --end)

    --it("can join an Actor as a child of a provided parent id", function()
    --    local a0 = Actor(0)
    --    local a1 = a0:child{}
    --    local a2 = a1:child{}
    --    assert.is_equal(a1:id(), 1)
    --    assert.is_equal(a2:id(), 2)
    --    assert.is_equal(a1:parent():id(), 0)
    --    assert.is_equal(a2:parent():id(), 1)

    --    a2:bench()
    --    a1:delete()
    --    a2:join(0)
    --    assert.is_equal(a2:parent():id(), 0)
    --    assert.is_equal(Actor(0):children(), 1)
    --    a2:delete()
    --end)

    --it("handles the audience for each Actor", function()
    --    local a0 = Actor(0)
    --    local a1 = a0:child{}

    --    local a2 = a0:child{}
    --    local a3 = a2:child{}
    --    local a4 = a2:child{}

    --    local a5 = a0:child{}

    --    --     0
    --    --   / | \
    --    --  1  2  5
    --    --    / \
    --    --   3   4

    --    assert.is_equal(a0:id(), 0)
    --    assert.is_equal(a1:id(), 1)
    --    assert.is_equal(a2:id(), 2)
    --    assert.is_equal(a3:id(), 3)
    --    assert.is_equal(a4:id(), 4)
    --    assert.is_equal(a5:id(), 5)

    --    assert.are_same(a2:audience("yell"), {0, 1, 2, 3, 4, 5})
    --    assert.are_same(a2:audience("say"), {0, 1, 2, 5})
    --    assert.are_same(a2:audience("command"), {2, 3, 4})
    --end)
end)
