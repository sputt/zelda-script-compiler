script_table:
	.dw	delay_return
		;arg1
	.dw	defer
		;arg1,arg2
	.dw	change_tile
		;(arg1/16)+arg2, arg3
	.dw	change_tile_group
		;(arg1/16)+arg2, arg3, arg4, arg5
	.dw	prevent
		;arg1, arg2
	.dw	prevent_tile
		;(arg1/16)+arg2, arg3
	.dw	show_text
		;*arg1
	.dw	show_text_script
		;*arg1,arg2
	.dw	show_double_text
		;*arg1
	.dw	scroll_screen
		;arg1,arg2,arg3
	.dw	trigger_screen
		;arg1,arg2,arg3
	.dw	trigger_screen_fast
		;arg1,arg2,arg3
	.dw	move_screen
		;arg1,arg2,arg3
	.dw	create_enemy
		;arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9
	.dw	create_object
		;arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,*arg0
	.dw	set_player_anim
		;arg1, *arg2
	.dw	set_animate_attr8
		;((arg1&$0F)<<4)|(arg2&$0F),arg3
	.dw	set_animate_attr16
		;((arg1&$0F)<<4)|(arg2&$0F),*arg3
	.dw	set_object_attr8
		;((arg1&$0F)<<4)|(arg2&$0F),arg3
	.dw	set_object_attr16
		;((arg1&$0F)<<4)|(arg2&$0F),*arg3
	.dw	set_enemy_attr8
		;((arg1&$0F)<<4)|(arg2&$0F),arg3
	.dw	set_enemy_attr16
		;((arg1&$0F)<<4)|(arg2&$0F),*arg3
	.dw	set_misc_attr8
		;((arg1&$0F)<<4)|(arg2&$0F),arg3
	.dw	set_misc_attr16
		;((arg1&$0F)<<4)|(arg2&$0F),*arg3
	.dw	ex_update
		;null
	.dw	quake
		;arg1
	.dw	code
		;*arg1
	.dw	ll_new_pcom
		;arg1,*arg2
	.dw	ll_new_mcom
		;arg1,*arg2
	.dw	ll_set
		;*arg1,arg2
	.dw	kill_pcom
		;null
	.dw	set_player_gen
		;arg1
	.dw	show_choice
		;*arg1,arg2,arg3
	.dw	modify
		;arg1,arg2,arg3,arg4
	.dw	transaction
		;arg1,arg2,arg3
	.dw	goto
		;arg1
	.dw	return
		;null
;end of definitions (keep this comment for external program)
#define script_pcall(xx) exx\ push hl\ exx\ pcall(xx)\ exx\ pop hl\ exx

last = $0F

return_to_script:
	ld	a,(object_param1)
return_to_script_skip:
	ld	c,a
	ld	b,0
	ld	hl,script_buffer
	add	hl,bc
	jr	_


;to run a script, copy the script to
;script_buffer, and call run_script	
run_script:
	ld	hl,script_buffer
_
	exx
script_loop:
	call	get_arg8

	add	a,a
	ld	d,0
	ld	e,a
	ld	hl,script_table
	add	hl,de
	ld	de,script_loop
	push	de
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a
	jp	(hl)
	
;returns 8-bit argument in a
get_arg8:
	exx
	ld	a,(hl)
	inc	hl
	exx
	ret
	
;returns 16-bit argument in hl
get_arg16:
	exx
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	push	de
	exx
	pop	hl
	ret

;---------------------------------------
;---------------------------------------
delay_return:
	ld	hl,invert_buffer+12+18
	ld	(p_com_ptr),hl
	call	get_arg8
delay_return_variable:
	ld	hl,p_com_ctr
	add	a,20
	ld	(hl),a
	ld	hl,m_com_ctr
	add	a,3
	ld	(hl),a
	
	res f_lockscreen,(iy+flags+1)
	ret
;---------------------------------------
;---------------------------------------
defer:
	call	get_arg8
	push	af
	call	get_arg8
	ld	(object_param1),a
	pop	af
	ld	(player_flip_ctr),a
	ld	hl,_defer_com
	call	set_p_com_skip
	pop	hl
	set	f_input,(iy+flags)
	set	f_invunerable,(iy+flags)
	ret

_defer_com:
	.dw	no_action
	.db	1
	.dw	return_to_script
	.db	1
	.dw	kill_p_com
	
;---------------------------------------
;---------------------------------------
change_tile:
	call	get_arg8		;fetches offset
	ld	e,a
	ld	d,0
	ld	hl,map_data
	add	hl,de
	call	get_arg8		;fetches tile
	
