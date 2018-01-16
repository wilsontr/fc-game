

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

#define MAX_JUMP_HEIGHT			 100

#define COLLISION_MAP_SIZE 	     960

#define POTION_HORIZ_VELOCITY	 2
#define POTION_INIT_VERTICAL_VEL 2

#define PLAYER_INIT_JUMP_VEL 	 4


#pragma bss-name (push, "ZEROPAGE")
#pragma data-name (push, "ZEROPAGE")

u8 oam_off;

#pragma data-name(pop)
#pragma bss-name (pop)


/* Commonly-used test variables */

static u8 leftSide, rightSide, topSide, bottomSide;
static u16 testCorner;

/* Bookkeeping and state */

extern const u8 paldat[];

static u8 i;
static u8 pad, oamSpriteIndex;
static u8 touch;
static u8 frameCount;
static u8 spriteFlickerIndex = 0;
static u8 sprPriorityToggle = 0;

static u8 palSprites[4];
static u8 palBG[4];


/* Player */
static u8 playerX;
static u8 playerY;
static u8 playerDir;
static u8 jumpIteration;
static u8 playerEnemyColliding;
static u8 playerFrame = 0;
static u8 playerFalling = 0;
static u8 playerJumping = 0;
static u8 playerJumpHeight = 0;
static u8 playerJumpCounter = 0;
static signed char playerVertVel = 0;
static char playerVertAccel = 0;
static u8 jumpButtonPressed = 0;


const u8 playerFrames[2][4] = {
	{ 0x08, 0x09, 0x18, 0x19 },
	{ 0x28, 0x29, 0x38, 0x39 }
};

u8 playerSpriteData[17] = {
	0, 0, 0x08, 0x3,
	8, 0, 0x09, 0x3,
	0, 8, 0x18, 0x3,
	8, 8, 0x19, 0x3,
	128
};


/* Potion */

static u8 potionX = -8;
static u8 potionY = -8;
static u8 potionIsActive = 0;
static u8 potionDirection;
static signed char potionVerticalVel = 0;
static u8 potionMoveCounter = 0;

u8 potionSpriteData[5] = {
	0, 0, 0x2A, 0x0,
	128
};

/* Enemies */

static u8 numEnemies;
static u8 enemySpriteCount = 0;

