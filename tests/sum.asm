# Sums 1..5 into r1 (expected result: 15). The familiar test program
# from phase 2/3 development, now as a real standalone .asm file
# instead of a hardcoded C string array.
        ADDI r1, r0, 0   # sum = 0
        ADDI r2, r0, 1   # i = 1
        ADDI r3, r0, 15   # limit = 6
loop:   BGE  r2, r3, end
        ADD  r1, r1, r2
        ADDI r2, r2, 1
        J    loop
end:    HALT
