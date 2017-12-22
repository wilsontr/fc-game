;
; File generated by cc65 v 2.16 - Git N/A
;
	.fopt		compiler,"cc65 v 2.16 - Git N/A"
	.setcpu		"6502"
	.smart		on
	.autoimport	on
	.case		on
	.debuginfo	off
	.importzp	sp, sreg, regsave, regbank
	.importzp	tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4
	.macpack	longbranch
	.forceimport	__STARTUP__
	.import		_pal_bg
	.import		_pal_spr
	.import		_ppu_wait_frame
	.import		_ppu_on_all
	.import		_oam_meta_spr
	.import		_pad_poll
	.import		_vram_adr
	.import		_vram_unrle
	.export		_test_nam
	.export		_testColl
	.export		_testSprite
	.export		_palSprites
	.export		_palBG
	.export		_main

.segment	"RODATA"

_test_nam:
	.byte	$01
	.byte	$40
	.byte	$01
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$0A
	.byte	$41
	.byte	$01
	.byte	$02
	.byte	$00
	.byte	$01
	.byte	$0F
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$0D
	.byte	$41
	.byte	$00
	.byte	$01
	.byte	$0E
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$0E
	.byte	$41
	.byte	$00
	.byte	$01
	.byte	$0D
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$1D
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$10
	.byte	$41
	.byte	$00
	.byte	$01
	.byte	$0B
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$1D
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$12
	.byte	$41
	.byte	$00
	.byte	$01
	.byte	$09
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$1D
	.byte	$40
	.byte	$40
	.byte	$50
	.byte	$00
	.byte	$01
	.byte	$13
	.byte	$41
	.byte	$00
	.byte	$01
	.byte	$07
	.byte	$40
	.byte	$40
	.byte	$50
	.byte	$00
	.byte	$01
	.byte	$1C
	.byte	$40
	.byte	$40
	.byte	$50
	.byte	$00
	.byte	$01
	.byte	$1C
	.byte	$40
	.byte	$40
	.byte	$50
	.byte	$00
	.byte	$01
	.byte	$1C
	.byte	$40
	.byte	$40
	.byte	$50
	.byte	$00
	.byte	$01
	.byte	$1C
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$1D
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$1D
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$1D
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$19
	.byte	$41
	.byte	$41
	.byte	$00
	.byte	$00
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$1A
	.byte	$41
	.byte	$01
	.byte	$02
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$1D
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$1D
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$1D
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$1D
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$1D
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$02
	.byte	$40
	.byte	$01
	.byte	$04
	.byte	$00
	.byte	$01
	.byte	$03
	.byte	$40
	.byte	$01
	.byte	$05
	.byte	$00
	.byte	$01
	.byte	$03
	.byte	$40
	.byte	$01
	.byte	$04
	.byte	$00
	.byte	$01
	.byte	$02
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$00
	.byte	$41
	.byte	$01
	.byte	$05
	.byte	$00
	.byte	$01
	.byte	$03
	.byte	$41
	.byte	$01
	.byte	$05
	.byte	$00
	.byte	$01
	.byte	$03
	.byte	$41
	.byte	$01
	.byte	$06
	.byte	$00
	.byte	$40
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$04
	.byte	$41
	.byte	$41
	.byte	$00
	.byte	$01
	.byte	$16
	.byte	$40
	.byte	$01
	.byte	$40
	.byte	$00
	.byte	$01
	.byte	$3E
	.byte	$00
	.byte	$01
	.byte	$00
_testColl:
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$01
	.byte	$01
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$01
	.byte	$01
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$01
	.byte	$01
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$01
	.byte	$01
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$01
	.byte	$01
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$01
	.byte	$01
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$01
	.byte	$01
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$01
	.byte	$01
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$01
	.byte	$01
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$01
	.byte	$01
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$01
	.byte	$01
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$01
	.byte	$01
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$00
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
	.byte	$01
