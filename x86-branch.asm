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
; fastest timing shows).
; The "second" determination is based on the IP, not frequency: even if the
; second branch is jumped to 2x as often, the first branch retains the fast
; behavior.
; an lfence before the benchmark but outside the loop seems to reduce
; the variance
;
;
; indirect branches
;
; indirect seem to have a best-case throughput of 1 per 2 cycles,
; regardless of spacing

; fill the given number of bytes with nop3, using
; plain nop for the 0-2 odd bytes at the end
%macro fillnop3 1-2 2
times ((%1 - %2) / 3) nop3
times ((%1 - %2) % 3) nop
%endmacro

%macro jmp_forward 1
%assign jump_bytes %1
%if %1 < 2
%error jump should be at least two bytes
%assign jump_bytes 2
%endif
    jmp %%target
    fillnop3 jump_bytes
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

define_jmp_forward   8,   8
define_jmp_forward  16,  16
define_jmp_forward  32,  32
define_jmp_forward  64,  64
define_jmp_forward  96,  96
define_jmp_forward 128, 128

; This test consists of a series of forward jumps, two per "block", organized
; into two chains: a first chain whose jmp is always first in the block and
; targets the first jmp in the second block, and a second chain that is similar
; except using the second jump.
;
; We go though three chains every iteration, either 2 first and 1 second, or
; 2 second and 1 first chains. This test shows that the first jump in a block
; is resolved faster (1 cycle) by the BTB, since the variant with 2:1
; first:second jumps is faster by 1 cycle per jmp tham the 1:2 variant.
;
; %1 total gap (between first/second jump in one block and first/second
; %2 first gap (between first and second jump targets)
; jump in the next)
; %3: 0 means the second jump occurs in a 2:1 ratio with the first
;     1 means the second jump occurs in a 1:2 ratio with the first
; %4 offset of the first jump after the start of the block
%macro define_jmp_forward_dual 4
define_bench jmp_forward_dual_%1_%2_%3
    xor eax, eax
    jmp .first_1

align 64

%assign reps 10
%assign fill1 %2
%assign fill2 (%1 - %2 - %4)

%assign i 1
%rep (reps-1)
    %assign next (i+1)
    fillnop3 %4, 0
.first_%+i:
    jmp  .first_%+next
    fillnop3 fill1
.second_%+i:
    jmp .second_%+next
    fillnop3 fill2
%assign i (i+1)
%endrep

%if %3
%define target1 first
%define target2 second
%else
%define target1 second
%define target2 first
%endif

; the final iteration of the first pass jumps to
; the first target of the second pass, unconditionally
. %+ target1 %+ _ %+ reps:
    jmp     . %+ target2 %+ _1

; while the second iteration alternates between jumping again
; to the second pass and the first pass (in the latter case
; also checking for termination). So the first:second passes are
; executed in a 1:2 ratio
. %+ target2 %+ _ %+ reps:
    xor     eax, 1
    jnz      . %+ target2 %+ _1 
    dec rdi
    jnz . %+ target1 %+ _1
    ret
%endmacro

define_jmp_forward_dual 32, 16, 0, 3
define_jmp_forward_dual 32,  6, 0, 3
define_jmp_forward_dual 64, 16, 0, 3
define_jmp_forward_dual 64, 48, 0, 3
define_jmp_forward_dual 32, 16, 1, 3
define_jmp_forward_dual 32,  6, 1, 3
define_jmp_forward_dual 64, 16, 1, 3
define_jmp_forward_dual 64, 48, 1, 3

struc jmp_loop_args
    .total  : resd 1
    .first  : resd 1
endstruc

; this is similar to the jmp_forward_dual test
; except that we execute the forward jump region
; 100x times in a loop, split in a configurable way
; between the first and second regions
;
; %1 total gap (between first/second jump in one block and first/second
; %2 first gap (between first and second jump targets)
%macro define_jmp_loop_generic 2
define_bench jmp_loop_generic_%1_%2
%define block_size  %1
%define gap         %2

    mov edx, [rsi + jmp_loop_args.total]
    mov ecx, [rsi + jmp_loop_args.first]

%define total_iters edx
%define first_iters ecx

    xor eax, eax
    jmp .bottom
    ud2

align 64

%assign reps 10
%assign fill1 gap
%assign fill2 (block_size - gap)

%assign i 1
%rep (reps-1)
    %assign next (i+1)
.first_%+i:
    jmp  .first_%+next
    fillnop3 fill1
.second_%+i:
    jmp .second_%+next
    fillnop3 fill2
%assign i (i+1)
%endrep

.first_%+i:
    nop
    nop
.second_%+i:

.bottom:
    inc eax
    cmp eax, first_iters
    jle .first_1
    cmp eax, total_iters
    jle .second_1


    xor eax, eax
    dec rdi
    jnz .bottom

.done:
    ret
%endmacro

define_jmp_loop_generic 32, 16

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
