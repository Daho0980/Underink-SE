.data
.align 4
scaleFactor:
    .float 0.0001


.text
.global  _updateVolume
.p2align 2

_updateVolume:
    movss       XMM2, [RIP + scaleFactor]

    mulss       XMM0, XMM1
    mulss       XMM0, XMM2
    movss       [RDI], XMM0

    ret