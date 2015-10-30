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
        assert.is_equal(#actor:scripts(), 0)
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
        local scripts = actor:scripts()
        assert.is_equal(scripts[1]:probe("coordinates")[1], 3)
        assert.is_equal(scripts[1]:probe("coordinates")[2], 5)

        assert.is_equal(scripts[2]:probe("weapon"), "axe")

        assert.is_equal(scripts[3]:probe("coordinates")[1], 400)
        assert.is_equal(scripts[3]:probe("coordinates")[2], 600)
    end)

    it("can be given a list of scripts, which overwrite any previous scripts owned", function()
        local scripts = actor:scripts{ {"weapon", "flail", "north"}, {"draw", 128, 256} }
        assert.is_equal(#scripts, 2)
        assert.is_equal(scripts[1]:probe("weapon"), "flail")
        assert.is_equal(scripts[2]:probe("coordinates")[1], 128)
        assert.is_equal(scripts[2]:probe("coordinates")[2], 256)
    end)

    it("can be created from a list of scripts", function()
        actor = Dialogue.Actor.new{ {"draw", 1, 1}, {"weapon"} }
        local scripts = actor:scripts()
        assert.is_equal(#scripts, 2)
        assert.is_equal(scripts[1]:probe("coordinates")[1], 1)
        assert.is_equal(scripts[1]:probe("coordinates")[2], 1)
        assert.is_equal(scripts[2]:probe("weapon"), "dagger")
        assert.is_equal(scripts[2]:probe("direction"), "down")
    end)

    it("can create a child", function()
        local child = actor:child{ {"draw", 2, 4} }
        assert.is_equal(child:scripts()[1]:probe("coordinates")[1], 2)
        assert.is_equal(actor:children()[1]:scripts()[1]:probe("coordinates")[1], 2)
    end)

    it("can abandon its children", function()
        assert.is_equal(actor:abandon(), 1)
        assert.is_equal(#actor:children(), 0)
    end)

    it("can be given a list of children, which overwrites any children created", function()
        local children = actor:children{ 
            { {"weapon", "sword & board", "down"}, {"draw", 128, 256} },
            { {"weapon", "magic missile", "up"}, {"draw", 120, 250} },
            { {"weapon", "stupid bow", "up"}, {"draw", 115, 245} },
        }
        assert.is_equal(#children, 3)
        assert.is_equal(#actor:children(), 3)

        assert.is_equal(children[1]:scripts()[1]:probe("weapon"), "sword & board")
        assert.is_equal(children[2]:scripts()[1]:probe("weapon"), "magic missile")
        assert.is_equal(children[3]:scripts()[1]:probe("weapon"), "stupid bow")
    end)

    pending("cannot recieve a message without a mailbox")
    pending("cannot send a message without a mailbox")
    pending("can be given a mailbox")
end)

describe("A Dialogue", function()
    local dialogue = Dialogue.new{
        { {"weapon", "Crown", "North"} },
        {
            { 
                { {"draw", 400, 200} },
                {}
            },
            { 
                { {"draw", 2, 4} },
                {}
            }
        }
    }
    pending("has a method 'audience' which returns a list of actors filtered by the tone")
end)

describe("A Mailbox", function()
    local actor = Dialogue.Actor.new{ {"draw", 1, 1}, {"weapon"} }
    local mailbox = Dialogue.Mailbox.new(4):pause()

    describe("can have Envelopes", function()
        local envelope = Dialogue.Mailbox.Envelope.new(mailbox, actor, "yell", {"update"})

        it("that hold the message inside", function()
            assert.are.same(envelope:message(), {"update"})
        end)
    end)

    it("can create and add an envelope", function()
        mailbox:add(actor, "yell", {"attack", 2, 3})
        assert.is_equal(#mailbox:envelopes(), 2)
        assert.are.same(mailbox:envelopes()[2]:message(), {"attack", 2, 3})
    end)

    pending("can be started which processes envelopes as they come")
end)
