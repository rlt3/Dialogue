Dialogue = require("Dialogue")

-- I want Dialogue to error out if any Script fails to load. After loading 
-- Dialogue will do the default of reporting any error and unloading scripts
-- if an error repeats `Dialogue.unload_at_error_count` times.
Dialogue.strict_load = true

-- Unload a script if it has 1 error
Dialogue.unload_at_error_count = 1

-- The number of threads we want processing actions
Dialogue.Director.worker_count = 2

-- When an actor sends a message, it has a tone. That tone is used as a key to
-- get the audience. A message almost always has many audience members. With
-- this option, those audience members can be sent the message in sequential
-- order or in any order.
Dialogue.sequential_tones = true

-- An actor can be created asynchronously in any Worker thread/main thread
-- while the system is running. Or they can be created synchronously *before*
-- the system is running in the stage file. If implicit_actors is true, it will
-- send all 'creation' actions to the main thread so one can have access to the
-- actor in the interpreter state (with complete metatables for the scripts).
-- Otherwise, actors will be created in any thread available and trying to use
-- that actor in the interpreter is undefined because its metatables might be
-- messed up.
Dialogue.implicit_actors = false

for i = 0, 1000 do
    Dialogue.Director{"new", "John"}
end

--os.execute("sleep " .. tonumber(0.25))
