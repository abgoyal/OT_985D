
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/assembler.h>
#include <asm/hwcap.h>
#include <asm/pgtable-hwdef.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/ptrace.h>
#include "proc-macros.S"

#define CACHE_DLINESIZE	32

#define CACHE_DSEGMENTS	8

#define CACHE_DENTRIES	64

#define CACHE_DLIMIT	65536


	.text
ENTRY(cpu_arm920_proc_init)
	mov	pc, lr

ENTRY(cpu_arm920_proc_fin)
	stmfd	sp!, {lr}
	mov	ip, #PSR_F_BIT | PSR_I_BIT | SVC_MODE
	msr	cpsr_c, ip
#ifndef CONFIG_CPU_DCACHE_WRITETHROUGH
	bl	arm920_flush_kern_cache_all
#else
	bl	v4wt_flush_kern_cache_all
#endif
	mrc	p15, 0, r0, c1, c0, 0		@ ctrl register
	bic	r0, r0, #0x1000			@ ...i............
	bic	r0, r0, #0x000e			@ ............wca.
	mcr	p15, 0, r0, c1, c0, 0		@ disable caches
	ldmfd	sp!, {pc}

	.align	5
ENTRY(cpu_arm920_reset)
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

	.align	5
ENTRY(cpu_arm920_do_idle)
	mcr	p15, 0, r0, c7, c0, 4		@ Wait for interrupt
	mov	pc, lr


#ifndef CONFIG_CPU_DCACHE_WRITETHROUGH

ENTRY(arm920_flush_user_cache_all)
	/* FALLTHROUGH */

ENTRY(arm920_flush_kern_cache_all)
	mov	r2, #VM_EXEC
	mov	ip, #0
__flush_whole_cache:
	mov	r1, #(CACHE_DSEGMENTS - 1) << 5	@ 8 segments
1:	orr	r3, r1, #(CACHE_DENTRIES - 1) << 26 @ 64 entries
2:	mcr	p15, 0, r3, c7, c14, 2		@ clean+invalidate D index
	subs	r3, r3, #1 << 26
	bcs	2b				@ entries 63 to 0
	subs	r1, r1, #1 << 5
	bcs	1b				@ segments 7 to 0
	tst	r2, #VM_EXEC
	mcrne	p15, 0, ip, c7, c5, 0		@ invalidate I cache
	mcrne	p15, 0, ip, c7, c10, 4		@ drain WB
	mov	pc, lr

ENTRY(arm920_flush_user_cache_range)
	mov	ip, #0
	sub	r3, r1, r0			@ calculate total size
	cmp	r3, #CACHE_DLIMIT
	bhs	__flush_whole_cache

1:	mcr	p15, 0, r0, c7, c14, 1		@ clean+invalidate D entry
	tst	r2, #VM_EXEC
	mcrne	p15, 0, r0, c7, c5, 1		@ invalidate I entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	tst	r2, #VM_EXEC
	mcrne	p15, 0, ip, c7, c10, 4		@ drain WB
	mov	pc, lr

ENTRY(arm920_coherent_kern_range)
	/* FALLTHROUGH */

ENTRY(arm920_coherent_user_range)
	bic	r0, r0, #CACHE_DLINESIZE - 1
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	mcr	p15, 0, r0, c7, c5, 1		@ invalidate I entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

ENTRY(arm920_flush_kern_dcache_area)
	add	r1, r0, r1
1:	mcr	p15, 0, r0, c7, c14, 1		@ clean+invalidate D entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		@ invalidate I cache
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

arm920_dma_inv_range:
	tst	r0, #CACHE_DLINESIZE - 1
	bic	r0, r0, #CACHE_DLINESIZE - 1
	mcrne	p15, 0, r0, c7, c10, 1		@ clean D entry
	tst	r1, #CACHE_DLINESIZE - 1
	mcrne	p15, 0, r1, c7, c10, 1		@ clean D entry
1:	mcr	p15, 0, r0, c7, c6, 1		@ invalidate D entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

arm920_dma_clean_range:
	bic	r0, r0, #CACHE_DLINESIZE - 1
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

ENTRY(arm920_dma_flush_range)
	bic	r0, r0, #CACHE_DLINESIZE - 1
1:	mcr	p15, 0, r0, c7, c14, 1		@ clean+invalidate D entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

ENTRY(arm920_dma_map_area)
	add	r1, r1, r0
	cmp	r2, #DMA_TO_DEVICE
	beq	arm920_dma_clean_range
	bcs	arm920_dma_inv_range
	b	arm920_dma_flush_range
ENDPROC(arm920_dma_map_area)

ENTRY(arm920_dma_unmap_area)
	mov	pc, lr
ENDPROC(arm920_dma_unmap_area)

ENTRY(arm920_cache_fns)
	.long	arm920_flush_kern_cache_all
	.long	arm920_flush_user_cache_all
	.long	arm920_flush_user_cache_range
	.long	arm920_coherent_kern_range
	.long	arm920_coherent_user_range
	.long	arm920_flush_kern_dcache_area
	.long	arm920_dma_map_area
	.long	arm920_dma_unmap_area
	.long	arm920_dma_flush_range

