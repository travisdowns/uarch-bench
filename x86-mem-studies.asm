BITS 64
default rel

%include "x86_helpers.asm"

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
