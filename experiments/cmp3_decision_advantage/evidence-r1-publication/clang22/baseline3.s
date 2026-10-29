	.build_version macos, 26, 0	sdk_version 26, 5
	.section	__TEXT,__text,regular,pure_instructions
	.globl	__Z9baseline3xx                 ; -- Begin function _Z9baseline3xx
	.p2align	2
__Z9baseline3xx:                        ; @_Z9baseline3xx
	.cfi_startproc
; %bb.0:
	cmp	x0, x1
	b.ge	LBB0_2
; %bb.1:
	b	__Z11target_lessxx
LBB0_2:
	b.ne	LBB0_4
; %bb.3:
	mov	x1, x0
	b	__Z12target_equalxx
LBB0_4:
	b	__Z14target_greaterxx
	.cfi_endproc
                                        ; -- End function
	.globl	__Z16baseline3_policyxx         ; -- Begin function _Z16baseline3_policyxx
	.p2align	2
__Z16baseline3_policyxx:                ; @_Z16baseline3_policyxx
	.cfi_startproc
; %bb.0:
	cmn	x0, #101
	b.le	LBB1_3
; %bb.1:
	cmp	x0, #101
	b.lt	LBB1_4
; %bb.2:
	b	__Z14target_greaterxx
LBB1_3:
	b	__Z11target_lessxx
LBB1_4:
	b	__Z12target_equalxx
	.cfi_endproc
                                        ; -- End function
	.globl	__Z15probe_baseline3xxb         ; -- Begin function _Z15probe_baseline3xxb
	.p2align	2
__Z15probe_baseline3xxb:                ; @_Z15probe_baseline3xxb
	.cfi_startproc
; %bb.0:
	mov	w8, #-1                         ; =0xffffffff
	cmp	x0, x1
	cset	w9, ne
	csel	w8, w8, w9, lt
	mov	w9, #-1                         ; =0xffffffff
	cmp	x0, #100
	cset	w10, gt
	cmn	x0, #100
	csel	w9, w9, w10, lt
	cmp	w2, #0
	csel	w0, w8, w9, eq
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
