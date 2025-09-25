ENTRY(_start)
SECTIONS {
    . = 0x80000000;

    _stext = .;
    .text : {
        *(.text .text.*)
    }
    . = ALIGN(0x1000);
    _etext = .;

    _srodata = .;
    .rodata : {
        *(.rodata .rodata.*)
    }
    . = ALIGN(0x1000);
    _erodata = .;

    _sdata = .;
    .data : {
        *(.data .data.*)
    }
    . = ALIGN(0x1000);
    _edata = .;

    _sbss = .;
    .bss : {
        bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        bss_end = .;
    }
    . = ALIGN(0x1000);
    _ebss = .;

    .stack : {
        . = ALIGN(0x1000);
        stack_bottom = .;
        . = . + 0x4000;
        stack_top = .;
    }
    . = ALIGN(0x1000);

    PROVIDE(_end = .);
}