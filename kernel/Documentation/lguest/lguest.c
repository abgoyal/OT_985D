
#define _LARGEFILE64_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <stdint.h>
#include <stdlib.h>
#include <elf.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/eventfd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <linux/if_tun.h>
#include <sys/uio.h>
#include <termios.h>
#include <getopt.h>
#include <assert.h>
#include <sched.h>
#include <limits.h>
#include <stddef.h>
#include <signal.h>
#include "linux/lguest_launcher.h"
#include "linux/virtio_config.h"
#include "linux/virtio_net.h"
#include "linux/virtio_blk.h"
#include "linux/virtio_console.h"
#include "linux/virtio_rng.h"
#include "linux/virtio_ring.h"
#include "asm/bootparam.h"
typedef unsigned long long u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
/*:*/

#define PAGE_PRESENT 0x7 	/* Present, RW, Execute */
#define BRIDGE_PFX "bridge:"
#ifndef SIOCBRADDIF
#define SIOCBRADDIF	0x89a2		/* add interface to bridge      */
#endif
/* We can have up to 256 pages for devices. */
#define DEVICE_PAGES 256
/* This will occupy 3 pages: it must be a power of 2. */
#define VIRTQUEUE_NUM 256

static bool verbose;
#define verbose(args...) \
	do { if (verbose) printf(args); } while(0)
/*:*/

/* The pointer to the start of guest memory. */
static void *guest_base;
/* The maximum guest physical address allowed, and maximum possible. */
static unsigned long guest_limit, guest_max;
/* The /dev/lguest file descriptor. */
static int lguest_fd;

/* a per-cpu variable indicating whose vcpu is currently running */
static unsigned int __thread cpu_id;

/* This is our list of devices. */
struct device_list {
	/* Counter to assign interrupt numbers. */
	unsigned int next_irq;

	/* Counter to print out convenient device numbers. */
	unsigned int device_num;

	/* The descriptor page for the devices. */
	u8 *descpage;

	/* A single linked list of devices. */
	struct device *dev;
	/* And a pointer to the last device for easy append. */
	struct device *lastdev;
};

/* The list of Guest devices, based on command line arguments. */
static struct device_list devices;

/* The device structure describes a single device. */
struct device {
	/* The linked-list pointer. */
	struct device *next;

	/* The device's descriptor, as mapped into the Guest. */
	struct lguest_device_desc *desc;

	/* We can't trust desc values once Guest has booted: we use these. */
	unsigned int feature_len;
	unsigned int num_vq;

	/* The name of this device, for --verbose. */
	const char *name;

	/* Any queues attached to this device */
	struct virtqueue *vq;

	/* Is it operational */
	bool running;

	/* Does Guest want an intrrupt on empty? */
	bool irq_on_empty;

	/* Device-specific data. */
	void *priv;
};

/* The virtqueue structure describes a queue attached to a device. */
struct virtqueue {
	struct virtqueue *next;

	/* Which device owns me. */
	struct device *dev;

	/* The configuration for this queue. */
	struct lguest_vqconfig config;

	/* The actual ring of buffers. */
	struct vring vring;

	/* Last available index we saw. */
	u16 last_avail_idx;

	/* How many are used since we sent last irq? */
	unsigned int pending_used;

	/* Eventfd where Guest notifications arrive. */
	int eventfd;

	/* Function for the thread which is servicing this virtqueue. */
	void (*service)(struct virtqueue *vq);
	pid_t thread;
};

/* Remember the arguments to the program so we can "reboot" */
static char **main_args;

/* The original tty settings to restore on exit. */
static struct termios orig_term;

#define wmb() __asm__ __volatile__("" : : : "memory")
#define mb() __asm__ __volatile__("" : : : "memory")

