.section __TEXT,__const
    .p2align 6
vector_24_shuffleIndices:
    .byte 0,1,2,0xFF   , 3,4,5,0xFF   , 6,7,8,0xFF   , 9,10,11,0xFF
    .byte 12,13,14,0xFF, 15,16,17,0xFF, 18,19,20,0xFF, 21,22,23,0xFF
    .byte 24,25,26,0xFF, 27,28,29,0xFF, 30,31,32,0xFF, 33,34,35,0xFF
    .byte 36,37,38,0xFF, 39,40,41,0xFF, 42,43,44,0xFF, 45,46,47,0xFF
vector_24_packIndices:
    .byte 0,1,2, 4,5,6   , 8,9,10  , 12,13,14, 16,17,18, 20
    .byte 21,22, 24,25,26, 28,29,30, 32,33,34, 36,37,38, 40,41
    .byte 42   , 44,45,46, 48,49,50, 52,53,54, 56,57,58, 60,61,62

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
- w24 : size of bytes to read(on vector)

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
    stp         x23, x24, [sp, #-16]!
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

        mov         w24, #16

        b           loop
    pre_16:
        adr         x19, vector_16
        adr         x20, scalar_16

        mov         w21, #32767
        mov         w22, #-32768
        dup         v19.4s, w21
        dup         v20.4s, w22

        mov         w24, #64

        b           loop
    pre_24:
        adr         x19, vector_24
        adr         x20, scalar_24

        ldr         w21, =0x7FFFFF
        ldr         w22, =-0x800000
        dup         v19.4s, w21
        dup         v20.4s, w22

        mov         w24, #48

        b           loop
    pre_32:
        adr         x19, vector_32
        adr         x20, scalar_32

        ldr         w21, =0x7FFFFFFF
        ldr         w22, =-0x80000000
        dup         v19.4s, w21
        dup         v20.4s, w22

        mov         w24, #128

        b           loop

loop:
    cmp         x12, x7                         // size - read'
    b.ls        end_loop                        // this address jump use unsigned comparison.

    add         x8, x0, x7

    cbnz        w9, do_scalar

    sub         x13, x12, x7
    cmp         x13, x24
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
        add         x7, x7, x24

    continue_loop_end:
        cbnz        w10, volumeRamping

        b           loop
end_loop:
    ldp         x29, x30, [sp], #16
    ldp         x23, x24, [sp], #16
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

vector_16_old:
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
vector_16:
    ld1         {v3.8h, v4.8h, v5.8h, v6.8h}, [x8]

    sxtl        v21.4s, v3.4h
    sxtl2       v22.4s, v3.8h
    sxtl        v23.4s, v4.4h
    sxtl2       v24.4s, v4.8h
    sxtl        v25.4s, v5.4h
    sxtl2       v26.4s, v5.8h
    sxtl        v27.4s, v6.4h
    sxtl2       v28.4s, v6.8h

    scvtf       v21.4s, v21.4s
    scvtf       v22.4s, v22.4s
    scvtf       v23.4s, v23.4s
    scvtf       v24.4s, v24.4s
    scvtf       v25.4s, v25.4s
    scvtf       v26.4s, v26.4s
    scvtf       v27.4s, v27.4s
    scvtf       v28.4s, v28.4s

    fmul        v21.4s, v21.4s, v18.s[0]
    fmul        v22.4s, v22.4s, v18.s[0]
    fmul        v23.4s, v23.4s, v18.s[0]
    fmul        v24.4s, v24.4s, v18.s[0]
    fmul        v25.4s, v25.4s, v18.s[0]
    fmul        v26.4s, v26.4s, v18.s[0]
    fmul        v27.4s, v27.4s, v18.s[0]
    fmul        v28.4s, v28.4s, v18.s[0]

    fcvtzs      v21.4s, v21.4s
    fcvtzs      v22.4s, v22.4s
    fcvtzs      v23.4s, v23.4s
    fcvtzs      v24.4s, v24.4s
    fcvtzs      v25.4s, v25.4s
    fcvtzs      v26.4s, v26.4s
    fcvtzs      v27.4s, v27.4s
    fcvtzs      v28.4s, v28.4s

    smin        v21.4s, v21.4s, v19.4s
    smax        v21.4s, v21.4s, v20.4s
    smin        v22.4s, v22.4s, v19.4s
    smax        v22.4s, v22.4s, v20.4s
    smin        v23.4s, v23.4s, v19.4s
    smax        v23.4s, v23.4s, v20.4s
    smin        v24.4s, v24.4s, v19.4s
    smax        v24.4s, v24.4s, v20.4s
    smin        v25.4s, v25.4s, v19.4s
    smax        v25.4s, v25.4s, v20.4s
    smin        v26.4s, v26.4s, v19.4s
    smax        v26.4s, v26.4s, v20.4s
    smin        v27.4s, v27.4s, v19.4s
    smax        v27.4s, v27.4s, v20.4s
    smin        v28.4s, v28.4s, v19.4s
    smax        v28.4s, v28.4s, v20.4s

    sqxtn       v3.4h, v21.4s
    sqxtn2      v3.8h, v22.4s
    sqxtn       v4.4h, v23.4s
    sqxtn2      v4.8h, v24.4s
    sqxtn       v5.4h, v25.4s
    sqxtn2      v5.8h, v26.4s
    sqxtn       v6.4h, v27.4s
    sqxtn2      v6.8h, v28.4s

    st1         {v3.8h, v4.8h, v5.8h, v6.8h}, [x8]

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

vector_24:
    ld1         {v3.16b, v4.16b, v5.16b}, [x8]
    // v3 : 3 3 3 3 3 1
    // v4 : 2 3 3 3 3 2
    // v5 : 1 3 3 3 3 3

    // shuffle
    adrp        x13, vector_24_shuffleIndices@PAGE
    add         x13, x13, vector_24_shuffleIndices@PAGEOFF
    ld1         {v21.16b, v22.16b, v23.16b, v24.16b}, [x13]

    tbl         v25.16b, {v3.16b, v4.16b, v5.16b}, v21.16b
    tbl         v26.16b, {v3.16b, v4.16b, v5.16b}, v22.16b
    tbl         v27.16b, {v3.16b, v4.16b, v5.16b}, v23.16b
    tbl         v28.16b, {v3.16b, v4.16b, v5.16b}, v24.16b

    // signed extension
    shl         v25.4s, v25.4s, #8
    sshr        v25.4s, v25.4s, #8
    shl         v26.4s, v26.4s, #8
    sshr        v26.4s, v26.4s, #8
    shl         v27.4s, v27.4s, #8
    sshr        v27.4s, v27.4s, #8
    shl         v28.4s, v28.4s, #8
    sshr        v28.4s, v28.4s, #8

    scvtf       v25.4s, v25.4s
    scvtf       v26.4s, v26.4s
    scvtf       v27.4s, v27.4s
    scvtf       v28.4s, v28.4s

    fmul        v25.4s, v25.4s, v18.s[0]
    fmul        v26.4s, v26.4s, v18.s[0]
    fmul        v27.4s, v27.4s, v18.s[0]
    fmul        v28.4s, v28.4s, v18.s[0]

    fcvtzs      v25.4s, v25.4s
    fcvtzs      v26.4s, v26.4s
    fcvtzs      v27.4s, v27.4s
    fcvtzs      v28.4s, v28.4s

    smin        v25.4s, v25.4s, v19.4s
    smax        v25.4s, v25.4s, v20.4s
    smin        v26.4s, v26.4s, v19.4s
    smax        v26.4s, v26.4s, v20.4s
    smin        v27.4s, v27.4s, v19.4s
    smax        v27.4s, v27.4s, v20.4s
    smin        v28.4s, v28.4s, v19.4s
    smax        v28.4s, v28.4s, v20.4s

    // pack
    adrp        x13, vector_24_packIndices@PAGE
    add         x13, x13, vector_24_packIndices@PAGEOFF
    ld1         {v21.16b, v22.16b, v23.16b}, [x13]

    tbl         v3.16b, {v25.16b, v26.16b, v27.16b, v28.16b}, v21.16b
    tbl         v4.16b, {v25.16b, v26.16b, v27.16b, v28.16b}, v22.16b
    tbl         v5.16b, {v25.16b, v26.16b, v27.16b, v28.16b}, v23.16b

    st1         {v3.16b, v4.16b, v5.16b}, [x8]

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

vector_32_old:
    ld1         {v3.4s}, [x8]

    scvtf       v4.4s, v3.4s
    fmul        v4.4s, v4.4s, v18.s[0]
    fcvtzs      v4.4s, v4.4s

    smin        v4.4s, v4.4s, v19.4s
    smax        v4.4s, v4.4s, v20.4s

    st1         {v4.4s}, [x8]

    b           continue_loop
vector_32:
    ld1         {v21.4s, v22.4s, v23.4s, v24.4s}, [x8]
    ld1         {v25.4s, v26.4s, v27.4s, v28.4s}, [x8]

    scvtf       v21.4s, v21.4s
    scvtf       v22.4s, v22.4s
    scvtf       v23.4s, v23.4s
    scvtf       v24.4s, v24.4s
    scvtf       v25.4s, v25.4s
    scvtf       v26.4s, v26.4s
    scvtf       v27.4s, v27.4s
    scvtf       v28.4s, v28.4s

    fmul        v21.4s, v21.4s, v18.s[0]
    fmul        v22.4s, v22.4s, v18.s[0]
    fmul        v23.4s, v23.4s, v18.s[0]
    fmul        v24.4s, v24.4s, v18.s[0]
    fmul        v25.4s, v25.4s, v18.s[0]
    fmul        v26.4s, v26.4s, v18.s[0]
    fmul        v27.4s, v27.4s, v18.s[0]
    fmul        v28.4s, v28.4s, v18.s[0]

    fcvtzs      v21.4s, v21.4s
    fcvtzs      v22.4s, v22.4s
    fcvtzs      v23.4s, v23.4s
    fcvtzs      v24.4s, v24.4s
    fcvtzs      v25.4s, v25.4s
    fcvtzs      v26.4s, v26.4s
    fcvtzs      v27.4s, v27.4s
    fcvtzs      v28.4s, v28.4s

    smin        v21.4s, v21.4s, v19.4s
    smax        v21.4s, v21.4s, v20.4s
    smin        v22.4s, v22.4s, v19.4s
    smax        v22.4s, v22.4s, v20.4s
    smin        v23.4s, v23.4s, v19.4s
    smax        v23.4s, v23.4s, v20.4s
    smin        v24.4s, v24.4s, v19.4s
    smax        v24.4s, v24.4s, v20.4s
    smin        v25.4s, v25.4s, v19.4s
    smax        v25.4s, v25.4s, v20.4s
    smin        v26.4s, v26.4s, v19.4s
    smax        v26.4s, v26.4s, v20.4s
    smin        v27.4s, v27.4s, v19.4s
    smax        v27.4s, v27.4s, v20.4s
    smin        v28.4s, v28.4s, v19.4s
    smax        v28.4s, v28.4s, v20.4s

    st1         {v21.4s, v22.4s, v23.4s, v24.4s}, [x8], #64
    st1         {v25.4s, v26.4s, v27.4s, v28.4s}, [x8]
    sub         x8, x8, #64

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