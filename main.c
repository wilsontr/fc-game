

#include <stdint.h>
#include <stdlib.h>
#include "neslib.h"
#include "newmap.h"
#include "newmap_coll.h"


void __fastcall__ memcpy(void *dst, void *src, unsigned int len);


typedef uint8_t u8;
typedef uint16_t u16;

#define ENEMY_DATA_SIZE	17
#define NUM_ENEMIES 	4

#define TILE_NOCOLLIDE   		0
#define TILE_ALLCOLLIDE   		1
#define TILE_ENEMYCOLLIDE 		2
#define TILE_PLAYERSTART  		3
#define TILE_ENEMY1START_LEFT 	4	
#define TILE_ENEMY1START_RIGHT	5	
#define TILE_LADDER				6
#define TILE_LADDER_TOP			7
#define TILE_GLUE				8

#define MAX_JUMP_HEIGHT			 100

// TODO Think about RLE-encoding collision maps again
#define COLLISION_MAP_SIZE 	     204

#define POTION_HORIZ_VELOCITY	 2
#define POTION_INIT_VERTICAL_VEL 2

#define PLAYER_INIT_JUMP_VEL 	 3

#define GLUE_INIT_LIFESPAN	 	 240
#define MAX_GLUE_COUNT 			 5


#define PLAYER_STATE_NORMAL		 0
#define PLAYER_STATE_DEAD		 1
#define PLAYER_STATE_CLIMBING	 2
#define PLAYER_STATE_JUMPING	 3
#define PLAYER_STATE_FALLING	 4
#define PLAYER_JUMP_VEL   		 2
#define PLAYER_MOVE_VEL			 1

#define PLAYER_FALL_SPEED		 3
#define PLAYER_JUMP_COUNTER_INTERVAL 6
#define GRAVITY_ACCELERATION 	 2

#define PLAYER_FRAME_STANDING 	 0
#define PLAYER_FRAME_CLIMBING	 2
#define PLAYER_FRAME_JUMPING	 3

#define PLAYER_WALK_ANIMATE_INTERVAL 0x07

#define ENEMY_STATE_NORMAL 		 0
#define ENEMY_STATE_MUSHROOM	 1
#define ENEMY_STATE_DEAD		 3

#define POTION_TOSS_WAIT_TIME	60

#define SFX_JUMP			0
#define SFX_GLUEDROP		1

#define CHANNEL_SQUARE1 	0
#define CHANNEL_SQUARE2 	1
#define CHANNEL_TRIANGLE 	2
#define CHANNEL_NOISE	 	3


#pragma bss-name (push, "ZEROPAGE")
#pragma data-name (push, "ZEROPAGE")

u8 oam_off;
static u8 playerX = 0;
static u8 playerY = 0;
static u8 enemyIndex = 0;
static u8 frameCount;
static u8 enemyColliding = 0;
static u8 i;
static u16 collisionIndex;
static u8 leftSide, rightSide, topSide, bottomSide;
static u8 potionX;
static u8 potionY;
static u8 potionTossTimer;

#pragma data-name(pop)
#pragma bss-name (pop)

static uint8_t * collisionMap;


/* Commonly-used test variables */

static u8 playerStartX = 50;
static u8 playerStartY = 50;


/* Bookkeeping and state */

extern const u8 paldat[];


static u8 pad, oamSpriteIndex;
static u8 touch;
static u8 spriteFlickerIndex = 0;
static u8 sprPriorityToggle = 0;

static u8 palSprites[4];
static u8 palBG[4];

static u8 horizontalCollideCheck;
static u8 verticalCollideCheck;
static u8 collideBottom;
static u8 collisionLeft, collisionRight;


/* Player */
static u8 playerDir;
static u8 playerEnemyColliding;
static u8 playerFrame = 0;
static u8 playerJumpCounter = 0;
static u8 playerState = PLAYER_STATE_NORMAL;
static signed char playerVertVel = 0;
static u8 jumpButtonReset = 1;


const u8 playerFrames[4][4] = {
	{ 0x08, 0x09, 0x18, 0x19 },
	{ 0x28, 0x29, 0x38, 0x39 },
	{ 0x68, 0x69, 0x78, 0x79 },
	{ 0x48, 0x49, 0x58, 0x59 }
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
	u8 active;
	u8 spriteData[17];
};

typedef struct glueStruct glue;

static glue glueData[MAX_GLUE_COUNT];
static glue *gluePointer;

