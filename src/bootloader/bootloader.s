[BITS 16]
[ORG 0x7C00]
xor ax, ax
mov ds, ax
mov si, 0xb800
mov byte [ds:si], 0x40

mov si, szCheckExtDisk 
call puts 
check_extdsk:
mov ah, 0x41
mov bx, 0x55AA
int 0x13 
jc check_extdsk_nf 
; -----------------------------
; 0x0000 0500
mov dh, 16

load_data:
xor ax, ax
mov ds, ax  
mov si, DAPACK
mov ah, 0x42
int 0x13
jc read_f
mov si, szOK
call puts
mov word [addres], ax 
add ax, 0x2000
mov ax, word [addres]
mov word [startlba], ax
add ax, 16
mov ax, word [startlba]
cmp dh, 0
jz _exit
dec dh
jmp load_data 

_exit:
jmp 0x0000:0x7e00



read_f:
mov al, ah
add al, 0x30
call putchar 
mov si, szRdFail
call puts
jmp $ 

check_extdsk_nf:
mov si, szChkExtF
call puts
jmp $ 

puts:
mov al, [si]
inc si
or al, al
jz puts_ex
call putchar
jmp puts

puts_ex:
ret 


putchar:
MOV AH, 0x0E	
MOV BH, 0x00	
MOV BL, 0x07	
INT 0x10	
RET		

szCheckExtDisk: db 'Checking for extended disks..', 0x0
szChkExtF: 	db 'Failed to load ext. disk operations', 0x0
szOK:		db 'Loaded correctly', 0x0
szRdFail:	db 'Failed to read', 0x0

DAPACK:
		db 	0x10
		db 	0
		DW 	0x16
addres: 	dw 	0x7E00
		dw 	0
startlba:	dd 	1
		dd 	0

TIMES 510 - ($ - $$) db 0	;Fill the rest of sector with 0
DW 0xAA55			;Add boot signature at the end of bootloader


