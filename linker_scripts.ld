/*
 * STM32F411RE memory layout.
 *
 * Design note:
 * - Sector 7 (the final 128 KiB) is reserved for persistent game data.
 * - Every C-runtime section is placed explicitly so that no linker "orphan"
 *   can silently end up in SRAM without being initialized by Reset_Handler.
 */
ENTRY(Reset_Handler)

_Min_Heap_Size  = 1K;
_Min_Stack_Size = 4K;

MEMORY
{
    FLASH_APP (rx)  : ORIGIN = 0x08000000, LENGTH = 384K
    FLASH_NVM (r)   : ORIGIN = 0x08060000, LENGTH = 128K
    SRAM      (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* The Cortex-M vector table stores this linker-provided initial MSP value. */
__StackTop   = ORIGIN(SRAM) + LENGTH(SRAM);
__StackLimit = __StackTop - _Min_Stack_Size;
__SramStart  = ORIGIN(SRAM);
__SramEnd    = ORIGIN(SRAM) + LENGTH(SRAM);
PROVIDE(_estack = __StackTop);

SECTIONS
{
    /*
     * KEEP is mandatory: --gc-sections cannot see a normal C reference to the
     * vector table and would otherwise remove it from the final image.
     */
    .isr_vector :
    {
        . = ALIGN(512);
        KEEP(*(.isr_vector))
        KEEP(*(.isr_section))
        . = ALIGN(4);
    } > FLASH_APP

    .text :
    {
        *(.text)
        *(.text.*)
        KEEP(*(.init))
        KEEP(*(.fini))
        *(.rodata)
        *(.rodata.*)
        /*
         * Name linker-generated interworking veneers explicitly. They are
         * normally empty for Cortex-M4 but must never become silent orphans.
         */
        *(.glue_7)
        *(.glue_7t)
        *(.vfp11_veneer)
        *(.v4_bx)
        *(.iplt)
        . = ALIGN(4);
    } > FLASH_APP

    .eh_frame :
    {
        *(.eh_frame)
        *(.eh_frame.*)
    } > FLASH_APP

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH_APP

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH_APP

    /* Static firmware normally emits no relocations; keep empty tables explicit. */
    .rel.dyn :
    {
        *(.rel.iplt)
        *(.rel.*)
    } > FLASH_APP

    /*
     * Constructor tables remain in Flash. Reset_Handler can therefore call
     * __libc_init_array() without depending on uninitialized SRAM contents.
     */
    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH_APP

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH_APP

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN(__fini_array_end = .);
        . = ALIGN(4);
    } > FLASH_APP

    /* _sidata is the Flash load address copied into SRAM by Reset_Handler. */
    _sidata = LOADADDR(.data);
    _la_data = _sidata; /* Backward-compatible alias for existing debug scripts. */

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data.*)
        *(.igot.plt)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH_APP

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = .;
    } > SRAM

    /*
     * Crash information intentionally survives a software reset. The record
     * contains a magic value, so random SRAM after power-on is safely ignored.
     */
    .noinit (NOLOAD) :
    {
        . = ALIGN(8);
        __noinit_start__ = .;
        *(.noinit)
        *(.noinit.*)
        . = ALIGN(8);
        __noinit_end__ = .;
    } > SRAM

    __static_ram_end__ = .;
    __heap_start__ = ALIGN(__static_ram_end__, 8);
    /* Use the canonical CMSIS-style stack symbol defined at the SRAM top. */
    __heap_end__ = __StackLimit;
    PROVIDE(end = __heap_start__);
    PROVIDE(_end = __heap_start__);

    /*
     * This symbol includes every Flash-loaded section, including constructor
     * tables and .data's load image. It makes the NVM overlap check complete.
     */
    __flash_image_end__ = LOADADDR(.data) + SIZEOF(.data);
    __flash_app_end__ = __flash_image_end__;
    __nvm_start__ = ORIGIN(FLASH_NVM);
    __nvm_end__ = ORIGIN(FLASH_NVM) + LENGTH(FLASH_NVM);

    ASSERT(__flash_image_end__ <= __nvm_start__,
           "ERROR: Firmware overlaps reserved Flash NVM sector")
    ASSERT((__heap_start__ + _Min_Heap_Size) <= __StackLimit,
           "ERROR: SRAM does not leave the required heap and main stack")
    ASSERT((__StackTop & 7) == 0,
           "ERROR: Cortex-M initial stack must be 8-byte aligned")

    /*
     * Debug/metadata sections have address zero and never consume MCU memory.
     * Listing them explicitly keeps --orphan-handling useful for detecting a
     * genuinely forgotten allocatable runtime section.
     */
    .stab             0 : { *(.stab) }
    .stabstr          0 : { *(.stabstr) }
    .comment          0 : { *(.comment) }
    .debug            0 : { *(.debug) }
    .line             0 : { *(.line) }
    .debug_srcinfo    0 : { *(.debug_srcinfo) }
    .debug_sfnames    0 : { *(.debug_sfnames) }
    .debug_aranges    0 : { *(.debug_aranges) }
    .debug_pubnames   0 : { *(.debug_pubnames) }
    .debug_info       0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev     0 : { *(.debug_abbrev) }
    .debug_line       0 : { *(.debug_line .debug_line.*) }
    .debug_frame      0 : { *(.debug_frame) }
    .debug_str        0 : { *(.debug_str) }
    .debug_loc        0 : { *(.debug_loc) }
    .debug_macinfo    0 : { *(.debug_macinfo) }
    .debug_weaknames  0 : { *(.debug_weaknames) }
    .debug_funcnames  0 : { *(.debug_funcnames) }
    .debug_typenames  0 : { *(.debug_typenames) }
    .debug_varnames   0 : { *(.debug_varnames) }
    .debug_pubtypes   0 : { *(.debug_pubtypes) }
    .debug_ranges     0 : { *(.debug_ranges) }
    .debug_macro      0 : { *(.debug_macro) }
    .debug_addr       0 : { *(.debug_addr) }
    .debug_line_str   0 : { *(.debug_line_str) }
    .debug_loclists   0 : { *(.debug_loclists) }
    .debug_rnglists   0 : { *(.debug_rnglists) }
    .ARM.attributes   0 : { KEEP(*(.ARM.attributes)) }

    /DISCARD/ :
    {
        *(.note.GNU-stack)
    }
}
