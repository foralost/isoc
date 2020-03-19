.387
		PUBLIC	dump_msg_
		PUBLIC	print_msg_
		EXTRN	__STK:BYTE
		EXTRN	_small_code_:BYTE
DGROUP		GROUP	CONST,CONST2,_DATA
_TEXT		SEGMENT	BYTE PUBLIC USE16 'CODE'
		ASSUME CS:_TEXT, DS:DGROUP, SS:DGROUP
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
_TEXT		ENDS
CONST		SEGMENT	WORD PUBLIC USE16 'DATA'
L$3:
    DB	68H, 65H, 6cH, 6cH, 6fH, 0

CONST		ENDS
CONST2		SEGMENT	WORD PUBLIC USE16 'DATA'
CONST2		ENDS
_DATA		SEGMENT	WORD PUBLIC USE16 'DATA'
_DATA		ENDS
		END
