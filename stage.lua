head = Actor()
one = Actor(head)
two = Actor(one)
three = Actor(one)
Actor(three)
five = Actor(head)

if two:ref() then
    one:delete()
    Actor(five)
    Actor(five)
    two:deref()
end
