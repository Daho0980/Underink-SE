.text
    .align 2
    increase_value: .float 0.01

    .global _updateAudioDataAmplitude
    .p2align 4

/* [ -- CALLING CONVENTION -- ]
:: scalar ::
- x0~x7 : caller-saved
- x8 : XR; Indirect Result Reg. when returning a large strct, the adrs is stored here. since it has no use for it now, it can be used as a simple volatile reg.
- x9~x15 : caller-saved
- x16, x17 : Intra Procedure call scratch registers // volatile
             so x8~x17 is usable registers. y'can use it.

- x18 : this register is LIKELY ALREADY IN USE ON THE OS side // so NO
- x19~x28 : callee-saved. if used, it must be stored on the stack, which ma result in ^cycle loss^.
...
- x29 : stack frame pointer
- x30 : jumpback path. the return adr is stored after the op is completed. it can be overwritter, so it must be stored on the stack when calling other fns.
- x31 : sp or zr

:: vector ::
- v0~v7 : caller-saved
- v8~v15 : only the lower 64 bits(d8~d15) are non-volatile.
- v16~v31 : caller-saved
 */

/* [ -- INPUT REGISTERS -- ]
:: scalar ::
- x0 : origin pointer (*)
- x1 : size -> x12
- w2 : sample rate
- w3 : channels
- w4 : bit depth -> byte
- x5 : reflected volume pointer (*)
- x6 : easing function pointer (*)
       float : (float, float, float)
       REFER : the easing function can destroy all registers except v8~v31.
               except for input.

:: vector ::
s0 : target volume -> s2
 */

