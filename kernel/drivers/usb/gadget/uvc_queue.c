

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <asm/atomic.h>

#include "uvc.h"


static void
uvc_queue_init(struct uvc_video_queue *queue, enum v4l2_buf_type type)
{
	mutex_init(&queue->mutex);
	spin_lock_init(&queue->irqlock);
	INIT_LIST_HEAD(&queue->mainqueue);
	INIT_LIST_HEAD(&queue->irqqueue);
	queue->type = type;
}

static int uvc_free_buffers(struct uvc_video_queue *queue)
{
	unsigned int i;

	for (i = 0; i < queue->count; ++i) {
		if (queue->buffer[i].vma_use_count != 0)
			return -EBUSY;
	}

	if (queue->count) {
		vfree(queue->mem);
		queue->count = 0;
	}

	return 0;
}

static int
uvc_alloc_buffers(struct uvc_video_queue *queue, unsigned int nbuffers,
		  unsigned int buflength)
{
	unsigned int bufsize = PAGE_ALIGN(buflength);
	unsigned int i;
	void *mem = NULL;
	int ret;

	if (nbuffers > UVC_MAX_VIDEO_BUFFERS)
		nbuffers = UVC_MAX_VIDEO_BUFFERS;

	mutex_lock(&queue->mutex);

	if ((ret = uvc_free_buffers(queue)) < 0)
		goto done;

	/* Bail out if no buffers should be allocated. */
	if (nbuffers == 0)
		goto done;

	/* Decrement the number of buffers until allocation succeeds. */
	for (; nbuffers > 0; --nbuffers) {
		mem = vmalloc_32(nbuffers * bufsize);
		if (mem != NULL)
			break;
	}

	if (mem == NULL) {
		ret = -ENOMEM;
		goto done;
	}

	for (i = 0; i < nbuffers; ++i) {
		memset(&queue->buffer[i], 0, sizeof queue->buffer[i]);
		queue->buffer[i].buf.index = i;
		queue->buffer[i].buf.m.offset = i * bufsize;
		queue->buffer[i].buf.length = buflength;
		queue->buffer[i].buf.type = queue->type;
		queue->buffer[i].buf.sequence = 0;
		queue->buffer[i].buf.field = V4L2_FIELD_NONE;
		queue->buffer[i].buf.memory = V4L2_MEMORY_MMAP;
		queue->buffer[i].buf.flags = 0;
		init_waitqueue_head(&queue->buffer[i].wait);
	}

	queue->mem = mem;
	queue->count = nbuffers;
	queue->buf_size = bufsize;
	ret = nbuffers;

done:
	mutex_unlock(&queue->mutex);
	return ret;
}

static void __uvc_query_buffer(struct uvc_buffer *buf,
		struct v4l2_buffer *v4l2_buf)
{
	memcpy(v4l2_buf, &buf->buf, sizeof *v4l2_buf);

	if (buf->vma_use_count)
		v4l2_buf->flags |= V4L2_BUF_FLAG_MAPPED;

	switch (buf->state) {
	case UVC_BUF_STATE_ERROR:
	case UVC_BUF_STATE_DONE:
		v4l2_buf->flags |= V4L2_BUF_FLAG_DONE;
		break;
	case UVC_BUF_STATE_QUEUED:
	case UVC_BUF_STATE_ACTIVE:
		v4l2_buf->flags |= V4L2_BUF_FLAG_QUEUED;
		break;
	case UVC_BUF_STATE_IDLE:
	default:
		break;
	}
}

static int
uvc_query_buffer(struct uvc_video_queue *queue, struct v4l2_buffer *v4l2_buf)
{
	int ret = 0;

	mutex_lock(&queue->mutex);
	if (v4l2_buf->index >= queue->count) {
		ret = -EINVAL;
		goto done;
	}

	__uvc_query_buffer(&queue->buffer[v4l2_buf->index], v4l2_buf);

done:
	mutex_unlock(&queue->mutex);
	return ret;
}