#define convert(iov, type) \
	((type *)_convert((iov), sizeof(type), __alignof__(type), #type))

static void *_convert(struct iovec *iov, size_t size, size_t align,
		      const char *name)
{
	if (iov->iov_len != size)
		errx(1, "Bad iovec size %zu for %s", iov->iov_len, name);
	if ((unsigned long)iov->iov_base % align != 0)
		errx(1, "Bad alignment %p for %s", iov->iov_base, name);
	return iov->iov_base;
}

/* Wrapper for the last available index.  Makes it easier to change. */
#define lg_last_avail(vq)	((vq)->last_avail_idx)

#define cpu_to_le16(v16) (v16)
#define cpu_to_le32(v32) (v32)
#define cpu_to_le64(v64) (v64)
#define le16_to_cpu(v16) (v16)
#define le32_to_cpu(v32) (v32)
#define le64_to_cpu(v64) (v64)

/* Is this iovec empty? */
static bool iov_empty(const struct iovec iov[], unsigned int num_iov)
{
	unsigned int i;

	for (i = 0; i < num_iov; i++)
		if (iov[i].iov_len)
			return false;
	return true;
}

/* Take len bytes from the front of this iovec. */
static void iov_consume(struct iovec iov[], unsigned num_iov, unsigned len)
{
	unsigned int i;

	for (i = 0; i < num_iov; i++) {
		unsigned int used;

		used = iov[i].iov_len < len ? iov[i].iov_len : len;
		iov[i].iov_base += used;
		iov[i].iov_len -= used;
		len -= used;
	}
	assert(len == 0);
}

/* The device virtqueue descriptors are followed by feature bitmasks. */
static u8 *get_feature_bits(struct device *dev)
{
	return (u8 *)(dev->desc + 1)
		+ dev->num_vq * sizeof(struct lguest_vqconfig);
}

static void *from_guest_phys(unsigned long addr)
{
	return guest_base + addr;
}

static unsigned long to_guest_phys(const void *addr)
{
	return (addr - guest_base);
}

static int open_or_die(const char *name, int flags)
{
	int fd = open(name, flags);
	if (fd < 0)
		err(1, "Failed to open %s", name);
	return fd;
}

/* map_zeroed_pages() takes a number of pages. */
static void *map_zeroed_pages(unsigned int num)
{
	int fd = open_or_die("/dev/zero", O_RDONLY);
	void *addr;

	/*
	 * We use a private mapping (ie. if we write to the page, it will be
	 * copied).
	 */
	addr = mmap(NULL, getpagesize() * num,
		    PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE, fd, 0);
	if (addr == MAP_FAILED)
		err(1, "Mmapping %u pages of /dev/zero", num);

	/*
	 * One neat mmap feature is that you can close the fd, and it
	 * stays mapped.
	 */
	close(fd);

	return addr;
}

/* Get some more pages for a device. */
static void *get_pages(unsigned int num)
{
	void *addr = from_guest_phys(guest_limit);

	guest_limit += num * getpagesize();
	if (guest_limit > guest_max)
		errx(1, "Not enough memory for devices");
	return addr;
}

static void map_at(int fd, void *addr, unsigned long offset, unsigned long len)
{
	ssize_t r;

	/*
	 * We map writable even though for some segments are marked read-only.
	 * The kernel really wants to be writable: it patches its own
	 * instructions.
	 *
	 * MAP_PRIVATE means that the page won't be copied until a write is
	 * done to it.  This allows us to share untouched memory between
	 * Guests.
	 */
	if (mmap(addr, len, PROT_READ|PROT_WRITE|PROT_EXEC,
		 MAP_FIXED|MAP_PRIVATE, fd, offset) != MAP_FAILED)
		return;

	/* pread does a seek and a read in one shot: saves a few lines. */
	r = pread(fd, addr, len, offset);
	if (r != len)
		err(1, "Reading offset %lu len %lu gave %zi", offset, len, r);
}

static unsigned long map_elf(int elf_fd, const Elf32_Ehdr *ehdr)
{
	Elf32_Phdr phdr[ehdr->e_phnum];
	unsigned int i;

	/*
	 * Sanity checks on the main ELF header: an x86 executable with a
	 * reasonable number of correctly-sized program headers.
	 */
	if (ehdr->e_type != ET_EXEC
	    || ehdr->e_machine != EM_386
	    || ehdr->e_phentsize != sizeof(Elf32_Phdr)
	    || ehdr->e_phnum < 1 || ehdr->e_phnum > 65536U/sizeof(Elf32_Phdr))
		errx(1, "Malformed elf header");

	/*
	 * An ELF executable contains an ELF header and a number of "program"
	 * headers which indicate which parts ("segments") of the program to
	 * load where.
	 */

	/* We read in all the program headers at once: */
	if (lseek(elf_fd, ehdr->e_phoff, SEEK_SET) < 0)
		err(1, "Seeking to program headers");
	if (read(elf_fd, phdr, sizeof(phdr)) != sizeof(phdr))
		err(1, "Reading program headers");

	/*
	 * Try all the headers: there are usually only three.  A read-only one,
	 * a read-write one, and a "note" section which we don't load.
	 */
	for (i = 0; i < ehdr->e_phnum; i++) {
		/* If this isn't a loadable segment, we ignore it */
		if (phdr[i].p_type != PT_LOAD)
			continue;

		verbose("Section %i: size %i addr %p\n",
			i, phdr[i].p_memsz, (void *)phdr[i].p_paddr);

		/* We map this section of the file at its physical address. */
		map_at(elf_fd, from_guest_phys(phdr[i].p_paddr),
		       phdr[i].p_offset, phdr[i].p_filesz);
	}

	/* The entry point is given in the ELF header. */
	return ehdr->e_entry;
}

static unsigned long load_bzimage(int fd)
{
	struct boot_params boot;
	int r;
	/* Modern bzImages get loaded at 1M. */
	void *p = from_guest_phys(0x100000);

	/*
	 * Go back to the start of the file and read the header.  It should be
	 * a Linux boot header (see Documentation/x86/i386/boot.txt)
	 */
	lseek(fd, 0, SEEK_SET);
	read(fd, &boot, sizeof(boot));

	/* Inside the setup_hdr, we expect the magic "HdrS" */
	if (memcmp(&boot.hdr.header, "HdrS", 4) != 0)
		errx(1, "This doesn't look like a bzImage to me");

	/* Skip over the extra sectors of the header. */
	lseek(fd, (boot.hdr.setup_sects+1) * 512, SEEK_SET);

	/* Now read everything into memory. in nice big chunks. */
	while ((r = read(fd, p, 65536)) > 0)
		p += r;

	/* Finally, code32_start tells us where to enter the kernel. */
	return boot.hdr.code32_start;
}

static unsigned long load_kernel(int fd)
{
	Elf32_Ehdr hdr;

	/* Read in the first few bytes. */
	if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr))
		err(1, "Reading kernel");

	/* If it's an ELF file, it starts with "\177ELF" */
	if (memcmp(hdr.e_ident, ELFMAG, SELFMAG) == 0)
		return map_elf(fd, &hdr);

	/* Otherwise we assume it's a bzImage, and try to load it. */
	return load_bzimage(fd);
}

static inline unsigned long page_align(unsigned long addr)
{
	/* Add upwards and truncate downwards. */
	return ((addr + getpagesize()-1) & ~(getpagesize()-1));
}

static unsigned long load_initrd(const char *name, unsigned long mem)
{
	int ifd;
	struct stat st;
	unsigned long len;

	ifd = open_or_die(name, O_RDONLY);
	/* fstat() is needed to get the file size. */
	if (fstat(ifd, &st) < 0)
		err(1, "fstat() on initrd '%s'", name);

	/*
	 * We map the initrd at the top of memory, but mmap wants it to be
	 * page-aligned, so we round the size up for that.
	 */
	len = page_align(st.st_size);
	map_at(ifd, from_guest_phys(mem - len), 0, st.st_size);
	/*
	 * Once a file is mapped, you can close the file descriptor.  It's a
	 * little odd, but quite useful.
	 */
	close(ifd);
	verbose("mapped initrd %s size=%lu @ %p\n", name, len, (void*)mem-len);

	/* We return the initrd size. */
	return len;
}
/*:*/

static void concat(char *dst, char *args[])
{
	unsigned int i, len = 0;

	for (i = 0; args[i]; i++) {
		if (i) {
			strcat(dst+len, " ");
			len++;
		}
		strcpy(dst+len, args[i]);
		len += strlen(args[i]);
	}
	/* In case it's empty. */
	dst[len] = '\0';
}

static void tell_kernel(unsigned long start)
{
	unsigned long args[] = { LHREQ_INITIALIZE,
				 (unsigned long)guest_base,
				 guest_limit / getpagesize(), start };
	verbose("Guest: %p - %p (%#lx)\n",
		guest_base, guest_base + guest_limit, guest_limit);
	lguest_fd = open_or_die("/dev/lguest", O_RDWR);
	if (write(lguest_fd, args, sizeof(args)) < 0)
		err(1, "Writing to /dev/lguest");
}
/*:*/

static void *_check_pointer(unsigned long addr, unsigned int size,
			    unsigned int line)
{
	/*
	 * We have to separately check addr and addr+size, because size could
	 * be huge and addr + size might wrap around.
	 */
	if (addr >= guest_limit || addr + size >= guest_limit)
		errx(1, "%s:%i: Invalid address %#lx", __FILE__, line, addr);
	/*
	 * We return a pointer for the caller's convenience, now we know it's
	 * safe to use.
	 */
	return from_guest_phys(addr);
}
/* A macro which transparently hands the line number to the real function. */
#define check_pointer(addr,size) _check_pointer(addr, size, __LINE__)