/* [ -- EXTRA REGISTERS -- ]
::scalar::                              total(excluding calcReg) : 10 regs
    - dynamic -
- x7  : read' bytes in a loop
- x8  : index
- w9  : scalar trigger
- w10 : volume ramping trigger
- w11 : channel counting
- x13~x17: calcReg
    - static -
- x19 : vector label address
- x20 : scalar label address
- w21 : spectrum max
- w22 : spectrum min
- w23 : ramp direction
        0 : down
        1 : up

::vector::                              total(excluding calcReg) : 6 regs
    - dynamic -
- s0 : easing t&easing result out
- s1 : reflected volume & easing start
- s2 : target volume    & easing end
- s3~s7 : easing function zone(only in volume ramping) & calcReg
- s16 : progress (0.0 ~ 1.0)
- s17 : increase value(for progress)
- s18 : current volume(to apply amplitude)
- v19 : spectrum max
- v20 : spectrum min
- v21~31 : calcReg
 */
 _updateAudioDataAmplitude:
    stp         x19, x20, [sp, #-16]!
    stp         x21, x22, [sp, #-16]!
    stp         x23, xzr, [sp, #-16]!       // xzr padding
    stp         x29, x30, [sp, #-16]!

    mov         x12, x1
    fmov        s2, s0                      // move target volume
    ldr         s1, [x5]                    // load reflected to reflected_static reg

    lsr         w4, w4, #3                  // 3 bit right shift -> bitDepth/8

    mov         x7, #0                      // reset read' bytes
    mov         w11, #1                     // reset channel counting
    fmov        s16, #0.0                   // reset progress
    ldr         s17, increase_value         // set increase value
    fmov        s18, s1                     // load reflected vol to curr vol

    fcmp        s2, s1                      // tgt - refl
    cset        w23, gt                     // set ramp direction
    cset        w9, ne                      // activate scalar trigger if tgt!=refl
    cset        w10, ne                     // activate ramping trigger if tgt!=refl

    cmp         w4, #1                      // 8
    b.eq        pre_8
    cmp         w4, #2                      // 16
    b.eq        pre_16
    cmp         w4, #3                      // 24
    b.eq        pre_24
    cmp         w4, #4                      // 32
    b.eq        pre_32

    ret

    pre_8:
        adr         x19, vector_8
        adr         x20, scalar_8

        mov         w21, #127
        mov         w22, #-128
        dup         v19.4s, w21
        dup         v20.4s, w22

        b           loop
    pre_16:
        adr         x19, vector_16
        adr         x20, scalar_16

        mov         w21, #32767
        mov         w22, #-32768
        dup         v19.4s, w21
        dup         v20.4s, w22

        b           loop
    pre_24:
        adr         x20, scalar_24

        ldr         w21, =0x7FFFFF
        ldr         w22, =-0x800000

        mov         w9, #1

        b           loop
    pre_32:
        adr         x19, vector_32
        adr         x20, scalar_32

        ldr         w21, =0x7FFFFFFF
        ldr         w22, =-0x80000000
        dup         v19.4s, w21
        dup         v20.4s, w22

        b           loop

loop:
    cmp         x12, x7                         // size - read'
    b.ls        end_loop                        // this address jump use unsigned comparison.

    add         x8, x0, x7

    cbnz        w9, do_scalar

    sub         x13, x12, x7
    cmp         x13, #16
    cset        w9, lt

    cbnz        w9, do_scalar
    br          x19

    do_scalar:
        br          x20
continue_loop:
    cbz         w9, inc_vector

    inc_scalar:
        add         x7, x7, x4
        b           continue_loop_end
    inc_vector:
        add         x7, x7, #16

    continue_loop_end:
        cbnz        w10, volumeRamping

        b           loop
end_loop:
    ldp         x29, x30, [sp], #16
    ldp         x23, xzr, [sp], #16
    ldp         x21, x22, [sp], #16
    ldp         x19, x20, [sp], #16

    ret

volumeRamping:
    fmov        s0, s16                     // replicate progress to 'first argument'

    /* input:
        s0 : progress
        s1 : reflected volume (start)
        s2 : target volume    (end)
     */
    blr         x6
    /* output:
        s0 : out
        s1 : start (reflected) [maintained]
        s2 : end   (target)    [maintained]

        INFO : the easing function is guaranteed not to change start & end,
               so register shifting to preserve value is unnecessary.
     */

    cmp         w11, w3
    b.ne        VR_increaseChannleCount     // if (channel count < channel)

    fadd        s16, s16, s17
    mov         w11, #1
    b           VR_skipIncreaseChannel

    VR_increaseChannleCount:
        add         w11, w11, #1

    VR_skipIncreaseChannel:

    fmov        s18, s0

    fcmp        s18, s2                     // pre-save the (current - target) operation result to the NZCV flag
    cbz         w23, VR_rampDown
    VR_rampUp:
        b.lt        VR_end

    VR_blockade:
        mov         w10, #0
        fmov        s18, s2
        str         s2, [x5]

    VR_end:
        b           loop

    VR_rampDown:
        b.le        VR_blockade
        b           VR_end

vector_8:
    ld1         {v3.16b}, [x8]

    sxtl        v4.8h, v3.8b
    sxtl2       v5.8h, v3.16b
    sxtl        v6.4s, v4.4h                // set 1 - v6
    sxtl2       v7.4s, v4.8h                // set 2 - v7
    sxtl        v4.4s, v5.4h                // set 3 - v4
    sxtl2       v5.4s, v5.8h                // set 4 - v5

    scvtf       v6.4s, v6.4s
    scvtf       v7.4s, v7.4s
    scvtf       v4.4s, v4.4s
    scvtf       v5.4s, v5.4s

    fmul        v6.4s, v6.4s, v18.s[0]
    fmul        v7.4s, v7.4s, v18.s[0]
    fmul        v4.4s, v4.4s, v18.s[0]
    fmul        v5.4s, v5.4s, v18.s[0]

    fcvtzs      v6.4s, v6.4s
    fcvtzs      v7.4s, v7.4s
    fcvtzs      v4.4s, v4.4s
    fcvtzs      v5.4s, v5.4s

    smin        v6.4s, v6.4s, v19.4s
    smax        v6.4s, v6.4s, v20.4s
    smin        v7.4s, v7.4s, v19.4s
    smax        v7.4s, v7.4s, v20.4s
    smin        v4.4s, v4.4s, v19.4s
    smax        v4.4s, v4.4s, v20.4s
    smin        v5.4s, v5.4s, v19.4s
    smax        v5.4s, v5.4s, v20.4s

    sqxtn       v21.4h, v6.4s
    sqxtn2      v21.8h, v7.4s               // pack 1 - v21
    sqxtn       v6.4h, v4.4s
    sqxtn2      v6.8h, v5.4s                // pack 2 - v6
    sqxtn       v3.8b, v21.8h
    sqxtn2      v3.16b, v6.8h               // final pack - v3

    st1         {v3.16b}, [x8]

    b           continue_loop
scalar_8:
    ldrsb       w13, [x8]
    scvtf       s3, w13
    fmul        s3, s3, s18
    fcvtzs      w13, s3

    cmp         w13, w21
    csel        w13, w21, w13, gt
    cmp         w13, w22
    csel        w13, w22, w13, lt

    strb        w13, [x8]

    b           continue_loop

vector_16:
    ld1         {v3.8h}, [x8]

    sxtl        v4.4s, v3.4h
    sxtl2       v5.4s, v3.8h

    scvtf       v4.4s, v4.4s
    scvtf       v5.4s, v5.4s
    fmul        v4.4s, v4.4s, v18.s[0]
    fmul        v5.4s, v5.4s, v18.s[0]
    fcvtzs      v4.4s, v4.4s
    fcvtzs      v5.4s, v5.4s

    smin        v4.4s, v4.4s, v19.4s 
    smax        v4.4s, v4.4s, v20.4s
    smin        v5.4s, v5.4s, v19.4s
    smax        v5.4s, v5.4s, v20.4s

    sqxtn       v3.4h, v4.4s
    sqxtn2      v3.8h, v5.4s

    st1         {v3.8h}, [x8]

    b           continue_loop
scalar_16:
    ldrsh       w13, [x8]
    scvtf       s3, w13
    fmul        s3, s3, s18
    fcvtzs      w13, s3

    cmp         w13, w21
    csel        w13, w21, w13, gt
    cmp         w13, w22
    csel        w13, w22, w13, lt

    strh        w13, [x8]

    b           continue_loop

scalar_24:
    ldrb        w13, [x8]
    ldrb        w14, [x8, #1]
    ldrb        w15, [x8, #2]
    orr         w13, w13, w14, lsl #8
    orr         w13, w13, w15, lsl #16

    sbfx        w13, w13, #0, #24

    scvtf       s3, w13
    fmul        s3, s3, s18
    fcvtzs      w13, s3

    cmp         w13, w21
    csel        w13, w21, w13, gt
    cmp         w13, w22
    csel        w13, w22, w13, lt

    strb        w13, [x8]
    lsr         w14, w13, #8
    strb        w14, [x8, #1]
    lsr         w14, w13, #16
    strb        w14, [x8, #2]

    b           continue_loop

vector_32:
    ld1         {v3.4s}, [x8]
    scvtf       v4.4s, v3.4s
    fmul        v4.4s, v4.4s, v18.s[0]
    fcvtzs      v4.4s, v4.4s

    smin        v4.4s, v4.4s, v19.4s
    smax        v4.4s, v4.4s, v20.4s

    st1         {v4.4s}, [x8]

    b           continue_loop
scalar_32:
    ldr         w13, [x8]
    scvtf       s3, w13
    fmul        s3, s3, s18
    fcvtzs      w13, s3

    cmp         w13, w21
    csel        w13, w21, w13, gt
    cmp         w13, w22
    csel        w13, w22, w13, lt

    str         w13, [x8]

    b           continue_loop