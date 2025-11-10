.text
    .global _updateAudioDataAmplitude
    .p2align 4

_updateAudioDataAmplitude:
    mov        w3, #0
    mov        w8, #0
    mov        w9, w1

    cmp        w9, #1
    b.eq       pre_8
    cmp        w9, #2
    b.eq       pre_16
    cmp        w9, #3
    b.eq       pre_24
    cmp        w9, #4
    b.eq       pre_32

    ret

    pre_8:
        adr        x10, vector_8
        adr        x11, scalar_8

        mov        w12, #127
        mov        w13, #-128
        dup        v12.4s, w12
        dup        v13.4s, w13

        b          loop
    pre_16:
        adr        x10, vector_16
        adr        x11, scalar_16

        mov        w12, #32767
        mov        w13, #-32768
        dup        v12.4s, w12
        dup        v13.4s, w13

        b          loop
    pre_24:
        adr        x11, scalar_24

        ldr        w12, =0x7FFFFF
        ldr        w13, =-0x800000

        b          loop
    pre_32:
        adr        x10, vector_32
        adr        x11, scalar_32

        ldr        w12, =0x7FFFFFFF
        ldr        w13, =-0x80000000
        dup        v12.4s, w12
        dup        v13.4s, w13

        b          loop

loop:
    cmp        w2, w3
    b.ls       end_loop

    add        x4, x0, x3

    cmp        w9, #3
    b.eq       do_scalar
    cbnz       w8, do_scalar

    sub        w5, w2, w3
    cmp        w5, #16
    cset       w8, lt

    cbnz       w8, do_scalar
    br         x10

    do_scalar:
        br         x11
continue_loop:
    cmp        w8, #0
    b.eq       inc_vector

    inc_scalar:
        add        w3, w3, w9
        b          loop
    inc_vector:
        add        w3, w3, #16
        b          loop
end_loop:
    ret

vector_8:
    ld1        {v7.16b}, [x4]

    // 8 -> 32
    sxtl       v1.8h, v7.8b
    sxtl2      v2.8h, v7.16b
    sxtl       v3.4s, v1.4h
    sxtl2      v4.4s, v1.8h
    sxtl       v5.4s, v2.4h
    sxtl2      v6.4s, v2.8h

    scvtf      v3.4s, v3.4s
    scvtf      v4.4s, v4.4s
    scvtf      v5.4s, v5.4s
    scvtf      v6.4s, v6.4s

    dup        v8.4s, v0.s[0]
    fmul       v3.4s, v3.4s, v8.4s
    fmul       v4.4s, v4.4s, v8.4s
    fmul       v5.4s, v5.4s, v8.4s
    fmul       v6.4s, v6.4s, v8.4s

    fcvtzs     v3.4s, v3.4s
    fcvtzs     v4.4s, v4.4s
    fcvtzs     v5.4s, v5.4s
    fcvtzs     v6.4s, v6.4s

    smax       v3.4s, v3.4s, v13.4s
    smin       v3.4s, v3.4s, v12.4s
    smax       v4.4s, v4.4s, v13.4s
    smin       v4.4s, v4.4s, v12.4s
    smax       v5.4s, v5.4s, v13.4s
    smin       v5.4s, v5.4s, v12.4s
    smax       v6.4s, v6.4s, v13.4s
    smin       v6.4s, v6.4s, v12.4s

    // 32 -> 8
    sqxtn      v1.4h, v3.4s
    sqxtn2     v1.8h, v4.4s
    sqxtn      v2.4h, v5.4s
    sqxtn2     v2.8h, v6.4s
    sqxtn      v7.8b, v1.8h
    sqxtn2     v7.16b, v2.8h

    st1        {v7.16b}, [x4]

    b          continue_loop
scalar_8:
    ldrsb      w5, [x4]
    scvtf      s1, w5
    fmul       s1, s1, s0
    fcvtzs     w5, s1

    cmp        w5, w12
    csel       w5, w12, w5, gt
    cmp        w5, w13
    csel       w5, w13, w5, lt

    strb       w5, [x4]

    b          continue_loop

vector_16:
    ld1        {v7.8h}, [x4]

    sxtl       v1.4s, v7.4h
    sxtl2      v2.4s, v7.8h

    dup        v8.4s, v0.s[0]
    scvtf      v1.4s, v1.4s
    scvtf      v2.4s, v2.4s
    fmul       v1.4s, v1.4s, v8.4s
    fmul       v2.4s, v2.4s, v8.4s
    fcvtzs     v1.4s, v1.4s
    fcvtzs     v2.4s, v2.4s

    smax       v1.4s, v1.4s, v13.4s
    smin       v1.4s, v1.4s, v12.4s
    smax       v2.4s, v2.4s, v13.4s
    smin       v2.4s, v2.4s, v12.4s

    sqxtn      v7.4h, v1.4s
    sqxtn2     v7.8h, v2.4s

    st1        {v7.8h}, [x4]

    b          continue_loop
scalar_16:
    ldrsh      w5, [x4]
    scvtf      s1, w5
    fmul       s1, s1, s0
    fcvtzs     w5, s1

    cmp        w5, w12
    csel       w5, w12, w5, gt
    cmp        w5, w13
    csel       w5, w13, w5, lt

    strh       w5, [x4]

    b          continue_loop

scalar_24:
    ldrb       w5, [x4]
    ldrb       w6, [x4, #1]
    ldrb       w7, [x4, #2]
    orr        w5, w5, w6, lsl #8
    orr        w5, w5, w7, lsl #16
    
    sbfx       w5, w5, #0, #24

    scvtf      s1, w5
    fmul       s1, s1, s0
    fcvtzs     w5, s1

    cmp        w5, w12
    csel       w5, w12, w5, gt
    cmp        w5, w13
    csel       w5, w13, w5, lt

    and        w5, w5, #0x00FFFFFF

    strb       w5, [x4]
    lsr        w6, w5, #8
    strb       w6, [x4, #1]
    lsr        w6, w5, #16
    strb       w6, [x4, #2]

    b          continue_loop

vector_32:
    ld1        {v7.4s}, [x4]
    scvtf      v1.4s, v7.4s
    fmul       v1.4s, v1.4s, v0.s[0]
    fcvtzs     v2.4s, v1.4s

    smax       v2.4s, v2.4s, v13.4s
    smin       v2.4s, v2.4s, v12.4s

    st1        {v2.4s}, [x4]

    b          continue_loop
scalar_32:
    ldr        w5, [x4]
    scvtf      s1, w5
    fmul       s1, s1, s0
    fcvtzs     w5, s1

    cmp        w5, w12
    csel       w5, w12, w5, gt
    cmp        w5, w13
    csel       w5, w13, w5, lt

    str        w5, [x4]

    b          continue_loop