;e - tile offset
;a - what to change to
;hl - map to change
change_tile_skip:
	ld b,(hl)		;old tile
	ld	(hl),a
	xor b
	bit col_bit,a
	jp nz,ex_update	;update screen
	
	push de
	call find_enemy
	pop de
	ret nz

	ld (hl+),e
	ld (hl+),e_noharm | e_invunerable | et_small_chu
	ld a,(screen_xc)
	push hl
	pop ix
	ld (hl+),a
	ld (hl+),16
	ld a,(screen_yc)
	ld (hl+),a
	ld (hl+),16
	ld (hl+),0 	;z
	ld (hl+),1	;d
	ld (hl+),d_down	;gen
	ld (hl+),1
	ld (hl+),lo(tile_dummy_com+1)
	ld (hl+),hi(tile_dummy_com+1)
	ld (hl+),1
	ld (hl+),lo(poof_anim+1)
	ld (hl),hi(poof_anim+1)
	xor a
	ret
;---------------------------------------
;---------------------------------------
;for use with 2 identical tiles and up
change_tile_group:
	call	get_arg8		;offset into map_data
	ld	e,a
	ld	d,0
	ld	hl,map_data
	add	hl,de
	call	get_arg8		;offset for each tile
	ld	e,a
	bit	7,a
	jr	z,_pos_tile_offset
	dec	d			;$FF
_pos_tile_offset:
	call	get_arg8		;number
	ld	b,a
	call	get_arg8		;tile to change to
_tile_change_loop:
	push bc
	push af
	push de
	push hl
	ld bc,-map_data
	add hl,bc
	ld e,l
	pop hl\ push hl
	call change_tile_skip
	jr nz,_
	set ef_anim,(ix+enemy_flags)
_
	pop hl
	pop de
	pop af
	pop bc
	add hl,de
	push af
	ld a,b
	add a,b
	inc a

	ld (ix+enemy_com_ctr),a
	ld (ix+enemy_anim_ctr),a
	pop af
	djnz	_tile_change_loop
	res ef_anim,(ix+enemy_flags)
	ret
;---------------------------------------
;---------------------------------------
prevent:
	call	get_arg8		;type
	ld	c,a	
	call	get_arg8
	ld	b,a			;number
	jp	new_prev_entry
;---------------------------------------
;---------------------------------------
prevent_tile:
	call	get_arg8		;read in offset
	ld	c,a
	call	get_arg8		;read in tile change
	ld	b,a
	jp	new_tprev_entry
;---------------------------------------
;---------------------------------------
modify:
	call	get_arg8
	ld	c,a
	call	get_arg8
	ld	b,a
	call	get_arg8
	ld	e,a
	call	get_arg8
	jp	new_mod_entry

;---------------------------------------
;---------------------------------------
show_choice:
	pop hl
	call get_arg16		;text
	call get_arg8
	ld (object_param1),a
	call get_arg8
	ld (object_param1+1),a
	ld de,show_choice_text_script_return
	jp choice_text_code
show_choice_text_script_return:
	call kill_p_com
	
	ld a,(choice_selection)
	or a
	jp z,return_to_script
	ld hl,object_param1+1
	ld a,(hl)
	dec hl
	ld (hl),a
	jp return_to_script
;---------------------------------------
;---------------------------------------	
show_text:
	call	get_arg16		;hl gets text
	exx
	push hl
	exx
	call	normal_text
	exx
	pop hl
	exx
	ret
;---------------------------------------
;---------------------------------------
show_text_script:
	pop	hl
	call	get_arg16		;text
	call	get_arg8
	ld	(object_param1),a
	ld	de,show_text_script_return
	jp	code_text
show_text_script_return:
	call kill_p_com
	jp return_to_script
	
show_double_text:
	call	get_arg16
	jp	normal_text
;---------------------------------------
;---------------------------------------
scroll_screen:
	call	_generic_scroll
	PCALL(ready_focus)
	pop	hl
	exx
	ret
;---------------------------------------
;---------------------------------------
trigger_screen:
	call	_generic_scroll
	PCALL(trigger_focus)
	pop	hl
	exx
	ret
trigger_screen_fast:
	call	trigger_screen
	ld	hl,update_screen_focus_double
	ld	(invert_buffer+12+6),hl
	ret
;---------------------------------------
;---------------------------------------	
move_screen:
	call	_generic_scroll
	PCALL(ready_move)
	pop	hl
	exx
	ret
				
_generic_scroll:
	res f_lockscreen,(iy+flags+1)
	
	ld hl,(player_xc)
	ld bc,64 << 8 | 96
	call center_rect
	ex de,hl
	
	call	get_arg16
	ld a,l
	cp -1
	jr nz,_
	ld l,e
_
	ld a,h
	cp -1
	jr nz,_
	ld h,d
_
	call	get_arg8
	ld	(object_param1),a
	ld	de,return_to_script
	pop	ix
	exx
	push	hl
	exx
	jp	(ix)
;---------------------------------------
;---------------------------------------
;command format
; opcode
; high nibble - number
; low nibble  - attribute
; new value (either 8 or 16 bits)
;e.g.
; enemy[3].gen = d_up;
; set_enemy_attr(3,enemy_gen,d_up)
set_animate_attr8:
set_animate_attr16:
	ld	c,animate_width
	ld	hl,object_array
	jr	do_attr
