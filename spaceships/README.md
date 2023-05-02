# Spaceships


A simple spaceships game.

Inspired by this [tutorial](https://www.parallelrealities.co.uk/tutorials/#shooter).
However, almost nothing has been copied directly.

The font is borrowed from [here](https://opengameart.org/content/bitmap-font-maker).\
The sounds are taken from [here](https://opengameart.org/content/space-shooter-sound-effects).\
The textures were handmade using GIMP.


## Running the game

The game should work on Linux and MacOS.

1. Make sure that SDL2, SDL2 Image and SDL2 Mixer are installed.
2. Run run.sh

Use arrow keys to move, space to shoot and q to exit at any point.
Press r to play again once lost.


## Screencast

![Screencast](https://raw.githubusercontent.com/oskarrrrrrr/tiny-projects-assets/main/spaceships.gif)


## Comments

To keep things simple:
- The game speed is tied to FPS which means that the gameplay can be easily broken.
However, it should work fine on most computers because the FPS are capped at 60 and the game is simple enough that even very old computers should reach that peak FPS count.
- Window size is fixed. 
- Text in the game is rendered without any external library. To keep it simple a monospaced font was used.


Other comments:
- Bullets entities are stored in an array which avoids unnecessary memory allocations.
- Other entities are stored in linked lists which seems to be a popular for simple C games.
- Textures and sounds are cached to make sure they are loaded only once. The code for
textures and sound chunks is almost the same. Maybe it could be rafactored to avoid
the duplication.
- Enumerating through some of the collections is a bit pesky and error prone.
