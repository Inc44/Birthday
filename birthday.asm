; nasm -f elf64 birthday.asm && gcc -no-pie -o birthday birthday.o -lpthread -Ofast && ./birthday
extern clock_gettime
extern printf
extern pthread_create
extern pthread_join
extern pthread_exit
section .data
	DAYS_IN_YEAR			equ	365
	NUM_THREADS				equ	768
	PEOPLE					equ	24
	TOTAL_SIMULATIONS		equ	1000000
	MULTIPLIER				equ	1664525
	INCREMENT				equ	1013904223
	SIMULATIONS_PER_THREAD	equ	TOTAL_SIMULATIONS / NUM_THREADS
	CLOCK_MONOTONIC			equ	1
	fmt_prob:				db	"Probability: %.9f", 10, 0
	fmt_time:				db	"Execution Time: %.3f s", 10, 0
	sec_to_nsec:			dq	1e9
	nsec_to_sec:			dq	1e-9
section .bss
	threads			resq	NUM_THREADS
	thread_ids		resq	NUM_THREADS
	success_count	resd	NUM_THREADS
	start_time:		resq	2
	end_time:		resq	2
	seed_time:		resq	2
section .text
	global main
simulate:
	; Allocate 376 bytes on the stack:
	; 	365 bytes for birthdays array
	; 	3 bytes for stack alignment padding (to a 16-byte boundary)
	; 	8 bytes for return address pushed by 'call'
	sub			rsp, 376
	mov			r12, [rdi]						; Load thread ID value from first argument
	; Seed
	mov			edi, CLOCK_MONOTONIC			; Load clock ID
	lea			rsi, [rel seed_time]			; Load address of seed_time
	call		clock_gettime
	mov			rax, [rel seed_time]			; seed.tv_sec
	mul			rax, [rel sec_to_nsec]			; Convert seconds part to nanoseconds
	add			rax, [rel seed_time+8]			; seed.tv_nsec
	xor			rax, r12						; state = seed ^ thread_id for unique seed
	mov			r8d, eax						; Convert state to lower 32 bits
	xor			r11d, r11d						; local_success_count = 0
	mov			r10d, SIMULATIONS_PER_THREAD	; sim = SIMULATIONS_PER_THREAD
.simulations_per_thread:						; for (;;)
	mov			rdi, rsp						; Load address of birthdays array
	xor			eax, eax						; Prepare 0 to write
	mov			ecx, DAYS_IN_YEAR				; rep stosb loop counter
	rep			stosb							; Store byte from accumulator and advance pointer
	mov			esi, PEOPLE						; i = PEOPLE
.people:										; for (;;)
	; LCG
	mul			r8d, r8d, MULTIPLIER			; state = state * MULTIPLIER
	add			r8d, INCREMENT					; state = state + INCREMENT
	; birthday = state % DAYS_IN_YEAR
	mov			eax, r8d
	xor			edx, edx
	mov			ecx, DAYS_IN_YEAR
	div			ecx
	; birthdays[birthday]++
	mov			rdi, rsp
	add			rdi, rdx
	inc			byte [rdi]
	dec			esi								; i--
	jnz			.people							; i != 0
	xor			esi, esi						; exactly_two_count = 0
	mov			ecx, DAYS_IN_YEAR				; i = DAYS_IN_YEAR
	mov			rdi, rsp						; Load address of birthdays array
.days_in_year:									; for (;;)
	movzx		eax, byte [rdi]					; Load byte from birthdays[i] and zero-extend to eax
	cmp			eax, 2							; birthdays[i] == 2
	jne			.exactly_two_count
	inc			esi								; exactly_two_count++
.exactly_two_count:
	inc			rdi								; Advance pointer to next day
	dec			ecx								; i--
	jnz			.days_in_year					; i != 0
	cmp			esi, 1							; exactly_two_count == 1
	jne			.local_success_count
	inc			r11d							; local_success_count++
.local_success_count:
	dec			r10d							; sim--
	jnz			.simulations_per_thread 		; sim != 0
	mov			rbx, r12						; Load thread ID
	mov			[success_count + rbx*4], r11d	; success_count[thread_id] = local_success_count
	xor			rdi, rdi						; Set return value to NULL
	call		pthread_exit
main:
	; Allocate 8 bytes on the stack:
	; 	8 bytes for return address pushed by 'call'
	sub			rsp, 8
	; Start Time
	mov			edi, CLOCK_MONOTONIC			; Load clock ID as first argument
	lea			rsi, [rel start_time]			; Load address of start_time as second argument
	call		clock_gettime
	; Create threads
	xor			rbx, rbx						; t = 0
.create:
	mov			[thread_ids + rbx*8], rbx		; thread_ids[t] = t
	lea			rdi, [threads + rbx*8]			; Load address of thread handle as first argument
	xor			rsi, rsi						; Load NULL for attributes as second argument
	mov			rdx, simulate					; Load simulate function pointer for start routine as third argument
	lea			rcx, [thread_ids + rbx*8]		; Load address of thread ID for start routine argument as fourth argument
	call		pthread_create
	inc			rbx								; t++
	cmp			rbx, NUM_THREADS				; t < NUM_THREADS
	jne			.create
	; Join threads
	xor			rbx, rbx						; t = 0
.join:
	mov			rdi, [threads + rbx*8]			; Load thread handle as first argument
	xor			rsi, rsi						; Load NULL as second argument
	call		pthread_join
	inc			rbx								; t++
	cmp			rbx, NUM_THREADS				; t < NUM_THREADS
	jne			.join
	; Total Success Count
	xor			r11d, r11d						; total_success_count = 0
	xor			rbx, rbx						; t = 0
.num_threads:
	add			r11d, [success_count + rbx*4]	; total_success_count += success_count[t]
	inc			rbx								; t++
	cmp			rbx, NUM_THREADS				; t < NUM_THREADS
	jne			.num_threads
	; Probability
	cvtsi2sd	xmm0, r11d						; Convert total_success_count to double
	mov			eax, TOTAL_SIMULATIONS
	cvtsi2sd	xmm1, eax						; Convert TOTAL_SIMULATIONS to double
	divsd		xmm0, xmm1						; probability = total_success_count / TOTAL_SIMULATIONS
	; Print probability
	lea			rdi, fmt_prob					; Format string as first argument
	mov			al, 1							; Number of arguments
	call		printf
	; End Time
	mov			edi, CLOCK_MONOTONIC			; Load clock ID
	lea			rsi, [rel end_time]				; Load address of end_time
	call		clock_gettime
	; Elapsed Time
	mov			rax, [rel end_time]				; end_time.tv_sec
	sub			rax, [rel start_time]			; elapsed_sec = end_sec - start_sec
	cvtsi2sd	xmm0, rax						; Convert elapsed_sec to double
	mov			rdx, [rel end_time+8]			; end_time.tv_nsec
	sub			rdx, [rel start_time+8]			; elapsed_nsec = end_nsec - start_nsec
	cvtsi2sd	xmm1, rdx						; Convert elapsed_nsec to double
	mulsd		xmm1, [rel nsec_to_sec]			; Convert nanoseconds part to seconds
	addsd		xmm0, xmm1						; Combine elapsed_sec and converted elapsed_nsec
	; Print execution time
	lea			rdi, fmt_time					; Format string as first argument
	mov			al, 1							; Number of arguments
	call		printf
	; Exit
	xor			eax, eax						; Set return code to 0 (success)
	add			rsp, 8							; Deallocate stack space
	ret											; Return from main