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
.subsections_via_symbols
