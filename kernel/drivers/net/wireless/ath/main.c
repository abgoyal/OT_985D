

#include <linux/kernel.h>
#include <linux/module.h>

#include "ath.h"

MODULE_AUTHOR("Atheros Communications");
MODULE_DESCRIPTION("Shared library for Atheros wireless LAN cards.");
MODULE_LICENSE("Dual BSD/GPL");

struct sk_buff *ath_rxbuf_alloc(struct ath_common *common,
				u32 len,
				gfp_t gfp_mask)
{
	struct sk_buff *skb;
	u32 off;

	/*
	 * Cache-line-align.  This is important (for the
	 * 5210 at least) as not doing so causes bogus data
	 * in rx'd frames.
	 */

	/* Note: the kernel can allocate a value greater than
	 * what we ask it to give us. We really only need 4 KB as that
	 * is this hardware supports and in fact we need at least 3849
	 * as that is the MAX AMSDU size this hardware supports.
	 * Unfortunately this means we may get 8 KB here from the
	 * kernel... and that is actually what is observed on some
	 * systems :( */
	skb = __dev_alloc_skb(len + common->cachelsz - 1, gfp_mask);
	if (skb != NULL) {
		off = ((unsigned long) skb->data) % common->cachelsz;
		if (off != 0)
			skb_reserve(skb, common->cachelsz - off);
	} else {
		printk(KERN_ERR "skbuff alloc of size %u failed\n", len);
		return NULL;
	}

	return skb;
}
EXPORT_SYMBOL(ath_rxbuf_alloc);
