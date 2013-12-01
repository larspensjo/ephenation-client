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
* Get selection marker to work. Some ideas [here](http://mtnphil.wordpress.com/2012/09/12/faking-water-reflections-with-fourier-coefficients/)
* Implement SSAO

Graphical effects
-----------------
* Upgrade to GLFW 3, to support alternative displays
* Add distance dependent blurring
* Implement LOD (scale blocks with a factor of 2)
* Enable blurred shadows again
* Add surface effects. These shall be possible to configure using special block types.
* Use fading effect at block border, where needed
* Moss, rust, dust, dirt
* Use screen space reflection for wet surfaces as a surface effect. This should work really well as there usually is information
at shallow angles and translucent at near perpendicular angles.
* Update textures with higher resolution (512x512)
* libRocket Dialogs projected on blocks (a special case of the action block)
* For underground, use automatic green tints for positive up normals
* Make three sets of skyboxes, for different time of day
* Better looking water

Sound
-----
* Rewrite sound management using [OpenAL Soft](http://kcat.strangesoft.net/openal.html) and [Alure](http://kcat.strangesoft.net/alure.html).
* Implement environment depending echo using delayed emitters at a number of directions

Algorithms
----------
* Fix the issue with low initial FPS
* Timers should also be drawn live on the screen
* The transformation of a chunk into printable data should be managed by a Streaming class
* Pre-load chunks of TP destination

Installation
------------
* Investigate the use of [Ermine](http://www.magicermine.com/features.html) and FPM to create Linux installation
* Uninstalling the game should also remove the cache.
