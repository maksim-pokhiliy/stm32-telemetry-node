ENTRY(Reset_Handler)

MEMORY
{
    FLASH : ORIGIN = 0x08000000, LENGTH = 512K
    SRAM : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    .vector_table : {
        KEEP(*(.vector_table))
    } > FLASH

    .text : {
        *(.text*)
    } > FLASH

    .rodata : {
        *(.rodata*)
    } > FLASH

    .data : {
        . = ALIGN(4);
        DATA_SRAM_START = .;

        *(.data*)

        . = ALIGN(4);
        DATA_SRAM_END = .;
    } > SRAM AT> FLASH

    .bss : {
        . = ALIGN(4);
        BSS_SRAM_START = .;

        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        BSS_SRAM_END = .;
    } > SRAM
}

DATA_FLASH_IMAGE_START = LOADADDR(.data);
STACK_TOP = ORIGIN(SRAM) + LENGTH(SRAM);
