%include "x86-helpers.asm"

nasm_util_assert_boilerplate
thunk_boilerplate


; %1 name suffix
; %2 nop count
%macro define_decode_complex 2
define_bench decode_complex_%1
.top:
%rep 200
    cwd
    xor eax, eax
    times %2 nop
%endrep
    dec rdi
    jnz .top

    ret
%endmacro

define_decode_complex 211,1
define_decode_complex 2111,2
define_decode_complex 21111,3