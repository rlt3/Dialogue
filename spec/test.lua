_G.Dialogue = require("Dialogue")

Dialogue.Post.init(2, 1024)

describe("The Post", function()
    local actor = Dialogue.Actor.new{ {"draw", 2, 4} }
    os.execute("sleep " .. tonumber(0.5))

    pending("requires an actor and action")

    it("can handle the `load' action", function()
        assert.is_equal(2, actor:scripts():nth(1):probe("coordinates")[1])
    end)

    it("can handle the `send' action", function()
        Dialogue.Post.send(actor, 'send', 'move', 2, 2)
        os.execute("sleep " .. tonumber(0.5))
        assert.is_equal(4, actor:scripts():nth(1):probe("coordinates")[1])
    end)
end)

describe("A Dialogue", function()
    local dialogue = Dialogue.new{
        { {"weapon", "Crown", "North"} },
        {
            { 
                { "Lead", {"draw", 2, 4} },
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
    os.execute("sleep " .. tonumber(1.0))

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

    it("can be created from a mix of Lead and regular Actors", function()
        assert.are.same(a:scripts()[1]:probe("coordinates"), {2, 4})
    end)

    it("allows for Actors to send message by via Tone yell", function()
        dialogue:yell("attack")
        os.execute("sleep " .. tonumber(0.5))

        assert.is_equal(dialogue:scripts()[1]:probe("durability"), 9)
        assert.is_equal(c:scripts()[1]:probe("durability"), 9)
        assert.is_equal(d:scripts()[1]:probe("durability"), 9)
    end)

    it("allows for Actors to send message by via Tone command", function()
        b:command("attack")
        os.execute("sleep " .. tonumber(0.5))
        assert.is_equal(c:scripts()[1]:probe("durability"), 8)
        assert.is_equal(d:scripts()[1]:probe("durability"), 8)
    end)

    it("allows for Actors to send message by via Tone say", function()
        b:say("move", 1, 1)
        os.execute("sleep " .. tonumber(0.5))

        --a:receive()
        assert.are.same(a:scripts()[1]:probe("coordinates"), {3, 5})
        assert.are.same(b:scripts()[1]:probe("coordinates"), {401, 201})
        assert.are.same(e:scripts()[1]:probe("coordinates"), {21, 7})
    end)

    it("allows for Actors to send message by via Tone think", function()
        b:think("move", -1, -1)
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same(b:scripts()[1]:probe("coordinates"), {400, 200})
    end)

    it("allows for Actors to send message by via Tone whisper", function()
        b:whisper(e, "move", -1, -1)
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same(e:scripts()[1]:probe("coordinates"), {20, 6})
    end)

    it("can handle Actors sending messages which send messages", function()
        b:think("move", 2, 2)
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same(b:scripts()[1]:probe("coordinates"), {402, 202})

        b:whisper(e, "bump")
        os.execute("sleep " .. tonumber(0.5))
        assert.are.same(e:scripts()[1]:probe("coordinates"), {19, 5})
        assert.are.same(b:scripts()[1]:probe("coordinates"), {401, 201})
    end)
 
    --it("has an optional 'author' argument", function()
    --    dialogue:whisper(b, "watch")
    --    os.execute("sleep " .. tonumber(0.5))
    --    assert.are.same(dialogue:__tostring(), b:scripts()[1]:probe("follow"))
    --end)
end)
