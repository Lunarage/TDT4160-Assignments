inputs = [2,3,4,9]

for r0 in inputs:
    r1 = 0
    r2 = 0
    r3 = True
    # 0x00 ldi %r2, 1
    r2 = 1
    while r3:
        # 0x04 ldi %r1, 1
        r1 = 1
        # 0x08 mul %r2, %r2, %r0
        r2 = r2 * r0
        # 0x0C sub %r0, %r0, %r1
        r0 = r0 - r1
        # Condition for loop
        # 0x10 cmp %r3, %r0, %r1
        r3 = r0 > r1
        # 0x14 jgt %r3, -16
    print(r2)

#Weird factorial but OK
