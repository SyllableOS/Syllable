[BITS 16]
[ORG 0x100]

ISTACK   EQU     200h             ; hardware IRQ safe stack size (in bytes)

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

tsk_back	EQU	0
tsk_esp0        EQU	4
tsk_sp0         EQU	8
tsk_esp1        EQU	12
tsk_sp1         EQU	16
tsk_esp2        EQU	20
tsk_sp2         EQU	24
tsk_ocr3        EQU	28
tsk_oeip        EQU	32
tsk_oeflags     EQU	36
tsk_oeax        EQU	40
tsk_oecx        EQU	44
tsk_oedx        EQU	48
tsk_oebx        EQU	52
tsk_oesp        EQU	56
tsk_oebp        EQU	60
tsk_oesi        EQU	64
tsk_oedi        EQU	68
tsk_oes         EQU	72
tsk_ocs         EQU	76
tsk_oss         EQU	80
tsk_ods         EQU	84
tsk_ofs         EQU	88
tsk_ogs         EQU	92
tsk_oldtr       EQU	96
tsk_iomap       EQU	100
	
[SECTION .text]
	  jmp start
		
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
setup_com:
	mov	al,0x83
	mov	dx,0x2fb
	out	dx,al
	
	mov	al,0x01
	mov	dx,0x2f8
	out	dx,al

	mov	al,0x00
	mov	dx,0x2f9
	out	dx,al
	

	mov	al,0x03
	mov	dx,0x2fb
	out	dx,al
	
	mov	al,0x00
	mov	dx,0x2f9
	out	dx,al
	ret
	
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
write_com16:
	push	eax
	push	ebx
	push	edx
.loop:	
	mov	dx,0x2fd
.wait:	
	in	al,dx
	and	al,0x40
	jz	.wait
	mov	dx,0x2f8
	mov	al,[ebx]
	cmp	al,0
	je	.done
	out	dx,al
	inc	ebx
	jmp	.loop
.done:
	pop	edx
	pop	ebx
	pop	eax
	ret

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
print_number16:
	push	eax
	push	ebx
	push	ecx
	push	edx
	mov	edx,ebx
	mov	eax,8
	mov	ebx,.tmp_str
.loop:	
	rol	edx,4
	mov	ecx,edx
	and	ecx,0x0f
	add	ecx,_hextbl
	mov	cl,[ecx]
	mov	[.tmp_str],cl
	call	write_com16
	dec	eax
	jnz	.loop
	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
	ret
.tmp_str:
	db	0,0
	
str_nl:			db 13, 10, 0


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
		
load_stage2:
	mov	bx,.str_load
	call	write_com16
	mov	bx,stage2_path
	call	write_com16
	mov	bx,str_nl
	call	write_com16
	
	mov	ax,0x3d00	; open read only
	mov	dx,stage2_path
	int	0x21

	jnc	.ok
	mov	ebx,.str_open_err
	call	write_com16
.ok:
	mov	bx,ax		; move file handle
	mov	ah,0x3f		; read from file
	mov	cx,20000	; number of bytes to read
	mov	dx,loader	; pointer to buffer
	int	0x21
	ret
.str_open_err:		db 'Failed to open stage 2 loader', 13, 10, 0
.str_load:		db 'Load stage 2 loader: ', 0

	
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
parse_args:
	push	cx
	push	si
	push	di
	
	xor	cx,cx
	mov	cl,[128]	; get argument length from PSP
	mov	si,129		; address of the arg string in PSP
	mov	di,stage2_path	; we use the first arg as path for the stage2 loader
	
.loop1:				; first we skip leading white spaces
	cmp	cx,0
	je	.invalid_args
	cmp	byte [si],' '
	jne	.loop2
	inc	si
	dec	cx
	jmp	.loop1
.loop2:				; then we copy the first argument to stage2_path
	cmp	cx,0
	je	.done
	cmp	byte [si],' '
	je	.done
	movsb
	dec	cx
	jmp	.loop2
.done
	mov	byte [di],0	; zero terminate the path
	
	mov	di,arg_string	; then we send the remaining arguments to the stage2 loader
	rep	movsb
	mov	byte [di],0	; zero terminate the arg string
	mov	ax,0
	pop	di
	pop	si
	pop	di
	ret
