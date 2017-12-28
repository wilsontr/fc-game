

#include <stdint.h>
#include "neslib.h"
#include "test_nam.h"
#include "test_nam_coll_rle.h"

void __fastcall__ memcpy (void* dest, const void* src, int count);

typedef uint8_t u8;
typedef uint16_t u16;

#define ENEMY_DATA_SIZE	17
#define NUM_ENEMIES 	4
#define DIRECTION_LEFT  0x1
#define DIRECTION_RIGHT 0x2
#define DIRECTION_UP	0x4
#define DIRECTION_DOWN 	0x8

#pragma bss-name (push, "ZEROPAGE")
#pragma data-name (push, "ZEROPAGE")

u8 oam_off;

#pragma data-name(pop)
#pragma bss-name (pop)

static u8 enemyInitX = 150;
static u8 enemyInitY = 50;

static u8 player_x;
static u8 player_y;
static u8 playerDir;

static u8 enemy_x;
static u8 enemy_y;

static u8 leftSide, rightSide, topSide, bottomSide;
static u16 testCorner;


//variables

static u8 i;
static u8 pad,spr;
static u8 touch;
static u8 frame;
static u8 playerFrame;
static u8 playerEnemyColliding;

struct enemyStruct {
	u8 x;
	u8 y; 
	u8 frame;
	u8 direction;

	u8 range;
	u8 initX;
	u8 initY;
	u8 initDir;
};

typedef struct enemyStruct enemy;

u8 testColl[960];

u8 X1_Right_Side;	//for collision test
u8 X1_Left_Side;
u8 Y1_Bottom;
u8 Y1_Top;
u16 corner;

enemy enemyData[4] = {
	// x, y, frame, direction, range, initX, initY, initDir 
	{ 150, 50,  0, 0, 50, 150, 50,  PAD_LEFT  },
	{ 75,  75,  1, 0, 25, 75,  75,  PAD_RIGHT },
	{ 100, 100, 1, 0, 30, 100, 100, PAD_LEFT  },
	{ 200, 200, 0, 0, 60, 200, 200, PAD_LEFT  }
};


extern const u8 paldat[];

const u8 playerFrames[2][4] = {
	{ 0x04, 0x05, 0x14, 0x15 },
	{ 0x24, 0x25, 0x34, 0x35 }
};


const u8 enemyFrames[2][4] = {
	{ 0x06, 0x07, 0x16, 0x17 },
	{ 0x26, 0x27, 0x36, 0x37 }
};

// x offset, y offset, tile, attribute

u8 playerSpriteData[17] = {
	0, 0, 0x04, 0x2,
	8, 0, 0x05, 0x2,
	0, 8, 0x14, 0x2,
	8, 8, 0x15, 0x2,
	128
};

const u8 enemySpriteDataTemplate[17] = {
	0, 0, 0, 0x3,
	8, 0, 0, 0x3,
	0, 8, 0, 0x3,
	8, 8, 0, 0x3,
	128
};

u8 enemySpriteData[4][17];


u8 palSprites[4];
u8 palBG[4];

void __fastcall__ setFrame(u8 *sprite, const u8 *frame) {
	sprite[2] = frame[0];
	sprite[6] = frame[1];
	sprite[10] = frame[2];
	sprite[14] = frame[3];
}

void __fastcall__ setPalette(u8 *sprite, u8 palette) {
	u8 i;
	for ( i = 3; i <= 15; i = i + 4 ) {
		sprite[i] &= ~(0x3);
		sprite[i] |= palette;
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
			testColl[outPointer] = currentByte;
			++outPointer;
		}
	}
}

void __fastcall__ four_Sides(u8 originX, u8 originY) {
	if (originX < (255 - 1)){	// find the left side
		X1_Left_Side = originX + 1;
	}
	else {
		X1_Left_Side = 255;
	}
	if (originX < (255 - 15)){	// find the right side
		X1_Right_Side = originX + 15;
	}
	else {
		X1_Right_Side = 255;
	}
	Y1_Top = originY + 1;	// our top is the same as the master Y
	
	if (originY < (255)){ // find the bottom side
		Y1_Bottom = originY + 16;
	}
	else {
		Y1_Bottom = 255;
	}
}

u16 __fastcall__ getCollisionIndex(u8 screenX, u8 screenY) {
	return ((u16) screenX >> 3) + (((u16) screenY >> 3) << 5);
}