const u8 glueSpriteDataTemplate[17] = {
	0, 0, 0x46, 0x0,
	8, 0, 0x47, 0x0,
	0, 8, 0x56, 0x0,
	8, 8, 0x57, 0x0,
	128
};

const u8 glueTileData[4] = {
	0x46, 0x47, 0x56, 0x57
};


/* Potion */
/*
static u8 potionIsActive = 0;
static u8 potionDirection;
static signed char potionVerticalVel = 0;
static u8 potionMoveCounter = 0;

u8 potionSpriteData[5] = {
	0, 0, 0x2A, 0x0,
	128
};
*/

/* Mushroom */

/*
const u8 mushroomSpriteDataTemplate[17] = {
	0, 0, 0x2B, 0x2,
	8, 0, 0x2C, 0x2,
	0, 8, 0x3B, 0x2,
	8, 8, 0x3C, 0x2,
	128
};
*/

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
	u8 collidingWithPotion;
	u8 state;
};

typedef struct enemyStruct enemy;

enemy enemyData[8];

const u8 enemyFrames[2][4] = {
	{ 0x06, 0x07, 0x16, 0x17 },
	{ 0x26, 0x27, 0x36, 0x37 }
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

void killPotion(void);
void setupMap(void);
void __fastcall__ setSpriteFrame(u8 *sprite, const u8 *frame);
void __fastcall__ setSpritePalette(u8 *sprite, u8 palette);

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
	u8 collByte, k;
	u8 mapX = 0;
	u8 mapY = 0;
	u8 index = 0;
	enemy newEnemy = { 0, 0, 0, PAD_LEFT };
	potionTossTimer = 0;

	potionX = -8;
	potionY = -8;

	collisionMap = (u8 *) newmap_coll;

	enemyIndex = 0;


	// initialize glue data

	for ( index = 0; index < sizeof(MAX_GLUE_COUNT); ++index ) {
		gluePointer = &(glueData[index]);
		gluePointer->active = 0;
	}

	
	// populate the screen based on the collision map

	for ( index = 0; index < COLLISION_MAP_SIZE; ++index ) {
		collByte = collisionMap[index];

		if ( collByte == TILE_PLAYERSTART ) {
			
			playerStartX = mapX << 4;
			playerStartY = (mapY << 4) - 1;

		} else if ( ( collByte == TILE_ENEMY1START_RIGHT ) || ( collByte == TILE_ENEMY1START_LEFT ) ) {
			
			enemyData[enemyIndex] = newEnemy;
			enemyData[enemyIndex].x = mapX << 4;
			enemyData[enemyIndex].y = (mapY << 4) - 1;
			enemyData[enemyIndex].direction = ( collByte == TILE_ENEMY1START_RIGHT ) ? PAD_RIGHT : PAD_LEFT;
			enemyData[enemyIndex].collidingWithPotion = 0;
			enemyData[enemyIndex].state = ENEMY_STATE_NORMAL;

			for ( k = 0; k < ENEMY_DATA_SIZE; ++k ) {
				enemySpriteData[enemyIndex][k] = enemySpriteDataTemplate[k];
			}

			setSpriteFrame(enemySpriteData[enemyIndex], enemyFrames[0]);
			++enemyIndex;
		}

		++mapX;
		if ( mapX >= 16 ) {
			mapX = 0;
			++mapY;
		}
	}

	numEnemies = enemyIndex;

}


/*********** Sprite Management ***********/

static u8 sSpriteIndex, sFrameIndex;

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

			if ( currentEnemy->collidingWithPotion ) {
				setSpritePalette(enemySpriteData[spriteFlickerIndex], 0x0);
			} else {
				setSpritePalette(enemySpriteData[spriteFlickerIndex], 0x3);
			}			

			//setSpritePriority(enemySpriteData[i], sprPriorityToggle);
			//sprPriorityToggle ^= 1;
			oamSpriteIndex = oam_meta_spr(currentEnemy->x, currentEnemy->y, oamSpriteIndex, enemySpriteData[spriteFlickerIndex]);				
		} 
	}	
}

