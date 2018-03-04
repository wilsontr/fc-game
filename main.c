
// TODO next:

// - boss
// - flesh out enemy behavior
// - bashing enemies for medals, medal chaining
// - more sound effects
// - lives in scoreboard
// - break out headers/modules
// - moving platforms?
// - study enemy behavior in games	
// - level music theme
// - title screen

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "neslib.h"
#include "maps/newmap.h"
#include "maps/newmap_coll.h"
#include "maps/map2.h"
#include "maps/map2_coll.h"


void __fastcall__ memcpy(void *dst, void *src, unsigned int len);


typedef uint8_t u8;
typedef uint16_t u16;

#define TILE_NOCOLLIDE   		0
#define TILE_ALLCOLLIDE   		1
#define TILE_ENEMYCOLLIDE 		2
#define TILE_ENEMY1START_LEFT 	4	
#define TILE_ENEMY1START_RIGHT	5	
#define TILE_LADDER				6
#define TILE_LADDER_TOP			7
#define TILE_FRUIT				8
#define TILE_PLAYER				9
#define TILE_BOSS				10
#define TILE_WATER				11
#define TILE_JEWEL				12	  	
#define TILE_PLATFORM			13	

// TODO Think about RLE-encoding collision maps again
#define COLLISION_MAP_SIZE 	    240

#define PLAYER_STATE_NORMAL		0
#define PLAYER_STATE_DEAD		1
#define PLAYER_STATE_CLIMBING	2
#define PLAYER_STATE_JUMPING	3
#define PLAYER_STATE_FALLING	4

#define PLAYER_INIT_LIVES		3

#define PLAYER_MOVE_VEL			1
#define PLAYER_FALL_SPEED		3
#define PLAYER_JUMP_COUNTER_INTERVAL 6
#define GRAVITY_ACCELERATION 	2
#define PLAYER_INIT_JUMP_VEL 	3
#define MAX_PLAYER_VERT_VEL 	-3

#define PLAYER_FRAME_STANDING 	0
#define PLAYER_FRAME_CLIMBING	2
#define PLAYER_FRAME_JUMPING	3
#define PLAYER_FRAME_DEAD		4

#define PLAYER_WALK_ANIMATE_INTERVAL 0x07
#define PLAYER_DEAD_INTERVAL	90

#define GLUE_INIT_LIFESPAN	 	240
#define MAX_GLUE_COUNT 			1
#define GLUE_INIT_DURATION		255

#define MAX_PLATFORM_COUNT		3
#define PLATFORM_SPEED			1

#define GLUE_FRAME_BIG			0
#define GLUE_FRAME_MEDIUM		1
#define GLUE_FRAME_SMALL		2

#define GLUE_STATE_INACTIVE		0
#define GLUE_STATE_ACTIVE		1
#define GLUE_STATE_FORMING		2

#define ENEMY_STATE_NORMAL 		0
#define ENEMY_STATE_GLUED		1
#define ENEMY_STATE_DEAD		3

#define ENEMY_DATA_SIZE		17
#define MAX_ENEMY_COUNT		5

#define SFX_JUMP			0
#define SFX_GLUEDROP		1
#define SFX_BEEP			2
#define SFX_GLUESTUCK		3

#define CHANNEL_SQUARE1 	0
#define CHANNEL_SQUARE2 	1
#define CHANNEL_TRIANGLE 	2
#define CHANNEL_NOISE	 	3

#define SCORE_VALUE_FRUIT	8
#define SCORE_VALUE_JEWEL	20


#pragma bss-name (push, "ZEROPAGE")
#pragma data-name (push, "ZEROPAGE")

u8 oam_off;
static u8 playerX = 0;
static u8 playerY = 0;
static u8 enemyIndex = 0;
static u8 frameCount;
static u8 i;
static u16 collisionIndex;
static u8 leftSide, rightSide, topSide, bottomSide;
static u8 pad, oamSpriteIndex;
static u8 horizontalCollideCheck;
static u8 verticalCollideCheck;
static u8 collideBottom;
static u8 enemyColliding;
static u8 playerStartX;
static u8 playerStartY;
static u8 levelComplete;


#pragma data-name(pop)
#pragma bss-name (pop)

static u8 collisionMap[COLLISION_MAP_SIZE];

/* Bookkeeping and state */

// sprite palette comes from fcgame.dat
// imported in crt0.s

extern const u8 paldat[];
const unsigned char bgPalette[16]={ 0x0f,0x11,0x21,0x30,0x0f,0x04,0x14,0x34,0x0f,0x16,0x29,0x30,0x0f,0x20,0x28,0x21 };


const u8 * currentCollisionData;


static u8 touch;
static u8 spriteFlickerIndex = 0;
static u8 sprPriorityToggle = 0;

static unsigned int playerScore = 0;

static u8 palSprites[4];
//static u8 palBG[4];

static u8 collisionLeft, collisionRight;


/* Player */
static u8 playerDir;
static u8 playerEnemyColliding;
static u8 playerFrame = 0;
static u8 playerJumpCounter = 0;
static u8 playerState = PLAYER_STATE_NORMAL;
static signed char playerVertVel = 0;
static u8 jumpButtonReset = 1;
static u8 glueButtonReset = 1;
static u8 playerSittingOnSprite = 0;
static u8 playerLives;
static u8 playerAnimationCounter;


const u8 playerFrames[5][4] = {
	{ 0x08, 0x09, 0x18, 0x19 }, // walk 1
	{ 0x28, 0x29, 0x38, 0x39 }, // walk 2
	{ 0x0A, 0x0B, 0x1A, 0x1B }, // on ladder
	{ 0x48, 0x49, 0x58, 0x59 }, // jump
	{ 0x2A, 0x2B, 0x3A, 0x3B }  // dead
};

static u8 playerSpriteData[17] = {
	0, 0, 0x08, 0x3,
	8, 0, 0x09, 0x3,
	0, 8, 0x18, 0x3,
	8, 8, 0x19, 0x3,
	128
};

static u8 * nametableUpdateList;

/* Glue */

struct glueStruct {
	u8 x;
	u8 y;
	u8 timeLeft;
	u8 frame;
	u8 state;
	u8 collisionIndex;
	u8 spriteData[17];
};

typedef struct glueStruct glue;

static glue glueData[MAX_GLUE_COUNT];
static glue *gluePointer;

