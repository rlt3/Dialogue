Dialogue.Post.init(2, 1024)
actor = Dialogue.Actor.new{ {"draw", 2, 4} }
os.execute("sleep " .. tonumber(0.5))
print(actor:scripts():nth(1):probe("coordinates")[1])
