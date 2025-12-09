.text
    .global _linear
    .p2align 4
// This easing function follows the following formula:
// f(t) = t
// ...well. look, it's linear.
// Actually, Idk if this is really useful.

// s0 : t && out
// s1 : start
// s2 : end
// s3 : calcReg
_linear:
    fsub        s3, s2, s1          // end - start -> s3
    fmul        s3, s3, s0          // (end-start) * t -> s3
    fadd        s0, s3, s1          // ...+start -> s0

    ret