static u8 glueIndex;
static u8 glueColliding;
static u8 glueCollidedIndex;
static u8 glueTop;
static u8 glueLeft;
static u8 glueRight;
static u8 glueBottom;

const u8 glueFrames[3][4] = {
	{ 0x46, 0x47, 0x56, 0x57 }, // big
	{ 0x66, 0x67, 0x76, 0x77 }, // medium
	{ 0x86, 0x87, 0x96, 0x97 }  // small
};

const u8 glueSpriteDataTemplate[17] = {
	0, 0, 0x46, 0x0,
	8, 0, 0x47, 0x0,
	0, 8, 0x56, 0x0,
	8, 8, 0x57, 0x0,
	128
};

/* Platforms */


const u8 platformSpriteDataTemplate[17] = {
	0, 0, 0x82, 0x1,
	8, 0, 0x83, 0x1,
	0, 8, 0x92, 0x1,
	8, 8, 0x93, 0x1,
	128
};

struct platformStruct {
	u8 x;
	u8 y;
	u8 isActive;
	u8 direction;
	u8 playerColliding;
};

typedef struct platformStruct platform;

static platform platformData[MAX_PLATFORM_COUNT];
static platform *platformPointer = NULL;
static u8 platformIndex;

static u8 platformTop;
static u8 platformBottom;
static u8 platformLeft;
static u8 platformRight;

static u8 playerPlatformStuck = 0;

/* Enemies */

static u8 numEnemies;
static u8 enemySpriteCount = 0;

/* For collision checking */
static u8 enemyTop;
static u8 enemyBottom;
static u8 enemyLeft;
static u8 enemyRight;
static u8 enemyCollidedIndex = 0;



struct enemyStruct {
	u8 x;
	u8 y; 
	u8 frame;
	u8 direction;
	u8 state;
	u8 glueTimeLeft;
};

typedef struct enemyStruct enemy;

enemy enemyData[MAX_ENEMY_COUNT];

const u8 enemyFrames[2][4] = {
	{ 0x06, 0x07, 0x16, 0x17 },
	{ 0x26, 0x27, 0x36, 0x37 }
};

const u8 emptyFrame[4] = {
	0, 0, 0, 0
};

const u8 fruitFrame[4] = {
	0x20, 0x21, 0x30, 0x31
};

const u8 jewelFrame[4] = {
	0x42, 0x43, 0x52, 0x53
};

// x offset, y offset, tile, attribute

const u8 enemySpriteDataTemplate[17] = {
	0, 0, 0, 0x3,
	8, 0, 0, 0x3,
	0, 8, 0, 0x3,
	8, 8, 0, 0x3,
	128
};

static u8 enemySpriteData[10][17];
static enemy * currentEnemy;






/*********** Score/tile updates ***********/

const u8 tileUpdateListInit[9 * 3 + 1] = {

	0, 0, 0,
	0, 0, 0,
	0, 0, 0, 

	0, 0, 0,
	0, 0, 0,
	0, 0, 0,

	0, 0, 0, 
	0, 0, 0,
	0, 0, 0,
	
	NT_UPD_EOF
};

u8 tileUpdateList[9 * 3 + 1];


static u8 scoreUpdateListData[8] = { 
	MSB(NTADR_A(3, 3)) | NT_UPD_HORZ, // MSB
	LSB(NTADR_A(3, 3)),  // LSB
	4, // Byte count
	0, 0, 0, 0,
	NT_UPD_EOF // EOF
};

static u8 scoreUpdateList[8] = {
	0, 0, 0, 0, 0, 0, 0, 0
};

static u8 digitsArray[4] = {
	0, 0, 0, 0
};
static u8 decadeCount;
static unsigned int valueToConvert;
static u8 scoreChanged = 0;

/* Sprite maintenance */

static u8 sSpriteIndex, sFrameIndex;

/* Prototypes */

void setupMap(void);
void killPlayer(void);
void __fastcall__ setSpriteFrame(u8 *sprite, const u8 *frame);
void __fastcall__ setSpritePalette(u8 *sprite, u8 palette);
void __fastcall__ updateMapTile(u8 mapX, u8 mapY, const u8 * tileFrame);
void __fastcall__ putMapTile(u8 mapX, u8 mapY, const u8 * tileFrame);


/*********** Unused: utility function for decompressing RLE collision map data ***********/

/*
void unrleCollision(void) {
	u8 i = 0;
	u8 j = 0;
	u16 size = sizeof(test_nam_coll_rle); 
	u8 currentByte;
	u8 byteCount;
	u16 outPointer = 0;

	while ( i <= size ) {
		currentByte = test_nam_coll_rle[i];
		++i;
		byteCount = test_nam_coll_rle[i];
		++i;
		for ( j = 0; j < byteCount; ++j ) {
			collisionMap[outPointer] = currentByte;
			++outPointer;
		}
	}
}
*/


/*********** State Setup ***********/

