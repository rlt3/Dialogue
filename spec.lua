_G.Mailbox = require("Mailbox")

describe("A Mailbox", function()
    local mailbox = Mailbox.new(8)

    it("can accept an envelope", function()
        count = mailbox:add{"foo", "bar"}
        assert.is_equal(count, 1)
    end)
end)
