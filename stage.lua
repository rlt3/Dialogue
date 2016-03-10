a0 = Actor{ {"draw", 200, 400} }
a1 = Actor{ {"draw", 200, 400} }
a2 = Actor{ {"draw", 200, 400} }
a0:load()
a1:load()
a2:load()
Director{a0, "send", {"output", "foobar", 3}}
Director{a1, "send", {"output", "bar", 2}}
Director{a2, "send", {"output", "foo", 1}}
--os.execute("sleep " .. tonumber(7))
--a0:send{"move", 2, 2}
--print(a0:probe(1, "coordinates")[1])
--print(a0:probe(1, "coordinates")[2])
--Director({a0, "send", {"update"}}, 2)
--Director({a0, "send", {"move", 1}}, 2)
--Director({a0, "bad"}, 2)
--Director{a0}
--Director{37, "send", {"update"}}