void setupMap(void) {
	u8 collisionByte;
	u8 mapX = 0;
	u8 mapY = 0;
	u8 index = 0;

	enemy newEnemy = { 0, 0, 0, PAD_LEFT };
	platform newPlatform = { 0, 0, 1, PAD_LEFT, 0 };

	memcpy(collisionMap, currentCollisionData, COLLISION_MAP_SIZE);
	enemyIndex = 0;
	platformIndex = 0;
	numEnemies = 0;

	// initialize glue data

	for ( index = 0; index < MAX_GLUE_COUNT; ++index ) {
		gluePointer = &(glueData[index]);
		gluePointer->state = GLUE_STATE_INACTIVE;
	}

	// initialize platform data

	for ( index = 0; index < MAX_PLATFORM_COUNT; ++index ) {
		platformPointer = &(platformData[index]);
		platformPointer->isActive = 0;
	}

	
	// populate the screen based on the collision map

	for ( index = 0; index < COLLISION_MAP_SIZE; ++index ) {
		collisionByte = collisionMap[index];

		if ( collisionByte == TILE_PLAYER ) {		
			
			playerStartX = mapX << 4;
			playerStartY = (mapY << 4) - 1;

		} else if ( ( collisionByte == TILE_ENEMY1START_RIGHT ) || ( collisionByte == TILE_ENEMY1START_LEFT ) ) {
			if ( enemyIndex < MAX_ENEMY_COUNT ) {
				enemyData[enemyIndex] = newEnemy;
				enemyData[enemyIndex].x = mapX << 4;
				enemyData[enemyIndex].y = (mapY << 4) - 1;
				enemyData[enemyIndex].direction = ( collisionByte == TILE_ENEMY1START_RIGHT ) ? PAD_RIGHT : PAD_LEFT;
				enemyData[enemyIndex].state = ENEMY_STATE_NORMAL;

				for ( i = 0; i < ENEMY_DATA_SIZE; ++i ) {
					enemySpriteData[enemyIndex][i] = enemySpriteDataTemplate[i];
				}

				setSpriteFrame(enemySpriteData[enemyIndex], enemyFrames[0]);
				++enemyIndex;				
			}

		} else if ( collisionByte == TILE_PLATFORM ) {
			if ( platformIndex < MAX_PLATFORM_COUNT ) {
				platformData[platformIndex] = newPlatform;
				platformData[platformIndex].x = mapX << 4;
				platformData[platformIndex].y = (mapY << 4) - 1;
				platformData[platformIndex].isActive = 1;
				platformData[platformIndex].direction = PAD_LEFT;
				++platformIndex;
			}
		} else if ( collisionByte == TILE_FRUIT ) {
			putMapTile(mapX << 1, mapY << 1, fruitFrame);
		} else if ( collisionByte == TILE_JEWEL ) {
			putMapTile(mapX << 1, mapY << 1, jewelFrame);
		}

		++mapX;
		if ( mapX > 15 ) {
			mapX = 0;
			++mapY;
		}
	}

	playerX = playerStartX;
	playerY = playerStartY;
	numEnemies = enemyIndex;

	
}

/*********** Nametable/Background Updates ***********/

void __fastcall__ putStr(unsigned int adr, const char *str)
{
	vram_adr(adr);
	while(1)
	{
		if(!*str) break;
		vram_put((*str++) + 0xA0); // +0xA0 because ASCII code 0x20 is placed in tile C0 of the CHR
	}
}

void __fastcall__ updateMapTile(u8 mapX, u8 mapY, const u8 * tileFrame) {
	tileUpdateList[0] = MSB(NTADR_A(mapX, mapY));
	tileUpdateList[1] = LSB(NTADR_A(mapX, mapY));
	tileUpdateList[2] = tileFrame[0];

	tileUpdateList[3] = MSB(NTADR_A(mapX + 1, mapY));
	tileUpdateList[4] = LSB(NTADR_A(mapX + 1, mapY));
	tileUpdateList[5] = tileFrame[1];

	tileUpdateList[6] = MSB(NTADR_A(mapX, mapY + 1));
	tileUpdateList[7] = LSB(NTADR_A(mapX, mapY + 1));
	tileUpdateList[8] = tileFrame[2];

	tileUpdateList[9] = MSB(NTADR_A(mapX + 1, mapY + 1));
	tileUpdateList[10] = LSB(NTADR_A(mapX + 1, mapY + 1));
	tileUpdateList[11] = tileFrame[3];

	set_vram_update(tileUpdateList);	
}

void __fastcall__ putMapTile(u8 mapX, u8 mapY, const u8 * tileFrame) {
	vram_adr(NTADR_A(mapX, mapY++));
	vram_put(*tileFrame++);
	vram_put(*tileFrame++);
	vram_adr(NTADR_A(mapX, mapY));
	vram_put(*tileFrame++);
	vram_put(*tileFrame++);
}

/*********** Sprite Management ***********/



void __fastcall__ setSpriteFrame(u8 *sprite, const u8 *frame) {
	sSpriteIndex = 2;
	sFrameIndex = 0;

	*(sprite + 2) = *(frame);
	*(sprite + 6) = *(frame + 1);
	*(sprite + 10) = *(frame + 2);
	*(sprite + 14) = *(frame + 3);
}

void __fastcall__ setSpritePalette(u8 *sprite, u8 palette) {
	u8 i;
	for ( i = 3; i <= 15; i = i + 4 ) {
		sprite[i] &= ~(0x3);
		sprite[i] |= palette;
	}
}

void __fastcall__ setSpritePriority(u8 *sprite, u8 priority) {
	if ( priority ) {
		sprite[3] |= OAM_BEHIND;
		sprite[7] |= OAM_BEHIND;
		sprite[11] |= OAM_BEHIND;
		sprite[15] |= OAM_BEHIND;
	} else {
		sprite[3] &= ~OAM_BEHIND;
		sprite[7] &= ~OAM_BEHIND;
		sprite[11] &= ~OAM_BEHIND;
		sprite[15] &= ~OAM_BEHIND;
	}
}

void __fastcall__ flipSprite(u8 *sprite, u8 flip) {

	if ( flip ) {
		sprite[0] = 8;
		sprite[3] |= OAM_FLIP_H;

		sprite[4] = 0;
		sprite[7] |= OAM_FLIP_H;

		sprite[8] = 8;
		sprite[11] |= OAM_FLIP_H;

		sprite[12] = 0;
		sprite[15] |= OAM_FLIP_H;
	} else {
		sprite[0] = 0;
		sprite[3] &= ~OAM_FLIP_H;

		sprite[4] = 8;
		sprite[7] &= ~OAM_FLIP_H;

		sprite[8] = 0;
		sprite[11] &= ~OAM_FLIP_H;

		sprite[12] = 8;
		sprite[15] &= ~OAM_FLIP_H;		
	}
}


void spriteCount(void) {
	if ( ++enemySpriteCount >= numEnemies ) {
		enemySpriteCount = 0;
	}	
}



void updateEnemySprites(void) {
	// update enemy sprites
	for ( i = 0; i < numEnemies; ++i ) {

		spriteCount();
		spriteFlickerIndex = enemySpriteCount;

		currentEnemy = &(enemyData[spriteFlickerIndex]);
		
		if ( currentEnemy->state == ENEMY_STATE_NORMAL ) {
			// animate
			if ( ( frameCount & 0x0F ) == 0x0F ) {
				currentEnemy->frame ^= 1;
				setSpriteFrame(enemySpriteData[spriteFlickerIndex], enemyFrames[currentEnemy->frame]);
			}

			setSpritePalette(enemySpriteData[spriteFlickerIndex], 0x3);
		}
			

		if ( ( currentEnemy->state == ENEMY_STATE_NORMAL ) || ( currentEnemy->state == ENEMY_STATE_GLUED ) ) {
			//setSpritePriority(enemySpriteData[i], sprPriorityToggle);
			//sprPriorityToggle ^= 1;
			oamSpriteIndex = oam_meta_spr(currentEnemy->x, currentEnemy->y, oamSpriteIndex, enemySpriteData[spriteFlickerIndex]);				
		} 
	}	
}

