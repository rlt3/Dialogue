_G.Dialogue = require("Dialogue")

describe("a Script", function()
    local script = Dialogue.Actor.Script.new{ "weapon", "axe", "down" }

    it("fails sending if not loaded", function()
        local errfn = function()
            script:send{ "attack" }
        end
        assert.has_error(errfn, "Script isn't loaded!")
    end)

    it("holds internal state", function()
        script:load()
        script:send{ "attack" }
        assert.is_equal(script:probe("durability"), 9)
    end)

    it("can be reloaded to its initial state", function()
        script:load()
        assert.is_equal(script:probe("durability"), 10)
    end)

    it("can be reloaded by giving it a new initial state", function()
        script:load{ "weapon", "sword", "up" }
        assert.is_equal(script:probe("weapon"), "sword")
    end)
end)
