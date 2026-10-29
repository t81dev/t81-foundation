	.build_version macos, 26, 0	sdk_version 26, 5
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_main                           ; -- Begin function main
	.p2align	2
_main:                                  ; @main
Lfunc_begin0:
	.cfi_startproc
	.cfi_personality 155, ___gxx_personality_v0
	.cfi_lsda 16, Lexception0
; %bb.0:
	sub	sp, sp, #352
	stp	x28, x27, [sp, #256]            ; 16-byte Folded Spill
	stp	x26, x25, [sp, #272]            ; 16-byte Folded Spill
	stp	x24, x23, [sp, #288]            ; 16-byte Folded Spill
	stp	x22, x21, [sp, #304]            ; 16-byte Folded Spill
	stp	x20, x19, [sp, #320]            ; 16-byte Folded Spill
	stp	x29, x30, [sp, #336]            ; 16-byte Folded Spill
	.cfi_def_cfa_offset 352
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset w25, -72
	.cfi_offset w26, -80
	.cfi_offset w27, -88
	.cfi_offset w28, -96
	mov	x19, x1
	mov	x20, x0
Ltmp0:
	bl	__Z21check_resource_budgetv
Ltmp1:
; %bb.1:
	cmp	w20, #2
	b.lt	LBB0_53
; %bb.2:
	str	xzr, [sp, #32]                  ; 8-byte Folded Spill
	str	wzr, [sp, #40]                  ; 4-byte Folded Spill
	str	wzr, [sp, #56]                  ; 4-byte Folded Spill
	mov	w8, w20
	add	x28, x19, #8
	sub	x23, x8, #1
	mov	x24, #-9                        ; =0xfffffffffffffff7
	movk	x24, #32767, lsl #48
	mov	x26, #11565                     ; =0x2d2d
	movk	x26, #30052, lsl #16
	movk	x26, #28781, lsl #32
	movk	x26, #25389, lsl #48
	mov	x27, #11632                     ; =0x2d70
	movk	x27, #28515, lsl #16
	movk	x27, #28786, lsl #32
	movk	x27, #29557, lsl #48
	mov	w25, #25                        ; =0x19
	b	LBB0_5
LBB0_3:                                 ;   in Loop: Header=BB0_5 Depth=1
	mov	w8, #1                          ; =0x1
	str	w8, [sp, #36]                   ; 4-byte Folded Spill
LBB0_4:                                 ;   in Loop: Header=BB0_5 Depth=1
	subs	x23, x23, #1
	b.eq	LBB0_55
LBB0_5:                                 ; =>This Inner Loop Header: Depth=1
	ldr	x20, [x28], #8
	mov	x0, x20
	bl	_strlen
	cmp	x0, x24
	b.hs	LBB0_140
; %bb.6:                                ;   in Loop: Header=BB0_5 Depth=1
	mov	x19, x0
	cmp	x0, #23
	b.hs	LBB0_16
; %bb.7:                                ;   in Loop: Header=BB0_5 Depth=1
	strb	w19, [sp, #135]
	add	x21, sp, #112
	cbnz	x19, LBB0_18
; %bb.8:                                ;   in Loop: Header=BB0_5 Depth=1
	strb	wzr, [x21, x19]
	ldrsb	w9, [sp, #135]
	and	w8, w9, #0xff
	cmp	w9, #0
	cset	w22, lt
	tbnz	w9, #31, LBB0_19
LBB0_9:                                 ;   in Loop: Header=BB0_5 Depth=1
	cmp	w8, #10
	b.eq	LBB0_24
; %bb.10:                               ;   in Loop: Header=BB0_5 Depth=1
	cmp	w8, #13
	b.ne	LBB0_25
; %bb.11:                               ;   in Loop: Header=BB0_5 Depth=1
	add	x9, sp, #112
	ldr	x10, [x9]
	ldur	x9, [x9, #5]
	cmp	x10, x26
	ccmp	x9, x27, #0, eq
	b.eq	LBB0_23
LBB0_12:                                ;   in Loop: Header=BB0_5 Depth=1
	ldr	x9, [sp, #120]
	tbz	w8, #7, LBB0_14
; %bb.13:                               ;   in Loop: Header=BB0_5 Depth=1
	cmp	x9, #24
	b.eq	LBB0_28
LBB0_14:                                ;   in Loop: Header=BB0_5 Depth=1
	tbnz	w8, #7, LBB0_31
; %bb.15:                               ;   in Loop: Header=BB0_5 Depth=1
	cmp	w8, #11
	b.eq	LBB0_26
	b	LBB0_34
LBB0_16:                                ;   in Loop: Header=BB0_5 Depth=1
	orr	x8, x19, #0x7
	cmp	x8, #23
	csinc	x22, x25, x8, eq
Ltmp2:
	mov	x0, x22
	bl	__Znwm
Ltmp3:
; %bb.17:                               ;   in Loop: Header=BB0_5 Depth=1
	mov	x21, x0
	orr	x8, x22, #0x8000000000000000
	stp	x19, x8, [sp, #120]
	str	x0, [sp, #112]
LBB0_18:                                ;   in Loop: Header=BB0_5 Depth=1
	mov	x0, x21
	mov	x1, x20
	mov	x2, x19
	bl	_memmove
	strb	wzr, [x21, x19]
	ldrsb	w9, [sp, #135]
	and	w8, w9, #0xff
	cmp	w9, #0
	cset	w22, lt
	tbz	w9, #31, LBB0_9
LBB0_19:                                ;   in Loop: Header=BB0_5 Depth=1
	ldr	x9, [sp, #120]
	cmp	x9, #10
	b.ne	LBB0_21
; %bb.20:                               ;   in Loop: Header=BB0_5 Depth=1
	ldr	x10, [sp, #112]
	ldr	x11, [x10]
	ldrh	w10, [x10, #8]
	mov	x12, #11565                     ; =0x2d2d
	movk	x12, #25971, lsl #16
	movk	x12, #26220, lsl #32
	movk	x12, #25972, lsl #48
	cmp	x11, x12
	mov	w11, #29811                     ; =0x7473
	ccmp	w10, w11, #0, eq
	b.eq	LBB0_48
LBB0_21:                                ;   in Loop: Header=BB0_5 Depth=1
	cmp	x9, #13
	b.ne	LBB0_27
; %bb.22:                               ;   in Loop: Header=BB0_5 Depth=1
	ldr	x9, [sp, #112]
	ldr	x10, [x9]
	ldur	x9, [x9, #5]
	cmp	x10, x26
	ccmp	x9, x27, #0, eq
	b.ne	LBB0_12
LBB0_23:                                ;   in Loop: Header=BB0_5 Depth=1
	mov	w9, #1                          ; =0x1
	str	w9, [sp, #40]                   ; 4-byte Folded Spill
	b	LBB0_51
LBB0_24:                                ;   in Loop: Header=BB0_5 Depth=1
	ldr	x9, [sp, #112]
	ldrh	w10, [sp, #120]
	mov	x11, #11565                     ; =0x2d2d
	movk	x11, #25971, lsl #16
	movk	x11, #26220, lsl #32
	movk	x11, #25972, lsl #48
	cmp	x9, x11
	mov	w9, #29811                      ; =0x7473
	ccmp	w10, w9, #0, eq
	b.eq	LBB0_3
LBB0_25:                                ;   in Loop: Header=BB0_5 Depth=1
	ldr	x9, [sp, #120]
	cmp	w8, #11
	b.ne	LBB0_34
LBB0_26:                                ;   in Loop: Header=BB0_5 Depth=1
	add	x10, sp, #112
	b	LBB0_33
LBB0_27:                                ;   in Loop: Header=BB0_5 Depth=1
	ldr	x9, [sp, #120]
	cmp	x9, #24
	b.ne	LBB0_31
LBB0_28:                                ;   in Loop: Header=BB0_5 Depth=1
	ldr	x9, [sp, #112]
	ldp	x10, x11, [x9]
	ldr	x12, [x9, #16]
	mov	x13, #11565                     ; =0x2d2d
	movk	x13, #30052, lsl #16
	movk	x13, #28781, lsl #32
	movk	x13, #25133, lsl #48
	cmp	x10, x13
	mov	x10, #28527                     ; =0x6f6f
	movk	x10, #29556, lsl #16
	movk	x10, #29300, lsl #32
	movk	x10, #28769, lsl #48
	ccmp	x11, x10, #0, eq
	mov	x10, #26925                     ; =0x692d
	movk	x10, #25710, lsl #16
	movk	x10, #25449, lsl #32
	movk	x10, #29541, lsl #48
	ccmp	x12, x10, #0, eq
	b.eq	LBB0_47
; %bb.29:                               ;   in Loop: Header=BB0_5 Depth=1
	ldp	x10, x11, [x9]
	ldr	x9, [x9, #16]
	mov	x12, #11565                     ; =0x2d2d
	movk	x12, #25972, lsl #16
	movk	x12, #29811, lsl #32
	movk	x12, #26157, lsl #48
	cmp	x10, x12
	mov	x10, #30049                     ; =0x7561
	movk	x10, #29804, lsl #16
	movk	x10, #25389, lsl #32
	movk	x10, #29295, lsl #48
	ccmp	x11, x10, #0, eq
	mov	x10, #25970                     ; =0x6572
	movk	x10, #29795, lsl #16
	movk	x10, #25966, lsl #32
	movk	x10, #29555, lsl #48
	ccmp	x9, x10, #0, eq
	b.eq	LBB0_49
; %bb.30:                               ;   in Loop: Header=BB0_5 Depth=1
	mov	w9, #24                         ; =0x18
	b	LBB0_34
LBB0_31:                                ;   in Loop: Header=BB0_5 Depth=1
	cmp	x9, #11
	b.ne	LBB0_38
; %bb.32:                               ;   in Loop: Header=BB0_5 Depth=1
	ldr	x10, [sp, #112]
LBB0_33:                                ;   in Loop: Header=BB0_5 Depth=1
	ldr	x11, [x10]
	ldur	x10, [x10, #3]
	mov	x12, #11565                     ; =0x2d2d
	movk	x12, #29296, lsl #16
	movk	x12, #25199, lsl #32
	movk	x12, #11621, lsl #48
	cmp	x11, x12
	mov	x11, #28530                     ; =0x6f72
	movk	x11, #25954, lsl #16
	movk	x11, #28717, lsl #32
	movk	x11, #30061, lsl #48
	ccmp	x10, x11, #0, eq
	b.eq	LBB0_46
LBB0_34:                                ;   in Loop: Header=BB0_5 Depth=1
	tbnz	w8, #7, LBB0_38
; %bb.35:                               ;   in Loop: Header=BB0_5 Depth=1
	cmp	w8, #7
	b.eq	LBB0_42
; %bb.36:                               ;   in Loop: Header=BB0_5 Depth=1
	cmp	w8, #20
	b.ne	LBB0_141
; %bb.37:                               ;   in Loop: Header=BB0_5 Depth=1
	ldp	x9, x10, [sp, #112]
	ldr	w11, [sp, #128]
	mov	x12, #11565                     ; =0x2d2d
	movk	x12, #25972, lsl #16
	movk	x12, #29811, lsl #32
	movk	x12, #26157, lsl #48
	cmp	x9, x12
	mov	x9, #30049                      ; =0x7561
	movk	x9, #29804, lsl #16
	movk	x9, #28717, lsl #32
	movk	x9, #31073, lsl #48
	ccmp	x10, x9, #0, eq
	mov	w9, #28524                      ; =0x6f6c
	movk	w9, #25697, lsl #16
	ccmp	x11, x9, #0, eq
	b.eq	LBB0_43
	b	LBB0_141
LBB0_38:                                ;   in Loop: Header=BB0_5 Depth=1
	cmp	x9, #7
	b.eq	LBB0_44
; %bb.39:                               ;   in Loop: Header=BB0_5 Depth=1
	cmp	x9, #20
	b.ne	LBB0_142
; %bb.40:                               ;   in Loop: Header=BB0_5 Depth=1
	ldr	x9, [sp, #112]
	ldp	x10, x11, [x9]
	ldr	w9, [x9, #16]
	mov	x12, #11565                     ; =0x2d2d
	movk	x12, #25972, lsl #16
	movk	x12, #29811, lsl #32
	movk	x12, #26157, lsl #48
	cmp	x10, x12
	mov	x10, #30049                     ; =0x7561
	movk	x10, #29804, lsl #16
	movk	x10, #28717, lsl #32
	movk	x10, #31073, lsl #48
	ccmp	x11, x10, #0, eq
	mov	w10, #28524                     ; =0x6f6c
	movk	w10, #25697, lsl #16
	ccmp	x9, x10, #0, eq
	b.eq	LBB0_43
; %bb.41:                               ;   in Loop: Header=BB0_5 Depth=1
	cmp	w8, #7
	b.ne	LBB0_142
LBB0_42:                                ;   in Loop: Header=BB0_5 Depth=1
	add	x9, sp, #112
	b	LBB0_45
LBB0_43:                                ;   in Loop: Header=BB0_5 Depth=1
	mov	w9, #2                          ; =0x2
	b	LBB0_50
LBB0_44:                                ;   in Loop: Header=BB0_5 Depth=1
	ldr	x9, [sp, #112]
LBB0_45:                                ;   in Loop: Header=BB0_5 Depth=1
	ldr	w10, [x9]
	ldur	w9, [x9, #3]
	mov	w11, #11565                     ; =0x2d2d
	movk	w11, #28019, lsl #16
	cmp	w10, w11
	mov	w10, #28525                     ; =0x6f6d
	movk	w10, #25963, lsl #16
	ccmp	w9, w10, #0, eq
	b.eq	LBB0_51
	b	LBB0_142
LBB0_46:                                ;   in Loop: Header=BB0_5 Depth=1
	mov	w9, #1                          ; =0x1
	str	w9, [sp, #32]                   ; 4-byte Folded Spill
	b	LBB0_51
LBB0_47:                                ;   in Loop: Header=BB0_5 Depth=1
	mov	w8, #1                          ; =0x1
	str	w8, [sp, #56]                   ; 4-byte Folded Spill
	b	LBB0_52
LBB0_48:                                ;   in Loop: Header=BB0_5 Depth=1
	mov	w9, #1                          ; =0x1
	str	w9, [sp, #36]                   ; 4-byte Folded Spill
	b	LBB0_51
LBB0_49:                                ;   in Loop: Header=BB0_5 Depth=1
	mov	w9, #1                          ; =0x1
LBB0_50:                                ;   in Loop: Header=BB0_5 Depth=1
	adrp	x10, __MergedGlobals@PAGE
	str	w9, [x10, __MergedGlobals@PAGEOFF]
LBB0_51:                                ;   in Loop: Header=BB0_5 Depth=1
	tbz	w8, #7, LBB0_4
LBB0_52:                                ;   in Loop: Header=BB0_5 Depth=1
	ldr	x0, [sp, #112]
	bl	__ZdlPv
	b	LBB0_4
LBB0_53:
Lloh0:
	adrp	x8, __MergedGlobals@PAGE
Lloh1:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF]
	cbnz	w8, LBB0_144
; %bb.54:
	str	xzr, [sp, #32]                  ; 8-byte Folded Spill
	str	wzr, [sp, #40]                  ; 4-byte Folded Spill
	b	LBB0_60
LBB0_55:
Lloh2:
	adrp	x8, __MergedGlobals@PAGE
Lloh3:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF]
	cmp	w8, #0
	cset	w8, eq
	ldr	w9, [sp, #36]                   ; 4-byte Folded Reload
	orr	w8, w8, w9
	tbz	w8, #0, LBB0_144
; %bb.56:
	ldr	w8, [sp, #56]                   ; 4-byte Folded Reload
	tbz	w8, #0, LBB0_60
; %bb.57:
	mov	w19, #0                         ; =0x0
	mov	x20, #21417                     ; =0x53a9
	movk	x20, #22027, lsl #16
	movk	x20, #50449, lsl #32
	movk	x20, #17285, lsl #48
	mov	w21, #21417                     ; =0x53a9
	movk	w21, #22027, lsl #16
	mov	x22, #1527                      ; =0x5f7
	movk	x22, #17640, lsl #16
	movk	x22, #48262, lsl #32
	movk	x22, #52896, lsl #48
	mov	x25, #22359                     ; =0x5757
	movk	x25, #14592, lsl #16
	movk	x25, #38627, lsl #32
	movk	x25, #62820, lsl #48
	mov	w23, #50449                     ; =0xc511
	movk	w23, #17285, lsl #16
	mov	x24, #23972                     ; =0x5da4
	movk	x24, #8134, lsl #16
	movk	x24, #60677, lsl #32
	movk	x24, #9056, lsl #48
	mov	x26, #-3689348814741910324      ; =0xcccccccccccccccc
	movk	x26, #52429
	mov	w27, #10                        ; =0xa
	mov	w28, #34464                     ; =0x86a0
	movk	w28, #1, lsl #16
LBB0_58:                                ; =>This Inner Loop Header: Depth=1
	eor	x8, x22, x25
	lsr	x9, x22, #58
	mul	x10, x25, x20
	lsr	x11, x25, #32
	umull	x12, w25, w21
	lsr	x12, x12, #32
	umaddl	x12, w11, w21, x12
	umull	x13, w25, w23
	add	x13, x13, w12, uxtw
	mul	x14, x25, x24
	cmn	x10, #4
	madd	x14, x22, x20, x14
	umaddl	x11, w11, w23, x14
	cinc	x11, x11, hi
	add	x11, x11, x12, lsr #32
	add	x22, x11, x13, lsr #32
	add	x25, x10, #3
	ror	x10, x8, x9
	cmp	x9, #0
	csel	x8, x8, x10, eq
	cmp	x8, #6
	b.lo	LBB0_58
; %bb.59:                               ;   in Loop: Header=BB0_58 Depth=1
	umulh	x9, x8, x26
	lsr	x9, x9, #3
	msub	w0, w9, w27, w8
	bl	_putchar
	add	w19, w19, #1
	cmp	w19, w28
	b.ne	LBB0_58
	b	LBB0_133
LBB0_60:
	mov	w23, #0                         ; =0x0
	mov	x26, #30787                     ; =0x7843
	movk	x26, #29108, lsl #16
	movk	x26, #23236, lsl #32
	movk	x26, #2684, lsl #48
	mov	w27, #34464                     ; =0x86a0
	movk	w27, #1, lsl #16
	b	LBB0_62
LBB0_61:                                ;   in Loop: Header=BB0_62 Depth=1
	add	w23, w23, #1
	cmp	w23, #3
	b.eq	LBB0_113
LBB0_62:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_94 Depth 2
                                        ;     Child Loop BB0_100 Depth 2
                                        ;       Child Loop BB0_102 Depth 3
                                        ;     Child Loop BB0_108 Depth 2
Ltmp17:
	mov	x0, #0                          ; =0x0
	mov	x1, #0                          ; =0x0
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp18:
; %bb.63:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp19:
	mov	w0, #42                         ; =0x2a
	mov	w1, #42                         ; =0x2a
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp20:
; %bb.64:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp21:
	mov	x0, #-42                        ; =0xffffffffffffffd6
	mov	x1, #-42                        ; =0xffffffffffffffd6
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp22:
; %bb.65:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp23:
	mov	x0, #9223372036854775807        ; =0x7fffffffffffffff
	mov	x1, #9223372036854775807        ; =0x7fffffffffffffff
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp24:
; %bb.66:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp25:
	mov	x0, #-9223372036854775808       ; =0x8000000000000000
	mov	x1, #-9223372036854775808       ; =0x8000000000000000
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp26:
; %bb.67:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp27:
	mov	x0, #-9223372036854775808       ; =0x8000000000000000
	mov	x1, #9223372036854775807        ; =0x7fffffffffffffff
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp28:
; %bb.68:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp29:
	mov	x0, #9223372036854775807        ; =0x7fffffffffffffff
	mov	x1, #-9223372036854775808       ; =0x8000000000000000
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp30:
; %bb.69:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp31:
	mov	w0, #100                        ; =0x64
	mov	w1, #101                        ; =0x65
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp32:
; %bb.70:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp33:
	mov	w0, #101                        ; =0x65
	mov	w1, #100                        ; =0x64
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp34:
; %bb.71:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp35:
	mov	x0, #-101                       ; =0xffffffffffffff9b
	mov	x1, #-100                       ; =0xffffffffffffff9c
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp36:
; %bb.72:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp37:
	mov	x0, #-100                       ; =0xffffffffffffff9c
	mov	x1, #-101                       ; =0xffffffffffffff9b
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp38:
; %bb.73:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp39:
	mov	x0, #-1                         ; =0xffffffffffffffff
	mov	x1, #0                          ; =0x0
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp40:
; %bb.74:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp41:
	mov	x0, #0                          ; =0x0
	mov	x1, #-1                         ; =0xffffffffffffffff
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp42:
; %bb.75:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp43:
	mov	x0, #0                          ; =0x0
	mov	w1, #1                          ; =0x1
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp44:
; %bb.76:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp45:
	mov	w0, #1                          ; =0x1
	mov	x1, #0                          ; =0x0
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp46:
; %bb.77:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp47:
	mov	x0, #-1                         ; =0xffffffffffffffff
	mov	w1, #1                          ; =0x1
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp48:
; %bb.78:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp49:
	mov	w0, #1                          ; =0x1
	mov	x1, #-1                         ; =0xffffffffffffffff
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp50:
; %bb.79:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp51:
	mov	x0, #1                          ; =0x1
	movk	x0, #32, lsl #48
	mov	x1, #2                          ; =0x2
	movk	x1, #32, lsl #48
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp52:
; %bb.80:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp53:
	mov	x0, #2                          ; =0x2
	movk	x0, #32, lsl #48
	mov	x1, #1                          ; =0x1
	movk	x1, #32, lsl #48
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp54:
; %bb.81:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp55:
	mov	x0, #9223372036854775806        ; =0x7ffffffffffffffe
	mov	x1, #9223372036854775807        ; =0x7fffffffffffffff
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp56:
; %bb.82:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp58:
	mov	x0, #-9223372036854775808       ; =0x8000000000000000
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp59:
; %bb.83:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp60:
	mov	x0, #-101                       ; =0xffffffffffffff9b
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp61:
; %bb.84:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp62:
	mov	x0, #-100                       ; =0xffffffffffffff9c
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp63:
; %bb.85:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp64:
	mov	x0, #-99                        ; =0xffffffffffffff9d
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp65:
; %bb.86:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp66:
	mov	w0, #99                         ; =0x63
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp67:
; %bb.87:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp68:
	mov	w0, #100                        ; =0x64
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp69:
; %bb.88:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp70:
	mov	w0, #101                        ; =0x65
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp71:
; %bb.89:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp72:
	mov	x0, #9223372036854775807        ; =0x7fffffffffffffff
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp73:
; %bb.90:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp75:
	add	x0, sp, #88
	mov	x1, x23
	bl	__ZL15generate_corpusim
Ltmp76:
; %bb.91:                               ;   in Loop: Header=BB0_62 Depth=1
Ltmp78:
	add	x0, sp, #64
	mov	x1, x23
	bl	__ZL15generate_corpusim
Ltmp79:
; %bb.92:                               ;   in Loop: Header=BB0_62 Depth=1
	ldp	x24, x21, [sp, #88]
	subs	x8, x21, x24
	str	w23, [sp, #44]                  ; 4-byte Folded Spill
	b.eq	LBB0_107
; %bb.93:                               ;   in Loop: Header=BB0_62 Depth=1
	asr	x19, x8, #4
	ldr	x8, [sp, #64]
	add	x20, x24, #8
	add	x22, x8, #8
LBB0_94:                                ;   Parent Loop BB0_62 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldur	x0, [x20, #-8]
	ldur	x8, [x22, #-8]
	cmp	x0, x8
	b.ne	LBB0_138
; %bb.95:                               ;   in Loop: Header=BB0_94 Depth=2
	ldr	x1, [x20], #16
	ldr	x8, [x22], #16
	cmp	x1, x8
	b.ne	LBB0_138
; %bb.96:                               ;   in Loop: Header=BB0_94 Depth=2
Ltmp87:
	mov	x2, x23
	bl	__ZL6verify4Pairi
Ltmp88:
; %bb.97:                               ;   in Loop: Header=BB0_94 Depth=2
	subs	x19, x19, #1
	b.ne	LBB0_94
; %bb.98:                               ;   in Loop: Header=BB0_62 Depth=1
	cmp	w23, #1
Lloh4:
	adrp	x8, __ZL5modes@PAGE
Lloh5:
	add	x8, x8, __ZL5modes@PAGEOFF
Lloh6:
	adrp	x9, __ZL12policy_modes@PAGE
Lloh7:
	add	x9, x9, __ZL12policy_modes@PAGEOFF
	csel	x25, x9, x8, eq
	add	x8, x8, #40
	add	x9, x9, #40
	csel	x8, x9, x8, eq
	stp	x8, x24, [sp, #48]              ; 16-byte Folded Spill
	cmp	x25, x8
	b.ne	LBB0_100
	b	LBB0_109
LBB0_99:                                ;   in Loop: Header=BB0_100 Depth=2
	adrp	x8, __MergedGlobals@PAGE+8
	str	x20, [x8, __MergedGlobals@PAGEOFF+8]
	add	x25, x25, #8
	ldp	x8, x24, [sp, #48]              ; 16-byte Folded Reload
	cmp	x25, x8
	b.eq	LBB0_109
LBB0_100:                               ;   Parent Loop BB0_62 Depth=1
                                        ; =>  This Loop Header: Depth=2
                                        ;       Child Loop BB0_102 Depth 3
	mov	x20, #0                         ; =0x0
	ldr	x19, [x25]
	mov	x22, #-1                        ; =0xffffffffffffffff
	mov	w28, #1                         ; =0x1
	mov	x23, x24
	b	LBB0_102
LBB0_101:                               ;   in Loop: Header=BB0_102 Depth=3
	add	x20, x0, x20
	add	x23, x23, #16
	sub	x22, x22, #1
	add	x28, x28, #1
	cmp	x23, x21
	b.eq	LBB0_99
LBB0_102:                               ;   Parent Loop BB0_62 Depth=1
                                        ;     Parent Loop BB0_100 Depth=2
                                        ; =>    This Inner Loop Header: Depth=3
	ldp	x0, x1, [x23]
Ltmp90:
	blr	x19
Ltmp91:
; %bb.103:                              ;   in Loop: Header=BB0_102 Depth=3
	lsr	x8, x28, #5
	umulh	x8, x8, x26
	lsr	x8, x8, #7
	madd	x8, x8, x27, x22
	cbnz	x8, LBB0_101
; %bb.104:                              ;   in Loop: Header=BB0_102 Depth=3
	movi.2d	v0, #0000000000000000
	stp	q0, q0, [sp, #224]
	stp	q0, q0, [sp, #192]
	stp	q0, q0, [sp, #160]
	stp	q0, q0, [sp, #128]
	str	q0, [sp, #112]
Ltmp92:
	add	x1, sp, #112
	mov	x24, x0
	mov	w0, #0                          ; =0x0
	bl	_getrusage
	mov	x8, x0
	mov	x0, x24
Ltmp93:
; %bb.105:                              ;   in Loop: Header=BB0_102 Depth=3
	cbnz	w8, LBB0_136
; %bb.106:                              ;   in Loop: Header=BB0_102 Depth=3
	ldr	x8, [sp, #112]
	ldrsw	x9, [sp, #120]
	ldr	x10, [sp, #128]
	ldrsw	x11, [sp, #136]
	ldr	x12, [sp, #144]
	add	x8, x10, x8
	add	x9, x11, x9
	mov	w10, #16960                     ; =0x4240
	movk	w10, #15, lsl #16
	madd	x8, x8, x10, x9
	mov	w9, #17920                      ; =0x4600
	movk	w9, #9155, lsl #16
	cmp	x8, x9
	mov	w8, #25857                      ; =0x6501
	movk	w8, #7629, lsl #16
	ccmp	x12, x8, #2, ls
	b.lo	LBB0_101
	b	LBB0_135
LBB0_107:                               ;   in Loop: Header=BB0_62 Depth=1
	cmp	w23, #1
Lloh8:
	adrp	x9, __ZL5modes@PAGE
Lloh9:
	add	x9, x9, __ZL5modes@PAGEOFF
Lloh10:
	adrp	x10, __ZL12policy_modes@PAGE
Lloh11:
	add	x10, x10, __ZL12policy_modes@PAGEOFF
	csel	x8, x10, x9, eq
	add	x9, x9, #40
	add	x10, x10, #40
	csel	x9, x10, x9, eq
	cmp	x8, x9
	b.eq	LBB0_109
LBB0_108:                               ;   Parent Loop BB0_62 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	adrp	x10, __MergedGlobals@PAGE+8
	str	xzr, [x10, __MergedGlobals@PAGEOFF+8]
	add	x8, x8, #8
	cmp	x8, x9
	b.ne	LBB0_108
LBB0_109:                               ;   in Loop: Header=BB0_62 Depth=1
	ldr	x0, [sp, #64]
	cbz	x0, LBB0_111
; %bb.110:                              ;   in Loop: Header=BB0_62 Depth=1
	bl	__ZdlPv
LBB0_111:                               ;   in Loop: Header=BB0_62 Depth=1
	ldr	w23, [sp, #44]                  ; 4-byte Folded Reload
	cbz	x24, LBB0_61
; %bb.112:                              ;   in Loop: Header=BB0_62 Depth=1
	mov	x0, x24
	bl	__ZdlPv
	b	LBB0_61
LBB0_113:
Lloh12:
	adrp	x8, ___stderrp@GOTPAGE
Lloh13:
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
Lloh14:
	ldr	x3, [x8]
Lloh15:
	adrp	x0, l_.str.12@PAGE
Lloh16:
	add	x0, x0, l_.str.12@PAGEOFF
	mov	w1, #70                         ; =0x46
	mov	w2, #1                          ; =0x1
	bl	_fwrite
	ldr	w8, [sp, #40]                   ; 4-byte Folded Reload
	tbz	w8, #0, LBB0_132
; %bb.114:
Ltmp116:
	add	x0, sp, #112
	mov	w1, #0                          ; =0x0
	bl	__ZL15generate_corpusim
Ltmp117:
; %bb.115:
	ldp	x19, x21, [sp, #112]
	cmp	x19, x21
	b.eq	LBB0_118
; %bb.116:
Lloh17:
	adrp	x20, l_.str.13@PAGE
Lloh18:
	add	x20, x20, l_.str.13@PAGEOFF
	mov	x22, x19
LBB0_117:                               ; =>This Inner Loop Header: Depth=1
	ldp	x8, x9, [x22], #16
	stp	x8, x9, [sp, #8]
	str	xzr, [sp]
	mov	x0, x20
	bl	_printf
	cmp	x22, x21
	b.ne	LBB0_117
LBB0_118:
	cbz	x19, LBB0_120
; %bb.119:
	mov	x0, x19
	bl	__ZdlPv
LBB0_120:
Ltmp118:
	add	x0, sp, #112
	mov	w1, #1                          ; =0x1
	bl	__ZL15generate_corpusim
Ltmp119:
; %bb.121:
	ldp	x19, x21, [sp, #112]
	cmp	x19, x21
	b.eq	LBB0_124
; %bb.122:
	mov	w22, #1                         ; =0x1
Lloh19:
	adrp	x20, l_.str.13@PAGE
Lloh20:
	add	x20, x20, l_.str.13@PAGEOFF
	mov	x23, x19
LBB0_123:                               ; =>This Inner Loop Header: Depth=1
	ldp	x8, x9, [x23], #16
	stp	x8, x9, [sp, #8]
	str	x22, [sp]
	mov	x0, x20
	bl	_printf
	cmp	x23, x21
	b.ne	LBB0_123
LBB0_124:
	cbz	x19, LBB0_126
; %bb.125:
	mov	x0, x19
	bl	__ZdlPv
LBB0_126:
Ltmp120:
	add	x0, sp, #112
	mov	w1, #2                          ; =0x2
	bl	__ZL15generate_corpusim
Ltmp121:
; %bb.127:
	ldp	x19, x21, [sp, #112]
	cmp	x19, x21
	b.eq	LBB0_130
; %bb.128:
	mov	w22, #2                         ; =0x2
Lloh21:
	adrp	x20, l_.str.13@PAGE
Lloh22:
	add	x20, x20, l_.str.13@PAGEOFF
	mov	x23, x19
LBB0_129:                               ; =>This Inner Loop Header: Depth=1
	ldp	x8, x9, [x23], #16
	stp	x8, x9, [sp, #8]
	str	x22, [sp]
	mov	x0, x20
	bl	_printf
	cmp	x23, x21
	b.ne	LBB0_129
LBB0_130:
	cbz	x19, LBB0_133
; %bb.131:
	mov	x0, x19
	bl	__ZdlPv
	b	LBB0_133
LBB0_132:
	ldr	w8, [sp, #36]                   ; 4-byte Folded Reload
	tbz	w8, #0, LBB0_146
LBB0_133:
	mov	w0, #0                          ; =0x0
LBB0_134:
	ldp	x29, x30, [sp, #336]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #320]            ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #304]            ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #288]            ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #272]            ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #256]            ; 16-byte Folded Reload
	add	sp, sp, #352
	ret
LBB0_135:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x22, x0
Ltmp98:
Lloh23:
	adrp	x1, l_.str.18@PAGE
Lloh24:
	add	x1, x1, l_.str.18@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp99:
	b	LBB0_137
LBB0_136:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x22, x0
Ltmp95:
Lloh25:
	adrp	x1, l_.str.17@PAGE
Lloh26:
	add	x1, x1, l_.str.17@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp96:
LBB0_137:
Ltmp101:
Lloh27:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
Lloh28:
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
Lloh29:
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
Lloh30:
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	mov	x0, x22
	ldr	x24, [sp, #56]                  ; 8-byte Folded Reload
	bl	___cxa_throw
Ltmp102:
	b	LBB0_151
LBB0_138:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x22, x0
Ltmp81:
Lloh31:
	adrp	x1, l_.str.11@PAGE
Lloh32:
	add	x1, x1, l_.str.11@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp82:
; %bb.139:
Ltmp84:
Lloh33:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
Lloh34:
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
Lloh35:
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
Lloh36:
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	mov	x0, x22
	bl	___cxa_throw
Ltmp85:
	b	LBB0_151
LBB0_140:
Ltmp123:
	bl	__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB9nqe210106Ev
Ltmp124:
	b	LBB0_151
LBB0_141:
	mov	w22, #0                         ; =0x0
LBB0_142:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x20, x0
Ltmp5:
Lloh37:
	adrp	x1, l_.str.7@PAGE
Lloh38:
	add	x1, x1, l_.str.7@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp6:
; %bb.143:
Ltmp8:
Lloh39:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
Lloh40:
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
Lloh41:
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
Lloh42:
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	mov	x0, x20
	bl	___cxa_throw
Ltmp9:
	b	LBB0_151
LBB0_144:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x20, x0
Ltmp11:
Lloh43:
	adrp	x1, l_.str.8@PAGE
Lloh44:
	add	x1, x1, l_.str.8@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp12:
; %bb.145:
Ltmp14:
Lloh45:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
Lloh46:
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
Lloh47:
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
Lloh48:
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	mov	x0, x20
	bl	___cxa_throw
Ltmp15:
	b	LBB0_151
LBB0_146:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x20, x0
	ldr	w8, [sp, #32]                   ; 4-byte Folded Reload
	tbz	w8, #0, LBB0_149
; %bb.147:
Ltmp110:
Lloh49:
	adrp	x1, l_.str.15@PAGE
Lloh50:
	add	x1, x1, l_.str.15@PAGEOFF
	mov	x0, x20
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp111:
; %bb.148:
Ltmp113:
Lloh51:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
Lloh52:
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
Lloh53:
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
Lloh54:
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	mov	x0, x20
	bl	___cxa_throw
Ltmp114:
	b	LBB0_151
LBB0_149:
Ltmp104:
Lloh55:
	adrp	x1, l_.str.14@PAGE
Lloh56:
	add	x1, x1, l_.str.14@PAGEOFF
	mov	x0, x20
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp105:
; %bb.150:
Ltmp107:
Lloh57:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
Lloh58:
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
Lloh59:
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
Lloh60:
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	mov	x0, x20
	bl	___cxa_throw
Ltmp108:
LBB0_151:
	brk	#0x1
LBB0_152:
Ltmp109:
	b	LBB0_173
LBB0_153:
Ltmp106:
	b	LBB0_157
LBB0_154:
Ltmp115:
	b	LBB0_173
LBB0_155:
Ltmp112:
	b	LBB0_157
LBB0_156:
Ltmp13:
LBB0_157:
	mov	x19, x1
	mov	x21, x0
	mov	x0, x20
	bl	___cxa_free_exception
	b	LBB0_186
LBB0_158:
Ltmp122:
	b	LBB0_173
LBB0_159:
Ltmp16:
	b	LBB0_173
LBB0_160:
Ltmp10:
	mov	x19, x1
	mov	x21, x0
	b	LBB0_162
LBB0_161:
Ltmp7:
	mov	x19, x1
	mov	x21, x0
	mov	x0, x20
	bl	___cxa_free_exception
LBB0_162:
	cbz	w22, LBB0_186
; %bb.163:
	ldr	x0, [sp, #112]
	b	LBB0_185
LBB0_164:
Ltmp4:
	b	LBB0_173
LBB0_165:
Ltmp125:
	b	LBB0_173
LBB0_166:
Ltmp80:
	mov	x19, x1
	mov	x21, x0
	ldr	x24, [sp, #88]
	b	LBB0_183
LBB0_167:
Ltmp77:
	b	LBB0_173
LBB0_168:
Ltmp74:
	b	LBB0_173
LBB0_169:
Ltmp89:
	b	LBB0_178
LBB0_170:
Ltmp86:
	b	LBB0_178
LBB0_171:
Ltmp83:
	mov	x19, x1
	mov	x21, x0
	mov	x0, x22
	bl	___cxa_free_exception
	b	LBB0_181
LBB0_172:
Ltmp57:
LBB0_173:
	mov	x19, x1
	mov	x21, x0
	b	LBB0_186
LBB0_174:
Ltmp97:
	b	LBB0_176
LBB0_175:
Ltmp100:
LBB0_176:
	mov	x19, x1
	mov	x21, x0
	mov	x0, x22
	bl	___cxa_free_exception
	b	LBB0_180
LBB0_177:
Ltmp103:
LBB0_178:
	mov	x19, x1
	mov	x21, x0
	b	LBB0_181
LBB0_179:
Ltmp94:
	mov	x19, x1
	mov	x21, x0
LBB0_180:
	ldr	x24, [sp, #56]                  ; 8-byte Folded Reload
LBB0_181:
	ldr	x0, [sp, #64]
	cbz	x0, LBB0_183
; %bb.182:
	bl	__ZdlPv
LBB0_183:
	cbz	x24, LBB0_186
; %bb.184:
	mov	x0, x24
LBB0_185:
	bl	__ZdlPv
LBB0_186:
	mov	x0, x21
	cmp	w19, #1
	b.ne	LBB0_188
; %bb.187:
	bl	___cxa_begin_catch
Lloh61:
	adrp	x8, ___stderrp@GOTPAGE
Lloh62:
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
Lloh63:
	ldr	x19, [x8]
	ldr	x8, [x0]
	ldr	x8, [x8, #16]
	blr	x8
	str	x0, [sp]
Lloh64:
	adrp	x1, l_.str.16@PAGE
Lloh65:
	add	x1, x1, l_.str.16@PAGEOFF
	mov	x0, x19
	bl	_fprintf
	bl	___cxa_end_catch
	mov	w0, #6                          ; =0x6
	b	LBB0_134
LBB0_188:
	bl	__Unwind_Resume
	.loh AdrpLdr	Lloh0, Lloh1
	.loh AdrpLdr	Lloh2, Lloh3
	.loh AdrpAdd	Lloh6, Lloh7
	.loh AdrpAdd	Lloh4, Lloh5
	.loh AdrpAdd	Lloh10, Lloh11
	.loh AdrpAdd	Lloh8, Lloh9
	.loh AdrpAdd	Lloh15, Lloh16
	.loh AdrpLdrGotLdr	Lloh12, Lloh13, Lloh14
	.loh AdrpAdd	Lloh17, Lloh18
	.loh AdrpAdd	Lloh19, Lloh20
	.loh AdrpAdd	Lloh21, Lloh22
	.loh AdrpAdd	Lloh23, Lloh24
	.loh AdrpAdd	Lloh25, Lloh26
	.loh AdrpLdrGot	Lloh29, Lloh30
	.loh AdrpLdrGot	Lloh27, Lloh28
	.loh AdrpAdd	Lloh31, Lloh32
	.loh AdrpLdrGot	Lloh35, Lloh36
	.loh AdrpLdrGot	Lloh33, Lloh34
	.loh AdrpAdd	Lloh37, Lloh38
	.loh AdrpLdrGot	Lloh41, Lloh42
	.loh AdrpLdrGot	Lloh39, Lloh40
	.loh AdrpAdd	Lloh43, Lloh44
	.loh AdrpLdrGot	Lloh47, Lloh48
	.loh AdrpLdrGot	Lloh45, Lloh46
	.loh AdrpAdd	Lloh49, Lloh50
	.loh AdrpLdrGot	Lloh53, Lloh54
	.loh AdrpLdrGot	Lloh51, Lloh52
	.loh AdrpAdd	Lloh55, Lloh56
	.loh AdrpLdrGot	Lloh59, Lloh60
	.loh AdrpLdrGot	Lloh57, Lloh58
	.loh AdrpAdd	Lloh64, Lloh65
	.loh AdrpLdrGotLdr	Lloh61, Lloh62, Lloh63
Lfunc_end0:
	.cfi_endproc
	.section	__TEXT,__gcc_except_tab
	.p2align	2, 0x0
GCC_except_table0:
Lexception0:
	.byte	255                             ; @LPStart Encoding = omit
	.byte	155                             ; @TType Encoding = indirect pcrel sdata4
	.uleb128 Lttbase0-Lttbaseref0
Lttbaseref0:
	.byte	1                               ; Call site Encoding = uleb128
	.uleb128 Lcst_end0-Lcst_begin0
Lcst_begin0:
	.uleb128 Ltmp0-Lfunc_begin0             ; >> Call Site 1 <<
	.uleb128 Ltmp1-Ltmp0                    ;   Call between Ltmp0 and Ltmp1
	.uleb128 Ltmp16-Lfunc_begin0            ;     jumps to Ltmp16
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp2-Lfunc_begin0             ; >> Call Site 2 <<
	.uleb128 Ltmp3-Ltmp2                    ;   Call between Ltmp2 and Ltmp3
	.uleb128 Ltmp4-Lfunc_begin0             ;     jumps to Ltmp4
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp3-Lfunc_begin0             ; >> Call Site 3 <<
	.uleb128 Ltmp17-Ltmp3                   ;   Call between Ltmp3 and Ltmp17
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp17-Lfunc_begin0            ; >> Call Site 4 <<
	.uleb128 Ltmp56-Ltmp17                  ;   Call between Ltmp17 and Ltmp56
	.uleb128 Ltmp57-Lfunc_begin0            ;     jumps to Ltmp57
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp58-Lfunc_begin0            ; >> Call Site 5 <<
	.uleb128 Ltmp73-Ltmp58                  ;   Call between Ltmp58 and Ltmp73
	.uleb128 Ltmp74-Lfunc_begin0            ;     jumps to Ltmp74
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp75-Lfunc_begin0            ; >> Call Site 6 <<
	.uleb128 Ltmp76-Ltmp75                  ;   Call between Ltmp75 and Ltmp76
	.uleb128 Ltmp77-Lfunc_begin0            ;     jumps to Ltmp77
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp78-Lfunc_begin0            ; >> Call Site 7 <<
	.uleb128 Ltmp79-Ltmp78                  ;   Call between Ltmp78 and Ltmp79
	.uleb128 Ltmp80-Lfunc_begin0            ;     jumps to Ltmp80
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp87-Lfunc_begin0            ; >> Call Site 8 <<
	.uleb128 Ltmp88-Ltmp87                  ;   Call between Ltmp87 and Ltmp88
	.uleb128 Ltmp89-Lfunc_begin0            ;     jumps to Ltmp89
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp90-Lfunc_begin0            ; >> Call Site 9 <<
	.uleb128 Ltmp93-Ltmp90                  ;   Call between Ltmp90 and Ltmp93
	.uleb128 Ltmp94-Lfunc_begin0            ;     jumps to Ltmp94
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp116-Lfunc_begin0           ; >> Call Site 10 <<
	.uleb128 Ltmp121-Ltmp116                ;   Call between Ltmp116 and Ltmp121
	.uleb128 Ltmp122-Lfunc_begin0           ;     jumps to Ltmp122
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp121-Lfunc_begin0           ; >> Call Site 11 <<
	.uleb128 Ltmp98-Ltmp121                 ;   Call between Ltmp121 and Ltmp98
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp98-Lfunc_begin0            ; >> Call Site 12 <<
	.uleb128 Ltmp99-Ltmp98                  ;   Call between Ltmp98 and Ltmp99
	.uleb128 Ltmp100-Lfunc_begin0           ;     jumps to Ltmp100
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp99-Lfunc_begin0            ; >> Call Site 13 <<
	.uleb128 Ltmp95-Ltmp99                  ;   Call between Ltmp99 and Ltmp95
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp95-Lfunc_begin0            ; >> Call Site 14 <<
	.uleb128 Ltmp96-Ltmp95                  ;   Call between Ltmp95 and Ltmp96
	.uleb128 Ltmp97-Lfunc_begin0            ;     jumps to Ltmp97
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp101-Lfunc_begin0           ; >> Call Site 15 <<
	.uleb128 Ltmp102-Ltmp101                ;   Call between Ltmp101 and Ltmp102
	.uleb128 Ltmp103-Lfunc_begin0           ;     jumps to Ltmp103
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp102-Lfunc_begin0           ; >> Call Site 16 <<
	.uleb128 Ltmp81-Ltmp102                 ;   Call between Ltmp102 and Ltmp81
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp81-Lfunc_begin0            ; >> Call Site 17 <<
	.uleb128 Ltmp82-Ltmp81                  ;   Call between Ltmp81 and Ltmp82
	.uleb128 Ltmp83-Lfunc_begin0            ;     jumps to Ltmp83
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp84-Lfunc_begin0            ; >> Call Site 18 <<
	.uleb128 Ltmp85-Ltmp84                  ;   Call between Ltmp84 and Ltmp85
	.uleb128 Ltmp86-Lfunc_begin0            ;     jumps to Ltmp86
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp123-Lfunc_begin0           ; >> Call Site 19 <<
	.uleb128 Ltmp124-Ltmp123                ;   Call between Ltmp123 and Ltmp124
	.uleb128 Ltmp125-Lfunc_begin0           ;     jumps to Ltmp125
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp124-Lfunc_begin0           ; >> Call Site 20 <<
	.uleb128 Ltmp5-Ltmp124                  ;   Call between Ltmp124 and Ltmp5
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp5-Lfunc_begin0             ; >> Call Site 21 <<
	.uleb128 Ltmp6-Ltmp5                    ;   Call between Ltmp5 and Ltmp6
	.uleb128 Ltmp7-Lfunc_begin0             ;     jumps to Ltmp7
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp8-Lfunc_begin0             ; >> Call Site 22 <<
	.uleb128 Ltmp9-Ltmp8                    ;   Call between Ltmp8 and Ltmp9
	.uleb128 Ltmp10-Lfunc_begin0            ;     jumps to Ltmp10
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp9-Lfunc_begin0             ; >> Call Site 23 <<
	.uleb128 Ltmp11-Ltmp9                   ;   Call between Ltmp9 and Ltmp11
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp11-Lfunc_begin0            ; >> Call Site 24 <<
	.uleb128 Ltmp12-Ltmp11                  ;   Call between Ltmp11 and Ltmp12
	.uleb128 Ltmp13-Lfunc_begin0            ;     jumps to Ltmp13
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp14-Lfunc_begin0            ; >> Call Site 25 <<
	.uleb128 Ltmp15-Ltmp14                  ;   Call between Ltmp14 and Ltmp15
	.uleb128 Ltmp16-Lfunc_begin0            ;     jumps to Ltmp16
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp15-Lfunc_begin0            ; >> Call Site 26 <<
	.uleb128 Ltmp110-Ltmp15                 ;   Call between Ltmp15 and Ltmp110
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp110-Lfunc_begin0           ; >> Call Site 27 <<
	.uleb128 Ltmp111-Ltmp110                ;   Call between Ltmp110 and Ltmp111
	.uleb128 Ltmp112-Lfunc_begin0           ;     jumps to Ltmp112
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp113-Lfunc_begin0           ; >> Call Site 28 <<
	.uleb128 Ltmp114-Ltmp113                ;   Call between Ltmp113 and Ltmp114
	.uleb128 Ltmp115-Lfunc_begin0           ;     jumps to Ltmp115
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp104-Lfunc_begin0           ; >> Call Site 29 <<
	.uleb128 Ltmp105-Ltmp104                ;   Call between Ltmp104 and Ltmp105
	.uleb128 Ltmp106-Lfunc_begin0           ;     jumps to Ltmp106
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp107-Lfunc_begin0           ; >> Call Site 30 <<
	.uleb128 Ltmp108-Ltmp107                ;   Call between Ltmp107 and Ltmp108
	.uleb128 Ltmp109-Lfunc_begin0           ;     jumps to Ltmp109
	.byte	5                               ;   On action: 3
	.uleb128 Ltmp108-Lfunc_begin0           ; >> Call Site 31 <<
	.uleb128 Lfunc_end0-Ltmp108             ;   Call between Ltmp108 and Lfunc_end0
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
Lcst_end0:
	.byte	0                               ; >> Action Record 1 <<
                                        ;   Cleanup
	.byte	0                               ;   No further actions
	.byte	0                               ; >> Action Record 2 <<
                                        ;   Cleanup
	.byte	125                             ;   Continue to action 1
	.byte	1                               ; >> Action Record 3 <<
                                        ;   Catch TypeInfo 1
	.byte	125                             ;   Continue to action 2
	.p2align	2, 0x0
                                        ; >> Catch TypeInfos <<
Ltmp166:                                ; TypeInfo 1
	.long	__ZTISt9exception@GOT-Ltmp166
Lttbase0:
	.p2align	2, 0x0
                                        ; -- End function
	.section	__TEXT,__text,regular,pure_instructions
	.globl	__Z21check_resource_budgetv     ; -- Begin function _Z21check_resource_budgetv
	.weak_def_can_be_hidden	__Z21check_resource_budgetv
	.p2align	2
__Z21check_resource_budgetv:            ; @_Z21check_resource_budgetv
Lfunc_begin1:
	.cfi_startproc
	.cfi_personality 155, ___gxx_personality_v0
	.cfi_lsda 16, Lexception1
; %bb.0:
	sub	sp, sp, #176
	stp	x20, x19, [sp, #144]            ; 16-byte Folded Spill
	stp	x29, x30, [sp, #160]            ; 16-byte Folded Spill
	.cfi_def_cfa_offset 176
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	movi.2d	v0, #0000000000000000
	stp	q0, q0, [sp, #112]
	stp	q0, q0, [sp, #80]
	stp	q0, q0, [sp, #48]
	stp	q0, q0, [sp, #16]
	str	q0, [sp]
	mov	x1, sp
	mov	w0, #0                          ; =0x0
	bl	_getrusage
	cbnz	w0, LBB1_3
; %bb.1:
	ldr	x8, [sp]
	ldrsw	x9, [sp, #8]
	ldr	x10, [sp, #16]
	ldrsw	x11, [sp, #24]
	ldr	x12, [sp, #32]
	add	x8, x10, x8
	mov	w10, #16960                     ; =0x4240
	movk	w10, #15, lsl #16
	add	x9, x11, x9
	madd	x8, x8, x10, x9
	mov	w9, #17920                      ; =0x4600
	movk	w9, #9155, lsl #16
	cmp	x8, x9
	mov	w8, #25857                      ; =0x6501
	movk	w8, #7629, lsl #16
	ccmp	x12, x8, #2, ls
	b.hs	LBB1_5
; %bb.2:
	ldp	x29, x30, [sp, #160]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #144]            ; 16-byte Folded Reload
	add	sp, sp, #176
	ret
LBB1_3:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x19, x0
Ltmp126:
Lloh66:
	adrp	x1, l_.str.17@PAGE
Lloh67:
	add	x1, x1, l_.str.17@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp127:
; %bb.4:
	mov	x0, x19
	bl	__Z21check_resource_budgetv.cold.1
LBB1_5:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x19, x0
Ltmp129:
Lloh68:
	adrp	x1, l_.str.18@PAGE
Lloh69:
	add	x1, x1, l_.str.18@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp130:
; %bb.6:
	mov	x0, x19
	bl	__Z21check_resource_budgetv.cold.2
LBB1_7:
Ltmp131:
	mov	x20, x0
	mov	x0, x19
	bl	___cxa_free_exception
	mov	x0, x20
	bl	__Unwind_Resume
LBB1_8:
Ltmp128:
	mov	x20, x0
	mov	x0, x19
	bl	___cxa_free_exception
	mov	x0, x20
	bl	__Unwind_Resume
	.loh AdrpAdd	Lloh66, Lloh67
	.loh AdrpAdd	Lloh68, Lloh69
Lfunc_end1:
	.cfi_endproc
	.section	__TEXT,__gcc_except_tab
	.p2align	2, 0x0
GCC_except_table1:
Lexception1:
	.byte	255                             ; @LPStart Encoding = omit
	.byte	255                             ; @TType Encoding = omit
	.byte	1                               ; Call site Encoding = uleb128
	.uleb128 Lcst_end1-Lcst_begin1
Lcst_begin1:
	.uleb128 Lfunc_begin1-Lfunc_begin1      ; >> Call Site 1 <<
	.uleb128 Ltmp126-Lfunc_begin1           ;   Call between Lfunc_begin1 and Ltmp126
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp126-Lfunc_begin1           ; >> Call Site 2 <<
	.uleb128 Ltmp127-Ltmp126                ;   Call between Ltmp126 and Ltmp127
	.uleb128 Ltmp128-Lfunc_begin1           ;     jumps to Ltmp128
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp127-Lfunc_begin1           ; >> Call Site 3 <<
	.uleb128 Ltmp129-Ltmp127                ;   Call between Ltmp127 and Ltmp129
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp129-Lfunc_begin1           ; >> Call Site 4 <<
	.uleb128 Ltmp130-Ltmp129                ;   Call between Ltmp129 and Ltmp130
	.uleb128 Ltmp131-Lfunc_begin1           ;     jumps to Ltmp131
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp130-Lfunc_begin1           ; >> Call Site 5 <<
	.uleb128 Lfunc_end1-Ltmp130             ;   Call between Ltmp130 and Lfunc_end1
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
Lcst_end1:
	.p2align	2, 0x0
                                        ; -- End function
	.section	__TEXT,__text,regular,pure_instructions
	.p2align	2                               ; -- Begin function _ZL6verify4Pairi
__ZL6verify4Pairi:                      ; @_ZL6verify4Pairi
Lfunc_begin2:
	.cfi_startproc
	.cfi_personality 155, ___gxx_personality_v0
	.cfi_lsda 16, Lexception2
; %bb.0:
	stp	x24, x23, [sp, #-64]!           ; 16-byte Folded Spill
	stp	x22, x21, [sp, #16]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #32]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	.cfi_def_cfa_offset 64
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	mov	x19, x1
	mov	x20, x0
	cmp	w2, #1
	b.ne	LBB2_20
; %bb.1:
	cmp	x20, #100
	cset	w8, gt
	mov	w9, #2                          ; =0x2
	cinc	x9, x9, gt
	cmn	x20, #100
	csinv	w21, w8, wzr, ge
	csinc	x22, x9, xzr, ge
	mov	x0, x20
	mov	x1, x19
	bl	__Z15probe_candidatexxb
	adrp	x23, __MergedGlobals@PAGE
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	ccmp	w0, w21, #0, ne
	b.ne	LBB2_43
; %bb.2:
	cmp	w8, #2
	b.eq	LBB2_41
; %bb.3:
	eor	x8, x20, x19, lsl #1
	add	x22, x22, x8
	mov	x0, x20
	mov	x1, x19
	bl	__Z16candidate_policyxx
	cmp	x0, x22
	b.ne	LBB2_41
; %bb.4:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #1                          ; =0x1
	bl	__Z15probe_baseline1xxb
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_43
; %bb.5:
	cmp	w0, w21
	b.ne	LBB2_43
; %bb.6:
	cmp	w8, #2
	b.eq	LBB2_41
; %bb.7:
	mov	x0, x20
	mov	x1, x19
	bl	__Z16baseline1_policyxx
	cmp	x0, x22
	b.ne	LBB2_41
; %bb.8:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #1                          ; =0x1
	bl	__Z15probe_baseline2xxb
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_43
; %bb.9:
	cmp	w0, w21
	b.ne	LBB2_43
; %bb.10:
	cmp	w8, #2
	b.eq	LBB2_41
; %bb.11:
	mov	x0, x20
	mov	x1, x19
	bl	__Z16baseline2_policyxx
	cmp	x0, x22
	b.ne	LBB2_41
; %bb.12:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #1                          ; =0x1
	bl	__Z15probe_baseline3xxb
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_43
; %bb.13:
	cmp	w0, w21
	b.ne	LBB2_43
; %bb.14:
	cmp	w8, #2
	b.eq	LBB2_41
; %bb.15:
	mov	x0, x20
	mov	x1, x19
	bl	__Z16baseline3_policyxx
	cmp	x0, x22
	b.ne	LBB2_41
; %bb.16:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #1                          ; =0x1
	bl	__Z15probe_baseline4xxb
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_43
; %bb.17:
	cmp	w0, w21
	b.ne	LBB2_43
; %bb.18:
	cmp	w8, #2
	b.eq	LBB2_41
; %bb.19:
	mov	x0, x20
	mov	x1, x19
	bl	__Z16baseline4_policyxx
	b	LBB2_39
LBB2_20:
	cmp	x20, x19
	cset	w8, gt
	csinv	w21, w8, wzr, ge
	mov	x0, x20
	mov	x1, x19
	mov	w2, #0                          ; =0x0
	bl	__Z15probe_candidatexxb
	adrp	x23, __MergedGlobals@PAGE
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	ccmp	w0, w21, #0, ne
	b.ne	LBB2_43
; %bb.21:
	cmp	w8, #2
	b.eq	LBB2_41
; %bb.22:
	eor	x8, x20, x19, lsl #1
	add	w9, w21, #2
	add	x22, x8, x9
	mov	x0, x20
	mov	x1, x19
	bl	__Z9candidatexx
	cmp	x0, x22
	b.ne	LBB2_41
; %bb.23:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #0                          ; =0x0
	bl	__Z15probe_baseline1xxb
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_43
; %bb.24:
	cmp	w0, w21
	b.ne	LBB2_43
; %bb.25:
	cmp	w8, #2
	b.eq	LBB2_41
; %bb.26:
	mov	x0, x20
	mov	x1, x19
	bl	__Z9baseline1xx
	cmp	x0, x22
	b.ne	LBB2_41
; %bb.27:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #0                          ; =0x0
	bl	__Z15probe_baseline2xxb
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_43
; %bb.28:
	cmp	w0, w21
	b.ne	LBB2_43
; %bb.29:
	cmp	w8, #2
	b.eq	LBB2_41
; %bb.30:
	mov	x0, x20
	mov	x1, x19
	bl	__Z9baseline2xx
	cmp	x0, x22
	b.ne	LBB2_41
; %bb.31:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #0                          ; =0x0
	bl	__Z15probe_baseline3xxb
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_43
; %bb.32:
	cmp	w0, w21
	b.ne	LBB2_43
; %bb.33:
	cmp	w8, #2
	b.eq	LBB2_41
; %bb.34:
	mov	x0, x20
	mov	x1, x19
	bl	__Z9baseline3xx
	cmp	x0, x22
	b.ne	LBB2_41
; %bb.35:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #0                          ; =0x0
	bl	__Z15probe_baseline4xxb
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_43
; %bb.36:
	cmp	w0, w21
	b.ne	LBB2_43
; %bb.37:
	cmp	w8, #2
	b.eq	LBB2_41
; %bb.38:
	mov	x0, x20
	mov	x1, x19
	bl	__Z9baseline4xx
LBB2_39:
	cmp	x0, x22
	b.ne	LBB2_41
; %bb.40:
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB2_41:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x19, x0
Ltmp135:
Lloh70:
	adrp	x1, l_.str.23@PAGE
Lloh71:
	add	x1, x1, l_.str.23@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp136:
; %bb.42:
	mov	x0, x19
	bl	__ZL6verify4Pairi.cold.2
LBB2_43:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x19, x0
Ltmp132:
Lloh72:
	adrp	x1, l_.str.22@PAGE
Lloh73:
	add	x1, x1, l_.str.22@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp133:
; %bb.44:
	mov	x0, x19
	bl	__ZL6verify4Pairi.cold.1
LBB2_45:
Ltmp134:
	mov	x20, x0
	mov	x0, x19
	bl	___cxa_free_exception
	mov	x0, x20
	bl	__Unwind_Resume
LBB2_46:
Ltmp137:
	mov	x20, x0
	mov	x0, x19
	bl	___cxa_free_exception
	mov	x0, x20
	bl	__Unwind_Resume
	.loh AdrpAdd	Lloh70, Lloh71
	.loh AdrpAdd	Lloh72, Lloh73
Lfunc_end2:
	.cfi_endproc
	.section	__TEXT,__gcc_except_tab
	.p2align	2, 0x0
GCC_except_table2:
Lexception2:
	.byte	255                             ; @LPStart Encoding = omit
	.byte	255                             ; @TType Encoding = omit
	.byte	1                               ; Call site Encoding = uleb128
	.uleb128 Lcst_end2-Lcst_begin2
Lcst_begin2:
	.uleb128 Lfunc_begin2-Lfunc_begin2      ; >> Call Site 1 <<
	.uleb128 Ltmp135-Lfunc_begin2           ;   Call between Lfunc_begin2 and Ltmp135
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp135-Lfunc_begin2           ; >> Call Site 2 <<
	.uleb128 Ltmp136-Ltmp135                ;   Call between Ltmp135 and Ltmp136
	.uleb128 Ltmp137-Lfunc_begin2           ;     jumps to Ltmp137
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp136-Lfunc_begin2           ; >> Call Site 3 <<
	.uleb128 Ltmp132-Ltmp136                ;   Call between Ltmp136 and Ltmp132
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp132-Lfunc_begin2           ; >> Call Site 4 <<
	.uleb128 Ltmp133-Ltmp132                ;   Call between Ltmp132 and Ltmp133
	.uleb128 Ltmp134-Lfunc_begin2           ;     jumps to Ltmp134
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp133-Lfunc_begin2           ; >> Call Site 5 <<
	.uleb128 Lfunc_end2-Ltmp133             ;   Call between Ltmp133 and Lfunc_end2
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
Lcst_end2:
	.p2align	2, 0x0
                                        ; -- End function
	.section	__TEXT,__text,regular,pure_instructions
	.p2align	2                               ; -- Begin function _ZL15generate_corpusim
__ZL15generate_corpusim:                ; @_ZL15generate_corpusim
Lfunc_begin3:
	.cfi_startproc
	.cfi_personality 155, ___gxx_personality_v0
	.cfi_lsda 16, Lexception3
; %bb.0:
	sub	sp, sp, #176
	stp	d9, d8, [sp, #64]               ; 16-byte Folded Spill
	stp	x28, x27, [sp, #80]             ; 16-byte Folded Spill
	stp	x26, x25, [sp, #96]             ; 16-byte Folded Spill
	stp	x24, x23, [sp, #112]            ; 16-byte Folded Spill
	stp	x22, x21, [sp, #128]            ; 16-byte Folded Spill
	stp	x20, x19, [sp, #144]            ; 16-byte Folded Spill
	stp	x29, x30, [sp, #160]            ; 16-byte Folded Spill
	.cfi_def_cfa_offset 176
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset w25, -72
	.cfi_offset w26, -80
	.cfi_offset w27, -88
	.cfi_offset w28, -96
	.cfi_offset b8, -104
	.cfi_offset b9, -112
	tbnz	w1, #31, LBB3_39
; %bb.1:
	mov	x21, x1
	mov	x27, x0
	mov	x19, #22359                     ; =0x5757
	movk	x19, #14592, lsl #16
	movk	x19, #38627, lsl #32
	movk	x19, #62820, lsl #48
	mov	x28, #1527                      ; =0x5f7
	movk	x28, #17640, lsl #16
	movk	x28, #48262, lsl #32
	movk	x28, #52896, lsl #48
	mov	x23, #21417                     ; =0x53a9
	movk	x23, #22027, lsl #16
	movk	x23, #50449, lsl #32
	movk	x23, #17285, lsl #48
	mov	x24, #23972                     ; =0x5da4
	movk	x24, #8134, lsl #16
	movk	x24, #60677, lsl #32
	movk	x24, #9056, lsl #48
	stp	xzr, xzr, [x0]
	mov	w25, #21417                     ; =0x53a9
	movk	w25, #22027, lsl #16
	mov	w26, #50449                     ; =0xc511
	movk	w26, #17285, lsl #16
	str	xzr, [x0, #16]
	mov	w20, #28928                     ; =0x7100
	movk	w20, #2, lsl #16
	mov	w0, #28928                      ; =0x7100
	movk	w0, #2, lsl #16
	bl	__Znwm
	add	x8, x0, x20
	stp	x0, x0, [x27]
	mov	x9, x8
	str	x8, [x27, #16]
	cmp	w21, #0
	mov	x8, #6148914691236517205        ; =0x5555555555555555
	movk	x8, #16341, lsl #48
	fmov	d0, x8
	mov	x8, #7378697629483820646        ; =0x6666666666666666
	movk	x8, #16350, lsl #48
	fmov	d1, x8
	fcsel	d8, d1, d0, eq
	mov	x8, #6148914691236517205        ; =0x5555555555555555
	movk	x8, #16357, lsl #48
	fmov	d0, x8
	mov	x8, #-3689348814741910324       ; =0xcccccccccccccccc
	movk	x8, #52429
	movk	x8, #16352, lsl #48
	fmov	d1, x8
	fcsel	d9, d1, d0, eq
	cmp	w21, #1
	mov	x22, x27
	str	x27, [sp, #32]                  ; 8-byte Folded Spill
	str	x0, [sp, #56]                   ; 8-byte Folded Spill
	b.ne	LBB3_21
; %bb.2:
	str	xzr, [sp, #48]                  ; 8-byte Folded Spill
	mov	x27, x0
	mov	x21, x9
	b	LBB3_5
LBB3_3:                                 ;   in Loop: Header=BB3_5 Depth=1
	stp	x20, xzr, [x8], #16
	mov	x9, x22
LBB3_4:                                 ;   in Loop: Header=BB3_5 Depth=1
	mov	x27, x8
	mov	x22, x9
	str	x8, [x9, #8]
	ldr	x9, [sp, #48]                   ; 8-byte Folded Reload
	add	x9, x9, #1
	mov	w8, #10000                      ; =0x2710
	str	x9, [sp, #48]                   ; 8-byte Folded Spill
	cmp	x9, x8
	b.eq	LBB3_33
LBB3_5:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB3_8 Depth 2
                                        ;     Child Loop BB3_13 Depth 2
	ldr	x8, [sp, #48]                   ; 8-byte Folded Reload
	cbnz	x8, LBB3_7
; %bb.6:                                ;   in Loop: Header=BB3_5 Depth=1
Ltmp149:
	bl	__Z21check_resource_budgetv
Ltmp150:
LBB3_7:                                 ;   in Loop: Header=BB3_5 Depth=1
	mul	x9, x19, x23
	lsr	x8, x19, #32
	umull	x10, w19, w25
	lsr	x10, x10, #32
	umaddl	x10, w8, w25, x10
	umull	x11, w19, w26
	add	x11, x11, w10, uxtw
	mul	x12, x28, x23
	cmn	x9, #4
	madd	x12, x19, x24, x12
	umaddl	x8, w8, w26, x12
	cinc	x8, x8, hi
	add	x8, x8, x10, lsr #32
	add	x8, x8, x11, lsr #32
	add	x9, x9, #3
LBB3_8:                                 ;   Parent Loop BB3_5 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	eor	x10, x8, x9
	lsr	x11, x8, #58
	mul	x12, x9, x23
	lsr	x13, x9, #32
	umull	x14, w9, w25
	lsr	x14, x14, #32
	umaddl	x14, w13, w25, x14
	umull	x15, w9, w26
	add	x15, x15, w14, uxtw
	mul	x9, x9, x24
	cmn	x12, #4
	madd	x8, x8, x23, x9
	umaddl	x8, w13, w26, x8
	cinc	x8, x8, hi
	add	x8, x8, x14, lsr #32
	add	x8, x8, x15, lsr #32
	add	x9, x12, #3
	ror	x12, x10, x11
	cmp	x11, #0
	csel	x10, x10, x12, eq
	cmp	x10, #616
	b.lo	LBB3_8
; %bb.9:                                ;   in Loop: Header=BB3_5 Depth=1
	eor	x11, x19, x28
	lsr	x12, x28, #58
	ror	x13, x11, x12
	cmp	x12, #0
	csel	x11, x11, x13, eq
	lsr	x11, x11, #11
	ucvtf	d0, x11, #53
	lsr	x11, x10, #3
	mov	x12, #63439                     ; =0xf7cf
	movk	x12, #58195, lsl #16
	movk	x12, #39845, lsl #32
	movk	x12, #8388, lsl #48
	umulh	x11, x11, x12
	lsr	x11, x11, #4
	mov	w12, #1000                      ; =0x3e8
	msub	x10, x11, x12, x10
	mov	x11, #-7378697629483820647      ; =0x9999999999999999
	movk	x11, #39322
	movk	x11, #16329, lsl #48
	fmov	d1, x11
	fcmp	d0, d1
	b.pl	LBB3_11
; %bb.10:                               ;   in Loop: Header=BB3_5 Depth=1
	mov	x11, #-101                      ; =0xffffffffffffff9b
	sub	x20, x11, x10
	b	LBB3_16
LBB3_11:                                ;   in Loop: Header=BB3_5 Depth=1
	mov	x11, #3689348814741910323       ; =0x3333333333333333
	movk	x11, #16339, lsl #48
	fmov	d1, x11
	fcmp	d0, d1
	b.pl	LBB3_15
; %bb.12:                               ;   in Loop: Header=BB3_5 Depth=1
	mov	x19, x9
	mov	x28, x8
LBB3_13:                                ;   Parent Loop BB3_5 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	eor	x8, x28, x19
	lsr	x9, x28, #58
	mul	x10, x19, x23
	lsr	x11, x19, #32
	umull	x12, w19, w25
	lsr	x12, x12, #32
	umaddl	x12, w11, w25, x12
	umull	x13, w19, w26
	add	x13, x13, w12, uxtw
	mul	x14, x19, x24
	cmn	x10, #4
	madd	x14, x28, x23, x14
	umaddl	x11, w11, w26, x14
	cinc	x11, x11, hi
	add	x11, x11, x12, lsr #32
	add	x28, x11, x13, lsr #32
	add	x19, x10, #3
	ror	x10, x8, x9
	cmp	x9, #0
	csel	x8, x8, x10, eq
	cmp	x8, #151
	b.lo	LBB3_13
; %bb.14:                               ;   in Loop: Header=BB3_5 Depth=1
	mov	x9, #41409                      ; =0xa1c1
	movk	x9, #23801, lsl #16
	movk	x9, #48255, lsl #32
	movk	x9, #17932, lsl #48
	umulh	x9, x8, x9
	sub	x10, x8, x9
	add	x9, x9, x10, lsr #1
	lsr	x9, x9, #7
	mov	w10, #201                       ; =0xc9
	msub	x8, x9, x10, x8
	sub	x20, x8, #100
	mov	x8, x27
	cmp	x27, x21
	b.lo	LBB3_3
	b	LBB3_17
LBB3_15:                                ;   in Loop: Header=BB3_5 Depth=1
	add	x20, x10, #101
LBB3_16:                                ;   in Loop: Header=BB3_5 Depth=1
	mov	x28, x8
	mov	x19, x9
	mov	x8, x27
	cmp	x27, x21
	b.lo	LBB3_3
LBB3_17:                                ;   in Loop: Header=BB3_5 Depth=1
	mov	x11, x21
	ldr	x10, [sp, #56]                  ; 8-byte Folded Reload
	sub	x21, x8, x10
	asr	x27, x21, #4
	add	x8, x27, #1
	lsr	x9, x8, #60
	cbnz	x9, LBB3_34
; %bb.18:                               ;   in Loop: Header=BB3_5 Depth=1
	sub	x9, x11, x10
	asr	x10, x9, #3
	cmp	x10, x8
	csel	x8, x10, x8, hi
	mov	x10, #9223372036854775792       ; =0x7ffffffffffffff0
	cmp	x9, x10
	mov	x9, #1152921504606846975        ; =0xfffffffffffffff
	csel	x22, x8, x9, lo
	lsr	x8, x22, #60
	cbnz	x8, LBB3_36
; %bb.19:                               ;   in Loop: Header=BB3_5 Depth=1
	lsl	x0, x22, #4
Ltmp152:
	bl	__Znwm
Ltmp153:
; %bb.20:                               ;   in Loop: Header=BB3_5 Depth=1
	add	x8, x0, x21
	add	x9, x0, x22, lsl #4
	str	x9, [sp, #40]                   ; 8-byte Folded Spill
	stp	x20, xzr, [x8]
	add	x20, x8, #16
	str	x20, [sp, #24]                  ; 8-byte Folded Spill
	sub	x22, x8, x27, lsl #4
	mov	x0, x22
	ldr	x27, [sp, #56]                  ; 8-byte Folded Reload
	mov	x1, x27
	mov	x2, x21
	bl	_memcpy
	ldr	x21, [sp, #32]                  ; 8-byte Folded Reload
	stp	x22, x20, [x21]
	ldr	x20, [sp, #40]                  ; 8-byte Folded Reload
	str	x20, [x21, #16]
	mov	x0, x27
	bl	__ZdlPv
	ldr	x8, [sp, #24]                   ; 8-byte Folded Reload
	str	x22, [sp, #56]                  ; 8-byte Folded Spill
	mov	x9, x21
	mov	x21, x20
	b	LBB3_4
LBB3_21:
	mov	x8, #0                          ; =0x0
	mov	x21, x0
	str	x9, [sp, #40]                   ; 8-byte Folded Spill
	b	LBB3_24
LBB3_22:                                ;   in Loop: Header=BB3_24 Depth=1
	stp	x19, x16, [x21], #16
LBB3_23:                                ;   in Loop: Header=BB3_24 Depth=1
	umull	x8, w20, w25
	lsr	x8, x8, #32
	umaddl	x8, w13, w25, x8
	umull	x9, w20, w26
	add	x9, x9, w8, uxtw
	add	x8, x14, x8, lsr #32
	madd	x8, x27, x23, x8
	add	x28, x8, x9, lsr #32
	add	x19, x12, #3
	mov	x22, x15
	str	x21, [x15, #8]
	ldr	x8, [sp, #48]                   ; 8-byte Folded Reload
	add	x8, x8, #1
	mov	w9, #10000                      ; =0x2710
	cmp	x8, x9
	b.eq	LBB3_33
LBB3_24:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB3_27 Depth 2
	str	x8, [sp, #48]                   ; 8-byte Folded Spill
	cbnz	x8, LBB3_26
; %bb.25:                               ;   in Loop: Header=BB3_24 Depth=1
Ltmp138:
	bl	__Z21check_resource_budgetv
Ltmp139:
LBB3_26:                                ;   in Loop: Header=BB3_24 Depth=1
	mul	x8, x19, x23
	lsr	x9, x19, #32
	umull	x10, w19, w25
	lsr	x10, x10, #32
	umaddl	x10, w9, w25, x10
	umull	x11, w19, w26
	add	x11, x11, w10, uxtw
	mul	x12, x28, x23
	cmn	x8, #4
	madd	x12, x19, x24, x12
	umaddl	x9, w9, w26, x12
	cinc	x9, x9, hi
	add	x9, x9, x10, lsr #32
	add	x27, x9, x11, lsr #32
	add	x20, x8, #3
	mov	x15, x22
LBB3_27:                                ;   Parent Loop BB3_24 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	eor	x8, x27, x20
	lsr	x9, x27, #58
	mul	x10, x20, x23
	lsr	x11, x20, #32
	umull	x12, w20, w25
	lsr	x12, x12, #32
	umaddl	x12, w11, w25, x12
	umull	x13, w20, w26
	add	x13, x13, w12, uxtw
	mul	x14, x20, x24
	cmn	x10, #4
	madd	x14, x27, x23, x14
	umaddl	x11, w11, w26, x14
	cinc	x11, x11, hi
	add	x11, x11, x12, lsr #32
	add	x27, x11, x13, lsr #32
	add	x20, x10, #3
	ror	x10, x8, x9
	cmp	x9, #0
	csel	x8, x8, x10, eq
	lsr	x9, x8, #31
	cbz	x9, LBB3_27
; %bb.28:                               ;   in Loop: Header=BB3_24 Depth=1
	eor	x9, x19, x28
	lsr	x10, x28, #58
	ror	x11, x9, x10
	cmp	x10, #0
	csel	x9, x9, x11, eq
	lsr	x9, x9, #11
	ucvtf	d0, x9, #53
	mov	w9, #-2147483648                ; =0x80000000
	add	x9, x8, x9
	mov	x10, #-2147483648               ; =0xffffffff80000000
	cmp	x8, x10
	csel	x8, x8, x9, lo
	eor	x9, x27, x20
	lsr	x10, x27, #58
	mul	x12, x20, x23
	lsr	x13, x20, #32
	mul	x11, x20, x24
	cmn	x12, #4
	umaddl	x11, w13, w26, x11
	cinc	x14, x11, hi
	ror	x11, x9, x10
	cmp	x10, #0
	csel	x9, x9, x11, eq
	and	x9, x9, #0x3fffffff
	fcmp	d0, d9
	csinv	x10, xzr, x9, mi
	fcmp	d0, d8
	csinc	x9, x10, x9, pl
	mov	x10, #1073741824                ; =0x40000000
	movk	x10, #32768, lsl #48
	add	x19, x8, x10
	add	x16, x19, x9
	ldr	x11, [sp, #40]                  ; 8-byte Folded Reload
	cmp	x21, x11
	b.lo	LBB3_22
; %bb.29:                               ;   in Loop: Header=BB3_24 Depth=1
	ldr	x10, [sp, #56]                  ; 8-byte Folded Reload
	sub	x21, x21, x10
	asr	x28, x21, #4
	add	x8, x28, #1
	lsr	x9, x8, #60
	cbnz	x9, LBB3_35
; %bb.30:                               ;   in Loop: Header=BB3_24 Depth=1
	stp	x16, x14, [sp]                  ; 16-byte Folded Spill
	stp	x13, x12, [sp, #16]             ; 16-byte Folded Spill
	sub	x9, x11, x10
	asr	x10, x9, #3
	cmp	x10, x8
	csel	x8, x10, x8, hi
	mov	x10, #9223372036854775792       ; =0x7ffffffffffffff0
	cmp	x9, x10
	mov	x9, #1152921504606846975        ; =0xfffffffffffffff
	csel	x22, x8, x9, lo
	lsr	x8, x22, #60
	cbnz	x8, LBB3_37
; %bb.31:                               ;   in Loop: Header=BB3_24 Depth=1
	lsl	x0, x22, #4
Ltmp141:
	bl	__Znwm
Ltmp142:
; %bb.32:                               ;   in Loop: Header=BB3_24 Depth=1
	add	x8, x0, x21
	add	x9, x0, x22, lsl #4
	ldr	x10, [sp]                       ; 8-byte Folded Reload
	stp	x19, x10, [x8]
	add	x10, x8, #16
	str	x10, [sp]                       ; 8-byte Folded Spill
	sub	x22, x8, x28, lsl #4
	mov	x0, x22
	ldr	x1, [sp, #56]                   ; 8-byte Folded Reload
	mov	x2, x21
	mov	x21, x9
	bl	_memcpy
	ldr	x19, [sp, #32]                  ; 8-byte Folded Reload
	ldr	x28, [sp]                       ; 8-byte Folded Reload
	stp	x22, x28, [x19]
	str	x21, [sp, #40]                  ; 8-byte Folded Spill
	str	x21, [x19, #16]
	ldr	x0, [sp, #56]                   ; 8-byte Folded Reload
	bl	__ZdlPv
	mov	x21, x28
	str	x22, [sp, #56]                  ; 8-byte Folded Spill
	mov	x15, x19
	ldp	x13, x12, [sp, #16]             ; 16-byte Folded Reload
	ldr	x14, [sp, #8]                   ; 8-byte Folded Reload
	b	LBB3_23
LBB3_33:
	ldp	x29, x30, [sp, #160]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #144]            ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #128]            ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #112]            ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #96]             ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #80]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp, #64]               ; 16-byte Folded Reload
	add	sp, sp, #176
	ret
LBB3_34:
Ltmp157:
	bl	__ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe210106Ev
Ltmp158:
	b	LBB3_38
LBB3_35:
Ltmp146:
	bl	__ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe210106Ev
Ltmp147:
	b	LBB3_38
LBB3_36:
Ltmp155:
	bl	__ZSt28__throw_bad_array_new_lengthB9nqe210106v
Ltmp156:
	b	LBB3_38
LBB3_37:
Ltmp144:
	bl	__ZSt28__throw_bad_array_new_lengthB9nqe210106v
Ltmp145:
LBB3_38:
	brk	#0x1
LBB3_39:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x19, x0
Ltmp160:
Lloh74:
	adrp	x1, l_.str.24@PAGE
Lloh75:
	add	x1, x1, l_.str.24@PAGEOFF
	bl	__ZNSt16invalid_argumentC1B9nqe210106EPKc
Ltmp161:
; %bb.40:
	mov	x0, x19
	bl	__ZL15generate_corpusim.cold.1
LBB3_41:
Ltmp162:
	mov	x21, x0
	mov	x0, x19
	bl	___cxa_free_exception
	mov	x0, x21
	bl	__Unwind_Resume
LBB3_42:
Ltmp140:
	b	LBB3_48
LBB3_43:
Ltmp151:
	b	LBB3_48
LBB3_44:
Ltmp143:
	b	LBB3_48
LBB3_45:
Ltmp154:
	b	LBB3_48
LBB3_46:
Ltmp148:
	b	LBB3_48
LBB3_47:
Ltmp159:
LBB3_48:
	mov	x21, x0
	ldr	x8, [sp, #56]                   ; 8-byte Folded Reload
	cbz	x8, LBB3_50
; %bb.49:
	ldr	x8, [sp, #32]                   ; 8-byte Folded Reload
	ldr	x0, [sp, #56]                   ; 8-byte Folded Reload
	str	x0, [x8, #8]
	bl	__ZdlPv
LBB3_50:
	mov	x0, x21
	bl	__Unwind_Resume
	.loh AdrpAdd	Lloh74, Lloh75
Lfunc_end3:
	.cfi_endproc
	.section	__TEXT,__gcc_except_tab
	.p2align	2, 0x0
GCC_except_table3:
Lexception3:
	.byte	255                             ; @LPStart Encoding = omit
	.byte	255                             ; @TType Encoding = omit
	.byte	1                               ; Call site Encoding = uleb128
	.uleb128 Lcst_end3-Lcst_begin3
Lcst_begin3:
	.uleb128 Lfunc_begin3-Lfunc_begin3      ; >> Call Site 1 <<
	.uleb128 Ltmp149-Lfunc_begin3           ;   Call between Lfunc_begin3 and Ltmp149
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp149-Lfunc_begin3           ; >> Call Site 2 <<
	.uleb128 Ltmp150-Ltmp149                ;   Call between Ltmp149 and Ltmp150
	.uleb128 Ltmp151-Lfunc_begin3           ;     jumps to Ltmp151
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp152-Lfunc_begin3           ; >> Call Site 3 <<
	.uleb128 Ltmp153-Ltmp152                ;   Call between Ltmp152 and Ltmp153
	.uleb128 Ltmp154-Lfunc_begin3           ;     jumps to Ltmp154
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp153-Lfunc_begin3           ; >> Call Site 4 <<
	.uleb128 Ltmp138-Ltmp153                ;   Call between Ltmp153 and Ltmp138
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp138-Lfunc_begin3           ; >> Call Site 5 <<
	.uleb128 Ltmp139-Ltmp138                ;   Call between Ltmp138 and Ltmp139
	.uleb128 Ltmp140-Lfunc_begin3           ;     jumps to Ltmp140
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp141-Lfunc_begin3           ; >> Call Site 6 <<
	.uleb128 Ltmp142-Ltmp141                ;   Call between Ltmp141 and Ltmp142
	.uleb128 Ltmp143-Lfunc_begin3           ;     jumps to Ltmp143
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp142-Lfunc_begin3           ; >> Call Site 7 <<
	.uleb128 Ltmp157-Ltmp142                ;   Call between Ltmp142 and Ltmp157
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp157-Lfunc_begin3           ; >> Call Site 8 <<
	.uleb128 Ltmp158-Ltmp157                ;   Call between Ltmp157 and Ltmp158
	.uleb128 Ltmp159-Lfunc_begin3           ;     jumps to Ltmp159
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp146-Lfunc_begin3           ; >> Call Site 9 <<
	.uleb128 Ltmp147-Ltmp146                ;   Call between Ltmp146 and Ltmp147
	.uleb128 Ltmp148-Lfunc_begin3           ;     jumps to Ltmp148
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp155-Lfunc_begin3           ; >> Call Site 10 <<
	.uleb128 Ltmp156-Ltmp155                ;   Call between Ltmp155 and Ltmp156
	.uleb128 Ltmp159-Lfunc_begin3           ;     jumps to Ltmp159
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp144-Lfunc_begin3           ; >> Call Site 11 <<
	.uleb128 Ltmp145-Ltmp144                ;   Call between Ltmp144 and Ltmp145
	.uleb128 Ltmp148-Lfunc_begin3           ;     jumps to Ltmp148
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp145-Lfunc_begin3           ; >> Call Site 12 <<
	.uleb128 Ltmp160-Ltmp145                ;   Call between Ltmp145 and Ltmp160
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp160-Lfunc_begin3           ; >> Call Site 13 <<
	.uleb128 Ltmp161-Ltmp160                ;   Call between Ltmp160 and Ltmp161
	.uleb128 Ltmp162-Lfunc_begin3           ;     jumps to Ltmp162
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp161-Lfunc_begin3           ; >> Call Site 14 <<
	.uleb128 Lfunc_end3-Ltmp161             ;   Call between Ltmp161 and Lfunc_end3
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
Lcst_end3:
	.p2align	2, 0x0
                                        ; -- End function
	.section	__TEXT,__text,regular,pure_instructions
	.private_extern	__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB9nqe210106Ev ; -- Begin function _ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB9nqe210106Ev
	.globl	__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB9nqe210106Ev
	.weak_def_can_be_hidden	__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB9nqe210106Ev
	.p2align	2
__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB9nqe210106Ev: ; @_ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB9nqe210106Ev
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh76:
	adrp	x0, l_.str.19@PAGE
Lloh77:
	add	x0, x0, l_.str.19@PAGEOFF
	bl	__ZNSt3__120__throw_length_errorB9nqe210106EPKc
	.loh AdrpAdd	Lloh76, Lloh77
	.cfi_endproc
                                        ; -- End function
	.private_extern	__ZNSt3__120__throw_length_errorB9nqe210106EPKc ; -- Begin function _ZNSt3__120__throw_length_errorB9nqe210106EPKc
	.globl	__ZNSt3__120__throw_length_errorB9nqe210106EPKc
	.weak_def_can_be_hidden	__ZNSt3__120__throw_length_errorB9nqe210106EPKc
	.p2align	2
__ZNSt3__120__throw_length_errorB9nqe210106EPKc: ; @_ZNSt3__120__throw_length_errorB9nqe210106EPKc
Lfunc_begin4:
	.cfi_startproc
	.cfi_personality 155, ___gxx_personality_v0
	.cfi_lsda 16, Lexception4
; %bb.0:
	stp	x20, x19, [sp, #-32]!           ; 16-byte Folded Spill
	stp	x29, x30, [sp, #16]             ; 16-byte Folded Spill
	.cfi_def_cfa_offset 32
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	mov	x20, x0
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x19, x0
Ltmp163:
	mov	x1, x20
	bl	__ZNSt12length_errorC1B9nqe210106EPKc
Ltmp164:
; %bb.1:
Lloh78:
	adrp	x1, __ZTISt12length_error@GOTPAGE
Lloh79:
	ldr	x1, [x1, __ZTISt12length_error@GOTPAGEOFF]
Lloh80:
	adrp	x2, __ZNSt12length_errorD1Ev@GOTPAGE
Lloh81:
	ldr	x2, [x2, __ZNSt12length_errorD1Ev@GOTPAGEOFF]
	mov	x0, x19
	bl	___cxa_throw
LBB5_2:
Ltmp165:
	mov	x20, x0
	mov	x0, x19
	bl	___cxa_free_exception
	mov	x0, x20
	bl	__Unwind_Resume
	.loh AdrpLdrGot	Lloh80, Lloh81
	.loh AdrpLdrGot	Lloh78, Lloh79
Lfunc_end4:
	.cfi_endproc
	.section	__TEXT,__gcc_except_tab
	.p2align	2, 0x0
GCC_except_table5:
Lexception4:
	.byte	255                             ; @LPStart Encoding = omit
	.byte	255                             ; @TType Encoding = omit
	.byte	1                               ; Call site Encoding = uleb128
	.uleb128 Lcst_end4-Lcst_begin4
Lcst_begin4:
	.uleb128 Lfunc_begin4-Lfunc_begin4      ; >> Call Site 1 <<
	.uleb128 Ltmp163-Lfunc_begin4           ;   Call between Lfunc_begin4 and Ltmp163
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp163-Lfunc_begin4           ; >> Call Site 2 <<
	.uleb128 Ltmp164-Ltmp163                ;   Call between Ltmp163 and Ltmp164
	.uleb128 Ltmp165-Lfunc_begin4           ;     jumps to Ltmp165
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp164-Lfunc_begin4           ; >> Call Site 3 <<
	.uleb128 Lfunc_end4-Ltmp164             ;   Call between Ltmp164 and Lfunc_end4
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
Lcst_end4:
	.p2align	2, 0x0
                                        ; -- End function
	.section	__TEXT,__text,regular,pure_instructions
	.private_extern	__ZNSt12length_errorC1B9nqe210106EPKc ; -- Begin function _ZNSt12length_errorC1B9nqe210106EPKc
	.globl	__ZNSt12length_errorC1B9nqe210106EPKc
	.weak_def_can_be_hidden	__ZNSt12length_errorC1B9nqe210106EPKc
	.p2align	2
__ZNSt12length_errorC1B9nqe210106EPKc:  ; @_ZNSt12length_errorC1B9nqe210106EPKc
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	__ZNSt11logic_errorC2EPKc
Lloh82:
	adrp	x8, __ZTVSt12length_error@GOTPAGE
Lloh83:
	ldr	x8, [x8, __ZTVSt12length_error@GOTPAGEOFF]
	add	x8, x8, #16
	str	x8, [x0]
	ldp	x29, x30, [sp], #16             ; 16-byte Folded Reload
	ret
	.loh AdrpLdrGot	Lloh82, Lloh83
	.cfi_endproc
                                        ; -- End function
	.private_extern	__ZSt28__throw_bad_array_new_lengthB9nqe210106v ; -- Begin function _ZSt28__throw_bad_array_new_lengthB9nqe210106v
	.globl	__ZSt28__throw_bad_array_new_lengthB9nqe210106v
	.weak_def_can_be_hidden	__ZSt28__throw_bad_array_new_lengthB9nqe210106v
	.p2align	2
__ZSt28__throw_bad_array_new_lengthB9nqe210106v: ; @_ZSt28__throw_bad_array_new_lengthB9nqe210106v
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	mov	w0, #8                          ; =0x8
	bl	___cxa_allocate_exception
	bl	__ZNSt20bad_array_new_lengthC1Ev
Lloh84:
	adrp	x1, __ZTISt20bad_array_new_length@GOTPAGE
Lloh85:
	ldr	x1, [x1, __ZTISt20bad_array_new_length@GOTPAGEOFF]
Lloh86:
	adrp	x2, __ZNSt20bad_array_new_lengthD1Ev@GOTPAGE
Lloh87:
	ldr	x2, [x2, __ZNSt20bad_array_new_lengthD1Ev@GOTPAGEOFF]
	bl	___cxa_throw
	.loh AdrpLdrGot	Lloh86, Lloh87
	.loh AdrpLdrGot	Lloh84, Lloh85
	.cfi_endproc
                                        ; -- End function
	.private_extern	__ZNSt16invalid_argumentC1B9nqe210106EPKc ; -- Begin function _ZNSt16invalid_argumentC1B9nqe210106EPKc
	.globl	__ZNSt16invalid_argumentC1B9nqe210106EPKc
	.weak_def_can_be_hidden	__ZNSt16invalid_argumentC1B9nqe210106EPKc
	.p2align	2
__ZNSt16invalid_argumentC1B9nqe210106EPKc: ; @_ZNSt16invalid_argumentC1B9nqe210106EPKc
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	__ZNSt11logic_errorC2EPKc
Lloh88:
	adrp	x8, __ZTVSt16invalid_argument@GOTPAGE
Lloh89:
	ldr	x8, [x8, __ZTVSt16invalid_argument@GOTPAGEOFF]
	add	x8, x8, #16
	str	x8, [x0]
	ldp	x29, x30, [sp], #16             ; 16-byte Folded Reload
	ret
	.loh AdrpLdrGot	Lloh88, Lloh89
	.cfi_endproc
                                        ; -- End function
	.private_extern	__ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe210106Ev ; -- Begin function _ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe210106Ev
	.globl	__ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe210106Ev
	.weak_def_can_be_hidden	__ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe210106Ev
	.p2align	2
__ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe210106Ev: ; @_ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe210106Ev
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh90:
	adrp	x0, l_.str.25@PAGE
Lloh91:
	add	x0, x0, l_.str.25@PAGEOFF
	bl	__ZNSt3__120__throw_length_errorB9nqe210106EPKc
	.loh AdrpAdd	Lloh90, Lloh91
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function _Z21check_resource_budgetv.cold.1
__Z21check_resource_budgetv.cold.1:     ; @_Z21check_resource_budgetv.cold.1
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	_OUTLINED_FUNCTION_0
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function _Z21check_resource_budgetv.cold.2
__Z21check_resource_budgetv.cold.2:     ; @_Z21check_resource_budgetv.cold.2
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	_OUTLINED_FUNCTION_0
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function _ZL6verify4Pairi.cold.1
__ZL6verify4Pairi.cold.1:               ; @_ZL6verify4Pairi.cold.1
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	_OUTLINED_FUNCTION_0
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function _ZL6verify4Pairi.cold.2
__ZL6verify4Pairi.cold.2:               ; @_ZL6verify4Pairi.cold.2
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	_OUTLINED_FUNCTION_0
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function _ZL15generate_corpusim.cold.1
__ZL15generate_corpusim.cold.1:         ; @_ZL15generate_corpusim.cold.1
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh92:
	adrp	x1, __ZTISt16invalid_argument@GOTPAGE
Lloh93:
	ldr	x1, [x1, __ZTISt16invalid_argument@GOTPAGEOFF]
Lloh94:
	adrp	x2, __ZNSt16invalid_argumentD1Ev@GOTPAGE
Lloh95:
	ldr	x2, [x2, __ZNSt16invalid_argumentD1Ev@GOTPAGEOFF]
	bl	___cxa_throw
	.loh AdrpLdrGot	Lloh94, Lloh95
	.loh AdrpLdrGot	Lloh92, Lloh93
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function OUTLINED_FUNCTION_0
_OUTLINED_FUNCTION_0:                   ; @OUTLINED_FUNCTION_0 Thunk
	.cfi_startproc
; %bb.0:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	b	___cxa_throw
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"--selftest"

l_.str.1:                               ; @.str.1
	.asciz	"--dump-corpus"

l_.str.2:                               ; @.str.2
	.asciz	"--dump-bootstrap-indices"

l_.str.3:                               ; @.str.3
	.asciz	"--probe-pmu"

l_.str.4:                               ; @.str.4
	.asciz	"--test-fault-correctness"

l_.str.5:                               ; @.str.5
	.asciz	"--test-fault-payload"

l_.str.6:                               ; @.str.6
	.asciz	"--smoke"

l_.str.7:                               ; @.str.7
	.asciz	"INVALID_ARGUMENT"

l_.str.8:                               ; @.str.8
	.asciz	"TEST_FAULT_REQUIRES_SELFTEST"

l_.str.11:                              ; @.str.11
	.asciz	"CORPUS_NONDETERMINISTIC"

l_.str.12:                              ; @.str.12
	.asciz	"PASS PCG=4 BOUNDARY=300 POLICY=120 GENERATED=150000 DETERMINISM=30000\n"

l_.str.13:                              ; @.str.13
	.asciz	"%d,%lld,%lld\n"

l_.str.14:                              ; @.str.14
	.asciz	"MEASUREMENT_NOT_AUTHORIZED_RUNNER_INCOMPLETE"

l_.str.15:                              ; @.str.15
	.asciz	"PMU_UNAVAILABLE"

l_.str.16:                              ; @.str.16
	.asciz	"BLOCKED \342\200\224 %s\n"

l_.str.17:                              ; @.str.17
	.asciz	"RESOURCE_ACCOUNTING_FAILURE"

l_.str.18:                              ; @.str.18
	.asciz	"BUDGET_EXCEEDED"

l_.str.19:                              ; @.str.19
	.asciz	"basic_string"

l_.str.22:                              ; @.str.22
	.asciz	"CORRECTNESS_FAILURE"

l_.str.23:                              ; @.str.23
	.asciz	"PAYLOAD_PARITY"

l_.str.24:                              ; @.str.24
	.asciz	"INVALID_PATTERN"

l_.str.25:                              ; @.str.25
	.asciz	"vector"

	.section	__DATA,__const
	.p2align	3, 0x0                          ; @_ZL12policy_modes
__ZL12policy_modes:
	.quad	__Z16candidate_policyxx
	.quad	__Z16baseline1_policyxx
	.quad	__Z16baseline2_policyxx
	.quad	__Z16baseline3_policyxx
	.quad	__Z16baseline4_policyxx

	.p2align	3, 0x0                          ; @_ZL5modes
__ZL5modes:
	.quad	__Z9candidatexx
	.quad	__Z9baseline1xx
	.quad	__Z9baseline2xx
	.quad	__Z9baseline3xx
	.quad	__Z9baseline4xx

.zerofill __DATA,__bss,__MergedGlobals,16,3 ; @_MergedGlobals
.subsections_via_symbols