void updateGlueSprites(void) {
	
	

	for ( i = 0; i < MAX_GLUE_COUNT; ++i ) {
		gluePointer = &(glueData[i]);
		if ( gluePointer->active == 1 ) {
			oamSpriteIndex = oam_meta_spr(gluePointer->x, gluePointer->y, oamSpriteIndex, glueSpriteDataTemplate);	
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
	}
	
	// update player sprite
	oamSpriteIndex = oam_meta_spr(playerX, playerY, oamSpriteIndex, playerSpriteData);	
}

void drawScoreboard(void) {
	// TODO: flesh this out
	// TODO: set correct pal color (any bg pal entry with white as color #1)
	// CONSIDER: using separate nametable with sprite zero hit trick

	//pal_col(1,0x30); //set white color
	vram_adr(NTADR_A(0, 1));
	vram_fill(0x10, 5);
}

u8 updateListData[8] = { 
	MSB(NTADR_A(0, 1)) | NT_UPD_HORZ, // MSB
	LSB(NTADR_A(0, 1)),  // LSB
	4, // Byte count
	0xA1, 0xA2, 0xA3, 0xA4, // Bytes to write
	NT_UPD_EOF // EOF
};

u8 updateList[8];

void updateScoreboard(void) {
	memcpy(updateList, updateListData, sizeof(updateListData));

	set_vram_update(updateList);
}
/*********** Collision Checking ***********/

void __fastcall__ four_Sides(u8 originX, u8 originY) {

	leftSide = originX + 1;
	rightSide = originX + 15;
	topSide = originY;
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


u8 __fastcall__ smallCollideCheckVertical(u8 originX, u8 originY, u8 direction) {

	leftSide = originX;
	rightSide = originX + 8;
	topSide = originY;
	bottomSide = originY + 8;

	collisionIndex = 0;

	if ( direction & PAD_UP ) {
		getCollisionIndex(rightSide, topSide);
		if ( collisionMap[collisionIndex] != TILE_ALLCOLLIDE ) {
			getCollisionIndex(leftSide, topSide);
		}
	} else if ( direction & PAD_DOWN ) {
		getCollisionIndex(rightSide, bottomSide);
		if ( collisionMap[collisionIndex] != TILE_ALLCOLLIDE ) {
			getCollisionIndex(leftSide, bottomSide);
		}
	}

	if ( collisionIndex ) {
		return collisionMap[collisionIndex];	
	} else {
		return 0;
	}
	
}


void __fastcall__ collideCheckVertical(u8 originX, u8 originY, u8 direction) {

	leftSide = originX + 1;
	rightSide = originX + 15;
	topSide = originY + 1;
	bottomSide = originY + 17;

	collisionIndex = 0;

	if ( direction & PAD_UP ) {
		getCollisionIndex(rightSide, topSide);
		if ( collisionMap[collisionIndex] != TILE_ALLCOLLIDE )  {
			getCollisionIndex(leftSide, topSide);
		}
	} else if ( direction & PAD_DOWN ) {
		getCollisionIndex(rightSide, bottomSide);
		if ( collisionMap[collisionIndex] != TILE_ALLCOLLIDE ) {
			getCollisionIndex(leftSide, bottomSide);
		}
	}

	verticalCollideCheck = collisionMap[collisionIndex];	
}

void __fastcall__ collideCheckHorizontal(u8 originX, u8 originY, u8 direction) {

	leftSide = originX + 1;
	rightSide = originX + 15;
	topSide = originY + 4;
	bottomSide = originY + 12;

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

void __fastcall__ bgHorizCollideCheck(u8 *x, u8 *y, u8 dir) {
	collideCheckHorizontal(*x, *y, dir);
	if ( horizontalCollideCheck == TILE_ALLCOLLIDE ) {
		if ( dir & PAD_LEFT ) {
			*x = (*x & 0xf8) + 7;
		} else if ( dir & PAD_RIGHT ) {
			*x = (*x & 0xf8);
		}		
	}
}

void checkPlayerLadderCollision(void) {
	
	leftSide = playerX + 3;
	rightSide = playerX + 11;
	//topSide = playerY + 9;
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
			playerX = ( playerX + 3 ) & 0xf0;	
		} else if ( ( collisionRight == TILE_LADDER )  || ( collisionRight == TILE_LADDER_TOP ) ) {
			playerX = ( playerX - 3 ) & 0xf0;
		}
		
	} else {
		playerState = PLAYER_STATE_NORMAL;
	}
}

void __fastcall__ bgVertCollideCheck(u8 *x, u8 *y, u8 dir) {
	collideCheckVertical(*x, *y, dir);
	if ( verticalCollideCheck ) {
		if ( dir & PAD_UP ) {
			*y = (*y & 0xf8) + 15;
		} else {
			*y = (*y & 0xf8);
		}
	}
}

void enemyCollideCheck(void) {

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
				case ENEMY_STATE_MUSHROOM:
					currentEnemy->state = ENEMY_STATE_DEAD;
					break;
				case ENEMY_STATE_DEAD:
					break;
			}
		}		
		++enemyIndex;
	}
}

