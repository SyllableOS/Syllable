[BITS 16]
[ORG 0x0000]
[SECTION .text]
	
;CS_DESC   equ	0x08

;desc    struc
;lml     dw      0		; segment limit 0-15
;bsl     dw      0		; base 0-15
;bsm     db      0		; base 16-23
;acc     db      0		; P/DPL/1/TYPE/A
;lmh     db      0		; G/X/O/AVL/(limit 16-19)
;bsh     db      0		; base 24-31
;desc    ends
	
desc_lml     EQU      0
desc_bsl     EQU      2
desc_bsm     EQU      4
desc_acc     EQU      5
desc_lmh     EQU      6
desc_bsh     EQU      7

CS_DESC   equ	0x08
DS_DESC	  equ 	0x10
ZERO_DS   equ	0x18
ZERO_CS	  equ	0x38
TASK_DESC equ	0x40
STG2_CS	  equ	0x48
STG2_DS	  equ	0x50
	
		
	jmp	start
	db	0
kernel_entry:	
	dd	0x101693
kernel_stack:
	dd	0
kernel_ds:
	dd	0
kernel_cs:
	dd	0
gdt32a:	
	dd	0
	
	
start:
	cli		; Should be done already, just a bit paranoide
	cld		; clear the direction flag

	mov	ax,cs
	mov	ds,ax
	mov	es,ax
	mov	fs,ax
	mov	gs,ax
	mov	ss,ax

	xor	eax,eax
	mov	ax,cs
	shl	eax,4
	
	add	dword [gdt_48 + 2],eax
		
	mov	[code32dsc + desc_bsl],ax	; base 0-15
	mov	[data32dsc + desc_bsl],ax	; base 0-15
	shr	eax,8
	mov	[code32dsc + desc_bsm],ah	; base 16-23
	mov	[data32dsc + desc_bsm],ah	; base 16-23
	
	mov	eax,[kernel_entry]
	mov	[entry_off],eax
	
	mov	eax,[gdt_48_addr]
	lgdt 	[eax]        ; Init protected mode
;	lgdt 	[gdt_48_addr]        ; Init protected mode
	
	mov	eax,cr0
	or	al,1
	mov	cr0,eax
	db	0xea
	dw	start32,CS_DESC
	nop
	nop

[BITS 32]
	
start32:
	mov	eax,[idt_48_addr]
	lidt	[eax]
	
	mov	al,0x00
	mov	dx,0x3c8
	out	dx,al

	mov	al,0x1f
	mov	dx,0x3c9
	out	dx,al

	mov	al,0x1f
	mov	dx,0x3c9
	out	dx,al

	mov	al,0xff
	mov	dx,0x3c9
	out	dx,al

	
	mov	esp,[kernel_stack]
	mov	eax,ZERO_DS ; [kernel_ds]
	mov	ss,ax
	mov	ds,ax
	mov	es,ax
	mov	fs,ax
	mov	gs,ax

	
	db	0eah
entry_off:	dd	0
entry_seg:	dw	ZERO_CS
		
	pushfd				; Just pass our own flags
	push DWORD	ZERO_CS ; [kernel_cs]	; NULL based code descriptor
	push DWORD	[kernel_entry]	; Entry point of kernel init
	iretd


gdt32           dw	0,0
		db	0,0,0,0
	
code32dsc       dw      0ffffh, 0
		db	0, 10011010b, 11001111b, 0
	
data32dsc       dw	0ffffh, 0
		db	0, 10010010b, 11001111b, 0

zerodsc         dw	0ffffh, 0
		db	0, 10010010b, 11001111b, 0

ov86taskdsc     dw	0ffffh, 0 ;v86task,
		db	0, 11101001b, 0, 0

retrealc        dw	0ffffh, 0
		db	0, 10011010b, 0, 0
retreald        dw	0ffffh, 0
		db	0, 10010010b, 0, 0

zerocode        dw	0ffffh, 0
		db	0, 10011010b, 11001111b, 0

v86taskdsc      dw	0ffffh, 0 ;v86task
		db	0, 11101001b, 0, 0

st2_cs_desc:    dw      0ffffh, 0
		db	0, 10011010b, 11001111b, 0
	
st2_ds_desc:    dw	0ffffh, 0
		db	0, 10010010b, 11001111b, 0

		
idt_48_addr:
	dd	idt_48
gdt_48_addr:
	dd	gdt_48
	
idt_48:
	dw	0			; idt limit=0
	dd	0			; idt base=0L

gdt_48:
	dw	0xffff			; gdt limit=0
gdt_48_base:
	dw	gdt32,0			; gdt base=0L
	
