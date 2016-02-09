_G.arg = {}
require 'busted.runner'()

describe("An Actor", function()
    local a = nil

    it("is created from a definition", function()
        a = Actor{"a", "definition"}
    end)

    pending("is just a reference id")
    pending("can lookup actors by strings")
end)
