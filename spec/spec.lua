_G.Dialogue = require("Dialogue")

describe("An Actor", function()
    local actor = actor

    it("is created by passing it a table of the form {'module', arg1, arg2, ..., argn}", function()
        actor = Dialogue.Actor.new{ {"draw", 400, 600} }
        assert.is_equal(1, #actor:scripts())
    end)

    it("is loaded almost immediately in its own thread", function()
        os.execute("sleep " .. tonumber(0.5))
        local script = actor:scripts()[1]
        assert.are.same({400, 600}, script:probe("coordinates"))
    end)

    it("can handle being garbage collected", function()
        actor = nil
        collectgarbage()
        actor = Dialogue.Actor.new{ {"draw", 1, 2}, {"draw", 3, 4} }
        os.execute("sleep " .. tonumber(0.5))
        assert.is_equal(2, #actor:scripts())
        assert.are.same({1, 2}, actor:scripts()[1]:probe("coordinates"))
        assert.are.same({3, 4}, actor:scripts()[2]:probe("coordinates"))
    end)

    it("can handle Scripts erroring out gracefully", function()
        local errfn = function()
            actor = Dialogue.Actor.new{ {"bad", 2, 4} }
            os.execute("sleep " .. tonumber(0.5))
            actor:scripts()[1]:probe("coordinates")
        end
        assert.has_error(errfn, "Cannot Probe: The Script's module isn't valid or has errors.")
    end)

    it("can be sent messages", function()
        actor = Dialogue.Actor.new{ {"draw", 1, 2} }
        os.execute("sleep " .. tonumber(0.5))
        actor:send{"move", 1, 1}
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same({2, 3}, actor:scripts()[1]:probe("coordinates"))
    end)

    pending("can handle removing any script")

end)
