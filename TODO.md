IDEAS
=====

Oculus Rift
-----------
* Draw the experience, hp and mana bar in the HUD
* Remove the weapon from the display
* Use bold font for better readability
* When OVR is selected, force screen size matching the display, and full screen view
* Remove mouse and draw a special one instead that depends on distortion and stereo view
* Use alternative dialogs, adapted for OR
* Fix problem with flickering screen when looking far to the sides
* Some calibration is wrong, leading to screen moving when head turns
* Use super sampling for improved picture quality
* Split the HealthBar into several triangles
* Get selection marker to work

Graphical effects
-----------------
* Upgrade to GLFW 3, to support alternative displays
* Add distance dependent blurring
** Implement LOD (scale blocks with a factor of 2)
* Enable blurred shadows again
* Add surface effects. These shall be possible to configure using special block types.
** Use fading effect at block border, where needed
** Moss, rust, dust, dirt
* Update textures with higher resolution (512x512)
* libRocket Dialogs projected on blocks (a special case of the action block)
* For underground, use automatic green tints for positive up normals

Sound
-----
* Rewrite sound management using [OpenAL Soft](http://kcat.strangesoft.net/openal.html) and [Alure](http://kcat.strangesoft.net/alure.html).
* Implement environment depending echo using delayed emitters at a number of directions

Algorithms
----------
* Fix the issue with low initial FPS
* Implement signaling using [simplesignal.cc](http://www.testbit.eu/2013/cpp11-signal-system-performance/)

Installation
------------
* Investigate the use of [Ermine](http://www.magicermine.com/features.html) and FPM to create Linux installation
