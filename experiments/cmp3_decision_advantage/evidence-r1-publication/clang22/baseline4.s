	.build_version macos, 26, 0	sdk_version 26, 5
	.section	__TEXT,__text,regular,pure_instructions
	.globl	__Z9baseline4xx                 ; -- Begin function _Z9baseline4xx
	.p2align	2
__Z9baseline4xx:                        ; @_Z9baseline4xx
	.cfi_startproc
; %bb.0:
	cmp	x0, x1
	cset	w8, gt
	csinv	w8, w8, wzr, ge
	cbz	w8, LBB0_3
; %bb.1:
	cmn	w8, #1
	b.ne	LBB0_4
; %bb.2:
	b	__Z11target_lessxx
LBB0_3:
	b	__Z12target_equalxx
LBB0_4:
	b	__Z14target_greaterxx
	.cfi_endproc
                                        ; -- End function
	.globl	__Z16baseline4_policyxx         ; -- Begin function _Z16baseline4_policyxx
	.p2align	2
__Z16baseline4_policyxx:                ; @_Z16baseline4_policyxx
	.cfi_startproc
; %bb.0:
	cmp	x0, #100
	cset	w8, gt
	cmn	x0, #100
	csinv	w8, w8, wzr, ge
	cbz	w8, LBB1_3
; %bb.1:
	cmn	w8, #1
	b.ne	LBB1_4
; %bb.2:
	b	__Z11target_lessxx
LBB1_3:
	b	__Z12target_equalxx
LBB1_4:
	b	__Z14target_greaterxx
	.cfi_endproc
                                        ; -- End function
	.globl	__Z15probe_baseline4xxb         ; -- Begin function _Z15probe_baseline4xxb
	.p2align	2
__Z15probe_baseline4xxb:                ; @_Z15probe_baseline4xxb
	.cfi_startproc
; %bb.0:
	cmp	x0, x1
	mov	w8, #-1                         ; =0xffffffff
	cneg	w8, w8, ge
	csel	w8, wzr, w8, eq
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
