// Copyright 2012 The Ephenation Authors
//
// This file is part of Ephenation.
//
// Ephenation is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3.
//
// Ephenation is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Ephenation.  If not, see <http://www.gnu.org/licenses/>.
//

#include <GL/glew.h>

#include <stdlib.h>

#include "glm/glm.hpp"
#include "textures.h"
#include "imageloader.h"
#include "render.h"
#include "chunk.h"
#include "simplexnoise1234.h"
#include "glm/glm.hpp"
#include "primitives.h"
#include "Options.h"

#define NELEM(x) (int)(sizeof x / sizeof x[0])

// Flags for controlling how textures are handled
enum TextureFlags {
	TF_SRGB = 1,       // Default is the linear color space
	TF_NOMIPMAP = 2,   // Default is to generate mipmap
	TF_NEAREST = 4,    // Use no interpolations
	TF_MIPMAP2 = 8,    // Use special home made mipmaps
	TF_BOOLAPHA = 0x10 // Alpha will be either 0 or 1
};

// The order in this table can be random.
// This table may only contain textures known by the server.
// If a texture is added, don't forget to update switch statement in DrawQuadList().
// A game texture with 0 pointer for bitmap is loaded elsewhere.
static GameTexture gameTextures[] = {
	{ BT_Snow,          0, "textures/Snow.bmp", "Snow", TF_SRGB },
	{ BT_Water,         0, "textures/water.bmp", "Water", TF_SRGB },
	{ BT_BrownWater,    0, "textures/BrownWater.bmp", "Brown water", TF_SRGB },
	{ BT_Window,        0, "textures/Window.bmp", "Window", TF_SRGB },
	{ BT_Soil,          0, "textures/soil.bmp", "Soil", TF_SRGB },
	{ BT_Logs,          0, "textures/logs.bmp", "Logs", TF_SRGB },
	{ BT_Brick,         0, "textures/brick.bmp", "Brick", TF_SRGB|TF_MIPMAP2 },
	{ BT_Stone,         0, "textures/stone_texture.bmp", "Stone", TF_SRGB },
	{ BT_Stone2,        0, "textures/evenly_worn_stone.bmp", "Stone2", TF_SRGB },
	{ BT_Concrete,      0, "textures/concrete.bmp", "Concrete", TF_SRGB },
	{ BT_WhiteConcrete, 0, "textures/whiteconcrete.bmp", "Concrete2", TF_SRGB },
	{ BT_Gravel,        0, "textures/gravel.bmp", "Gravel", TF_SRGB },
	{ BT_TiledStone,    0, "textures/tiledstone.bmp", "Tiled Stone", TF_SRGB },
	{ BT_Ladder,        0, "textures/LadderOnStone.bmp", "Ladder", TF_SRGB },
	{ BT_Sand,          0, "textures/sand.bmp", "Sand", TF_SRGB },
	{ BT_Treasure,      0, "textures/coin.bmp", "Treasure", TF_NOMIPMAP|TF_NEAREST|TF_BOOLAPHA },
	{ BT_Quest,         0, "textures/QuestSymbol.bmp", "Quest", TF_SRGB|TF_MIPMAP2|TF_NEAREST|TF_BOOLAPHA },
	{ BT_Lamp1,         0, 0, "Small torch" },
	{ BT_Lamp2,         0, 0, "Big torch" },
	{ BT_Bark,          0, 0, "Tree bark" },
	{ BT_Tree1,         0, "textures/tree-texture1.bmp", "Small tree", TF_SRGB },
	{ BT_Tree2,         0, "textures/tree-texture2.bmp", "Medium tree", TF_SRGB },
	{ BT_Tree3,         0, "textures/tree-texture3.bmp", "Big tree", TF_SRGB },
	{ BT_Tuft,          0, "textures/TuftOfGrass.bmp", "Tuft of grass", TF_SRGB|TF_MIPMAP2|TF_NEAREST|TF_BOOLAPHA },
	{ BT_Flowers,       0, "textures/RedFlowers.bmp", "Flowers", TF_NOMIPMAP|TF_NEAREST|TF_BOOLAPHA },
	{ BT_SmallFog,      0, "textures/SmallFog.bmp", "Small fog", TF_SRGB },
	{ BT_BigFog,        0, "textures/BigFog.bmp", "Big fog", TF_SRGB },
	{ BT_Hedge,         0, "textures/hedge.bmp", "Hedge", TF_SRGB },
	{ BT_CobbleStone,   0, "textures/CobbleStone.bmp", "Cobblestone", TF_SRGB|TF_MIPMAP2 },
	{ BT_DeTrigger,     0, 0, "Detrigger" },
	{ BT_Text,          0, "textures/textblock.bmp", "Activation block", TF_SRGB },
	{ BT_Link,          0, 0, "Link" },
	{ BT_Trigger,       0, 0, "Trigger" },
	{ BT_Black,         0, 0, "Black" },
	{ BT_Teleport,      0, "textures/Teleport.bmp", "Teleport" },
};

