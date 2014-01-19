# ephenation-client
Ephenation is a MMORPG client-server game.
The purpose of the game is to allow adventures in an unlimited world and to create their your own adventures
for others to play.
The server is designed to handle large amount (~10000) of players.

The server is designed to run on a Linux platform, and the client is design to run on both Windows and Linux
(based on OpenGL 3.3). There is now preliminary support for Oculus Rift.

See [Ephenation server wiki](https://github.com/larspensjo/ephenation-server/wiki) for server documentation
and [Ephenation client wiki](https://github.com/larspensjo/ephenation-client/wiki) for client documentation.

There is a page about the game progress at [Google+](https://plus.google.com/u/0/b/116961322217479341351/116961322217479341351/posts)
and a blog about some of the OpenGL experiences [Ephenation OpenGL](http://ephenationopengl.blogspot.se/).

![Pict](https://lh5.googleusercontent.com/-osMriYp7jLg/UGcrFi1suUI/AAAAAAAAAUY/deSaaYmIsco/s650/Valley_2012-09-30.jpeg)

## Hardware requirements

AMD and NVIDIA graphic cards are supported, but not Intel HD3000 or HD4000.

## Participation

Interested in joining the project? All talents are welcome, send mail to info@ephenation.net.

## Linux build

The [meson build system](https://sourceforge.net/projects/meson/) is used.

```mkdir build```
```meson Source build```
```cd build```
```ninja```
