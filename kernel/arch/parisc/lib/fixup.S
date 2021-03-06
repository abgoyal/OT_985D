
#include <asm/asm-offsets.h>
#include <asm/assembly.h>
#include <asm/errno.h>
#include <linux/linkage.h>

#ifdef CONFIG_SMP
	.macro  get_fault_ip t1 t2
	addil LT%__per_cpu_offset,%r27
	LDREG RT%__per_cpu_offset(%r1),\t1
	/* t2 = smp_processor_id() */
	mfctl 30,\t2
	ldw TI_CPU(\t2),\t2
#ifdef CONFIG_64BIT
	extrd,u \t2,63,32,\t2
#endif
	/* t2 = &__per_cpu_offset[smp_processor_id()]; */
	LDREGX \t2(\t1),\t2 
	addil LT%exception_data,%r27
	LDREG RT%exception_data(%r1),\t1
	/* t1 = &__get_cpu_var(exception_data) */
	add,l \t1,\t2,\t1
	/* t1 = t1->fault_ip */
	LDREG EXCDATA_IP(\t1), \t1
	.endm
#else
	.macro  get_fault_ip t1 t2
	/* t1 = &__get_cpu_var(exception_data) */
	addil LT%exception_data,%r27
	LDREG RT%exception_data(%r1),\t2
	/* t1 = t2->fault_ip */
	LDREG EXCDATA_IP(\t2), \t1
	.endm
#endif

	.level LEVEL

	.text
	.section .fixup, "ax"

	/* get_user() fixups, store -EFAULT in r8, and 0 in r9 */
ENTRY(fixup_get_user_skip_1)
	get_fault_ip %r1,%r8
	ldo 4(%r1), %r1
	ldi -EFAULT, %r8
	bv %r0(%r1)
	copy %r0, %r9
ENDPROC(fixup_get_user_skip_1)

ENTRY(fixup_get_user_skip_2)
	get_fault_ip %r1,%r8
	ldo 8(%r1), %r1
	ldi -EFAULT, %r8
	bv %r0(%r1)
	copy %r0, %r9
ENDPROC(fixup_get_user_skip_2)

	/* put_user() fixups, store -EFAULT in r8 */
ENTRY(fixup_put_user_skip_1)
	get_fault_ip %r1,%r8
	ldo 4(%r1), %r1
	bv %r0(%r1)
	ldi -EFAULT, %r8
ENDPROC(fixup_put_user_skip_1)

ENTRY(fixup_put_user_skip_2)
	get_fault_ip %r1,%r8
	ldo 8(%r1), %r1
	bv %r0(%r1)
	ldi -EFAULT, %r8
ENDPROC(fixup_put_user_skip_2)

