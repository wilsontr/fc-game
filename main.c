

#include <stdint.h>
#include "neslib.h"
#include "test_nam.h"
#include "test_nam_coll_rle.h"

void __fastcall__ memcpy (void* dest, const void* src, int count);

typedef uint8_t u8;
typedef uint16_t u16;


#pragma bss-name (push, "ZEROPAGE")
#pragma data-name (push, "ZEROPAGE")

u8 oam_off;

#pragma data-name(pop)
#pragma bss-name (pop)


static u8 player_x;
static u8 player_y;

//variables

static u8 i;
static u8 pad,spr;
static u8 touch;
static u8 frame;

u8 testColl[960];

u8 X1_Right_Side;	//for collision test
u8 X1_Left_Side;
u8 Y1_Bottom;
u8 Y1_Top;
u16 corner;
u8 playerFlip;

extern const u8 paldat[];




// x offset, y offset, tile, attribute

u8 playerSpriteData[17] = {
	0, 0, 0x04, 0,
	8, 0, 0x05, 0,
	0, 8, 0x14, 0,
	8, 8, 0x15, 0,
	128
};

u8 palSprites[4];
u8 palBG[4];

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

void four_Sides (void){
	if (player_x < (255 - 1)){	// find the left side
		X1_Left_Side = player_x + 1;
	}
	else {
		X1_Left_Side = 255;
	}
	if (player_x < (255 - 15)){	// find the right side
		X1_Right_Side = player_x + 15;
	}
	else {
		X1_Right_Side = 255;
	}
	Y1_Top = player_y + 1;	// our top is the same as the master Y
	
	if (player_y < (255)){ // find the bottom side
		Y1_Bottom = player_y + 16;
	}
	else {
		Y1_Bottom = 255;
	}
}

u16 __fastcall__ getCollisionIndex(u8 screenX, u8 screenY) {
	return ((u16) screenX >> 3) + (((u16) screenY >> 3) << 5);
}

void collide_Check_LR (void){
	four_Sides();	// set the L R bottom top variables
	
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


void collide_Check_UD (void){
	four_Sides();
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

void main(void)
{

	memcpy(palSprites, paldat + 8, 4);
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

	//init other vars
	
	touch = 0; // collision flag
	frame = 0; // frame counter

	playerFlip = 0;


	

	// now the main loop

	while(1)
	{
		ppu_wait_frame(); // wait for next TV frame
			
	
		//process player
		
		spr = 0;
		i = 0;

		// This is dumb and memory-intensive. Find a way to rearrange the tiles programmatically

		if ( playerFlip) {
			
		} else {
			
		}

		spr = oam_meta_spr(player_x, player_y, spr, playerSpriteData);
		
		
		pad = pad_poll(i);


		if ( pad & PAD_RIGHT ) {
			//playerFlip = 1;
			flipSprite(playerSpriteData, 1);
		} else if ( pad & PAD_LEFT ) {
			//playerFlip = 0;
			flipSprite(playerSpriteData, 0);
		}



		if(pad&PAD_LEFT  && player_x >  0)  player_x -= 2;
		if(pad&PAD_RIGHT && player_x < 240) player_x += 2;
		collide_Check_LR();

		if(pad&PAD_UP    && player_y > 0)   player_y -= 2;
		if(pad&PAD_DOWN  && player_y < 220) player_y += 2;
		collide_Check_UD();

		//check for collision for a smaller bounding box
		//metasprite is 24x24, collision box is 20x20
		
		/*
		if(!(cat_x[0]+22< cat_x[1]+2 ||
		     cat_x[0]+ 2>=cat_x[1]+22||
	         cat_y[0]+22< cat_y[1]+2 ||
		     cat_y[0]+ 2>=cat_y[1]+22)) touch=1; else touch=0;
		*/

		++frame;
	}
	
}