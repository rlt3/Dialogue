actor = Dialogue.Actor.new{ {"draw", 2, 4} }
Dialogue.Post.init(4, 1024)

Dialogue.Post.send(actor, 'send')
