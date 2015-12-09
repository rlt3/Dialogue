_G.Dialogue = require("Dialogue")

describe("An Actor", function()
    local actor = actor

    it("is created by passing it a table of the form {'module' [, arg1, arg2, ..., argn]}", function()
        actor = Dialogue.Actor.new{ {"draw", 400, 600} }
        assert.is_equal(1, #actor:scripts())
    end)

    it("is loaded almost immediately in its own thread", function()
        os.execute("sleep " .. tonumber(0.5))
        local script = actor:scripts()[1]
        assert.are.same({400, 600}, script:probe("coordinates"))
    end)

    it("can handle being garbage collected", function()
        actor = nil
        collectgarbage()
        actor = Dialogue.Actor.new{ {"draw", 1, 2}, {"draw", 3, 4} }
        os.execute("sleep " .. tonumber(0.5))
        assert.is_equal(2, #actor:scripts())
        assert.are.same({1, 2}, actor:scripts()[1]:probe("coordinates"))
        assert.are.same({3, 4}, actor:scripts()[2]:probe("coordinates"))
    end)

    it("can handle Scripts erroring out gracefully", function()
        local errfn = function()
            actor = Dialogue.Actor.new{ {"bad", 2, 4} }
            os.execute("sleep " .. tonumber(0.5))
            actor:scripts()[1]:probe("coordinates")
        end
        assert.has_error(errfn, "Cannot Probe: The Script's module isn't valid or has errors.")
    end)

    it("can be sent messages of the form {'method' [, arg1, arg2, ..., argn]}", function()
        actor = Dialogue.Actor.new{ {"draw", 1, 2} }
        os.execute("sleep " .. tonumber(0.5))
        actor:send{"move", 1, 1}
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same({2, 3}, actor:scripts()[1]:probe("coordinates"))
    end)

    --it(", if busy, processes any messages received on the next check", function()
    --    actor:send{"wait_move", 3, 2}
    --    os.execute("sleep " .. tonumber(0.5))
    --    actor:send{"move", 2, 2}
    --    assert.are.same({5, 5}, actor:scripts()[1]:probe("coordinates"))
    --    actor:send{"move", -7, -7}
    --    os.execute("sleep " .. tonumber(0.5))
    --    assert.are.same({0, 0}, actor:scripts()[1]:probe("coordinates"))
    --end)

    it("skips sending messages to Scripts which have errors", function()
        actor = Dialogue.Actor.new{ {"bad"}, {"draw", 250, 250} }
        os.execute("sleep " .. tonumber(0.5))
        actor:send{"move", 1, 1}
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same({251, 251}, actor:scripts()[2]:probe("coordinates"))
    end)

    it("can give a bad script a new module and definition to load", function()
        actor:scripts()[1]:load{"draw", 5, 5}
        os.execute("sleep " .. tonumber(0.5))
        actor:send{"move", 1, 1}
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same({6, 6}, actor:scripts()[1]:probe("coordinates"))
        assert.are.same({252, 252}, actor:scripts()[2]:probe("coordinates"))
    end)

    it("can handle manually reloading scripts to its original state for any reason", function()
        actor:scripts()[2]:load()
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same({250, 250}, actor:scripts()[2]:probe("coordinates"))
    end)

    it("can handle removing any script", function()
        assert.is_equal(2, #actor:scripts())
        actor:scripts()[1]:remove()
        os.execute("sleep " .. tonumber(0.5))
        assert.is_equal(1, #actor:scripts())
        assert.are.same({250, 250}, actor:scripts()[1]:probe("coordinates"))
    end)

    it("can reload all of its scripts", function()
        actor:load()
        os.execute("sleep " .. tonumber(0.5))
        assert.is_equal(1, #actor:scripts())
        actor:send{"move", -250, -250}
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same({0, 0}, actor:scripts()[1]:probe("coordinates"))
    end)

    it("has special 'Lead Actor-only' methods", function()
        local errfn = function()
            actor:receive()
        end

        assert.has_error(errfn, "attempt to call method 'receive' (a nil value)")
    end)

    describe("A Lead Actor", function()
        it("is created from a regular actor, which closes its thread", function()
            actor:lead()
            actor:send{"move", 2, 2}
            actor:send{"move", 13, 13}
            assert.are.same({0, 0}, actor:scripts()[1]:probe("coordinates"))
        end)

        it("has to process its messages manually", function()
            actor:receive()
            assert.are.same({15, 15}, actor:scripts()[1]:probe("coordinates"))
        end)

        it("can be reloaded too", function()
            actor:load()
            assert.are.same({250, 250}, actor:scripts()[1]:probe("coordinates"))
        end)
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
    
    local a = a
    local b = b
    local c = c
    local d = d
    local e = e

    it("is a tree of Actors", function()
        a = dialogue:children()[1]
        b = dialogue:children()[2]
        c = b:children()[1]
        d = b:children()[2]
        e = dialogue:children()[3]

        assert.are.same({a, b, e}, dialogue:children());
        assert.are.same({c, d}, b:children());
    end)

    it("allows for scoping of messages through audience", function()
        -- Since our Audience method exists for many threads/lua_States to call
        -- so we can't push references, but lightuserdata pointers. __eq will
        -- not get called on lightuserdata vs userdata
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

        audience = b:audience("think")
        assert.is_equal(#audience, 1)
        assert.is_equal(audience[1]:__tostring(), b:__tostring())

        audience = b:audience("yell")
        assert.is_equal(#audience, 6)
        assert.is_equal(audience[1]:__tostring(), dialogue:__tostring())
        assert.is_equal(audience[2]:__tostring(), a:__tostring())
        assert.is_equal(audience[3]:__tostring(), b:__tostring())
        assert.is_equal(audience[4]:__tostring(), c:__tostring())
        assert.is_equal(audience[5]:__tostring(), d:__tostring())
        assert.is_equal(audience[6]:__tostring(), e:__tostring())
    end)

    it("allows for Actors to send message by via Tone yell", function()
        dialogue:yell{"attack"}
        os.execute("sleep " .. tonumber(0.5))

        assert.is_equal(dialogue:scripts()[1]:probe("durability"), 9)
        assert.is_equal(c:scripts()[1]:probe("durability"), 9)
        assert.is_equal(d:scripts()[1]:probe("durability"), 9)
    end)

    it("allows for Actors to send message by via Tone command", function()
        b:command{"attack"}
        os.execute("sleep " .. tonumber(0.5))
        assert.is_equal(c:scripts()[1]:probe("durability"), 8)
        assert.is_equal(d:scripts()[1]:probe("durability"), 8)
    end)

    it("allows for Actors to send message by via Tone say", function()
        b:say{"move", 1, 1}
        os.execute("sleep " .. tonumber(0.5))

        assert.are.same(a:scripts()[1]:probe("coordinates"), {3, 5})
        assert.are.same(b:scripts()[1]:probe("coordinates"), {401, 201})
        assert.are.same(e:scripts()[1]:probe("coordinates"), {21, 7})
    end)

    it("allows for Actors to send message by via Tone think", function()
        b:think{"move", -1, -1}
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same(b:scripts()[1]:probe("coordinates"), {400, 200})
    end)

    it("allows for Actors to send message by via Tone whisper", function()
        b:whisper(e, {"move", -1, -1})
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same(e:scripts()[1]:probe("coordinates"), {20, 6})
    end)
 
    it("has an optional 'author' argument", function()
        dialogue:whisper(b, {"watch"})
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same(dialogue:__tostring(), b:scripts()[1]:probe("follow"))
    end)
end)
