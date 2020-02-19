
%include "x86-helpers.asm"

nasm_util_assert_boilerplate
thunk_boilerplate

; segregate some particular benchamrk here if you want to repeatedly compile different versions of it quickly

%ifndef UNROLLB
;%warning 'UNROLLB' defined to default of 1
%define UNROLLB 1
%endif

%ifndef UNROLLX
;%warning UNROLLX defined to default of 1
%define UNROLLX 1
%else
;%warning 'UNROLLX' defined externally to UNROLLX
%endif

;; do a loop over the first half of all the cache linnes, then loop
;; over the second half
;; performance is crap
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define_bench bandwidth_test256_2loops
mov     rdx, [rsi + region.size]
mov     rsi, [rsi + region.start]

.top:
mov     rax, rdx
mov     rcx, rsi
lfence

vpxor ymm0, ymm0, ymm0
vpxor ymm1, ymm1, ymm1
vpxor ymm2, ymm2, ymm2

.inner:

mov r9, rcx
lea r8, [rcx + UNROLLB * 64]

.firsttop:
vpaddb ymm0, ymm0, [rcx]
add rcx, 64
cmp rcx, r8
jb  .firsttop

lea rcx, [r9 + 32]
lea r8,  [rcx + UNROLLB * 64]

.secondtop:
vpaddb ymm1, ymm1, [rcx]
vpaddb ymm2, ymm2, [rcx + 64]
add rcx, 128
cmp rcx, r8
jb  .secondtop

mov     rcx, r9

add     rcx, UNROLLB * 64
sub     rax, UNROLLB * 64
jge      .inner

dec rdi
jnz .top
ret


;; Interleaved 2-pass
;; the main loop does UNROLLB first half reads
;; then does UNROLLB second half reads hopefully finding the line in L1
;; OK but very jaggy performance
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define_bench bandwidth_test256_2pass
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
%rep UNROLLB/2
vpaddb ymm0, ymm0, [rcx + offset]
vpaddb ymm1, ymm1, [rcx + offset + 64]
%assign offset (offset + 128)
%endrep
%if (UNROLLB % 2 == 1)
vpaddb ymm0, ymm0, [rcx + offset]
%endif


add     rcx, UNROLLB * 64
sub     rax, UNROLLB * 64
jge      .inner

dec rdi
jnz .top
ret

;; the main loop interleaves in a fine-grained way an initial access
;; to a cache line, and then the second access to the cache line with
;; the former running UNROLLB lines ahead of the latter (once UNROLLB
;; gets to about 5 or 6 it seems the second access hits in L1 and max
;; speed is achieved) - good and very flat performance approaching
;; exactly 1.5 cycles/line
;;
;; UNROLLX must be even for it to work properly (to "pair up" the reads hitting L1)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define_bench bandwidth_test256i
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
%assign offset (offset + 64)
%endrep

%assign offset 0
%rep UNROLLX
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

define_bench movd_xmm
vzeroall
.top:
    %rep 100
    vpor   xmm0, xmm0, xmm0
    movd   eax, xmm0
    movd   xmm0, eax
    %endrep
    dec rdi
    jnz .top
    ret

define_bench movd_ymm
    vzeroupper
    vpor ymm0, ymm0, ymm0
.top:
    %rep 100
    vpor   ymm0, ymm0, ymm0
    movd   eax, xmm0
    movd   xmm0, eax
    %endrep
    dec rdi
    jnz .top
    ret


define_bench rep_movsb
    sub rsp, 1024
    mov r8, rdi
.top:
%rep 100
    mov ecx, 1024
    mov rdi, rsp
    rep stosb
%endrep
    dec r8
    jnz .top

    add     rsp, 1024
    ret