static unsigned next_desc(struct vring_desc *desc,
			  unsigned int i, unsigned int max)
{
	unsigned int next;

	/* If this descriptor says it doesn't chain, we're done. */
	if (!(desc[i].flags & VRING_DESC_F_NEXT))
		return max;

	/* Check they're not leading us off end of descriptors. */
	next = desc[i].next;
	/* Make sure compiler knows to grab that: we don't want it changing! */
	wmb();

	if (next >= max)
		errx(1, "Desc next is %u", next);

	return next;
}

static void trigger_irq(struct virtqueue *vq)
{
	unsigned long buf[] = { LHREQ_IRQ, vq->config.irq };

	/* Don't inform them if nothing used. */
	if (!vq->pending_used)
		return;
	vq->pending_used = 0;

	/* If they don't want an interrupt, don't send one... */
	if (vq->vring.avail->flags & VRING_AVAIL_F_NO_INTERRUPT) {
		/* ... unless they've asked us to force one on empty. */
		if (!vq->dev->irq_on_empty
		    || lg_last_avail(vq) != vq->vring.avail->idx)
			return;
	}

	/* Send the Guest an interrupt tell them we used something up. */
	if (write(lguest_fd, buf, sizeof(buf)) != 0)
		err(1, "Triggering irq %i", vq->config.irq);
}

static unsigned wait_for_vq_desc(struct virtqueue *vq,
				 struct iovec iov[],
				 unsigned int *out_num, unsigned int *in_num)
{
	unsigned int i, head, max;
	struct vring_desc *desc;
	u16 last_avail = lg_last_avail(vq);

	/* There's nothing available? */
	while (last_avail == vq->vring.avail->idx) {
		u64 event;

		/*
		 * Since we're about to sleep, now is a good time to tell the
		 * Guest about what we've used up to now.
		 */
		trigger_irq(vq);

		/* OK, now we need to know about added descriptors. */
		vq->vring.used->flags &= ~VRING_USED_F_NO_NOTIFY;

		/*
		 * They could have slipped one in as we were doing that: make
		 * sure it's written, then check again.
		 */
		mb();
		if (last_avail != vq->vring.avail->idx) {
			vq->vring.used->flags |= VRING_USED_F_NO_NOTIFY;
			break;
		}

		/* Nothing new?  Wait for eventfd to tell us they refilled. */
		if (read(vq->eventfd, &event, sizeof(event)) != sizeof(event))
			errx(1, "Event read failed?");

		/* We don't need to be notified again. */
		vq->vring.used->flags |= VRING_USED_F_NO_NOTIFY;
	}

	/* Check it isn't doing very strange things with descriptor numbers. */
	if ((u16)(vq->vring.avail->idx - last_avail) > vq->vring.num)
		errx(1, "Guest moved used index from %u to %u",
		     last_avail, vq->vring.avail->idx);

	/*
	 * Grab the next descriptor number they're advertising, and increment
	 * the index we've seen.
	 */
	head = vq->vring.avail->ring[last_avail % vq->vring.num];
	lg_last_avail(vq)++;

	/* If their number is silly, that's a fatal mistake. */
	if (head >= vq->vring.num)
		errx(1, "Guest says index %u is available", head);

	/* When we start there are none of either input nor output. */
	*out_num = *in_num = 0;

	max = vq->vring.num;
	desc = vq->vring.desc;
	i = head;

	/*
	 * If this is an indirect entry, then this buffer contains a descriptor
	 * table which we handle as if it's any normal descriptor chain.
	 */
	if (desc[i].flags & VRING_DESC_F_INDIRECT) {
		if (desc[i].len % sizeof(struct vring_desc))
			errx(1, "Invalid size for indirect buffer table");

		max = desc[i].len / sizeof(struct vring_desc);
		desc = check_pointer(desc[i].addr, desc[i].len);
		i = 0;
	}

	do {
		/* Grab the first descriptor, and check it's OK. */
		iov[*out_num + *in_num].iov_len = desc[i].len;
		iov[*out_num + *in_num].iov_base
			= check_pointer(desc[i].addr, desc[i].len);
		/* If this is an input descriptor, increment that count. */
		if (desc[i].flags & VRING_DESC_F_WRITE)
			(*in_num)++;
		else {
			/*
			 * If it's an output descriptor, they're all supposed
			 * to come before any input descriptors.
			 */
			if (*in_num)
				errx(1, "Descriptor has out after in");
			(*out_num)++;
		}

		/* If we've got too many, that implies a descriptor loop. */
		if (*out_num + *in_num > max)
			errx(1, "Looped descriptor");
	} while ((i = next_desc(desc, i, max)) != max);

	return head;
}

static void add_used(struct virtqueue *vq, unsigned int head, int len)
{
	struct vring_used_elem *used;

	/*
	 * The virtqueue contains a ring of used buffers.  Get a pointer to the
	 * next entry in that used ring.
	 */
	used = &vq->vring.used->ring[vq->vring.used->idx % vq->vring.num];
	used->id = head;
	used->len = len;
	/* Make sure buffer is written before we update index. */
	wmb();
	vq->vring.used->idx++;
	vq->pending_used++;
}

/* And here's the combo meal deal.  Supersize me! */
static void add_used_and_trigger(struct virtqueue *vq, unsigned head, int len)
{
	add_used(vq, head, len);
	trigger_irq(vq);
}

struct console_abort {
	/* How many times have they hit ^C? */
	int count;
	/* When did they start? */
	struct timeval start;
};

/* This is the routine which handles console input (ie. stdin). */
static void console_input(struct virtqueue *vq)
{
	int len;
	unsigned int head, in_num, out_num;
	struct console_abort *abort = vq->dev->priv;
	struct iovec iov[vq->vring.num];

	/* Make sure there's a descriptor available. */
	head = wait_for_vq_desc(vq, iov, &out_num, &in_num);
	if (out_num)
		errx(1, "Output buffers in console in queue?");

	/* Read into it.  This is where we usually wait. */
	len = readv(STDIN_FILENO, iov, in_num);
	if (len <= 0) {
		/* Ran out of input? */
		warnx("Failed to get console input, ignoring console.");
		/*
		 * For simplicity, dying threads kill the whole Launcher.  So
		 * just nap here.
		 */
		for (;;)
			pause();
	}

	/* Tell the Guest we used a buffer. */
	add_used_and_trigger(vq, head, len);

	/*
	 * Three ^C within one second?  Exit.
	 *
	 * This is such a hack, but works surprisingly well.  Each ^C has to
	 * be in a buffer by itself, so they can't be too fast.  But we check
	 * that we get three within about a second, so they can't be too
	 * slow.
	 */
	if (len != 1 || ((char *)iov[0].iov_base)[0] != 3) {
		abort->count = 0;
		return;
	}

	abort->count++;
	if (abort->count == 1)
		gettimeofday(&abort->start, NULL);
	else if (abort->count == 3) {
		struct timeval now;
		gettimeofday(&now, NULL);
		/* Kill all Launcher processes with SIGINT, like normal ^C */
		if (now.tv_sec <= abort->start.tv_sec+1)
			kill(0, SIGINT);
		abort->count = 0;
	}
}

