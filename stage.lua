Dialogue.Post.init(2, 1024)
actor = Dialogue.Actor.new{ {"draw", 2, 4} }
Dialogue.Post.send(actor, 'send', 'update')