set_object_attr8:
set_object_attr16:
	ld	c,object_width
	ld	hl,object_array
	jr	do_attr
set_enemy_attr8:
set_enemy_attr16:
	ld	c,enemy_width
	ld	hl,enemy_array
	jr	do_attr
set_misc_attr8:
set_misc_attr16:
	ld	c,misc_width
	ld	hl,misc_array

do_attr:
	exx
	dec	hl
	ld	a,(hl)
	rra 			;8bits are even, 16bits are odd
	ex	af,af'
	xor 	a
	inc	hl
	rld			;puts left nibble in a
	exx
	cp $0F
	jr nz,_
	ld a,(swap)	
_
	or	a
	jr	z,_wmullsk
	ld	b,a
	xor	a
_wmull:
	add	a,c
	djnz	_wmull
_wmullsk:
	ld	c,a
	exx
	xor	a
	rld
	inc	hl
	exx
	add	a,c
	ld	c,a
	ld	b,0
	add	hl,bc		;hl points to byte to modify

	call	get_arg8
	ld	(hl),a
	ex	af,af'
	ret	nc		;ret if even
	call	get_arg8
	inc	hl
	ld	(hl),a
	ret

;---------------------------------------
;---------------------------------------
ll_new_mcom:
	call	get_arg8
	ld	hl,m_com_ctr
	ld	(hl),a
	ex	de,hl
	call	get_arg16
	ex	de,hl
	inc	hl
	ld	(hl),e
	inc	hl
	ld	(hl),d
	ret
;---------------------------------------
;---------------------------------------
goto:
	call get_arg8
	pop de
	jp return_to_script_skip
	
;---------------------------------------
;---------------------------------------
return:
	pop	hl
	ret
	
ex_update:
	exx
	push	hl
	call	great_update
_scr_code_return:
	pop	hl
	exx
	ret
	
;---------------------------------------
;---------------------------------------
; example call: create_enemy(health, flags, x, w, y, h, z ,d);
create_enemy:
	;IMPLEMENTATION DEPENDENT CODE HERE:::
	call find_enemy
	ret nz
	ld (swap),a			;store the last enemy accessed
	push hl
	exx
	pop de
	ld bc,enemy_gen - enemy_health + 1
	ldir
	push hl
	ex de,hl
	ld bc,enemy_x - enemy_com_ctr
	add hl,bc
	push hl
	pop ix
	pcall(return_to_normal)
	pop hl
	exx
	ret
;---------------------------------------
;---------------------------------------
create_object:
	call find_object
	ret nz
	ld (swap),a			;store the last object access
	push hl
	exx
	pop de
	ld bc,object_width
	ldir
	exx
	ret
;---------------------------------------
;---------------------------------------
quake:
	call get_arg8
	ld hl,screen_shake_com
	jp set_m_com

;---------------------------------------
;---------------------------------------
set_player_anim:
	call	get_arg8
	ld	(p_anim_ctr),a
	call	get_arg16
	ld	(p_anim_ptr),hl
	ret
;---------------------------------------
;---------------------------------------
set_player_gen:
	call	get_arg8
	ld	(gen_state),a
	ret
;---------------------------------------
;---------------------------------------
set_overhead_anim:

;---------------------------------------
;---------------------------------------
code:
	call	get_arg16
	exx
	push	hl
	exx
	ld	bc,_scr_code_return	;this is in ex_update
	push	bc
	jp	(hl)
;---------------------------------------
;---------------------------------------
ll_new_pcom:
	call	get_arg8
	ld	(p_com_ctr),a
	call	get_arg16
	ld	(p_com_ptr),hl
	ret
;---------------------------------------
;---------------------------------------
ll_set:
	call	get_arg16
	call	get_arg8
	ld	(hl),a
	ret
;---------------------------------------
;---------------------------------------
transaction:
	call get_arg8
	ld b,a
	call get_arg8
	ld e,a
	call get_arg8
	ld d,a
	
	ld a,(rupees)
	sub b
	daa
	jr c,not_enough_rupees

	ld a,b
	call sub_rupees
	ld a,e
	pop de
	jp return_to_script_skip
	
not_enough_rupees:
	ld a,d
	ld (object_param1),a
	pop de
	ld hl,str_not_enough
	ld de,show_text_script_return
	jp code_text
	
; a - how many rupees to add
add_rupees:
	ld	hl,rupees
	add	a,(hl)
	ld b,a
	daa
	cp b
	jr	nc,no_overflow_r
	ld	a,$99
no_overflow_r:
	ld	(hl),a
	ret
	
sub_rupees:
	ld hl,rupees
	ld b,a
	ld a,(hl)
	sub b
	daa
	ld (hl),a
	ret
;---------------------------------------
;---------------------------------------