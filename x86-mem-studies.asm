BITS 64
default rel

%include "x86-helpers.asm"

nasm_util_assert_boilerplate
thunk_boilerplate

; classic pointer chasing benchmark
define_bench replay_crossing
    push_callee_saved
    mov  rbp, rsp

    and rsp, -64
    sub rsp,  66 ; rsp points 2 bytes before a 64b boundary

    mov [rsp], rsp ; set up the pointer loop

    mov rax, rsp
    mov rbx, rsp
    mov rcx, rsp
    xor r15, r15
    mov  r8, 1

mov rcx, rsp
align 64
.top:
%rep 100
    mov rax, [rax + r15]

    add rax, 0
    add rax, 0
    add rax, 0

    lfence
%endrep
    dec rdi
    jnz .top

    mov rsp, rbp
    pop_callee_saved
    ret

%macro fw_define_start 1
define_bench %1
    push rbp
    mov  rbp, rsp
    and rsp, -64
    sub rsp, 64

    lea rcx, [rsp + 10]
    sub rcx, 10
    mov eax, 1
    vpcmpeqd xmm0, xmm0, xmm0
    jmp .top
align 32
.top:
%endmacro

%macro fw_define_end 0
    dec rdi
    jnz .top

    mov rsp, rbp
    pop rbp
    ret
%endmacro

fw_define_start fw_write_read
%rep 100
    mov [rsp], rax
    mov rax, [rsp]
%endrep
fw_define_end

fw_define_start fw_write_read_rcx
%rep 100
    mov [rcx], rax
    mov rax, [rcx]
%endrep
fw_define_end

fw_define_start fw_write_read_rcx4
%rep 100
    mov [rcx], rax
    times 4 mov rax, [rcx]
%endrep
fw_define_end

fw_define_start fw_write_readx
%rep 100
    vmovdqa [rsp], xmm0
    vmovdqa xmm0, [rsp]
%endrep
fw_define_end

fw_define_start fw_split_write_read
%rep 100
    mov [rsp], rax
    mov [rsp + 8], rax
    vmovdqa xmm0, [rsp]
%endrep
fw_define_end

fw_define_start fw_split_write_read_chained
%rep 100
    mov [rsp], rax
    mov [rsp + 8], rax
    vmovdqa xmm0, [rsp]
    vmovq rax, xmm0
%endrep
fw_define_end


fw_define_start fw_write_split_read
%rep 100
    vmovdqa [rsp], xmm0
    mov     rax, [rsp]
    vmovq    xmm0, rax
%endrep
fw_define_end

; same as fw_write_split_read, but reads from both halves of the 16b written value
; this mostly just adds 1 cycle to the time without revealing any new uarch details
; on current uarch, but maybe it will false in the future
fw_define_start fw_write_split_read_both
%rep 100
    vmovdqa [rsp], xmm0
    mov     rax, [rsp]
    add     rax, [rsp + 8]
    vmovq    xmm0, rax
%endrep
fw_define_end


; write a null NT line then one extra
;
; %1 size of store in bytes
; %2 full instruction to use
; %3 0 or 1
;    0: normal full line write
;    1: extra write per line
%macro define_nt_extra 3
%assign BITSIZE (%1 * 8)
%if %3 == 0
define_bench nt_normal_ %+ BITSIZE
%else
define_bench nt_extra_ %+ BITSIZE
%endif

mov     rdx, [rsi + region.size]
mov     rsi, [rsi + region.start]

xor         eax, eax
mov         r8, -1
vpcmpeqd    ymm0, ymm0, ymm0

.top:
mov     rax, rdx
mov     rcx, rsi

.inner:
%assign offset 0
%rep (64 / %1)
%2
%assign offset (offset + %1)
%endrep

%if %3 != 0
%assign offset 0
%rep 5
%2
%assign offset ((offset + %1) % 64)
%endrep
%endif

%undef offset ; needed to prevent offset from being expanded wrongly in the macro invocations below
add     rcx, 64
sub     rax, 64
jge      .inner

dec rdi
jnz .top
ret
%endmacro

%macro define_nt_both 2
define_nt_extra %1, {%2}, 0
define_nt_extra %1, {%2}, 1
%endmacro

define_nt_both  4, {movnti   [rcx + offset],  r8d}
define_nt_both  8, {movnti   [rcx + offset],  r8 }
define_nt_both 16, {vmovntdq [rcx + offset], xmm0}
define_nt_both 32, {vmovntdq [rcx + offset], ymm0}
define_nt_both 64, {vmovntdq [rcx + offset], zmm0}