/*

void collide_Check_LR (void) {
	if ((pad & PAD_RIGHT) != 0){ 	// first check right
		corner = getCollisionIndex(X1_Right_Side, Y1_Top); // top right
		if (testColl[corner] != 0) {
			player_x = (player_x & 0xf8); // if collision, realign
		} else {
			corner = getCollisionIndex(X1_Right_Side, Y1_Bottom); // bottom right
			if (testColl[corner] != 0)
				player_x = (player_x & 0xf8); // if collision, realign
		}
	}
	else if ((pad & PAD_LEFT) != 0){ // check left
		corner = getCollisionIndex(X1_Left_Side, Y1_Top); // top left
		if (testColl[corner] != 0) {
			player_x = (player_x & 0xf8) + 8; // if collision, realign
		} else {
			corner = getCollisionIndex(X1_Left_Side, Y1_Bottom); // bottom left
			if (testColl[corner] != 0)
				player_x = (player_x & 0xf8) + 8; // if collision, realign			
		}
	}
}


void collide_Check_UD (void) {
	if ((pad & PAD_DOWN) != 0){ // down first
		corner = getCollisionIndex(X1_Right_Side, Y1_Bottom); // bottom right
		if (testColl[corner] != 0) {
			player_y = (player_y & 0xf8) - 1; // if collision, realign
		} else {
			corner = getCollisionIndex(X1_Left_Side, Y1_Bottom); // bottom left
			if (testColl[corner] != 0)
				player_y = (player_y & 0xf8) - 1; // if collision, realign			
		}
	}
	else if ((pad & PAD_UP) != 0) { //or up
		corner = getCollisionIndex(X1_Right_Side, Y1_Top); // top right
		if (testColl[corner] != 0)
			player_y = (player_y & 0xf8) + 7; // if collision, realign

		corner = getCollisionIndex(X1_Left_Side, Y1_Top);  // top left
		if (testColl[corner] != 0)
			player_y = (player_y & 0xf8) + 7; // if collision, realign
	}
}
*/

u8 __fastcall__ collideCheckVertical(u8 originX, u8 originY, u8 direction) {

	leftSide = originX + 1;
	rightSide = originX + 15;
	topSide = originY + 1;
	bottomSide = originY + 16;

	if ( ( (direction & PAD_UP) != 0) ) {
		testCorner = getCollisionIndex(rightSide, topSide);
		if ( testColl[testCorner] == 0 ) {
			testCorner = getCollisionIndex(leftSide, topSide);
		}
	} else if ( (direction & PAD_DOWN) != 0 ) {
		testCorner = getCollisionIndex(rightSide, bottomSide);
		if ( testColl[testCorner] == 0 ) {
			testCorner = getCollisionIndex(leftSide, bottomSide);
		}
	}

	return testColl[testCorner];
}

u8 __fastcall__ collideCheckHorizontal(u8 originX, u8 originY, u8 direction) {

	leftSide = originX + 1;
	rightSide = originX + 15;
	topSide = originY + 1;
	bottomSide = originY + 16;

	if ( ( (direction & PAD_LEFT) != 0 ) ) {
		testCorner = getCollisionIndex(leftSide, topSide);
		if ( testColl[testCorner] == 0 ) {
			testCorner = getCollisionIndex(leftSide, bottomSide);
		}
	} else if ( (direction & PAD_RIGHT) != 0 ) {
		testCorner = getCollisionIndex(rightSide, topSide);
		if ( testColl[testCorner] == 0 ) {
			testCorner = getCollisionIndex(rightSide, bottomSide);
		}
	}

	return testColl[testCorner];
}


