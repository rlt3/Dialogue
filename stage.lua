_G.arg = {}
require 'busted.runner'()

describe("The Tree of Actors", function()
    local a0
    local a1
    local a2
    local a3
    local a4

    it("gives all created actors ids", function()
        a0 = Actor()
        a1 = Actor(a0)
        a2 = Actor(a1)
        a3 = Actor(a1)
        Actor(a0)
        a4 = Actor(a0)

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

    it("reuses unused ids on creations", function()
        -- since 3 (a3's id) isn't being used anymore, this will 'fill its spot'
        -- a3 will also become a child of a0 whereas before it was a1
        a3 = Actor(a0)
        assert.is_equal(a3:id(), 3)
    end)

    it("skips checked-out unused ids", function()
        -- a1 now has 1 children. deleting a1 will mark a1 and its descendents
        -- as garbage. Before that, we reference (checkout) a2 so it won't be
        -- cleaned up when another Actor is created
        if a2:ref() then
            a1:delete()
            -- 1 was a1's unused spot. Even tho 2 was the next unused, it was 
            -- checked-out, so it found the next unused: 6
            assert.is_equal(Actor(a4):id(), 1)
            assert.is_equal(Actor(a4):id(), 6)
            a2:deref()
        end
    end)
end)
