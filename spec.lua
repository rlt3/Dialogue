_G.Mailbox = require("Mailbox")

describe("A Mailbox", function()
    local mailbox = Mailbox.new(8)

    it("can accept an envelope", function()
        count = mailbox:add{"foo", "bar"}

        os.execute("sleep " .. tonumber(0.5))
        assert.is_equal(count, 1)
    end)

    it("processes them automatically", function()
        mailbox:add{"amazing", "grace"}
        mailbox:add{"rolling", "stone"}
        mailbox:add{"all", "along", "the", "watchtower"}

        os.execute("sleep " .. tonumber(4))
        assert.is_equal(mailbox:count(), 0)
    end)
end)
