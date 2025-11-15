; nasm -f elf64 birthday.asm && gcc -no-pie -o birthday birthday.o -lpthread -Ofast && ./birthday
extern clock_gettime
extern printf
section .data
	DAYS_IN_YEAR		equ	365
	PEOPLE				equ	24
	TOTAL_SIMULATIONS	equ	1000000
	MULTIPLIER			equ	1664525
	INCREMENT			equ	1013904223
	CLOCK_MONOTONIC		equ	1
	fmt_prob:			db	"Probability: %.9f", 10, 0
	fmt_time:			db	"Execution Time: %.3f s", 10, 0
	sec_to_nsec:		dq	1e9
	nsec_to_sec:		dq	1e-9
section .bss
	start_time:	resq	2
	end_time:	resq	2
	seed_time:	resq	2
	birthdays:	resb	365
section .text
	global main
simulate:
	lea			rdi, [rel birthdays]	; Load address of birthdays array
	xor			eax, eax
	mov			ecx, DAYS_IN_YEAR		; rep stosb loop counter
	rep			stosb					; Store byte from accumulator and advance pointer
	mov			esi, PEOPLE				; i = PEOPLE
.people:								; for (;;)
	; LCG
	mul			r8d, r8d, MULTIPLIER	; state = state * MULTIPLIER
	add			r8d, INCREMENT			; state = state + INCREMENT
	; birthday = state % DAYS_IN_YEAR
	mov			eax, r8d
	xor			edx, edx
	mov			ecx, DAYS_IN_YEAR
	div			ecx
	; birthdays[birthday]++
	lea			rdi, [rel birthdays]
	add			rdi, rdx
	inc			byte [rdi]
	dec			esi						; i--
	jnz			.people					; i != 0
	xor			esi, esi				; exactly_two_count = 0
	mov			ecx, DAYS_IN_YEAR		; i = DAYS_IN_YEAR
	lea			rdi, [rel birthdays]	; Load address of birthdays array
.days_in_year:							; for (;;)
	movzx		eax, byte [rdi]			; Load byte from birthdays[i] and zero-extend to eax
	cmp			eax, 2					; birthdays[i] == 2
	jne			.exactly_two_count
	inc			esi						; exactly_two_count++
.exactly_two_count:
	inc			rdi						; Advance pointer to next day
	dec			ecx						; i--
	jnz			.days_in_year			; i != 0
	cmp			esi, 1					; exactly_two_count == 1
	sete		al						; local_success_count++
	ret
main:
	; Allocate 408 bytes on the stack:
	; 	16 bytes for start_time (timespec):
	; 		8 bytes for tv_sec
	; 		8 bytes for tv_nsec
	; 	16 bytes for end_time (timespec):
	; 		8 bytes for tv_sec
	; 		8 bytes for tv_nsec
	; 	365 bytes for birthdays array
	; 	3 bytes for stack alignment padding (to a 16-byte boundary)
	; 	8 bytes for return address pushed by 'call'
	sub			rsp, 408
	; Start Time
	mov			edi, CLOCK_MONOTONIC	; Load clock ID as first argument
	lea			rsi, [rel start_time]	; Load address of start_time as second argument
	call		clock_gettime
	; Seed
	mov			edi, CLOCK_MONOTONIC	; Load clock ID
	lea			rsi, [rel seed_time]	; Load address of seed_time
	call		clock_gettime
	mov			rax, [rel seed_time]	; seed.tv_sec
	mul			rax, [rel sec_to_nsec]	; Convert seconds part to nanoseconds
	add			rax, [rel seed_time+8]	; seed.tv_nsec
	mov			r8d, eax				; Convert state to lower 32 bits
	xor			r11d, r11d				; total_success_count = 0
	mov			r10d, TOTAL_SIMULATIONS	; sim = TOTAL_SIMULATIONS
.simulate:								; for (;;)
	call		simulate
	add			r11d, eax				; total_success_count++
	dec			r10d					; sim--
	jnz			.simulate				; sim != 0
	; Probability
	cvtsi2sd	xmm0, r11d				; Convert total_success_count to double
	mov			eax, TOTAL_SIMULATIONS
	cvtsi2sd	xmm1, eax				; Convert TOTAL_SIMULATIONS to double
	divsd		xmm0, xmm1				; probability = total_success_count / TOTAL_SIMULATIONS
	; Print probability
	lea			rdi, fmt_prob
	call		printf
	; End Time
	mov			edi, CLOCK_MONOTONIC	; Load clock ID
	lea			rsi, [rel end_time]		; Load address of end_time
	call		clock_gettime
	; Elapsed Time
	mov			rax, [rel end_time]		; end_time.tv_sec
	sub			rax, [rel start_time]	; elapsed_sec = end_sec - start_sec
	cvtsi2sd	xmm0, rax				; Convert elapsed_sec to double
	mov			rdx, [rel end_time+8]	; end_time.tv_nsec
	sub			rdx, [rel start_time+8]	; elapsed_nsec = end_nsec - start_nsec
	cvtsi2sd	xmm1, rdx				; Convert elapsed_nsec to double
	mulsd		xmm1, [rel nsec_to_sec]	; Convert nanoseconds part to seconds
	addsd		xmm0, xmm1				; Combine elapsed_sec and converted elapsed_nsec
	; Print execution time
	lea			rdi, fmt_time
	call		printf
	; Exit
	xor			eax, eax				; Set return code to 0 (success)
	add			rsp, 408				; Deallocate stack space
	ret									; Return from main