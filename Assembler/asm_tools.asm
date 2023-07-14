.code

Get_Pos_Address proc

	; Параметры:
	; RCX - screen_buffer
	; RDX - pos
	; Возврат: RDI


	; 1.1 pos.Y * pos.Screen_Width 
	mov rax, rdx
	shr rax, 16  ; AX = pos.Y_Pos
	movzx rax, ax ; RAX = AX

	mov rbx, rdx
	shr rbx, 32  ; BX = pos.Screen_Width
	movzx rbx, bx ; RBX = BX

	imul rax, rbx  ; RAX = RAX*RBX

	; 1.2 pox.X_pos к RAX
	movzx rbx, dx ; RBX = DX = pos.X_Pos
	add rax, rbx  ; PAX = pos.Y_Pos * pos.Screen_Width + pos.X_Pos = смещение в символах

	; 1.3 RAX содержит смещение начала строки в символах, надо в байтах. символ = 4 байта
	shl rax, 2 ; RAX = RAX * 4
	mov rdi, rcx  ; RDI = screen_buffer
	add rdi, rax  ; RDI = screen_buffer + address_offset

	ret

Get_Pos_Address endp

;-------------------------------------------------------------------------------------------------------------
Get_Screen_Width_Size proc
	
	; Вычисление ширины экрана в байта
	; RDX - SPos spos или SArea_Pos pos
	; Возвртат: R11 = pos.Screen_Width * 4
	mov r11, rdx 
	shr r11, 32 ; R11 = pos
	movzx r11, r11w ; R11 = R11W = pos.Screen_Width
	shl r11, 2 ; R11 = pos.Screen_Width * 4 = Ширина экрана в байтах

Get_Screen_Width_Size endp

;-------------------------------------------------------------------------------------------------------------
Draw_Start_Symbol proc
; Выводим стартовый символ
; Параметры:
; RDI - текущий адрес в буфере окна
; R8 - symbol
; Возврат: нет

	push rax
	push rbx

	mov eax, r8d
	mov rbx, r8
	shr rbx, 32  ; RBX = EBX = { symbol.Start_Symbol, symbol.End_Symbol }
	mov ax, bx  ; EAX = { symbol.Attributes, symbol.Start_Symbol }

	stosd

	pop rbx
	pop rax

	ret

Draw_Start_Symbol endp

;-------------------------------------------------------------------------------------------------------------
Draw_End_Symbol proc
; Выводим конечный символ
; Параметры:
; EAX - { symbol.Attributes, symbol.Main_Symbol }
; RDI - текущий адрес в буфере окна
; R8 - symbol
; Возврат: нет

	mov rbx, r8
	shr rbx, 48  ; RBX = BX = symbol.End_Symbol
	mov ax, bx  ; EAX = { symbol.Attributes, symbol.End_Symbol }

	stosd

	ret

Draw_End_Symbol endp

;-------------------------------------------------------------------------------------------------------------
Draw_Line_Horizontal proc
	; extern "C" void Draw_Line_Horizontal(CHAR_INFO *screen_buffer, SPos pos, ASymbol symbol);
	; Параметры:
	; RCX - screen_buffer
	; RDX - pos
	; R8 - symbol
	
	; стек
	push rax
	push rbx
	push rcx
	push rdi

	; 1. Адрес вывода address_offset =(pos.Y_Pos * pos.Screen_Width + pos.X_Pos) * 4
	call Get_Pos_Address ; RDI = позиция символа в буфере screen_buffer в позиции pos
	
	; 2. Вывод ствртового символа
	call Draw_Start_Symbol

	; 3. Вывод символов symbol.Main_Symbol
	mov eax, r8d
	mov rcx, rdx
	shr rcx, 48  ; RCX = CX = pos.Len

	;rep префикс повторения, stosd записывает содержимое eax в указатель rdi и увеличит адрес на 4 байта
	rep stosd

	; 4. Конечный символ
	call Draw_End_Symbol

	pop rdi
	pop rcx
	pop rbx
	pop rax

	ret

Draw_Line_Horizontal endp

;-------------------------------------------------------------------------------------------------------------
; extern "C" void Draw_Line_Vertical(CHAR_INFO * screen_buffer, SPos pos, ASymbol symbol);
Draw_Line_Vertical proc
; Параметры:
	; RCX - screen_buffer
	; RDX - pos
	; R8 - symbol

	push rax
	push rcx
	push rdi
	push r11

	; 1. Адрес вывода
	call Get_Pos_Address

	; 2. Коррекция позиции вывода
	mov r11, rdx
	shr r11, 32 ; R11 = pos
	movzx r11, r11w ; = pos.Screen_Width
	dec r11
	shl r11, 2 ; R11 = pos.Screen_Width * 4

	; 3. Вывод стартового символа
	call Draw_Start_Symbol

	add rdi, r11

	; 4. Счетчик цикла
	mov rcx, rdx
	shr rcx, 48 ;RCX = CX = pos.Len

	mov eax, r8d ; EAX = symbol

	_1:
		stosd ; Вывод символа
		add rdi, r11

	loop _1

	; 5. Конечный символ
	call Draw_End_Symbol

	pop r11
	pop rdi
	pop rcx
	pop rax

	ret
