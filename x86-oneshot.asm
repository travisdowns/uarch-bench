%include "x86-helpers.asm"

nasm_util_assert_boilerplate
thunk_boilerplate

define_bench raw_rdpmc0_overhead
assert_eq rdi, 0
assert_eq rsi, 0
mov     r8, rdx
readpmc0_start
readpmc0_end
store_pfcnow0 r8
ret

define_bench raw_rdpmc4_overhead
assert_eq rdi, 0
assert_eq rsi, 0
mov     r8, rdx
readpmc4_start
readpmc4_end
store_pfcnow4 r8
ret


; 100 stores with raw rdpmc calls
define_bench store_raw_libpfc
; loop count should be 1 since we don't have a loop here
assert_eq rdi, 1
sub     rsp, 128
mov     r8, rdx
readpmc4_start
.top:
%assign offset 0
%rep 100
mov     [rsp], BYTE 42
;sfence
%assign offset offset+1
%endrep
readpmc4_end
store_pfcnow4 r8
add     rsp, 128
ret


%macro define_oneshot_adds 1
define_bench oneshot_%1adds
    push    rbx

    mov     r8, rdx
    xor     eax, eax
    cpuid
    xor     r9d, r9d

    readpmc0_start_nofence
    lfence
    times (%1) add r9, 1
    lfence
    readpmc0_end_nofence

    store_pfcnow0 r8

    pop     rbx
    ret
%endmacro

define_oneshot_adds 6
define_oneshot_adds 11

define_bench oneshot_loadadds
    push    rbx

    mov     r8, rdx
    xor     eax, eax
    cpuid
    xor     r9d, r9d

    readpmc0_start_nofence
    lfence
    mov     r9, [rsp]
    times 6 add r9, 1
    lfence
    readpmc0_end_nofence

    store_pfcnow0 r8

    pop     rbx
    ret