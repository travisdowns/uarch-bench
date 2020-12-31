%include "x86-helpers.asm"

nasm_util_assert_boilerplate
thunk_boilerplate


; %1 name suffix
; %2 nop count
; %3 payload instruction
%macro define_decode_complex 3
define_bench decode_complex_%1
.top:
%rep 200
    %3
    xor eax, eax
    times %2 nop
%endrep
    dec rdi
    jnz .top

    ret
%endmacro

define_decode_complex 211,1,cwd
define_decode_complex 2111,2,cwd
define_decode_complex 21111,3,cwd

define_decode_complex 31,0,{xadd eax, eax}
define_decode_complex 311,1,{xadd eax, eax}
define_decode_complex 3111,2,{xadd eax, eax}