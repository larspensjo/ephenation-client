IDEAS
=====

Oculus Rift
-----------
* Use bold font for better readability
* Use alternative dialogs, adapted for OR
* Use super sampling for improved picture quality
* Get selection marker to work
* Get construction mode to work
* Flying and swimming needs to be fixed
* Magic portals don't work (can't click on)
* The mouse pointer should be hidden when not needed
* The OVR checkbox in options doesn't reflect the settings correctly

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
* Better looking water. Some ideas [here](http://mtnphil.wordpress.com/2012/09/12/faking-water-reflections-with-fourier-coefficients/)
* Implement SSAO

Sound
-----
* Rewrite sound management using [OpenAL Soft](http://kcat.strangesoft.net/openal.html) and [Alure](http://kcat.strangesoft.net/alure.html).
* Implement environment depending echo using delayed emitters at a number of directions
* Sound from walking is wrong (generating too many steps)

Algorithms
----------
* Fix the issue with low initial FPS
* Timers should also be drawn live on the screen
* The transformation of a chunk into printable data should be managed by a Streaming class
* Pre-load chunks of TP destination
* Update to newer version of libRocket

Installation
------------
* Investigate the use of [Ermine](http://www.magicermine.com/features.html) and FPM to create Linux installation
* Uninstalling the game should also remove the cache.
