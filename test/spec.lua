_G.Mailbox = require("Mailbox")

describe("A Mailbox", function()
    local mailbox = Mailbox.new(8)

    it("can accept an envelope", function()
        envelopes = mailbox:add{"foo", "bar"}
        assert.is_equal(#envelopes, 1)
    end)
end)
