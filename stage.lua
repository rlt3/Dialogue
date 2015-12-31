--actor = Dialogue.Actor.new{"Lead", {"draw", 2, 2}}
--print(Dialogue.Actor.new{"Lead", {"draw", 2, 2}})


--actor = Dialogue.Actor.new('super', {'foo', 'bar'})
--actor = Dialogue.Actor.new('super')
is_lead, table = Dialogue.Actor.new{"Lead", {"draw", 2, 2}}
print(is_lead);
print(table);
