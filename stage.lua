a0 = Actor{ {"draw", 200, 400} }
a0:load()
a0:send{"move", 2, 2}
print(a0:probe(1, "coordinates")[1])
print(a0:probe(1, "coordinates")[2])
