%include "x86_helpers.asm"

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
%macro define_rs_fsqrt_op 3-*

%xdefine ratio %1

define_bench rs_fsqrt_%2%1
    push rbp
    mov  rbp, rsp

    and rsp,  -64 ; align to 64-byte boundary
    sub rsp,  128 ; we have a 128 byte byte buffer above rsp, aligned to 64 bytes

    fldz
    vzeroupper
.top:
%rep 32
    fsqrt
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

    fstp st0
    mov  rsp, rbp
    pop  rbp
    vzeroupper
    ret
%endmacro


%assign i 0
%rep 20
define_rs_fsqrt_op i, nop     , nop
define_rs_fsqrt_op i, add     , {add eax, 1}
define_rs_fsqrt_op i, xorzero , {xor eax, eax}
define_rs_fsqrt_op i, load    , {mov eax, [rsp]}
define_rs_fsqrt_op i, store   , {mov [rsp], eax}
define_rs_fsqrt_op i, paddb   , {paddb xmm0, xmm1}
define_rs_fsqrt_op i, vpaddb  , {vpaddb xmm0, xmm1, xmm2}
define_rs_fsqrt_op i, add_padd, {vpaddb xmm0, xmm1, xmm2}, {add eax, 1} ; mixed scalar and vector adds

%assign i (i + 1)
%endrep