void __fastcall__ bgVertCollideCheck(u8 *x, u8 *y, u8 dir) {
	u8 colliding = collideCheckVertical(*x, *y, dir);
	if ( colliding == 1 ) {
		if ( dir & PAD_UP ) {
			*y = (*y & 0xf8) + 7;
		} else if ( dir & PAD_DOWN ) {
			*y = (*y & 0xf8) - 1;
		}
	}
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



void updateEnemies(void) {

	u8 i;
	for ( i = 0; i < NUM_ENEMIES; i++ ) {
		if ( enemyData[i].direction == PAD_RIGHT ) {
			enemyData[i].x += 1;
			if ( collideCheckHorizontal(enemyData[i].x, enemyData[i].y, PAD_RIGHT) ) {
				flipSprite(enemySpriteData[i], 0);
				enemyData[i].direction = PAD_LEFT;
			}
		} else {
			enemyData[i].x -= 1;
			if ( collideCheckHorizontal(enemyData[i].x, enemyData[i].y, PAD_LEFT) ) {
				flipSprite(enemySpriteData[i], 1);
				enemyData[i].direction = PAD_RIGHT;
			}
		}

		/*
		if ( enemyData[i].initDir == PAD_RIGHT ) {
			if ( enemyData[i].x <= enemyData[i].initX ) {
				flipSprite(enemySpriteData[i], 1);
				enemyData[i].direction = PAD_RIGHT;				
			} else if ( enemyData[i].x > ( enemyData[i].initX + enemyData[i].range ) ) {
				flipSprite(enemySpriteData[i], 0);
				enemyData[i].direction = PAD_LEFT;
			}		
		} else {
			if ( enemyData[i].x <= ( enemyData[i].initX - enemyData[i].range ) ) {
				flipSprite(enemySpriteData[i], 1);
				enemyData[i].direction = PAD_RIGHT;				
			} else if ( enemyData[i].x > enemyData[i].initX ) {
				flipSprite(enemySpriteData[i], 0);
				enemyData[i].direction = PAD_LEFT;
			}					
		}
		*/
	}
}


void playerEnemyCollideCheck(void) {

	u8 enemyTop;
	u8 enemyBottom;
	u8 enemyLeft;
	u8 enemyRight;
	u8 j;

	playerEnemyColliding = 0;

	for ( j = 0; j < NUM_ENEMIES; ++j ) {
		enemyTop = enemyData[j].y + 2;
		enemyBottom = enemyData[j].y + 14;
		enemyLeft = enemyData[j].x + 2;
		enemyRight = enemyData[j].x + 14;

		if ( !( X1_Right_Side < enemyLeft  || 
				X1_Left_Side >= enemyRight || 
				Y1_Bottom <  enemyTop || 
				Y1_Top    >= enemyBottom ) ) {
			playerEnemyColliding = 1;
		}		
	}
}

void main(void)
{

	// TODO next:

	// - generalize/rewrite background collision detection
	// - add gravity
	// - study enemy behavior in games

	u8 j;


	memcpy(palSprites, paldat, 16);
	memcpy(palBG, paldat, 4);

	unrleCollision();

	pal_spr(palSprites);
	pal_bg(palBG);

	vram_adr(NAMETABLE_A); //unpack nametable into VRAM
	vram_unrle(test_nam);	

	ppu_on_all(); //enable rendering
	//set initial coords
	
	player_x = 52;
	player_y = 100;
	playerDir = PAD_RIGHT;

	enemy_x = enemyInitX;
	enemy_y = enemyInitY;

	//init other vars
	
	touch = 0; // collision flag
	frame = 0; // frame counter

	playerFrame = 0;

	
	setFrame(playerSpriteData, playerFrames[playerFrame]);

	for ( i = 0; i < NUM_ENEMIES; ++i ) {
		enemyData[i].x = enemyData[i].initX;
		enemyData[i].y = enemyData[i].initY;
		enemyData[i].direction = enemyData[i].initDir;

		//memcpy(enemySpriteData[i], *(enemySpriteDataTemplate), 17);
		for ( j = 0; j < ENEMY_DATA_SIZE; ++j ) {
			enemySpriteData[i][j] = enemySpriteDataTemplate[j];
		}

		if ( enemyData[i].direction == PAD_RIGHT ) {
			flipSprite(enemySpriteData[i], PAD_RIGHT);
		}		
		setFrame(enemySpriteData[i], enemyFrames[enemyData[i].frame]);
	}
	

	// now the main loop

	while ( 1 )
	{
		ppu_wait_frame(); // wait for next TV frame
	
		//process player
		
		spr = 0;
		i = 0;

		// update player movement
		pad = pad_poll(i);

		if ( pad & PAD_RIGHT ) {
			flipSprite(playerSpriteData, 1);
		} else if ( pad & PAD_LEFT ) {
			flipSprite(playerSpriteData, 0);
		}		

		// animate player sprite
		if ( ( frame & 0x0F ) == 0x0F ) {
			playerFrame ^= 1;
			setFrame(playerSpriteData, playerFrames[playerFrame]);

		} 

		// update player sprite
		spr = oam_meta_spr(player_x, player_y, spr, playerSpriteData);

		// update enemy sprites
		for ( i = 0; i < NUM_ENEMIES; ++i ) {
			
			// animate
			if ( ( frame & 0x0F ) == 0x0F ) {
				enemyData[i].frame ^= 1;
				setFrame(enemySpriteData[i], enemyFrames[enemyData[i].frame]);
			}
			
			spr = oam_meta_spr(enemyData[i].x, enemyData[i].y, spr, enemySpriteData[i]);	
		}

		updateEnemies();

		playerDir = pad;
		
	
		if ( pad&PAD_LEFT  && player_x > 0 ) {
			player_x -= 2;
		}
		if ( pad&PAD_RIGHT && player_x < 240 ) {
			player_x += 2;
		}

	
		// four_Sides(player_x, player_y);	
		// collide_Check_LR();		

		bgHorizCollideCheck(&player_x, &player_y, playerDir);

		if ( pad&PAD_UP    && player_y > 0 ) { 
			player_y -= 2;
		}
		if ( pad&PAD_DOWN  && player_y < 220 ) {
			player_y += 2;
		}

		// four_Sides(player_x, player_y);	
		// collide_Check_UD();


		bgVertCollideCheck(&player_x, &player_y, playerDir);
		
		four_Sides(player_x, player_y);	
		playerEnemyCollideCheck();

		
		if ( playerEnemyColliding ) {
			setPalette(playerSpriteData, 0x0);
		} else {
			setPalette(playerSpriteData, 0x2);
		}

		++frame;
	}
}