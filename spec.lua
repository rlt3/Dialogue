_G.Dialogue = require("Dialogue")

describe("a Script", function()
    local script = script

    it("needs at least one element in state table given", function()
        local errfn = function()
            script = Dialogue.Actor.Script.new{}
        end
        assert.has_error(errfn, "bad argument #1 to 'new' (Table needs to have a module name!)")
    end)

    it("needs that one element to be the name of a valid Lua module", function()
        local errfn = function()
            script = Dialogue.Actor.Script.new{ "invalid_module" }
            script:load()
        end
        assert.has_error(errfn, "Require failed for module 'invalid_module'")
    end)

    it("needs that module to return a table with function element new", function()
        local errfn = function()
            script = Dialogue.Actor.Script.new{ "valid_module_no_new" }
            script:load()
        end
        assert.has_error(errfn, "valid_module_no_new.new() failed")
    end)

    it("fails sending if not loaded", function()
        local errfn = function()
            script = Dialogue.Actor.Script.new{ "weapon", "axe", "down" }
            script:send{ "attack" }
        end
        assert.has_error(errfn, "Script isn't loaded!")
    end)

    it("holds private, internal state responding only to and by messages", function()
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

describe("an Actor", function()
    local actor = actor

    it("can have any number of scripts", function()
        actor = Dialogue.Actor.new{ {"draw", 0, 2}, {"weapon", "sword", "up"} }
        actor = Dialogue.Actor.new{ {"weapon", "sword", "up"} }
        actor = Dialogue.Actor.new{}
    end)

    it("can be given a script", function()
        actor:give{"weapon", "scimitar", "north"}
        assert.is_equal(#actor:scripts(), 1)
    end)
    
    it("can be given a list of scripts, which overwrite any previous scripts owned", function()
        actor:give{ {"weapon", "scimitar", "north"}, {"draw", 2, 4}, {"collision", 2, 4} }
        assert.is_equal(#actor:scripts(), 3)
    end)

    pending("can be given a child")
    pending("can be given a list of children")
    pending("automatically loads any scripts given")
    pending("cannot recieve a message without a mailbox")
    pending("cannot send a message without a mailbox")
    pending("can be given a mailbox")
end)