static int
uvc_queue_buffer(struct uvc_video_queue *queue, struct v4l2_buffer *v4l2_buf)
{
	struct uvc_buffer *buf;
	unsigned long flags;
	int ret = 0;

	uvc_trace(UVC_TRACE_CAPTURE, "Queuing buffer %u.\n", v4l2_buf->index);

	if (v4l2_buf->type != queue->type ||
	    v4l2_buf->memory != V4L2_MEMORY_MMAP) {
		uvc_trace(UVC_TRACE_CAPTURE, "[E] Invalid buffer type (%u) "
			"and/or memory (%u).\n", v4l2_buf->type,
			v4l2_buf->memory);
		return -EINVAL;
	}

	mutex_lock(&queue->mutex);
	if (v4l2_buf->index >= queue->count) {
		uvc_trace(UVC_TRACE_CAPTURE, "[E] Out of range index.\n");
		ret = -EINVAL;
		goto done;
	}

	buf = &queue->buffer[v4l2_buf->index];
	if (buf->state != UVC_BUF_STATE_IDLE) {
		uvc_trace(UVC_TRACE_CAPTURE, "[E] Invalid buffer state "
			"(%u).\n", buf->state);
		ret = -EINVAL;
		goto done;
	}

	if (v4l2_buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT &&
	    v4l2_buf->bytesused > buf->buf.length) {
		uvc_trace(UVC_TRACE_CAPTURE, "[E] Bytes used out of bounds.\n");
		ret = -EINVAL;
		goto done;
	}

	if (v4l2_buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		buf->buf.bytesused = 0;
	else
		buf->buf.bytesused = v4l2_buf->bytesused;

	spin_lock_irqsave(&queue->irqlock, flags);
	if (queue->flags & UVC_QUEUE_DISCONNECTED) {
		spin_unlock_irqrestore(&queue->irqlock, flags);
		ret = -ENODEV;
		goto done;
	}
	buf->state = UVC_BUF_STATE_QUEUED;

	ret = (queue->flags & UVC_QUEUE_PAUSED) != 0;
	queue->flags &= ~UVC_QUEUE_PAUSED;

	list_add_tail(&buf->stream, &queue->mainqueue);
	list_add_tail(&buf->queue, &queue->irqqueue);
	spin_unlock_irqrestore(&queue->irqlock, flags);

done:
	mutex_unlock(&queue->mutex);
	return ret;
}

static int uvc_queue_waiton(struct uvc_buffer *buf, int nonblocking)
{
	if (nonblocking) {
		return (buf->state != UVC_BUF_STATE_QUEUED &&
			buf->state != UVC_BUF_STATE_ACTIVE)
			? 0 : -EAGAIN;
	}

	return wait_event_interruptible(buf->wait,
		buf->state != UVC_BUF_STATE_QUEUED &&
		buf->state != UVC_BUF_STATE_ACTIVE);
}

static int
uvc_dequeue_buffer(struct uvc_video_queue *queue, struct v4l2_buffer *v4l2_buf,
		   int nonblocking)
{
	struct uvc_buffer *buf;
	int ret = 0;

	if (v4l2_buf->type != queue->type ||
	    v4l2_buf->memory != V4L2_MEMORY_MMAP) {
		uvc_trace(UVC_TRACE_CAPTURE, "[E] Invalid buffer type (%u) "
			"and/or memory (%u).\n", v4l2_buf->type,
			v4l2_buf->memory);
		return -EINVAL;
	}

	mutex_lock(&queue->mutex);
	if (list_empty(&queue->mainqueue)) {
		uvc_trace(UVC_TRACE_CAPTURE, "[E] Empty buffer queue.\n");
		ret = -EINVAL;
		goto done;
	}

	buf = list_first_entry(&queue->mainqueue, struct uvc_buffer, stream);
	if ((ret = uvc_queue_waiton(buf, nonblocking)) < 0)
		goto done;

	uvc_trace(UVC_TRACE_CAPTURE, "Dequeuing buffer %u (%u, %u bytes).\n",
		buf->buf.index, buf->state, buf->buf.bytesused);

	switch (buf->state) {
	case UVC_BUF_STATE_ERROR:
		uvc_trace(UVC_TRACE_CAPTURE, "[W] Corrupted data "
			"(transmission error).\n");
		ret = -EIO;
	case UVC_BUF_STATE_DONE:
		buf->state = UVC_BUF_STATE_IDLE;
		break;

	case UVC_BUF_STATE_IDLE:
	case UVC_BUF_STATE_QUEUED:
	case UVC_BUF_STATE_ACTIVE:
	default:
		uvc_trace(UVC_TRACE_CAPTURE, "[E] Invalid buffer state %u "
			"(driver bug?).\n", buf->state);
		ret = -EINVAL;
		goto done;
	}

	list_del(&buf->stream);
	__uvc_query_buffer(buf, v4l2_buf);

done:
	mutex_unlock(&queue->mutex);
	return ret;
}

static unsigned int
uvc_queue_poll(struct uvc_video_queue *queue, struct file *file,
	       poll_table *wait)
{
	struct uvc_buffer *buf;
	unsigned int mask = 0;

	mutex_lock(&queue->mutex);
	if (list_empty(&queue->mainqueue))
		goto done;

	buf = list_first_entry(&queue->mainqueue, struct uvc_buffer, stream);

	poll_wait(file, &buf->wait, wait);
	if (buf->state == UVC_BUF_STATE_DONE ||
	    buf->state == UVC_BUF_STATE_ERROR)
		mask |= POLLOUT | POLLWRNORM;

done:
	mutex_unlock(&queue->mutex);
	return mask;
}

static void uvc_vm_open(struct vm_area_struct *vma)
{
	struct uvc_buffer *buffer = vma->vm_private_data;
	buffer->vma_use_count++;
}

static void uvc_vm_close(struct vm_area_struct *vma)
{
	struct uvc_buffer *buffer = vma->vm_private_data;
	buffer->vma_use_count--;
}

static struct vm_operations_struct uvc_vm_ops = {
	.open		= uvc_vm_open,
	.close		= uvc_vm_close,
};

static int
uvc_queue_mmap(struct uvc_video_queue *queue, struct vm_area_struct *vma)
{
	struct uvc_buffer *uninitialized_var(buffer);
	struct page *page;
	unsigned long addr, start, size;
	unsigned int i;
	int ret = 0;

	start = vma->vm_start;
	size = vma->vm_end - vma->vm_start;

	mutex_lock(&queue->mutex);

	for (i = 0; i < queue->count; ++i) {
		buffer = &queue->buffer[i];
		if ((buffer->buf.m.offset >> PAGE_SHIFT) == vma->vm_pgoff)
			break;
	}

	if (i == queue->count || size != queue->buf_size) {
		ret = -EINVAL;
		goto done;
	}

	/*
	 * VM_IO marks the area as being an mmaped region for I/O to a
	 * device. It also prevents the region from being core dumped.
	 */
	vma->vm_flags |= VM_IO;

	addr = (unsigned long)queue->mem + buffer->buf.m.offset;
	while (size > 0) {
		page = vmalloc_to_page((void *)addr);
		if ((ret = vm_insert_page(vma, start, page)) < 0)
			goto done;

		start += PAGE_SIZE;
		addr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

	vma->vm_ops = &uvc_vm_ops;
	vma->vm_private_data = buffer;
	uvc_vm_open(vma);

done:
	mutex_unlock(&queue->mutex);
	return ret;
}

static void uvc_queue_cancel(struct uvc_video_queue *queue, int disconnect)
{
	struct uvc_buffer *buf;
	unsigned long flags;

	spin_lock_irqsave(&queue->irqlock, flags);
	while (!list_empty(&queue->irqqueue)) {
		buf = list_first_entry(&queue->irqqueue, struct uvc_buffer,
				       queue);
		list_del(&buf->queue);
		buf->state = UVC_BUF_STATE_ERROR;
		wake_up(&buf->wait);
	}
	/* This must be protected by the irqlock spinlock to avoid race
	 * conditions between uvc_queue_buffer and the disconnection event that
	 * could result in an interruptible wait in uvc_dequeue_buffer. Do not
	 * blindly replace this logic by checking for the UVC_DEV_DISCONNECTED
	 * state outside the queue code.
	 */
	if (disconnect)
		queue->flags |= UVC_QUEUE_DISCONNECTED;
	spin_unlock_irqrestore(&queue->irqlock, flags);
}

static int uvc_queue_enable(struct uvc_video_queue *queue, int enable)
{
	unsigned int i;
	int ret = 0;

	mutex_lock(&queue->mutex);
	if (enable) {
		if (uvc_queue_streaming(queue)) {
			ret = -EBUSY;
			goto done;
		}
		queue->sequence = 0;
		queue->flags |= UVC_QUEUE_STREAMING;
		queue->buf_used = 0;
	} else {
		uvc_queue_cancel(queue, 0);
		INIT_LIST_HEAD(&queue->mainqueue);

		for (i = 0; i < queue->count; ++i)
			queue->buffer[i].state = UVC_BUF_STATE_IDLE;

		queue->flags &= ~UVC_QUEUE_STREAMING;
	}

done:
	mutex_unlock(&queue->mutex);
	return ret;
}

static struct uvc_buffer *
uvc_queue_next_buffer(struct uvc_video_queue *queue, struct uvc_buffer *buf)
{
	struct uvc_buffer *nextbuf;
	unsigned long flags;

	if ((queue->flags & UVC_QUEUE_DROP_INCOMPLETE) &&
	    buf->buf.length != buf->buf.bytesused) {
		buf->state = UVC_BUF_STATE_QUEUED;
		buf->buf.bytesused = 0;
		return buf;
	}

	spin_lock_irqsave(&queue->irqlock, flags);
	list_del(&buf->queue);
	if (!list_empty(&queue->irqqueue))
		nextbuf = list_first_entry(&queue->irqqueue, struct uvc_buffer,
					   queue);
	else
		nextbuf = NULL;
	spin_unlock_irqrestore(&queue->irqlock, flags);

	buf->buf.sequence = queue->sequence++;
	do_gettimeofday(&buf->buf.timestamp);

	wake_up(&buf->wait);
	return nextbuf;
}

static struct uvc_buffer *uvc_queue_head(struct uvc_video_queue *queue)
{
	struct uvc_buffer *buf = NULL;

	if (!list_empty(&queue->irqqueue))
		buf = list_first_entry(&queue->irqqueue, struct uvc_buffer,
				       queue);
	else
		queue->flags |= UVC_QUEUE_PAUSED;

	return buf;
}

