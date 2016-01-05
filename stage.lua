Dialogue.Post.init(4, 1024)

for i = 1, 100000 do
   Dialogue.Post.send(actor, 'send')
end