const int GameTexture::fgNumBuildBlocks = NELEM(gameTextures);

const GameTexture *GameTexture::GetGameTexture(int index) {
	return &gameTextures[index];
}

GLuint BlockTypeTotextureId[256];

GLuint GameTexture::GrassTextureId, GameTexture::TuftOfGrass, GameTexture::Flowers;
GLuint GameTexture::LeafTextureId;
GLuint GameTexture::StoneTexture2Id;
GLuint GameTexture::TreeBarkId, GameTexture::Hedge, GameTexture::Window, GameTexture::Snow, GameTexture::Branch;
GLuint GameTexture::Sky1Id, GameTexture::Sky2Id, GameTexture::Sky3Id, GameTexture::Sky4Id, GameTexture::SkyupId;
GLuint GameTexture::RedScalesId, GameTexture::Fur1Id, GameTexture::MonsterFace1, GameTexture::PlayerFace[5], GameTexture::Morran;
GLuint GameTexture::LanternSideId, GameTexture::Teleport;
GLuint GameTexture::InGameUiId, GameTexture::LightBallsHeal, GameTexture::InventoryId, GameTexture::EquipmentId;
GLuint GameTexture::RedColor, GameTexture::GreenColor, GameTexture::BlueColor, GameTexture::DarkGray;
GLuint GameTexture::RedChunkBorder, GameTexture::BlueChunkBorder, GameTexture::GreenChunkBorder;
GLuint GameTexture::CompassRose, GameTexture::DamageIndication;
GLuint GameTexture::WEP1, GameTexture::WEP2, GameTexture::WEP3, GameTexture::WEP4;
GLuint GameTexture::Coin, GameTexture::Quest;
GLuint GameTexture::WEP1Text, GameTexture::WEP2Text, GameTexture::WEP3Text, GameTexture::WEP4Text;
GLuint GameTexture::DialogBackground;
GLuint GameTexture::PoissonDisk;