// void potionEnemyCollideCheck(void) {
// 	enemyColliding = 0;

// 	if ( potionIsActive ) {
// 		four_SidesSmall(potionX, potionY);
// 		enemyCollideCheck();
// 		if ( enemyColliding ) {
// 			killPotion();
// 			enemyData[enemyCollidedIndex].state = ENEMY_STATE_MUSHROOM;
// 		}
// 	}

// 	enemyColliding = 0;
// }




/*********** State Management ***********/

void updateEnemyMovement(void) {

	for ( i = 0; i < numEnemies; i++ ) {
		currentEnemy = &(enemyData[i]);
		if ( currentEnemy->state == ENEMY_STATE_MUSHROOM ) {
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
				playerY = (playerY & 0xF8) + 6;
			}			
		} else {
			// *** Player is moving down in jump

			// check collision below
			collideCheckVertical(playerX, playerY + 2, PAD_DOWN);
			
			if ( ( verticalCollideCheck == TILE_ALLCOLLIDE ) || ( verticalCollideCheck == TILE_LADDER_TOP ) ) { 
				// *** Player lands from jump				
				playerY = (playerY & 0xF8) + 7;
				collideBottom = 1;
				playerState = PLAYER_STATE_NORMAL;							
				playerFrame = PLAYER_FRAME_STANDING;
			}
		}

		// update velocity
		// accelerate toward ground
		if ( ( playerVertVel > -3 ) && ( playerJumpCounter == PLAYER_JUMP_COUNTER_INTERVAL ) ) {
			playerVertVel -= GRAVITY_ACCELERATION; 
			playerJumpCounter = 0;
		}

		++playerJumpCounter;
		
	} else if ( ( playerState == PLAYER_STATE_NORMAL ) || ( playerState == PLAYER_STATE_FALLING ) ) {
		// *** Player is not jumping

		// check collision below
		collideCheckVertical(playerX, playerY + 4, PAD_DOWN);

		if ( ( verticalCollideCheck == TILE_ALLCOLLIDE ) || ( verticalCollideCheck == TILE_LADDER_TOP ) ) { 
			// *** Player is standing on the ground
			playerY = (playerY & 0xF8) + 7;
			collideBottom = 1;
			playerState = PLAYER_STATE_NORMAL;			
		} else {
			// *** Player is falling
			playerY += PLAYER_FALL_SPEED;
			collideBottom = 0;
			playerState = PLAYER_STATE_FALLING;
		}	
	}

	if ( collideBottom ) {
		// *** Actions player can take when the player is standing on the ground

		if ( ( jumpButtonReset != 0 ) && ( pad & PAD_A ) ) {
			// *** Start a jump

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
			playerY++;
			checkPlayerLadderCollision();
		} 
	} 
}

void updatePlayerClimbing(void) {
	checkPlayerLadderCollision();
	
	if ( pad & PAD_UP ) { 
		// *** Climb down
		--playerY;
	} else if ( pad & PAD_DOWN ) {
		// *** Climb up
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
		if ( playerX > 0 ) {
			playerX -= PLAYER_MOVE_VEL;
		}
	} else if ( pad & PAD_RIGHT ) {
		playerDir = PAD_RIGHT;
		if ( playerX < 240 ) {
			playerX += PLAYER_MOVE_VEL;
		}
	}

	bgHorizCollideCheck(&playerX, &playerY, pad);
}


/*

void  updatePlayerAttack(void) {
	if ( ( pad & PAD_B ) && ( potionTossTimer == 0 ) ) {
		// spawn a potion
		if ( !potionIsActive ) {
			potionIsActive = 1;
			potionMoveCounter = 0;
			potionY = playerY + 6;
			if ( playerDir == PAD_RIGHT ) {
				potionX = playerX + 12;
			} else {
				potionX = playerX - 4;
			}
			potionDirection = playerDir;
			potionVerticalVel = POTION_INIT_VERTICAL_VEL;
			++potionTossTimer;
		}
	}
}

*/

const u8 tileUpdateListInit[3 + 3 + 3 + 3 + 1] = {
	MSB(NTADR_A(0, 0)), LSB(NTADR_A(0, 0)), 0,//non-sequental updates
	MSB(NTADR_A(1, 0)), LSB(NTADR_A(1, 0)), 0,
	MSB(NTADR_A(0, 1)), LSB(NTADR_A(0, 1)), 0,
	MSB(NTADR_A(1, 1)), LSB(NTADR_A(1, 1)), 0,
	NT_UPD_EOF
};

