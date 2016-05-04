root = Actor({ {"Graphics", 400, 600}, {"Game"} }, -1, 1)

root:child{ {"Player"}, {"Paddle", 50, 100}, {"Moveable", 10, 50, 100} }
--root:child{ {"Opponent"}, {"Paddle", 500, 100}, {"Moveable", 10, 500, 100} }
--root:child{ {"Ball", 300, 150}, {"Moveable", 300, 150} }
