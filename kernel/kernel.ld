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
        PROVIDE(_etext = .);
    }

    .rodata : {
        . = ALIGN(16);
        *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
        . = ALIGN(16);
        *(.rodata .rodata.*)
    }

    .data : {
        . = ALIGN(16);
        *(.sdata .sdata.*) /* do not need to distinguish this from .data */
        . = ALIGN(16);
        *(.data .data.*)
    }

    .bss : {
        . = ALIGN(16);
        bss_start = .;
        *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
        . = ALIGN(16);
        *(.bss .bss.*)
        bss_end = .;
    }

    .stack : {
        . = ALIGN(0x1000);
        stack_bottom = .;
        . = . + 0x4000;
        stack_top = .;
    }
    . = ALIGN(0x1000);

    PROVIDE(_end = .);
}