ENTRY(_start)              /* 告诉链接器，程序入口是 _start */
SECTIONS {
    . = 0x80000000;        /* RISC-V virt 平台规定内核从这里加载 */

    .text : {
        *(.text .text.*)   /* 所有代码放这里 */
    }

    .rodata : {
        *(.rodata .rodata.*)
    }

    .data : {
        *(.data .data.*)   /* 已初始化的全局变量 */
    }

    .bss : {
        bss_start = .;     /* 符号，BSS 段开始 */
        *(.bss .bss.*)
        *(COMMON)          /* 未初始化的全局变量 */
        bss_end = .;       /* 符号，BSS 段结束 */
    }

    .stack : {
        . = ALIGN(16);
        stack_bottom = .;
        . = . + 0x4000;    /* 给栈分配 16KB */
        stack_top = .;     /* 栈顶符号，entry.S 会用 */
    }
}
