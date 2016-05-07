--root = Actor({ {"Graphics", 400, 600}, {"Game"} }, -1, 1)
--
--root:child{ {"Player"}, {"Paddle", 50, 100}, {"Moveable", 10, 50, 100} }
--root:child{ {"Opponent"}, {"Paddle", 500, 100}, {"Moveable", 10, 500, 100} }
--root:child{ {"Ball", 300, 150}, {"Moveable", 300, 150} }

--m = Actor{ {"Body", { w = 10, h = 10, x = 50, y = 50 }} }

game = Actor({ {"Game"} }, -1, 1)
m = Actor{ {"Body", 10, 10, 50, 50} }
