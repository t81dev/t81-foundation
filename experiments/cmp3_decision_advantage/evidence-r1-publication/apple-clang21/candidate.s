	.build_version macos, 26, 0	sdk_version 26, 5
	.section	__TEXT,__text,regular,pure_instructions
	.globl	__Z9candidatexx                 ; -- Begin function _Z9candidatexx
	.p2align	2
__Z9candidatexx:                        ; @_Z9candidatexx
	.cfi_startproc
; %bb.0:
	cmp	x0, x1
	cset	w8, gt
	csinv	w8, w8, wzr, ge
	add	w8, w8, #1
Lloh0:
	adrp	x9, _targets@GOTPAGE
Lloh1:
	ldr	x9, [x9, _targets@GOTPAGEOFF]
	ldr	x2, [x9, w8, uxtw #3]
	br	x2
	.loh AdrpLdrGot	Lloh0, Lloh1
	.cfi_endproc
                                        ; -- End function
	.globl	__Z16candidate_policyxx         ; -- Begin function _Z16candidate_policyxx
	.p2align	2
__Z16candidate_policyxx:                ; @_Z16candidate_policyxx
	.cfi_startproc
; %bb.0:
	cmp	x0, #101
	mov	w8, #1                          ; =0x1
	cinc	x8, x8, ge
	cmn	x0, #100
	csel	x8, xzr, x8, lt
Lloh2:
	adrp	x9, _targets@GOTPAGE
Lloh3:
	ldr	x9, [x9, _targets@GOTPAGEOFF]
	ldr	x2, [x9, x8, lsl #3]
	br	x2
	.loh AdrpLdrGot	Lloh2, Lloh3
	.cfi_endproc
                                        ; -- End function
	.globl	__Z15probe_candidatexxb         ; -- Begin function _Z15probe_candidatexxb
	.p2align	2
__Z15probe_candidatexxb:                ; @_Z15probe_candidatexxb
	.cfi_startproc
; %bb.0:
	cbz	w2, LBB2_2
; %bb.1:
	mov	w8, #-1                         ; =0xffffffff
	cmp	x0, #100
	cset	w9, gt
	cmn	x0, #100
	csel	w0, w8, w9, lt
	ret
LBB2_2:
	cmp	x0, x1
	cset	w8, gt
	csinv	w0, w8, wzr, ge
	ret
	.cfi_endproc
                                        ; -- End function
	.section	__DATA,__const
	.globl	_targets                        ; @targets
	.weak_def_can_be_hidden	_targets
	.p2align	3, 0x0
_targets:
	.quad	__Z11target_lessxx
	.quad	__Z12target_equalxx
	.quad	__Z14target_greaterxx

.subsections_via_symbols