void updateGlueSprites(void) {
	for ( i = 0; i < MAX_GLUE_COUNT; ++i ) {
		gluePointer = &(glueData[i]);
		if ( gluePointer->state == GLUE_STATE_ACTIVE ) {
			if ( ( frameCount & 0x1F ) == 0x1F ) {
				gluePointer->frame ^= 1;
				flipSprite(gluePointer->spriteData, gluePointer->frame);
			}
			if ( gluePointer->timeLeft < 60 ) {
				setSpriteFrame(gluePointer->spriteData, glueFrames[GLUE_FRAME_SMALL]);
			} else if ( gluePointer->timeLeft < 120 ) {
				setSpriteFrame(gluePointer->spriteData, glueFrames[GLUE_FRAME_MEDIUM]);
			}

			oamSpriteIndex = oam_meta_spr(gluePointer->x, gluePointer->y, oamSpriteIndex, gluePointer->spriteData);	
		}
	}
}

void updatePlatformSprites(void) {
	for ( i = 0; i < MAX_PLATFORM_COUNT; ++i ) {
		platformPointer = &(platformData[i]);
		if ( platformPointer->isActive == 1 ) {
			oamSpriteIndex = oam_meta_spr(platformPointer->x, platformPointer->y, oamSpriteIndex, platformSpriteDataTemplate);	
		}
	}
}

void updatePlayerSprite(void) {

	if ( playerState == PLAYER_STATE_NORMAL ) {

		if ( ( pad & PAD_LEFT) || ( pad & PAD_RIGHT ) ) {
			if ( pad & PAD_RIGHT ) {
				flipSprite(playerSpriteData, 1);
			} else if ( pad & PAD_LEFT ) {
				flipSprite(playerSpriteData, 0);
			}		

			// animate walking player sprite
			if ( ( frameCount & PLAYER_WALK_ANIMATE_INTERVAL ) == PLAYER_WALK_ANIMATE_INTERVAL ) {
				playerFrame ^= 1;
			} 								
		} else {
			playerFrame = PLAYER_FRAME_STANDING;
		}

		setSpriteFrame(playerSpriteData, playerFrames[playerFrame]);

	} else if ( playerState == PLAYER_STATE_JUMPING ) {

		if ( pad & PAD_RIGHT ) {
			flipSprite(playerSpriteData, 1);
		} else if ( pad & PAD_LEFT ) {
			flipSprite(playerSpriteData, 0);
		}		

		setSpriteFrame(playerSpriteData, playerFrames[PLAYER_FRAME_JUMPING]);

	} else if ( playerState == PLAYER_STATE_CLIMBING ) {

		if ( ( frameCount & PLAYER_WALK_ANIMATE_INTERVAL ) == PLAYER_WALK_ANIMATE_INTERVAL ) {
			playerFrame ^= 1;
			flipSprite(playerSpriteData, playerFrame);
		}

		setSpriteFrame(playerSpriteData, playerFrames[PLAYER_FRAME_CLIMBING]);
	} else if ( playerState == PLAYER_STATE_DEAD ) {
		setSpriteFrame(playerSpriteData, playerFrames[PLAYER_FRAME_DEAD]);
	}
	
	// update player sprite
	oamSpriteIndex = oam_meta_spr(playerX, playerY, oamSpriteIndex, playerSpriteData);	
}

void drawScoreboard(void) {
	// TODO: flesh this out
	// CONSIDER: using separate nametable with sprite zero hit trick

	//pal_col(1,0x30); //set white color
	//vram_adr(NTADR_A(0, 1));
	// vram_fill(0x10, 5);
	vram_adr(NTADR_A(2, 3));
	vram_put(0xDD);
	putStr(NTADR_A(3, 3), "000000");
}


//static u8 playerScoreString[6];

void updateScoreboard(void) {

	if ( !scoreChanged ) {
		return;
	}

	valueToConvert = playerScore;

	for ( i = 0; i < 4; ++i ) {
		digitsArray[i] = 0;
	}

	decadeCount = 0;

	while ( valueToConvert >= 1000 ) {
		valueToConvert -= 1000;
		++decadeCount;
	}

	digitsArray[0] = decadeCount;
	decadeCount = 0;

	while ( valueToConvert >= 100 ) {
		valueToConvert -= 100;
		++decadeCount;
	}

	digitsArray[1] = decadeCount;
	decadeCount = 0;

	while ( valueToConvert >= 10 ) {
		valueToConvert -= 10;
		++decadeCount;
	}

	digitsArray[2] = decadeCount;
	digitsArray[3] = valueToConvert;
	

	
	for ( i = 0; i < 4; ++i ) {
		scoreUpdateList[3 + i] = digitsArray[i] + 0xD0;
	}
	
	set_vram_update(scoreUpdateList);
	scoreChanged = 0;
}

void __fastcall__ addScore(u8 addValue) {
	playerScore += addValue;
	scoreChanged = 1;
}

/*********** Collision Checking ***********/

void __fastcall__ four_Sides(u8 originX, u8 originY) {

	leftSide = originX + 2;
	rightSide = originX + 14;
	topSide = originY + 4;
	bottomSide = originY + 15;
}

void __fastcall__ glueFourSides(u8 originX, u8 originY) {

	leftSide = originX + 6;
	rightSide = originX + 9;
	topSide = originY + 6;
	bottomSide = originY + 14;
}

void __fastcall__ four_SidesSmall(u8 originX, u8 originY) {

	leftSide = originX + 1;
	rightSide = originX + 7;
	topSide = originY;
	bottomSide = originY + 7;
}

void __fastcall__ getCollisionIndex(u8 screenX, u8 screenY) {
	collisionIndex = ((screenX & 0xf0) >> 4) + (screenY & 0xf0);
}

