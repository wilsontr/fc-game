

#include <stdint.h>
#include "neslib.h"
#include "map1.h"
#include "map1_coll.h"


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

#define MAX_JUMP_HEIGHT			 100

#define COLLISION_MAP_SIZE 	     960

#define POTION_HORIZ_VELOCITY	 2
#define POTION_INIT_VERTICAL_VEL 2

#define PLAYER_INIT_JUMP_VEL 	 3


#define PLAYER_STATE_NORMAL		 0
#define PLAYER_STATE_DEAD		 1
#define PLAYER_STATE_CLIMBING	 2
#define PLAYER_STATE_JUMPING	 3
#define PLAYER_STATE_FALLING	 4
#define PLAYER_JUMP_VEL   		 2
#define PLAYER_MOVE_VEL			 1

#define PLAYER_FALL_SPEED		 3

#define PLAYER_FRAME_CLIMBING	 2

#define ENEMY_STATE_NORMAL 		 0
#define ENEMY_STATE_MUSHROOM	 1
#define ENEMY_STATE_DEAD		 3

#define POTION_TOSS_WAIT_TIME	60


#pragma bss-name (push, "ZEROPAGE")
#pragma data-name (push, "ZEROPAGE")

u8 oam_off;
static u8 playerX;
static u8 playerY;
static u8 enemyIndex = 0;
static u8 frameCount;
static u8 enemyColliding = 0;
static u8 i;
static u16 collisionIndex;
static u8 leftSide, rightSide, topSide, bottomSide;
static u8 enemyVertCollide;
static u8 potionX;
static u8 potionY;
static u8 potionTossTimer;

#pragma data-name(pop)
#pragma bss-name (pop)

static uint8_t * collisionMap;


/* Commonly-used test variables */



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

static u8 jumpCollideCheckTile;
static u8 collideBottom;

static u8 collisionLeft, collisionRight;


/* Player */
static u8 playerDir;
static u8 playerEnemyColliding;
static u8 playerFrame = 0;
static u8 playerJumping = 0;
static u8 playerJumpCounter = 0;
static u8 playerJumpDirection = 0;
static u8 initPlayerJumpY;
static u8 playerState = PLAYER_STATE_NORMAL;
static signed char playerVertVel = 0;
static u8 jumpButtonReset = 1;


const u8 playerFrames[3][4] = {
	{ 0x08, 0x09, 0x18, 0x19 },
	{ 0x28, 0x29, 0x38, 0x39 },
	{ 0x68, 0x69, 0x78, 0x79 },
};

u8 playerSpriteData[17] = {
	0, 0, 0x08, 0x3,
	8, 0, 0x09, 0x3,
	0, 8, 0x18, 0x3,
	8, 8, 0x19, 0x3,
	128
};

u8 * nametableUpdateList;

/* Potion */

static u8 potionIsActive = 0;
static u8 potionDirection;
static signed char potionVerticalVel = 0;
static u8 potionMoveCounter = 0;

u8 potionSpriteData[5] = {
	0, 0, 0x2A, 0x0,
	128
};

/* Mushroom */

const u8 mushroomSpriteDataTemplate[17] = {
	0, 0, 0x2B, 0x2,
	8, 0, 0x2C, 0x2,
	0, 8, 0x3B, 0x2,
	8, 8, 0x3C, 0x2,
	128
};

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

enemy enemyData[20];

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
static u8 * currentEnemySprite;

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
	u16 index = 0;
	enemy newEnemy = { 0, 0, 0, PAD_LEFT };
	potionTossTimer = 0;

	potionX = -8;
	potionY = -8;

	collisionMap = (u8 *) map1_coll;

	enemyIndex = 0;
	
	for ( index; index <= COLLISION_MAP_SIZE; ++index ) {
		collByte = collisionMap[index];

		if ( collByte == TILE_PLAYERSTART ) {
			playerX = mapX << 3;
			playerY = (mapY << 3) - 1;
		}
		
		if ( ( collByte == TILE_ENEMY1START_RIGHT ) || ( collByte == TILE_ENEMY1START_LEFT ) ) {
			enemyData[enemyIndex] = newEnemy;
			enemyData[enemyIndex].x = mapX << 3;
			enemyData[enemyIndex].y = (mapY << 3) - 1;
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
		if ( mapX >= 32 ) {
			mapX = 0;
			++mapY;
		}
	}

	numEnemies = enemyIndex;

}