/* This is the routine which handles console output (ie. stdout). */
static void console_output(struct virtqueue *vq)
{
	unsigned int head, out, in;
	struct iovec iov[vq->vring.num];

	/* We usually wait in here, for the Guest to give us something. */
	head = wait_for_vq_desc(vq, iov, &out, &in);
	if (in)
		errx(1, "Input buffers in console output queue?");

	/* writev can return a partial write, so we loop here. */
	while (!iov_empty(iov, out)) {
		int len = writev(STDOUT_FILENO, iov, out);
		if (len <= 0)
			err(1, "Write to stdout gave %i", len);
		iov_consume(iov, out, len);
	}

	/*
	 * We're finished with that buffer: if we're going to sleep,
	 * wait_for_vq_desc() will prod the Guest with an interrupt.
	 */
	add_used(vq, head, 0);
}

struct net_info {
	int tunfd;
};

static void net_output(struct virtqueue *vq)
{
	struct net_info *net_info = vq->dev->priv;
	unsigned int head, out, in;
	struct iovec iov[vq->vring.num];

	/* We usually wait in here for the Guest to give us a packet. */
	head = wait_for_vq_desc(vq, iov, &out, &in);
	if (in)
		errx(1, "Input buffers in net output queue?");
	/*
	 * Send the whole thing through to /dev/net/tun.  It expects the exact
	 * same format: what a coincidence!
	 */
	if (writev(net_info->tunfd, iov, out) < 0)
		errx(1, "Write to tun failed?");

	/*
	 * Done with that one; wait_for_vq_desc() will send the interrupt if
	 * all packets are processed.
	 */
	add_used(vq, head, 0);
}

static bool will_block(int fd)
{
	fd_set fdset;
	struct timeval zero = { 0, 0 };
	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);
	return select(fd+1, &fdset, NULL, NULL, &zero) != 1;
}

static void net_input(struct virtqueue *vq)
{
	int len;
	unsigned int head, out, in;
	struct iovec iov[vq->vring.num];
	struct net_info *net_info = vq->dev->priv;

	/*
	 * Get a descriptor to write an incoming packet into.  This will also
	 * send an interrupt if they're out of descriptors.
	 */
	head = wait_for_vq_desc(vq, iov, &out, &in);
	if (out)
		errx(1, "Output buffers in net input queue?");

	/*
	 * If it looks like we'll block reading from the tun device, send them
	 * an interrupt.
	 */
	if (vq->pending_used && will_block(net_info->tunfd))
		trigger_irq(vq);

	/*
	 * Read in the packet.  This is where we normally wait (when there's no
	 * incoming network traffic).
	 */
	len = readv(net_info->tunfd, iov, in);
	if (len <= 0)
		err(1, "Failed to read from tun.");

	/*
	 * Mark that packet buffer as used, but don't interrupt here.  We want
	 * to wait until we've done as much work as we can.
	 */
	add_used(vq, head, len);
}
/*:*/

/* This is the helper to create threads: run the service routine in a loop. */
static int do_thread(void *_vq)
{
	struct virtqueue *vq = _vq;

	for (;;)
		vq->service(vq);
	return 0;
}

static void kill_launcher(int signal)
{
	kill(0, SIGTERM);
}

static void reset_device(struct device *dev)
{
	struct virtqueue *vq;

	verbose("Resetting device %s\n", dev->name);

	/* Clear any features they've acked. */
	memset(get_feature_bits(dev) + dev->feature_len, 0, dev->feature_len);

	/* We're going to be explicitly killing threads, so ignore them. */
	signal(SIGCHLD, SIG_IGN);

	/* Zero out the virtqueues, get rid of their threads */
	for (vq = dev->vq; vq; vq = vq->next) {
		if (vq->thread != (pid_t)-1) {
			kill(vq->thread, SIGTERM);
			waitpid(vq->thread, NULL, 0);
			vq->thread = (pid_t)-1;
		}
		memset(vq->vring.desc, 0,
		       vring_size(vq->config.num, LGUEST_VRING_ALIGN));
		lg_last_avail(vq) = 0;
	}
	dev->running = false;

	/* Now we care if threads die. */
	signal(SIGCHLD, (void *)kill_launcher);
}

static void create_thread(struct virtqueue *vq)
{
	/*
	 * Create stack for thread.  Since the stack grows upwards, we point
	 * the stack pointer to the end of this region.
	 */
	char *stack = malloc(32768);
	unsigned long args[] = { LHREQ_EVENTFD,
				 vq->config.pfn*getpagesize(), 0 };

	/* Create a zero-initialized eventfd. */
	vq->eventfd = eventfd(0, 0);
	if (vq->eventfd < 0)
		err(1, "Creating eventfd");
	args[2] = vq->eventfd;

	/*
	 * Attach an eventfd to this virtqueue: it will go off when the Guest
	 * does an LHCALL_NOTIFY for this vq.
	 */
	if (write(lguest_fd, &args, sizeof(args)) != 0)
		err(1, "Attaching eventfd");

	/*
	 * CLONE_VM: because it has to access the Guest memory, and SIGCHLD so
	 * we get a signal if it dies.
	 */
	vq->thread = clone(do_thread, stack + 32768, CLONE_VM | SIGCHLD, vq);
	if (vq->thread == (pid_t)-1)
		err(1, "Creating clone");

	/* We close our local copy now the child has it. */
	close(vq->eventfd);
}

static bool accepted_feature(struct device *dev, unsigned int bit)
{
	const u8 *features = get_feature_bits(dev) + dev->feature_len;

	if (dev->feature_len < bit / CHAR_BIT)
		return false;
	return features[bit / CHAR_BIT] & (1 << (bit % CHAR_BIT));
}

static void start_device(struct device *dev)
{
	unsigned int i;
	struct virtqueue *vq;

	verbose("Device %s OK: offered", dev->name);
	for (i = 0; i < dev->feature_len; i++)
		verbose(" %02x", get_feature_bits(dev)[i]);
	verbose(", accepted");
	for (i = 0; i < dev->feature_len; i++)
		verbose(" %02x", get_feature_bits(dev)
			[dev->feature_len+i]);

	dev->irq_on_empty = accepted_feature(dev, VIRTIO_F_NOTIFY_ON_EMPTY);

	for (vq = dev->vq; vq; vq = vq->next) {
		if (vq->service)
			create_thread(vq);
	}
	dev->running = true;
}

static void cleanup_devices(void)
{
	struct device *dev;

	for (dev = devices.dev; dev; dev = dev->next)
		reset_device(dev);

	/* If we saved off the original terminal settings, restore them now. */
	if (orig_term.c_lflag & (ISIG|ICANON|ECHO))
		tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
}