using glm::vec2;
// This table is from http://asawicki.info/Download/Productions/Applications/PoissonDiscGenerator/2D.txt
const vec2 gPoissonDisk[64] = {
	vec2( 0.282571, 0.023957 ),
	vec2( 0.792657, 0.945738 ),
	vec2( 0.922361, 0.411756 ),
	vec2( 0.165838, 0.552995 ),
	vec2( 0.566027, 0.216651 ),
	vec2( 0.335398, 0.783654 ),
	vec2( 0.0190741, 0.318522 ),
	vec2( 0.647572, 0.581896 ),
	vec2( 0.916288, 0.0120243 ),
	vec2( 0.0278329, 0.866634 ),
	vec2( 0.398053, 0.4214 ),
	vec2( 0.00289926, 0.051149 ),
	vec2( 0.517624, 0.989044 ),
	vec2( 0.963744, 0.719901 ),
	vec2( 0.76867, 0.018128 ),
	vec2( 0.684194, 0.167302 ),
	vec2( 0.727103, 0.410871 ),
	vec2( 0.557482, 0.724143 ),
	vec2( 0.483352, 0.0527055 ),
	vec2( 0.162877, 0.351482 ),
	vec2( 0.959716, 0.180578 ),
	vec2( 0.140355, 0.112003 ),
	vec2( 0.796228, 0.223365 ),
	vec2( 0.187048, 0.787225 ),
	vec2( 0.55446, 0.35612 ),
	vec2( 0.449965, 0.640522 ),
	vec2( 0.438917, 0.194769 ),
	vec2( 0.791253, 0.565325 ),
	vec2( 0.719718, 0.794794 ),
	vec2( 0.0651875, 0.708609 ),
	vec2( 0.641987, 0.0233772 ),
	vec2( 0.376415, 0.944243 ),
	vec2( 0.827723, 0.723258 ),
	vec2( 0.968627, 0.884518 ),
	vec2( 0.263405, 0.458968 ),
	vec2( 0.985717, 0.559587 ),
	vec2( 0.0616169, 0.468612 ),
	vec2( 0.159154, 0.934782 ),
	vec2( 0.287301, 0.284768 ),
	vec2( 0.550066, 0.849391 ),
	vec2( 0.353587, 0.003296 ),
	vec2( 0.000671407, 0.582507 ),
	vec2( 0.850459, 0.461989 ),
	vec2( 0.526139, 0.640126 ),
	vec2( 0.786889, 0.487686 ),
	vec2( 0.164129, 0.02472 ),
	vec2( 0.517075, 0.90933 ),
	vec2( 0.316111, 0.663564 ),
	vec2( 0.09476, 0.895749 ),
	vec2( 0.298288, 0.195318 ),
	vec2( 0.427229, 0.7828 ),
	vec2( 0.734764, 0.266152 ),
	vec2( 0.0816065, 0.965972 ),
	vec2( 0.698935, 0.646352 ),
	vec2( 0.281899, 0.355144 ),
	vec2( 0.871334, 0.303171 ),
	vec2( 0.138249, 0.661214 ),
	vec2( 0.202399, 0.252449 ),
	vec2( 0.0734275, 0.399853 ),
	vec2( 0.786767, 0.660268 ),
	vec2( 0.933744, 0.508621 ),
	vec2( 0.398236, 0.0509049 ),
	vec2( 0.500473, 0.130253 ),
	vec2( 0.0332957, 0.526292)
};

//Makes the image into a texture, and returns the id of the texture. The texture is returned as bound
GLuint loadTexture(shared_ptr<Image> image, unsigned fl = 0) {
	bool mipmap = true;
	if (fl & TF_NOMIPMAP)
		mipmap = false;
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	GLint internalFormat = GL_RGB;
	GLenum filter = GL_LINEAR, mipmapFilter = GL_LINEAR_MIPMAP_LINEAR;
	if (fl & TF_NEAREST) {
		filter = GL_NEAREST;
		mipmapFilter = GL_NEAREST_MIPMAP_NEAREST;
	}
	if (fl & TF_SRGB)
		internalFormat = GL_SRGB;
	if (image->fFormat == GL_RGBA || image->fFormat == GL_BGRA) {
		if (fl & TF_SRGB)
			internalFormat = GL_SRGB8_ALPHA8;
		else
			internalFormat = GL_RGBA;
	}
	if (mipmap)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmapFilter);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	if (mipmap && Options::fgOptions.fAnisotropicFiltering) {
		if (GLEW_EXT_texture_filter_anisotropic) {
			static int max = 0;
			if (max == 0)
				glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
			// printf("GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT: %d\n", max);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max);
		}
	}
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image->width, image->height, 0, image->fFormat, GL_UNSIGNED_BYTE, image->pixels.get());
	if (fl & TF_MIPMAP2) {
		for (GLint level = 1; image->width > 1 && image->height > 1; level++) {
			// The mipmap has to be done in linear space.
			if (fl & TF_SRGB)
				image->MakeLinear(); // This bitmap was already transferred to the GPU.
			image = image->MipMapNextLevel();
			if (fl & TF_SRGB)
				image->MakeNonLinear(); // Make down sampled bitmap of SRGB format again, which is what is transferred to the GPU.
			glTexImage2D(GL_TEXTURE_2D, level, internalFormat, image->width, image->height, 0, image->fFormat, GL_UNSIGNED_BYTE, image->pixels.get());
		}

	} else if (mipmap) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	return textureId;
}

// Not used currently.
// Create a simple uniform color image.
shared_ptr<Image> ColorImage(int red, int green, int blue) {
	const int size = 16;
	unique_ptr<unsigned char[]> pixels(new unsigned char[size*size*3]);
	for(int y = 0; y < size; y++) {
		for(int x = 0; x < size; x++) {
			pixels[3 * (size * y + x) + 0] = red;
			pixels[3 * (size * y + x) + 1] = green;
			pixels[3 * (size * y + x) + 2] = blue;
		}
	}
	return std::make_shared<Image>(std::move(pixels), size, size, GL_RGB);
}