#endif


ENTRY(cpu_arm920_dcache_clean_area)
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	add	r0, r0, #CACHE_DLINESIZE
	subs	r1, r1, #CACHE_DLINESIZE
	bhi	1b
	mov	pc, lr

/* =============================== PageTable ============================== */

	.align	5
ENTRY(cpu_arm920_switch_mm)
#ifdef CONFIG_MMU
	mov	ip, #0
#ifdef CONFIG_CPU_DCACHE_WRITETHROUGH
	mcr	p15, 0, ip, c7, c6, 0		@ invalidate D cache
#else
@ && 'Clean & Invalidate whole DCache'
@ && Re-written to use Index Ops.
@ && Uses registers r1, r3 and ip

	mov	r1, #(CACHE_DSEGMENTS - 1) << 5	@ 8 segments
1:	orr	r3, r1, #(CACHE_DENTRIES - 1) << 26 @ 64 entries
2:	mcr	p15, 0, r3, c7, c14, 2		@ clean & invalidate D index
	subs	r3, r3, #1 << 26
	bcs	2b				@ entries 63 to 0
	subs	r1, r1, #1 << 5
	bcs	1b				@ segments 7 to 0
#endif
	mcr	p15, 0, ip, c7, c5, 0		@ invalidate I cache
	mcr	p15, 0, ip, c7, c10, 4		@ drain WB
	mcr	p15, 0, r0, c2, c0, 0		@ load page table pointer
	mcr	p15, 0, ip, c8, c7, 0		@ invalidate I & D TLBs
#endif
	mov	pc, lr

	.align	5
ENTRY(cpu_arm920_set_pte_ext)
#ifdef CONFIG_MMU
	armv3_set_pte_ext
	mov	r0, r0
	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
#endif
	mov	pc, lr

	__INIT

	.type	__arm920_setup, #function
__arm920_setup:
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7		@ invalidate I,D caches on v4
	mcr	p15, 0, r0, c7, c10, 4		@ drain write buffer on v4
#ifdef CONFIG_MMU
	mcr	p15, 0, r0, c8, c7		@ invalidate I,D TLBs on v4
#endif
	adr	r5, arm920_crval
	ldmia	r5, {r5, r6}
	mrc	p15, 0, r0, c1, c0		@ get control register v4
	bic	r0, r0, r5
	orr	r0, r0, r6
	mov	pc, lr
	.size	__arm920_setup, . - __arm920_setup

	/*
	 *  R
	 * .RVI ZFRS BLDP WCAM
	 * ..11 0001 ..11 0101
	 * 
	 */
	.type	arm920_crval, #object
arm920_crval:
	crval	clear=0x00003f3f, mmuset=0x00003135, ucset=0x00001130

	__INITDATA

	.type	arm920_processor_functions, #object
arm920_processor_functions:
	.word	v4t_early_abort
	.word	legacy_pabort
	.word	cpu_arm920_proc_init
	.word	cpu_arm920_proc_fin
	.word	cpu_arm920_reset
	.word   cpu_arm920_do_idle
	.word	cpu_arm920_dcache_clean_area
	.word	cpu_arm920_switch_mm
	.word	cpu_arm920_set_pte_ext
	.size	arm920_processor_functions, . - arm920_processor_functions

	.section ".rodata"

	.type	cpu_arch_name, #object
cpu_arch_name:
	.asciz	"armv4t"
	.size	cpu_arch_name, . - cpu_arch_name

	.type	cpu_elf_name, #object
cpu_elf_name:
	.asciz	"v4"
	.size	cpu_elf_name, . - cpu_elf_name

	.type	cpu_arm920_name, #object
cpu_arm920_name:
	.asciz	"ARM920T"
	.size	cpu_arm920_name, . - cpu_arm920_name

	.align

	.section ".proc.info.init", #alloc, #execinstr

	.type	__arm920_proc_info,#object
__arm920_proc_info:
	.long	0x41009200
	.long	0xff00fff0
	.long   PMD_TYPE_SECT | \
		PMD_SECT_BUFFERABLE | \
		PMD_SECT_CACHEABLE | \
		PMD_BIT4 | \
		PMD_SECT_AP_WRITE | \
		PMD_SECT_AP_READ
	.long   PMD_TYPE_SECT | \
		PMD_BIT4 | \
		PMD_SECT_AP_WRITE | \
		PMD_SECT_AP_READ
	b	__arm920_setup
	.long	cpu_arch_name
	.long	cpu_elf_name
	.long	HWCAP_SWP | HWCAP_HALF | HWCAP_THUMB
	.long	cpu_arm920_name
	.long	arm920_processor_functions
	.long	v4wbi_tlb_fns
	.long	v4wb_user_fns
#ifndef CONFIG_CPU_DCACHE_WRITETHROUGH
	.long	arm920_cache_fns
#else
	.long	v4wt_cache_fns
#endif
	.size	__arm920_proc_info, . - __arm920_proc_info
