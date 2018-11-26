from AlphaGomoku.play import Game, Human

black = Human(True)
white = Human()
game = Game(black, white, visualization=False, visualization_time=1)
game.play(1)
