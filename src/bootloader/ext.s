[BITS 16]
[ORG 0x7E00]
xor ax, ax
mov ss, ax 
mov ds, ax
mov sp, 0x7BFF
mov bp, 0x7BFF
push szOK 
call puts_pre 
pop ax
jmp $

puts_pre:
mov si, sp
add si, 2

mov si, [ds:si]
puts:
mov al, [ds:si] 
inc si
or al, al
jz puts_ex
call putchar
jmp puts

puts_ex:
ret

dump_msg_:
	push		ax
	mov		ax,8
	call		near ptr __STK
	pop		ax
	push		bx
	push		cx
	push		si
	mov		bx,ax
	mov		cx,dx
	mov		si,0b800H
	xor		ax,ax
L$1:
	cmp		ax,cx
	jge		L$2
	mov		dl,byte ptr [bx]
	mov		byte ptr [si],dl
	inc		ax
	jmp		L$1
L$2:
	pop		si
	pop		cx
	pop		bx
	ret
print_msg_:
	mov		ax,4
	call		near ptr __STK
	push		dx
	mov		dx,5
	mov		ax,offset DGROUP:L$3
	call		near ptr dump_msg_
	pop		dx
	ret

putchar:
MOV AH, 0x0E	
MOV BH, 0x00	
MOV BL, 0x07	
INT 0x10	
RET		


szOK:			db 'Hello from 7E00',0
