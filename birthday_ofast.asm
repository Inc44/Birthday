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
	; Allocate 392 bytes on the stack:
	; 	365 bytes for birthdays array
	; 	19 bytes for vector padding (to a 32-byte boundary)
	; 	8 bytes for return address pushed by 'call'
	sub			rsp, 392
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
	; AVX2 Constants
	mov			eax, 0x02020202					; Load 4 bytes with value 2
	movd		xmm0, eax						; Load 4 bytes as lower 4 bytes
	vpbroadcastd ymm0, xmm0						; Broadcast lower 4 bytes to all 32 bytes
	vpxor		ymm1, ymm1, ymm1				; 0
.simulations_per_thread:						; for (;;)
%assign i 0										; i = 0
%rep DAYS_IN_YEAR / 32 + 1						; for (;;)
	vmovdqu		[rsp + i], ymm1					; Load 32 bytes from birthdays[i] and zero them
	%assign i i+32								; i += 32
%endrep											; i < DAYS_IN_YEAR
%rep PEOPLE										; for (i = 0;; i++)
	; LCG
	imul		r8d, r8d, MULTIPLIER			; state = state * MULTIPLIER
	add			r8d, INCREMENT					; state = state + INCREMENT
	; birthday = (state * DAYS_IN_YEAR) >> 32
	imul		rax, r8, DAYS_IN_YEAR
	shr			rax, 32
	; birthdays[birthday]++
	inc			byte [rsp + rax]
%endrep											; i < PEOPLE
	; AVX2 Vectorized Check
	vpxor		ymm2, ymm2, ymm2				; exactly_two_count = 0
	vpxor		ymm3, ymm3, ymm3				; exactly_two_count_1 = 0
	vpxor		ymm4, ymm4, ymm4				; exactly_two_count_2 = 0
	vpxor		ymm5, ymm5, ymm5				; exactly_two_count_3 = 0
%assign i 0										; i = 0
%rep DAYS_IN_YEAR / 128 + 1						; for (;;)
	; Load 32 bytes from birthdays[i]
	vmovdqu		ymm6, [rsp + i]					; Block 1
	vmovdqu		ymm7, [rsp + i + 32]			; Block 2
	vmovdqu		ymm8, [rsp + i + 64]			; Block 3
	vmovdqu		ymm9, [rsp + i + 96]			; Block 4
	; birthdays[i] == 2 then -1 else 0
	vpcmpeqb	ymm6, ymm6, ymm0				; Block 1
	vpcmpeqb	ymm7, ymm7, ymm0				; Block 2
	vpcmpeqb	ymm8, ymm8, ymm0				; Block 3
	vpcmpeqb	ymm9, ymm9, ymm0				; Block 4
	; exactly_two_count++
	vpsubb		ymm2, ymm2, ymm6				; Block 1
	vpsubb		ymm3, ymm3, ymm7				; Block 2
	vpsubb		ymm4, ymm4, ymm8				; Block 3
	vpsubb		ymm5, ymm5, ymm9				; Block 4
	%assign i i+128								; i += 128 Advance pointer to next chunk of days
%endrep											; i < DAYS_IN_YEAR
	; exactly_two_count Sum
	vpaddb		ymm2, ymm2, ymm3				; exactly_two_count += exactly_two_count_1
	vpaddb		ymm4, ymm4, ymm5				; exactly_two_count_2 += exactly_two_count_3
	vpaddb		ymm2, ymm2, ymm4				; exactly_two_count += exactly_two_count_2
	; exactly_two_count Horizontal Sum
	vpsadbw		ymm2, ymm2, ymm1				; Absolute sum of byte differences into 4 8-byte parts
	vextracti128 xmm3, ymm2, 1					; Load higher 16 bytes of sum
	vpaddq		xmm2, xmm2, xmm3				; Add lower and higher 16 bytes of sum
	movq		rax, xmm2						; Load lower 8 bytes of sum
	vpshufd		xmm2, xmm2, 0x4E				; Swap lower and higher 8 bytes of sum
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