/*********** Sprite Management ***********/

static u8 sSpriteIndex, sFrameIndex;
static u8 * sSpritePtr, sFramePtr;

void __fastcall__ setSpriteFrame(u8 *sprite, const u8 *frame) {
	sSpriteIndex = 2;
	sFrameIndex = 0;

	*(sprite + 2) = *(frame);
	*(sprite + 6) = *(frame + 1);
	*(sprite + 10) = *(frame + 2);
	*(sprite + 14) = *(frame + 3);

	/*
	sprite[2] = frame[0];
	sprite[6] = frame[1];
	sprite[10] = frame[2];
	sprite[14] = frame[3];
	*/
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
		//currentEnemySprite = &(enemySpriteData[spriteFlickerIndex][0]);
		
		if ( (*currentEnemy).state == ENEMY_STATE_NORMAL ) {
			// animate
			if ( ( frameCount & 0x0F ) == 0x0F ) {
				(*currentEnemy).frame ^= 1;
				setSpriteFrame(enemySpriteData[spriteFlickerIndex], enemyFrames[(*currentEnemy).frame]);
				//setSpriteFrame(currentEnemySprite, enemyFrames[(*currentEnemy).frame]);
			}

			if ( (*currentEnemy).collidingWithPotion ) {
				setSpritePalette(enemySpriteData[spriteFlickerIndex], 0x0);
			} else {
				setSpritePalette(enemySpriteData[spriteFlickerIndex], 0x3);
			}			

			//setSpritePriority(enemySpriteData[i], sprPriorityToggle);
			//sprPriorityToggle ^= 1;
			oamSpriteIndex = oam_meta_spr((*currentEnemy).x, (*currentEnemy).y, oamSpriteIndex, enemySpriteData[spriteFlickerIndex]);				
		} else if ( (*currentEnemy).state == ENEMY_STATE_MUSHROOM ) {
			//setSpritePriority(enemySpriteData[i], sprPriorityToggle);
			//sprPriorityToggle ^= 1;
			oamSpriteIndex = oam_meta_spr((*currentEnemy).x, (*currentEnemy).y, oamSpriteIndex, mushroomSpriteDataTemplate);							
		}
		
	}	
}

void updatePlayerSprite(void) {

	if ( ( playerState == PLAYER_STATE_NORMAL ) || ( playerState == PLAYER_STATE_JUMPING ) ) {
		if ( pad & PAD_RIGHT ) {
			flipSprite(playerSpriteData, 1);
		} else if ( pad & PAD_LEFT ) {
			flipSprite(playerSpriteData, 0);
		}		

		setSpriteFrame(playerSpriteData, playerFrames[playerFrame]);
		// animate player sprite
		if ( ( frameCount & 0x0F ) == 0x0F ) {
			playerFrame ^= 1;
			setSpriteFrame(playerSpriteData, playerFrames[playerFrame]);
		} 	
	} else if ( playerState == PLAYER_STATE_CLIMBING ) {
		setSpriteFrame(playerSpriteData, playerFrames[PLAYER_FRAME_CLIMBING]);

		if ( ( frameCount & 0x0F ) == 0x0F ) {
			playerFrame ^= 1;
			flipSprite(playerSpriteData, playerFrame);
		}
	}
	
	// update player sprite
	oamSpriteIndex = oam_meta_spr(playerX, playerY, oamSpriteIndex, playerSpriteData);	
}

void updatePotionSprite(void) {
	oamSpriteIndex = oam_meta_spr(potionX, potionY, oamSpriteIndex, potionSpriteData);	
}

void drawScoreboard(void) {
	// TODO: flesh this out
	// TODO: set correct pal color (any bg pal entry with white as color #1)
	// CONSIDER: using separate nametable with sprite zero hit trick

	//pal_col(1,0x30); //set while color
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
	collisionIndex = ( screenX >> 3 ) + ( ( screenY & 0xF8 ) << 2);
}


