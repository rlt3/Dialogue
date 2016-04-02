--game = Actor.lead{ {"Graphics"} }
game = Actor({ {"Graphics"} }, -1, 1)
game:child{ {"Paddle", 500, 100}, {"Moveable", 10, 500, 100} }
game:child{ {"Paddle", 50, 100}, {"Moveable", 10, 50, 100} }
game:child{ {"Ball", 300, 150} }
