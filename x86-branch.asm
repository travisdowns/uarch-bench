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
    and rdx, 127
    lea rdx, [rax + rdx * 2]
    jmp rdx

.nops:
    times 128 nop2

    dec rdi
    jnz .top
    nop2
    ret
%endmacro

define_indirect_variable 0

; ALIGNMODE p6,nojmp

%define LAT_BODY_SIZE 64
%define LAT_BODY_COUNT_LG2 3
%define LAT_BODY_COUNT (1 << LAT_BODY_COUNT_LG2)

; %1 1st instruction
; %2 2nd instruction
%macro lat_body 2
    ; chaining instructions
    %1
    %2
    ; do the LCG
    imul    rdx, rax ; x*a
    add     rdx, rcx ; + c
    mov      r8, rdx
    shr      r8, 45  ; high bits have much longer cycles
    and     r8d, (LAT_BODY_COUNT - 1)
    imul     r8, LAT_BODY_SIZE
    lea      r8, [r8 + .top]
    dec     rdi
    cmovz    r8, r11 ; jump to end when rdi == 0
    jmp      r8
%endmacro

; %1 name suffix
; %2 fist payload instruction
; %3 fist payload instruction
%macro define_load_alloc 3
define_bench la_%1
    push 0
    xor r10d, r10d
    lea r11, [rel .end]
    mov rax, 6364136223846793005
    mov ecx, 1
    mov rdx, rax

    jmp .top

align 64
.top:
%rep LAT_BODY_COUNT
    lat_body {%2},{%3}
ALIGN 64
%endrep
.end:
    pop rax
    ret

%assign GAP (.end - .top)
%assign GAPPER (GAP / LAT_BODY_COUNT)
%if GAP != LAT_BODY_SIZE * LAT_BODY_COUNT
%error wrong LAT BODY SIZE (LAT_BODY_SIZE) vs GAPPER
%endif
%undef GAP
%undef GAPPER

%endmacro

; See this tweet from Rivet Amber
; https://twitter.com/rivet_amber/status/1312141314629148673
;
; The idea is that IACA shows that load instructions take an extra 3 cycles (4 total)
; from allocation until when they can be dispatched.
;
; We test this by setting up a test that jumps around randomly using indirect branches
; between 8 identical blocks. This will mispredict with a rate of 87.5% (7/8) and we
; use the misprediction to ensure that allocation starts fresh every time, and we put
; two 'payload' instrucitons as the first two in each block, and include them in the dep
; chain leading to the next branch mispredict. In particular, they modify (but don't
; actually change the value) of rdx which is contains the state of the rng and is used
; to calculate the next jump address.
;
; We have two variants of this test with differnet payload: one with a load+add and a nop,
; and one with two lea instructions. Nominally, both add 6 cycles to the latency chain (5
; cycles for the load + 1 for the add, and 3 + 3 for the two LEAs), so if the measured
; performacne is the same, we conclude that load don't have this additional allocation time.
;
; On Skylake, I find that both run in the same time, indicating no additional allocation time.
define_load_alloc load,{add rdx, [rsp + r10]},{nop5}
define_load_alloc lea,{lea rdx, [rdx + r10 - 1]},{lea rdx, [rdx + r10 + 1]}