u8 __fastcall__ smallCollideCheckVertical(u8 originX, u8 originY, u8 direction) {

	leftSide = originX;
	rightSide = originX + 8;
	topSide = originY;
	bottomSide = originY + 8;

	collisionIndex = 0;

	if ( direction & PAD_UP ) {
		collisionIndex = ( rightSide >> 3 ) + ( ( topSide & 0xF8 ) << 2);
		if ( collisionMap[collisionIndex] != TILE_ALLCOLLIDE ) {
			collisionIndex = ( leftSide >> 3 ) + ( ( topSide & 0xF8 ) << 2);
		}
	} else if ( direction & PAD_DOWN ) {
		collisionIndex = ( rightSide >> 3 ) + ( ( bottomSide & 0xF8 ) << 2);
		if ( collisionMap[collisionIndex] != TILE_ALLCOLLIDE ) {
			//getCollisionIndex(leftSide, bottomSide);
			collisionIndex = ( leftSide >> 3 ) + ( ( bottomSide & 0xF8 ) << 2);
		}
	}

	if ( collisionIndex ) {
		return collisionMap[collisionIndex];	
	} else {
		return 0;
	}
	
}


void __fastcall__ collideCheckVertical(u8 originX, u8 originY, u8 direction) {

	leftSide = originX + 2;
	rightSide = originX + 14;
	topSide = originY + 1;
	bottomSide = originY + 17;

	collisionIndex = 0;

	if ( direction & PAD_UP ) {
		collisionIndex = ( rightSide >> 3 ) + ( ( topSide & 0xF8 ) << 2);
		if ( collisionMap[collisionIndex] != TILE_ALLCOLLIDE )  {
			collisionIndex = ( leftSide >> 3 ) + ( ( topSide & 0xF8 ) << 2);
		}
	} else if ( direction & PAD_DOWN ) {
		collisionIndex = ( rightSide >> 3 ) + ( ( bottomSide & 0xF8 ) << 2);
		if ( collisionMap[collisionIndex] != TILE_ALLCOLLIDE ) {
			collisionIndex = ( leftSide >> 3 ) + ( ( bottomSide & 0xF8 ) << 2);
		}
	}

	verticalCollideCheck = collisionMap[collisionIndex];	
}

void __fastcall__ collideCheckHorizontal(u8 originX, u8 originY, u8 direction) {

	leftSide = originX;
	rightSide = originX + 16;
	topSide = originY + 4;
	bottomSide = originY + 12;

	if ( direction & PAD_LEFT ) {
		collisionIndex = ( leftSide >> 3 ) + ( ( topSide & 0xF8 ) << 2);
		if ( collisionMap[collisionIndex] == TILE_NOCOLLIDE ) {
			collisionIndex = ( leftSide >> 3 ) + ( ( bottomSide & 0xF8 ) << 2);
		}
	} else if ( direction & PAD_RIGHT ) {
		getCollisionIndex(rightSide, topSide);
		if ( collisionMap[collisionIndex] == TILE_NOCOLLIDE ) {
			collisionIndex = ( rightSide >> 3 ) + ( ( bottomSide & 0xF8 ) << 2);
		}
	}

	horizontalCollideCheck = collisionMap[collisionIndex];
}

void checkPlayerLadderCollision(void) {

	leftSide = playerX + 3;
	rightSide = playerX + 11;
	topSide = playerY + 9;
	bottomSide = playerY + 16;

	collisionLeft = collisionMap[( leftSide >> 3 ) + ( ( bottomSide & 0xF8 ) << 2)];
	collisionRight = collisionMap[( rightSide >> 3 ) + ( ( bottomSide & 0xF8 ) << 2)];

	if (
		( ( collisionLeft == TILE_LADDER )  || ( collisionLeft == TILE_LADDER_TOP ) ) &&
		( ( collisionRight == TILE_LADDER ) || ( collisionRight == TILE_LADDER_TOP ) ) 
		)
	{
		playerState = PLAYER_STATE_CLIMBING;
		if ( collisionLeft == TILE_LADDER ) {
			playerX = ( playerX + 3 ) & 0xf8;	
		} else if ( collisionRight == TILE_LADDER ) {
			playerX = ( playerX - 3 ) & 0xf8;
		}
	} else {
		playerState = PLAYER_STATE_NORMAL;
	}
}

