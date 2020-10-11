%include "x86-helpers.asm"

nasm_util_assert_boilerplate
thunk_boilerplate

; branch notes
;
; The BTB seems to have a 32-byte granularity and only the first taken
; branch in a 32-byte region gets a "fast" back-to-back prediction, of
; 1 cycle
; the second branch in a 32-byte region appears to take at least 2 cycles
; sometimes longer, exhibiting patterns such as 5.0, 5.67 or 6.33 cycles for
; two such branches for the *same* benchmark, from run to run
; an lfence inside the loop appears to remove the variance (only the
; fastest timing shows)
; an lfence before the benchmark but outside the loop seems to reduce
; the variance
;
;
; indirect branches
;
; indirect seem to have a best-case throughput of 1 per 2 cycles,
; regardless of spacing

%macro jmp_forward 1
%assign jump_bytes %1
%if %1 < 2
%error jump should be at least two bytes
%assign jump_bytes 2
%endif
    jmp %%target
    times ((jump_bytes-2) / 3) nop3
    times ((jump_bytes-2) % 3) nop
%%target:
%undef jump_bytes
%endmacro


; make a benchmark that jumps forward by %1 bytes
; then by %2 bytes, repeated 10 times
;
; shows the important of jump spacing on BTB access
; and or prediction latency
;
; %1 first jump distance
; %2 second jump distance
%macro define_jmp_forward 2
define_bench jmp_forward_%1_%2
    xor eax, eax
    clc
    jmp .top

align 64
.top:

%rep 10
    jmp_forward %1
    jmp_forward %2
%endrep

    dec rdi
    jnz .top
    nop2
    ret
%endmacro

define_jmp_forward  8,  8
define_jmp_forward 16, 16
define_jmp_forward 32, 32
define_jmp_forward 64, 64

%macro indirect_forward 1
%%before:
    add rax, %%target - %%before
    jmp rax
%%after:

%assign rem (%1 - (%%after - %%before))
%if rem < 0
%error indirect jump bytes too small: smaller than lea/jmp pair
%endif
    times (rem / 3) nop3
    times (rem % 3) nop
%%target:

%undef rem
%endmacro

%macro define_indirect_forward 1
define_bench indirect_forward_%1
    xor eax, eax
    clc
    jmp .top

align 64
.top:
    lea rax, [.before]

.before:

%rep 20
    indirect_forward %1
%endrep

    dec rdi
    jnz .top
    nop2
    ret
%endmacro

define_indirect_forward  8
define_indirect_forward 16
define_indirect_forward 32
define_indirect_forward 64

%macro define_indirect_variable 1
define_bench define_indirect_variable
    xor eax, eax
    clc
    lea rax, [.nops]

    jmp .top
align 64
.top:

.before:

    mov rdx, rdi
    and rdx, 63
    lea rdx, [rax + rdx * 2]
    jmp rdx

.nops:
    times 64 nop2

    dec rdi
    jnz .top
    nop2
    ret
%endmacro

define_indirect_variable 0