void __fastcall__ collideCheckVertical(u8 originX, u8 originY, u8 direction) {

	leftSide = originX + 3;
	rightSide = originX + 13;
	topSide = originY + 10;
	bottomSide = originY + 16;

	collisionIndex = 0;
	verticalCollideCheck = TILE_NOCOLLIDE;

	if ( direction & PAD_UP ) {
		getCollisionIndex(rightSide - 4, topSide);
		if ( collisionMap[collisionIndex] != TILE_ALLCOLLIDE )  {
			getCollisionIndex(leftSide + 4, topSide);
		}
		verticalCollideCheck = collisionMap[collisionIndex];	
	} else if ( direction & PAD_DOWN ) {
		getCollisionIndex(rightSide, bottomSide);
		if ( collisionMap[collisionIndex] != TILE_ALLCOLLIDE ) {
			getCollisionIndex(leftSide, bottomSide);
		}
		verticalCollideCheck = collisionMap[collisionIndex];			

		if ( collisionMap[collisionIndex] != TILE_ALLCOLLIDE ) {
			// check platform collision
			for ( platformIndex = 0; platformIndex < MAX_PLATFORM_COUNT; ++platformIndex ) {
				platformPointer = &(platformData[platformIndex]);

				if ( platformPointer->isActive == 1 ) {
					platformTop = platformPointer->y;
					platformBottom = platformPointer->y + 16;
					platformLeft = platformPointer->x;
					platformRight = platformPointer->x + 16;

					if ( !( rightSide  <  platformLeft || 
							leftSide   > platformRight || 
							bottomSide <  platformTop  ||
							topSide    > platformBottom ) ) {

						verticalCollideCheck = TILE_PLATFORM;
						platformPointer->playerColliding = 1;
						playerPlatformStuck = 1;
					} else {
						platformPointer->playerColliding = 0;
					}
				}
			}

		}
	}

	
}


void __fastcall__ collideCheckHorizontal(u8 originX, u8 originY, u8 direction) {

	leftSide = originX + 3;
	rightSide = originX + 13;
	topSide = originY + 10;
	bottomSide = originY + 16;

	if ( direction & PAD_LEFT ) {
		getCollisionIndex(leftSide, topSide);
		if ( collisionMap[collisionIndex] == TILE_NOCOLLIDE ) {
			getCollisionIndex(leftSide, bottomSide);
		}
	} else if ( direction & PAD_RIGHT ) {
		getCollisionIndex(rightSide, topSide);
		if ( collisionMap[collisionIndex] == TILE_NOCOLLIDE ) {
			getCollisionIndex(rightSide, bottomSide);
		}
	}

	horizontalCollideCheck = collisionMap[collisionIndex];
}

void __fastcall__ collideCheckHorizontalFull(u8 originX, u8 originY, u8 direction) {

	leftSide = originX;
	rightSide = originX + 15;
	topSide = originY + 8;

	if ( direction & PAD_LEFT ) {
		getCollisionIndex(leftSide, topSide);
	} else if ( direction & PAD_RIGHT ) {
		getCollisionIndex(rightSide, topSide);
	}

	horizontalCollideCheck = collisionMap[collisionIndex];
}

void __fastcall__ bgHorizCollideCheck(u8 *x, u8 *y, u8 dir) {
	collideCheckHorizontal(*x, *y, dir);
	if ( horizontalCollideCheck == TILE_ALLCOLLIDE ) {
		if ( dir & PAD_LEFT ) {
			*x = (*x & 0xf0) + 13;

		} else if ( dir & PAD_RIGHT ) {
			//*x = (*x & 0xf0) + 5;
			*x = (*x & 0xf0) + 2;
		}		
	}
}

void checkPlayerLadderCollision(void) {
	
	leftSide = playerX + 6;
	rightSide = playerX + 10;
	bottomSide = playerY + 15;
	

	getCollisionIndex(leftSide, bottomSide);
	collisionLeft = collisionMap[collisionIndex];
	getCollisionIndex(rightSide, bottomSide);
	collisionRight = collisionMap[collisionIndex];


	if (
		( ( collisionLeft == TILE_LADDER ) || ( collisionLeft == TILE_LADDER_TOP ) ) &&
		( ( collisionRight == TILE_LADDER ) || ( collisionRight == TILE_LADDER_TOP ) ) 
		)
	{
		playerState = PLAYER_STATE_CLIMBING;
		
		if ( ( collisionLeft == TILE_LADDER ) || ( collisionLeft == TILE_LADDER_TOP ) ) {
			playerX = ( playerX + 7 ) & 0xf0;	
		} else if ( ( collisionRight == TILE_LADDER )  || ( collisionRight == TILE_LADDER_TOP ) ) {
			playerX = ( playerX + 11 ) & 0xf0;
		}
		
	} else {
		playerState = PLAYER_STATE_NORMAL;
	}
}

/*
void __fastcall__ bgVertCollideCheck(u8 *x, u8 *y, u8 dir) {
	collideCheckVertical(*x, *y, dir);
	if ( verticalCollideCheck ) {
		if ( dir & PAD_UP ) {
			// *y = (*y & 0xf8) + 15;
			*y = (*y & 0xf0);
		} else {
			*y = (*y & 0xf0);
		}
	}
}
*/


void playerEnemyCollideCheck(void) {

	enemyIndex = 0;
	enemyColliding = 0;
	enemyCollidedIndex = 0;
	playerSittingOnSprite = 0;

	while ( !enemyColliding && ( enemyIndex < numEnemies ) ) {
		currentEnemy = &(enemyData[enemyIndex]);
		enemyTop = currentEnemy->y + 3;
		enemyBottom = currentEnemy->y + 13;
		enemyLeft = currentEnemy->x + 2;
		enemyRight = currentEnemy->x + 14;

		if ( !( rightSide  <  enemyLeft  || 
				leftSide   > enemyRight || 
				bottomSide <  enemyTop   || 
				topSide    > enemyBottom ) ) {

			switch ( currentEnemy->state ) {

				case ENEMY_STATE_NORMAL: 
					enemyCollidedIndex = enemyIndex;
					killPlayer();
					break;

				case ENEMY_STATE_GLUED:

					if ( ( bottomSide >= (enemyTop) ) && ( bottomSide <= ( enemyTop + 4 ) ) ) {
						if ( !( pad & PAD_UP ) && ( playerVertVel < 0 ) ) {
							playerY = ( enemyTop - 15 );
							playerSittingOnSprite = 1;
							playerState = PLAYER_STATE_NORMAL;							
							playerFrame = PLAYER_FRAME_STANDING;																									
						}
					} else if ( ( rightSide > enemyLeft ) && ( leftSide < enemyLeft ) && ( pad & PAD_RIGHT ) ) {
						playerX = ( enemyLeft - 13 );
					} else if ( ( leftSide < enemyRight ) && ( rightSide > enemyRight ) && ( pad & PAD_LEFT ) ) { 
						playerX = ( enemyRight - 3 );
					} 
					break;

				case ENEMY_STATE_DEAD:
					break;
			}
		}		
		++enemyIndex;
	}
}

