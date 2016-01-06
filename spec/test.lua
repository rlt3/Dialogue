_G.Dialogue = require("Dialogue")

Dialogue.Post.init(2, 1024)

describe("The Post", function()
    local actor = Dialogue.Actor.new{ {"draw", 2, 4} }
    os.execute("sleep " .. tonumber(0.5))

    pending("requires an actor and action")

    it("can handle the `load' action", function()
        assert.is_equal(2, actor:scripts():nth(1):probe("coordinates")[1])
    end)

    it("can handle the `send' action", function()
        Dialogue.Post.send(actor, 'send', 'move', 2, 2)
        os.execute("sleep " .. tonumber(0.5))
        assert.is_equal(4, actor:scripts():nth(1):probe("coordinates")[1])
    end)
end)