Draw_Line_Vertical endp

;-------------------------------------------------------------------------------------------------------------
Show_Colors proc
	; extern "C" void Show_Colors(CHAR_INFO * screen_buffer, SPos pos, ASymbol symbol);
	; Параметры:
	; RCX - screen_buffer
	; RDX - pos
	; R8 - symbol

	push rax
	push rbx
	push rcx
	push rdi
	push r10
	push r11

	; 1. Адрес вывода address_offset =(pos.Y_Pos * pos.Screen_Width + pos.X_Pos) * 4
	call Get_Pos_Address

	mov r10, rdi

	; 2 Коррекция позиции вывода
	call Get_Screen_Width_Size
	sub r11, 4

	; 3. Циклы
	mov rax, r8  ; RAX = EAX = symbol

	and rax, 0ffffh ; Обнуление байтов RAX
	mov rbx, 16
	xor rcx, rcx ; RCX = 0

	_0:
		mov cl, 16

	_1:
		stosd
		add rax, 010000h ; Единица, смещенная на 16 разрдов влево

	loop _1

	add r10, r11
	mov rdi, r10

	dec rbx
	jnz _0
	

	pop r11
	pop r10
	pop rdx
	pop rcx
	pop rbx
	pop rax

	ret

Show_Colors endp

;-------------------------------------------------------------------------------------------------------------
Clear_Area proc
; extern "C" void Clear_Area(CHAR_INFO *screen_buffer, SArea_Pos area_pos, ASymbol symbol);

	push rax
	push rbx
	push rcx
	push rdi
	push r10
	push r11

	; Параметры:
	; RCX - screen_buffer
	; RDX - area_pos
	; R8 - symbol


	; 1. Адрес вывода
	call Get_Pos_Address
	
	mov r10, rdi

	; 2. Вычисление коррекции позиции ввода
	call Get_Screen_Width_Size

	; 2. Подготовка циклов
	mov rax, r8
	mov rbx, rdx
	shr rbx, 48 ; BH = area_pos.Height BL = area_pos.Width
	xor rcx, rcx ; RCX = 0

	; 3. Цикл
_0:
	mov cl, bl
	rep stosd

	add r10, r11
	mov rdi, r10
	dec bh
	jnz _0

	pop r11
	pop r10
	pop rdi
	pop rcx
	pop rbx
	pop rax

	ret

Clear_Area endp

;-------------------------------------------------------------------------------------------------------------
Draw_Text proc
;extern "C" int Draw_Text(CHAR_INFO * screen_buffer, SText_Pos pos, const wchar_t *str);
	; Параметры:
	; RCX - screen_buffer
	; RDX - pos
	; R8 - str
	; Возврат: RAX - длина строки


	push rbx
	push rdi
	push r8

	call  Get_Pos_Address

	mov rax, rdx
	shr rax, 32 ; Старшая половина EAX содержит pos. Attributes
	
	xor rbx, rbx ; RBX = 0

_1:
	mov ax, [ r8 ] ; AL - символ строки

	cmp ax, 0 ; Сравнение регистров
	je _exit; Переход if =

	add r8, 2 ; Переход указателя на следующий символ

	stosd
	inc rbx
	jmp _1

_exit:
	
	mov rax, rbx
	pop r8
	pop rdi
	pop rbx

	ret
Draw_Text endp

;-------------------------------------------------------------------------------------------------------------
Draw_Limited_Text proc
; extern "C" void Draw_Limited_Text(CHAR_INFO * screen_buffer, SText_Pos pos, unsigned short limit;
	; Параметры:
	; RCX - screen_buffer
	; RDX - pos
	; R8 - str
	; R9 - limit
	; Возврат: RAX - длина строки


	push rax
	push rcx
	push rdi
	push r8
	push r9

	call  Get_Pos_Address

	mov rax, rdx
	shr rax, 32 ; Старшая половина EAX содержит pos. Attributes
	
_1:
	mov ax, [ r8 ] ; AL - символ строки

	cmp ax, 0 ; Сравнение регистров
	je _fill_spaces; Переход if =

	add r8, 2 ; Переход указателя на следующий символ

	stosd

	dec r9
	cmp r9, 0
	je _exit
	jmp _1

_fill_spaces:
	mov ax, 020h ; Сохранение пробелов
	mov rcx, r9
	rep stosd


_exit:
	pop r9
	pop r8
	pop rdi
	pop rcx
	pop rax
	ret

Draw_Limited_Text endp

end