_G.Dialogue = require("Dialogue")

describe("An Actor", function()
    local actor = actor

    it("is created by passing it a table of the form {'module', arg1, arg2, ..., argn}", function()
        actor = Dialogue.Actor.new{ {"draw", 400, 600} }
        assert.is_equal(1, #actor:scripts())
    end)

end)
