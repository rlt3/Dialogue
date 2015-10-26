_G.Dialogue = require("Dialogue")

describe("An Actor", function()
    local actor = Dialogue.Actor.new{}

    describe("can have scripts", function()
        local script = script

        it("that need at least one element in table given", function()
           local errfn = function()
                script = Dialogue.Actor.Script.new(actor, {})
            end
            assert.has_error(errfn, "bad argument #2 to 'new' (Table needs to have a module name!)")
        end)

        it("that need that one element to be the name of a valid Lua module", function()
            local errfn = function()
                script = Dialogue.Actor.Script.new(actor, { "invalid_module" })
                script:load()
            end
            assert.has_error(errfn, "Require failed for module 'invalid_module'")
        end)

        it("that need that module to return a table with function element new", function()
            local errfn = function()
                script = Dialogue.Actor.Script.new(actor, { "valid_module_no_new" })
                script:load()
            end
            assert.has_error(errfn, "valid_module_no_new.new() failed")
        end)

        it("that will fail sending if not loaded", function()
            local errfn = function()
                script = Dialogue.Actor.Script.new(actor, { "weapon", "axe", "down" })
                script:send{ "attack" }
            end
            assert.has_error(errfn, "Script isn't loaded!")
        end)

        it("that hold private, internal state responding only to and by messages", function()
            script:load()
            script:send{ "attack" }
            assert.is_equal(script:probe("durability"), 9)
        end)

        it("that can be reloaded to their initial states", function()
            script:load()
            assert.is_equal(script:probe("durability"), 10)
        end)

        it("that can be reloaded by giving new initial states", function()
            script:load{ "weapon", "sword", "up" }
            assert.is_equal(script:probe("weapon"), "sword")
        end)
    end)

    it("can remove all the scripts it owns", function()
        assert.is_equal(actor:drop(), 3)
    end)

    it("can be given a script directly, which is automatically loaded", function()
        script = actor:give{ "draw", 2, 4 }
        script:send{"move", 1, 1}
        assert.is_equal(script:probe("coordinates")[1], 3)
        assert.is_equal(script:probe("coordinates")[2], 5)
    end)

    it("can have any number of any script", function()
        actor:give{ "weapon", "axe", "up" }
        actor:give{ "draw", 400, 600 }
        assert.is_equal(#actor:scripts(), 3)
    end)

    it("keeps each script mutually exclusive", function()
        assert.is_equal(actor:scripts()[1]:probe("coordinates")[1], 3)
        assert.is_equal(actor:scripts()[1]:probe("coordinates")[2], 5)

        assert.is_equal(actor:scripts()[2]:probe("weapon"), "axe")

        assert.is_equal(actor:scripts()[3]:probe("coordinates")[1], 400)
        assert.is_equal(actor:scripts()[3]:probe("coordinates")[2], 600)
    end)

    pending("can be given a list of scripts, which overwrite any previous scripts owned")
    pending("can be given a child")
    pending("can be given a list of children")
    pending("automatically loads any scripts given")
    pending("cannot recieve a message without a mailbox")
    pending("cannot send a message without a mailbox")
    pending("can be given a mailbox")

end)
