IDEAS
=====

Oculus Rift
-----------
* Use bold font for better readability
* Dialogs are usually made for high resolution screens, so they become very big
* Information text drawn by vsfl does not show properly in stereo mode
* Use bigger render target, to use more of the OVR display
* The mouse pointer should be hidden when not needed
* The OVR checkbox in options doesn't reflect the settings correctly
* Drawing monster stats should use gProjectionView
* The map should only be computed once in OVR mode, not once for each eye
* Text in options windows doesn't always show

Graphical effects
-----------------
* Lamps and spherical fogs are cut off too hard at the edge
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
* Better looking water. Some ideas [here](http://mtnphil.wordpress.com/2012/09/12/faking-water-reflections-with-fourier-coefficients/),
[and here](http://www.jayconrod.com/posts/34/water-simulation-in-glsl)
* Implement SSAO
* Make water splash when a player or monsters falls into water

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
* Teleport shall use a jump parabola instead of instant transfer. If inside rock, black will have to be used.

Installation
------------
* Investigate the use of [Ermine](http://www.magicermine.com/features.html) and FPM to create Linux installation
* Uninstalling the game should also remove the cache.
