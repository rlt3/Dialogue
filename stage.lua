_G.arg = {}
require 'busted.runner'()

describe("The Tree of Actors", function()
    local a0
    local a1
    local a2
    local a3
    local a4

    it("assigns ids to all created actors", function()
        a0 = Actor({})
        a1 = a0:child{}
        a2 = a1:child{}
        a3 = a1:child{}
        a0:child{}
        a4 = a0:child{}

        assert.is_equal(a0:id(), 0)
        assert.is_equal(a1:id(), 1)
        assert.is_equal(a2:id(), 2)
        assert.is_equal(a3:id(), 3)
        assert.is_equal(a4:id(), 5)
    end)

    it("can delete any actor by id", function()
        a3:delete()
        a3 = nil
    end)

    it("reuses garbage ids when creating new actors", function()
        -- since 3 (a3's id) isn't being used anymore, this will 'fill its spot'
        -- a3 will also become a child of a0 whereas before it was a child of a1
        a3 = a0:child{}
        assert.is_equal(a3:id(), 3)
    end)

    it("doesn't reuse ids which are garbage but still being referenced", function()
        -- a1 now has 1 child (a2). deleting a1 will mark a1 and its descendents
        -- as garbage. Before that, we reference (lock) a2 so it won't be
        -- cleaned up when another Actor is created
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
end)
