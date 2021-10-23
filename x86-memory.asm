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
