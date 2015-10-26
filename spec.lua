_G.Dialogue = require("Dialogue")

describe("An Actor", function()
    local actor = Dialogue.Actor.new{}

    describe("has scripts", function()
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
end)
