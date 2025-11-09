.data
.align 4
scaleFactor:
    .float 0.0001


.text
.global  _updateVolume
.p2align 4

_updateVolume:
    adrp        x9, scaleFactor@PAGE
    ldr         s2, [x9, scaleFactor@PAGEOFF]

    fmul        s0, s0, s1
    fmul        s0, s0, s2
    str         s0, [x0]

    ret