static u8 tileUpdateList[3 + 3 + 3 + 1];


void updatePlayerGlue(void) {
	u8 glueX, glueY;
	if ( pad & PAD_B ) {

		glueY = ((playerY + 8) & 0xf0) >> 4;
		if ( playerDir == PAD_RIGHT ) {
			glueX = (((playerX + 8) & 0xf0) >> 4) + 1;
		} else {
			glueX = (((playerX + 8) & 0xf0) >> 4 ) - 1;
		}

		getCollisionIndex(glueX, glueY);

		if ( ( collisionMap[collisionIndex] == TILE_NOCOLLIDE ) || ( collisionMap[collisionIndex] == TILE_ENEMYCOLLIDE ) ) {
			// Create glue sprite
			// TODO: Add animation


			gluePointer = &(glueData[0]);
			gluePointer->x = glueX << 4;
			gluePointer->y = (glueY << 4) - 2;
			gluePointer->active = 1;
			gluePointer->timeLeft = GLUE_INIT_LIFESPAN;

			sfx_play(SFX_GLUEDROP, CHANNEL_SQUARE1);				

		}

		/*
		glueTileX = glueX << 1;
		glueTileY = glueY << 1;

		tileUpdateList[0] = MSB(NTADR_A(glueTileX, glueTileY));
		tileUpdateList[1] = LSB(NTADR_A(glueTileX, glueTileY));

		tileUpdateList[3] = MSB(NTADR_A(glueTileX + 1, glueTileY));
		tileUpdateList[4] = LSB(NTADR_A(glueTileX + 1, glueTileY));

		tileUpdateList[6] = MSB(NTADR_A(glueTileX, glueTileY + 1));
		tileUpdateList[7] = LSB(NTADR_A(glueTileX, glueTileY + 1));

		tileUpdateList[9] = MSB(NTADR_A(glueTileX + 1, glueTileY + 1));
		tileUpdateList[10] = LSB(NTADR_A(glueTileX + 1, glueTileY + 1));

		set_vram_update(tileUpdateList);
		*/

		
	}
}

/*********** Main ***********/

void main(void)
{

	// TODO next:

	// - animate player/monster walks properly
	// - build a proper level
	// - add dying/lives/level reset
	// - study enemy behavior in games


	memcpy(palSprites, paldat, 16);
	memcpy(palBG, paldat + 16, 4);

	memcpy(tileUpdateList, tileUpdateListInit, sizeof(tileUpdateListInit));
	tileUpdateList[2] = glueTileData[0];
	tileUpdateList[5] = glueTileData[1];
	tileUpdateList[8] = glueTileData[2];
	tileUpdateList[11] = glueTileData[3];


	//unrleCollision();

	pal_spr(palSprites);
	pal_bg(palBG);

	vram_adr(NAMETABLE_A); //unpack nametable into VRAM
	vram_unrle(newmap);	

	drawScoreboard();

	ppu_on_all(); //enable rendering

	//set initial coords
	playerDir = PAD_LEFT;

	//init other vars
	touch = 0; // collision flag
	frameCount = 0; // frame counter

	playerFrame = 0;
	
	setSpriteFrame(playerSpriteData, playerFrames[playerFrame]);
	setupMap();

	playerX = playerStartX;
	playerY = playerStartY;

	// now the main loop

	while ( 1 )
	{
		ppu_wait_frame(); // wait for next TV frame
	
		//process player
		
		oamSpriteIndex = 0;
		i = 0;

		sprPriorityToggle = frameCount & 0xFE;

		// update player movement
		pad = pad_poll(i);

		updateScoreboard();

		updatePlayerSprite();
		updateEnemySprites();
		updateGlueSprites();
		spriteCount();

		updateEnemyMovement();
		updatePlayerHorizontalMovement();
		updatePlayerVerticalMovement();
		updatePlayerGlue();


		enemyColliding = 0;
		four_Sides(playerX, playerY);
		enemyCollideCheck();
		playerEnemyColliding = enemyColliding;
		//gluePlayerCollideCheck();
		//glueEnemyCollideCheck();

		
		if ( playerEnemyColliding ) {
			setSpritePalette(playerSpriteData, 0x0);
		} else {
			setSpritePalette(playerSpriteData, 0x3);
		}

		oam_hide_rest(oamSpriteIndex);

		++frameCount;
	}
}