/* When the Guest tells us they updated the status field, we handle it. */
static void update_device_status(struct device *dev)
{
	/* A zero status is a reset, otherwise it's a set of flags. */
	if (dev->desc->status == 0)
		reset_device(dev);
	else if (dev->desc->status & VIRTIO_CONFIG_S_FAILED) {
		warnx("Device %s configuration FAILED", dev->name);
		if (dev->running)
			reset_device(dev);
	} else if (dev->desc->status & VIRTIO_CONFIG_S_DRIVER_OK) {
		if (!dev->running)
			start_device(dev);
	}
}

static void handle_output(unsigned long addr)
{
	struct device *i;

	/* Check each device. */
	for (i = devices.dev; i; i = i->next) {
		struct virtqueue *vq;

		/*
		 * Notifications to device descriptors mean they updated the
		 * device status.
		 */
		if (from_guest_phys(addr) == i->desc) {
			update_device_status(i);
			return;
		}

		/*
		 * Devices *can* be used before status is set to DRIVER_OK.
		 * The original plan was that they would never do this: they
		 * would always finish setting up their status bits before
		 * actually touching the virtqueues.  In practice, we allowed
		 * them to, and they do (eg. the disk probes for partition
		 * tables as part of initialization).
		 *
		 * If we see this, we start the device: once it's running, we
		 * expect the device to catch all the notifications.
		 */
		for (vq = i->vq; vq; vq = vq->next) {
			if (addr != vq->config.pfn*getpagesize())
				continue;
			if (i->running)
				errx(1, "Notification on running %s", i->name);
			/* This just calls create_thread() for each virtqueue */
			start_device(i);
			return;
		}
	}

	/*
	 * Early console write is done using notify on a nul-terminated string
	 * in Guest memory.  It's also great for hacking debugging messages
	 * into a Guest.
	 */
	if (addr >= guest_limit)
		errx(1, "Bad NOTIFY %#lx", addr);

	write(STDOUT_FILENO, from_guest_phys(addr),
	      strnlen(from_guest_phys(addr), guest_limit - addr));
}


static u8 *device_config(const struct device *dev)
{
	return (void *)(dev->desc + 1)
		+ dev->num_vq * sizeof(struct lguest_vqconfig)
		+ dev->feature_len * 2;
}

static struct lguest_device_desc *new_dev_desc(u16 type)
{
	struct lguest_device_desc d = { .type = type };
	void *p;

	/* Figure out where the next device config is, based on the last one. */
	if (devices.lastdev)
		p = device_config(devices.lastdev)
			+ devices.lastdev->desc->config_len;
	else
		p = devices.descpage;

	/* We only have one page for all the descriptors. */
	if (p + sizeof(d) > (void *)devices.descpage + getpagesize())
		errx(1, "Too many devices");

	/* p might not be aligned, so we memcpy in. */
	return memcpy(p, &d, sizeof(d));
}

static void add_virtqueue(struct device *dev, unsigned int num_descs,
			  void (*service)(struct virtqueue *))
{
	unsigned int pages;
	struct virtqueue **i, *vq = malloc(sizeof(*vq));
	void *p;

	/* First we need some memory for this virtqueue. */
	pages = (vring_size(num_descs, LGUEST_VRING_ALIGN) + getpagesize() - 1)
		/ getpagesize();
	p = get_pages(pages);

	/* Initialize the virtqueue */
	vq->next = NULL;
	vq->last_avail_idx = 0;
	vq->dev = dev;

	/*
	 * This is the routine the service thread will run, and its Process ID
	 * once it's running.
	 */
	vq->service = service;
	vq->thread = (pid_t)-1;

	/* Initialize the configuration. */
	vq->config.num = num_descs;
	vq->config.irq = devices.next_irq++;
	vq->config.pfn = to_guest_phys(p) / getpagesize();

	/* Initialize the vring. */
	vring_init(&vq->vring, num_descs, p, LGUEST_VRING_ALIGN);

	/*
	 * Append virtqueue to this device's descriptor.  We use
	 * device_config() to get the end of the device's current virtqueues;
	 * we check that we haven't added any config or feature information
	 * yet, otherwise we'd be overwriting them.
	 */
	assert(dev->desc->config_len == 0 && dev->desc->feature_len == 0);
	memcpy(device_config(dev), &vq->config, sizeof(vq->config));
	dev->num_vq++;
	dev->desc->num_vq++;

	verbose("Virtqueue page %#lx\n", to_guest_phys(p));

	/*
	 * Add to tail of list, so dev->vq is first vq, dev->vq->next is
	 * second.
	 */
	for (i = &dev->vq; *i; i = &(*i)->next);
	*i = vq;
}

static void add_feature(struct device *dev, unsigned bit)
{
	u8 *features = get_feature_bits(dev);

	/* We can't extend the feature bits once we've added config bytes */
	if (dev->desc->feature_len <= bit / CHAR_BIT) {
		assert(dev->desc->config_len == 0);
		dev->feature_len = dev->desc->feature_len = (bit/CHAR_BIT) + 1;
	}

	features[bit / CHAR_BIT] |= (1 << (bit % CHAR_BIT));
}

static void set_config(struct device *dev, unsigned len, const void *conf)
{
	/* Check we haven't overflowed our single page. */
	if (device_config(dev) + len > devices.descpage + getpagesize())
		errx(1, "Too many devices");

	/* Copy in the config information, and store the length. */
	memcpy(device_config(dev), conf, len);
	dev->desc->config_len = len;

	/* Size must fit in config_len field (8 bits)! */
	assert(dev->desc->config_len == len);
}

static struct device *new_device(const char *name, u16 type)
{
	struct device *dev = malloc(sizeof(*dev));

	/* Now we populate the fields one at a time. */
	dev->desc = new_dev_desc(type);
	dev->name = name;
	dev->vq = NULL;
	dev->feature_len = 0;
	dev->num_vq = 0;
	dev->running = false;

	/*
	 * Append to device list.  Prepending to a single-linked list is
	 * easier, but the user expects the devices to be arranged on the bus
	 * in command-line order.  The first network device on the command line
	 * is eth0, the first block device /dev/vda, etc.
	 */
	if (devices.lastdev)
		devices.lastdev->next = dev;
	else
		devices.dev = dev;
	devices.lastdev = dev;

	return dev;
}

