	.build_version macos, 26, 0	sdk_version 26, 5
	.section	__TEXT,__text,regular,pure_instructions
	.globl	__Z11target_lessxx              ; -- Begin function _Z11target_lessxx
	.p2align	2
__Z11target_lessxx:                     ; @_Z11target_lessxx
	.cfi_startproc
; %bb.0:
	eor	x8, x0, x1, lsl #1
	add	x0, x8, #1
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	__Z12target_equalxx             ; -- Begin function _Z12target_equalxx
	.p2align	2
__Z12target_equalxx:                    ; @_Z12target_equalxx
	.cfi_startproc
; %bb.0:
	eor	x8, x0, x1, lsl #1
	add	x0, x8, #2
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	__Z14target_greaterxx           ; -- Begin function _Z14target_greaterxx
	.p2align	2
__Z14target_greaterxx:                  ; @_Z14target_greaterxx
	.cfi_startproc
; %bb.0:
	eor	x8, x0, x1, lsl #1
	add	x0, x8, #3
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