.invalid_args:
	mov	ax,-1
	pop	di
	pop	si
	pop	di
	ret
	
start:
	cld		; clear the direction flag
	call	setup_com

	xor	eax,eax
	mov	ax,cs
	mov	ds,ax	; data and code resides in the same segment
	mov	es,ax
	mov	fs,ax
	mov	gs,ax
	
		
	mov	[code_seg],ax
	shl	eax,4
	mov	[abs_start_addr],eax	; Get the physical address we was loaded to
	cli

	; relocate the offsets in GDT an IDT descriptors.
	add	dword [gdt32a + 2],eax	
	add	dword [idt32a + 2],eax
		
	mov	[code32dsc + desc_bsl],ax	; base 0-15
	mov	[data32dsc + desc_bsl],ax	; base 0-15
	shr	eax,8
	mov	[code32dsc + desc_bsm],ah	; base 16-23
	mov	[data32dsc + desc_bsm],ah	; base 16-23
	
	mov	eax,[abs_start_addr]
	mov	[retrealc + desc_bsl],ax	; base 0-15
	mov	[retreald + desc_bsl],ax	; base 0-15
	shr	eax,8
	mov	[retrealc + desc_bsm],ah	; base 16-23
	mov	[retreald + desc_bsm],ah	; base 16-23

	; Init selectors for stage2 loader.
	
	mov	eax,loader
	add	eax,[abs_start_addr]
	mov	[st2_cs_desc + desc_bsl],ax	; base 0-15
	mov	[st2_ds_desc + desc_bsl],ax	; base 0-15
	shr	eax,8
	mov	[st2_cs_desc + desc_bsm],ah	; base 16-23
	mov	[st2_ds_desc + desc_bsm],ah	; base 16-23
	
	mov	eax,v86task
	add	eax,[abs_start_addr]
	
	mov	[v86taskdsc + desc_bsl],ax	; base 0-15
	shr	eax,8
	mov	[v86taskdsc + desc_bsm],ah	; base 16-23

	call	parse_args
	cmp	ax,0
	jne	.invalid_args	
	call	load_stage2
	
	mov	ebx,.str_enter_pmode
	call	write_com16
	
	lgdt	[gdt32a]        ; Init protected mode
	mov	eax,cr0
	or	al,1
	mov	cr0,eax
	db	0eah
	dw	start32,CS_DESC

.invalid_args:	
	mov	bx,.str_invalid_args
	call	write_com16
	mov	ax,4c00h
	int	21h
	
.str_invalid_args:	db 'Usage: boot stage2_loader_path [stage2 args]', 13, 10, 0
.str_enter_pmode:	db 'Entering protected mode', 13, 10, 0
		
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
v86sys:				; V86 interrupt
	pop	eax
	mov	[cs:v86sysintnum],al
	popad
	add	esp,2		; flags
	pop	es
	pop	ds
	pop	fs
	pop	gs

	db	0cdh		; int xx
v86sysintnum    db      ?

	push	gs
	push	fs
	push	ds
	push	es
	pushf
	pushad
	sub	esp,4	; skip int num

	int 	0fdh	; pseudo int-number (0fdh) detected by exception handler

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
	; morph back to real mode
prerealmode:                            ; 16 bit protected mode to real mode
	mov	eax,realmode
	; THIS IS BROKEN
;	add	ax,[code_seg]
	mov	[.call_seg],ax
	
	lidt	[defidt]
	mov	eax,cr0
	and	al,0feh
	mov	cr0,eax
	db	0eah
.call_off:	dw realmode
.call_seg:	dw 0
realmode:                               ; Back in real mode
	mov	ax,cs                       ; Fix up regs and quit to DOS
	mov	ds,ax
	mov	es,ax
	mov	ss,ax
	xor	eax,eax
	mov	fs,ax
	mov	gs,ax
	xor	ebx,ebx
	xor	ecx,ecx
	xor	edx,edx
	xor	esi,esi
	xor	edi,edi
	xor	ebp,ebp
	mov	esp,stack16e	;	+200h !!!!!!!!!!!!!!!!!!!!!!
	mov	ax,4c00h
	int	21h
	dw      ?,?,?
	
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
[BITS 32]