static void setup_console(void)
{
	struct device *dev;

	/* If we can save the initial standard input settings... */
	if (tcgetattr(STDIN_FILENO, &orig_term) == 0) {
		struct termios term = orig_term;
		/*
		 * Then we turn off echo, line buffering and ^C etc: We want a
		 * raw input stream to the Guest.
		 */
		term.c_lflag &= ~(ISIG|ICANON|ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &term);
	}

	dev = new_device("console", VIRTIO_ID_CONSOLE);

	/* We store the console state in dev->priv, and initialize it. */
	dev->priv = malloc(sizeof(struct console_abort));
	((struct console_abort *)dev->priv)->count = 0;

	/*
	 * The console needs two virtqueues: the input then the output.  When
	 * they put something the input queue, we make sure we're listening to
	 * stdin.  When they put something in the output queue, we write it to
	 * stdout.
	 */
	add_virtqueue(dev, VIRTQUEUE_NUM, console_input);
	add_virtqueue(dev, VIRTQUEUE_NUM, console_output);

	verbose("device %u: console\n", ++devices.device_num);
}
/*:*/


static u32 str2ip(const char *ipaddr)
{
	unsigned int b[4];

	if (sscanf(ipaddr, "%u.%u.%u.%u", &b[0], &b[1], &b[2], &b[3]) != 4)
		errx(1, "Failed to parse IP address '%s'", ipaddr);
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

static void str2mac(const char *macaddr, unsigned char mac[6])
{
	unsigned int m[6];
	if (sscanf(macaddr, "%02x:%02x:%02x:%02x:%02x:%02x",
		   &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) != 6)
		errx(1, "Failed to parse mac address '%s'", macaddr);
	mac[0] = m[0];
	mac[1] = m[1];
	mac[2] = m[2];
	mac[3] = m[3];
	mac[4] = m[4];
	mac[5] = m[5];
}

static void add_to_bridge(int fd, const char *if_name, const char *br_name)
{
	int ifidx;
	struct ifreq ifr;

	if (!*br_name)
		errx(1, "must specify bridge name");

	ifidx = if_nametoindex(if_name);
	if (!ifidx)
		errx(1, "interface %s does not exist!", if_name);

	strncpy(ifr.ifr_name, br_name, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ-1] = '\0';
	ifr.ifr_ifindex = ifidx;
	if (ioctl(fd, SIOCBRADDIF, &ifr) < 0)
		err(1, "can't add %s to bridge %s", if_name, br_name);
}

static void configure_device(int fd, const char *tapif, u32 ipaddr)
{
	struct ifreq ifr;
	struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, tapif);

	/* Don't read these incantations.  Just cut & paste them like I did! */
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = htonl(ipaddr);
	if (ioctl(fd, SIOCSIFADDR, &ifr) != 0)
		err(1, "Setting %s interface address", tapif);
	ifr.ifr_flags = IFF_UP;
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) != 0)
		err(1, "Bringing interface %s up", tapif);
}

static int get_tun_device(char tapif[IFNAMSIZ])
{
	struct ifreq ifr;
	int netfd;

	/* Start with this zeroed.  Messy but sure. */
	memset(&ifr, 0, sizeof(ifr));

	/*
	 * We open the /dev/net/tun device and tell it we want a tap device.  A
	 * tap device is like a tun device, only somehow different.  To tell
	 * the truth, I completely blundered my way through this code, but it
	 * works now!
	 */
	netfd = open_or_die("/dev/net/tun", O_RDWR);
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI | IFF_VNET_HDR;
	strcpy(ifr.ifr_name, "tap%d");
	if (ioctl(netfd, TUNSETIFF, &ifr) != 0)
		err(1, "configuring /dev/net/tun");

	if (ioctl(netfd, TUNSETOFFLOAD,
		  TUN_F_CSUM|TUN_F_TSO4|TUN_F_TSO6|TUN_F_TSO_ECN) != 0)
		err(1, "Could not set features for tun device");

	/*
	 * We don't need checksums calculated for packets coming in this
	 * device: trust us!
	 */
	ioctl(netfd, TUNSETNOCSUM, 1);

	memcpy(tapif, ifr.ifr_name, IFNAMSIZ);
	return netfd;
}

static void setup_tun_net(char *arg)
{
	struct device *dev;
	struct net_info *net_info = malloc(sizeof(*net_info));
	int ipfd;
	u32 ip = INADDR_ANY;
	bool bridging = false;
	char tapif[IFNAMSIZ], *p;
	struct virtio_net_config conf;

	net_info->tunfd = get_tun_device(tapif);

	/* First we create a new network device. */
	dev = new_device("net", VIRTIO_ID_NET);
	dev->priv = net_info;

	/* Network devices need a recv and a send queue, just like console. */
	add_virtqueue(dev, VIRTQUEUE_NUM, net_input);
	add_virtqueue(dev, VIRTQUEUE_NUM, net_output);

	/*
	 * We need a socket to perform the magic network ioctls to bring up the
	 * tap interface, connect to the bridge etc.  Any socket will do!
	 */
	ipfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (ipfd < 0)
		err(1, "opening IP socket");

	/* If the command line was --tunnet=bridge:<name> do bridging. */
	if (!strncmp(BRIDGE_PFX, arg, strlen(BRIDGE_PFX))) {
		arg += strlen(BRIDGE_PFX);
		bridging = true;
	}

	/* A mac address may follow the bridge name or IP address */
	p = strchr(arg, ':');
	if (p) {
		str2mac(p+1, conf.mac);
		add_feature(dev, VIRTIO_NET_F_MAC);
		*p = '\0';
	}

	/* arg is now either an IP address or a bridge name */
	if (bridging)
		add_to_bridge(ipfd, tapif, arg);
	else
		ip = str2ip(arg);

	/* Set up the tun device. */
	configure_device(ipfd, tapif, ip);

	add_feature(dev, VIRTIO_F_NOTIFY_ON_EMPTY);
	/* Expect Guest to handle everything except UFO */
	add_feature(dev, VIRTIO_NET_F_CSUM);
	add_feature(dev, VIRTIO_NET_F_GUEST_CSUM);
	add_feature(dev, VIRTIO_NET_F_GUEST_TSO4);
	add_feature(dev, VIRTIO_NET_F_GUEST_TSO6);
	add_feature(dev, VIRTIO_NET_F_GUEST_ECN);
	add_feature(dev, VIRTIO_NET_F_HOST_TSO4);
	add_feature(dev, VIRTIO_NET_F_HOST_TSO6);
	add_feature(dev, VIRTIO_NET_F_HOST_ECN);
	/* We handle indirect ring entries */
	add_feature(dev, VIRTIO_RING_F_INDIRECT_DESC);
	set_config(dev, sizeof(conf), &conf);

	/* We don't need the socket any more; setup is done. */
	close(ipfd);

	devices.device_num++;

	if (bridging)
		verbose("device %u: tun %s attached to bridge: %s\n",
			devices.device_num, tapif, arg);
	else
		verbose("device %u: tun %s: %s\n",
			devices.device_num, tapif, arg);
}
/*:*/

/* This hangs off device->priv. */
struct vblk_info {
	/* The size of the file. */
	off64_t len;

	/* The file descriptor for the file. */
	int fd;

};

