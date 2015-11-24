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

describe("A Mailbox", function()
    local actor = Dialogue.Actor.new{ {"draw", 1, 1}, {"weapon"} }
    local mailbox = Dialogue.Mailbox.new(8)
    
    describe("can have Envelopes", function()
        local envelope = Dialogue.Mailbox.Envelope.new(actor, "think", {"move", 20, 1000})

        it("that hold the message inside", function()
            assert.are.same(envelope:message(), {"move", 20, 1000})
        end)
    end)

    it("can accept an envelope", function()
        mailbox:add(actor, "think", {"move", 20, 1000})

        os.execute("sleep " .. tonumber(0.5))
        assert.are.same(actor:scripts()[1]:probe("coordinates"), {21, 1001})
    end)

    it("processes the Envelopes it receives automatically", function()
        mailbox:add(actor, "think", {"move", 15, 20})
        mailbox:add(actor, "think", {"move", 5, 0})

        os.execute("sleep " .. tonumber(0.5))
        assert.is_equal(mailbox:count(), 0)
        assert.are.same(actor:scripts()[1]:probe("coordinates"), {41, 1021})
    end)
end)

describe("A Dialogue", function()
    local dialogue = Dialogue.new{
        { {"weapon", "Crown", "North"} },
        {
            { 
                { {"draw", 2, 4} },
                {}
            },
            { 
                { {"draw", 400, 200} },
                {
                    { 
                        { {"weapon", "bullet", "south"} },
                        {}
                    },
                    { 
                        { {"weapon", "bomb", "south-east"} },
                        {}
                    }
                }
            },
            { 
                { {"draw", 20, 6} },
                {}
            }
        }
    }

    -- The 'form' of the tree:
    --    dialogue
    --     / | \
    --    a  b  e
    --      / \
    --     c   d
    
    local a = dialogue:children()[1]
    local b = dialogue:children()[2]
    local c = b:children()[1]
    local d = b:children()[2]
    local e = dialogue:children()[3]

    it("can be created from a table of tables", function()
        assert.is_equal(dialogue:scripts()[1]:probe("weapon"), "Crown")
        assert.are.same(a:scripts()[1]:probe("coordinates"), {2, 4})
        assert.are.same(b:scripts()[1]:probe("coordinates"), {400, 200})
        assert.is_equal(c:scripts()[1]:probe("weapon"), "bullet")
        assert.is_equal(d:scripts()[1]:probe("weapon"), "bomb")
        assert.are.same(e:scripts()[1]:probe("coordinates"), {20, 6})
    end)

    it("has a method 'audience' which returns a list of actors filtered by the tone", function()
        -- Since we're working with objects (Actors, Scripts, Envelopes, &c)
        -- which get transfered (by pointer) through many Lua stacks, our 
        -- heavy userdata might be represented as light userdata as it has 
        -- made rounds through the system. Since the __eq metamethod doesn't
        -- get called for comparison of heavy vs light userdata, we simply
        -- do this string comparison of output to see they are the same.
        local audience = b:audience("say")
        assert.is_equal(#audience, 4)
        assert.is_equal(audience[1]:__tostring(), dialogue:__tostring())
        assert.is_equal(audience[2]:__tostring(), a:__tostring())
        assert.is_equal(audience[3]:__tostring(), b:__tostring())
        assert.is_equal(audience[4]:__tostring(), e:__tostring())

        audience = b:audience("command")
        assert.is_equal(#audience, 2)
        assert.is_equal(audience[1]:__tostring(), c:__tostring())
        assert.is_equal(audience[2]:__tostring(), d:__tostring())

        audience = b:audience("yell")
        assert.is_equal(#audience, 6)
        assert.is_equal(audience[1]:__tostring(), dialogue:__tostring())
        assert.is_equal(audience[2]:__tostring(), a:__tostring())
        assert.is_equal(audience[3]:__tostring(), b:__tostring())
        assert.is_equal(audience[4]:__tostring(), c:__tostring())
        assert.is_equal(audience[5]:__tostring(), d:__tostring())
        assert.is_equal(audience[6]:__tostring(), e:__tostring())
    end)

    it("has a Mailbox which can be accessed by every Actor", function()
        local mailbox = dialogue:mailbox()

        assert.is_equal(a:mailbox(), mailbox)
        assert.is_equal(a:mailbox(), mailbox)
        assert.is_equal(b:mailbox(), mailbox)
        assert.is_equal(c:mailbox(), mailbox)
        assert.is_equal(d:mailbox(), mailbox)
        assert.is_equal(e:mailbox(), mailbox)
    end)

    it("allows for its Actors to send messages via their audience", function()
        local mailbox = dialogue:mailbox()

        assert.is_equal(c:scripts()[1]:probe("durability"), 10)
        assert.is_equal(d:scripts()[1]:probe("durability"), 10)

        mailbox:add(b, "command", {"attack"})
        os.execute("sleep " .. tonumber(0.5))

        assert.is_equal(c:scripts()[1]:probe("durability"), 9)
        assert.is_equal(d:scripts()[1]:probe("durability"), 9)
    end)

    it("can handle Actors sending messages which send messages", function()
        b:think{"move", 2, 2}
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same(b:scripts()[1]:probe("coordinates"), {402, 202})

        dialogue:yell{"walk"}
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same(a:scripts()[1]:probe("coordinates"), {4, 6})
        assert.are.same(b:scripts()[1]:probe("coordinates"), {404, 204})
        assert.are.same(e:scripts()[1]:probe("coordinates"), {22, 8})
    end)
end)
