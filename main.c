
#include "neslib.h"

/*
// frame counter
static unsigned char frame;

static unsigned char player_x;
static unsigned char player_y;

static unsigned char spr;

*/





//variables

static unsigned char i;
static unsigned char pad,spr;
static unsigned char touch;
static unsigned char frame;

//two players coords

static unsigned char cat_x[2];
static unsigned char cat_y[2];


const unsigned char testSprite[] = {
	//0, 0, 0x90, 1
	0, 0, 0x20, 0,
	128
};


/*
//first player metasprite, data structure explained in neslib.h

const unsigned char metaCat1[]={
	0,	0,	0x50,	0,
	8,	0,	0x51,	0,
	16,	0,	0x52,	0,
	0,	8,	0x60,	0,
	8,	8,	0x61,	0,
	16,	8,	0x62,	0,
	0,	16,	0x70,	0,
	8,	16,	0x71,	0,
	16,	16,	0x72,	0,
	128
};

//second player metasprite, the only difference is palette number

const unsigned char metaCat2[]={
	0,	0,	0x50,	1,
	8,	0,	0x51,	1,
	16,	0,	0x52,	1,
	0,	8,	0x60,	1,
	8,	8,	0x61,	1,
	16,	8,	0x62,	1,
	0,	16,	0x70,	1,
	8,	16,	0x71,	1,
	16,	16,	0x72,	1,
	128
};
*/




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

	



	ppu_on_all(); //enable rendering
	//set initial coords
	
	cat_x[0]=52;
	cat_y[0]=100;
	cat_x[1]=180;
	cat_y[1]=100;

	//init other vars
	
	touch=0;//collision flag
	frame=0;//frame counter

	//now the main loop

	while(1)
	{
		ppu_wait_frame();//wait for next TV frame
	

		pal_col(16, 0x27);//set first sprite color
		pal_col(17, 0x21);//set first sprite color
		pal_col(18, 0x22);//set first sprite color
		pal_col(19, 0x23);//set first sprite color
		pal_col(20, 0x24);//set first sprite color
		//pal_col(21,touch?i:0x26);//set second sprite color

		//process players
		
		spr=0;
		i=0;

		//for(i=0;i<2;++i)
		//{
			//display metasprite
			
			//spr=oam_meta_spr(cat_x[i],cat_y[i],spr,!i?metaCat1:metaCat2);

			spr = oam_meta_spr(cat_x[0], cat_y[0], spr, testSprite);

			//poll pad and change coordinates
			
			pad=pad_poll(i);

			if(pad&PAD_LEFT &&cat_x[i]>  0) cat_x[i]-=2;
			if(pad&PAD_RIGHT&&cat_x[i]<232) cat_x[i]+=2;
			if(pad&PAD_UP   &&cat_y[i]>  0) cat_y[i]-=2;
			if(pad&PAD_DOWN &&cat_y[i]<212) cat_y[i]+=2;
		//}

		//check for collision for a smaller bounding box
		//metasprite is 24x24, collision box is 20x20
		
		if(!(cat_x[0]+22< cat_x[1]+2 ||
		     cat_x[0]+ 2>=cat_x[1]+22||
	         cat_y[0]+22< cat_y[1]+2 ||
		     cat_y[0]+ 2>=cat_y[1]+22)) touch=1; else touch=0;

		frame++;
	}
	
}