static void blk_request(struct virtqueue *vq)
{
	struct vblk_info *vblk = vq->dev->priv;
	unsigned int head, out_num, in_num, wlen;
	int ret;
	u8 *in;
	struct virtio_blk_outhdr *out;
	struct iovec iov[vq->vring.num];
	off64_t off;

	/*
	 * Get the next request, where we normally wait.  It triggers the
	 * interrupt to acknowledge previously serviced requests (if any).
	 */
	head = wait_for_vq_desc(vq, iov, &out_num, &in_num);

	/*
	 * Every block request should contain at least one output buffer
	 * (detailing the location on disk and the type of request) and one
	 * input buffer (to hold the result).
	 */
	if (out_num == 0 || in_num == 0)
		errx(1, "Bad virtblk cmd %u out=%u in=%u",
		     head, out_num, in_num);

	out = convert(&iov[0], struct virtio_blk_outhdr);
	in = convert(&iov[out_num+in_num-1], u8);
	/*
	 * For historical reasons, block operations are expressed in 512 byte
	 * "sectors".
	 */
	off = out->sector * 512;

	/*
	 * The block device implements "barriers", where the Guest indicates
	 * that it wants all previous writes to occur before this write.  We
	 * don't have a way of asking our kernel to do a barrier, so we just
	 * synchronize all the data in the file.  Pretty poor, no?
	 */
	if (out->type & VIRTIO_BLK_T_BARRIER)
		fdatasync(vblk->fd);

	/*
	 * In general the virtio block driver is allowed to try SCSI commands.
	 * It'd be nice if we supported eject, for example, but we don't.
	 */
	if (out->type & VIRTIO_BLK_T_SCSI_CMD) {
		fprintf(stderr, "Scsi commands unsupported\n");
		*in = VIRTIO_BLK_S_UNSUPP;
		wlen = sizeof(*in);
	} else if (out->type & VIRTIO_BLK_T_OUT) {
		/*
		 * Write
		 *
		 * Move to the right location in the block file.  This can fail
		 * if they try to write past end.
		 */
		if (lseek64(vblk->fd, off, SEEK_SET) != off)
			err(1, "Bad seek to sector %llu", out->sector);

		ret = writev(vblk->fd, iov+1, out_num-1);
		verbose("WRITE to sector %llu: %i\n", out->sector, ret);

		/*
		 * Grr... Now we know how long the descriptor they sent was, we
		 * make sure they didn't try to write over the end of the block
		 * file (possibly extending it).
		 */
		if (ret > 0 && off + ret > vblk->len) {
			/* Trim it back to the correct length */
			ftruncate64(vblk->fd, vblk->len);
			/* Die, bad Guest, die. */
			errx(1, "Write past end %llu+%u", off, ret);
		}
		wlen = sizeof(*in);
		*in = (ret >= 0 ? VIRTIO_BLK_S_OK : VIRTIO_BLK_S_IOERR);
	} else {
		/*
		 * Read
		 *
		 * Move to the right location in the block file.  This can fail
		 * if they try to read past end.
		 */
		if (lseek64(vblk->fd, off, SEEK_SET) != off)
			err(1, "Bad seek to sector %llu", out->sector);

		ret = readv(vblk->fd, iov+1, in_num-1);
		verbose("READ from sector %llu: %i\n", out->sector, ret);
		if (ret >= 0) {
			wlen = sizeof(*in) + ret;
			*in = VIRTIO_BLK_S_OK;
		} else {
			wlen = sizeof(*in);
			*in = VIRTIO_BLK_S_IOERR;
		}
	}

	/*
	 * OK, so we noted that it was pretty poor to use an fdatasync as a
	 * barrier.  But Christoph Hellwig points out that we need a sync
	 * *afterwards* as well: "Barriers specify no reordering to the front
	 * or the back."  And Jens Axboe confirmed it, so here we are:
	 */
	if (out->type & VIRTIO_BLK_T_BARRIER)
		fdatasync(vblk->fd);

	/* Finished that request. */
	add_used(vq, head, wlen);
}

/*L:198 This actually sets up a virtual block device. */
static void setup_block_file(const char *filename)
{
	struct device *dev;
	struct vblk_info *vblk;
	struct virtio_blk_config conf;

	/* Creat the device. */
	dev = new_device("block", VIRTIO_ID_BLOCK);

	/* The device has one virtqueue, where the Guest places requests. */
	add_virtqueue(dev, VIRTQUEUE_NUM, blk_request);

	/* Allocate the room for our own bookkeeping */
	vblk = dev->priv = malloc(sizeof(*vblk));

	/* First we open the file and store the length. */
	vblk->fd = open_or_die(filename, O_RDWR|O_LARGEFILE);
	vblk->len = lseek64(vblk->fd, 0, SEEK_END);

	/* We support barriers. */
	add_feature(dev, VIRTIO_BLK_F_BARRIER);

	/* Tell Guest how many sectors this device has. */
	conf.capacity = cpu_to_le64(vblk->len / 512);

	/*
	 * Tell Guest not to put in too many descriptors at once: two are used
	 * for the in and out elements.
	 */
	add_feature(dev, VIRTIO_BLK_F_SEG_MAX);
	conf.seg_max = cpu_to_le32(VIRTQUEUE_NUM - 2);

	/* Don't try to put whole struct: we have 8 bit limit. */
	set_config(dev, offsetof(struct virtio_blk_config, geometry), &conf);

	verbose("device %u: virtblock %llu sectors\n",
		++devices.device_num, le64_to_cpu(conf.capacity));
}

struct rng_info {
	int rfd;
};

static void rng_input(struct virtqueue *vq)
{
	int len;
	unsigned int head, in_num, out_num, totlen = 0;
	struct rng_info *rng_info = vq->dev->priv;
	struct iovec iov[vq->vring.num];

	/* First we need a buffer from the Guests's virtqueue. */
	head = wait_for_vq_desc(vq, iov, &out_num, &in_num);
	if (out_num)
		errx(1, "Output buffers in rng?");

	/*
	 * Just like the console write, we loop to cover the whole iovec.
	 * In this case, short reads actually happen quite a bit.
	 */
	while (!iov_empty(iov, in_num)) {
		len = readv(rng_info->rfd, iov, in_num);
		if (len <= 0)
			err(1, "Read from /dev/random gave %i", len);
		iov_consume(iov, in_num, len);
		totlen += len;
	}

	/* Tell the Guest about the new input. */
	add_used(vq, head, totlen);
}

static void setup_rng(void)
{
	struct device *dev;
	struct rng_info *rng_info = malloc(sizeof(*rng_info));

	/* Our device's privat info simply contains the /dev/random fd. */
	rng_info->rfd = open_or_die("/dev/random", O_RDONLY);

	/* Create the new device. */
	dev = new_device("rng", VIRTIO_ID_RNG);
	dev->priv = rng_info;

	/* The device has one virtqueue, where the Guest places inbufs. */
	add_virtqueue(dev, VIRTQUEUE_NUM, rng_input);

	verbose("device %u: rng\n", devices.device_num++);
}
/* That's the end of device setup. */

