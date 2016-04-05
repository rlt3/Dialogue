_G.arg = {}
require 'busted.runner'()

--
-- This tests the Director/Workers of the system and how they handle the
-- Actions which are sent between Actors. The Actors here will be tested just
-- enough so that they can load and send Actions that the Director and Workers
-- will process. See actor.lua for tests on Actors specifically and company.lua
-- for tests on how those Actors fit inside the tree.
--

function wait(n)
    -- we use sleep to wait for asynchronous actions to occur
    os.execute("sleep " .. tonumber(n))
end

describe("The Director", function()
    local a0, a1, a2, a3, a4, a5

    setup(function()
        a0 = Actor{ {"test-script", "root", 0, {}} }
        a1 = a0:child{ {"test-script", "one", 1, {}} }

        a2 = a0:child{ {"test-script", "two", 2, {}} }
        a3 = a2:child{ {"test-script", "three", 3, {}} }
        a4 = a2:child{ {"test-script", "four", 4, {}} }

        a5 = a0:child{ {"test-script", "five", 5, {}} }

        --     0
        --   / | \
        --  1  2  5
        --    / \
        --   3   4
        wait(1)
    end)

    teardown(function()
        a0:remove()
    end)

    it("doesn't throw a Lua error when given an erroneous Action", function()
        -- the asynchronous nature doesn't allow us to catch the error in a 
        -- normal workflow, but it is still caught and printed to the console
        --
        -- a valid Action is defined:
        --  { actor, method [, arg1 [, arg2 [, ... [, argN]]]] }
        --
        -- An invalid action is defined by a bad actor or invalid method or
        -- either the actor or method missing.
        Director{}
        Director{20, "load"}
        Director{ {35}, "load"}
        Director{ Actor(50), "load"}
        Director{a0, "bad"}
    end)

    after_each(function()
        -- reset the tree
        a0:load("all")
        a1:load("all")
        a2:load("all")
        a3:load("all")
        a4:load("all")
        a5:load("all")
    end)

    it("accepts Actions, which are serialized method calls to an Actor", function()
        assert.is_equal(a0:probe(1, "string"), "root")
        -- a0:send{"name_is", "tim"}
        Director{a0, "send", {"name_is", "head"}}
        wait(0.25)
        assert.is_equal(a0:probe(1, "string"), "head")
    end)

    it("is the handler the `async' method", function()
        assert.is_equal(a0:probe(1, "string"), "root")
        a0:async("send", {"name_is", "head"})
        wait(0.25)
        assert.is_equal(a0:probe(1, "string"), "head")
    end)

    it("is what the Tones use to send messages", function()
        -- think
        assert.is_equal(a0:probe(1, "string"), "root")
        a0:think{"name_is", "head"}
        wait(0.25)
        assert.is_equal(a0:probe(1, "string"), "head")

        -- whisper
        assert.is_equal(a0:probe(1, "string"), "head")
        a1:whisper(a0, {"name_is", "root"})
        wait(0.25)
        assert.is_equal(a0:probe(1, "string"), "root")

        -- yell
        assert.is_equal(a0:probe(1, "numeral"), 0)
        assert.is_equal(a1:probe(1, "numeral"), 1)
        assert.is_equal(a2:probe(1, "numeral"), 2)
        assert.is_equal(a3:probe(1, "numeral"), 3)
        assert.is_equal(a4:probe(1, "numeral"), 4)
        assert.is_equal(a5:probe(1, "numeral"), 5)
        a0:yell{"increment_by", 10}
        wait(0.50)
        assert.is_equal(a0:probe(1, "numeral"), 10)
        assert.is_equal(a1:probe(1, "numeral"), 11)
        assert.is_equal(a2:probe(1, "numeral"), 12)
        assert.is_equal(a3:probe(1, "numeral"), 13)
        assert.is_equal(a4:probe(1, "numeral"), 14)
        assert.is_equal(a5:probe(1, "numeral"), 15)
        
        -- command
        assert.is_equal(a2:probe(1, "numeral"), 12)
        assert.is_equal(a3:probe(1, "numeral"), 13)
        assert.is_equal(a4:probe(1, "numeral"), 14)
        a2:command{"increment_by", 10}
        wait(0.50)
        assert.is_equal(a2:probe(1, "numeral"), 22)
        assert.is_equal(a3:probe(1, "numeral"), 23)
        assert.is_equal(a4:probe(1, "numeral"), 24)

        -- say
        assert.is_equal(a0:probe(1, "numeral"), 10)
        assert.is_equal(a1:probe(1, "numeral"), 11)
        assert.is_equal(a2:probe(1, "numeral"), 22)
        assert.is_equal(a5:probe(1, "numeral"), 15)
        a2:say{"increment_by", 10}
        wait(0.50)
        assert.is_equal(a0:probe(1, "numeral"), 20)
        assert.is_equal(a1:probe(1, "numeral"), 21)
        assert.is_equal(a2:probe(1, "numeral"), 32)
        assert.is_equal(a5:probe(1, "numeral"), 25)
    end)

    it("allows for actions which send actions", function()
        assert.is_equal(a0:probe(1, "numeral"), 0)
        assert.is_equal(a1:probe(1, "numeral"), 1)
        assert.is_equal(a2:probe(1, "numeral"), 2)
        assert.is_equal(a3:probe(1, "numeral"), 3)
        assert.is_equal(a4:probe(1, "numeral"), 4)
        assert.is_equal(a5:probe(1, "numeral"), 5)
        a2:think{"proxy", "yell", {"increment_by", 5}}
        wait(0.50)
        assert.is_equal(a0:probe(1, "numeral"), 5)
        assert.is_equal(a1:probe(1, "numeral"), 6)
        assert.is_equal(a2:probe(1, "numeral"), 7)
        assert.is_equal(a3:probe(1, "numeral"), 8)
        assert.is_equal(a4:probe(1, "numeral"), 9)
        assert.is_equal(a5:probe(1, "numeral"), 10)
    end)
end)
