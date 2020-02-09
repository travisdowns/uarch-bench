; monoid tests go here because they are slow to compile, so we don't want them in the main x86_methods.asm
; file which gets compiled often

%include "x86-helpers.asm"

nasm_util_assert_boilerplate
thunk_boilerplate

%define nopd(x) nop %+ x

; given a list of byte counts, generate a series of nops
; one for each count with that number of bytes
%macro multinop 1-*
%rep %0
    nopd(%1)
    %rotate 1
%endrep
%endmacro

%define DECODE_OPS 50400/2

%macro define_decode 2-*
%define ICOUNT (%0-1)
%if (DECODE_OPS % ICOUNT) != 0
%error "count of body instructions should divide DECODE_OPS" REPS
%endif
define_bench decode%1
ALIGN 64
times 0 nop
.top:
%rep (DECODE_OPS / ICOUNT)
multinop %{2:-1}
%endrep
dec rdi
jnz .top
ret
%endmacro

define_decode 33334,3,3,3,3,4
define_decode 33333,3,3,3,3,3
define_decode 16x1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
define_decode 8x2,2,2,2,2,2,2,2,2
define_decode 4x4,4,4,4,4
define_decode 5551,5,5,5,1
define_decode 664,6,6,4
define_decode 88,8,8
define_decode 871,8,7,1
define_decode 8833334,8,8,3,3,3,3,4
define_decode 884444,8,8,4,4,4,4

; https://twitter.com/_monoid/status/1106976673646415872
define_bench decode_monoid
.top:
%rep 200
%rep 8
    multinop 3,3,3,3,4
%endrep
%rep 8
    multinop 5,5,6
%endrep
%endrep
    dec rdi
    jnz .top
    ret

; https://twitter.com/_monoid/status/1107760616650035205
define_bench decode_monoid2
.top:
%rep 1000
    db 0x90
    db 0x90
    db 0x90
    db 0x66, 0x66, 0x66, 0x66, 0x66, 0x90
    db 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x90

    db 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x90
    db 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x90
    db 0x90
    db 0x90
%endrep
    dec rdi
    jnz .top
    ret

define_bench decode_monoid3
.top:
%rep 1000
    nop
    nop
    nop
    nop6
    nop7

    nop7
    nop7
    nop
    nop
%endrep
    dec rdi
    jnz .top
    ret
