

#ifndef _ASM_MICROBLAZE_UNALIGNED_H
#define _ASM_MICROBLAZE_UNALIGNED_H

# ifdef __KERNEL__

# include <linux/unaligned/be_struct.h>
# include <linux/unaligned/le_byteshift.h>
# include <linux/unaligned/generic.h>

# define get_unaligned	__get_unaligned_be
# define put_unaligned	__put_unaligned_be

# endif	/* __KERNEL__ */
#endif /* _ASM_MICROBLAZE_UNALIGNED_H */
