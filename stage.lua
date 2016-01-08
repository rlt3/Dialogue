Dialogue = require("Dialogue")

-- I want Dialogue to error out if any Script fails to load. After loading 
-- Dialogue will do the default of reporting any error and unloading scripts
-- if an error repeats `Dialogue.unload_at_error_count` times.
Dialogue.strict_load = true

-- Unload a script if it has 1 error
Dialogue.unload_at_error_count = 1

-- The number of threads we want processing actions
Dialogue.worker_count = 1

-- When an actor sends a message, it has a tone. That tone is used as a key to
-- get the audience. A message almost always has many audience members. With
-- this option, those audience members can be sent the message in sequential
-- order or in any order.
Dialogue.sequential_tones = true

Dialogue{"new", "Matt"}
Dialogue{"new", "John"}
Dialogue{"new"}
Dialogue{"new", "Kim"}
Dialogue{"new", "Jenny"}
