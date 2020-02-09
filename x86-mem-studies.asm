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

    mov eax, 1
    vpcmpeqd xmm0, xmm0, xmm0
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
