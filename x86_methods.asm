BITS 64
default rel

%include "nasm-utils/nasm-utils-inc.asm"

thunk_boilerplate

; aligns and declares the global label for the bench with the given name
; also potentally checks the ABI compliance (if enabled)
%macro define_bench 1
ALIGN 64
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

; a benchmark that immediately returns, useful as the "base" method
; to cancel out some forms of overhead
define_bench dummy_bench
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

define_bench dense_calls
.top:
%rep 16
call dummy_bench
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
call dummy_bench
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

; %1 - bench size in KiB
; %2 - bench name
%macro serial_load_bench_tmpl 1
; parallel loads with large stride (across pages, defeating prefetcher)
define_bench serial_load_bench%1
%define SIZE   (%1 * 1024)
%define MASK   (SIZE - 1)
xor     eax, eax
xor     ecx, ecx
mov     rdx, rsi
.top:

%define UF 8

%assign s 0
%rep UF
mov     rax, [rax + rdx + STRIDE * s]
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

serial_load_bench_tmpl   16
serial_load_bench_tmpl   32
serial_load_bench_tmpl   64
serial_load_bench_tmpl  128
serial_load_bench_tmpl  256
serial_load_bench_tmpl  512
serial_load_bench_tmpl 2048

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

; https://news.ycombinator.com/item?id=15935283
; https://eli.thegreenplace.net/2013/12/03/intel-i7-loop-performance-anomaly
define_bench loop_weirdness_fast
.top:
add     QWORD [rsp - 8], 1
times 10 mov eax, 1
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