// Create a simple uniform color image, with premultiplied alpha
shared_ptr<Image> ColorImage(int red, int green, int blue, int alpha) {
	const int size = 16;
	unique_ptr<unsigned char[]> pixels(new unsigned char[size*size*4]);
	for(int y = 0; y < size; y++) {
		for(int x = 0; x < size; x++) {
			pixels[4 * (size * y + x) + 0] = red*alpha/255;
			pixels[4 * (size * y + x) + 1] = green*alpha/255;
			pixels[4 * (size * y + x) + 2] = blue*alpha/255;
			pixels[4 * (size * y + x) + 3] = alpha;
		}
	}
	return std::make_shared<Image>(std::move(pixels), size, size, GL_RGBA);
}

void GameTexture::Init(void) {
	for (int i=0; i<NELEM(gameTextures); i++) {
		if (gameTextures[i].fileName) {
			bool boolAlpha = false;
			if (gameTextures[i].flag & TF_BOOLAPHA)
				boolAlpha = true;
			GLuint id = loadTexture(loadBMP(gameTextures[i].fileName, boolAlpha), gameTextures[i].flag);
			gameTextures[i].id = id;
			BlockTypeTotextureId[gameTextures[i].blType] = id;
		} else switch (gameTextures[i].blType) {
			case BT_Lamp1:
				gameTextures[i].id = loadTexture(ColorImage(127, 127, 0, 64), TF_NOMIPMAP);
				BlockTypeTotextureId[gameTextures[i].blType] = gameTextures[i].id;
				break;
			case BT_Lamp2:
				gameTextures[i].id = loadTexture(ColorImage(255, 255, 0, 64), TF_NOMIPMAP);
				BlockTypeTotextureId[gameTextures[i].blType] = gameTextures[i].id;
				break;
			case BT_DeTrigger:
				gameTextures[i].id = loadTexture(ColorImage(0, 255, 255, 180), TF_NOMIPMAP);
				BlockTypeTotextureId[gameTextures[i].blType] = gameTextures[i].id;
				break;
			case BT_Black:
				gameTextures[i].id = loadTexture(ColorImage(0, 0, 0, 255), TF_NOMIPMAP);
				BlockTypeTotextureId[gameTextures[i].blType] = gameTextures[i].id;
				break;
			case BT_Link:
				GreenColor = gameTextures[i].id = loadTexture(ColorImage(0, 255, 0, 180), TF_NOMIPMAP);
				BlockTypeTotextureId[gameTextures[i].blType] = gameTextures[i].id;
				break;
			case BT_Trigger:
				RedColor = gameTextures[i].id = loadTexture(ColorImage(255, 0, 0, 180), TF_NOMIPMAP);
				BlockTypeTotextureId[gameTextures[i].blType] = gameTextures[i].id;
				break;
			}
	}

	Coin = BlockTypeTotextureId[BT_Treasure];
	Quest = BlockTypeTotextureId[BT_Quest];
	Teleport = BlockTypeTotextureId[BT_Teleport];
	TuftOfGrass = BlockTypeTotextureId[BT_Tuft];
	Flowers = BlockTypeTotextureId[BT_Flowers];

	// Now load special textures, not represented by a block type
	GrassTextureId = loadTexture(loadBMP("textures/grass.bmp"), TF_SRGB);
	BlockTypeTotextureId[BT_TopSoil] = GrassTextureId;

	LeafTextureId = loadTexture(loadBMP("textures/leaves.bmp"), TF_SRGB);
//	BlockTypeTotextureId[BT_TopSoil] = LeafTextureId;

	StoneTexture2Id = loadTexture(loadBMP("textures/evenly_worn_stone.bmp"), TF_SRGB);
	BlockTypeTotextureId[BT_Stone2] = StoneTexture2Id;

	TreeBarkId = loadTexture(loadBMP("textures/tree-bark-texture.bmp"), TF_SRGB);
	BlockTypeTotextureId[BT_Bark] = TreeBarkId;

	DarkGray = loadTexture(ColorImage(15, 15, 15), TF_NOMIPMAP);

	Sky1Id = loadTexture(loadBMP("textures/sky1.bmp"), TF_NOMIPMAP|TF_SRGB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	Sky2Id = loadTexture(loadBMP("textures/sky2.bmp"), TF_NOMIPMAP|TF_SRGB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	Sky3Id = loadTexture(loadBMP("textures/sky3.bmp"), TF_NOMIPMAP|TF_SRGB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	Sky4Id = loadTexture(loadBMP("textures/sky4.bmp"), TF_NOMIPMAP|TF_SRGB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	SkyupId = loadTexture(loadBMP("textures/skyup.bmp"), TF_NOMIPMAP|TF_SRGB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	RedScalesId = loadTexture(loadBMP("textures/RedScales.bmp"), TF_SRGB);
	Morran = loadTexture(loadBMP("textures/morran.bmp"), TF_SRGB);

	Fur1Id = loadTexture(loadBMP("textures/fur1.bmp"), TF_SRGB);

	DamageIndication = loadTexture(loadBMP("textures/DamageIndication.bmp"), TF_NOMIPMAP);

	LanternSideId = loadTexture(loadBMP("textures/LanternSide.bmp"), TF_SRGB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	InGameUiId = loadTexture(loadBMP("textures/InGameUI.bmp"), TF_NOMIPMAP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	InventoryId = loadTexture(loadBMP("textures/Inventory.bmp"), TF_NOMIPMAP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	EquipmentId = loadTexture(loadBMP("textures/EquipmentIcons.bmp"), TF_NOMIPMAP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	MonsterFace1 = loadTexture(loadBMP("textures/MonsterFace1.bmp"));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	LightBallsHeal = loadTexture(loadBMP("textures/LightBallsHeal.bmp"), TF_NOMIPMAP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	PlayerFace[0] = loadTexture(loadBMP("textures/narrowhouse_ugly_kid.bmp"));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	PlayerFace[1] = loadTexture(loadBMP("textures/ugly_kid_02.bmp"));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	PlayerFace[2] = loadTexture(loadBMP("textures/ugly_kid_03.bmp"));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	PlayerFace[3] = loadTexture(loadBMP("textures/ugly_kid_04.bmp"));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	PlayerFace[4] = loadTexture(loadBMP("textures/ugly_kid_05.bmp"));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	RedChunkBorder = loadTexture(loadBMP("textures/RedChunkBorder.bmp"), TF_SRGB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GreenChunkBorder = loadTexture(loadBMP("textures/GreenChunkBorder.bmp"), TF_SRGB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	BlueChunkBorder = loadTexture(loadBMP("textures/BlueChunkBorder.bmp"), TF_SRGB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	CompassRose = loadTexture(loadBMP("textures/CompassRose.bmp"), TF_NOMIPMAP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	WEP2 = loadTexture(loadBMP("textures/WEP2.bmp"), TF_NOMIPMAP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	WEP1 = loadTexture(loadBMP("textures/WEP1.bmp"), TF_NOMIPMAP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	WEP1Text = loadTexture(loadBMP("textures/WEP1Text.bmp"), TF_NOMIPMAP);
	WEP2Text = loadTexture(loadBMP("textures/WEP2Text.bmp"), TF_NOMIPMAP);
	WEP3Text = loadTexture(loadBMP("textures/WEP3Text.bmp"), TF_NOMIPMAP);
	WEP4Text = loadTexture(loadBMP("textures/WEP4Text.bmp"), TF_NOMIPMAP);

	Branch = loadTexture(loadBMP("textures/branch.bmp", true), TF_SRGB|TF_MIPMAP2|TF_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	DialogBackground = loadTexture(loadBMP("textures/DialogBackground.bmp"), TF_NOMIPMAP|TF_NEAREST|TF_SRGB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// This is just a safety precaution, if there would be an unrecognized block
	for (int i=0; i<256; i++) {
		if (BlockTypeTotextureId[i] == 0)
			BlockTypeTotextureId[i] = GameTexture::CompassRose;
	}

	glGenTextures(1, &PoissonDisk);
	glBindTexture(GL_TEXTURE_1D, PoissonDisk);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RG32F, NELEM(gPoissonDisk), 0, GL_RG, GL_FLOAT, gPoissonDisk);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

// Load bitmaps to be used for the GUI.
GLuint LoadBitmapForGui(shared_ptr<Image> img) {
	GLuint ret = loadTexture(img, TF_NOMIPMAP|TF_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return ret;
}