void __fastcall__ bgVertCollideCheck(u8 *x, u8 *y, u8 dir) {
	collideCheckVertical(*x, *y, dir);
	if ( verticalCollideCheck ) {
		if ( dir & PAD_UP ) {
			*y = (*y & 0xf8) + 7;
		} else {
			*y = (*y & 0xf8) - 1;
		}
	}
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


void enemyCollideCheck(void) {

	enemyIndex = 0;
	enemyColliding = 0;
	enemyCollidedIndex = 0;

	while ( !enemyColliding && ( enemyIndex < numEnemies ) ) {
		currentEnemy = &(enemyData[enemyIndex]);
		enemyTop = (*currentEnemy).y + 2;
		enemyBottom = (*currentEnemy).y + 14;
		enemyLeft = (*currentEnemy).x + 2;
		enemyRight = (*currentEnemy).x + 14;

		if ( !( rightSide  <  enemyLeft  || 
				leftSide   >= enemyRight || 
				bottomSide <  enemyTop   || 
				topSide    >= enemyBottom ) ) {

			switch ( (*currentEnemy).state ) {
				case ENEMY_STATE_NORMAL: 
					enemyColliding = 1;
					enemyCollidedIndex = enemyIndex;
					break;
				case ENEMY_STATE_MUSHROOM:
					(*currentEnemy).state = ENEMY_STATE_DEAD;
					break;
				case ENEMY_STATE_DEAD:
					break;
			}
		}		
		++enemyIndex;
	}
}

void potionEnemyCollideCheck(void) {
	enemyColliding = 0;

	if ( potionIsActive ) {
		four_SidesSmall(potionX, potionY);
		enemyCollideCheck();
		if ( enemyColliding ) {
			killPotion();
			enemyData[enemyCollidedIndex].state = ENEMY_STATE_MUSHROOM;
		}
	}

	enemyColliding = 0;
}




/*********** State Management ***********/

void updateEnemyMovement(void) {

	for ( i = 0; i < numEnemies; i++ ) {
		currentEnemy = &(enemyData[i]);
		if ( (*currentEnemy).state == ENEMY_STATE_MUSHROOM ) {
			continue;
		}
		collideCheckVertical((*currentEnemy).x, (*currentEnemy).y + 1, PAD_DOWN);
		if ( verticalCollideCheck != TILE_ALLCOLLIDE ) {
			(*currentEnemy).y += 1;
		} else {
			if ( (*currentEnemy).direction == PAD_RIGHT ) {
				(*currentEnemy).x += 1;
				 collideCheckHorizontal((*currentEnemy).x, (*currentEnemy).y, PAD_RIGHT);
				if ( ( horizontalCollideCheck == TILE_ALLCOLLIDE ) || ( horizontalCollideCheck == TILE_ENEMYCOLLIDE ) ) {
					flipSprite(enemySpriteData[i], 0);
					(*currentEnemy).direction = PAD_LEFT;
				}
			} else {
				(*currentEnemy).x -= 1;
				collideCheckHorizontal((*currentEnemy).x, (*currentEnemy).y, PAD_LEFT);
				if ( ( horizontalCollideCheck == TILE_ALLCOLLIDE ) || ( horizontalCollideCheck == TILE_ENEMYCOLLIDE ) ) {
					flipSprite(enemySpriteData[i], 1);
					(*currentEnemy).direction = PAD_RIGHT;
				}
			}			
		}
	}

}



void updatePlayerJumpFall(void) {

	collideBottom = 0;

	if ( !(pad & PAD_A ) ) {
		jumpButtonReset = 1;
	}

	// stop ascent when player releases A button
	if ( !( pad & PAD_A ) && playerJumping && ( playerVertVel > 0 ) ) {
		playerVertVel = 0;
	}


	if ( playerState == PLAYER_STATE_JUMPING ) {
		// moving up, jumping
		playerY -= playerVertVel;
		if ( playerVertVel > 0 ) {
			collideCheckVertical(playerX, playerY, PAD_UP);
			// check collision above
			if ( verticalCollideCheck == TILE_ALLCOLLIDE ) { 
				playerY = (playerY & 0xF8) + 8;
			}			
		} else {
			collideCheckVertical(playerX, playerY + 2, PAD_DOWN);
			// check collision below
			if ( ( verticalCollideCheck == TILE_ALLCOLLIDE ) || ( verticalCollideCheck == TILE_LADDER_TOP ) ) { 
				playerY = (playerY & 0xF8) + 7;
				collideBottom = 1;
				playerState = PLAYER_STATE_NORMAL;							
			}
		}
		
	} else {
		// check collision below
		collideCheckVertical(playerX, playerY + 2, PAD_DOWN);
		if ( ( verticalCollideCheck == TILE_ALLCOLLIDE ) || ( verticalCollideCheck == TILE_LADDER_TOP ) ) { 
			// sitting on the ground
			playerY = (playerY & 0xF8) + 7;
			collideBottom = 1;
			playerState = PLAYER_STATE_NORMAL;			
		} else {
			// falling
			playerY += PLAYER_FALL_SPEED;
			collideBottom = 0;
			playerState = PLAYER_STATE_FALLING;
		}			
	}

	if ( collideBottom ) {
		if ( pad & PAD_UP )  {
			checkPlayerLadderCollision();
		}

		if ( ( pad & PAD_DOWN ) && ( verticalCollideCheck == TILE_LADDER_TOP ) ) {
			playerY++;
			checkPlayerLadderCollision();
		}

		if ( ( jumpButtonReset ) && ( pad & PAD_A ) ) {
			playerVertVel = PLAYER_INIT_JUMP_VEL;
			playerState = PLAYER_STATE_JUMPING;
			playerJumpCounter = 0;
			jumpButtonReset = 0;
		} 	
	}

	// update velocity
	// acceleration toward ground
	if ( ( playerVertVel > -3 ) && ( playerJumpCounter == 4 ) ) {
		playerVertVel -= 1; 
		playerJumpCounter = 0;
	}

	++playerJumpCounter;
}

/*
void simpleUpdatePlayerJumpFall(void) {
	if ( !(pad & PAD_A ) ) {
		jumpButtonReset = 1;
	}

	if ( playerState == PLAYER_STATE_JUMPING ) {

		
		if ( playerJumpCounter <= 12 ) {
			playerY -= playerVertVel;
		} else if ( playerJumpCounter <= 24 ) {
			playerY += playerVertVel;
		} else {
			playerJumpCounter = 0;
			playerState = PLAYER_STATE_NORMAL;
		}


		switch ( playerJumpDirection ) {
			case PAD_LEFT: playerX -= PLAYER_MOVE_VEL; break;
			case PAD_RIGHT: playerX += PLAYER_MOVE_VEL; break;
		}

		++playerJumpCounter;

		//if ( playerY >= initPlayerJumpY ) {
		// playerState = PLAYER_STATE_NORMAL;
		// 		}

	} else {
		collideCheckVertical(playerX, playerY + 2, PAD_DOWN);
		if ( ( verticalCollideCheck == TILE_ALLCOLLIDE ) || ( verticalCollideCheck == TILE_LADDER_TOP ) ) { 
			playerY = (playerY & 0xF8) + 7;
			collideBottom = 1;
			playerState = PLAYER_STATE_NORMAL;			

			if ( pad & PAD_UP )  {
				checkPlayerLadderCollision();
			}

			if ( ( pad & PAD_DOWN ) && ( verticalCollideCheck == TILE_LADDER_TOP ) ) {
				playerY++;
				checkPlayerLadderCollision();
			}

			
			if ( ( jumpButtonReset ) && ( pad & PAD_A ) ) {
				if ( pad & PAD_LEFT ) {
					playerJumpDirection = PAD_LEFT;
				} else if ( pad & PAD_RIGHT ) {
					playerJumpDirection = PAD_RIGHT;
				} else {
					playerJumpDirection = 0;
				}
				//playerVertVel = PLAYER_INIT_JUMP_VEL;
				playerVertVel = 3;
				playerState = PLAYER_STATE_JUMPING;
				playerJumpCounter = 0;
				jumpButtonReset = 0;
				initPlayerJumpY = 0;
			}

		} else {
			playerY += PLAYER_FALL_SPEED;
			playerState = PLAYER_STATE_FALLING;
			collideBottom = 0;
		}	
	}
}
*/
void updatePlayerClimbing(void) {
	u8 jumpCollideCheckTile;

	checkPlayerLadderCollision();
	
	
	if ( pad & PAD_UP ) { 
		--playerY;
	} else if ( pad & PAD_DOWN ) {
		++playerY;
		collideCheckVertical(playerX, playerY + 4, PAD_DOWN);
		if ( ( verticalCollideCheck == TILE_ALLCOLLIDE ) || ( verticalCollideCheck == TILE_LADDER_TOP ) ) { 	
			playerState = PLAYER_STATE_NORMAL;
		}	
	}
}

void updatePlayerVerticalMovement(void) {

	if ( playerState == PLAYER_STATE_CLIMBING ) {
		updatePlayerClimbing(); 
	} else {
		//simpleUpdatePlayerJumpFall();
		updatePlayerJumpFall();
	}
}

void playerMoveHorizontal(void) {	
	if ( ( playerState != PLAYER_STATE_NORMAL ) && ( playerState != PLAYER_STATE_JUMPING ) ) {
		return;
	}

	if ( pad & PAD_DOWN ) {
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

void __fastcall__ updatePotionMovement(void) {
	u8 potionCollided = 0;
	if ( potionTossTimer > 0 ) {
		++potionTossTimer;
		if ( potionTossTimer >= POTION_TOSS_WAIT_TIME ) {
			potionTossTimer = 0;
		}
	}
	if ( potionIsActive ) {
		/* horizontal movement */
		if ( potionDirection == PAD_LEFT ) {
			potionX -= POTION_HORIZ_VELOCITY;
		} else {
			potionX += POTION_HORIZ_VELOCITY;
		}

		if ( ( potionX <= 8 ) || ( potionX >= 248 ) ) {
			potionCollided = 1;
		}

		/* vertical movement */

		if ( potionVerticalVel > 0 ) {
			// moving up in arc 
			if ( smallCollideCheckVertical(potionX, potionY + 8, PAD_UP) == TILE_ALLCOLLIDE ) { 
				/* collided with ceiling */
				potionCollided = 1;				
			} else {
				potionY -= potionVerticalVel;
			}
			
		} else {
			// falling
			if ( smallCollideCheckVertical(potionX, potionY, PAD_DOWN) == TILE_ALLCOLLIDE ) { 
				/* collided with ground */
				potionCollided = 1;				
			} else {
				potionY -= potionVerticalVel;
			}		
		}

		if ( potionY >= 240 ) {
			potionCollided = 1;
		}			

		// acceleration toward ground
		if ( ( potionVerticalVel >= -3 ) && ( potionMoveCounter == 3 ) ) {
			potionVerticalVel -= 1; 
			potionMoveCounter = 0;
		}		

		++potionMoveCounter;

		if ( potionCollided ) {
			potionIsActive = 0;
			potionX = -8;
			potionY = -8;
		}
	}
}

void simpleUpdatePotionMovement(void) {
	u8 potionCollided = 0;
	if ( potionIsActive ) {
		/* horizontal movement */
		if ( potionDirection == PAD_LEFT ) {
			potionX -= POTION_HORIZ_VELOCITY;
		} else {
			potionX += POTION_HORIZ_VELOCITY;
		}

		if ( ( potionX <= 8 ) || ( potionX >= 248 ) ) {
			potionCollided = 1;
		}

		++potionMoveCounter;

		if ( potionCollided ) {
			killPotion();
		}		

	}
}

void killPotion(void) {
	potionIsActive = 0;
	potionX = -8;
	potionY = -8;
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

	//unrleCollision();

	pal_spr(palSprites);
	pal_bg(palBG);

	vram_adr(NAMETABLE_A); //unpack nametable into VRAM
	vram_unrle(map1);	

	drawScoreboard();

	ppu_on_all(); //enable rendering

	//set initial coords
	playerX = 0;
	playerY = 0;
	playerDir = PAD_LEFT;

	//init other vars
	touch = 0; // collision flag
	frameCount = 0; // frame counter

	playerFrame = 0;
	
	setSpriteFrame(playerSpriteData, playerFrames[playerFrame]);
	setupMap();

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
		updatePotionSprite();
		spriteCount();

		updateEnemyMovement();
		playerMoveHorizontal();
		updatePlayerVerticalMovement();
		updatePlayerAttack();
		updatePotionMovement();

		enemyColliding = 0;
		four_Sides(playerX, playerY);
		enemyCollideCheck();
		playerEnemyColliding = enemyColliding;
		potionEnemyCollideCheck();

		
		if ( playerEnemyColliding ) {
			setSpritePalette(playerSpriteData, 0x0);
		} else {
			setSpritePalette(playerSpriteData, 0x3);
		}

		oam_hide_rest(oamSpriteIndex);

		++frameCount;
	}
}