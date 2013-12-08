Copyright 2012-2013 The Ephenation Authors

This file is part of Ephenation.

Ephenation is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3.

Ephenation is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Ephenation.  If not, see <http://www.gnu.org/licenses/>.

Ephenation Readme
=================
* Adventure, kill monsters, go up in level
* Monsters get progressively harder from the starting point
* Find and use better equipments
* Find a free area, allocate a territory of your own and redesign it as you want
* Make an exciting adventure for others, or a place to invite friends to
* Look at the high scores of popular areas, to find places to go adventure
* To kill a monster, use TAB to select it and initiate attack with '1'.

Release 5.0b 2013-12-08
=======================
* Support for Oculus Rift

Release 4.3 2013-02-24
======================
* Replace all dialogs with in-game dialogs.
* Add a few animated monsters.
* New block types: colored lights (red, green or blue).
* Enable Undo and Redo from the construction mode.
* When the game is stared, the last world position is shown in the background.
* Improve shadows.
* The long distance view cut-off is replaced with a fog of distance.
* F11 takes a snapshot of the screen, and saves in the Pictures folder.
* Option --hidegui runs the game without the GUI.
* Numerous minor bug fixes.

Release 4.2 2012-09-29
======================
This is a maintenance release, to improve functionality but no new features.
* Improved FPS from drawing trees.
* Improved memory usage, both on CPU and GPU.
* Enhanced fog display, smoother borders and visibility against the sky background.
* Possibility to calibrate lighting.
* Animation when using magical portals.
* Changing graphical settings that require game restart is clarified.
* Enhanced default settings (with smooth terrain).
* Option to use VSYNC.

Release 4.1 2012-08-21
======================
* Improved underground caves, mostly connected to each other.
* Showing sword when wielded.
* New block types: Tree bark, Tuft of grass and flowers.
* Animated monsters and player.
* Speed up display of chunks instead of blue sky.
* Basic in-game dialogs.
* Show waves on the water.
* New option for showing the world as smooth instead of blocky.
* Live shadows.
* Use smooth player movemement.
* Improved map ('m'), that can also be zoomed with the mouse wheel.
* Show the player and monsters on the map.

Release 4.0 2012-05-05
======================
* You can now use magical portals, if you have high enough level
* Flying is only possible in your own territory
* New block type: Quest symbol (rotating exclamation mark)
  Use this symbol to indicate that there is an adventure
* New block type: Reward symbol (rotating coin)
  Use this symbol to indicate an immediate reward, not to lure the player into traps
* Enable keyboard auto repeat when typing commands or messages
* Improve healing spell
* The "/territory claim" command can now be used with the arguments north, south, west and east
* Traps in your territory are now hidden from other players
* Enable double click in the inventory to use items
* The game will warn if there is a newer version available
* Default inhibit for activator blocks is now longer than 10 seconds if there are rewards

Release 3.3 2012-04-01
======================
* You can now see the name and level of other near players
* Activator blocks can turn other blocks invisible
* Drop unused items and get experience points for them
* Possible to configure ambient light
* Resurrection point can be updated with a scroll
* Activator blocks can have level depending conditions
* You now get score when someone explores your territory (/score)
*   The high score is shown in the web page
*   You get points for others being in your territory
*   You also get points for others getting damaged by monsters
*   You need enough points to be able to give rewards
* A ping information can be activated from the options menu
* A login failure now results in a message, instead of only shutting own the program

Release 3.2 2012-02-26
======================
* Swimming when in water
* Scrolling damage text
* Selected monster level information
* Improved graphics with less lagg during startup
* Third person view (using mouse scroll)
* Show weapon on player when in third person view
* Fix problem with sound not being updated sometimes
* Inventory screen shows resulting stats from equipment
* Implement drag-and-drop in inventory screen
* Full screen mode
* Armors and helmets can now drop from random monsters
* Armors and helmets will now change the player stats
* New players now start in a special start area

Release 3.1 2012-01-22
======================
* New block types: small and big fog.
* There are now weapons and armor that can be equipped, and an inventory screen ('i').
* Using weapons will increase damage in fights.
* Weapons and armor has a level that depends on where they were found.
* High quality equipment lasts more levels than low quality equipment.
* Use quick buttons for using items (F1, F2, etc).
* Improved message window, texts no longer flowing outside of window width.
* Speed up construction mode.
* Improved lighting from lamps.
* Shadows beneath monsters and trees.
* Improved rendering speed, and 4 different configurable performances. This is controlled
  from the options dialog.