? EQU 0 ; Attempt to emulate MASM	

CS_DESC   equ	0x08
DS_DESC	  equ 	0x10
ZERO_DS   equ	0x18
ZERO_CS	  equ	0x38
TASK_DESC equ	0x40
STG2_CS	  equ	0x48
STG2_DS	  equ	0x50

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
start32:
	lidt		[cs:idt32a]

	mov		ax,DS_DESC	; set data selectors
	mov		ds,ax
	mov		es,ax
	mov		fs,ax
	mov		ss,ax
	mov		ax,ZERO_DS		; set zero based data selector
	mov		gs,ax
	mov		esp,stack32e

	mov		ax,40h
	ltr		ax			; set v86task

	pushfd
	pop		eax
	or		ah,30h		; IO privilege level 3
	and		ah,~40h		; clear nested task flag
	push		eax
	popfd

	in		al,21h
	mov		[oirqm21],al
	or		al,3
	out		21h,al
	in		al,0a1h
	mov		[oirqma1],al
	mov		bx,2820h
	call		set8529vektorz
	call		enableA20
	sti

	mov		eax,arg_string
	sub		eax,loader	; the stage2 loader think this is address 0 so...
	push		eax		; pass a pointer to the argument string as third parameter
	
	mov		eax,text_end
	add		eax,[abs_start_addr]
	sub		eax,loader	; the stage2 loader think this is address 0 so...
	add		eax,15
	and		eax,~15		; align it to a segment boundary
	sub		eax,[abs_start_addr]
	push		eax		; pass the first free address (after the loader) as seccond parameter.
	
	mov		eax,loader
	add		eax,[abs_start_addr]
	push		eax		; pass the base address of the loader as first parameter.
	
	
	mov		bx,STG2_DS
	mov		ds,bx
	mov		es,bx
	mov		fs,bx
	mov		gs,bx

	push DWORD	_exit	; If the loader returns
	pushfd               	; Just pass our own flags
	push DWORD	STG2_CS	; NULL based code descriptor
	push DWORD	0x74	; Entry point of stage-2 loader
	iretd

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
callv86:
	pushad
	push		ds
	push		es
	push		fs
	push		gs

	push		ebx	; save pointer to parameter block

	
	mov		dl,al	;	save interrupt number

	mov		cx,DS_DESC	; data selector
	mov		ds,cx
	mov		cx,ZERO_DS	; null based data selector
	mov		es,cx
	mov		cx,STG2_DS	; data selector used by stage2 loader
	mov		gs,cx
	xor		ecx,ecx
	mov		cx,ss

	push dword	[v86task + tsk_sp0]
	push dword	[v86task + tsk_esp0]

	mov		[v86task + tsk_sp0],ecx
	mov		[v86task + tsk_esp0],esp	; adress of stack when a exception occures

	mov		ecx,[v86stacka]
	sub dword	[v86stacka],ISTACK	; alloc space for nested interrupts

	sub		ecx,100		; alloc space for parameter block

	xor		eax,eax
	push		eax		; GS
	push		eax		; FS
	push		eax		; DS
	push		eax		; ES

	push DWORD	[code_seg] ; SS
	push		ecx		; ESP

	mov		[ecx],edx	; real int num
	add		ecx,4

	cmp		ebx,0
	je		.no_param
	mov		edx,12		; copy everyting but SS
.copy_param:
	mov		eax,[gs:ebx]	; copy parameter block
	mov		[ecx],eax

	add		ebx,4
	add		ecx,4
	dec		edx
	jnz		.copy_param
.no_param:

	pushfd
	pop		eax
	or		eax,20000h	; set VM flag
	push		eax		; EFLAGS

	push DWORD	[code_seg]	; to segment (real)
	push DWORD	v86sys		; to offset (real)
	iretd

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
retfromv86:
	add		esp,36+32

	mov		ax,DS_DESC
	mov		ds,ax
	mov		ax,ZERO_DS
	mov		es,ax
	mov		cx,STG2_DS	; data selector used by stage2 loader
	mov		gs,cx

	

	pop dword	[v86task + tsk_esp0]
	pop dword	[v86task + tsk_sp0]

	pop		ebx		; pointer to parameter block

	cmp		ebx,0
	je		.no_result

	mov		ecx,[v86stacka]
	add		ecx,ISTACK-96

	mov		edx,12
