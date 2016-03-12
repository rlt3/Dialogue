_G.arg = {}
require 'busted.runner'()

describe("The Company", function()
    local a0, a1, a2, a3, a4, a5

    local create_tree = function()
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
    end

    setup(create_tree)

    teardown(function()
        a0:remove()
    end)

    --it("will resize automatically as Actors are created", function()
    --    local parent = a0:child{}
    --    assert.is_equal(parent:id(), 6)
    --    for i = 7, 100 do
    --        assert.is_equal(parent:child{}:id(), i)
    --    end
    --    --parent:remove()
    --    --a0:delete()
    --    --create_tree()
    --end)

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
        assert.has_error(function() 
            a3:parent()
        end, "Cannot get parent of invalid Actor `3`!")

        assert.has_error(function() 
            a2:child{}
        end, "Failed to create actor: invalid parent id `2`!")

        assert.has_error(function() 
            a2:send{}
        end, "Actor id `2` is an invalid reference!")

        assert.has_error(function() 
            a2:load()
        end, "Actor id `2` is an invalid reference!")

        assert.has_error(function() 
            a2:probe()
        end, "Actor id `2` is an invalid reference!")

        assert.has_error(function() 
            a2:remove()
        end, "Cannot delete invalid reference `2`!")

        assert.has_error(function() 
            a2:bench()
        end, "Cannot bench invalid reference `2`!")

        assert.has_error(function() 
            a2:join()
        end, "Cannot join `2`: id invalid!")

        assert.has_error(function() 
            a2:async("send", {})
        end, "Starting async method `send` failed: invalid Actor id `2`!")

        -- Fail silently right now
        a2:children()
        a2:audience("foo")
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

    it("it doesn't recycle ids which are still being referenced", function()
        local actor = Actor({}, 5)
        assert.is_equal(actor:id(), 6)

        -- lock is the same mechanism as reference
        assert.is_true(actor:lock())
        a5:remove()

        -- id is `5` because a1 was deleted above and `5` became free
        assert.is_equal(a0:child{}:id(), 5)

        -- but id here is `7`. skips `6` because it is still ref'd
        assert.is_equal(a0:child{}:id(), 7)

        assert.is_true(actor:unlock())
        assert.is_equal(a0:child{}:id(), 6)

        Actor(6):remove()
        Actor(7):remove()
    end)

    it("allows benching and joining of Actors from the Company tree", function()
        a1:bench()
        assert.are_same(a0:children(), {2, 5})
        a1:join()
        assert.are_same(a0:children(), {1, 2, 5})
    end)

    it("doesn't allow joining of non-benched actors", function()
        assert.has_error(function() 
            a5:join()
        end, "Cannot join `5`: not benched!")
    end)

    it("doesn't allow joining of benched actors with bad parents", function()
        local actor = a5:child{}
        assert.is_equal(actor:id(), 6)
        actor:bench()
        a5:remove()
        assert.has_error(function() 
            actor:join()
        end, "Cannot join `6`: bad parent!")
        actor:remove()
        assert.is_equal(a0:child{}:id(), 5)
    end)

    it("allows for a benched actor to join as a child of any parent", function()
        a5:bench()
        a5:join(1)
        assert.are_same(a0:children(), {1, 2})
        assert.are_same(a1:children(), {5})
        a5:bench()
        a5:join(0)
        assert.are_same(a0:children(), {1, 2, 5})
    end)

    it("handles the audience for each Actor", function()
        assert.are_same(a2:audience("yell"), {0, 1, 2, 3, 4, 5})
        assert.are_same(a2:audience("say"), {0, 1, 2, 5})
        assert.are_same(a2:audience("command"), {2, 3, 4})
    end)
end)
