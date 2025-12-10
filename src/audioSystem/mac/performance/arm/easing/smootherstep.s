.text
    .align 2
    CONST_6_0:  .float 6.0
    CONST_15_0: .float 15.0
    CONST_10_0: .float 10.0

    .global _smootherstep
    .p2align 4

// This easing function follows the following formula:
// f(t) = (t**3) * ((t*((t*6)-15))+10)

// s0 : t && out
// s1 : start
// s2 : end
// s3 : calcReg
// s4 : calcReg1
// s5 : const 6.0 & const 15.0 & const 10.0
_smootherstep:

    fmul        s3, s0, s0          // s0 ** 2 -> s3
    fmul        s3, s3, s0          // s3 * s0 -> s3
    // -> s3 = t**3

    ldr         s5, CONST_6_0
    fmul        s4, s0, s5          // s0 * 6 -> s4
    ldr         s5, CONST_15_0
    fsub        s4, s4, s5          // s4 -= 15
    fmul        s4, s4, s0          // s4 *= s0(t)
    ldr         s5, CONST_10_0
    fadd        s4, s4, s5          // s4 += 10
    // -> s4 = ((t*((t*6)-15))+10)

    fmul        s3, s3, s4          // s3 * s4 -> s3
    // -> s3 = f(t) scale 0~1

    fsub        s4, s2, s1          // end - start -> s4
    fmul        s3, s3, s4          // (end-start) * f(t) -> s3
    fadd        s0, s3, s1          // ...+start -> s0

    ret