

#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/assembler.h>
#include <asm/hwcap.h>
#include <asm/pgtable-hwdef.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/ptrace.h>
#include "proc-macros.S"

#define CACHE_DLINESIZE	16

#define CACHE_DSEGMENTS	2

#define CACHE_DENTRIES	256

#define CACHE_DLIMIT	8192

	.text
ENTRY(cpu_arm925_proc_init)
	mov	pc, lr

ENTRY(cpu_arm925_proc_fin)
	stmfd	sp!, {lr}
	mov	ip, #PSR_F_BIT | PSR_I_BIT | SVC_MODE
	msr	cpsr_c, ip
	bl	arm925_flush_kern_cache_all
	mrc	p15, 0, r0, c1, c0, 0		@ ctrl register
	bic	r0, r0, #0x1000			@ ...i............
	bic	r0, r0, #0x000e			@ ............wca.
	mcr	p15, 0, r0, c1, c0, 0		@ disable caches
	ldmfd	sp!, {pc}

	.align	5
ENTRY(cpu_arm925_reset)
	/* Send software reset to MPU and DSP */
	mov	ip, #0xff000000
	orr	ip, ip, #0x00fe0000
	orr	ip, ip, #0x0000ce00
	mov	r4, #1
	strh	r4, [ip, #0x10]

	mov	ip, #0
	mcr	p15, 0, ip, c7, c7, 0		@ invalidate I,D caches
	mcr	p15, 0, ip, c7, c10, 4		@ drain WB
#ifdef CONFIG_MMU
	mcr	p15, 0, ip, c8, c7, 0		@ invalidate I & D TLBs
#endif
	mrc	p15, 0, ip, c1, c0, 0		@ ctrl register
	bic	ip, ip, #0x000f			@ ............wcam
	bic	ip, ip, #0x1100			@ ...i...s........
	mcr	p15, 0, ip, c1, c0, 0		@ ctrl register
	mov	pc, r0

	.align	10
ENTRY(cpu_arm925_do_idle)
	mov	r0, #0
	mrc	p15, 0, r1, c1, c0, 0		@ Read control register
	mcr	p15, 0, r0, c7, c10, 4		@ Drain write buffer
	bic	r2, r1, #1 << 12
	mcr	p15, 0, r2, c1, c0, 0		@ Disable I cache
	mcr	p15, 0, r0, c7, c0, 4		@ Wait for interrupt
	mcr	p15, 0, r1, c1, c0, 0		@ Restore ICache enable
	mov	pc, lr

ENTRY(arm925_flush_user_cache_all)
	/* FALLTHROUGH */

ENTRY(arm925_flush_kern_cache_all)
	mov	r2, #VM_EXEC
	mov	ip, #0
__flush_whole_cache:
#ifdef CONFIG_CPU_DCACHE_WRITETHROUGH
	mcr	p15, 0, ip, c7, c6, 0		@ invalidate D cache
#else
	/* Flush entries in both segments at once, see NOTE1 above */
	mov	r3, #(CACHE_DENTRIES - 1) << 4	@ 256 entries in segment
2:	mcr	p15, 0, r3, c7, c14, 2		@ clean+invalidate D index
	subs	r3, r3, #1 << 4
	bcs	2b				@ entries 255 to 0
#endif
	tst	r2, #VM_EXEC
	mcrne	p15, 0, ip, c7, c5, 0		@ invalidate I cache
	mcrne	p15, 0, ip, c7, c10, 4		@ drain WB
	mov	pc, lr

ENTRY(arm925_flush_user_cache_range)
	mov	ip, #0
	sub	r3, r1, r0			@ calculate total size
	cmp	r3, #CACHE_DLIMIT
	bgt	__flush_whole_cache
1:	tst	r2, #VM_EXEC
#ifdef CONFIG_CPU_DCACHE_WRITETHROUGH
	mcr	p15, 0, r0, c7, c6, 1		@ invalidate D entry
	mcrne	p15, 0, r0, c7, c5, 1		@ invalidate I entry
	add	r0, r0, #CACHE_DLINESIZE
	mcr	p15, 0, r0, c7, c6, 1		@ invalidate D entry
	mcrne	p15, 0, r0, c7, c5, 1		@ invalidate I entry
	add	r0, r0, #CACHE_DLINESIZE
#else
	mcr	p15, 0, r0, c7, c14, 1		@ clean and invalidate D entry
	mcrne	p15, 0, r0, c7, c5, 1		@ invalidate I entry
	add	r0, r0, #CACHE_DLINESIZE
	mcr	p15, 0, r0, c7, c14, 1		@ clean and invalidate D entry
	mcrne	p15, 0, r0, c7, c5, 1		@ invalidate I entry
	add	r0, r0, #CACHE_DLINESIZE
#endif
	cmp	r0, r1
	blo	1b
	tst	r2, #VM_EXEC
	mcrne	p15, 0, ip, c7, c10, 4		@ drain WB
	mov	pc, lr

ENTRY(arm925_coherent_kern_range)
	/* FALLTHROUGH */

ENTRY(arm925_coherent_user_range)
	bic	r0, r0, #CACHE_DLINESIZE - 1
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	mcr	p15, 0, r0, c7, c5, 1		@ invalidate I entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

ENTRY(arm925_flush_kern_dcache_area)
	add	r1, r0, r1
1:	mcr	p15, 0, r0, c7, c14, 1		@ clean+invalidate D entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		@ invalidate I cache
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

arm925_dma_inv_range:
#ifndef CONFIG_CPU_DCACHE_WRITETHROUGH
	tst	r0, #CACHE_DLINESIZE - 1
	mcrne	p15, 0, r0, c7, c10, 1		@ clean D entry
	tst	r1, #CACHE_DLINESIZE - 1
	mcrne	p15, 0, r1, c7, c10, 1		@ clean D entry
#endif
	bic	r0, r0, #CACHE_DLINESIZE - 1
1:	mcr	p15, 0, r0, c7, c6, 1		@ invalidate D entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

arm925_dma_clean_range:
#ifndef CONFIG_CPU_DCACHE_WRITETHROUGH
	bic	r0, r0, #CACHE_DLINESIZE - 1
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
#endif
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

ENTRY(arm925_dma_flush_range)
	bic	r0, r0, #CACHE_DLINESIZE - 1
1:
#ifndef CONFIG_CPU_DCACHE_WRITETHROUGH
	mcr	p15, 0, r0, c7, c14, 1		@ clean+invalidate D entry
#else
	mcr	p15, 0, r0, c7, c6, 1		@ invalidate D entry
#endif
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

ENTRY(arm925_dma_map_area)
	add	r1, r1, r0
	cmp	r2, #DMA_TO_DEVICE
	beq	arm925_dma_clean_range
	bcs	arm925_dma_inv_range
	b	arm925_dma_flush_range
ENDPROC(arm925_dma_map_area)

ENTRY(arm925_dma_unmap_area)
	mov	pc, lr
ENDPROC(arm925_dma_unmap_area)

ENTRY(arm925_cache_fns)
	.long	arm925_flush_kern_cache_all
	.long	arm925_flush_user_cache_all
	.long	arm925_flush_user_cache_range
	.long	arm925_coherent_kern_range
	.long	arm925_coherent_user_range
	.long	arm925_flush_kern_dcache_area
	.long	arm925_dma_map_area
	.long	arm925_dma_unmap_area
	.long	arm925_dma_flush_range

ENTRY(cpu_arm925_dcache_clean_area)
#ifndef CONFIG_CPU_DCACHE_WRITETHROUGH
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	add	r0, r0, #CACHE_DLINESIZE
	subs	r1, r1, #CACHE_DLINESIZE
	bhi	1b
#endif
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

/* =============================== PageTable ============================== */

	.align	5
ENTRY(cpu_arm925_switch_mm)
#ifdef CONFIG_MMU
	mov	ip, #0
#ifdef CONFIG_CPU_DCACHE_WRITETHROUGH
	mcr	p15, 0, ip, c7, c6, 0		@ invalidate D cache
#else
	/* Flush entries in bothe segments at once, see NOTE1 above */
	mov	r3, #(CACHE_DENTRIES - 1) << 4	@ 256 entries in segment
2:	mcr	p15, 0, r3, c7, c14, 2		@ clean & invalidate D index
	subs	r3, r3, #1 << 4
	bcs	2b				@ entries 255 to 0
#endif
	mcr	p15, 0, ip, c7, c5, 0		@ invalidate I cache
	mcr	p15, 0, ip, c7, c10, 4		@ drain WB
	mcr	p15, 0, r0, c2, c0, 0		@ load page table pointer
	mcr	p15, 0, ip, c8, c7, 0		@ invalidate I & D TLBs
#endif
	mov	pc, lr

	.align	5
ENTRY(cpu_arm925_set_pte_ext)
#ifdef CONFIG_MMU
	armv3_set_pte_ext
	mov	r0, r0
#ifndef CONFIG_CPU_DCACHE_WRITETHROUGH
	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
#endif
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
#endif /* CONFIG_MMU */
	mov	pc, lr

	__INIT

	.type	__arm925_setup, #function
__arm925_setup:
	mov	r0, #0
#if defined(CONFIG_CPU_ICACHE_STREAMING_DISABLE)
        orr     r0,r0,#1 << 7
#endif

	/* Transparent on, D-cache clean & flush mode. See  NOTE2 above */
        orr     r0,r0,#1 << 1			@ transparent mode on
        mcr     p15, 0, r0, c15, c1, 0          @ write TI config register

	mov	r0, #0
	mcr	p15, 0, r0, c7, c7		@ invalidate I,D caches on v4
	mcr	p15, 0, r0, c7, c10, 4		@ drain write buffer on v4
#ifdef CONFIG_MMU
	mcr	p15, 0, r0, c8, c7		@ invalidate I,D TLBs on v4
#endif

#ifdef CONFIG_CPU_DCACHE_WRITETHROUGH
	mov	r0, #4				@ disable write-back on caches explicitly
	mcr	p15, 7, r0, c15, c0, 0
#endif

	adr	r5, arm925_crval
	ldmia	r5, {r5, r6}
	mrc	p15, 0, r0, c1, c0		@ get control register v4
	bic	r0, r0, r5
	orr	r0, r0, r6
#ifdef CONFIG_CPU_CACHE_ROUND_ROBIN
	orr	r0, r0, #0x4000			@ .1.. .... .... ....
#endif
	mov	pc, lr
	.size	__arm925_setup, . - __arm925_setup

	/*
	 *  R
	 * .RVI ZFRS BLDP WCAM
	 * .011 0001 ..11 1101
	 * 
	 */
	.type	arm925_crval, #object
arm925_crval:
	crval	clear=0x00007f3f, mmuset=0x0000313d, ucset=0x00001130

	__INITDATA

	.type	arm925_processor_functions, #object
arm925_processor_functions:
	.word	v4t_early_abort
	.word	legacy_pabort
	.word	cpu_arm925_proc_init
	.word	cpu_arm925_proc_fin
	.word	cpu_arm925_reset
	.word   cpu_arm925_do_idle
	.word	cpu_arm925_dcache_clean_area
	.word	cpu_arm925_switch_mm
	.word	cpu_arm925_set_pte_ext
	.size	arm925_processor_functions, . - arm925_processor_functions

	.section ".rodata"

	.type	cpu_arch_name, #object
cpu_arch_name:
	.asciz	"armv4t"
	.size	cpu_arch_name, . - cpu_arch_name

	.type	cpu_elf_name, #object
cpu_elf_name:
	.asciz	"v4"
	.size	cpu_elf_name, . - cpu_elf_name

	.type	cpu_arm925_name, #object
cpu_arm925_name:
	.asciz	"ARM925T"
	.size	cpu_arm925_name, . - cpu_arm925_name

	.align

	.section ".proc.info.init", #alloc, #execinstr

	.type	__arm925_proc_info,#object
__arm925_proc_info:
	.long	0x54029250
	.long	0xfffffff0
	.long   PMD_TYPE_SECT | \
		PMD_BIT4 | \
		PMD_SECT_AP_WRITE | \
		PMD_SECT_AP_READ
	.long   PMD_TYPE_SECT | \
		PMD_BIT4 | \
		PMD_SECT_AP_WRITE | \
		PMD_SECT_AP_READ
	b	__arm925_setup
	.long	cpu_arch_name
	.long	cpu_elf_name
	.long	HWCAP_SWP | HWCAP_HALF | HWCAP_THUMB
	.long	cpu_arm925_name
	.long	arm925_processor_functions
	.long	v4wbi_tlb_fns
	.long	v4wb_user_fns
	.long	arm925_cache_fns
	.size	__arm925_proc_info, . - __arm925_proc_info

	.type	__arm915_proc_info,#object
__arm915_proc_info:
	.long	0x54029150
	.long	0xfffffff0
	.long   PMD_TYPE_SECT | \
		PMD_BIT4 | \
		PMD_SECT_AP_WRITE | \
		PMD_SECT_AP_READ
	.long   PMD_TYPE_SECT | \
		PMD_BIT4 | \
		PMD_SECT_AP_WRITE | \
		PMD_SECT_AP_READ
	b	__arm925_setup
	.long	cpu_arch_name
	.long	cpu_elf_name
	.long	HWCAP_SWP | HWCAP_HALF | HWCAP_THUMB
	.long	cpu_arm925_name
	.long	arm925_processor_functions
	.long	v4wbi_tlb_fns
	.long	v4wb_user_fns
	.long	arm925_cache_fns
	.size	__arm925_proc_info, . - __arm925_proc_info
