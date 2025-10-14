ENTRY(_start)
SECTIONS {
    . = 0x80000000;

    _stext = .;
    .text : {
        kernel/entry.o(_entry)
        *(.text .text.*)
        . = ALIGN(0x1000);
        _trampoline = .;
        *(trampsec)
        . = ALIGN(0x1000);
        ASSERT(. - _trampoline == 0x1000, "error: trampoline larger than one page");
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