.copy_result:
	mov		eax,[ecx]
	mov		[gs:ebx],eax	; copy parameter block

	add		ebx,4
	add		ecx,4
	dec		edx
	jnz		.copy_result

	
.no_result:
	add dword	[v86stacka],ISTACK


	pop	gs
	pop 	fs
	pop 	es
	pop	ds
	popad

	iretd

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

v86gen:                                 ; General protection violation handler
	test	byte[esp+14],2	; test VM flag
	jnz	short v86genv86
	pushad
	mov	dl,13
	jmp	exception

v86genv86:
	add	esp,4		; kill error code
	pushad

	mov	ax,ZERO_DS
	mov	ds,ax					; data selector
	movzx	ebx,word [esp+36]	; old CS
	shl	ebx,4
	add	ebx,[esp+32]		; old EIP
	inc	word [esp+32]
	mov	al,[ebx]		; fetch instruction that caused the exception

	mov	dl,3			; int vect 3
	cmp	al,0cch			; int 3
	je	short v86int
	cmp	al,0ceh			; into
	ja	exc_trampoline
	je	short v86genv86ni
	inc	word [esp+32]
	mov	dl,[ebx+1]		; fetch interrupt number (after 'int' instruction)
	cmp	dl,0fdh			; int fdh is used as dummi int for return from real int simulation
	jne	short v86int

	jmp	retfromv86

v86genv86ni:
	mov	dl,4			; int vect 4
v86int:					; Do interrupt for V86 task
	mov	ax,ZERO_DS
	mov	ds,ax			; set data selector
	movzx	ebx,dl
	shl	ebx,2
	movzx	edx,word [esp+48]	; old ss
	shl	edx,4
	sub	word [esp+44],6		; increment old esp
	add	edx,[esp+44]		; edx = physical stack address - 6

	mov	ax,[esp+40]		; old flags
	mov	[edx+4],ax		; make sure old flags get loaded before entering int handler
	mov 	ax,[esp+36]		; old cs
	mov 	[edx+2],ax		; set return segment (from int handler to v86 routine)
	mov 	ax,[esp+32]		; old eip
	mov 	[edx],ax		; set return address (from int handler to v86 routine)
	and 	word [esp+40],0fcffh	; old flags (clear RESUME flag)
	mov 	eax,[ds:ebx]		; fetch interrupt vector
	mov 	[esp+32],ax		; old eip
	shr 	eax,16
	mov 	[esp+36],ax		; old cs
	popad
	iretd

exc_trampoline:	
	jmp	exception			
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
exc0:                                   ; Exceptions
	push ax
	mov al,0
	jmp irq
exc1:
	push ax
	mov al,1
        jmp irq
exc2:
	push ax
	mov al,2
        jmp irq
exc3:
	push ax
	mov al,3
        jmp irq
exc4:
	push ax
	mov al,4
        jmp irq
exc5:
	push ax
	mov al,5
        jmp irq
exc6:
        pushad
        mov dl,6
        jmp short exception
exc7:
	push ax
	mov al,7
        jmp irq
exc8:
        pushad
        mov dl,8
        jmp short exception
exc9:
        pushad
        mov dl,9
        jmp short exception
exca:
        pushad
        mov dl,10
        jmp short exception
excb:
        pushad
        mov dl,11
        jmp short exception
excc:
        pushad
        mov dl,12
        jmp short exception
exce:
        pushad
        mov dl,14
        jmp short exception
unexp:                                  ; Unexpected interrupt
        pushad
        mov dl,0ffh
exception:
	mov al,0ffh
	out 21h,al
	out 0a1h,al
	jmp _exit

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
irq0:                                   ; Do IRQs
	push 	ax
	push	ds
	mov 	al,8
	jmp 	irq
irq1:
	push	ax
	push	ds
	mov	al,9
	jmp	short irq