* Using special blue lighting below water.
* Better looking trees.
* Increase minimal distance where a fight can be started with monsters.

Release 3.0 2011-11-27
======================
* There is now an inventory (use 'i').
* Potions o mana and health can drop from monsters.
* Use items in the inventory with functions keys (F1, F2, etc).
  Key for toggling construction mode is now the 'C' button.
* Improved digging tool, (press mouse button and 'v' at the same time).
* New construction blocks: concrete, gravel and tiled stones.
* Monsters are small to fit in tight areas.
* Test button for sound in block activation.
* Allocate territory above and below with "/territory claim up" and "/territory claim down".
* List keys with command "/keys".
* ESC will now shop options dialog.
* 'm' key will show a map.

Release 2.13 2011-11-06
=======================
* Text activator can now create a key for the player. Every key is identified by the creator and a kd ID.
  A player can carry up to 10 keys (there is no inventory display support yet).
* The text activator can use a condition that depends on the player having a specific key.
* A failed test for a key can produce an optional message.
* Enabled the broadcast option for text activators.
* The default inhibit for the next text activation is 10s, but can now be set to any value.
* When changing to and from construction mode, the screen is now properly updated.
* The Help and Credits dialogs now have Close buttons.
* Added 19 sound effects that can be commanded from the text activator.
* Enabled the monster spawer from text activators.
  Monster level (from default-2 to +2) can be selected from a text activator.
* The dialopg used for creating text activators now have tooltips for most functions.
* Add statistics for the number if blocks added or deleted on the web page.

Release 2.12 2011-10-30
=======================
* A new type of activator is now available, text messages. This is a text message that will be shown. Using various options,
  it is possible to add and control other effects.
* New block types: A completly black block, second type of stone, brown water (bruns�s)
* When creating text activators, a help dialog is shown
* There are some issues with the music that can cause error messages upon exit

Release 2.11 2011-10-23
=======================
* When you get aggro from a monster, it will get selected if you don't have any other selection.
* Improved drawing of chunks, in order near first and then farther away. This speeds up the initial display when the game is started.
* Music is not playing non-stop
* Improved leaves
* Define different faces for different players
* Fix problem of other players remaining on the screen
* Slightly smaller monsters
* Commands referring to other players are no case insensitive (/tell and /friend)

Release 2.10 2011-10-16
=======================
* Add snow, window and hedge block
* You must own a territory to be allowed build in it
* New commands "/friend add [name]" and "/friend remove [name]"
* Remember last /command to simplify typing
* Can only jump with feet on the ground
* Added backward compatibility with Windows XP
* Chunks are saved in local cache to speed up and minimize communication
* Dead players can not trig traps
* Improved what you see when you are near a wall, lowering the risk of being "inside" it

Release 2.9 2011-10-09
======================
* Sound from footsteps of other players.
* The Microsoft runtime library no longer warns about repairing.
* Sounds when building.
* Decrease OpenGL requirements to 3.0.
* No longer need to be Windows administrator to save login data and options.
* Optimized communication protocol.
* Insert key as an alternative to B in build mode.
* V as an alternative to Delete in build mode.
* Support for ladders and climbing ladders using space.

Release 2.8 2011-10-02
======================
* Other players shown on mini map
* New command /tell [player] [msg]
* Growling from monsters
* No longer showing sky in big caves when the wall is far away
* Traps that spawn monsters work

Release 2.7 2011-09-27
======================
* Compass with mini map added
* Show near monsters on mini map
* Allow arrow keys to navigate
* Notification sound when someone /say something
* Build mode displays text description of material.
* Use F1 to toggle build mode.
* Use DEL button to delete blocks (instead of left-click).
* Build mode displays chunk limits, using colors red (some one elses), green(yours) and blue(not allocated).
* Long /say messages could crash the client.
* New commands: "/territory claim" will set you as the owner of the current chunk.
* New commands: "/territory show" will list all chunks you own.
* Playing music, depending on player position.
