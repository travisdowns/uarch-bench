%ifndef X86_HELPERS_GUARD
%define X86_HELPERS_GUARD

BITS 64
default rel

%ifdef __YASM_MAJOR__
%error "We don't support YASM compilation anymore, see issue #63"
%endif

%include "nasm-utils/nasm-utils-inc.asm"

; aligns and declares the global label for the bench with the given name
; also potentally checks the ABI compliance (if enabled)
%macro define_bench 1
%define current_bench %1
ALIGN 64
abi_checked_function %1
%endmacro


%define PFCNOW_INST   0    ; INST_RETIRED.ANY
%define PFCNOW_CLK    8    ; CPU_CLK_UNHALTED.THREAD (aka clock cycles)
%define PFCNOW_INST  16    ; CPU_CLK_UNHALTED.REF_TSC
%define PFCNOW_UBASE 24    ; first user-programmable counter


; do a single 64-bit readpmc with the given value leaving the result in rax
; clobbers rax, rcx, rdx
%macro rdpmc64 1
mov     rcx, %1
rdpmc
shl     rdx, 32
or      rax, rdx
%endmacro

; The timings reported by pairs of rdpmc instructions are quite sensitive to the exact position
; that lfence appears. Two back to back lfences execute more quickly than expected compared to
; two lfences separated by a single 1-cycle insruction; adding the 1 cycle instruction increases
; the measured time by about 4 cycles. The next few instrutions also generally measure "too high"
; with this approach. So it is best to compare not against a totally empty function, but against
; one with a few instructions, with the method-under-test also having those few instructions.
;
; In the absence of a properly designed "delta" test, we hack around this issue by ensuring that even
; the empty test has at least one instruction in the "body" between the two innermost lfence instructions.
; You can see that below with the lfence being before the final movq.
%macro readpmc0_start_nofence 0
rdpmc64  0x40000001
lfence ; not last, see above
movq    xmm0, rax
%endmacro

%macro readpmc0_end_nofence 0
rdpmc64  0x40000001
movq    rdx, xmm0
sub     rax, rdx
%endmacro

; the readpmcN functions read the PFCNOW_CLK (cycles) couunter, as well
; as N user-programmable counters
%macro readpmc0_start 0
mfence
lfence
readpmc0_start_nofence
%endmacro

%macro readpmc0_end 0
lfence
readpmc0_end_nofence
lfence
%endmacro

%macro readpmc4_start 0
mfence
lfence
readpmc0_start_nofence
; read the 4 user programmable counters and pack them in xmm1 and xmm2
rdpmc64   0
pinsrq  xmm1, rax, 0
rdpmc64   1
pinsrq  xmm1, rax, 1
rdpmc64   2
pinsrq  xmm2, rax, 0
rdpmc64   3
lfence  ; see the explanation above for why this isn't last
pinsrq  xmm2, rax, 1
%endmacro

; results are in xmm3 and xmm4
%macro readpmc4_end 0
lfence
readpmc0_end_nofence
movq     xmm0, rax
; read the 4 user programmable counters and pack them in xmm3 and xmm4
rdpmc64   0
pinsrq   xmm3, rax, 0
rdpmc64   1
pinsrq   xmm3, rax, 1
rdpmc64   2
pinsrq   xmm4, rax, 0
rdpmc64   3
pinsrq   xmm4, rax, 1

psubq    xmm3, xmm1
psubq    xmm4, xmm2
%endmacro

; if called after readpmc0_end, stores the results into the
; LibPfcNow object pointed to by the argument
%macro store_pfcnow0 1
mov  [%1 + PFCNOW_CLK], rax
%endmacro

; if called after readpmc4_end, stores the results into the
; LibPfcNow object pointed to by the argument
%macro store_pfcnow4 1
pextrq  [%1 + PFCNOW_CLK       ], xmm0, 0
pextrq  [%1 + PFCNOW_UBASE +  0], xmm3, 0
pextrq  [%1 + PFCNOW_UBASE +  8], xmm3, 1
pextrq  [%1 + PFCNOW_UBASE + 16], xmm4, 0
pextrq  [%1 + PFCNOW_UBASE + 24], xmm4, 1
%endmacro


; mirror of mem-benches.cpp::region
struc region
    .size  : resq 1
    .start : resq 1
endstruc

%endif ; X86_HELPERS_GUARD
