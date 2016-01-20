head = Actor{"Foo"}
bar = Actor({"Bar"}, head)
Actor({"Bar.Foo"}, bar)
Actor({"Bar.Bar"}, bar)
Actor({"Baz"}, head)
