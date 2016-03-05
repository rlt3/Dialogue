a0 = Actor{ {"draw", 200, 400} }
a0:load()
a0:send{"move", 2, 2}
print(a0:probe(1, "coordinates")[1])
print(a0:probe(1, "coordinates")[2])
Director({a0, "send", {"update"}}, 2)
Director({a0, "send", {"move", 1}}, 2)
Director({a0, "bad"}, 2)
Director{a0}
Director{37, "send", {"update"}}
os.execute("sleep " .. tonumber(1))
