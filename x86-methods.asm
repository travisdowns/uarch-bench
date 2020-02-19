%include "x86-helpers.asm"

nasm_util_assert_boilerplate
thunk_boilerplate

; a variant of define_bench that puts a "touch" function immediately following
; the
%macro make_touch 0
%ifndef current_bench
%error "current_bench must be defined for this to work"
%endif
GLOBAL current_bench %+ _touch:function
current_bench %+ _touch:
ret
%endmacro

;; This function executes a tight loop, where we expect that each iteration takes
;; once cycle (plus some penalty on loop exit due to the mispredict). We time this
;; function to calculate the effective CPU frequency. We could also consider a series
;; of dependent add calls, which we expect to each take 1 cycle as well. It isn't
;; really clear which assumption is the most likely to be true in the future, but
;; most delay loops seem to be of the "tight loop" variety, so let's choose that.
;; In the past, both of these approaches would have returned wrong values, e.g.,
;; twice the true frequency for the "add" approach on the double-pumped ALU on the P4
;; or half or less than the true frequency on CPUs that can't issue one taken branch
;; per cycle.
GLOBAL add_calibration_x86:function
ALIGN 32
add_calibration_x86:
times 2 sub  rdi, 1
jge add_calibration_x86
ret

define_bench dummy_bench_oneshot1
ret

define_bench dummy_bench_oneshot2
ret
make_touch

define_bench dep_add_noloop_128
xor eax, eax
times 128 add rax, rax
ret

define_bench dep_add_rax_rax
xor eax, eax
.top:
times 128 add rax, rax
dec rdi
jnz .top
ret

define_bench dep_imul128_rax
xor eax, eax
.top:
times 128  imul rax
dec rdi
jnz .top
ret

; because the 64x64=128 imul uses an implicit destination & first source
; we need to clear out eax each iteration to make it independent, although
; of course that may bias the measurement on some architectures
define_bench indep_imul128_rax
.top:
%rep 128
xor eax, eax
imul rax
%endrep
dec rdi
jnz .top
ret

define_bench dep_imul64_rax
xor eax, eax
.top:
times 128  imul rax, rax
dec rdi
jnz .top
ret

define_bench indep_imul64_rax
.top:
%rep 128
xor eax, eax
imul rax, rax
%endrep
dec rdi
jnz .top
ret

define_bench dep_pushpop
xor eax, eax
xor ecx, ecx
.top:
%rep 128
push rax
pop  rax
%endrep
dec rdi
jnz .top
ret

define_bench indep_pushpop
xor eax, eax
xor ecx, ecx
.top:
%rep 128
push rax
pop  rcx
%endrep
dec rdi
jnz .top
ret

define_bench div_64_64
xor eax,eax
xor edx,edx
mov ecx, 1
.top:
times 128 div rcx
dec rdi
jnz .top
ret

define_bench idiv_64_64
xor eax,eax
xor edx,edx
mov ecx, 1
.top:
times 128 idiv rcx
dec rdi
jnz .top
ret

define_bench nop1_128
.top:
times 128 nop
dec rdi
jnz .top
ret

define_bench nop2_128
.top:
times 128 nop2
dec rdi
jnz .top
ret

define_bench xor_eax_128
.top:
times 128 xor eax, eax
dec rdi
jnz .top
ret

%if 1

%macro define_alu_load 1
define_bench alu_load_6_%1
push_callee_saved

.top:
%rep 64
add eax, 1
add ebx, 1
add ecx, 1
add edx, 1
add esi, 1
add ebp, 1
%assign rnum 8
%rep %1
mov r %+ rnum, [rsp + (rnum - 8) * 4]
%assign rnum (rnum + 1)
%endrep
%undef rnum
%endrep
dec rdi
jnz .top

pop_callee_saved
ret
%endmacro

define_alu_load 0
define_alu_load 1
define_alu_load 2
define_alu_load 3
define_alu_load 4
define_alu_load 5
define_alu_load 6


%endif



empty_fn:
ret

define_bench dense_calls
.top:
%rep 16
call empty_fn
;nop
%endrep
dec rdi
jnz .top
ret

%macro sparse_calls 1
define_bench sparse %+ %1 %+ _calls
xor     eax, eax
.top:
%rep 16
call empty_fn
times %1 add eax, 1
%endrep
dec rdi
jnz .top
ret
%endmacro

sparse_calls 0
sparse_calls 1
sparse_calls 2
sparse_calls 3
sparse_calls 4
sparse_calls 5
sparse_calls 6
sparse_calls 7



%macro chained_calls 1
define_bench chained %+ %1 %+ _calls
xor     eax, eax
.top:
%rep 4
call call_chain_3
times %1 add eax, 1
%endrep
dec rdi
jnz .top
ret
%endmacro

chained_calls 0
chained_calls 1
chained_calls 2
chained_calls 3

pushpop:
push rax
pop  rax
ret

define_bench pushpop_calls
xor     eax, eax
.top:
%rep 16
call pushpop
%endrep
dec rdi
jnz .top
ret

%macro addrsp 1
addrsp%1:
add     QWORD [rax - %1], 0
add     QWORD [rax - %1], 0
ret
%endmacro

%macro addrsp_calls 1
addrsp %1
define_bench addrsp%1_calls
lea     rax, [rsp - 8]
.top:
%rep 16
call addrsp%1
%endrep
dec rdi
jnz .top
ret
%endmacro

addrsp_calls 0
addrsp_calls 8


define_bench indep_add
.top:
%rep 50
add eax, ebp
add ecx, ebp
add edx, ebp
add esi, ebp
add r8d, ebp
add r9d, ebp
add r10d, ebp
add r11d, ebp
%endrep
dec rdi
jnz .top
ret

; a basic bench that repeats a single op the given number
; of times, defaults to 128
; %1 the benchmark name
; %2 the full operation
; %3 the default of repeates (128 if not specified)
%macro define_single_op 2-3 128
define_bench %1
xor eax, eax
.top:
times %3 %2
dec rdi
jnz .top
ret
%endmacro

define_single_op rdtsc_bench,rdtsc
define_single_op rdtscp_bench,rdtscp