/*L:230 Reboot is pretty easy: clean up and exec() the Launcher afresh. */
static void __attribute__((noreturn)) restart_guest(void)
{
	unsigned int i;

	/*
	 * Since we don't track all open fds, we simply close everything beyond
	 * stderr.
	 */
	for (i = 3; i < FD_SETSIZE; i++)
		close(i);

	/* Reset all the devices (kills all threads). */
	cleanup_devices();

	execv(main_args[0], main_args);
	err(1, "Could not exec %s", main_args[0]);
}

static void __attribute__((noreturn)) run_guest(void)
{
	for (;;) {
		unsigned long notify_addr;
		int readval;

		/* We read from the /dev/lguest device to run the Guest. */
		readval = pread(lguest_fd, &notify_addr,
				sizeof(notify_addr), cpu_id);

		/* One unsigned long means the Guest did HCALL_NOTIFY */
		if (readval == sizeof(notify_addr)) {
			verbose("Notify on address %#lx\n", notify_addr);
			handle_output(notify_addr);
		/* ENOENT means the Guest died.  Reading tells us why. */
		} else if (errno == ENOENT) {
			char reason[1024] = { 0 };
			pread(lguest_fd, reason, sizeof(reason)-1, cpu_id);
			errx(1, "%s", reason);
		/* ERESTART means that we need to reboot the guest */
		} else if (errno == ERESTART) {
			restart_guest();
		/* Anything else means a bug or incompatible change. */
		} else
			err(1, "Running guest failed");
	}
}

static struct option opts[] = {
	{ "verbose", 0, NULL, 'v' },
	{ "tunnet", 1, NULL, 't' },
	{ "block", 1, NULL, 'b' },
	{ "rng", 0, NULL, 'r' },
	{ "initrd", 1, NULL, 'i' },
	{ NULL },
};
static void usage(void)
{
	errx(1, "Usage: lguest [--verbose] "
	     "[--tunnet=(<ipaddr>:<macaddr>|bridge:<bridgename>:<macaddr>)\n"
	     "|--block=<filename>|--initrd=<filename>]...\n"
	     "<mem-in-mb> vmlinux [args...]");
}

/*L:105 The main routine is where the real work begins: */
int main(int argc, char *argv[])
{
	/* Memory, code startpoint and size of the (optional) initrd. */
	unsigned long mem = 0, start, initrd_size = 0;
	/* Two temporaries. */
	int i, c;
	/* The boot information for the Guest. */
	struct boot_params *boot;
	/* If they specify an initrd file to load. */
	const char *initrd_name = NULL;

	/* Save the args: we "reboot" by execing ourselves again. */
	main_args = argv;

	/*
	 * First we initialize the device list.  We keep a pointer to the last
	 * device, and the next interrupt number to use for devices (1:
	 * remember that 0 is used by the timer).
	 */
	devices.lastdev = NULL;
	devices.next_irq = 1;

	/* We're CPU 0.  In fact, that's the only CPU possible right now. */
	cpu_id = 0;

	/*
	 * We need to know how much memory so we can set up the device
	 * descriptor and memory pages for the devices as we parse the command
	 * line.  So we quickly look through the arguments to find the amount
	 * of memory now.
	 */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			mem = atoi(argv[i]) * 1024 * 1024;
			/*
			 * We start by mapping anonymous pages over all of
			 * guest-physical memory range.  This fills it with 0,
			 * and ensures that the Guest won't be killed when it
			 * tries to access it.
			 */
			guest_base = map_zeroed_pages(mem / getpagesize()
						      + DEVICE_PAGES);
			guest_limit = mem;
			guest_max = mem + DEVICE_PAGES*getpagesize();
			devices.descpage = get_pages(1);
			break;
		}
	}

	/* The options are fairly straight-forward */
	while ((c = getopt_long(argc, argv, "v", opts, NULL)) != EOF) {
		switch (c) {
		case 'v':
			verbose = true;
			break;
		case 't':
			setup_tun_net(optarg);
			break;
		case 'b':
			setup_block_file(optarg);
			break;
		case 'r':
			setup_rng();
			break;
		case 'i':
			initrd_name = optarg;
			break;
		default:
			warnx("Unknown argument %s", argv[optind]);
			usage();
		}
	}
	/*
	 * After the other arguments we expect memory and kernel image name,
	 * followed by command line arguments for the kernel.
	 */
	if (optind + 2 > argc)
		usage();

	verbose("Guest base is at %p\n", guest_base);

	/* We always have a console device */
	setup_console();

	/* Now we load the kernel */
	start = load_kernel(open_or_die(argv[optind+1], O_RDONLY));

	/* Boot information is stashed at physical address 0 */
	boot = from_guest_phys(0);

	/* Map the initrd image if requested (at top of physical memory) */
	if (initrd_name) {
		initrd_size = load_initrd(initrd_name, mem);
		/*
		 * These are the location in the Linux boot header where the
		 * start and size of the initrd are expected to be found.
		 */
		boot->hdr.ramdisk_image = mem - initrd_size;
		boot->hdr.ramdisk_size = initrd_size;
		/* The bootloader type 0xFF means "unknown"; that's OK. */
		boot->hdr.type_of_loader = 0xFF;
	}

	/*
	 * The Linux boot header contains an "E820" memory map: ours is a
	 * simple, single region.
	 */
	boot->e820_entries = 1;
	boot->e820_map[0] = ((struct e820entry) { 0, mem, E820_RAM });
	/*
	 * The boot header contains a command line pointer: we put the command
	 * line after the boot header.
	 */
	boot->hdr.cmd_line_ptr = to_guest_phys(boot + 1);
	/* We use a simple helper to copy the arguments separated by spaces. */
	concat((char *)(boot + 1), argv+optind+2);

	/* Boot protocol version: 2.07 supports the fields for lguest. */
	boot->hdr.version = 0x207;

	/* The hardware_subarch value of "1" tells the Guest it's an lguest. */
	boot->hdr.hardware_subarch = 1;

	/* Tell the entry path not to try to reload segment registers. */
	boot->hdr.loadflags |= KEEP_SEGMENTS;

	/*
	 * We tell the kernel to initialize the Guest: this returns the open
	 * /dev/lguest file descriptor.
	 */
	tell_kernel(start);

	/* Ensure that we terminate if a device-servicing child dies. */
	signal(SIGCHLD, kill_launcher);

	/* If we exit via err(), this kills all the threads, restores tty. */
	atexit(cleanup_devices);

	/* Finally, run the Guest.  This doesn't return. */
	run_guest();
}
/*:*/

