.text
    .align 2
    CONST_6_0:  .float 6.0
    CONST_15_0: .float 15.0
    CONST_10_0: .float 10.0

    .global _smootherstep
    .p2align 4

// This easing function follows the following formula:
// f(t) = (t**3) * ((t*((t*6)-15))+10)
// In this formula, valid results can be obtained when t is a float value between 0 and 1.

// s0 : t && out
// s1 : start
// s2 : end
// s3 : calcReg
// s4 : calcReg1
// s5 : const 6.0
// s6 : const 15.0
// s7 : const 10.0
_smootherstep:
    ldr         s5, CONST_6_0
    ldr         s6, CONST_15_0
    ldr         s7, CONST_10_0

    fmul        s3, s0, s0          // s0**2 -> s3
    fmul        s3, s3, s0          // s3*s0 -> s3
    // -> s3 = t**3

    fmul        s4, s0, s5          // s0 * 6 -> s4
    fsub        s4, s4, s6          // s4 -= 15
    fmul        s4, s4, s0          // s4 *= s0(t)
    fadd        s4, s4, s7          // s4 += 10
    // -> s4 = ((t*((t*6)-15))+10)

    fmul        s3, s3, s4          // s3 * s4 -> s3
    // -> s3 = f(t) scale 0~1

    fsub        s4, s2, s1          // end - start -> s4
    fmul        s3, s3, s4          // (end-start)*f(t) -> s3
    fadd        s0, s3, s1          // ...+start -> s0

    ret