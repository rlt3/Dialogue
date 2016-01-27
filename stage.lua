head = Actor{"Foo"}
bar = Actor({"Bar"}, head)
Actor({"Bar.Foo"}, bar)
Actor({"Bar.Bar"}, bar)
baz = Actor({"Baz"}, head)
--bar:delete()
