actor = Dialogue.Actor.new{ "Lead", { "draw", 2, 2 } }
print(actor)
for i = 1, #actor:scripts() do
    print(actor:scripts()[i])
end
print(actor:scripts():nth(1):probe("coordinates")[1])
--Dialogue.Post.init(4, 1024)
Dialogue.Post.init()
Dialogue.Post.send(actor, 'send')
