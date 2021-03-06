

#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/compiler.h>
#include <linux/module.h>

#include <linux/string.h>
#include <asm/system.h>

#ifdef __HAVE_ARCH_MEMCPY
void *memcpy(void *v_dst, const void *v_src, __kernel_size_t c)
{
	const char *src = v_src;
	char *dst = v_dst;
#ifndef CONFIG_OPT_LIB_FUNCTION
	/* Simple, byte oriented memcpy. */
	while (c--)
		*dst++ = *src++;

	return v_dst;
#else
	/* The following code tries to optimize the copy by using unsigned
	 * alignment. This will work fine if both source and destination are
	 * aligned on the same boundary. However, if they are aligned on
	 * different boundaries shifts will be necessary. This might result in
	 * bad performance on MicroBlaze systems without a barrel shifter.
	 */
	const uint32_t *i_src;
	uint32_t *i_dst;

	if (likely(c >= 4)) {
		unsigned  value, buf_hold;

		/* Align the dstination to a word boundry. */
		/* This is done in an endian independant manner. */
		switch ((unsigned long)dst & 3) {
		case 1:
			*dst++ = *src++;
			--c;
		case 2:
			*dst++ = *src++;
			--c;
		case 3:
			*dst++ = *src++;
			--c;
		}

		i_dst = (void *)dst;

		/* Choose a copy scheme based on the source */
		/* alignment relative to dstination. */
		switch ((unsigned long)src & 3) {
		case 0x0:	/* Both byte offsets are aligned */
			i_src  = (const void *)src;

			for (; c >= 4; c -= 4)
				*i_dst++ = *i_src++;

			src  = (const void *)i_src;
			break;
		case 0x1:	/* Unaligned - Off by 1 */
			/* Word align the source */
			i_src = (const void *) ((unsigned)src & ~3);

			/* Load the holding buffer */
			buf_hold = *i_src++ << 8;

			for (; c >= 4; c -= 4) {
				value = *i_src++;
				*i_dst++ = buf_hold | value >> 24;
				buf_hold = value << 8;
			}

			/* Realign the source */
			src = (const void *)i_src;
			src -= 3;
			break;
		case 0x2:	/* Unaligned - Off by 2 */
			/* Word align the source */
			i_src = (const void *) ((unsigned)src & ~3);

			/* Load the holding buffer */
			buf_hold = *i_src++ << 16;

			for (; c >= 4; c -= 4) {
				value = *i_src++;
				*i_dst++ = buf_hold | value >> 16;
				buf_hold = value << 16;
			}

			/* Realign the source */
			src = (const void *)i_src;
			src -= 2;
			break;
		case 0x3:	/* Unaligned - Off by 3 */
			/* Word align the source */
			i_src = (const void *) ((unsigned)src & ~3);

			/* Load the holding buffer */
			buf_hold = *i_src++ << 24;

			for (; c >= 4; c -= 4) {
				value = *i_src++;
				*i_dst++ = buf_hold | value >> 8;
				buf_hold = value << 24;
			}

			/* Realign the source */
			src = (const void *)i_src;
			src -= 1;
			break;
		}
		dst = (void *)i_dst;
	}

	/* Finish off any remaining bytes */
	/* simple fast copy, ... unless a cache boundry is crossed */
	switch (c) {
	case 3:
		*dst++ = *src++;
	case 2:
		*dst++ = *src++;
	case 1:
		*dst++ = *src++;
	}

	return v_dst;
#endif
}
EXPORT_SYMBOL(memcpy);
#endif /* __HAVE_ARCH_MEMCPY */
