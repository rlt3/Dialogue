_G.arg = {}
require 'busted.runner'()

describe("An Actor object", function()
    --it("will error on creation if not given correct Script definitions", function()
    --    assert.has_error(function() 
    --        Actor{ {"good"}, "bad" }
    --    end, "Failed to create script: `bad` isn't a table!")

    --    assert.has_error(function() 
    --        Actor{ {"good"}, {}, {"another good"} }
    --    end, "Failed to create script: invalid definition!")
    --end)

    it("can be created by providing the id of an existing Actor", function()
        local a0 = Actor{}
        assert.is_equal(a0:id(), 0)
        a0 = nil
        a0 = Actor(0)
        assert.is_equal(a0:id(), 0)
        a0:remove()
    end)

    it("will error on load if the actor id is invalid", function()
        assert.has_error(function() 
            Actor(20):load()
        end, "Actor id `20` is an invalid reference!")
    end)

    it("will error on load if the any Script has an invalid module", function()
        local a0 = Actor{ {"invalid-module"} }
        assert.is_equal(a0:id(), 0)
        assert.has_error(function() 
            a0:load()
        end, "Cannot load module `invalid-module': require failed")
        a0:remove()
    end)

    it("will error on load if the any Script has a module with no new function", function()
        local a0 = Actor{ {"module-no-new"} }
        assert.is_equal(a0:id(), 0)
        assert.has_error(function() 
            a0:load()
        end, "Cannot load module `module-no-new': `new' is not a function!")
        a0:remove()
    end)

    it("cannot be probed if not loaded", function()
        local a0 = Actor{ {"module-no-new"} }
        assert.is_equal(a0:id(), 0)
        assert.has_error(function() 
            a0:probe(1, "coordinates")
        end, "Cannot probe `coordinates': not loaded!")
        a0:remove()
    end)

    it("cannot be sent a message if there are zero loaded Scripts", function()
        local a0 = Actor{ {"module-no-new"} }
        assert.is_equal(a0:id(), 0)
        -- Will only throw this error if there are no loaded Scripts -- as in
        -- if there's 1 out of 50 loaded Scripts, it won't throw an error.
        assert.has_error(function() 
            a0:send{"move", 2, 2}
        end, "Actor `0' has no loaded Scripts!")
        a0:remove()
    end)

    it("will error on any function with a bad message format", function()
        local a0 = Actor{ {"draw", 200, 400}, {"draw", 2, 4} }
        a0:load()
        assert.has_error(function() 
            a0:send{"move"}
        end, "attempt to perform arithmetic on local 'x' (a nil value)")
        a0:remove()
    end)

    it("can be sent messages which affects the real Actor's state", function()
        local a0 = Actor{ {"draw", 200, 400} }
        a0:load()
        a0:send{"move", 2, 2}
        assert.are_same({202, 402}, a0:probe(1, "coordinates"))
        a0:remove()
    end)
    
    it("will unload any Scripts that have errored", function()
        local a0 = Actor{ {"draw", 200, 400} }
        a0:load()

        -- an invalid message call
        assert.has_error(function() 
            a0:send{"move", 2}
        end, "attempt to perform arithmetic on local 'y' (a nil value)")

        assert.has_error(function() 
            a0:probe(1, "coordinates")
        end, "Cannot probe `coordinates': not loaded!")
        a0:remove()
    end)

    it("can reload any unloaded Scripts", function()
        local a0 = Actor{ {"draw", 200, 400} }
        a0:load()

        -- an invalid message call
        assert.has_error(function() 
            a0:send{"move", 2}
        end, "attempt to perform arithmetic on local 'y' (a nil value)")

        assert.has_error(function() 
            a0:probe(1, "coordinates")
        end, "Cannot probe `coordinates': not loaded!")

        a0:load(1)
        assert.are_same(a0:probe(1, "coordinates"), {200, 400})
        a0:remove()
    end)

    it("will unload first Script that errors from a message then stop processing message", function()
        local a0 = Actor{ {"draw", 200, 400}, {"draw", 2, 4} }
        assert.is_equal(a0:id(), 0)
        a0:load()

        -- an invalid message call
        assert.has_error(function() 
            a0:send{"move", 2}
        end, "attempt to perform arithmetic on local 'y' (a nil value)")

        -- now send a valid message, only the second script gets updated
        a0:send{"move", 2, 2}
        assert.has_error(function() 
            a0:probe(1, "coordinates")
        end, "Cannot probe `coordinates': not loaded!")

        assert.are_same(a0:probe(2, "coordinates"), {4, 6})

        a0:remove()
    end)
end)