void genericEnemyCollideCheck(void) {

	enemyIndex = 0;
	enemyColliding = 0;
	enemyCollidedIndex = 0;

	while ( !enemyColliding && ( enemyIndex < numEnemies ) ) {
		currentEnemy = &(enemyData[enemyIndex]);
		enemyTop = currentEnemy->y + 2;
		enemyBottom = currentEnemy->y + 14;
		enemyLeft = currentEnemy->x + 2;
		enemyRight = currentEnemy->x + 14;

		if ( !( rightSide  <  enemyLeft  || 
				leftSide   >= enemyRight || 
				bottomSide <  enemyTop   || 
				topSide    >= enemyBottom ) ) {

			switch ( currentEnemy->state ) {
				case ENEMY_STATE_NORMAL: 
					enemyColliding = 1;
					enemyCollidedIndex = enemyIndex;
					break;
				case ENEMY_STATE_DEAD:
					break;
			}
		}		
		++enemyIndex;
	}
}

void glueEnemyCollideCheck(void) {
	for ( i = 0; i < MAX_GLUE_COUNT; ++i ) {
		gluePointer = &(glueData[i]);
		if ( gluePointer->state == GLUE_STATE_ACTIVE ) {
			glueFourSides(gluePointer->x, gluePointer->y);
			genericEnemyCollideCheck();
			if ( enemyColliding ) {
				// kill glue
				gluePointer->state = GLUE_STATE_INACTIVE;
				gluePointer->x = 0;
				gluePointer->y = 0;

				// freeze enemy
				currentEnemy = &(enemyData[enemyCollidedIndex]);
				currentEnemy->state = ENEMY_STATE_GLUED;
				currentEnemy->glueTimeLeft = GLUE_INIT_DURATION;

				sfx_play(SFX_GLUESTUCK, CHANNEL_SQUARE1);
			}
		}
	}
}



void glueCollideCheck(void) {

	glueIndex = 0;
	glueColliding = 0;
	glueCollidedIndex = 0;

	while ( !glueColliding && ( glueIndex < MAX_GLUE_COUNT ) ) {
		gluePointer = &(glueData[glueIndex]);
		if ( gluePointer->state == GLUE_STATE_ACTIVE ) {
			glueTop = gluePointer->y + 2;
			glueBottom = gluePointer->y + 14;
			glueLeft = gluePointer->x + 2;
			glueRight = gluePointer->x + 14;

			if ( !( rightSide  <  glueLeft  || 
					leftSide   >= glueRight || 
					bottomSide <  glueTop   || 
					topSide    >= glueBottom ) ) {
				
				glueColliding = 1;
				glueCollidedIndex = glueIndex;				
			}					
		}
		++glueIndex;
	}
}


/*********** State Management ***********/

void updateEnemyMovement(void) {

	for ( i = 0; i < numEnemies; i++ ) {
		currentEnemy = &(enemyData[i]);
		if ( currentEnemy->state == ENEMY_STATE_GLUED ) {
			--(currentEnemy->glueTimeLeft);
			if ( currentEnemy->glueTimeLeft <= 0 ) {
				currentEnemy->state = ENEMY_STATE_NORMAL;
			}
			continue;
		}
		collideCheckVertical(currentEnemy->x, currentEnemy->y + 1, PAD_DOWN);
		if ( ( verticalCollideCheck != TILE_ALLCOLLIDE ) && ( verticalCollideCheck != TILE_LADDER_TOP ) ) {
			currentEnemy->y += 1;
		} else {
			if ( currentEnemy->direction == PAD_RIGHT ) {
				currentEnemy->x += 1;
				 collideCheckHorizontal(currentEnemy->x, currentEnemy->y, PAD_RIGHT);
				if ( ( horizontalCollideCheck == TILE_ALLCOLLIDE ) || ( horizontalCollideCheck == TILE_ENEMYCOLLIDE ) ) {
					flipSprite(enemySpriteData[i], 0);
					currentEnemy->direction = PAD_LEFT;
				}
			} else {
				currentEnemy->x -= 1;
				collideCheckHorizontal(currentEnemy->x, currentEnemy->y, PAD_LEFT);
				if ( ( horizontalCollideCheck == TILE_ALLCOLLIDE ) || ( horizontalCollideCheck == TILE_ENEMYCOLLIDE ) ) {
					flipSprite(enemySpriteData[i], 1);
					currentEnemy->direction = PAD_RIGHT;
				}
			}			
		}
	}
}

void updatePlatforms(void) {

	for ( i = 0; i < MAX_PLATFORM_COUNT; i++ ) {
		platformPointer = &(platformData[i]);
		if ( platformPointer->isActive == 1 ) {
			if ( ( frameCount & 0x01 ) == 0x01 ) {
				if ( platformPointer->direction == PAD_RIGHT ) {
					platformPointer->x += 1;
					if ( platformPointer->playerColliding || playerPlatformStuck ) {
						playerX += 1;
					}
					collideCheckHorizontalFull(platformPointer->x, platformPointer->y, PAD_RIGHT);
					if ( ( horizontalCollideCheck == TILE_ALLCOLLIDE ) ) {
						platformPointer->direction = PAD_LEFT;
					}
				} else {
					if ( platformPointer->playerColliding || playerPlatformStuck ) {
						playerX -= 1;
					}
					platformPointer->x -= 1;
					collideCheckHorizontalFull(platformPointer->x, platformPointer->y, PAD_LEFT);
					if ( ( horizontalCollideCheck == TILE_ALLCOLLIDE ) ) {
						platformPointer->direction = PAD_RIGHT;
					}
				}		
			}
		}
	}
}


