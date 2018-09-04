BITS 64
default rel

%include "nasm-utils/nasm-utils-inc.asm"
%include "x86_helpers.asm"

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
;vpaddb ymm0, ymm0, [rcx + offset]
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
;vpaddb ymm0, ymm0, [rcx + offset]
%assign offset (offset + 64)
%endrep

dec rdi
jnz .top
ret



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