_testSprite:
	.byte	$00
	.byte	$00
	.byte	$20
	.byte	$00
	.byte	$08
	.byte	$00
	.byte	$21
	.byte	$00
	.byte	$00
	.byte	$08
	.byte	$22
	.byte	$00
	.byte	$08
	.byte	$08
	.byte	$23
	.byte	$00
	.byte	$80
_palSprites:
	.byte	$0F
	.byte	$22
	.byte	$25
	.byte	$24
_palBG:
	.byte	$0F
	.byte	$06
	.byte	$17
	.byte	$16

.segment	"BSS"

.segment	"BSS"
_player_x:
	.res	1,$00
.segment	"BSS"
_player_y:
	.res	1,$00
.segment	"BSS"
_i:
	.res	1,$00
.segment	"BSS"
_pad:
	.res	1,$00
.segment	"BSS"
_spr:
	.res	1,$00
.segment	"BSS"
_touch:
	.res	1,$00
.segment	"BSS"
_frame:
	.res	1,$00

; ---------------------------------------------------------------
; void __near__ main (void)
; ---------------------------------------------------------------

.segment	"CODE"

.proc	_main: near

.segment	"CODE"

;
; pal_spr(palSprites);
;
	lda     #<(_palSprites)
	ldx     #>(_palSprites)
	jsr     _pal_spr
;
; pal_bg(palBG);
;
	lda     #<(_palBG)
	ldx     #>(_palBG)
	jsr     _pal_bg
;
; vram_adr(NAMETABLE_A); //unpack nametable into VRAM
;
	ldx     #$20
	lda     #$00
	jsr     _vram_adr
;
; vram_unrle(test_nam); 
;
	lda     #<(_test_nam)
	ldx     #>(_test_nam)
	jsr     _vram_unrle
;
; ppu_on_all(); //enable rendering
;
	jsr     _ppu_on_all
;
; player_x = 52;
;
	lda     #$34
	sta     _player_x
;
; player_y = 100;
;
	lda     #$64
	sta     _player_y
;
; touch = 0; // collision flag
;
	lda     #$00
	sta     _touch
;
; frame = 0; // frame counter
;
	sta     _frame
;
; ppu_wait_frame(); // wait for next TV frame
;
L01F8:	jsr     _ppu_wait_frame
;
; spr = 0;
;
	lda     #$00
	sta     _spr
;
; i = 0;
;
	sta     _i
;
; spr = oam_meta_spr(player_x, player_y, spr, testSprite);
;
	jsr     decsp3
	lda     _player_x
	ldy     #$02
	sta     (sp),y
	lda     _player_y
	dey
	sta     (sp),y
	lda     _spr
	dey
	sta     (sp),y
	lda     #<(_testSprite)
	ldx     #>(_testSprite)
	jsr     _oam_meta_spr
	sta     _spr
;
; pad = pad_poll(i);
;
	lda     _i
	jsr     _pad_poll
	sta     _pad
;
; if(pad&PAD_LEFT  && player_x >  0)  player_x -= 2;
;
	and     #$40
	beq     L0226
	lda     _player_x
	beq     L0226
	sec
	sbc     #$02
	sta     _player_x
;
; if(pad&PAD_RIGHT && player_x < 232) player_x += 2;
;
L0226:	lda     _pad
	and     #$80
	beq     L022A
	lda     _player_x
	cmp     #$E8
	bcs     L022A
	lda     #$02
	clc
	adc     _player_x
	sta     _player_x
;
; if(pad&PAD_UP    && player_y > 0)   player_y -= 2;
;
L022A:	lda     _pad
	and     #$10
	beq     L022E
	lda     _player_y
	beq     L022E
	sec
	sbc     #$02
	sta     _player_y
;
; if(pad&PAD_DOWN  && player_y < 212) player_y += 2;
;
L022E:	lda     _pad
	and     #$20
	beq     L0232
	lda     _player_y
	cmp     #$D4
	bcs     L0232
	lda     #$02
	clc
	adc     _player_y
	sta     _player_y
;
; ++frame;
;
L0232:	inc     _frame
;
; while(1)
;
	jmp     L01F8

.endproc

