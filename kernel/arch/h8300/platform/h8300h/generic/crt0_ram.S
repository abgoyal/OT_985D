

#define ASSEMBLY

#include <asm/linkage.h>
	
#if !defined(CONFIG_BLKDEV_RESERVE)
#if defined(CONFIG_GDB_DEBUG)
#define RAMEND (__ramend - 0xc000)
#else
#define RAMEND __ramend
#endif
#else
#define RAMEND CONFIG_BLKDEV_RESERVE_ADDRESS
#endif
	
	.global SYMBOL_NAME(_start)
	.global SYMBOL_NAME(command_line)
	.global SYMBOL_NAME(_platform_gpio_table)
	.global SYMBOL_NAME(_target_name)
	
	.h8300h

	.section .text
	.file	"crt0_ram.S"

	/* CPU Reset entry */
SYMBOL_NAME_LABEL(_start)
	mov.l	#RAMEND,sp
	ldc	#0x80,ccr

	/* Peripheral Setup */
	
#if defined(CONFIG_BLK_DEV_BLKMEM)
	/* move romfs image */
	jsr	@__move_romfs	
#endif
	
	/* .bss clear */
	mov.l	#__sbss,er5
	mov.l	#__ebss,er4
	sub.l	er5,er4
	shlr	er4
	shlr	er4
	sub.l	er0,er0
1:	
	mov.l	er0,@er5
	adds	#4,er5
	dec.l	#1,er4
	bne	1b

	/* copy kernel commandline */
	mov.l	#COMMAND_START,er5
	mov.l	#SYMBOL_NAME(command_line),er6
	mov.w	#512,r4
	eepmov.w

	/* uClinux kernel start */
	ldc	#0x90,ccr	/* running kernel */
	mov.l	#SYMBOL_NAME(init_thread_union),sp
	add.l	#0x2000,sp
	jsr	@_start_kernel
_exit:

	jmp	_exit

	rts

	/* I/O port assign information */
__platform_gpio_table:	
	mov.l	#gpio_table,er0
	rts

gpio_table:
	;; P1DDR
	.byte	0x00,0x00
	;; P2DDR
	.byte	0x00,0x00
	;; P3DDR
	.byte	0x00,0x00
	;; P4DDR
	.byte	0x00,0x00
	;; P5DDR
	.byte	0x00,0x00
	;; P6DDR
	.byte	0x00,0x00
	;; dummy
	.byte	0x00,0x00
	;; P8DDR
	.byte	0x00,0x00
	;; P9DDR
	.byte	0x00,0x00
	;; PADDR
	.byte	0x00,0x00
	;; PBDDR
	.byte	0x00,0x00

__target_name:	
	.asciz	"generic"
