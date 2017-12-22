
#include "neslib.h"
#include "test_nam.h"
#include "test_nam_coll.h"

/*
// frame counter
static unsigned char frame;


static unsigned char spr;

*/




static unsigned char player_x;
static unsigned char player_y;
static unsigned char colliding;



//variables

static unsigned char i;
static unsigned char pad,spr;
static unsigned char touch;
static unsigned char frame;



// x offset, y offset, tile, attribute

const unsigned char testSprite[] = {
	0, 0, 0x20, 0,
	8, 0, 0x21, 0,
	0, 8, 0x22, 0,
	8, 8, 0x23, 0,
	128
};

const unsigned char palSprites[4]={
	0x0f, 0x22, 0x25, 0x24
};

const unsigned char palSpritesAlt[4]={
	0x0f, 0x1B, 0x19, 0x29
};


const unsigned char palBG[4]={
	0x0f, 0x06, 0x17, 0x16
};

void checkCollision(void) {
	unsigned char collIndex = ( ( player_x & 0xF0 ) >> 4 ) + ( player_y & 0xF0 );
	if ( testColl[collIndex] ) {
		colliding = 1;
	} else {
		colliding = 0;
	}
}

void main(void)
{
	/*
	ppu_on_all(); //enable rendering

	frame = 0;

	player_x = 52;
	player_y = 100;


	while(1) {
		ppu_wait_frame();
		pal_col(71, 0x21);
		spr = 0;
		spr = oam_meta_spr(player_x, player_y, spr, testSprite);
		++frame;
	}
	*/

	colliding = 0;

	
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


	// now the main loop

	while(1)
	{
		ppu_wait_frame(); // wait for next TV frame
	

		checkCollision();

		if ( colliding ) {
			pal_spr(palSpritesAlt);
		} else {
			pal_spr(palSprites);
		}
		

		//pal_col(16, 0x27); //set first sprite color
		
		//process player
		
		spr = 0;
		i = 0;

		spr = oam_meta_spr(player_x, player_y, spr, testSprite);
		
		pad = pad_poll(i);

		if(pad&PAD_LEFT  && player_x >  0)  player_x -= 2;
		if(pad&PAD_RIGHT && player_x < 240) player_x += 2;
		if(pad&PAD_UP    && player_y > 0)   player_y -= 2;
		if(pad&PAD_DOWN  && player_y < 220) player_y += 2;

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