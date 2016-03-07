-- The number of Actor which can be created before resizing.
-- Default is 50
Dialogue.Company.base = 100

-- Maximum number of Actors. default is 1000
Dialogue.Company.max = 2000

-- The factor at which the Company resizes after going over the base.
-- E.g. if `company_base` is 10 and `company_resize_factor` is 2, then the size
-- of the company will be 20 after resizing, then 40, then 80, etc.
-- Default is 2
Dialogue.Company.resize_factor = 1

-- Automatically send an Action to load the just-created Actor
-- default is 1 (true)
Dialogue.Actor.auto_load = 0

-- Maximum number of Scripts for an Actor. default is 10
Dialogue.Actor.max_scripts = 50

-- Maximum number of Workers (threads). default is 4
Dialogue.Director.max_workers = 8

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