void checkPlayerGetItems() {
	
	getCollisionIndex(playerX + 8, playerY + 8);
	if ( collisionMap[collisionIndex] == TILE_FRUIT ) {
		addScore(SCORE_VALUE_FRUIT);
		collisionMap[collisionIndex] = TILE_NOCOLLIDE;
		updateMapTile((((playerX + 8) & 0xf0) >> 3), (((playerY + 8) & 0xf0) >> 3), emptyFrame);
		sfx_play(SFX_BEEP, CHANNEL_SQUARE1);

	} else if ( collisionMap[collisionIndex] == TILE_JEWEL ) {

		addScore(SCORE_VALUE_JEWEL);
		collisionMap[collisionIndex] = TILE_NOCOLLIDE;
		updateMapTile((((playerX + 8) & 0xf0) >> 3), (((playerY + 8) & 0xf0) >> 3), emptyFrame);		
		sfx_play(SFX_BEEP, CHANNEL_SQUARE1);
		delay(180);
		levelComplete = 1;
	} else if ( collisionMap[collisionIndex] == TILE_WATER ) {
		killPlayer();		
	}
}

void updatePlayerJumpFall(void) {

	collideBottom = 0;

	// stop ascent when player releases A button
	//if ( !( pad & PAD_A ) && playerJumping && ( playerVertVel > 0 ) ) {
		//playerVertVel = 0;
	//}

	if ( playerState == PLAYER_STATE_JUMPING ) {
		// *** Player is jumping
		
		playerY -= playerVertVel;
		if ( playerVertVel > 0 ) {
			// *** Player is moving up in jump

			collideCheckVertical(playerX, playerY, PAD_UP);
			// check collision above
			if ( ( verticalCollideCheck == TILE_ALLCOLLIDE ) || ( verticalCollideCheck == TILE_LADDER_TOP ) ) { 
				playerY = (playerY & 0xF0) + 6;
			}			
		} else {
			// *** Player is moving down in jump

			// check collision below
			collideCheckVertical(playerX, playerY + 2, PAD_DOWN);
			
			if ( ( verticalCollideCheck == TILE_ALLCOLLIDE ) || ( verticalCollideCheck == TILE_LADDER_TOP ) || ( verticalCollideCheck == TILE_PLATFORM ) ) { 
				// *** Player lands from jump				
				//playerY =  ( ( playerY + 2 ) & 0xF0 ) - 1;
				playerY =  ( ( playerY + 8 ) & 0xF0 ) - 1;
				collideBottom = 1;
				playerState = PLAYER_STATE_NORMAL;							
				playerFrame = PLAYER_FRAME_STANDING;
			}
		}

		// update velocity
		// accelerate toward ground
		if ( ( playerVertVel > MAX_PLAYER_VERT_VEL ) && ( playerJumpCounter == PLAYER_JUMP_COUNTER_INTERVAL ) ) {
			playerVertVel -= GRAVITY_ACCELERATION; 
			playerJumpCounter = 0;
		}

		++playerJumpCounter;
		
	} else if ( ( playerState == PLAYER_STATE_NORMAL ) || ( playerState == PLAYER_STATE_FALLING ) ) {
		// *** Player is not jumping

		// check collision below
		collideCheckVertical(playerX, playerY + 2, PAD_DOWN);

		if ( ( verticalCollideCheck == TILE_ALLCOLLIDE ) || ( verticalCollideCheck == TILE_LADDER_TOP ) || ( verticalCollideCheck == TILE_PLATFORM ) ) { 
			// *** Player is standing on the ground

			playerY = ((playerY + 8) & 0xF0) - 1;
			collideBottom = 1;
			playerState = PLAYER_STATE_NORMAL;			
		} else {
			// *** Player is falling
			playerY += PLAYER_FALL_SPEED;
			collideBottom = 0;
			playerState = PLAYER_STATE_FALLING;
		}	
	}

	if ( collideBottom || playerSittingOnSprite ) {
		// *** Actions player can take when the player is standing on the ground

		if ( ( jumpButtonReset != 0 ) && ( pad & PAD_A ) ) {
			// *** Start a jump

			playerPlatformStuck = 0;
			playerVertVel = PLAYER_INIT_JUMP_VEL;
			playerState = PLAYER_STATE_JUMPING;
			playerJumpCounter = 0;
			jumpButtonReset = 0;

			sfx_play(SFX_JUMP, CHANNEL_SQUARE1);

		} else if ( pad & PAD_UP ) {

			// *** See if we can move up a ladder
			checkPlayerLadderCollision();

		} else if ( ( pad & PAD_DOWN ) && ( verticalCollideCheck == TILE_LADDER_TOP ) ) {

			// *** Climb a little down the ladder and see if we've reached the bottom
			playerY += 2;
			checkPlayerLadderCollision();
		} 
	} 

}

void updatePlayerClimbing(void) {
	checkPlayerLadderCollision();
	
	if ( pad & PAD_UP ) { 
		// *** Climb up
		--playerY;
		playerPlatformStuck = 0;
	} else if ( pad & PAD_DOWN ) {
		// *** Climb down
		++playerY;
		collideCheckVertical(playerX, playerY + 1, PAD_DOWN);
		if ( ( verticalCollideCheck == TILE_ALLCOLLIDE ) ) { 	
			playerState = PLAYER_STATE_NORMAL;
		}	
	}
}

void updatePlayerVerticalMovement(void) {

	if ( !( pad & PAD_A ) ) {
		jumpButtonReset = 1;
	}	

	if ( playerState == PLAYER_STATE_CLIMBING ) {
		updatePlayerClimbing(); 
	} else {
		updatePlayerJumpFall();
	}
}

void updatePlayerHorizontalMovement(void) {	
	if ( ( playerState != PLAYER_STATE_NORMAL ) && ( playerState != PLAYER_STATE_JUMPING ) ) {
		return;
	}

	if ( pad & PAD_LEFT ) {
		playerDir = PAD_LEFT;
		playerPlatformStuck = 0;
		if ( playerX > 0 ) {
			playerX -= PLAYER_MOVE_VEL;
		}
	} else if ( pad & PAD_RIGHT ) {
		playerDir = PAD_RIGHT;
		playerPlatformStuck = 0;
		if ( playerX < 240 ) {
			playerX += PLAYER_MOVE_VEL;
		}
	}

	horizontalCollideCheck = TILE_NOCOLLIDE;

	bgHorizCollideCheck(&playerX, &playerY, pad);

	if ( horizontalCollideCheck != TILE_NOCOLLIDE ) {
		playerPlatformStuck = 0;
	}
}