irq2:
	push	ax
	push	ds
	mov	al,0ah
	jmp	short irq
irq3:
	push	ax
	push	ds
	mov	al,0bh
	jmp	short irq
irq4:
	push	ax
	push	ds
	mov	al,0ch
	jmp	short irq
irq5:
	push	ax
	push	ds
	mov	al,0dh
	jmp	short irq
irq6:
	push	ax
	push	ds
	mov	al,0eh
	jmp	short irq
irq7:
	push	ax
	push	ds
	mov	al,0fh
	jmp	short irq
irq8:
	push	ax
	push	ds
	mov	al,70h
	jmp	short irq
irq9:
	push	ax
	push	ds
	mov	al,71h
	jmp	short irq
irqa:
	push	ax
	push	ds
	mov	al,72h
	jmp	short irq
irqb:
	push	ax
	push	ds
	mov	al,73h
	jmp	short irq
irqc:
	push	ax
	push	ds
	mov	al,74h
	jmp	short irq
irqd:
	push	ax
	push	ds
	mov	al,75h
	jmp	short irq
irqe:
	push	ax
	push	ds
	mov	al,76h
	jmp	short irq
irqf:
	push	ax
	push	ds
	mov	al,77h
irq:
	test byte [esp+12],2
	jnz short v86irq

	push DWORD	DS_DESC	; A real mode IRQ in protected mode
	pop	ds
	push	ebx
	xor	ebx,ebx		; no register block
	int	30h
	pop	ebx
	pop	ds
	pop	ax
	iretd
	
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
v86irq:                                 ; An IRQ from V86 mode
	push DWORD	DS_DESC
	pop	ds
	mov	[v86irqnum],al
	pop	ds
	pop 	ax
	pushad
	push	ds
	push DWORD	DS_DESC
	pop	ds
	mov	dl,[v86irqnum]
	pop	ds
	jmp	v86int

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

set8529vektorz:                         ; Set new IRQ vektor numbers
        mov	al,11h                      ;  BL - low vektor base #
        out	20h,al                      ;  BH - high vektor base #
        jmp	short $+2
        mov	al,bl
        out	21h,al
        jmp	short $+2
        mov	al,4h
        out	21h,al
        jmp	short $+2
        mov	al,1h
        out	21h,al
        jmp	short $+2

        mov	al,11h
        out	0a0h,al
        jmp	short $+2
        mov	al,bh
        out	0a1h,al
        jmp	short $+2
        mov	al,2h
        out	0a1h,al
        jmp	short $+2
        mov	al,1h
        out	0a1h,al
        jmp	short $+2
        ret

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
write_com32:
	push	eax
	push	ebx
	push	edx
.loop:	
	mov	dx,0x2fd
.wait:	
	in	al,dx
	and	al,0x40
	jz	.wait
	mov	dx,0x2f8
	mov	al,[ebx]
	cmp	al,0
	je	.done
	out	dx,al
	inc	ebx
	jmp	.loop
.done:
	pop	edx
	pop	ebx
	pop	eax
	ret

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
print_number32:
	push	eax
	push	ebx
	push	ecx
	push	edx
	mov	edx,ebx
	mov	eax,8
	mov	ebx,.tmp_str
.loop:	
	rol	edx,4
	mov	ecx,edx
	and	ecx,0x0f
	add	ecx,_hextbl
	mov	cl,[ecx]
	mov	[.tmp_str],cl
	call	write_com32
	dec	eax
	jnz	.loop
	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
	ret
.tmp_str:
	db	0,0
	
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
enableA20:                              ; Enable gate A20
        call	.enableA20o1
        jnz	short .enableA20done
        mov	al,0d1h
        out	64h,al
        call	.enableA20o1
        jnz	short .enableA20done
        mov	al,0dfh
        out	60h,al
.enableA20o1:
        mov	ecx,20000h
.enableA20o1l:
        jmp short $+2
        in	al,64h
        test	al,2
        loopnz	.enableA20o1l