struct enemyStruct {
	u8 x;
	u8 y; 
	u8 frame;
	u8 direction;
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


/*********** Sprite Management ***********/

void __fastcall__ setSpriteFrame(u8 *sprite, const u8 *frame) {
	sprite[2] = frame[0];
	sprite[6] = frame[1];
	sprite[10] = frame[2];
	sprite[14] = frame[3];
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


u8 __fastcall__ spriteCount(void) {
	++enemySpriteCount;
	if ( enemySpriteCount >= numEnemies ) {
		enemySpriteCount = 0;
	}
	return enemySpriteCount;
}


void updateEnemySprites(void) {
	// update enemy sprites
	for ( i = 0; i < numEnemies; ++i ) {

		spriteFlickerIndex = spriteCount();
		
		// animate
		if ( ( frameCount & 0x0F ) == 0x0F ) {
			enemyData[spriteFlickerIndex].frame ^= 1;
			setSpriteFrame(enemySpriteData[spriteFlickerIndex], enemyFrames[enemyData[spriteFlickerIndex].frame]);
		}
		
		setSpritePriority(enemySpriteData[i], sprPriorityToggle);
		sprPriorityToggle ^= 1;
		oamSpriteIndex = oam_meta_spr(enemyData[spriteFlickerIndex].x, enemyData[spriteFlickerIndex].y, oamSpriteIndex, enemySpriteData[spriteFlickerIndex]);	
	}	
}

void updatePlayerSprite(void) {
	if ( pad & PAD_RIGHT ) {
		flipSprite(playerSpriteData, 1);
	} else if ( pad & PAD_LEFT ) {
		flipSprite(playerSpriteData, 0);
	}		

	// animate player sprite
	if ( ( frameCount & 0x0F ) == 0x0F ) {
		playerFrame ^= 1;
		setSpriteFrame(playerSpriteData, playerFrames[playerFrame]);
	} 

	// update player sprite
	oamSpriteIndex = oam_meta_spr(playerX, playerY, oamSpriteIndex, playerSpriteData);	
}

void updatePotionSprite(void) {
	//if ( potionIsActive ) {
		oamSpriteIndex = oam_meta_spr(potionX, potionY, oamSpriteIndex, potionSpriteData);	
	//}
}

/*********** Collision Checking ***********/

void __fastcall__ four_Sides(u8 originX, u8 originY) {
	if (originX < (255 - 1)){	// find the left side
		leftSide = originX + 1;
	}
	else {
		leftSide = 255;
	}
	if (originX < (255 - 15)){	// find the right side
		rightSide = originX + 15;
	}
	else {
		rightSide = 255;
	}
	topSide = originY + 1;	// our top is the same as the master Y
	
	if (originY < (255)){ // find the bottom side
		bottomSide = originY + 16;
	}
	else {
		bottomSide = 255;
	}
}

u16 __fastcall__ getCollisionIndex(u8 screenX, u8 screenY) {
	return ( screenX >> 3 ) + ( ( screenY >> 3 ) << 5);
}


u8 __fastcall__ smallCollideCheckVertical(u8 originX, u8 originY, u8 direction) {

	leftSide = originX ;
	rightSide = originX + 8;
	topSide = originY;
	bottomSide = originY + 8;

	if ( ( (direction & PAD_UP) != 0) ) {
		testCorner = getCollisionIndex(rightSide, topSide);
		if ( collisionMap[testCorner] == 0 ) {
			testCorner = getCollisionIndex(leftSide, topSide);
		}
	} else if ( (direction & PAD_DOWN) != 0 ) {
		testCorner = getCollisionIndex(rightSide, bottomSide);
		if ( collisionMap[testCorner] == 0 ) {
			testCorner = getCollisionIndex(leftSide, bottomSide);
		}
	}

	return collisionMap[testCorner];
}


u8 __fastcall__ collideCheckVertical(u8 originX, u8 originY, u8 direction) {

	leftSide = originX + 1;
	rightSide = originX + 15;
	topSide = originY + 1;
	bottomSide = originY + 17;

	if ( ( (direction & PAD_UP) != 0) ) {
		testCorner = getCollisionIndex(rightSide, topSide);
		if ( collisionMap[testCorner] == 0 ) {
			testCorner = getCollisionIndex(leftSide, topSide);
		}
	} else if ( (direction & PAD_DOWN) != 0 ) {
		testCorner = getCollisionIndex(rightSide, bottomSide);
		if ( collisionMap[testCorner] == 0 ) {
			testCorner = getCollisionIndex(leftSide, bottomSide);
		}
	}

	return collisionMap[testCorner];
}

u8 __fastcall__ collideCheckHorizontal(u8 originX, u8 originY, u8 direction) {

	leftSide = originX + 1;
	rightSide = originX + 15;
	topSide = originY + 1;
	bottomSide = originY + 16;

	if ( ( (direction & PAD_LEFT) != 0 ) ) {
		testCorner = getCollisionIndex(leftSide, topSide);
		if ( collisionMap[testCorner] == 0 ) {
			testCorner = getCollisionIndex(leftSide, bottomSide);
		}
	} else if ( (direction & PAD_RIGHT) != 0 ) {
		testCorner = getCollisionIndex(rightSide, topSide);
		if ( collisionMap[testCorner] == 0 ) {
			testCorner = getCollisionIndex(rightSide, bottomSide);
		}
	}

	return collisionMap[testCorner];
}


u8 __fastcall__ bgVertCollideCheck(u8 *x, u8 *y, u8 dir) {
	u8 colliding = collideCheckVertical(*x, *y, dir);
	if ( colliding == 1 ) {
		if ( dir & PAD_UP ) {
			*y = (*y & 0xf8) + 7;
		//} else if ( dir & PAD_DOWN ) {
		} else {
			*y = (*y & 0xf8) - 1;
		}
	}
	return colliding;
}

void __fastcall__ bgHorizCollideCheck(u8 *x, u8 *y, u8 dir) {
	u8 colliding = collideCheckHorizontal(*x, *y, dir);
	if ( colliding == 1 ) {
		if ( dir & PAD_LEFT ) {
			*x = (*x & 0xf8) + 8;
		} else if ( dir & PAD_RIGHT ) {
			*x = (*x & 0xf8);
		}		
	}
}


void playerEnemyCollideCheck(void) {

	static u8 enemyTop;
	static u8 enemyBottom;
	static u8 enemyLeft;
	static u8 enemyRight;
	static u8 j;

	playerEnemyColliding = 0;

	for ( j = 0; j < numEnemies; ++j ) {
		enemyTop = enemyData[j].y + 2;
		enemyBottom = enemyData[j].y + 14;
		enemyLeft = enemyData[j].x + 2;
		enemyRight = enemyData[j].x + 14;

		if ( !( rightSide < enemyLeft  || 
				leftSide >= enemyRight || 
				bottomSide <  enemyTop || 
				topSide    >= enemyBottom ) ) {
			playerEnemyColliding = 1;
		}		
	}
}

void potionEnemyCollideCheck(void) {

}

/*********** State Setup ***********/

void setupMap(void) {
	u8 collByte, k;
	u8 enemyIndex = 0;
	u8 mapX = 0;
	u8 mapY = 0;
	u16 index = 0;
	//u16 index, enemyX, enemyY;

	enemy newEnemy = { 0, 0, 0, PAD_LEFT };
	
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


	/*
	for ( i = 0; i < NUM_ENEMIES; ++i ) {
		enemyData[i].x = enemyData[i].initX;
		enemyData[i].y = enemyData[i].initY;
		enemyData[i].direction = enemyData[i].initDir;

		//memcpy(enemySpriteData[i], enemySpriteDataTemplate, ENEMY_DATA_SIZE);

		for ( j = 0; j < ENEMY_DATA_SIZE; ++j ) {
			enemySpriteData[i][j] = enemySpriteDataTemplate[j];
		}

		if ( enemyData[i].direction == PAD_RIGHT ) {
			flipSprite(enemySpriteData[i], PAD_RIGHT);
		}		
		setSpriteFrame(enemySpriteData[i], enemyFrames[enemyData[i].frame]);
	}
	*/
}


/*********** State Management ***********/

void updateEnemies(void) {

	static u8 i, vertCollide, horizCollide;
	for ( i = 0; i < numEnemies; i++ ) {
		vertCollide = collideCheckVertical(enemyData[i].x, enemyData[i].y + 1, PAD_DOWN);
		if ( vertCollide != TILE_ALLCOLLIDE ) {
			enemyData[i].y += 1;
		} else {
			if ( enemyData[i].direction == PAD_RIGHT ) {
				enemyData[i].x += 1;
				horizCollide = collideCheckHorizontal(enemyData[i].x, enemyData[i].y, PAD_RIGHT);
				if ( ( horizCollide == TILE_ALLCOLLIDE ) || ( horizCollide == TILE_ENEMYCOLLIDE ) ) {
					flipSprite(enemySpriteData[i], 0);
					enemyData[i].direction = PAD_LEFT;
				}
			} else {
				enemyData[i].x -= 1;
				horizCollide = collideCheckHorizontal(enemyData[i].x, enemyData[i].y, PAD_LEFT);
				if ( ( horizCollide == TILE_ALLCOLLIDE ) || ( horizCollide == TILE_ENEMYCOLLIDE ) ) {
					flipSprite(enemySpriteData[i], 1);
					enemyData[i].direction = PAD_RIGHT;
				}
			}			
		}
	}

}

void updatePlayerVerticalMovement(void) {

	if ( ( pad & PAD_A ) && ( playerY > 8 ) && ( !playerJumping ) ) {
		playerVertVel = PLAYER_INIT_JUMP_VEL;
		playerJumping = 1;
		playerJumpCounter = 0;
	} 

	// stop ascent when player releases A button
	if ( !( pad & PAD_A ) && playerJumping && ( playerVertVel > 0 ) ) {
		playerVertVel = 0;
	}

	if ( playerVertVel > 0 ) {
		// moving up, jumping
		playerY -= playerVertVel;
		if ( collideCheckVertical(playerX, playerY, PAD_UP) == TILE_ALLCOLLIDE ) { 
			playerY = (playerY & 0xF8) + 8;
		}			
	} else {
		// falling
		if ( collideCheckVertical(playerX, playerY + 2, PAD_DOWN) != TILE_ALLCOLLIDE ) { 
			playerY -= playerVertVel;
		} else {
			playerY = (playerY & 0xF8) + 7;
			playerJumping = 0;
		}			
	}

	// acceleration toward ground
	// setting max fall speed more than -3 causes falls through the floor - why?
	if ( ( playerJumpCounter == 4 ) && ( playerVertVel > -3 ) ) {
		playerVertVel -= 1; 
		playerJumpCounter = 0;
	}

	++playerJumpCounter;
}

void playerMoveHorizontal(void) {	
	if ( pad & PAD_LEFT ) {
		playerDir = PAD_LEFT;
	} else if ( pad & PAD_RIGHT ) {
		playerDir = PAD_RIGHT;
	}

	if ( ( pad & PAD_LEFT ) && playerX > 0 ) {
		playerX -= 1;
	}
	if ( ( pad & PAD_RIGHT ) && playerX < 240 ) {
		playerX += 1;
	}
	bgHorizCollideCheck(&playerX, &playerY, pad);
}



void updatePlayerAttack(void) {
	if ( pad & PAD_B ) {
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
		}
	}
}

void updatePotionMovement(void) {
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
		//if ( ( potionVerticalVel >= -1 ) && ( ( frameCount & 0x3 ) == 2 ) ) {
		if ( ( potionVerticalVel >= -2 ) && ( potionMoveCounter == 3 ) ) {
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


/*********** Main ***********/

void main(void)
{

	// TODO next:

	// - work on mechanics
	// - study enemy behavior in games
	// - make const array of map data and write js to automate building it from CSVs
	// - continue refactoring, break out code into modules
	// - if PRG ROM runs short, switch to NROM-256 or pack map data


	memcpy(palSprites, paldat, 16);
	memcpy(palBG, paldat + 16, 4);

	//unrleCollision();

	pal_spr(palSprites);
	pal_bg(palBG);

	vram_adr(NAMETABLE_A); //unpack nametable into VRAM
	vram_unrle(map1);	

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

		updatePlayerSprite();
		updateEnemySprites();
		updatePotionSprite();
		spriteCount();

		updateEnemies();
		playerMoveHorizontal();
		updatePlayerVerticalMovement();
		updatePlayerAttack();
		updatePotionMovement();

		four_Sides(playerX, playerY);	
		playerEnemyCollideCheck();
		potionEnemyCollideCheck();

		
		if ( playerEnemyColliding ) {
			setSpritePalette(playerSpriteData, 0x0);
		} else {
			setSpritePalette(playerSpriteData, 0x3);
		}

		++frameCount;
	}
}