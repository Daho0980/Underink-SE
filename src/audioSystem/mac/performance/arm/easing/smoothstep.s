.text
    .global _smoothstep
    .p2align 4
// This easing function follows the following formula:
// f(t) = (t**2) * (3-(t*2))

// s0 : t && out
// s1 : start
// s2 : end
// s3 : calcReg
// s4 : calcReg1
// s5 : const 2.0 & const 3.0
_smoothstep:

    fmul        s3, s0, s0          // s0 ** 2 -> s3
    // -> s3 = t**2

    fmov        s5, #2.0
    fmul        s4, s0, s5          // s0 * 2 -> s4
    fmov        s5, #3.0
    fsub        s4, s5, s4          // s4 = 3 - s4
    // s4 = (3-(t*2))

    fmul        s3, s3, s4          // s3 * s4 -> s3
    // -> s3 = f(t) scale 0~1

    fsub        s4, s2, s1          // end - start -> s4
    fmul        s3, s3, s4          // (end-start) * f(t) -> s3
    fadd        s0, s3, s1          // ...+start -> s0

    ret