.enableA20done:
        ret

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
_exit:
        cli
        mov	bx,7008h
        call	set8529vektorz
        mov	al,[oirqm21]
        out	21h,al
        mov	al,[oirqma1]
        out	0a1h,al
        mov	ax,30h
        mov	ds,ax
        mov	es,ax
        mov	fs,ax
        mov	gs,ax
        mov	ss,ax
        db	0eah
        dw	prerealmode,0,28h

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
[SECTION .data]

	
gdt32           dw	0,0
		db	0,0,0,0
	
code32dsc       dw      0ffffh, 0
		db	0, 10011010b, 11001111b, 0
	
data32dsc       dw	0ffffh, 0
		db	0, 10010010b, 11001111b, 0

zerodsc         dw	0ffffh, 0
		db	0, 10010010b, 11001111b, 0

ov86taskdsc     dw	0ffffh, v86task,
		db	0, 11101001b, 0, 0

retrealc        dw	0ffffh, 0
		db	0, 10011010b, 0, 0
retreald        dw	0ffffh, 0
		db	0, 10010010b, 0, 0

zerocode        dw	0ffffh, 0
		db	0, 10011010b, 11001111b, 0

v86taskdsc      dw	0ffffh, v86task
		db	0, 11101001b, 0, 0

st2_cs_desc:    dw      0ffffh, 0
		db	0, 10011010b, 11001111b, 0
	
st2_ds_desc:    dw	0ffffh, 0
		db	0, 10010010b, 11001111b, 0
	
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
idt32:	        dw      exc0,   8, 08e00h, 0	;0
                dw      exc1,   8, 08e00h, 0
                dw      exc2,   8, 08e00h, 0
                dw      exc3,   8, 08e00h, 0
                dw      exc4,   8, 08e00h, 0
                dw      exc5,   8, 08e00h, 0	;5
                dw      exc6,   8, 08e00h, 0
                dw      exc7,   8, 08e00h, 0
                dw      exc8,   8, 08e00h, 0
                dw      exc9,   8, 08e00h, 0
                dw      exca,   8, 08e00h, 0	;a
                dw      excb,   8, 08e00h, 0
                dw      excc,   8, 08e00h, 0
                dw      v86gen, 8, 08e00h, 0	 ;d
                dw      exce,   8, 08e00h, 0		 ;e
       TIMES 17 dw      unexp,  8, 08e00h, 0	;f-1f
                dw      irq0,   8, 08e00h, 0	;20h
                dw      irq1,   8, 08e00h, 0
                dw      irq2,   8, 08e00h, 0
                dw      irq3,   8, 08e00h, 0
                dw      irq4,   8, 08e00h, 0
                dw      irq5,   8, 08e00h, 0	;25
                dw      irq6,   8, 08e00h, 0
                dw      irq7,   8, 08e00h, 0

                dw      irq8,   8, 08e00h, 0
                dw      irq9,   8, 08e00h, 0
                dw      irqa,   8, 08e00h, 0	;2A
                dw      irqb,   8, 08e00h, 0
                dw      irqc,   8, 08e00h, 0
                dw      irqd,   8, 08e00h, 0
                dw      irqe,   8, 08e00h, 0
                dw      irqf,   8, 08e00h, 0	;2F
                dw      callv86,8, 08e00h, 0	;30

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
gdt32a          dw      03fh+8 + 16, gdt32,0
idt32a          dw      7ffh, idt32,0

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	

_hextbl:        db      '0123456789ABCDEF'


v86stacka	dd	v86stacke

		
v86task:
	times 104	db 0	; Task structure
	times 0x2000	db 0	; IO bitmap
defidt:	
	dw      3ffh, 0,0
	
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
[SECTION .bss]

arg_string:
	resb	128
stage2_path:
	resb	128


loader:
	resb	20000

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
	
v86irqnum:	resb	1	; num of IRQ that ocurred in V86 mode

oirqm21         resb	1	; old low IRQ mask
oirqma1         resb	1	; old high IRQ mask


abs_start_addr:	resd	1	; physical address we was loaded to
code_seg:	resw	1	; real-mode segment we was loaded to

	resb	8192
stack32e:

v86stack:	;  TIMES 4096*5	db	?
	resb	4096*5
v86stacke:
				
	
stack16:	
	resd	1024
stack16e:
	
text_end:
