/* ARM EABI helpers adapted from nSpire, Copyright (c) 2026 Max Alex.
 * Distributed under the MIT license in third_party/licenses/nSpire-MIT.txt.
 * Ndless syscall numbers verified against official syscall-list.h r2022. */
.syntax unified
.arm
.section .text.entry,"ax",%progbits
.global _start
.type _start,%function
_start:
    push {r4-r12,lr}
    bl main
    pop {r4-r12,pc}

.section .text,"ax",%progbits
/* Ndless r2022 stable SWI ABI. The ABI may clobber r4 and lr. */
.macro nd_call name,number
.global \name
.type \name,%function
\name:
    push {r4,lr}
    swi \number
    pop {r4,pc}
.endm
nd_call nd_open,0
nd_call nd_read,1
nd_call nd_write,2
nd_call nd_close,3
/* Stable Ndless SDK syscall-list.h: nuc_fseek=33, nuc_ftell=89. */
nd_call nd_seek,33
nd_call nd_tell,89
/* Nucleus directory calls from the stable Ndless syscall list. */
nd_call nd_mkdir,15
nd_call nd_opendir,95
nd_call nd_closedir,97
nd_call nd_stat,18
nd_call nd_unlink,19
nd_call nd_rename,20
nd_call nd_touch_read,75
nd_call nd_touch_write,76
/* Official SDK: has-syscall extension, calc_cmd address, formatter, OS free. */
nd_call nd_has_syscall,0x20000C
nd_call nd_calc_address,0x800153
nd_call nd_math_to_string,342
nd_call nd_os_free,6
/* Existing OS zlib ABI; used only for the tiny current language preference. */
nd_call nd_inflate_init,85
nd_call nd_inflate,86
nd_call nd_inflate_end,87
nd_call nd_hwtype,0x200002
nd_call nd_revision,0x200006
nd_call nd_blitter,0x20000D
nd_call nd_lcd_init,0x20000F

/* ARM EABI quotient/remainder convention: quotient r0, remainder r1. */
.global __aeabi_uidiv
.global __aeabi_uidivmod
.type __aeabi_uidiv,%function
.type __aeabi_uidivmod,%function
__aeabi_uidiv:
__aeabi_uidivmod:
    cmp r1,#0
    moveq r0,#0
    bxeq lr
    mov r2,#0
    mov r3,#0
    mov ip,#32
1:
    movs r0,r0,lsl #1
    adcs r3,r3,r3
    cmpcc r3,r1
    subcs r3,r3,r1
    adc r2,r2,r2
    subs ip,ip,#1
    bne 1b
    mov r0,r2
    mov r1,r3
    bx lr
.global __aeabi_idiv
.global __aeabi_idivmod
__aeabi_idiv:
__aeabi_idivmod:
    push {r4-r6,lr}
    mov r4,r0
    eor r5,r0,r1
    cmp r0,#0
    rsbmi r0,r0,#0
    cmp r1,#0
    rsbmi r1,r1,#0
    bl __aeabi_uidivmod
    cmp r5,#0
    rsbmi r0,r0,#0
    cmp r4,#0
    rsbmi r1,r1,#0
    pop {r4-r6,pc}

.section .text.__aeabi_uldivmod,"ax",%progbits
.global __aeabi_uldivmod
.type __aeabi_uldivmod,%function
/* RTABI32 5.2: numerator r0:r1, denominator r2:r3, quotient r0:r1,
 * remainder r2:r3 (low word first). No runtime or hardware division needed.
 * Keep the 65th remainder bit: large unsigned divisors can overflow a shift.
 * As with uidivmod above, a zero divisor returns zero; retain the numerator
 * as remainder. Presentation callers always supply a nonzero duration. */
__aeabi_uldivmod:
    orrs ip,r2,r3
    moveq r2,r0
    moveq r3,r1
    moveq r0,#0
    moveq r1,#0
    bxeq lr
    push {r4-r8,lr}
    mov r4,#0
    mov r5,#0
    mov r6,#0
    mov r7,#0
    mov r8,#64
1:
    adds r0,r0,r0
    adcs r1,r1,r1
    adcs r4,r4,r4
    adcs r5,r5,r5
    bcs 2f
    cmp r5,r3
    cmpeq r4,r2
    blo 3f
2:
    subs r4,r4,r2
    sbc r5,r5,r3
    adds r6,r6,r6
    adc r7,r7,r7
    orr r6,r6,#1
    b 4f
3:
    adds r6,r6,r6
    adc r7,r7,r7
4:
    subs r8,r8,#1
    bne 1b
    mov r0,r6
    mov r1,r7
    mov r2,r4
    mov r3,r5
    pop {r4-r8,pc}
.size __aeabi_uldivmod,.-__aeabi_uldivmod
.section .text.__aeabi_ldivmod,"ax",%progbits
.global __aeabi_ldivmod
.type __aeabi_ldivmod,%function
/* Signed RTABI32 uses the same four-word quotient/remainder result. Convert
 * two's-complement magnitudes without a signed C overflow (INT64_MIN is a
 * valid unsigned magnitude), reuse the tested divider, then restore signs.
 * Remainder has the numerator's sign. Keep the stack eight-byte aligned.
 * Zero divisors preserve the unsigned helper's quotient-zero fallback. */
__aeabi_ldivmod:
    push {r4-r6,lr}
    mov r4,r1
    eor r5,r1,r3
    cmp r1,#0
    bpl 1f
    rsbs r0,r0,#0
    rsc r1,r1,#0
1:
    cmp r3,#0
    bpl 2f
    rsbs r2,r2,#0
    rsc r3,r3,#0
2:
    bl __aeabi_uldivmod
    cmp r5,#0
    bpl 3f
    rsbs r0,r0,#0
    rsc r1,r1,#0
3:
    cmp r4,#0
    bpl 4f
    rsbs r2,r2,#0
    rsc r3,r3,#0
4:
    pop {r4-r6,pc}
.size __aeabi_ldivmod,.-__aeabi_ldivmod
.section .text,"ax",%progbits
.global __aeabi_memcpy
.global __aeabi_memcpy4
.global __aeabi_memcpy8
__aeabi_memcpy:
__aeabi_memcpy4:
__aeabi_memcpy8:
    b memcpy
.global __aeabi_memmove
__aeabi_memmove:
    b memmove
.global __aeabi_memset
.global __aeabi_memset4
.global __aeabi_memset8
__aeabi_memset:
__aeabi_memset4:
__aeabi_memset8:
    mov r3,r1
    mov r1,r2
    mov r2,r3
    b memset
.global __aeabi_memclr
.global __aeabi_memclr4
.global __aeabi_memclr8
__aeabi_memclr:
__aeabi_memclr4:
__aeabi_memclr8:
    mov r2,r1
    mov r1,#0
    b memset
.section .note.GNU-stack,"",%progbits