; pointer chasing loads from a single stack location on the stack
; %1 name suffix
; %2 load expression (don't include [])
; %3 offset if any to apply to pointer and load expression
%macro make_spc 3
define_bench sameloc_pointer_chase%1
or rcx, -1
inc rcx ; rcx is zero but this is just a fancy way of doing it to defeat zero-idiom recognition
lea rax, [rsp - 8 - %3]
push rax
.top:
times 128 mov rax, [%2 + %3]
dec rdi
jnz .top
pop rax
ret
%endmacro

make_spc ,rax,0
make_spc _complex,rax + rcx * 8,4096

; https://stackoverflow.com/q/52351397
define_bench sameloc_pointer_chase_diffpage
push rbp
mov  rbp, rsp
sub  rsp,  4096
and  rsp, -4096 ; align rsp to page boundary
lea rax, [rsp - 8]
mov [rax + 16], rax
.top:
times 128 mov rax, [rax + 16]
dec rdi
jnz .top
mov rsp, rbp
pop rbp
ret

; https://stackoverflow.com/q/52351397
define_bench sameloc_pointer_chase_alt
push rbp
mov  rbp, rsp
sub  rsp,  4096
and  rsp, -4096 ; align rsp to page boundary
lea rax, [rsp - 8]
mov [rax], rax
mov [rax + 16], rax
.top:
%rep 64
mov rax, [rax]
mov rax, [rax + 16]
%endrep
dec rdi
jnz .top
mov rsp, rbp
pop rbp
ret

; put an ALU op in the pointer chase path
define_bench sameloc_pointer_chase_alu
lea rax, [rsp - 8]
push rax
.top:
%rep 128
mov rax, [rax]
add rax, 0
%endrep
dec rdi
jnz .top
pop rax
ret

; put an ALU op in the pointer chase path
define_bench sameloc_pointer_chase_alu2
push rbp
mov  rbp, rsp

and  rsp, -4096  ; page align rsp to avoid inadvertent page crossing
sub  rsp, 2048 + 4096

and rcx, 0 ; avoid zero idiom detection
lea rax, [rsp - 8]
push rax
mov [rax + 2048], rax

.top:
%rep 128
mov rax, [rax + 2048]
mov rax, [rax]
add rax, 0
%endrep
dec rdi
jnz .top

mov rsp, rbp
pop rbp

ret

; put an ALU op in the pointer chase path
define_bench sameloc_pointer_chase_alu3
push rbp
mov  rbp, rsp

and  rsp, -4096  ; page align rsp to avoid inadvertent page crossing
sub  rsp, 2048 + 4096

and rcx, 0 ; avoid zero idiom detection
lea rax, [rsp - 8]
push rax
mov [rax + 2048], rax

.top:
%rep 128
mov rax, [rax + 2048]
add rax, 0
mov rax, [rax]
%endrep
dec rdi
jnz .top

mov rsp, rbp
pop rbp

ret


; do 8 parallel pointer chases to see if fast path (4-cycle) loads
; have a throughput restriction (e.g., only 1 per cycle)
define_bench sameloc_pointer_chase_8way
push rbp
mov  rbp, rsp

and  rsp, -4096  ; page align rsp to avoid inadvertent page crossing

push r12
push r13
push r14
push r15

%assign regn 8
%rep 8
%define reg r %+ regn
lea reg, [rsp - 8]
push reg
%assign regn (regn + 1)
%endrep

.top:
%rep 16
%assign regn 8
%rep 8
%define reg r %+ regn
mov reg, [reg]
%assign regn (regn + 1)
%endrep
%endrep
dec rdi
jnz .top

add rsp, 8 * 8

pop r15
pop r14
pop r13
pop r12

mov rsp, rbp
pop rbp

ret

; so we can treat r6 to r15 as a contiguous range
%define r6 rsi
%define r7 rdi

; do 10 parallel pointer chases to see if slow path (5-cycle) loads
; have a throughput restriction (e.g., only 1 per cycle)
define_bench sameloc_pointer_chase_8way5
push rbp
mov  rbp, rsp

sub  rsp,  8192
and  rsp, -4096  ; page align rsp to avoid inadvertent page crossing

push r12
push r13
push r14
push r15

mov rcx, rdi ; because we need rdi and rsi as r6 and r7 with altreg

%define offset 4096

%assign regn 6
%rep 10
%define reg r %+ regn
lea reg, [rsp - 8 - offset]
push reg
%assign regn (regn + 1)
%endrep

.top:
%rep 16
%assign regn 6
%rep 10
%define reg r %+ regn
mov reg, [reg + offset]
%assign regn (regn + 1)
%endrep
%endrep
dec rcx
jnz .top

add rsp, 10 * 8

pop r15
pop r14
pop r13
pop r12

mov rsp, rbp
pop rbp

ret

; do 10 parallel pointer chases, unrolled 10 times, where 9 out of the 10
; accesses on each change are 5-cycle and one is 4-cycle, to see if mixing
; 4 and 5-cycle operations slows things down
define_bench sameloc_pointer_chase_8way45
push rbp
mov  rbp, rsp

sub  rsp,  8192
and  rsp, -8192  ; page align rsp to avoid inadvertent page crossing

push r12
push r13
push r14
push r15

mov rcx, rdi

%define offset 4096
%define regcnt 10

%assign regn (16 - regcnt)
%rep regcnt
%define reg r %+ regn
lea reg, [rsp - 8]
push reg
mov [reg + offset], reg
%assign regn (regn + 1)
%endrep

.top:
%assign itr 0
%rep 10
%assign regn (16 - regcnt)
%rep regcnt
%define reg r %+ regn
%if (itr == (regn - (16 - regcnt)))
mov reg, [reg]
%else
mov reg, [reg + offset]
%endif
%assign regn (regn + 1)
%endrep
%assign itr (itr + 1)
%endrep
dec rcx
jnz .top

add rsp, regcnt * 8

pop r15
pop r14
pop r13
pop r12

mov rsp, rbp
pop rbp

ret



; a series of stores to the same location
define_bench store_same_loc
xor eax, eax
.top:
times 128 mov [rsp - 8], eax
dec rdi
jnz .top
ret

; a series of 16-bit stores to the same location, passed as the second parameter
define_bench store16_any
xor eax, eax
.top:
times 128 mov [rsi], ax
dec rdi
jnz .top
ret

; a series of 32-bit stores to the same location, passed as the second parameter
define_bench store32_any
xor eax, eax
.top:
times 128 mov [rsi], eax
dec rdi
jnz .top
ret

; a series of 64-bit stores to the same location, passed as the second parameter
define_bench store64_any
xor eax, eax
.top:
times 128 mov [rsi], rax
dec rdi
jnz .top
ret

; a series of AVX (REX-encoded) 128-bit stores to the same location, passed as the second parameter
define_bench store128_any
vpxor xmm0, xmm0, xmm0
.top:
times 128 vmovdqu [rsi], xmm0
dec rdi
jnz .top
ret

; a series of AVX (REX-encoded) 256-bit stores to the same location, passed as the second parameter
define_bench store256_any
vpxor xmm0, xmm0, xmm0
.top:
times 128 vmovdqu [rsi], ymm0
dec rdi
jnz .top
ret

; a series of AVX512 512-bit stores to the same location, passed as the second parameter
define_bench store512_any
vpxorq zmm16, zmm16, zmm16
.top:
times 128 vmovdqu64 [rsi], zmm16
dec rdi
jnz .top
ret

; a series of independent 16-bit loads from the same location, with location passed as the second parameter
; note that the loads are not zero-extended, so they only write the lower 16 bits of eax, and so on some
; implementations each load is actually dependent on the previous load (to merge in the upper bits of eax)
define_bench load16_any
xor eax, eax
.top:
times 128 mov ax, [rsi]
dec rdi
jnz .top
ret

; a series of independent 32-bit loads from the same location, with location passed as the second parameter
define_bench load32_any
.top:
times 128 mov eax, [rsi]
dec rdi
jnz .top
ret

; a series of independent 64-bit loads from the same location, with location passed as the second parameter
define_bench load64_any
.top:
times 128 mov rax, [rsi]
dec rdi
jnz .top
ret

; a series of independent 128-bit loads from the same location, with location passed as the second parameter
define_bench load128_any
.top:
times 128 vmovdqu xmm0, [rsi]
dec rdi
jnz .top
ret

; a series of independent 256-bit loads from the same location, with location passed as the second parameter
define_bench load256_any
.top:
%rep 64
vmovdqu ymm0, [rsi]
vmovdqu ymm1, [rsi]
%endrep
dec rdi
jnz .top
ret

; a series of independent 256-bit loads from the same location, with location passed as the second parameter
define_bench load512_any
.top:
%rep 64
vmovdqu64 zmm16, [rsi]
vmovdqu64 zmm16, [rsi]
%endrep
dec rdi
jnz .top
ret


; a series of stores to increasing locations without overlap, 1024 total touched
define_bench store64_disjoint
xor eax, eax
sub rsp, 1024
.top:
%assign offset 0
%rep 128
mov [rsp + offset], rax
%assign offset offset+8
%endrep
dec rdi
jnz .top
add rsp, 1024
ret

; Weird case where 32-bit add with memory source runs faster than 64-bit version
; Search for "Totally bizarre" in https://www.agner.org/optimize/blog/read.php?i=423
define_bench misc_add_loop32
push rbp
mov  rbp, rsp
push rbx
mov rcx, rdi
sub rsp, 256
and rsp, -64
lea rax, [rsp + 64]
lea rdi, [rsp + 128]
jmp .top
ALIGN 32
.top:
add edx, [rsp]
mov [rax], edi
blsi ebx, [rdi]
dec ecx
jnz .top

; cleanup
mov rbx, [rbp - 8]
mov rsp, rbp
pop rbp
ret

define_bench misc_add_loop64
push rbp
mov  rbp, rsp
push rbx
mov rcx, rdi
sub rsp, 256
and rsp, -64
lea rax, [rsp + 64]
lea rdi, [rsp + 128]

; oddly, the slowdown goes away when you do a 256-bit AVX op, like
; the ymm vpxor below, but not for xmm (at least with vzeroupper)
;vpxor ymm0, ymm0, ymm0
;vzeroupper
;vpxor xmm0, xmm0, xmm0

jmp .top
ALIGN 32
.top:
add rdx, [rsp]
mov [rax], rdi
blsi rbx, [rdi]
dec ecx
jnz .top
; cleanup
mov rbx, [rbp - 8]
mov rsp, rbp
pop rbp
ret

define_bench misc_port7
mov rax, rsp
mov rsi, [rsp]
xor edx, edx
.top:
mov ecx, [rax]
mov ecx, [rax]
mov [rax + rdx * 8], rsi
dec rdi
jnz .top
ret

define_bench misc_fusion_add
xor eax, eax
.top:
times 128 add ecx, [rsp]
dec rdi
jnz .top
ret

; does add-jo macro-fuse? (no, on Skylake and earlier)
define_bench misc_macro_fusion_addjo
xor eax, eax
.top:
%rep 128
add     eax, 1
jo      .never
%endrep
dec rdi
jnz .top
ret
.never:
ud2

; is adc, 0 treated specially compared to other immediates?
%macro adc_lat_bench 1
define_bench adc_%1_lat
xor eax, eax
xor ecx, ecx
.top:
times 128 adc rax, %1
dec rdi
jnz .top
ret
ud2
%endmacro

adc_lat_bench 0
adc_lat_bench 1
adc_lat_bench rcx

; is adc, 0 treated specially compared to other immediates?
%macro adc_tput_bench 1
define_bench adc_%1_tput
xor eax, eax
xor ecx, ecx
.top:
%rep 128
xor eax, eax
adc rax, %1
%endrep
dec rdi
jnz .top
ret
ud2
%endmacro

adc_tput_bench 0
adc_tput_bench 1
adc_tput_bench rcx

define_bench misc_flag_merge_1
xor eax, eax
.top:
%rep 128
add rcx, 5
inc rax
jna  .never
%endrep
dec rdi
jnz .top
ret
.never:
ud2

define_bench misc_flag_merge_2
xor eax, eax
.top:
%rep 128
add rcx, 5
inc rax
jc  .never
%endrep
dec rdi
jnz .top
ret
.never:
ud2

define_bench misc_flag_merge_3
xor eax, eax
.top:
%rep 128
inc rax
add rcx, 5
jo  .never
%endrep
dec rdi
jnz .top
ret
.never:
ud2

define_bench misc_flag_merge_4
xor eax, eax
.top:
%rep 128
add rcx, 5
dec rax
nop
jg  .never
%endrep
dec rdi
jnz .top
ret
.never:
ud2

define_bench misc_flag_merge_5
xor eax, eax
.top:
%rep 128
add rcx, 5
inc rax
nop
jna  .never
%endrep
dec rdi
jnz .top
ret
.never:
ud2

define_bench misc_flag_merge_6
xor eax, eax
.top:
%rep 128
add rcx, 5
inc rax
cmovbe rdx, r8
%endrep
dec rdi
jnz .top
ret
.never:
ud2

define_bench misc_flag_merge_7
xor eax, eax
.top:
%rep 128
add rcx, 5
inc rax
cmovc rdx, r8
%endrep
dec rdi
jnz .top
ret
.never:
ud2

define_bench misc_flag_merge_8
xor eax, eax
.top:
%rep 128
inc rax
add rcx, 5
cmovbe rdx, r8
%endrep
dec rdi
jnz .top
ret
.never:
ud2

define_bench misc_flag_merge_9
xor eax, eax
mov ecx, 1
mov edx, 1
.top:
%rep 128
and rcx, rdx
nop
jbe .never
%endrep
dec rdi
jnz .top
ret
.never:
ud2

; https://twitter.com/david_schor/status/1106687885825241089
define_bench david_schor1
    push rbx
    mov rdx, rdi
.l:
    inc rax
    inc rbx
    inc rcx
    nop
    nop
    dec rdx
    jnz .l

    pop rbx
    ret

; args:
; %1 the number of cmp, je pairs in the main loop
%macro define_macro_fusion 1
define_bench double_macro_fusion%1
    mov eax, 1
.top:
%rep %1
    cmp eax, 0
    je .never
%endrep
    dec rdi
    jnz .top
    ret
.never:
    ud2
%endmacro

; can two macro fused branches per cycle be sustained?
define_macro_fusion 256
define_macro_fusion 4000


define_bench dendibakh_fused
mov     rax, rdi
shl     rax, 2
sub     rsp, rax
.fused_loop:
inc     DWORD [rsp + rdi * 4 - 4]
dec     rdi
jnz     .fused_loop
add     rsp, rax
ret

define_bench dendibakh_fused_simple
mov     rax, rdi
shl     rax, 2
lea     rcx, [rsp - 4]
sub     rsp, rax
.fused_loop:
inc     DWORD [rcx]
sub     rcx, 4
dec     rdi
jnz     .fused_loop
add     rsp, rax
ret

define_bench dendibakh_fused_add_simple
mov     rax, rdi
shl     rax, 2
lea     rcx, [rsp - 4]
sub     rsp, rax
.fused_loop:
add     DWORD [rcx], 1
sub     rcx, 4
dec     rdi
jnz     .fused_loop
add     rsp, rax
ret

define_bench dendibakh_fused_add
mov     rax, rdi
shl     rax, 2
sub     rsp, rax
.fused_loop:
add     DWORD [rsp + rdi * 4 - 4], 1
dec     rdi
jnz     .fused_loop
add     rsp, rax
ret

define_bench dendibakh_unfused
mov     rax, rdi
shl     rax, 2
sub     rsp, rax
.unfused_loop:
mov     edx, DWORD [rsp + rdi * 4 - 4]
inc     edx
mov     DWORD [rsp + rdi * 4 - 4], edx
dec     rdi
jnz     .unfused_loop
add     rsp, rax
ret

define_bench vz_samereg
xor ecx, ecx
vzeroall
.top:
times 100 vpaddq zmm0, zmm0, zmm0
dec rdi
jnz .top
ret

define_bench vz_diffreg
xor ecx, ecx
vzeroall
.top:
times 100 vpaddq zmm0, zmm1, zmm0
dec rdi
jnz .top
ret

define_bench vz_diffreg16
xor ecx, ecx
vpxorq zmm16, zmm16, zmm16
vzeroall
.top:
times 100 vpaddq zmm0, zmm16, zmm0
dec rdi
jnz .top
ret

define_bench vz_diffreg16xor
xor ecx, ecx
vzeroall
vpxorq xmm16, xmm16, xmm16
.top:
times 100 vpaddq zmm0, zmm16, zmm0
dec rdi
jnz .top
ret


define_bench vz256_samereg
xor ecx, ecx
vzeroall
.top:
times 100 vpaddq ymm0, ymm0, ymm0
dec rdi
jnz .top
ret

define_bench vz256_diffreg
xor ecx, ecx
vzeroall
.top:
times 100 vpaddq ymm0, ymm1, ymm0
dec rdi
jnz .top
ret

define_bench vz128_samereg
xor ecx, ecx
vzeroall
.top:
times 100 vpaddq xmm0, xmm0, xmm0
dec rdi
jnz .top
ret

define_bench vz128_diffreg
xor ecx, ecx
vzeroall
.top:
times 100 vpaddq xmm0, xmm1, xmm0
dec rdi
jnz .top
ret

define_bench vzsse_samereg
xor ecx, ecx
vzeroall
.top:
times 100 paddq xmm0, xmm0
dec rdi
jnz .top
ret

define_bench vzsse_diffreg
xor ecx, ecx
vzeroall
.top:
times 100 paddq xmm0, xmm1
dec rdi
jnz .top
ret

%define fused_unroll_order 4
%define fused_unroll (1 << fused_unroll_order)

define_bench fusion_better_fused
push_callee_saved
xor     eax, eax
xor     edx, edx
xor     r8d, r8d
shr     rdi, fused_unroll_order
mov     rcx, rsi
jmp     .top
ALIGN 64
.top:
%assign offset 0
%rep    fused_unroll
add      r8d, DWORD [rcx ]
add      r9d, DWORD [rcx ]
add     r10d, DWORD [rcx ]
add     r11d, DWORD [rcx ]


test     r12d, 1
test     r13d, 1
test     r14d, 1
test     r15d, 1


; xor     r14d, r10d
; xor     r15d, r11d
%assign offset (offset + 16)
%endrep
add     rcx, (fused_unroll * 16)
dec     rdi
jnz     .top
xor     eax, edx
pop_callee_saved
ret

define_bench fusion_better_unfused
xor     eax, eax
mov     rcx, rsi
jmp     .top
ALIGN 64
.top:
add     eax, DWORD [rcx]
add     edx, DWORD [rcx + 4]
add     rcx, 8
dec     rdi
jnz     .top
xor     eax, edx
ret



%macro bmi_bench 1
define_bench bmi_%1
xor eax, eax
xor ecx, ecx
.top:
times 128 %1 eax, ecx
dec rdi
jnz .top
ret
%endmacro

bmi_bench  tzcnt
bmi_bench  lzcnt
bmi_bench popcnt

%define STRIDE 832  ; 13 * 64

; %1 - bench size in KiB
; %2 - load instruction
; %3 - bench name
%macro load_loop_bench_tmpl 3
; parallel loads with large stride (across pages, defeating prefetcher)
define_bench %3%1
%define SIZE   (%1 * 1024)
xor     ecx, ecx
mov     rdx, rsi
.top:

%define UF 8

%assign s 0
%rep UF
%2      [rdx + STRIDE * s]
%assign s s+1
%endrep

; check if final read of the next unrolled iteration will exceed the requested size
; and if so, wrap back to the start. Due to the runolling this means that the last
; few loads might be skipped, making the effective footprint somewhat smaler than
; requested by something like UF * 0.5 * STRIDE on average (mostly relevant for buffer
; sizes just above a cache size boundary)
lea     edx, [ecx + STRIDE * (UF*2-1) - SIZE]
add     ecx, STRIDE * UF
test    edx, edx
cmovns  ecx, edx
lea     rdx, [rsi + rcx]

dec rdi
jnz .top
ret
%endmacro

%macro all_parallel_benches 2
load_loop_bench_tmpl   16,{%1},%2
load_loop_bench_tmpl   32,{%1},%2
load_loop_bench_tmpl   64,{%1},%2
load_loop_bench_tmpl  128,{%1},%2
load_loop_bench_tmpl  256,{%1},%2
load_loop_bench_tmpl  512,{%1},%2
load_loop_bench_tmpl 2048,{%1},%2
%endmacro

all_parallel_benches {movzx   eax, BYTE},load_loop
all_parallel_benches prefetcht0,prefetcht0_bench
all_parallel_benches prefetcht1,prefetcht1_bench
all_parallel_benches prefetcht2,prefetcht2_bench
all_parallel_benches prefetchnta,prefetchnta_bench

%define UF 8

; all loads happen in parallel, so maximum MLP can be achieved
%macro parallel_mem_bench 3
define_bench parallel_mem_bench_%2

mov     rax, [rsi + region.size]
mov     rsi, [rsi + region.start]
neg     rax

lea     r9, [STRIDE * (2*UF-1) + eax]

xor     ecx, ecx  ; index
mov     rdx, rsi  ; offset into region

.top:

%assign s 0
%rep UF
%1      [rdx + STRIDE * s] %3
%assign s s+1
%endrep

; check if final read of the next unrolled iteration will exceed the requested size
; and if so, wrap back to the start. Due to the runolling this means that the last
; few loads might be skipped, making the effective footprint somewhat smaler than
; requested by something like UF * 0.5 * STRIDE on average (mostly relevant for buffer
; sizes just above a cache size boundary)
lea     edx, [rcx + r9]
add     ecx, STRIDE * UF
test    edx, edx
cmovns  ecx, edx

lea     rdx, [rsi + rcx]

dec rdi
jnz .top
ret
%endmacro

parallel_mem_bench {movzx   eax, BYTE},load,{}
parallel_mem_bench prefetcht0,prefetcht0,{}
parallel_mem_bench prefetcht1,prefetcht1,{}
parallel_mem_bench prefetcht2,prefetcht2,{}
parallel_mem_bench prefetchnta,prefetchnta,{}

parallel_mem_bench mov DWORD,store,{, 42}

; classic pointer chasing benchmark
define_bench serial_load_bench

mov     rsi, [rsi + region.start]

.top:
mov rsi, [rsi]
dec rdi
jnz .top

ret

%ifndef UNROLLB
%define UNROLLB 10
%endif

; %1 size of store in bytes
; %2 full instruction to use
; %3 prefix for benchmark name
%macro define_bandwidth 3
%assign BITSIZE (%1*8)
define_bench %3_bandwidth_ %+ BITSIZE
mov     rdx, [rsi + region.size]
mov     rsi, [rsi + region.start]

xor     eax, eax
vpxor   ymm0, ymm0, ymm0

.top:
mov     rax, rdx
mov     rcx, rsi

.inner:
%assign offset 0
%rep (64 / %1)
%2
%assign offset (offset + %1)
%endrep
%undef offset ; needed to prevent offset from being expanded wrongly in the macro invocations below
add     rcx, 64
sub     rax, 64
jge      .inner

dec rdi
jnz .top
ret
%endmacro

define_bandwidth  4,{mov      [rcx + offset], eax},store
define_bandwidth  8,{mov      [rcx + offset], rax},store
define_bandwidth 16,{vmovdqa  [rcx + offset],xmm0},store
define_bandwidth 32,{vmovdqa  [rcx + offset],ymm0},store
define_bandwidth 64,{vmovdqa64[rcx + offset],zmm0},store

define_bandwidth  4,{mov        r8d, [rcx + offset]},load
define_bandwidth  8,{mov        r8 , [rcx + offset]},load
define_bandwidth 16,{vmovdqa   xmm0, [rcx + offset]},load
define_bandwidth 32,{vmovdqa   ymm0, [rcx + offset]},load
define_bandwidth 64,{vmovdqa64 zmm0, [rcx + offset]},load
define_bandwidth 64,{movzx      r8d, BYTE [rcx + offset]},loadtouch



; version that doesn't interleave the loads in a "clever way"
define_bench bandwidth_test256
mov     rdx, [rsi + region.size]
mov     rsi, [rsi + region.start]

.top:
mov     rax, rdx
mov     rcx, rsi

vpxor ymm0, ymm0, ymm0
vpxor ymm1, ymm1, ymm1

.inner:

%assign offset 0
%rep UNROLLB
vpaddb ymm0, ymm0, [rcx + offset]
vpaddb ymm1, ymm1, [rcx + offset + 32]
%assign offset (offset + 64)
%endrep

add     rcx, UNROLLB * 64
sub     rax, UNROLLB * 64
jge      .inner

dec rdi
jnz .top
ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define_bench bandwidth_test256i_unroll
mov     rdx, [rsi + region.size]
mov     rsi, [rsi + region.start]

.top:
mov     rax, rdx
mov     rcx, rsi
lfence

vpxor ymm0, ymm0, ymm0
vpxor ymm1, ymm1, ymm1
vpxor ymm2, ymm2, ymm2
vpxor ymm3, ymm3, ymm3

.inner:

%assign offset 0
%rep UNROLLB/2
vpaddb ymm0, ymm0, [rcx + offset]
vpaddb ymm1, ymm1, [rcx + offset + 64]
%assign offset (offset + 128)
%endrep
.tail1:
%if (UNROLLB % 2 == 1)
vpaddb ymm0, ymm0, [rcx + offset]
%endif

.second:
%assign offset 32
%rep UNROLLB/2
vpaddb ymm2, ymm2, [rcx + offset]
vpaddb ymm3, ymm3, [rcx + offset + 64]
%assign offset (offset + 128)
%endrep
.tail2:
%if (UNROLLB % 2 == 1)
vpaddb ymm2, ymm2, [rcx + offset]
%endif

add     rcx, UNROLLB * 64
sub     rax, UNROLLB * 64
jge      .inner

dec rdi
jnz .top
ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define_bench bandwidth_test256i_orig
mov     rdx, [rsi + region.size]
mov     rsi, [rsi + region.start]

.top:
mov     rax, rdx
mov     rcx, rsi
lfence

vpxor ymm0, ymm0, ymm0
vpxor ymm1, ymm1, ymm1

.inner:

%assign offset 0
%rep UNROLLB
vpaddb ymm0, ymm0, [rcx + offset]
%assign offset (offset + 64)
%endrep

%assign offset 32
%rep UNROLLB
vpaddb ymm1, ymm1, [rcx + offset]
%assign offset (offset + 64)
%endrep

add     rcx, UNROLLB * 64
sub     rax, UNROLLB * 64
jge      .inner

dec rdi
jnz .top
ret

%ifndef UNROLLX
;%warning UNROLLX defined to default of 1
%define UNROLLX 1
%else
;%warning 'UNROLLX' defined externally to UNROLLX
%endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define_bench bandwidth_test256i_single
mov     rdx, [rsi + region.size]
mov     rsi, [rsi + region.start]

.top:
mov     rax, rdx
sub     rax, UNROLLB * 64 ; reduce main loop iterations since the intro/outro parts handle this
mov     rcx, rsi
lfence

vpxor ymm0, ymm0, ymm0
vpxor ymm1, ymm1, ymm1
vpxor ymm2, ymm1, ymm1

; lead-in loop which reads the first half of the first UNROLLB cache lines
%assign offset 0
%rep UNROLLB
vpaddb ymm0, ymm0, [rcx + offset]
%assign offset (offset + 64)
%endrep

.inner:

%assign offset 0
%rep UNROLLX
vpaddb ymm0, ymm0, [rcx + offset + UNROLLB * 64]
vpaddb ymm1, ymm1, [rcx + offset + 32]
%assign offset (offset + 64)
%endrep



add     rcx, UNROLLX * 64
sub     rax, UNROLLX * 64
jge      .inner

; lead out loop to read the remaining lines
%assign offset 0
%rep UNROLLB
vpaddb ymm0, ymm0, [rcx + offset]
%assign offset (offset + 64)
%endrep

dec rdi
jnz .top
ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define_bench bandwidth_test256i_double
mov     rdx, [rsi + region.size]
mov     rsi, [rsi + region.start]

.top:
mov     rax, rdx
sub     rax, UNROLLB * 64 ; reduce main loop iterations since the intro/outro parts handle this
mov     rcx, rsi
lfence

vpxor ymm0, ymm0, ymm0
vpxor ymm1, ymm1, ymm1
vpxor ymm2, ymm1, ymm1

; lead-in loop which reads the first half of the first UNROLLB cache lines
%assign offset 0
%rep UNROLLB
vpaddb ymm0, ymm0, [rcx + offset]
%assign offset (offset + 64)
%endrep

.inner:

%assign offset 0
%rep UNROLLX
vpaddb ymm0, ymm0, [rcx + offset + UNROLLB * 64]
vpaddb ymm0, ymm0, [rcx + offset + UNROLLB * 64 + 64]
vpaddb ymm1, ymm1, [rcx + offset + 32]
vpaddb ymm1, ymm1, [rcx + offset + 96]
%assign offset (offset + 128)
%endrep



add     rcx, 2 * UNROLLX * 64
sub     rax, 2 * UNROLLX * 64
jge      .inner

; lead out loop to read the remaining lines
%assign offset 0
%rep UNROLLB
vpaddb ymm0, ymm0, [rcx + offset]
%assign offset (offset + 64)
%endrep

dec rdi
jnz .top
ret

;
define_bench serial_load_bench2

xor     eax, eax
add     eax, 0
mov     rsi, [rsi + region.start]

.top:
mov rcx, [rsi + rax]
;add rcx, rax
;add rcx, rax
;xor rsi, rcx ; dummy dependency of rsi on rcx
;xor rsi, rcx
mov rsi, [rcx]
;mov rsi, rcx

dec rdi
jnz .top

ret

; one load only, as a baseline
define_bench serial_double_load_oneload
mov     rsi, [rsi + region.start]
.top:
mov rcx, [rsi]
mov rsi, rcx
dec rdi
jnz .top
ret

; testing latency of 2nd hit on the same L2 line
; dummy load FIRST
define_bench serial_double_load1
mov     rsi, [rsi + region.start]
.top:
mov rax, [rsi]
mov rcx, [rsi + 56]
mov rsi, rcx
dec rdi
jnz .top
ret

; same as above, but separate the first and second load by a cycle
; inserting a dummy ALU op between the two moves
define_bench serial_double_load_alu
mov     rsi, [rsi + region.start]
.top:
mov rax, [rsi]
add rsi, 0
mov rcx, [rsi + 56]
mov rsi, rcx
dec rdi
jnz .top
ret

; dummy load first, but lea rather than mov (no elim) to transfer rcx to rsi
; meaning that it's an ALU op that feeds both loads
define_bench serial_double_load_lea
mov     rsi, [rsi + region.start]
.top:
mov rax, [rsi]
mov rcx, [rsi + 56]
lea rsi, [rcx]
dec rdi
jnz .top
ret

; dummy second, but add dummy to something
define_bench serial_double_load_addd
xor eax, eax
xor edx, edx
mov     rsi, [rsi + region.start]
.top:
mov rcx, [rsi]
add rax, [rsi]
mov rsi, rcx
dec rdi
jnz .top
ret

; same as above, but use indexed addressing mode to see if that
; stops the issue
define_bench serial_double_load_indexed1
and edx, 0 ; must not be zeroing idiom
mov rsi, [rsi + region.start]
.top:
mov rax, [rsi]
mov rcx, [rsi + rdx + 56]
mov rsi, rcx
dec rdi
jnz .top
ret

; as above, but both loads indexed
define_bench serial_double_load_indexed2
;xor edx, edx
and edx, 0
mov rsi, [rsi + region.start]
.top:
mov rax, [rsi + rdx]
mov rcx, [rsi + rdx + 56]
mov rsi, rcx
dec rdi
jnz .top
ret

; as above, but key load first
define_bench serial_double_load_indexed3
;xor edx, edx
and edx, 0
mov rsi, [rsi + region.start]
.top:
mov rcx, [rsi + rdx + 56]
mov rax, [rsi + rdx]
mov rsi, rcx
dec rdi
jnz .top
ret

; testing latency of 2nd hit on the same L2 line
; dummy load SECOND
define_bench serial_double_load2
mov     rsi, [rsi + region.start]
.top:
mov rcx, [rsi + 56]
mov rax, [rsi]
mov rsi, rcx
dec rdi
jnz .top
ret

; testing latency of demand hit on the same L2 line after PF
; dummy load SECOND
; %1 test name suffix
; %2 offset for prefetch
; %3 pf hint
%macro make_serial_double_loadpf 3
define_bench serial_double_load%1
mov     rsi, [rsi + region.start]
.top:
prefetch%3 [rsi + %2]
mov rcx, [rsi + 56]
mov rsi, rcx
dec rdi
jnz .top
ret
%endmacro

make_serial_double_loadpf pf_diff,    0, t0
make_serial_double_loadpf pf_same,   56, t0
make_serial_double_loadpf pft1_diff,  0, t1


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;                                           ;;
;; GATHER GATHER GATHER GATHER GATHER GATHER ;;
;;                                           ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
%macro define_gather_bench 1

%define vec0 %1 %+ 0
%define vec1 %1 %+ 1
%define vec2 %1 %+ 2
%define vec3 %1 %+ 3
%define vec4 %1 %+ 4

define_bench gatherdd_%1
mov  r10, rsp
vzeroall

sub rsp,  1024
and rsp,  -64
mov rdx, rsp

xor eax, eax

vpxor    vec0, vec0, vec0

vmovdqa     vec1, [CONST_DWORD_0_7]
vmovdqa     vec3, vec1

vpcmpeqd    vec4, vec4, vec4

.top:
%rep 16
vmovdqa     vec2, vec4
vpxor       vec3, vec3, vec3
vpgatherdd  vec3, [rsp + vec1], vec2
%endrep
dec rdi
jnz .top

lfence

mov rsp, r10
ret
%endmacro

define_gather_bench xmm
define_gather_bench ymm

;;;;;;;;; gather latency ;;;;;;;;;;
%macro define_gather_lat_bench 1

%define vec0 %1 %+ 0
%define vec1 %1 %+ 1
%define vec2 %1 %+ 2
%define vec3 %1 %+ 3
%define vec4 %1 %+ 4
%define vec5 %1 %+ 5

define_bench gatherdd_lat_%1
mov  r10, rsp
vzeroall

sub rsp,  1024
and rsp,  -64
mov rdx, rsp

xor eax, eax

vpxor       vec0, vec0, vec0
vpcmpeqd    vec4, vec4, vec4
vpxor       vec5, vec5, vec5

vmovdqa     vec1, [CONST_DWORD_0_7]
vmovdqa     [rsp], vec1

.top:
%rep 16
vmovdqa     vec2, vec4
vpgatherdd  vec0, [rsp + vec1 * 4], vec2
vpand       vec0, vec0, vec5
vpor        vec1, vec1, vec0 ; make vec1 (indexes) depend on vec0 (result)
%endrep
dec rdi
jnz .top

mov rsp, r10
ret
%endmacro

define_gather_lat_bench xmm
define_gather_lat_bench ymm

; retpoline stuff

; the generic retpoline thunk, parameterized on the loop instruction
%macro retpoline_thunk 1
retpoline_thunk_%1:
call    .target
.loop:
%1
jmp .loop
.target:
lea rsp, [rsp + 8]
ret
%endmacro

%macro retpo_call 2
jmp     %%call
%%jmpthunk:
push %1
jmp retpoline_thunk_%2
%%call:
call %%jmpthunk
%endmacro


retpoline_thunk pause
retpoline_thunk lfence

ALIGN 16
empty_func:
ret

%macro body 0
call empty_func
%endmacro

%assign depth 128
%rep depth
call_chain_ %+ depth :
%assign depth depth-1
call call_chain_ %+ depth
ret
%endrep

call_chain_0:
ret

%define dense_nop_padding nop5

%macro retpoline_dense_call 1
define_bench retpoline_dense_call_%1
push r15
lea r15, [empty_func]
.top:
%rep 32
retpo_call r15,%1
dense_nop_padding
%endrep
dec rdi
jnz .top
pop r15
ret
%endmacro

retpoline_dense_call lfence
retpoline_dense_call pause

%define IMUL_COUNT 20
define_bench retpoline_sparse_call_base
push r15
lea r15, [empty_func]
.top:
%rep 8
times IMUL_COUNT imul eax, eax, 1
%endrep
dec rdi
jnz .top
pop r15
ret

%macro retpoline_sparse_indep_call 1
define_bench retpoline_sparse_indep_call_%1
push r15
lea r15, [empty_func]
.top:
%rep 8
retpo_call r15,%1
times IMUL_COUNT imul rax, rax, 1
%endrep
dec rdi
jnz .top
pop r15
ret
%endmacro

%macro retpoline_sparse_dep_call 1
define_bench retpoline_sparse_dep_call_%1
push r15
lea r15, [empty_func]
.top:
%rep 8
retpo_call r15,%1
times IMUL_COUNT imul r15, r15, 1
%endrep
dec rdi
jnz .top
pop r15
ret
%endmacro

retpoline_sparse_indep_call lfence
retpoline_sparse_indep_call pause
retpoline_sparse_dep_call lfence
retpoline_sparse_dep_call pause

define_bench indirect_dense_call_pred
push r15
lea r15, [empty_func]
.top:
%rep 32
call r15
dense_nop_padding
%endrep
dec rdi
jnz .top
pop r15
ret

define_bench indirect_dense_call_unpred
push r14
push r15
xor r14, r14
.top:
add  r14, 11
and  r14, 127
lea r15, [empty_func0 + r14 * 8]
%rep 32
call r15
%endrep
dec rdi
jnz .top
pop r15
pop r14
ret

; empty functions spaced out every 8 bytes
%assign i 0
%rep 128
empty_func %+ i :
ret
nop7
%assign i i+1
%endrep

%macro dsb_body 0
times 2 nop8
.outer:
mov     rax, 128
.top:
nop9
dec     rax
jne     .top
dec     rdi
jnz     .outer
ret
ud2
%endmacro

GLOBAL dsb_alignment_cross64,dsb_alignment_nocross64

ALIGN 64
times 4 nop8
; the loop ends up crossing a 64-byte boundary
dsb_alignment_cross64:
dsb_body

ALIGN 64
dsb_alignment_nocross64:
dsb_body


%define ADC_INNER_ITERS 250

; %1 name suffix
; %2 reg to use as src/dest
%macro define_adc_bench 2
define_bench adc_chain%1
push rbp
mov  rbp, rsp

mov  r9, rdi

sub  rsp, ADC_INNER_ITERS * 3 * 32

vzeroupper
;vpor    ymm0, ymm0, ymm0

.top:

mov  rcx, ADC_INNER_ITERS

lea  rsi, [rsp]
lea  rdx, [rsp + ADC_INNER_ITERS * 32]
lea  rdi, [rsp + ADC_INNER_ITERS * 32 * 2]

;vpor    ymm0, ymm0, ymm0
;vzeroupper
;vpor    xmm0, xmm0, xmm0

ALIGN 64
.Loop:

%assign offset 0
%rep 4

mov     %2, [rsi + offset]
mov     %2, [rsi + offset]
mov     [rdi + offset], %2

%assign offset (offset + 0)
%endrep

sub     rcx, 1
jne     .Loop

dec     r9
jnz     .top

mov     rsp, rbp
pop     rbp

ret
%endmacro


define_adc_bench 32, eax
define_adc_bench 64, rax


struc quadargs
    small:  resq 1
    ssize:  resq 1
    large:  resq 1
endstruc

%macro define_quadratic 1
define_bench quadratic%1
        times %1 nop
        mov rdx, rsi
        mov esi, 1000
        mov     ecx, 1 ; the array is all 0s so this never matches
.outer:
        xor     eax, eax

.inner:
        cmp     dword [rdx+rax*4], ecx
        jz      .match
        inc     eax
        cmp     eax, 1000
        jne     .inner

        dec     rdi
        jnz     .outer

        ret
.match:
        ud2 ; not possible because we input distinct arrays
%endmacro

%assign i 0
%rep 64
define_quadratic i
%assign i i+1
%endrep

; define the weird store bench
; %1 suffix
; %2 zeroing instruction
%macro define_weird_store 2
define_bench weird_store_%1
push rbp
mov rbp, rsp
sub rsp, 8196
mov rsi, rsp
mov eax, edi
.oloop:
mov ecx, 1000
%2
align 16
.iloop:
mov [rsi], edi
mov [rsp], edi
sub rsp, 4
dec ecx
jnz .iloop
lea rsp, [rbp - 8196]
dec eax
jnz .oloop

mov rsp, rbp
pop rbp
ret
%endmacro

define_weird_store mov,{mov edi, 0}
define_weird_store xor,{xor edi, edi}


; https://news.ycombinator.com/item?id=15935283
; https://eli.thegreenplace.net/2013/12/03/intel-i7-loop-performance-anomaly
define_bench loop_weirdness_fast
.top:
add     QWORD [rsp - 8], 1
times 10 mov eax, 1
dec     rdi
jne     .top
rep ret

define_bench tight_loop1
.top:
dec     rdi
jne     .top
rep ret


define_bench tight_loop2
.top:
xor eax, eax
jz .l1
nop
.l1:
dec     rdi
jne     .top
rep ret

define_bench tight_loop3
.top:
xor eax, eax
jnz .l1
nop
.l1:
dec     rdi
jne     .top
rep ret

%macro fwd_lat_with_delay 1
define_bench fwd_lat_delay_%1
lea     rdx, [rsp - 8]
.top:
mov     rcx, [rdx]
mov     [rdx], rcx
times   %1  add rdx, 0
dec     rdi
jne     .top
rep ret
%endmacro

fwd_lat_with_delay 0
fwd_lat_with_delay 1
fwd_lat_with_delay 2
fwd_lat_with_delay 3
fwd_lat_with_delay 4
fwd_lat_with_delay 5

define_bench train_noalias
;mov     r8, rbx
;xor     eax, eax
;cpuid
;mov     rbx, r8
lea     rdx, [rsi + 64]
mov     rdi, 520000
xor     ecx, ecx
.top:
%rep 100
imul    rdx, 1
mov     DWORD [rdx], ecx
mov     eax, DWORD [rsi]
nop3
%endrep
dec     rdi
jnz     .top
ret
ud2



define_bench oneshot_try2_4
mov     rdx, rsi
%rep 4
imul    rdx, 1
mov     DWORD [rdx], 0
mov     eax, DWORD [rsi]
%endrep
ret
ud2

define_bench oneshot_try2_10
mov     rdx, rsi
%rep 10
imul    rdx, 1
mov     DWORD [rdx], 0
mov     eax, DWORD [rsi]
%endrep
ret
ud2

define_bench oneshot_try2_1000
mov     rdx, rsi
%rep 1000
imul    rdx, 1
mov     DWORD [rdx], 0
mov     eax, DWORD [rsi]
%endrep
ret
ud2

define_bench oneshot_try2_20
mov     rdx, rsi
%rep 20
imul    rdx, 1
mov     DWORD [rdx], 0
mov     eax, DWORD [rsi]
%endrep
ret
ud2

define_bench oneshot_try2
mov     rdx, rsi
%rep 100
imul    rdx, 1
mov     DWORD [rdx], 0
mov     eax, DWORD [rsi]
%endrep
ret
ud2

define_bench oneshot_try1
%rep 100
mov     DWORD [rsi], 0
mov     eax, DWORD [rsi]
%endrep
ret
ud2

define_bench aliasing_loads
mov     rdx, rsi
;xor     ecx, ecx
times 0 nop
lfence
%rep 5
imul    rdx, 1
mov     DWORD [rdx], 0
mov     eax, DWORD [rsi]
%endrep
ret
ud2

%ifnnum NOPCOUNT
%define NOPCOUNT 0
%endif

%macro nops 0
jmp near %%skipnop
times NOPCOUNT nop
%%skipnop:
%endmacro


; separate training and timed regions
define_bench aliasing_loads_raw

    mov     r8, rdx ; rdx:rax are clobbered by rdpmc macros

    readpmc4_start

    mov     ecx, 10

.outer:

    lea     r11, [rsi + 64]
    mov     r10, 2000

    ; training loop
.train_top:
%rep 1
    times 3 imul    r11, 1
    mov     DWORD [r11], 0
    mov     eax, DWORD [rsi]
    nop
%endrep
    ;times 10 imul    r11, 1
    dec     r10
    jnz     .train_top

    lea     r9, [rsi + 8]

    ; this lfence is critical because we want to finish all the instructions related to
    ; training before we start the next section, since otherwise our attempt to delay
    ; the store may not work (e.g., because the training section will probably end
    ; with a backlog of imul that will execute and retire 1 every 3 cycles, so we will
    ; just take the instructions in the next cycle one every 3 cycles, so OoO can't do
    ; its magic
    lfence
    mov     rdi, 1
    jmp .top
ALIGN 256
.top:

    nops
%rep 2
    times 5 imul    r9, 1
    ; payload load(s)
    mov     DWORD [r9], 0
    add     eax, DWORD [rsi + 8]
%endrep

    dec     rdi
    jnz     .top

    lfence

    dec     ecx
    jnz     .outer

    readpmc4_end
    store_pfcnow4 r8
    ret
    ud2

%ifndef REPCOUNT
%define REPCOUNT 10
%endif


; mixed aliasing and non-aliasing loads
define_bench mixed_loads_raw

    mov     r8, rdx ; rdx:rax are clobbered by rdpmc macros

    readpmc4_start

    xor     eax, eax
    mov     ecx, 1

.outer:

    lea     r11, [rsi+ 64]
    mov     rdi, 1000

.top:
%assign r 1
%rep REPCOUNT
    ; rax is always zero in this loop
    times 2 lea    r11, strict [byte r11 + rax*2 + 0] ; delay the store address
    mov     DWORD [r11 + rax], eax ; the store to [rsi + 64]
    mov     r9d, DWORD [rsi + 64]  ; aliasing load
    nop9
    nop6
    mov     eax, DWORD [rsi]       ; non-aliasing load
    imul    eax, 1                 ; make the eax dep chain longer

%if r == (REPCOUNT/2)
    ; half way through the unroll, we add 19 bytes of nop so that the the aliasing loads
    ; in the second half of the loop collide with the non-aliasing loads in the second
    ; half and vice versa. Without this, we'll never get any slowdown, because even though
    ; we get collisions, the prediction is always exactly correct since aliasing loads collide
    ; with aliasing loads after "wrap around" (and non-aliasing with aliasing)
    nop9
    nop9
    nop1
%endif
%assign r r+1
%endrep

    dec     rdi
    jnz     .top

    lfence

    dec     ecx
    jnz     .outer

    readpmc4_end
    store_pfcnow4 r8
    ret
    ud2

%define ONESHOT_REPEAT_OUTER 1
%define ONESHOT_REPEAT_INNER 1
; like the fwd_lat_with_delay above, but without a loop, used in oneshot mode to
; examine transient behavior with code code
%macro fwd_lat_delay_oneshot 1
define_bench fwd_lat_delay_oneshot_%1
push    rbx

xor     eax, eax
cpuid
xor     eax, eax
xor     ecx, ecx
lea     rdx, [rsi + 8]

; put the CPU into hoist-OK mode (watchdog off)
mov     rdi, 1600
.topgood:
times 10 imul    rsi, 1
mov     [rax + rsi], rcx
times 2 mov     rax, [rdx]
times 5 imul    rsi, 1
mov     [rax + rsi], rcx
nop
times 2 mov     rax, [rdx]
times 5 imul    rsi, 1
mov     [rax + rsi], rcx
nop
nop
times 2 mov     rax, [rdx]
times 5 imul    rsi, 1
mov     [rax + rsi], rcx
nop
nop
nop
times 2 mov     rax, [rdx]
times 5 imul    rsi, 1
mov     [rax + rsi], rcx
times 2 mov     rax, [rdx]
dec     rdi
jnz     .topgood

; put the CPU into hoist-not-OK mode
%rep 10
times 5 imul rax, 1
mov     [rsi + rax], rcx
mov     rdi, [rsi]
mov     rdi, [rsi]
mov     rdi, [rsi]
mov     rdi, [rsi]
mov     rdi, [rsi]
mov     rdi, [rsi]
%endrep

pop     rbx
ret

mov     rdi, 500
.top:
; chain of instructions that is much slower when loads aren't hoisted above stores with unknwon
; addresses
mov     [rsi + rax], rcx
mov     rax, [rdx]
dec     rdi
jnz     .top

ret

.other:
mov     rdi, 1000
.top1:
mov     [rax], DWORD 1
mov     rsi, [rdx]
dec     rdi
jnz     .top1

%rep ONESHOT_REPEAT_OUTER
%rep ONESHOT_REPEAT_INNER
imul    rdx, 1
imul    rdx, 1
imul    rdx, 1
mov     [rdx], rsi
add     rcx, [rax + 8]
imul    rdx, 1
imul    rdx, 1
imul    rdx, 1
mov     [rdx], rsi
add     rcx, [rax + 8]
imul    rdx, 1
imul    rdx, 1
imul    rdx, 1
mov     [rdx], rsi
add     rcx, [rax + 8]
imul    rdx, 1
imul    rdx, 1
imul    rdx, 1
mov     [rdx], rsi
add     rcx, [rax + 8]

mov     rdi, 1
.top2:
imul    rax, 1
imul    rax, 1
imul    rax, 1
imul    rax, 1
mov     [rax], DWORD 0
mov     rsi, [rdx]
mov     rsi, [rdx]
mov     rsi, [rdx]
mov     rsi, [byte rdx]
mov     rsi, [rdx]
mov     rsi, [rdx]
mov     rsi, [rdx]
mov     rsi, [rdx]
mov     rsi, [rdx]
mov     rsi, [rdx]
mov     rsi, [byte rdx]
mov     rsi, [rdx]
mov     rsi, [rdx]
mov     rsi, [rdx]
mov     rsi, [rdx]
mov     rsi, [rdx]
mov     rsi, [rdx]
mov     rsi, [byte rdx]
mov     rsi, [rdx]
mov     rsi, [rdx]
mov     rsi, [rdx]
dec     rdi
jnz     .top2

imul    rdx, 1
imul    rdx, 1
imul    rdx, 1
mov     [rdx], rsi
add     rcx, [rax + 8]
imul    rdx, 1
imul    rdx, 1
imul    rdx, 1
mov     [rdx], rsi
add     rcx, [rax + 8]
imul    rdx, 1
imul    rdx, 1
imul    rdx, 1
mov     [rdx], rsi
add     rcx, [rax + 8]


;times   %1  add rcx, 0
%endrep
%rep ONESHOT_REPEAT_INNER * 0
mov     [rdx], rcx
mov     rcx, [rax - 8]
;times   %1  add rdx, 0
%endrep
%endrep
ret
ud2
%endmacro



fwd_lat_delay_oneshot 0
fwd_lat_delay_oneshot 1
fwd_lat_delay_oneshot 2
fwd_lat_delay_oneshot 3
fwd_lat_delay_oneshot 4
fwd_lat_delay_oneshot 5

%macro fwd_tput_conc 1
define_bench fwd_tput_conc_%1
sub     rsp, 256
lea     rdx, [rsp - 8]
.top:
%assign offset 0
%rep %1
mov     rcx, [rdx + offset]
mov     [rdx + offset], rcx
%assign offset offset+8
%endrep
dec     rdi
jne     .top
add     rsp, 256
rep ret
%endmacro

fwd_tput_conc 1
fwd_tput_conc 2
fwd_tput_conc 3
fwd_tput_conc 4
fwd_tput_conc 5
fwd_tput_conc 6
fwd_tput_conc 7
fwd_tput_conc 8
fwd_tput_conc 9
fwd_tput_conc 10

%macro bypass_mov_latency 1
define_bench bypass_%1_latency
sub     rsp, 120
xor     eax, eax
vpxor   xmm1, xmm1, xmm1
vpsubb  xmm1, xmm1, [rsp + rax] ; to exactly cancel out the vpaddb below
.top:
%1      xmm0, [rsp + rax] ; 6 cycles
vpaddb  xmm0, xmm0, xmm1  ; 1 cycle
vmovq   rax, xmm0         ; 2 cycles
dec     rdi
jnz     .top
add     rsp, 120
ret
%endmacro

bypass_mov_latency vmovdqa
bypass_mov_latency vmovdqu
bypass_mov_latency vmovups
bypass_mov_latency vmovupd

define_bench bypass_movq_latency
vpxor    xmm0, xmm0
.top:
vmovq    rax,  xmm0         ; 1 cycle
vmovq    xmm0,  rax         ; 1 cycle
dec     rdi
jnz     .top
ret

define_bench bypass_movd_latency
vpxor   xmm0, xmm0, xmm0
.top:
vmovd    eax,  xmm0         ; 1 cycle
vmovd    xmm0,  eax         ; 1 cycle
dec     rdi
jnz     .top
ret

%macro vector_load_load_lat 2
define_bench vector_load_load_lat_%1_%2
push    rbp
vzeroupper
mov     rbp, rsp
sub     rsp, 128
and     rsp, -64
xor     eax, eax
vpxor   xmm0, xmm0, xmm0
vmovdqu [rsp], ymm0
vmovdqu [rsp + 32], ymm0
vmovdqu [rsp + 64], ymm0
vmovdqu [rsp + 96], ymm0
.top:
%1      xmm0, [rsp + rax + %2] ; 6 cycles
vmovq   rax, xmm0         ; 2 cycles
dec     rdi
jnz     .top
mov     rsp, rbp
pop     rbp
ret
%endmacro

vector_load_load_lat   movdqu,  0
vector_load_load_lat   vmovdqu, 0
vector_load_load_lat   lddqu,   0
vector_load_load_lat   vlddqu,  0
vector_load_load_lat   movdqu,  63
vector_load_load_lat   vmovdqu, 63
vector_load_load_lat   lddqu,   63
vector_load_load_lat   vlddqu,  63




define_bench raw_rdpmc0_overhead
assert_eq rdi, 0
assert_eq rsi, 0
mov     r8, rdx
readpmc0_start
readpmc0_end
store_pfcnow0 r8
ret

define_bench raw_rdpmc4_overhead
assert_eq rdi, 0
assert_eq rsi, 0
mov     r8, rdx
readpmc4_start
readpmc4_end
store_pfcnow4 r8
ret


; store loop with raw rdpmc calls
define_bench store_raw_libpfc
; loop count should be 1 since we don't have a loop here
assert_eq rdi, 1
sub     rsp, 128
mov     r8, rdx
readpmc4_start
.top:
%assign offset 0
%rep 100
mov     [rsp], BYTE 42
;sfence
%assign offset offset+1
%endrep
readpmc4_end
store_pfcnow4 r8
add     rsp, 128
ret



define_bench syscall_asm
.top:
mov eax, esi
syscall
dec rdi
jnz .top
ret

define_bench syscall_asm_lfence_before
.top:
mov eax, esi
lfence
syscall
dec rdi
jnz .top
ret

define_bench syscall_asm_lfence_after
.top:
mov eax, esi
syscall
lfence
dec rdi
jnz .top
ret

define_bench lfence_only
.top:
times 8 lfence
dec rdi
jnz .top
ret

ud2

%define STRIDE 12736  ; 13 * 64


%define SIZE   (1 << 25)
%define MASK   (SIZE - 1)

; parallel loads with large stride (across pages, defeating prefetcher)
%macro parallel_miss_macro 1
xor     edx, edx
.top:
mov     eax, DWORD [rsi + rdx]
%1
add     rdx, STRIDE
and     rdx, MASK
dec     rdi
jnz     .top
ret
%endmacro

define_bench parallel_misses
parallel_miss_macro nop

define_bench lfenced_misses
parallel_miss_macro lfence

define_bench mfenced_misses
parallel_miss_macro mfence

define_bench sfenced_misses
parallel_miss_macro sfence

%macro syscall_123456 0
mov     eax, 123456
syscall
%endmacro

define_bench syscall_misses
parallel_miss_macro syscall_123456

%macro syscall_123456_lfence 0
lfence
syscall_123456
%endmacro

define_bench syscall_misses_lfence
parallel_miss_macro syscall_123456_lfence


ud2

; constants

align 32
CONST_DWORD_0_7:
dd 0,64,128,192,256,320,384,448

