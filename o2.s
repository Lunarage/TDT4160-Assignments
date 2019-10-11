.thumb
.syntax unified

.include "gpio_constants.s"     // Register-adresser og konstanter for GPIO
.include "sys-tick_constants.s" // Register-adresser og konstanter for SysTick

.text
    .global Start

Start:

    //Setup Systick
    ldr r0, =SYSTICK_BASE + SYSTICK_CTRL //SYSTICK CONTROL AND STATUS REGISTER
    ldr r1, =SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk
    str r1, [r0]

    ldr r0, =SYSTICK_BASE + SYSTICK_LOAD //SYSTICK RELOAD VALUE REGISTER
    ldr r1, =FREQUENCY / 10
    str r1, [r0]

    //Setup Interrupt
    ldr r0, =GPIO_BASE + GPIO_EXTIPSELH
    ldr r1, [r0] //CONTENTS OF EXTIPSELH
    mov r2, #0b1111
    lsl r2, r2, #4 //1111 0000
    mvn r2, r2 //INVERT 1111 ... 0000 1111
    and r2, r2, r1 //xxxx ... 0000 xxxx
    ldr r1, =BUTTON_PORT // 0001
    lsl r1, r1, #4 // 0000 ... 0001 0000
    orr r1, r1, r2 // xxxx ... xxx1 xxxx
    str r1, [r0]

    ldr r0, =GPIO_BASE + GPIO_EXTIFALL
    ldr r1, [r0]
    mov r2, #0b0001
    lsl r2, r2, BUTTON_PIN
    orr r2, r2, r2
    str r2, [r0]

    //RESET INTERRUPT FLAG REGISTER ON BIUTTON PIN
    ldr r0, =GPIO_BASE + GPIO_IFC
    mov r1, #0b0001
    lsl r1, r1, BUTTON_PIN
    str r1, [r0]

    //ENABLE INTERRUPT
    ldr r0, =GPIO_BASE + GPIO_IEN
    mov r1, #0b0001
    lsl r1, r1, BUTTON_PIN
    str r1, [r0]

//INFINITE LOOP
loop:
    b loop

.global SysTick_Handler
.thumb_func
SysTick_Handler:
    //TENTHS
    ldr r0, =tenths
    ldr r4, [r0]
    add r4, r4, #1
    cmp r4, #10
    bne write_tenths //SKIP THE REST UNLESS WE ARE AT 10 TENTHS
    mov r4, #0

    //SECONDS
    ldr r1, =seconds
    ldr r5, [r1]
    add r5, r5, #1

    //LED TOGGLE
    ldr r12, =GPIO_BASE + (PORT_SIZE * LED_PORT) + GPIO_PORT_DOUTTGL
    ldr r11, =1 << LED_PIN
    str r11, [r12]

    cmp r5, #60
    bne write_seconds //SKIP THE REST UNLESS WE ARE AT 60 SECONDS
    mov r5, #0

    //MINUTES
    ldr r2, =minutes
    ldr r6, [r2]
    add r6, r6, #1

    write_minutes:
        str r6, [r2]
    write_seconds:
        str r5, [r1]
    write_tenths:
        str r4, [r0]

    bx lr

.global GPIO_ODD_IRQHandler
.thumb_func
GPIO_ODD_IRQHandler:
    //TOGGLE CLOCK
    ldr r0, =SYSTICK_BASE + SYSTICK_CTRL
    ldr r1, [r0]
    eor r1, SysTick_CTRL_ENABLE_Msk //TOGGLE BIT FOR ENABLE WITH XOR
    str r1, [r0]

    //RESET INTERRUPT FLAG REGISTER ON BIUTTON PIN
    ldr r0, =GPIO_BASE + GPIO_IFC
    mov r1, #0b0001
    lsl r1, r1, BUTTON_PIN
    str r1, [r0]

    bx lr

NOP

