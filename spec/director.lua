_G.arg = {}
require 'busted.runner'()

describe("The Director", function()
    local a0

    describe("An Action", function()
        it("must have a valid Actor in the first index and a valid method in second index", function()
            assert.has_error(function() 
                Director{}
            end, "Action failed: Invalid length `0' for Action given! Length of 2 required.")

            -- Actor can be represented as an integer
            assert.has_error(function() 
                Director{0, "load"}
            end, "Action failed: Actor id `0` is an invalid reference!")

            -- Actor can be represented as a table with integer (usually with
            -- the Actor metatable attached)
            assert.has_error(function() 
                Director{ {0}, "load"}
            end, "Action failed: Actor id `0` is an invalid reference!")

            assert.has_error(function() 
                Director{ {0}, "bad"}
            end, "Action failed: Invalid method `bad' for Action given!")
        end)
    end)

    it("accepts Actions, which are serialized method calls to an Actor", function()
        a0 = Actor{ {"draw", 200, 400} }
        Director{a0, "load"}
        assert.are_same({200, 400}, a0:probe(1, "coordinates"))
    end)

end)
