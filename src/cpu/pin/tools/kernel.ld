SECTIONS
{
 . = 0xdead0000000;
 .text : { *(.text) }
 .data : { *(.data) }
 .bss : { *(.bss) }
}
