%include "x86-helpers.asm"

nasm_util_assert_boilerplate
thunk_boilerplate


; %1 suffix
; %2 chain instruction between load and store
%macro define_tlb_fencing 2
define_bench tlb_fencing_%1

    xor     eax, eax
    mov     r9 , [rsi + region.start]

    mov     r8 , [rsi + region.size]  
    sub     r8 , 64                   ; pointer to end of region (plus a bit of buffer)

    mov     r10, [rsi + region.size]
    sub     r10, 1 ; mask

    mov     rsi, r9   ; region start

.top:
    mov     rcx, rax
    and     rcx, r10
    add     rcx, r9
    mov     rdx, [rcx]
    %2
    mov     DWORD [rsi + rdx + 160], 0
    add     rax, (64 * 67)  ; advance a prime number of cache lines slightly larger than a page

    ;lfence   ; adding an lfence simulates the fencing that happens naturally
    dec     rdi
    jnz      .top

    ret
%endmacro

define_tlb_fencing dep, {xor rcx, rcx}     ; rcx is an unused dummy here
define_tlb_fencing indep, {xor rdx, rdx}   ; rdx breaks the load -> store address dep

; how many times the pattern is repeated (helps defray loop overhead)
%define LOAD_PATTERN_REPEAT 2

; load patterns within 1 or 2 cache lines
; tests for things like load merging
%macro define_load_pattern 0-*

%define LP_NAME load_pattern
%rep %0
%define LP_NAME %[LP_NAME]_%1
%rotate 1
%endrep

define_bench LP_NAME
    push rbp
    mov  rbp, rsp

    sub rsp, 1024
    and rsp, -1024

    mov rsi, rsp
    jmp .top

ALIGN 64
.top:
%rep LOAD_PATTERN_REPEAT
%rep %0
    mov rax, [rsi + %1]
%rotate 1
%endrep
%endrep
    sub rdi, (%0 * LOAD_PATTERN_REPEAT)
    jg .top

    mov rsp, rbp
    pop rbp
    ret
    

%endmacro

define_load_pattern 0,0,0,0
define_load_pattern 0,4,0,4
define_load_pattern 0,8,0,8
define_load_pattern 0,12,0,12
define_load_pattern 0,16,0,16
define_load_pattern 0,20,0,20
define_load_pattern 0,24,0,24
define_load_pattern 0,0,8,8
define_load_pattern 0,0,0,8
define_load_pattern 4,8,4,8
define_load_pattern 0,4,8,12
define_load_pattern 1,5,9,13
define_load_pattern 12,8,4,0
define_load_pattern 0,32,0,32
define_load_pattern 0,64,0,64
define_load_pattern 0,96,0,96
define_load_pattern 0,72,0,72
define_load_pattern 0,4,8
define_load_pattern 0,8,80

define_bench load_pattern_reg_0_4_8_12
    push rbp
    mov  rbp, rsp

    sub rsp, 1024
    and rsp, -1024

    mov rsi, rsp

    mov ecx,  0
    mov edx,  4
    mov r8d,  8
    mov r9d, 12

ALIGN 64
.top:

%rep LOAD_PATTERN_REPEAT
    mov rax, [rsi + rcx]
    mov rax, [rsi + rdx]
    mov rax, [rsi + r8]
    mov rax, [rsi + r9]
%endrep

    sub rdi, 4 * LOAD_PATTERN_REPEAT
    jg .top

    mov rsp, rbp
    pop rbp
    ret
