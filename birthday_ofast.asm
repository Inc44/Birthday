; nasm -f elf64 birthday_ofast.asm && gcc -no-pie -o birthday_ofast birthday_ofast.o -lpthread -Ofast && ./birthday_ofast
extern clock_gettime
extern pthread_exit
extern pthread_create
extern pthread_join
extern printf
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
	sec_to_nsec:			dq	1000000000
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
	imul		rax, [rel sec_to_nsec]			; Convert seconds part to nanoseconds
	add			rax, [rel seed_time+8]			; seed.tv_nsec
	xor			rax, r12						; state = seed ^ thread_id for unique seed
	mov			r8d, eax						; Convert state to lower 32 bits
	xor			r9d, r9d						; local_success_count = 0
	mov			r10d, SIMULATIONS_PER_THREAD	; sim = SIMULATIONS_PER_THREAD
	; SSE2 Constants
	mov			eax, 0x02020202					; Load 4 bytes with value 2
	movd		xmm0, eax						; Load 4 bytes as lower 4 bytes
	pshufd		xmm0, xmm0, 0					; Broadcast lower 4 bytes to all 16 bytes
	pxor		xmm1, xmm1						; 0
.simulations_per_thread:						; for (;;)
	mov			rdi, rsp						; Load address of birthdays array
	xor			eax, eax						; Prepare 0 to write
	mov			ecx, DAYS_IN_YEAR / 8 + 1		; rep stosq loop counter
	rep			stosq							; Store 8 bytes from accumulator and advance pointer
	mov			sil, PEOPLE						; i = PEOPLE
.people:										; for (;;)
	; LCG
	imul		r8d, r8d, MULTIPLIER			; state = state * MULTIPLIER
	add			r8d, INCREMENT					; state = state + INCREMENT
	; birthday = (state * DAYS_IN_YEAR) >> 32
	imul		rax, r8, DAYS_IN_YEAR
	shr			rax, 32
	; birthdays[birthday]++
	inc			byte [rsp + rax]
	dec			sil								; i--
	jnz			.people							; i != 0
	; SSE2 Vectorized Check
	pxor		xmm2, xmm2						; exactly_two_count = 0
	mov			cx, DAYS_IN_YEAR / 16 + 1		; i = DAYS_IN_YEAR / 16 + 1
	mov			rdi, rsp						; Load address of birthdays array
.days_in_year:									; for (;;)
	movdqu		xmm3, [rdi]						; Load 16 bytes from birthdays[i]
	pcmpeqb		xmm3, xmm0						; birthdays[i] == 2 then -1 else 0
	psubb		xmm2, xmm3						; exactly_two_count++
	add			rdi, 16							; Advance pointer to next chunk of days
	dec			cx								; i--
	jnz			.days_in_year					; i != 0
	; exactly_two_count Horizontal Sum
	psadbw		xmm2, xmm1						; Absolute sum of byte differences into lower and higher 8 bytes
	movq		rax, xmm2						; Load lower 8 bytes of sum
	pshufd		xmm2, xmm2, 0x4E				; Swap lower and higher 8 bytes of sum
	movq		rcx, xmm2						; Load higher 8 bytes of sum
	add			rax, rcx						; exactly_two_count = sum
	cmp			al, 1							; exactly_two_count == 1
	jne			.local_success_count
	inc			r9d								; local_success_count++
.local_success_count:
	dec			r10d							; sim--
	jnz			.simulations_per_thread			; sim != 0
	mov			[success_count + r12d*4], r9d	; success_count[thread_id] = local_success_count
	xor			edi, edi						; Set return value to NULL
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
	xor			ebx, ebx						; t = 0
.create:
	mov			[thread_ids + rbx*8], rbx		; thread_ids[t] = t
	lea			rdi, [threads + rbx*8]			; Load address of thread handle as first argument
	xor			esi, esi						; Load NULL for attributes as second argument
	mov			rdx, simulate					; Load simulate function pointer for start routine as third argument
	lea			rcx, [thread_ids + rbx*8]		; Load address of thread ID for start routine argument as fourth argument
	call		pthread_create
	inc			bx								; t++
	cmp			bx, NUM_THREADS					; t < NUM_THREADS
	jne			.create
	; Join threads
	xor			ebx, ebx						; t = 0
.join:
	mov			rdi, [threads + rbx*8]			; Load thread handle as first argument
	xor			esi, esi						; Load NULL as second argument
	call		pthread_join
	inc			bx								; t++
	cmp			bx, NUM_THREADS					; t < NUM_THREADS
	jne			.join
	; Total Success Count
	xor			r10d, r10d						; total_success_count = 0
	xor			ebx, ebx						; t = 0
.num_threads:
	add			r10d, [success_count + ebx*4]	; total_success_count += success_count[t]
	inc			bx								; t++
	cmp			bx, NUM_THREADS					; t < NUM_THREADS
	jne			.num_threads
	; Probability
	cvtsi2sd	xmm0, r10d						; Convert total_success_count to double
	mov			eax, TOTAL_SIMULATIONS
	cvtsi2sd	xmm1, eax						; Convert TOTAL_SIMULATIONS to double
	divsd		xmm0, xmm1						; probability = total_success_count / TOTAL_SIMULATIONS
	; Print probability
	lea			rdi, [rel fmt_prob]				; Format string as first argument
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
	lea			rdi, [rel fmt_time]				; Format string as first argument
	mov			al, 1							; Number of arguments
	call		printf
	; Exit
	xor			eax, eax						; Set return code to 0 (success)
	add			rsp, 8							; Deallocate stack space
	ret											; Return from main