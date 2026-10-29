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
Ltmp0:                                  ; EH_LABEL
	bl	__Z21check_resource_budgetv
Ltmp1:                                  ; EH_LABEL
; %bb.1:
	adrp	x8, __MergedGlobals@PAGE
	cmp	w20, #2
	b.lt	LBB0_31
; %bb.2:
	str	wzr, [sp, #48]                  ; 4-byte Spill
	str	xzr, [sp, #40]                  ; 8-byte Folded Spill
	str	wzr, [sp, #56]                  ; 4-byte Spill
	mov	x26, #11565                     ; =0x2d2d
	movk	x26, #25972, lsl #16
	movk	x26, #29811, lsl #32
	movk	x26, #26157, lsl #48
	mov	w8, w20
	add	x27, x19, #8
	sub	x28, x8, #1
	mov	x24, #-9                        ; =0xfffffffffffffff7
	movk	x24, #32767, lsl #48
	add	x25, sp, #112
	mov	w23, #25                        ; =0x19
	b	LBB0_6
LBB0_3:                                 ;   in Loop: Header=BB0_6 Depth=1
	mov	w8, #1                          ; =0x1
	str	w8, [sp, #56]                   ; 4-byte Spill
LBB0_4:                                 ;   in Loop: Header=BB0_6 Depth=1
	ldr	x8, [sp, #128]
	and	x1, x8, #0x7fffffffffffffff
	bl	__ZdlPvm
LBB0_5:                                 ;   in Loop: Header=BB0_6 Depth=1
	subs	x28, x28, #1
	b.eq	LBB0_33
LBB0_6:                                 ; =>This Inner Loop Header: Depth=1
	ldr	x19, [x27], #8
	mov	x0, x19
	bl	_strlen
	cmp	x0, x24
	b.hs	LBB0_124
; %bb.7:                                ;   in Loop: Header=BB0_6 Depth=1
	mov	x20, x0
	cmp	x0, #22
	b.hi	LBB0_9
; %bb.8:                                ;   in Loop: Header=BB0_6 Depth=1
	strb	w20, [sp, #135]
	add	x21, sp, #112
	b	LBB0_11
LBB0_9:                                 ;   in Loop: Header=BB0_6 Depth=1
	and	x8, x20, #0x7ffffffffffffff8
	add	x8, x8, #8
	cmp	x8, #24
	csel	x22, x23, x8, eq
Ltmp2:                                  ; EH_LABEL
	mov	x0, x22
	bl	__Znwm
Ltmp3:                                  ; EH_LABEL
; %bb.10:                               ;   in Loop: Header=BB0_6 Depth=1
	mov	x21, x0
	orr	x8, x22, #0x8000000000000000
	stp	x0, x20, [sp, #112]
	str	x8, [sp, #128]
LBB0_11:                                ;   in Loop: Header=BB0_6 Depth=1
	mov	x0, x21
	mov	x1, x19
	mov	x2, x20
	bl	_memmove
	strb	wzr, [x21, x20]
	ldrb	w8, [sp, #135]
	sxtb	w22, w8
	ldr	x9, [sp, #120]
	cmp	w22, #0
	csel	x8, x9, x8, mi
	cmp	x8, #13
	b.eq	LBB0_15
; %bb.12:                               ;   in Loop: Header=BB0_6 Depth=1
	cmp	x8, #10
	b.ne	LBB0_16
; %bb.13:                               ;   in Loop: Header=BB0_6 Depth=1
	ldr	x0, [sp, #112]
	cmp	w22, #0
	csel	x10, x0, x25, mi
	ldr	x11, [x10]
	ldrh	w10, [x10, #8]
	mov	x12, #11565                     ; =0x2d2d
	movk	x12, #25971, lsl #16
	movk	x12, #26220, lsl #32
	movk	x12, #25972, lsl #48
	cmp	x11, x12
	mov	w11, #29811                     ; =0x7473
	ccmp	w10, w11, #0, eq
	b.ne	LBB0_16
; %bb.14:                               ;   in Loop: Header=BB0_6 Depth=1
	mov	w8, #1                          ; =0x1
	str	w8, [sp, #48]                   ; 4-byte Spill
	tbz	w22, #31, LBB0_5
	b	LBB0_4
LBB0_15:                                ;   in Loop: Header=BB0_6 Depth=1
	ldr	x0, [sp, #112]
	cmp	w22, #0
	csel	x10, x0, x25, mi
	ldr	x11, [x10]
	ldur	x10, [x10, #5]
	mov	x12, #11565                     ; =0x2d2d
	movk	x12, #30052, lsl #16
	movk	x12, #28781, lsl #32
	movk	x12, #25389, lsl #48
	cmp	x11, x12
	mov	x11, #11632                     ; =0x2d70
	movk	x11, #28515, lsl #16
	movk	x11, #28786, lsl #32
	movk	x11, #29557, lsl #48
	ccmp	x10, x11, #0, eq
	b.eq	LBB0_28
LBB0_16:                                ;   in Loop: Header=BB0_6 Depth=1
	cmp	w22, #0
	ccmp	x9, #24, #0, mi
	cset	w9, eq
	b.ne	LBB0_18
; %bb.17:                               ;   in Loop: Header=BB0_6 Depth=1
	ldr	x0, [sp, #112]
	ldp	x10, x11, [x0]
	ldr	x12, [x0, #16]
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
	b.eq	LBB0_3
LBB0_18:                                ;   in Loop: Header=BB0_6 Depth=1
	cmp	x8, #11
	b.ne	LBB0_20
; %bb.19:                               ;   in Loop: Header=BB0_6 Depth=1
	ldr	x0, [sp, #112]
	cmp	w22, #0
	csel	x10, x0, x25, mi
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
	b.eq	LBB0_29
LBB0_20:                                ;   in Loop: Header=BB0_6 Depth=1
	cbz	w9, LBB0_22
; %bb.21:                               ;   in Loop: Header=BB0_6 Depth=1
	ldr	x0, [sp, #112]
	ldp	x9, x10, [x0]
	ldr	x11, [x0, #16]
	cmp	x9, x26
	mov	x9, #30049                      ; =0x7561
	movk	x9, #29804, lsl #16
	movk	x9, #25389, lsl #32
	movk	x9, #29295, lsl #48
	ccmp	x10, x9, #0, eq
	mov	x9, #25970                      ; =0x6572
	movk	x9, #29795, lsl #16
	movk	x9, #25966, lsl #32
	movk	x9, #29555, lsl #48
	ccmp	x11, x9, #0, eq
	b.eq	LBB0_30
LBB0_22:                                ;   in Loop: Header=BB0_6 Depth=1
	cmp	x8, #7
	b.eq	LBB0_26
; %bb.23:                               ;   in Loop: Header=BB0_6 Depth=1
	cmp	x8, #20
	b.ne	LBB0_125
; %bb.24:                               ;   in Loop: Header=BB0_6 Depth=1
	ldr	x0, [sp, #112]
	cmp	w22, #0
	csel	x8, x0, x25, mi
	ldp	x9, x10, [x8]
	ldr	w8, [x8, #16]
	cmp	x9, x26
	mov	x9, #30049                      ; =0x7561
	movk	x9, #29804, lsl #16
	movk	x9, #28717, lsl #32
	movk	x9, #31073, lsl #48
	ccmp	x10, x9, #0, eq
	mov	w9, #28524                      ; =0x6f6c
	movk	w9, #25697, lsl #16
	ccmp	w8, w9, #0, eq
	b.ne	LBB0_125
; %bb.25:                               ;   in Loop: Header=BB0_6 Depth=1
	adrp	x8, __MergedGlobals@PAGE
	mov	w9, #2                          ; =0x2
	str	w9, [x8, __MergedGlobals@PAGEOFF]
	tbz	w22, #31, LBB0_5
	b	LBB0_4
LBB0_26:                                ;   in Loop: Header=BB0_6 Depth=1
	ldr	x0, [sp, #112]
	cmp	w22, #0
	csel	x8, x0, x25, mi
	ldr	w9, [x8]
	ldur	w8, [x8, #3]
	mov	w10, #11565                     ; =0x2d2d
	movk	w10, #28019, lsl #16
	cmp	w9, w10
	mov	w9, #28525                      ; =0x6f6d
	movk	w9, #25963, lsl #16
	ccmp	w8, w9, #0, eq
	b.ne	LBB0_125
; %bb.27:                               ;   in Loop: Header=BB0_6 Depth=1
	tbz	w22, #31, LBB0_5
	b	LBB0_4
LBB0_28:                                ;   in Loop: Header=BB0_6 Depth=1
	mov	w8, #1                          ; =0x1
	str	w8, [sp, #44]                   ; 4-byte Spill
	tbz	w22, #31, LBB0_5
	b	LBB0_4
LBB0_29:                                ;   in Loop: Header=BB0_6 Depth=1
	mov	w8, #1                          ; =0x1
	str	w8, [sp, #40]                   ; 4-byte Spill
	tbz	w22, #31, LBB0_5
	b	LBB0_4
LBB0_30:                                ;   in Loop: Header=BB0_6 Depth=1
	adrp	x8, __MergedGlobals@PAGE
	mov	w9, #1                          ; =0x1
	str	w9, [x8, __MergedGlobals@PAGEOFF]
	b	LBB0_4
LBB0_31:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF]
	cbnz	w8, LBB0_127
; %bb.32:
	str	xzr, [sp, #40]                  ; 8-byte Folded Spill
	str	wzr, [sp, #48]                  ; 4-byte Spill
	b	LBB0_38
LBB0_33:
Lloh0:
	adrp	x8, __MergedGlobals@PAGE
Lloh1:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF]
	cmp	w8, #0
	cset	w8, eq
	ldr	w9, [sp, #48]                   ; 4-byte Reload
	orr	w8, w8, w9
	tbz	w8, #0, LBB0_127
; %bb.34:
	ldr	w8, [sp, #56]                   ; 4-byte Reload
	tbz	w8, #0, LBB0_38
; %bb.35:
	mov	w19, #0                         ; =0x0
	mov	x20, #1527                      ; =0x5f7
	movk	x20, #17640, lsl #16
	movk	x20, #48262, lsl #32
	movk	x20, #52896, lsl #48
	mov	x24, #22359                     ; =0x5757
	movk	x24, #14592, lsl #16
	movk	x24, #38627, lsl #32
	movk	x24, #62820, lsl #48
	mov	x21, #21417                     ; =0x53a9
	movk	x21, #22027, lsl #16
	movk	x21, #50449, lsl #32
	movk	x21, #17285, lsl #48
	mov	x22, #23972                     ; =0x5da4
	movk	x22, #8134, lsl #16
	movk	x22, #60677, lsl #32
	movk	x22, #9056, lsl #48
	mov	x23, #-3689348814741910324      ; =0xcccccccccccccccc
	movk	x23, #52429
	mov	w25, #10                        ; =0xa
	mov	w26, #34464                     ; =0x86a0
	movk	w26, #1, lsl #16
LBB0_36:                                ; =>This Inner Loop Header: Depth=1
	eor	x8, x20, x24
	lsr	x9, x20, #58
	mul	x10, x24, x21
	umulh	x11, x24, x21
	mul	x12, x24, x22
	cmn	x10, #4
	madd	x12, x20, x21, x12
	cinc	x12, x12, hi
	add	x20, x12, x11
	add	x24, x10, #3
	ror	x8, x8, x9
	cmp	x8, #6
	b.lo	LBB0_36
; %bb.37:                               ;   in Loop: Header=BB0_36 Depth=1
	umulh	x9, x8, x23
	lsr	x9, x9, #3
	msub	w0, w9, w25, w8
	bl	_putchar
	add	w19, w19, #1
	cmp	w19, w26
	b.ne	LBB0_36
	b	LBB0_118
LBB0_38:
	mov	w21, #0                         ; =0x0
	mov	x22, #30787                     ; =0x7843
	movk	x22, #29108, lsl #16
	movk	x22, #23236, lsl #32
	movk	x22, #2684, lsl #48
	mov	w26, #34464                     ; =0x86a0
	movk	w26, #1, lsl #16
	b	LBB0_40
LBB0_39:                                ;   in Loop: Header=BB0_40 Depth=1
	add	w21, w21, #1
	cmp	w21, #3
	b.eq	LBB0_95
LBB0_40:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_72 Depth 2
                                        ;     Child Loop BB0_86 Depth 2
                                        ;       Child Loop BB0_89 Depth 3
                                        ;     Child Loop BB0_79 Depth 2
Ltmp17:                                 ; EH_LABEL
	mov	x0, #0                          ; =0x0
	mov	x1, #0                          ; =0x0
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp18:                                 ; EH_LABEL
; %bb.41:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp19:                                 ; EH_LABEL
	mov	w0, #42                         ; =0x2a
	mov	w1, #42                         ; =0x2a
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp20:                                 ; EH_LABEL
; %bb.42:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp21:                                 ; EH_LABEL
	mov	x0, #-42                        ; =0xffffffffffffffd6
	mov	x1, #-42                        ; =0xffffffffffffffd6
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp22:                                 ; EH_LABEL
; %bb.43:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp23:                                 ; EH_LABEL
	mov	x0, #9223372036854775807        ; =0x7fffffffffffffff
	mov	x1, #9223372036854775807        ; =0x7fffffffffffffff
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp24:                                 ; EH_LABEL
; %bb.44:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp25:                                 ; EH_LABEL
	mov	x0, #-9223372036854775808       ; =0x8000000000000000
	mov	x1, #-9223372036854775808       ; =0x8000000000000000
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp26:                                 ; EH_LABEL
; %bb.45:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp27:                                 ; EH_LABEL
	mov	x0, #-9223372036854775808       ; =0x8000000000000000
	mov	x1, #9223372036854775807        ; =0x7fffffffffffffff
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp28:                                 ; EH_LABEL
; %bb.46:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp29:                                 ; EH_LABEL
	mov	x0, #9223372036854775807        ; =0x7fffffffffffffff
	mov	x1, #-9223372036854775808       ; =0x8000000000000000
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp30:                                 ; EH_LABEL
; %bb.47:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp31:                                 ; EH_LABEL
	mov	w0, #100                        ; =0x64
	mov	w1, #101                        ; =0x65
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp32:                                 ; EH_LABEL
; %bb.48:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp33:                                 ; EH_LABEL
	mov	w0, #101                        ; =0x65
	mov	w1, #100                        ; =0x64
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp34:                                 ; EH_LABEL
; %bb.49:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp35:                                 ; EH_LABEL
	mov	x0, #-101                       ; =0xffffffffffffff9b
	mov	x1, #-100                       ; =0xffffffffffffff9c
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp36:                                 ; EH_LABEL
; %bb.50:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp37:                                 ; EH_LABEL
	mov	x0, #-100                       ; =0xffffffffffffff9c
	mov	x1, #-101                       ; =0xffffffffffffff9b
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp38:                                 ; EH_LABEL
; %bb.51:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp39:                                 ; EH_LABEL
	mov	x0, #-1                         ; =0xffffffffffffffff
	mov	x1, #0                          ; =0x0
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp40:                                 ; EH_LABEL
; %bb.52:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp41:                                 ; EH_LABEL
	mov	x0, #0                          ; =0x0
	mov	x1, #-1                         ; =0xffffffffffffffff
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp42:                                 ; EH_LABEL
; %bb.53:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp43:                                 ; EH_LABEL
	mov	x0, #0                          ; =0x0
	mov	w1, #1                          ; =0x1
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp44:                                 ; EH_LABEL
; %bb.54:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp45:                                 ; EH_LABEL
	mov	w0, #1                          ; =0x1
	mov	x1, #0                          ; =0x0
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp46:                                 ; EH_LABEL
; %bb.55:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp47:                                 ; EH_LABEL
	mov	x0, #-1                         ; =0xffffffffffffffff
	mov	w1, #1                          ; =0x1
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp48:                                 ; EH_LABEL
; %bb.56:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp49:                                 ; EH_LABEL
	mov	w0, #1                          ; =0x1
	mov	x1, #-1                         ; =0xffffffffffffffff
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp50:                                 ; EH_LABEL
; %bb.57:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp51:                                 ; EH_LABEL
	mov	x0, #1                          ; =0x1
	movk	x0, #32, lsl #48
	mov	x1, #2                          ; =0x2
	movk	x1, #32, lsl #48
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp52:                                 ; EH_LABEL
; %bb.58:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp53:                                 ; EH_LABEL
	mov	x0, #2                          ; =0x2
	movk	x0, #32, lsl #48
	mov	x1, #1                          ; =0x1
	movk	x1, #32, lsl #48
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp54:                                 ; EH_LABEL
; %bb.59:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp55:                                 ; EH_LABEL
	mov	x0, #9223372036854775806        ; =0x7ffffffffffffffe
	mov	x1, #9223372036854775807        ; =0x7fffffffffffffff
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp56:                                 ; EH_LABEL
; %bb.60:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp58:                                 ; EH_LABEL
	mov	x0, #-9223372036854775808       ; =0x8000000000000000
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp59:                                 ; EH_LABEL
; %bb.61:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp60:                                 ; EH_LABEL
	mov	x0, #-101                       ; =0xffffffffffffff9b
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp61:                                 ; EH_LABEL
; %bb.62:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp62:                                 ; EH_LABEL
	mov	x0, #-100                       ; =0xffffffffffffff9c
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp63:                                 ; EH_LABEL
; %bb.63:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp64:                                 ; EH_LABEL
	mov	x0, #-99                        ; =0xffffffffffffff9d
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp65:                                 ; EH_LABEL
; %bb.64:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp66:                                 ; EH_LABEL
	mov	w0, #99                         ; =0x63
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp67:                                 ; EH_LABEL
; %bb.65:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp68:                                 ; EH_LABEL
	mov	w0, #100                        ; =0x64
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp69:                                 ; EH_LABEL
; %bb.66:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp70:                                 ; EH_LABEL
	mov	w0, #101                        ; =0x65
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp71:                                 ; EH_LABEL
; %bb.67:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp72:                                 ; EH_LABEL
	mov	x0, #9223372036854775807        ; =0x7fffffffffffffff
	mov	x1, #0                          ; =0x0
	mov	w2, #1                          ; =0x1
	bl	__ZL6verify4Pairi
Ltmp73:                                 ; EH_LABEL
; %bb.68:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp75:                                 ; EH_LABEL
	add	x0, sp, #88
	mov	x1, x21
	bl	__ZL15generate_corpusim
Ltmp76:                                 ; EH_LABEL
; %bb.69:                               ;   in Loop: Header=BB0_40 Depth=1
Ltmp78:                                 ; EH_LABEL
	add	x0, sp, #64
	mov	x1, x21
	bl	__ZL15generate_corpusim
Ltmp79:                                 ; EH_LABEL
; %bb.70:                               ;   in Loop: Header=BB0_40 Depth=1
	ldp	x8, x9, [sp, #88]
	cmp	x9, x8
	b.eq	LBB0_76
; %bb.71:                               ;   in Loop: Header=BB0_40 Depth=1
	mov	x19, #0                         ; =0x0
	mov	x20, #0                         ; =0x0
LBB0_72:                                ;   Parent Loop BB0_40 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	x0, [x8, x19]
	ldr	x9, [sp, #64]
	ldr	x10, [x9, x19]
	cmp	x0, x10
	b.ne	LBB0_122
; %bb.73:                               ;   in Loop: Header=BB0_72 Depth=2
	add	x8, x8, x19
	ldr	x1, [x8, #8]
	add	x8, x9, x19
	ldr	x8, [x8, #8]
	cmp	x1, x8
	b.ne	LBB0_122
; %bb.74:                               ;   in Loop: Header=BB0_72 Depth=2
Ltmp87:                                 ; EH_LABEL
	mov	x2, x21
	bl	__ZL6verify4Pairi
Ltmp88:                                 ; EH_LABEL
; %bb.75:                               ;   in Loop: Header=BB0_72 Depth=2
	add	x20, x20, #1
	ldp	x8, x9, [sp, #88]
	sub	x10, x9, x8
	add	x19, x19, #16
	cmp	x20, x10, asr #4
	b.lo	LBB0_72
LBB0_76:                                ;   in Loop: Header=BB0_40 Depth=1
	str	w21, [sp, #52]                  ; 4-byte Spill
	cmp	w21, #1
Lloh2:
	adrp	x10, __ZL5modes@PAGE
Lloh3:
	add	x10, x10, __ZL5modes@PAGEOFF
Lloh4:
	adrp	x11, __ZL12policy_modes@PAGE
Lloh5:
	add	x11, x11, __ZL12policy_modes@PAGEOFF
	csel	x24, x11, x10, eq
	add	x10, x10, #40
	add	x11, x11, #40
	csel	x10, x11, x10, eq
	cmp	x24, x10
	b.eq	LBB0_80
; %bb.77:                               ;   in Loop: Header=BB0_40 Depth=1
	cmp	x8, x9
	b.eq	LBB0_79
; %bb.78:                               ;   in Loop: Header=BB0_40 Depth=1
	str	x10, [sp, #56]                  ; 8-byte Spill
	b	LBB0_86
LBB0_79:                                ;   Parent Loop BB0_40 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	adrp	x8, __MergedGlobals@PAGE+8
	str	xzr, [x8, __MergedGlobals@PAGEOFF+8]
	add	x24, x24, #8
	cmp	x24, x10
	b.ne	LBB0_79
LBB0_80:                                ;   in Loop: Header=BB0_40 Depth=1
	ldr	x0, [sp, #64]
	cbz	x0, LBB0_82
; %bb.81:                               ;   in Loop: Header=BB0_40 Depth=1
	str	x0, [sp, #72]
	ldr	x8, [sp, #80]
	sub	x1, x8, x0
	bl	__ZdlPvm
LBB0_82:                                ;   in Loop: Header=BB0_40 Depth=1
	ldr	x0, [sp, #88]
	ldr	w21, [sp, #52]                  ; 4-byte Reload
	cbz	x0, LBB0_39
; %bb.83:                               ;   in Loop: Header=BB0_40 Depth=1
	str	x0, [sp, #96]
	ldr	x8, [sp, #104]
	sub	x1, x8, x0
	bl	__ZdlPvm
	b	LBB0_39
LBB0_84:                                ;   in Loop: Header=BB0_86 Depth=2
	mov	x21, #0                         ; =0x0
LBB0_85:                                ;   in Loop: Header=BB0_86 Depth=2
	adrp	x8, __MergedGlobals@PAGE+8
	str	x21, [x8, __MergedGlobals@PAGEOFF+8]
	add	x24, x24, #8
	ldr	x10, [sp, #56]                  ; 8-byte Reload
	cmp	x24, x10
	b.eq	LBB0_80
LBB0_86:                                ;   Parent Loop BB0_40 Depth=1
                                        ; =>  This Loop Header: Depth=2
                                        ;       Child Loop BB0_89 Depth 3
	ldp	x25, x27, [sp, #88]
	cmp	x25, x27
	b.eq	LBB0_84
; %bb.87:                               ;   in Loop: Header=BB0_86 Depth=2
	mov	x21, #0                         ; =0x0
	ldr	x28, [x24]
	mov	x23, #-1                        ; =0xffffffffffffffff
	mov	w19, #1                         ; =0x1
	b	LBB0_89
LBB0_88:                                ;   in Loop: Header=BB0_89 Depth=3
	add	x21, x0, x21
	add	x25, x25, #16
	sub	x23, x23, #1
	add	x19, x19, #1
	cmp	x25, x27
	b.eq	LBB0_85
LBB0_89:                                ;   Parent Loop BB0_40 Depth=1
                                        ;     Parent Loop BB0_86 Depth=2
                                        ; =>    This Inner Loop Header: Depth=3
	ldp	x0, x1, [x25]
Ltmp90:                                 ; EH_LABEL
	blr	x28
Ltmp91:                                 ; EH_LABEL
; %bb.90:                               ;   in Loop: Header=BB0_89 Depth=3
	lsr	x8, x19, #5
	umulh	x8, x8, x22
	lsr	x8, x8, #7
	madd	x8, x8, x26, x23
	cbnz	x8, LBB0_88
; %bb.91:                               ;   in Loop: Header=BB0_89 Depth=3
	movi.2d	v0, #0000000000000000
	stp	q0, q0, [sp, #224]
	stp	q0, q0, [sp, #192]
	stp	q0, q0, [sp, #160]
	stp	q0, q0, [sp, #128]
	str	q0, [sp, #112]
Ltmp92:                                 ; EH_LABEL
	add	x1, sp, #112
	mov	x20, x0
	mov	w0, #0                          ; =0x0
	bl	_getrusage
	mov	x8, x0
	mov	x0, x20
Ltmp93:                                 ; EH_LABEL
; %bb.92:                               ;   in Loop: Header=BB0_89 Depth=3
	cbnz	w8, LBB0_120
; %bb.93:                               ;   in Loop: Header=BB0_89 Depth=3
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
	b.lo	LBB0_88
; %bb.94:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x21, x0
Ltmp98:                                 ; EH_LABEL
Lloh6:
	adrp	x1, l_.str.18@PAGE
Lloh7:
	add	x1, x1, l_.str.18@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp99:                                 ; EH_LABEL
	b	LBB0_121
LBB0_95:
Lloh8:
	adrp	x8, ___stderrp@GOTPAGE
Lloh9:
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
Lloh10:
	ldr	x3, [x8]
Lloh11:
	adrp	x0, l_.str.12@PAGE
Lloh12:
	add	x0, x0, l_.str.12@PAGEOFF
	mov	w1, #70                         ; =0x46
	mov	w2, #1                          ; =0x1
	bl	_fwrite
	ldr	w8, [sp, #44]                   ; 4-byte Reload
	tbz	w8, #0, LBB0_117
; %bb.96:
Ltmp116:                                ; EH_LABEL
	add	x0, sp, #112
	mov	w1, #0                          ; =0x0
	bl	__ZL15generate_corpusim
Ltmp117:                                ; EH_LABEL
; %bb.97:
	ldp	x19, x21, [sp, #112]
	cmp	x19, x21
	b.eq	LBB0_101
; %bb.98:
Lloh13:
	adrp	x20, l_.str.13@PAGE
Lloh14:
	add	x20, x20, l_.str.13@PAGEOFF
LBB0_99:                                ; =>This Inner Loop Header: Depth=1
	ldp	x8, x9, [x19], #16
	stp	x8, x9, [sp, #8]
	str	xzr, [sp]
	mov	x0, x20
	bl	_printf
	cmp	x19, x21
	b.ne	LBB0_99
; %bb.100:
	ldr	x19, [sp, #112]
LBB0_101:
	cbz	x19, LBB0_103
; %bb.102:
	str	x19, [sp, #120]
	ldr	x8, [sp, #128]
	sub	x1, x8, x19
	mov	x0, x19
	bl	__ZdlPvm
LBB0_103:
Ltmp118:                                ; EH_LABEL
	add	x0, sp, #112
	mov	w1, #1                          ; =0x1
	bl	__ZL15generate_corpusim
Ltmp119:                                ; EH_LABEL
; %bb.104:
	ldp	x19, x21, [sp, #112]
	cmp	x19, x21
	b.eq	LBB0_108
; %bb.105:
	mov	w22, #1                         ; =0x1
Lloh15:
	adrp	x20, l_.str.13@PAGE
Lloh16:
	add	x20, x20, l_.str.13@PAGEOFF
LBB0_106:                               ; =>This Inner Loop Header: Depth=1
	ldp	x8, x9, [x19], #16
	stp	x8, x9, [sp, #8]
	str	x22, [sp]
	mov	x0, x20
	bl	_printf
	cmp	x19, x21
	b.ne	LBB0_106
; %bb.107:
	ldr	x19, [sp, #112]
LBB0_108:
	cbz	x19, LBB0_110
; %bb.109:
	str	x19, [sp, #120]
	ldr	x8, [sp, #128]
	sub	x1, x8, x19
	mov	x0, x19
	bl	__ZdlPvm
LBB0_110:
Ltmp120:                                ; EH_LABEL
	add	x0, sp, #112
	mov	w1, #2                          ; =0x2
	bl	__ZL15generate_corpusim
Ltmp121:                                ; EH_LABEL
; %bb.111:
	ldp	x19, x21, [sp, #112]
	cmp	x19, x21
	b.eq	LBB0_115
; %bb.112:
	mov	w22, #2                         ; =0x2
Lloh17:
	adrp	x20, l_.str.13@PAGE
Lloh18:
	add	x20, x20, l_.str.13@PAGEOFF
LBB0_113:                               ; =>This Inner Loop Header: Depth=1
	ldp	x8, x9, [x19], #16
	stp	x8, x9, [sp, #8]
	str	x22, [sp]
	mov	x0, x20
	bl	_printf
	cmp	x19, x21
	b.ne	LBB0_113
; %bb.114:
	ldr	x19, [sp, #112]
LBB0_115:
	cbz	x19, LBB0_118
; %bb.116:
	str	x19, [sp, #120]
	ldr	x8, [sp, #128]
	sub	x1, x8, x19
	mov	x0, x19
	bl	__ZdlPvm
	b	LBB0_118
LBB0_117:
	ldr	w8, [sp, #48]                   ; 4-byte Reload
	tbz	w8, #0, LBB0_129
LBB0_118:
	mov	w0, #0                          ; =0x0
LBB0_119:
	ldp	x29, x30, [sp, #336]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #320]            ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #304]            ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #288]            ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #272]            ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #256]            ; 16-byte Folded Reload
	add	sp, sp, #352
	ret
LBB0_120:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x21, x0
Ltmp95:                                 ; EH_LABEL
Lloh19:
	adrp	x1, l_.str.17@PAGE
Lloh20:
	add	x1, x1, l_.str.17@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp96:                                 ; EH_LABEL
LBB0_121:
Ltmp101:                                ; EH_LABEL
Lloh21:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
Lloh22:
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
Lloh23:
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
Lloh24:
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	mov	x0, x21
	bl	___cxa_throw
Ltmp102:                                ; EH_LABEL
	b	LBB0_134
LBB0_122:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x21, x0
Ltmp81:                                 ; EH_LABEL
Lloh25:
	adrp	x1, l_.str.11@PAGE
Lloh26:
	add	x1, x1, l_.str.11@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp82:                                 ; EH_LABEL
; %bb.123:
Ltmp84:                                 ; EH_LABEL
Lloh27:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
Lloh28:
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
Lloh29:
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
Lloh30:
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	mov	x0, x21
	bl	___cxa_throw
Ltmp85:                                 ; EH_LABEL
	b	LBB0_134
LBB0_124:
Ltmp123:                                ; EH_LABEL
	bl	__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB9nqe220101Ev
Ltmp124:                                ; EH_LABEL
	b	LBB0_134
LBB0_125:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x21, x0
Ltmp5:                                  ; EH_LABEL
Lloh31:
	adrp	x1, l_.str.7@PAGE
Lloh32:
	add	x1, x1, l_.str.7@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp6:                                  ; EH_LABEL
; %bb.126:
Ltmp8:                                  ; EH_LABEL
Lloh33:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
Lloh34:
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
Lloh35:
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
Lloh36:
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	mov	x0, x21
	bl	___cxa_throw
Ltmp9:                                  ; EH_LABEL
	b	LBB0_134
LBB0_127:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x21, x0
Ltmp11:                                 ; EH_LABEL
Lloh37:
	adrp	x1, l_.str.8@PAGE
Lloh38:
	add	x1, x1, l_.str.8@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp12:                                 ; EH_LABEL
; %bb.128:
Ltmp14:                                 ; EH_LABEL
Lloh39:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
Lloh40:
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
Lloh41:
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
Lloh42:
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	mov	x0, x21
	bl	___cxa_throw
Ltmp15:                                 ; EH_LABEL
	b	LBB0_134
LBB0_129:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x21, x0
	ldr	w8, [sp, #40]                   ; 4-byte Reload
	tbz	w8, #0, LBB0_132
; %bb.130:
Ltmp110:                                ; EH_LABEL
Lloh43:
	adrp	x1, l_.str.15@PAGE
Lloh44:
	add	x1, x1, l_.str.15@PAGEOFF
	mov	x0, x21
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp111:                                ; EH_LABEL
; %bb.131:
Ltmp113:                                ; EH_LABEL
Lloh45:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
Lloh46:
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
Lloh47:
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
Lloh48:
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	mov	x0, x21
	bl	___cxa_throw
Ltmp114:                                ; EH_LABEL
	b	LBB0_134
LBB0_132:
Ltmp104:                                ; EH_LABEL
Lloh49:
	adrp	x1, l_.str.14@PAGE
Lloh50:
	add	x1, x1, l_.str.14@PAGEOFF
	mov	x0, x21
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp105:                                ; EH_LABEL
; %bb.133:
Ltmp107:                                ; EH_LABEL
Lloh51:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
Lloh52:
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
Lloh53:
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
Lloh54:
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	mov	x0, x21
	bl	___cxa_throw
Ltmp108:                                ; EH_LABEL
LBB0_134:
	brk	#0x1
LBB0_135:
Ltmp109:                                ; EH_LABEL
	b	LBB0_156
LBB0_136:
Ltmp106:                                ; EH_LABEL
	b	LBB0_140
LBB0_137:
Ltmp115:                                ; EH_LABEL
	b	LBB0_156
LBB0_138:
Ltmp112:                                ; EH_LABEL
	b	LBB0_140
LBB0_139:
Ltmp13:                                 ; EH_LABEL
LBB0_140:
	mov	x19, x1
	mov	x20, x0
	mov	x0, x21
	bl	___cxa_free_exception
	b	LBB0_168
LBB0_141:
Ltmp122:                                ; EH_LABEL
	b	LBB0_156
LBB0_142:
Ltmp16:                                 ; EH_LABEL
	b	LBB0_156
LBB0_143:
Ltmp10:                                 ; EH_LABEL
	mov	x19, x1
	mov	x20, x0
	b	LBB0_145
LBB0_144:
Ltmp7:                                  ; EH_LABEL
	mov	x19, x1
	mov	x20, x0
	mov	x0, x21
	bl	___cxa_free_exception
LBB0_145:
	tbz	w22, #31, LBB0_168
; %bb.146:
	ldr	x0, [sp, #112]
	ldr	x8, [sp, #128]
	and	x1, x8, #0x7fffffffffffffff
	b	LBB0_167
LBB0_147:
Ltmp4:                                  ; EH_LABEL
	b	LBB0_156
LBB0_148:
Ltmp125:                                ; EH_LABEL
	b	LBB0_156
LBB0_149:
Ltmp80:                                 ; EH_LABEL
	mov	x19, x1
	mov	x20, x0
	b	LBB0_165
LBB0_150:
Ltmp77:                                 ; EH_LABEL
	b	LBB0_156
LBB0_151:
Ltmp74:                                 ; EH_LABEL
	b	LBB0_156
LBB0_152:
Ltmp89:                                 ; EH_LABEL
	b	LBB0_162
LBB0_153:
Ltmp86:                                 ; EH_LABEL
	b	LBB0_162
LBB0_154:
Ltmp83:                                 ; EH_LABEL
	b	LBB0_159
LBB0_155:
Ltmp57:                                 ; EH_LABEL
LBB0_156:
	mov	x19, x1
	mov	x20, x0
	b	LBB0_168
LBB0_157:
Ltmp97:                                 ; EH_LABEL
	b	LBB0_159
LBB0_158:
Ltmp100:                                ; EH_LABEL
LBB0_159:
	mov	x19, x1
	mov	x20, x0
	mov	x0, x21
	bl	___cxa_free_exception
	b	LBB0_163
LBB0_160:
Ltmp103:                                ; EH_LABEL
	b	LBB0_162
LBB0_161:
Ltmp94:                                 ; EH_LABEL
LBB0_162:
	mov	x19, x1
	mov	x20, x0
LBB0_163:
	ldr	x0, [sp, #64]
	cbz	x0, LBB0_165
; %bb.164:
	str	x0, [sp, #72]
	ldr	x8, [sp, #80]
	sub	x1, x8, x0
	bl	__ZdlPvm
LBB0_165:
	ldr	x0, [sp, #88]
	cbz	x0, LBB0_168
; %bb.166:
	str	x0, [sp, #96]
	ldr	x8, [sp, #104]
	sub	x1, x8, x0
LBB0_167:
	bl	__ZdlPvm
LBB0_168:
	mov	x0, x20
	cmp	w19, #1
	b.ne	LBB0_170
; %bb.169:
	bl	___cxa_begin_catch
Lloh55:
	adrp	x8, ___stderrp@GOTPAGE
Lloh56:
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
Lloh57:
	ldr	x19, [x8]
	ldr	x8, [x0]
	ldr	x8, [x8, #16]
	blr	x8
	str	x0, [sp]
Lloh58:
	adrp	x1, l_.str.16@PAGE
Lloh59:
	add	x1, x1, l_.str.16@PAGEOFF
	mov	x0, x19
	bl	_fprintf
	bl	___cxa_end_catch
	mov	w0, #6                          ; =0x6
	b	LBB0_119
LBB0_170:
	bl	__Unwind_Resume
	.loh AdrpLdr	Lloh0, Lloh1
	.loh AdrpAdd	Lloh4, Lloh5
	.loh AdrpAdd	Lloh2, Lloh3
	.loh AdrpAdd	Lloh6, Lloh7
	.loh AdrpAdd	Lloh11, Lloh12
	.loh AdrpLdrGotLdr	Lloh8, Lloh9, Lloh10
	.loh AdrpAdd	Lloh13, Lloh14
	.loh AdrpAdd	Lloh15, Lloh16
	.loh AdrpAdd	Lloh17, Lloh18
	.loh AdrpAdd	Lloh19, Lloh20
	.loh AdrpLdrGot	Lloh23, Lloh24
	.loh AdrpLdrGot	Lloh21, Lloh22
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
	.loh AdrpAdd	Lloh58, Lloh59
	.loh AdrpLdrGotLdr	Lloh55, Lloh56, Lloh57
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
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp2-Lfunc_begin0             ; >> Call Site 2 <<
	.uleb128 Ltmp3-Ltmp2                    ;   Call between Ltmp2 and Ltmp3
	.uleb128 Ltmp4-Lfunc_begin0             ;     jumps to Ltmp4
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp3-Lfunc_begin0             ; >> Call Site 3 <<
	.uleb128 Ltmp17-Ltmp3                   ;   Call between Ltmp3 and Ltmp17
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp17-Lfunc_begin0            ; >> Call Site 4 <<
	.uleb128 Ltmp56-Ltmp17                  ;   Call between Ltmp17 and Ltmp56
	.uleb128 Ltmp57-Lfunc_begin0            ;     jumps to Ltmp57
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp58-Lfunc_begin0            ; >> Call Site 5 <<
	.uleb128 Ltmp73-Ltmp58                  ;   Call between Ltmp58 and Ltmp73
	.uleb128 Ltmp74-Lfunc_begin0            ;     jumps to Ltmp74
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp75-Lfunc_begin0            ; >> Call Site 6 <<
	.uleb128 Ltmp76-Ltmp75                  ;   Call between Ltmp75 and Ltmp76
	.uleb128 Ltmp77-Lfunc_begin0            ;     jumps to Ltmp77
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp78-Lfunc_begin0            ; >> Call Site 7 <<
	.uleb128 Ltmp79-Ltmp78                  ;   Call between Ltmp78 and Ltmp79
	.uleb128 Ltmp80-Lfunc_begin0            ;     jumps to Ltmp80
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp87-Lfunc_begin0            ; >> Call Site 8 <<
	.uleb128 Ltmp88-Ltmp87                  ;   Call between Ltmp87 and Ltmp88
	.uleb128 Ltmp89-Lfunc_begin0            ;     jumps to Ltmp89
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp90-Lfunc_begin0            ; >> Call Site 9 <<
	.uleb128 Ltmp93-Ltmp90                  ;   Call between Ltmp90 and Ltmp93
	.uleb128 Ltmp94-Lfunc_begin0            ;     jumps to Ltmp94
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp93-Lfunc_begin0            ; >> Call Site 10 <<
	.uleb128 Ltmp98-Ltmp93                  ;   Call between Ltmp93 and Ltmp98
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp98-Lfunc_begin0            ; >> Call Site 11 <<
	.uleb128 Ltmp99-Ltmp98                  ;   Call between Ltmp98 and Ltmp99
	.uleb128 Ltmp100-Lfunc_begin0           ;     jumps to Ltmp100
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp116-Lfunc_begin0           ; >> Call Site 12 <<
	.uleb128 Ltmp121-Ltmp116                ;   Call between Ltmp116 and Ltmp121
	.uleb128 Ltmp122-Lfunc_begin0           ;     jumps to Ltmp122
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp121-Lfunc_begin0           ; >> Call Site 13 <<
	.uleb128 Ltmp95-Ltmp121                 ;   Call between Ltmp121 and Ltmp95
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp95-Lfunc_begin0            ; >> Call Site 14 <<
	.uleb128 Ltmp96-Ltmp95                  ;   Call between Ltmp95 and Ltmp96
	.uleb128 Ltmp97-Lfunc_begin0            ;     jumps to Ltmp97
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp101-Lfunc_begin0           ; >> Call Site 15 <<
	.uleb128 Ltmp102-Ltmp101                ;   Call between Ltmp101 and Ltmp102
	.uleb128 Ltmp103-Lfunc_begin0           ;     jumps to Ltmp103
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp102-Lfunc_begin0           ; >> Call Site 16 <<
	.uleb128 Ltmp81-Ltmp102                 ;   Call between Ltmp102 and Ltmp81
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp81-Lfunc_begin0            ; >> Call Site 17 <<
	.uleb128 Ltmp82-Ltmp81                  ;   Call between Ltmp81 and Ltmp82
	.uleb128 Ltmp83-Lfunc_begin0            ;     jumps to Ltmp83
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp84-Lfunc_begin0            ; >> Call Site 18 <<
	.uleb128 Ltmp85-Ltmp84                  ;   Call between Ltmp84 and Ltmp85
	.uleb128 Ltmp86-Lfunc_begin0            ;     jumps to Ltmp86
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp123-Lfunc_begin0           ; >> Call Site 19 <<
	.uleb128 Ltmp124-Ltmp123                ;   Call between Ltmp123 and Ltmp124
	.uleb128 Ltmp125-Lfunc_begin0           ;     jumps to Ltmp125
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp124-Lfunc_begin0           ; >> Call Site 20 <<
	.uleb128 Ltmp5-Ltmp124                  ;   Call between Ltmp124 and Ltmp5
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp5-Lfunc_begin0             ; >> Call Site 21 <<
	.uleb128 Ltmp6-Ltmp5                    ;   Call between Ltmp5 and Ltmp6
	.uleb128 Ltmp7-Lfunc_begin0             ;     jumps to Ltmp7
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp8-Lfunc_begin0             ; >> Call Site 22 <<
	.uleb128 Ltmp9-Ltmp8                    ;   Call between Ltmp8 and Ltmp9
	.uleb128 Ltmp10-Lfunc_begin0            ;     jumps to Ltmp10
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp9-Lfunc_begin0             ; >> Call Site 23 <<
	.uleb128 Ltmp11-Ltmp9                   ;   Call between Ltmp9 and Ltmp11
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp11-Lfunc_begin0            ; >> Call Site 24 <<
	.uleb128 Ltmp12-Ltmp11                  ;   Call between Ltmp11 and Ltmp12
	.uleb128 Ltmp13-Lfunc_begin0            ;     jumps to Ltmp13
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp14-Lfunc_begin0            ; >> Call Site 25 <<
	.uleb128 Ltmp15-Ltmp14                  ;   Call between Ltmp14 and Ltmp15
	.uleb128 Ltmp16-Lfunc_begin0            ;     jumps to Ltmp16
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp15-Lfunc_begin0            ; >> Call Site 26 <<
	.uleb128 Ltmp110-Ltmp15                 ;   Call between Ltmp15 and Ltmp110
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp110-Lfunc_begin0           ; >> Call Site 27 <<
	.uleb128 Ltmp111-Ltmp110                ;   Call between Ltmp110 and Ltmp111
	.uleb128 Ltmp112-Lfunc_begin0           ;     jumps to Ltmp112
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp113-Lfunc_begin0           ; >> Call Site 28 <<
	.uleb128 Ltmp114-Ltmp113                ;   Call between Ltmp113 and Ltmp114
	.uleb128 Ltmp115-Lfunc_begin0           ;     jumps to Ltmp115
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp104-Lfunc_begin0           ; >> Call Site 29 <<
	.uleb128 Ltmp105-Ltmp104                ;   Call between Ltmp104 and Ltmp105
	.uleb128 Ltmp106-Lfunc_begin0           ;     jumps to Ltmp106
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp107-Lfunc_begin0           ; >> Call Site 30 <<
	.uleb128 Ltmp108-Ltmp107                ;   Call between Ltmp107 and Ltmp108
	.uleb128 Ltmp109-Lfunc_begin0           ;     jumps to Ltmp109
	.byte	3                               ;   On action: 2
	.uleb128 Ltmp108-Lfunc_begin0           ; >> Call Site 31 <<
	.uleb128 Lfunc_end0-Ltmp108             ;   Call between Ltmp108 and Lfunc_end0
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
Lcst_end0:
	.byte	0                               ; >> Action Record 1 <<
                                        ;   Cleanup
	.byte	0                               ;   No further actions
	.byte	1                               ; >> Action Record 2 <<
                                        ;   Catch TypeInfo 1
	.byte	125                             ;   Continue to action 1
	.p2align	2, 0x0
                                        ; >> Catch TypeInfos <<
Ltmp156:                                ; TypeInfo 1
	.long	__ZTISt9exception@GOT-Ltmp156
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
	b.hs	LBB1_4
; %bb.2:
	ldp	x29, x30, [sp, #160]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #144]            ; 16-byte Folded Reload
	add	sp, sp, #176
	ret
LBB1_3:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x19, x0
Ltmp126:                                ; EH_LABEL
Lloh60:
	adrp	x1, l_.str.17@PAGE
Lloh61:
	add	x1, x1, l_.str.17@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp127:                                ; EH_LABEL
	b	LBB1_5
LBB1_4:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x19, x0
Ltmp129:                                ; EH_LABEL
Lloh62:
	adrp	x1, l_.str.18@PAGE
Lloh63:
	add	x1, x1, l_.str.18@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp130:                                ; EH_LABEL
LBB1_5:
Lloh64:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
Lloh65:
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
Lloh66:
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
Lloh67:
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	mov	x0, x19
	bl	___cxa_throw
LBB1_6:
Ltmp131:                                ; EH_LABEL
	mov	x20, x0
	mov	x0, x19
	bl	___cxa_free_exception
	mov	x0, x20
	bl	__Unwind_Resume
LBB1_7:
Ltmp128:                                ; EH_LABEL
	mov	x20, x0
	mov	x0, x19
	bl	___cxa_free_exception
	mov	x0, x20
	bl	__Unwind_Resume
	.loh AdrpAdd	Lloh60, Lloh61
	.loh AdrpAdd	Lloh62, Lloh63
	.loh AdrpLdrGot	Lloh66, Lloh67
	.loh AdrpLdrGot	Lloh64, Lloh65
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
	b.ne	LBB2_21
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
	cmp	w0, w21
	b.ne	LBB2_44
; %bb.2:
	adrp	x23, __MergedGlobals@PAGE
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_44
; %bb.3:
	cmp	w8, #2
	b.eq	LBB2_43
; %bb.4:
	eor	x8, x20, x19, lsl #1
	add	x22, x22, x8
	mov	x0, x20
	mov	x1, x19
	bl	__Z16candidate_policyxx
	cmp	x0, x22
	b.ne	LBB2_43
; %bb.5:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #1                          ; =0x1
	bl	__Z15probe_baseline1xxb
	cmp	w0, w21
	b.ne	LBB2_44
; %bb.6:
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_44
; %bb.7:
	cmp	w8, #2
	b.eq	LBB2_43
; %bb.8:
	mov	x0, x20
	mov	x1, x19
	bl	__Z16baseline1_policyxx
	cmp	x0, x22
	b.ne	LBB2_43
; %bb.9:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #1                          ; =0x1
	bl	__Z15probe_baseline2xxb
	cmp	w0, w21
	b.ne	LBB2_44
; %bb.10:
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_44
; %bb.11:
	cmp	w8, #2
	b.eq	LBB2_43
; %bb.12:
	mov	x0, x20
	mov	x1, x19
	bl	__Z16baseline2_policyxx
	cmp	x0, x22
	b.ne	LBB2_43
; %bb.13:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #1                          ; =0x1
	bl	__Z15probe_baseline3xxb
	cmp	w0, w21
	b.ne	LBB2_44
; %bb.14:
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_44
; %bb.15:
	cmp	w8, #2
	b.eq	LBB2_43
; %bb.16:
	mov	x0, x20
	mov	x1, x19
	bl	__Z16baseline3_policyxx
	cmp	x0, x22
	b.ne	LBB2_43
; %bb.17:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #1                          ; =0x1
	bl	__Z15probe_baseline4xxb
	cmp	w0, w21
	b.ne	LBB2_44
; %bb.18:
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_44
; %bb.19:
	cmp	w8, #2
	b.eq	LBB2_43
; %bb.20:
	mov	x0, x20
	mov	x1, x19
	bl	__Z16baseline4_policyxx
	b	LBB2_41
LBB2_21:
	cmp	x20, x19
	cset	w8, gt
	csinv	w21, w8, wzr, ge
	mov	x0, x20
	mov	x1, x19
	mov	w2, #0                          ; =0x0
	bl	__Z15probe_candidatexxb
	cmp	w0, w21
	b.ne	LBB2_44
; %bb.22:
	adrp	x23, __MergedGlobals@PAGE
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_44
; %bb.23:
	cmp	w8, #2
	b.eq	LBB2_43
; %bb.24:
	eor	x8, x20, x19, lsl #1
	add	w9, w21, #2
	add	x22, x8, x9
	mov	x0, x20
	mov	x1, x19
	bl	__Z9candidatexx
	cmp	x0, x22
	b.ne	LBB2_43
; %bb.25:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #0                          ; =0x0
	bl	__Z15probe_baseline1xxb
	cmp	w0, w21
	b.ne	LBB2_44
; %bb.26:
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_44
; %bb.27:
	cmp	w8, #2
	b.eq	LBB2_43
; %bb.28:
	mov	x0, x20
	mov	x1, x19
	bl	__Z9baseline1xx
	cmp	x0, x22
	b.ne	LBB2_43
; %bb.29:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #0                          ; =0x0
	bl	__Z15probe_baseline2xxb
	cmp	w0, w21
	b.ne	LBB2_44
; %bb.30:
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_44
; %bb.31:
	cmp	w8, #2
	b.eq	LBB2_43
; %bb.32:
	mov	x0, x20
	mov	x1, x19
	bl	__Z9baseline2xx
	cmp	x0, x22
	b.ne	LBB2_43
; %bb.33:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #0                          ; =0x0
	bl	__Z15probe_baseline3xxb
	cmp	w0, w21
	b.ne	LBB2_44
; %bb.34:
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_44
; %bb.35:
	cmp	w8, #2
	b.eq	LBB2_43
; %bb.36:
	mov	x0, x20
	mov	x1, x19
	bl	__Z9baseline3xx
	cmp	x0, x22
	b.ne	LBB2_43
; %bb.37:
	mov	x0, x20
	mov	x1, x19
	mov	w2, #0                          ; =0x0
	bl	__Z15probe_baseline4xxb
	cmp	w0, w21
	b.ne	LBB2_44
; %bb.38:
	ldr	w8, [x23, __MergedGlobals@PAGEOFF]
	cmp	w8, #1
	b.eq	LBB2_44
; %bb.39:
	cmp	w8, #2
	b.eq	LBB2_43
; %bb.40:
	mov	x0, x20
	mov	x1, x19
	bl	__Z9baseline4xx
LBB2_41:
	cmp	x0, x22
	b.ne	LBB2_43
; %bb.42:
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB2_43:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x19, x0
Ltmp135:                                ; EH_LABEL
Lloh68:
	adrp	x1, l_.str.23@PAGE
Lloh69:
	add	x1, x1, l_.str.23@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp136:                                ; EH_LABEL
	b	LBB2_45
LBB2_44:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x19, x0
Ltmp132:                                ; EH_LABEL
Lloh70:
	adrp	x1, l_.str.22@PAGE
Lloh71:
	add	x1, x1, l_.str.22@PAGEOFF
	bl	__ZNSt13runtime_errorC1EPKc
Ltmp133:                                ; EH_LABEL
LBB2_45:
Lloh72:
	adrp	x1, __ZTISt13runtime_error@GOTPAGE
Lloh73:
	ldr	x1, [x1, __ZTISt13runtime_error@GOTPAGEOFF]
Lloh74:
	adrp	x2, __ZNSt13runtime_errorD1Ev@GOTPAGE
Lloh75:
	ldr	x2, [x2, __ZNSt13runtime_errorD1Ev@GOTPAGEOFF]
	mov	x0, x19
	bl	___cxa_throw
LBB2_46:
Ltmp134:                                ; EH_LABEL
	mov	x20, x0
	mov	x0, x19
	bl	___cxa_free_exception
	mov	x0, x20
	bl	__Unwind_Resume
LBB2_47:
Ltmp137:                                ; EH_LABEL
	mov	x20, x0
	mov	x0, x19
	bl	___cxa_free_exception
	mov	x0, x20
	bl	__Unwind_Resume
	.loh AdrpAdd	Lloh68, Lloh69
	.loh AdrpAdd	Lloh70, Lloh71
	.loh AdrpLdrGot	Lloh74, Lloh75
	.loh AdrpLdrGot	Lloh72, Lloh73
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
	stp	d11, d10, [sp, #48]             ; 16-byte Folded Spill
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
	.cfi_offset b10, -120
	.cfi_offset b11, -128
	tbnz	w1, #31, LBB3_25
; %bb.1:
	mov	x20, x1
	mov	x19, x0
	stp	xzr, xzr, [x0]
	str	xzr, [x0, #16]
Ltmp138:                                ; EH_LABEL
	mov	w0, #28928                      ; =0x7100
	movk	w0, #2, lsl #16
	bl	__Znwm
Ltmp139:                                ; EH_LABEL
; %bb.2:
	mov	x21, #0                         ; =0x0
	mov	x22, #21417                     ; =0x53a9
	movk	x22, #22027, lsl #16
	movk	x22, #50449, lsl #32
	movk	x22, #17285, lsl #48
	mov	x23, #23972                     ; =0x5da4
	movk	x23, #8134, lsl #16
	movk	x23, #60677, lsl #32
	movk	x23, #9056, lsl #48
	add	x8, x0, #39, lsl #12            ; =159744
	add	x8, x8, #256
	stp	x0, x0, [x19]
	cmp	w20, #0
	str	x8, [x19, #16]
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
	mov	x25, #22359                     ; =0x5757
	movk	x25, #14592, lsl #16
	movk	x25, #38627, lsl #32
	movk	x25, #62820, lsl #48
	mov	x28, #1527                      ; =0x5f7
	movk	x28, #17640, lsl #16
	movk	x28, #48262, lsl #32
	movk	x28, #52896, lsl #48
	mov	x8, #-7378697629483820647       ; =0x9999999999999999
	movk	x8, #39322
	movk	x8, #16329, lsl #48
	fmov	d10, x8
	mov	x8, #3689348814741910323        ; =0x3333333333333333
	movk	x8, #16339, lsl #48
	fmov	d11, x8
	mov	w24, #10000                     ; =0x2710
	b	LBB3_7
LBB3_3:                                 ;   in Loop: Header=BB3_7 Depth=1
	mov	x9, #-101                       ; =0xffffffffffffff9b
	sub	x8, x9, x8
LBB3_4:                                 ;   in Loop: Header=BB3_7 Depth=1
	stp	x8, xzr, [sp]
	ldp	x8, x9, [x19, #8]
	stp	x19, x8, [sp, #32]
	add	x10, sp, #40
	str	x10, [sp, #16]
	mov	x10, sp
	str	x10, [sp, #24]
	cmp	x8, x9
	b.hs	LBB3_20
; %bb.5:                                ;   in Loop: Header=BB3_7 Depth=1
	ldr	q0, [sp]
	str	q0, [x8], #16
LBB3_6:                                 ;   in Loop: Header=BB3_7 Depth=1
	str	x8, [x19, #8]
	add	x21, x21, #1
	cmp	x21, x24
	b.eq	LBB3_24
LBB3_7:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB3_15 Depth 2
                                        ;     Child Loop BB3_10 Depth 2
                                        ;     Child Loop BB3_13 Depth 2
	cbnz	x21, LBB3_9
; %bb.8:                                ;   in Loop: Header=BB3_7 Depth=1
Ltmp141:                                ; EH_LABEL
	bl	__Z21check_resource_budgetv
Ltmp142:                                ; EH_LABEL
LBB3_9:                                 ;   in Loop: Header=BB3_7 Depth=1
	eor	x8, x28, x25
	lsr	x9, x28, #58
	mul	x10, x25, x22
	umulh	x11, x25, x22
	mul	x12, x25, x23
	cmn	x10, #4
	madd	x12, x28, x22, x12
	cinc	x12, x12, hi
	add	x28, x12, x11
	add	x25, x10, #3
	ror	x8, x8, x9
	lsr	x8, x8, #11
	ucvtf	d0, x8, #53
	cmp	w20, #1
	b.ne	LBB3_15
LBB3_10:                                ;   Parent Loop BB3_7 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	eor	x8, x28, x25
	lsr	x9, x28, #58
	mul	x10, x25, x22
	umulh	x11, x25, x22
	mul	x12, x25, x23
	cmn	x10, #4
	madd	x12, x28, x22, x12
	cinc	x12, x12, hi
	add	x28, x12, x11
	add	x25, x10, #3
	ror	x8, x8, x9
	cmp	x8, #616
	b.lo	LBB3_10
; %bb.11:                               ;   in Loop: Header=BB3_7 Depth=1
	lsr	x9, x8, #3
	mov	x10, #63439                     ; =0xf7cf
	movk	x10, #58195, lsl #16
	movk	x10, #39845, lsl #32
	movk	x10, #8388, lsl #48
	umulh	x9, x9, x10
	lsr	x9, x9, #4
	mov	w10, #1000                      ; =0x3e8
	msub	x8, x9, x10, x8
	fcmp	d0, d10
	b.mi	LBB3_3
; %bb.12:                               ;   in Loop: Header=BB3_7 Depth=1
	fcmp	d0, d11
	b.pl	LBB3_19
LBB3_13:                                ;   Parent Loop BB3_7 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	eor	x8, x28, x25
	lsr	x9, x28, #58
	mul	x10, x25, x22
	umulh	x11, x25, x22
	mul	x12, x25, x23
	cmn	x10, #4
	madd	x12, x28, x22, x12
	cinc	x12, x12, hi
	add	x28, x12, x11
	add	x25, x10, #3
	ror	x8, x8, x9
	cmp	x8, #151
	b.lo	LBB3_13
; %bb.14:                               ;   in Loop: Header=BB3_7 Depth=1
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
	sub	x8, x8, #100
	b	LBB3_4
LBB3_15:                                ;   Parent Loop BB3_7 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	eor	x8, x28, x25
	lsr	x9, x28, #58
	mul	x10, x25, x22
	umulh	x11, x25, x22
	mul	x12, x25, x23
	cmn	x10, #4
	madd	x12, x28, x22, x12
	cinc	x12, x12, hi
	add	x28, x12, x11
	add	x25, x10, #3
	ror	x8, x8, x9
	lsr	x9, x8, #31
	cbz	x9, LBB3_15
; %bb.16:                               ;   in Loop: Header=BB3_7 Depth=1
	mov	w9, #-2147483648                ; =0x80000000
	add	x9, x8, x9
	mov	x10, #-2147483648               ; =0xffffffff80000000
	cmp	x8, x10
	csel	x8, x8, x9, lo
	eor	x9, x28, x25
	lsr	x10, x28, #58
	mul	x27, x25, x22
	mul	x11, x25, x23
	cmn	x27, #4
	cinc	x26, x11, hi
	ror	x9, x9, x10
	and	x9, x9, #0x3fffffff
	fcmp	d0, d9
	csinv	x10, xzr, x9, mi
	fcmp	d0, d8
	csinc	x9, x10, x9, pl
	mov	x10, #1073741824                ; =0x40000000
	movk	x10, #32768, lsl #48
	add	x8, x8, x10
	add	x9, x8, x9
	stp	x8, x9, [sp]
	ldp	x8, x9, [x19, #8]
	stp	x19, x8, [sp, #32]
	add	x10, sp, #40
	str	x10, [sp, #16]
	mov	x10, sp
	str	x10, [sp, #24]
	cmp	x8, x9
	b.hs	LBB3_22
; %bb.17:                               ;   in Loop: Header=BB3_7 Depth=1
	ldr	q0, [sp]
	str	q0, [x8], #16
LBB3_18:                                ;   in Loop: Header=BB3_7 Depth=1
	umulh	x9, x25, x22
	madd	x10, x28, x22, x26
	add	x28, x10, x9
	add	x25, x27, #3
	b	LBB3_6
LBB3_19:                                ;   in Loop: Header=BB3_7 Depth=1
	add	x8, x8, #101
	b	LBB3_4
LBB3_20:                                ;   in Loop: Header=BB3_7 Depth=1
Ltmp147:                                ; EH_LABEL
	add	x0, sp, #16
	bl	__ZZNSt3__16vectorI4PairNS_9allocatorIS1_EEE12emplace_backIJS1_EEERS1_DpOT_ENKUlvE0_clEv
Ltmp148:                                ; EH_LABEL
; %bb.21:                               ;   in Loop: Header=BB3_7 Depth=1
	ldr	x8, [sp, #40]
	b	LBB3_6
LBB3_22:                                ;   in Loop: Header=BB3_7 Depth=1
Ltmp144:                                ; EH_LABEL
	add	x0, sp, #16
	bl	__ZZNSt3__16vectorI4PairNS_9allocatorIS1_EEE12emplace_backIJS1_EEERS1_DpOT_ENKUlvE0_clEv
Ltmp145:                                ; EH_LABEL
; %bb.23:                               ;   in Loop: Header=BB3_7 Depth=1
	ldr	x8, [sp, #40]
	b	LBB3_18
LBB3_24:
	ldp	x29, x30, [sp, #160]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #144]            ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #128]            ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #112]            ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #96]             ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #80]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp, #64]               ; 16-byte Folded Reload
	ldp	d11, d10, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #176
	ret
LBB3_25:
	mov	w0, #16                         ; =0x10
	bl	___cxa_allocate_exception
	mov	x19, x0
Ltmp150:                                ; EH_LABEL
Lloh76:
	adrp	x1, l_.str.24@PAGE
Lloh77:
	add	x1, x1, l_.str.24@PAGEOFF
	bl	__ZNSt16invalid_argumentC1B9nqe220101EPKc
Ltmp151:                                ; EH_LABEL
; %bb.26:
Lloh78:
	adrp	x1, __ZTISt16invalid_argument@GOTPAGE
Lloh79:
	ldr	x1, [x1, __ZTISt16invalid_argument@GOTPAGEOFF]
Lloh80:
	adrp	x2, __ZNSt16invalid_argumentD1Ev@GOTPAGE
Lloh81:
	ldr	x2, [x2, __ZNSt16invalid_argumentD1Ev@GOTPAGEOFF]
	mov	x0, x19
	bl	___cxa_throw
LBB3_27:
Ltmp146:                                ; EH_LABEL
	b	LBB3_32
LBB3_28:
Ltmp149:                                ; EH_LABEL
	b	LBB3_32
LBB3_29:
Ltmp152:                                ; EH_LABEL
	mov	x20, x0
	mov	x0, x19
	bl	___cxa_free_exception
	mov	x0, x20
	bl	__Unwind_Resume
LBB3_30:
Ltmp140:                                ; EH_LABEL
	mov	x20, x0
	mov	x0, #0                          ; =0x0
	b	LBB3_33
LBB3_31:
Ltmp143:                                ; EH_LABEL
LBB3_32:
	mov	x20, x0
	ldr	x0, [x19]
LBB3_33:
	cbz	x0, LBB3_35
; %bb.34:
	str	x0, [x19, #8]
	ldr	x8, [x19, #16]
	sub	x1, x8, x0
	bl	__ZdlPvm
LBB3_35:
	mov	x0, x20
	bl	__Unwind_Resume
	.loh AdrpAdd	Lloh76, Lloh77
	.loh AdrpLdrGot	Lloh80, Lloh81
	.loh AdrpLdrGot	Lloh78, Lloh79
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
	.uleb128 Ltmp138-Lfunc_begin3           ; >> Call Site 1 <<
	.uleb128 Ltmp139-Ltmp138                ;   Call between Ltmp138 and Ltmp139
	.uleb128 Ltmp140-Lfunc_begin3           ;     jumps to Ltmp140
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp141-Lfunc_begin3           ; >> Call Site 2 <<
	.uleb128 Ltmp142-Ltmp141                ;   Call between Ltmp141 and Ltmp142
	.uleb128 Ltmp143-Lfunc_begin3           ;     jumps to Ltmp143
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp147-Lfunc_begin3           ; >> Call Site 3 <<
	.uleb128 Ltmp148-Ltmp147                ;   Call between Ltmp147 and Ltmp148
	.uleb128 Ltmp149-Lfunc_begin3           ;     jumps to Ltmp149
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp144-Lfunc_begin3           ; >> Call Site 4 <<
	.uleb128 Ltmp145-Ltmp144                ;   Call between Ltmp144 and Ltmp145
	.uleb128 Ltmp146-Lfunc_begin3           ;     jumps to Ltmp146
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp145-Lfunc_begin3           ; >> Call Site 5 <<
	.uleb128 Ltmp150-Ltmp145                ;   Call between Ltmp145 and Ltmp150
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp150-Lfunc_begin3           ; >> Call Site 6 <<
	.uleb128 Ltmp151-Ltmp150                ;   Call between Ltmp150 and Ltmp151
	.uleb128 Ltmp152-Lfunc_begin3           ;     jumps to Ltmp152
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp151-Lfunc_begin3           ; >> Call Site 7 <<
	.uleb128 Lfunc_end3-Ltmp151             ;   Call between Ltmp151 and Lfunc_end3
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
Lcst_end3:
	.p2align	2, 0x0
                                        ; -- End function
	.section	__TEXT,__text,regular,pure_instructions
	.private_extern	__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB9nqe220101Ev ; -- Begin function _ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB9nqe220101Ev
	.globl	__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB9nqe220101Ev
	.weak_def_can_be_hidden	__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB9nqe220101Ev
	.p2align	2
__ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB9nqe220101Ev: ; @_ZNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEE20__throw_length_errorB9nqe220101Ev
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh82:
	adrp	x0, l_.str.19@PAGE
Lloh83:
	add	x0, x0, l_.str.19@PAGEOFF
	bl	__ZNSt3__120__throw_length_errorB9nqe220101EPKc
	.loh AdrpAdd	Lloh82, Lloh83
	.cfi_endproc
                                        ; -- End function
	.private_extern	__ZNSt3__120__throw_length_errorB9nqe220101EPKc ; -- Begin function _ZNSt3__120__throw_length_errorB9nqe220101EPKc
	.globl	__ZNSt3__120__throw_length_errorB9nqe220101EPKc
	.weak_def_can_be_hidden	__ZNSt3__120__throw_length_errorB9nqe220101EPKc
	.p2align	2
__ZNSt3__120__throw_length_errorB9nqe220101EPKc: ; @_ZNSt3__120__throw_length_errorB9nqe220101EPKc
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
Ltmp153:                                ; EH_LABEL
	mov	x1, x20
	bl	__ZNSt12length_errorC1B9nqe220101EPKc
Ltmp154:                                ; EH_LABEL
; %bb.1:
Lloh84:
	adrp	x1, __ZTISt12length_error@GOTPAGE
Lloh85:
	ldr	x1, [x1, __ZTISt12length_error@GOTPAGEOFF]
Lloh86:
	adrp	x2, __ZNSt12length_errorD1Ev@GOTPAGE
Lloh87:
	ldr	x2, [x2, __ZNSt12length_errorD1Ev@GOTPAGEOFF]
	mov	x0, x19
	bl	___cxa_throw
LBB5_2:
Ltmp155:                                ; EH_LABEL
	mov	x20, x0
	mov	x0, x19
	bl	___cxa_free_exception
	mov	x0, x20
	bl	__Unwind_Resume
	.loh AdrpLdrGot	Lloh86, Lloh87
	.loh AdrpLdrGot	Lloh84, Lloh85
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
	.uleb128 Ltmp153-Lfunc_begin4           ;   Call between Lfunc_begin4 and Ltmp153
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp153-Lfunc_begin4           ; >> Call Site 2 <<
	.uleb128 Ltmp154-Ltmp153                ;   Call between Ltmp153 and Ltmp154
	.uleb128 Ltmp155-Lfunc_begin4           ;     jumps to Ltmp155
	.byte	0                               ;   On action: cleanup
	.uleb128 Ltmp154-Lfunc_begin4           ; >> Call Site 3 <<
	.uleb128 Lfunc_end4-Ltmp154             ;   Call between Ltmp154 and Lfunc_end4
	.byte	0                               ;     has no landing pad
	.byte	0                               ;   On action: cleanup
Lcst_end4:
	.p2align	2, 0x0
                                        ; -- End function
	.section	__TEXT,__text,regular,pure_instructions
	.private_extern	__ZNSt12length_errorC1B9nqe220101EPKc ; -- Begin function _ZNSt12length_errorC1B9nqe220101EPKc
	.globl	__ZNSt12length_errorC1B9nqe220101EPKc
	.weak_def_can_be_hidden	__ZNSt12length_errorC1B9nqe220101EPKc
	.p2align	2
__ZNSt12length_errorC1B9nqe220101EPKc:  ; @_ZNSt12length_errorC1B9nqe220101EPKc
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	__ZNSt11logic_errorC2EPKc
Lloh88:
	adrp	x8, __ZTVSt12length_error@GOTPAGE
Lloh89:
	ldr	x8, [x8, __ZTVSt12length_error@GOTPAGEOFF]
	add	x8, x8, #16
	str	x8, [x0]
	ldp	x29, x30, [sp], #16             ; 16-byte Folded Reload
	ret
	.loh AdrpLdrGot	Lloh88, Lloh89
	.cfi_endproc
                                        ; -- End function
	.private_extern	__ZSt28__throw_bad_array_new_lengthB9nqe220101v ; -- Begin function _ZSt28__throw_bad_array_new_lengthB9nqe220101v
	.globl	__ZSt28__throw_bad_array_new_lengthB9nqe220101v
	.weak_def_can_be_hidden	__ZSt28__throw_bad_array_new_lengthB9nqe220101v
	.p2align	2
__ZSt28__throw_bad_array_new_lengthB9nqe220101v: ; @_ZSt28__throw_bad_array_new_lengthB9nqe220101v
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	mov	w0, #8                          ; =0x8
	bl	___cxa_allocate_exception
	bl	__ZNSt20bad_array_new_lengthC1Ev
Lloh90:
	adrp	x1, __ZTISt20bad_array_new_length@GOTPAGE
Lloh91:
	ldr	x1, [x1, __ZTISt20bad_array_new_length@GOTPAGEOFF]
Lloh92:
	adrp	x2, __ZNSt20bad_array_new_lengthD1Ev@GOTPAGE
Lloh93:
	ldr	x2, [x2, __ZNSt20bad_array_new_lengthD1Ev@GOTPAGEOFF]
	bl	___cxa_throw
	.loh AdrpLdrGot	Lloh92, Lloh93
	.loh AdrpLdrGot	Lloh90, Lloh91
	.cfi_endproc
                                        ; -- End function
	.private_extern	__ZNSt16invalid_argumentC1B9nqe220101EPKc ; -- Begin function _ZNSt16invalid_argumentC1B9nqe220101EPKc
	.globl	__ZNSt16invalid_argumentC1B9nqe220101EPKc
	.weak_def_can_be_hidden	__ZNSt16invalid_argumentC1B9nqe220101EPKc
	.p2align	2
__ZNSt16invalid_argumentC1B9nqe220101EPKc: ; @_ZNSt16invalid_argumentC1B9nqe220101EPKc
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	__ZNSt11logic_errorC2EPKc
Lloh94:
	adrp	x8, __ZTVSt16invalid_argument@GOTPAGE
Lloh95:
	ldr	x8, [x8, __ZTVSt16invalid_argument@GOTPAGEOFF]
	add	x8, x8, #16
	str	x8, [x0]
	ldp	x29, x30, [sp], #16             ; 16-byte Folded Reload
	ret
	.loh AdrpLdrGot	Lloh94, Lloh95
	.cfi_endproc
                                        ; -- End function
	.private_extern	__ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe220101Ev ; -- Begin function _ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe220101Ev
	.globl	__ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe220101Ev
	.weak_def_can_be_hidden	__ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe220101Ev
	.p2align	2
__ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe220101Ev: ; @_ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe220101Ev
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh96:
	adrp	x0, l_.str.25@PAGE
Lloh97:
	add	x0, x0, l_.str.25@PAGEOFF
	bl	__ZNSt3__120__throw_length_errorB9nqe220101EPKc
	.loh AdrpAdd	Lloh96, Lloh97
	.cfi_endproc
                                        ; -- End function
	.globl	__ZZNSt3__16vectorI4PairNS_9allocatorIS1_EEE12emplace_backIJS1_EEERS1_DpOT_ENKUlvE0_clEv ; -- Begin function _ZZNSt3__16vectorI4PairNS_9allocatorIS1_EEE12emplace_backIJS1_EEERS1_DpOT_ENKUlvE0_clEv
	.weak_def_can_be_hidden	__ZZNSt3__16vectorI4PairNS_9allocatorIS1_EEE12emplace_backIJS1_EEERS1_DpOT_ENKUlvE0_clEv
	.p2align	2
__ZZNSt3__16vectorI4PairNS_9allocatorIS1_EEE12emplace_backIJS1_EEERS1_DpOT_ENKUlvE0_clEv: ; @_ZZNSt3__16vectorI4PairNS_9allocatorIS1_EEE12emplace_backIJS1_EEERS1_DpOT_ENKUlvE0_clEv
	.cfi_startproc
; %bb.0:
	stp	x28, x27, [sp, #-96]!           ; 16-byte Folded Spill
	stp	x26, x25, [sp, #16]             ; 16-byte Folded Spill
	stp	x24, x23, [sp, #32]             ; 16-byte Folded Spill
	stp	x22, x21, [sp, #48]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #64]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #80]             ; 16-byte Folded Spill
	.cfi_def_cfa_offset 96
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
	ldr	x24, [x0, #16]
	ldp	x20, x8, [x24]
	sub	x21, x8, x20
	asr	x25, x21, #4
	add	x8, x25, #1
	lsr	x9, x8, #60
	cbnz	x9, LBB10_5
; %bb.1:
	ldr	x9, [x24, #16]
	mov	x10, #9223372036854775792       ; =0x7ffffffffffffff0
	sub	x22, x9, x20
	asr	x9, x22, #3
	cmp	x9, x8
	csel	x8, x9, x8, hi
	cmp	x22, x10
	mov	x9, #1152921504606846975        ; =0xfffffffffffffff
	csel	x8, x8, x9, lo
	lsr	x9, x8, #60
	cbnz	x9, LBB10_6
; %bb.2:
	mov	x19, x0
	ldr	x26, [x0, #8]
	lsl	x23, x8, #4
	mov	x0, x23
	bl	__Znwm
	add	x8, x0, x21
	add	x27, x0, x23
	ldr	q0, [x26]
	str	q0, [x8]
	add	x26, x8, #16
	sub	x23, x8, x25, lsl #4
	mov	x0, x23
	mov	x1, x20
	mov	x2, x21
	bl	_memcpy
	stp	x23, x26, [x24]
	str	x27, [x24, #16]
	cbz	x20, LBB10_4
; %bb.3:
	mov	x0, x20
	mov	x1, x22
	bl	__ZdlPvm
LBB10_4:
	ldr	x8, [x19]
	str	x26, [x8]
	ldp	x29, x30, [sp, #80]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #64]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #48]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #32]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #16]             ; 16-byte Folded Reload
	ldp	x28, x27, [sp], #96             ; 16-byte Folded Reload
	ret
LBB10_5:
	bl	__ZNSt3__16vectorI4PairNS_9allocatorIS1_EEE20__throw_length_errorB9nqe220101Ev
LBB10_6:
	bl	__ZSt28__throw_bad_array_new_lengthB9nqe220101v
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
