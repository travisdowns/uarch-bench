%include "x86-helpers.asm"

nasm_util_assert_boilerplate
thunk_boilerplate

; AVX-512 specific x86 tests

define_bench kreg_lat
xor eax, eax
kxorb k0, k0, k0
.top:
%rep 128
kmovb k0, eax
kmovb eax, k0
%endrep
dec rdi
jnz .top
ret

; these next two tests test whether kxorb same, same, same
; is dependency breaking. The only differences between the
; nz and z variants is that the former uses different
; registers for xor input (not zeroing), while z uses the
; same registers, so is in principle a zeroing idiom
define_bench kreg_lat_nz
xor eax, eax
kxorb k0, k0, k0
kxorb k1, k1, k1
.top:
%rep 128
kmovb k0, eax
kxorb k0, k0, k1
kmovb eax, k0
%endrep
dec rdi
jnz .top
ret

; variant with zeroing kxor
define_bench kreg_lat_z
xor eax, eax
kxorb k0, k0, k0
kxorb k1, k1, k1
.top:
%rep 128
kmovb k0, eax
kxorb k0, k0, k0
kmovb eax, k0
%endrep
dec rdi
jnz .top
ret

; variant with dep-breaking move from GP
define_bench kreg_lat_mov
xor eax, eax
xor ecx, ecx
kxorb k0, k0, k0
kxorb k1, k1, k1
.top:
%rep 128
kmovb k0, eax
kmovb k0, ecx
kmovb eax, k0
%endrep
dec rdi
jnz .top
ret