void updatePlayerGlue(void) {
	u8 glueX, glueY;
	u8 newGlueIndex = 255;
	u8 duplicateFound = 0;

	if ( ! ( pad & PAD_B ) ) {
		glueButtonReset = 1;
	}

	if ( ( pad & PAD_B ) && ( glueButtonReset == 1 ) ) {
		glueButtonReset = 0;


		glueY = ((playerY + 1));
		if ( playerDir == PAD_RIGHT ) {
			glueX = playerX + 16;
		} else {
			glueX = playerX - 16;
		}

		getCollisionIndex(glueX + 8, glueY + 8);

		if ( ( collisionMap[collisionIndex] != TILE_ALLCOLLIDE ) && 
			 ( collisionMap[collisionIndex] != TILE_LADDER_TOP ) ) {

			// Create glue sprite
			i = 0;
		
			do {
				if ( glueData[i].state == GLUE_STATE_INACTIVE ) {
					newGlueIndex = i;	
				} else if ( glueData[i].collisionIndex == collisionIndex ) {
					duplicateFound = 1;
				}
				++i;
			} while ( ( newGlueIndex == 255 ) && ( i < MAX_GLUE_COUNT ) && !duplicateFound );
			

			if ( newGlueIndex < 255 ) {
				gluePointer = &(glueData[newGlueIndex]);
				gluePointer->x = glueX;
				gluePointer->y = glueY - 1;
				gluePointer->frame = 0;
				gluePointer->state = GLUE_STATE_ACTIVE;
				gluePointer->timeLeft = GLUE_INIT_LIFESPAN;
				memcpy(gluePointer->spriteData, glueSpriteDataTemplate, sizeof(glueSpriteDataTemplate));				

				sfx_play(SFX_GLUEDROP, CHANNEL_SQUARE1);								
			}

		}
		
	}
}

void killPlayer(void) {
	// kill player
	playerState = PLAYER_STATE_DEAD;
	--playerLives;
	playerAnimationCounter = 0;
}

void updatePlayerState(void) {

	if ( playerState == PLAYER_STATE_DEAD ) {
		++playerAnimationCounter;
		if ( playerAnimationCounter == PLAYER_DEAD_INTERVAL ) {
			ppu_off();
			setupMap();
			ppu_on_all(); 
			playerState = PLAYER_STATE_NORMAL;
			playerAnimationCounter = 0;
		}
	} else {
		updatePlayerHorizontalMovement();
		updatePlayerVerticalMovement();
		updatePlayerGlue();		
	}
}

void updateGlues(void) {
	for ( i = 0; i < MAX_GLUE_COUNT; ++i ) {
		gluePointer = &(glueData[i]);
		if ( gluePointer->state == GLUE_STATE_ACTIVE ) {
			--(gluePointer->timeLeft); 

			if ( gluePointer->timeLeft <= 0 ) {
				// *** clear glue if its time has run out
				gluePointer->state = GLUE_STATE_INACTIVE;
				gluePointer->collisionIndex = 0;
			} else {
				// make glue fall 
				
				getCollisionIndex(gluePointer->x + 8, gluePointer->y + 18);
				
				if ( ( collisionMap[collisionIndex] == TILE_ALLCOLLIDE ) || ( collisionMap[collisionIndex] == TILE_LADDER_TOP ) ) {
					gluePointer->collisionIndex = collisionIndex;
					gluePointer->y = ((gluePointer->y + 8 ) & 0xF0) - 1;
				} else if ( collisionMap[collisionIndex] == TILE_WATER ) {
					// kill glue
					gluePointer->state = GLUE_STATE_INACTIVE;
				} else {
					four_Sides(gluePointer->x, gluePointer->y + 2);
					glueCollideCheck();
				 	if ( glueColliding && ( glueCollidedIndex != i ) ) {
						// clear glue if it's colliding with another glue
						gluePointer->state = GLUE_STATE_INACTIVE;
						gluePointer->collisionIndex = 0;
					} else {
						gluePointer->y += 2;
					}					

				}

			}
		} 
	}
}

/*********** Main ***********/

void main(void)
{
	int currentLevel = 0;

	const u8 * levels[2] = {
		map2,
		newmap
	};

	const u8 * levelCollisions[2] = {
		map2_coll,
		newmap_coll
	};

	memcpy(tileUpdateList, tileUpdateListInit, sizeof(tileUpdateListInit));
	memcpy(scoreUpdateList, scoreUpdateListData, sizeof(scoreUpdateListData));	

	memcpy(palSprites, paldat, 16);

	//unrleCollision();

	pal_spr(palSprites);
	pal_bg(bgPalette);
	

	//set initial coords
	playerDir = PAD_LEFT;

	//init other vars
	touch = 0; // collision flag
	frameCount = 0; // frame counter

	playerFrame = 0;
	
	setSpriteFrame(playerSpriteData, playerFrames[playerFrame]);
	playerLives = PLAYER_INIT_LIVES;

	//vram_adr(NAMETABLE_A); //unpack nametable into VRAM
	//vram_unrle(map2);	

	while ( 1 ) {


		ppu_off();

		vram_adr(NAMETABLE_A);
		vram_unrle(levels[currentLevel]);
		currentCollisionData = levelCollisions[currentLevel];

		setupMap();	
		
		ppu_off();
		drawScoreboard();
		ppu_on_all();

		levelComplete = 0;

		// now the main loop
		
		while ( !levelComplete )
		{
			ppu_wait_frame(); // wait for next TV frame
			updateScoreboard();

		
			//process player
			
			oamSpriteIndex = 0;
			i = 0;

			sprPriorityToggle = frameCount & 0xFE;

			// update player movement
			pad = pad_poll(i);
			

			updatePlayerSprite();
			updateEnemySprites();
			updateGlueSprites();
			updatePlatformSprites();
			spriteCount();
			
			updatePlayerState();

			if ( playerState != PLAYER_STATE_DEAD ) {
				checkPlayerGetItems();
				updateEnemyMovement();	
				updateGlues();
				updatePlatforms();
				enemyColliding = 0;
				four_Sides(playerX, playerY);
				playerEnemyCollideCheck();
				playerEnemyColliding = enemyColliding;
				glueEnemyCollideCheck();			
			}
			
			
			oam_hide_rest(oamSpriteIndex);

			++frameCount;
		}	

		

		++currentLevel;
		if ( currentLevel > 1 ) {
			currentLevel = 0;
		}

		
	}


	
}