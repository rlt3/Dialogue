_G.Dialogue = require("Dialogue")

describe("a Script", function()
    local script = script

    it("needs at least one element in state table given", function()
        local errfn = function()
            script = Dialogue.Actor.Script.new{}
        end
        assert.has_error(errfn, "bad argument #1 to 'new' (Table needs to have a module name!)")
    end)

    it("fails sending if not loaded", function()
        local errfn = function()
            script = Dialogue.Actor.Script.new{ "weapon", "axe", "down" }
            script:send{ "attack" }
        end
        assert.has_error(errfn, "Script isn't loaded!")
    end)

    it("needs a valid lua object table with a new function to be loaded", function()
        local errfn = function()
            script = Dialogue.Actor.Script.new{ "collision" }
            script:load()
        end
        assert.has_error(errfn, "Require failed for module collision")
    end)

    it("holds private, internal state responding only to and by messages", function()
        script = Dialogue.Actor.Script.new{ "weapon", "axe", "down" }
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
