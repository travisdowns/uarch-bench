BITS 64
default rel

%include "nasm-utils/nasm-utils-inc.asm"

thunk_boilerplate

; aligns and declares the global label for the bench with the given name
; also potentally checks the ABI compliance (if enabled)
%macro define_bench 1
ALIGN 32
abi_checked_function %1
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
GLOBAL add_calibration:function
ALIGN 32
add_calibration:
sub rdi, 1
jnz add_calibration
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

define_bench dep_imul64_rax
xor eax, eax
.top:
times 128  imul rax, rax
dec rdi
jnz .top
ret

; because the 64x64=128 imul uses an implicit destination & first source
; we need to clear out eax each iteration to make it idenpendent, although
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

; a dependent chain of multiplications
define_bench dep_mul_64
xor eax, eax
.top:
times 128 add rax, rax
dec rdi
jnz .top
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
vpxor xmm0, xmm0
.top:
times 128 vmovdqu [rsi], xmm0
dec rdi
jnz .top
ret

; a series of AVX (REX-encoded) 256-bit stores to the same location, passed as the second parameter
define_bench store256_any
vpxor xmm0, xmm0
.top:
times 128 vmovdqu [rsi], ymm0
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
jmp .top
ALIGN 32
.top:
add rdx, [rsp]
mov [rax], edi
blsi ebx, [rdi]
dec ecx
jnz .top

; cleanup
mov rbx, [rbp - 8]
mov rsp, rbp
pop rbp
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
%define MASK   (SIZE - 1)
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
