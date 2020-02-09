BITS 64
default rel

%include "x86-helpers.asm"

nasm_util_assert_boilerplate
thunk_boilerplate

; depedent series of adds
define_bench rs_dep_add
    xor eax, eax
.top:
times 128 add rax, rax
    dec rdi
    jnz .top
    ret

; dependent series of muls, takes 3 per cycle
define_bench rs_dep_imul
    xor eax, eax
.top:
    times 128  imul rax, rax
    dec rdi
    jnz .top
    ret

; 4 chains of indepedent adds
define_bench rs_dep_add4
    xor eax, eax
    .top:
%rep 128
    add rax, rax
    add rcx, rcx
    add rdx, rdx
    add r8 , r8
%endrep
    dec rdi
    jnz .top
    ret

; split stores, should fill the SB since split stores take 2 cycles to commit
define_bench rs_split_stores
    push rbp
    mov  rbp, rsp

    and rsp,  -64 ; align to 64-byte boundary
    sub rsp,  128 ; we have a 128 byte byte buffer above rsp, aligned to 64 bytes

.top:
    times 128 mov  qword [rsp + 60], 0
    dec rdi
    jnz .top

    mov rsp, rbp
    pop rbp
    ret

define_bench rs_dep_fsqrt
    fldz
.top:
    times 128 fsqrt
    dec rdi
    jnz .top

    fstp st0
    ret


; the following tests interleave fsqrt (latency 14) with varying numbers of nops
; in a 1:N ratio (N the number of nops per fqrt instruction.
; The idea is with a low nop:fsqrt ratio, the RS will be the limit, since the RS
; will fill with nops before exhausting another resource. With more nops, the ROB
; will be exhausted. Generally, if the RS has size R and the ROB size O, we expect
; the crossover point to be when N == O / R - 1;
;
; %1 number of filler ops
; %2 long latency op
; %3 name (suffix)
; %4 filler op asm
%macro define_rs_op_op 4-*

%xdefine ratio %1

define_bench rs_fsqrt_%3%1
    push rbp
    mov  rbp, rsp

    and rsp,  -64 ; align to 64-byte boundary
    sub rsp,  128 ; we have a 128 byte byte buffer above rsp, aligned to 64 bytes

    fldz
    vzeroupper
.top:
%rep 32
    %2
%rep ratio
%rep (%0 - 3)
    %4
%rotate 1
%endrep
%rotate 3
%endrep
%endrep
    dec rdi
    jnz .top

    fstp st0
    mov  rsp, rbp
    pop  rbp
    vzeroupper
    ret
%endmacro


%assign i 0
%rep 20
define_rs_op_op i, fsqrt, nop     , nop
define_rs_op_op i, fsqrt, add     , {add eax, 1}
define_rs_op_op i, fsqrt, xorzero , {xor eax, eax}
define_rs_op_op i, fsqrt, load    , {mov eax, [rsp]}
define_rs_op_op i, fsqrt, store   , {mov [rsp], eax}
define_rs_op_op i, fsqrt, paddb   , {paddb xmm0, xmm1}
define_rs_op_op i, fsqrt, vpaddb  , {vpaddb xmm0, xmm1, xmm2}
define_rs_op_op i, fsqrt, add_padd, {vpaddb xmm0, xmm1, xmm2}, {add eax, 1} ; mixed scalar and vector adds
define_rs_op_op i, fsqrt, load_dep, {mov eax, [rsp]}, {add eax, 0}, {add eax, 0}, {add eax, 0}
%assign i (i + 1)
%endrep


%macro define_rs_load_op 3-*

%xdefine ratio %1

; like the fsqrt bench, but with 5-cycle loads as the limiting instruction
define_bench rs_load_%2%1
    push rbp
    mov  rbp, rsp

    and rsp,  -64 ; align to 64-byte boundary
    sub rsp,  128 ; we have a 128 byte byte buffer above rsp, aligned to 64 bytes

    xor eax, eax
    xor ecx, ecx
    xor edx, edx
    mov QWORD [rsp], 0

    vzeroupper
.top:
%rep 32
    mov rax, [rax + rsp]
%rep ratio
%rep (%0 - 2)
    %3
%rotate 1
%endrep
%rotate 2
%endrep
%endrep
    dec rdi
    jnz .top

    mov  rsp, rbp
    pop  rbp
    vzeroupper
    ret
%endmacro

%assign i 0
%rep 20
define_rs_load_op i, nop     , nop
define_rs_load_op i, add     , {add edx, eax}, {add r8d, eax}, {add r9d, eax}
%assign i (i + 1)
%endrep


; here we test how many dependent loads can enter the RS at once
;
%macro define_rs_loadchain 2

%xdefine ratio %1

; like the fsqrt bench, but with 5-cycle loads as the limiting instruction
define_bench rs_loadchain%2
    push rbp
    mov  rbp, rsp

    and rsp,  -64 ; align to 64-byte boundary
    sub rsp,  128 ; we have a 128 byte byte buffer above rsp, aligned to 64 bytes

    mov QWORD [rsp], 0

    xor eax, eax
    xor ecx, ecx
    mov rdx, rsp



.top:
%rep 32

%rep %1
    ;imul rax, rax, 1
%endrep

%rep %2
    ;jc udd
    ;add eax, 0
    ;mov ecx, [rax + rdx]
    ;movsx  rcx, eax
    ;lea rcx, [rax + rdx]
    ;cdq
    ;imul rax, rax, 1
    popcnt rax, rax
    ;times 4 nop
%endrep

    lfence
    ;mov rax, 0

%endrep
    dec rdi
    jnz .top

    mov  rsp, rbp
    pop  rbp
    vzeroupper
    ret
%endmacro

%assign i 0
%rep 120
define_rs_loadchain 20,i
%assign i (i + 1)
%endrep


; test store buffer capacity
; %1 number of vsqrtss latency builders
; %2 number of dependent store instructions
%macro define_rs_storebuf 2

; the basic idea is that we issue a bunch of dependent vsqrtss
; then N indepedent stores
; then a dependency breaking op vxorps so that the next sqrt chian
; is indepedent
; when N is equal to size of the store buffer + 1, allocation will
; stall and the sqrt chains won't be able to run in parallel and there
; will be a big jump (~43 cycles on SKL) in the iteration time

define_bench rs_storebuf%2
    sub rsp,  8

    vxorps xmm0, xmm0, xmm0

.top:
%rep 32

%rep %1
    vsqrtss xmm0, xmm0, xmm0
%endrep
    movq rax, xmm0
%rep %2
    mov DWORD [rsp], 0
%endrep

    vxorps xmm0, xmm0, xmm0

%endrep
    dec rdi
    jnz .top

    add rsp, 8
    ret
%endmacro

%assign i 0
%rep 80
define_rs_storebuf 10,i
%assign i (i + 1)
%endrep

mov  ebx, [rsp]
mov  [rsp - 0x8], ebx
