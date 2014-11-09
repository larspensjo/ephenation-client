project('ephenation', 'cpp', 'c')

add_global_arguments('-std=gnu++11', language : 'cpp')
add_global_arguments('-DDEBUG', language : 'cpp')
add_global_arguments('-DDEBUG', language : 'c')

opengl = dependency('gl')
x11 = dependency('x11')
glew = dependency('glew')
assimp = dependency('assimp')
tinyxml = dependency('tinyxml')
openal = dependency('openal')
ogg = dependency('ogg')
vorbis = dependency('vorbis')
vorbisfile = dependency('vorbisfile')
xinerama = dependency('xinerama')

glfw = find_library('glfw')
pthread = find_library('pthread')
udev = find_library('udev')
alut = find_library('alut')
RocketCore = find_library('RocketCore')
RocketDebugger = find_library('RocketDebugger')
RocketControls = find_library('RocketControls')

libovr = 'contrib/libovr/lib/Release/libovr.a'

deps=[opengl, x11, pthread, glew, udev, assimp, glfw, tinyxml, alut, openal, ogg, vorbis,
	xinerama, RocketCore, RocketDebugger, RocketControls, udev, vorbisfile, xinerama]

glm = include_directories('contrib/glm')
libovrHDR = include_directories('contrib/libovr/include')
incdirs = [glm, libovrHDR]

src = ['animationmodels.cpp', 'assert.cpp', 'billboard.cpp', 'BuildingBlocks.cpp', 'ChunkBlocks.cpp',
	'chunkcache.cpp', 'chunk.cpp', 'ChunkObject.cpp', 'ChunkProcess.cpp', 'clipboard.cpp', 'connection.cpp',
	'crypt.cpp', 'Debug.cpp', 'DrawText.cpp', 'DrawTexture.cpp', 'errormanager.cpp', 'fboflat.cpp',
	'gamedialog.cpp', 'HealthBar.cpp', 'HudTransformation.cpp', 'imageloader.cpp', 'Inventory.cpp',
	'main.cpp', 'manageanimation.cpp', 'Map.cpp', 'modes.cpp', 'monsters.cpp', 'msgwindow.cpp', 'OculusRift.cpp',
	'Options.cpp', 'otherplayers.cpp', 'parse.cpp', 'player.cpp', 'primitives.cpp', 'rendercontrol.cpp',
	'render.cpp', 'ScrollingMessages.cpp', 'shaders/adddynamicshadow.cpp', 'shaders/addlocalfog.cpp',
	'shaders/addpointlight.cpp', 'shaders/addpointshadow.cpp', 'shaders/addssao.cpp', 'shaders/AnimationShader.cpp',
	'shaders/ChunkShader.cpp', 'shaders/ChunkShaderPicking.cpp', 'shaders/ColorShader.cpp',
	'shaders/DeferredLighting.cpp', 'shaders/DistanceBlurring.cpp', 'shaders/downsmpllum.cpp',
	'shaders/gaussblur.cpp', 'shaders/modulatedtextureshader.cpp', 'shaders/MonsterShader.cpp',
	'shaders/ScreenSpaceReflection.cpp', 'shaders/shader.cpp', 'shaders/shadowmapshader.cpp',
	'shaders/SimpleTextureShader.cpp', 'shaders/skybox.cpp', 'shaders/StageOneShader.cpp',
	'shaders/BarrelDistortion.cpp',
	'shaders/TranspShader.cpp', 'shadowrender.cpp', 'shapes/Cube.cpp', 'shapes/Cylinder.cpp',
	'shapes/octagon.cpp', 'shapes/quad.cpp', 'shapes/quadstage1.cpp', 'shapes/Tree.cpp',
	'simplexnoise1234.cpp', 'SoundControl.cpp', 'SuperChunkManager.cpp', 'Teleport.cpp', 'textures.cpp',
	'timemeasure.cpp', 'TSExec.cpp', 'ui/activator.cpp', 'ui/base.cpp', 'ui/Error.cpp', 'ui/factory.cpp',
	'ui/login.cpp', 'ui/mainuserinterface.cpp', 'ui/options.cpp', 'ui/RocketGui.cpp',
	'ui/RocketRenderInterface.cpp', 'ui/RocketSystemInterface.cpp', 'ui/simple.cpp', 'UndoOp.cpp',
	'uniformbuffer.cpp', 'vsfl/vsfl.cpp', 'worsttime.cpp', 'contrib/glsw.c', 'contrib/bstrlib.c',
	'OpenglBuffer.cpp', 'RenderTarget.cpp']

executable('ephenation', sources:src, deps : deps, include_dirs : incdirs, objects : libovr)
