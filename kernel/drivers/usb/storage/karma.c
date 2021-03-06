

#include <linux/module.h>
#include <linux/slab.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>

#include "usb.h"
#include "transport.h"
#include "debug.h"

MODULE_DESCRIPTION("Driver for Rio Karma");
MODULE_AUTHOR("Bob Copeland <me@bobcopeland.com>, Keith Bennett <keith@mcs.st-and.ac.uk>");
MODULE_LICENSE("GPL");

#define RIO_PREFIX "RIOP\x00"
#define RIO_PREFIX_LEN 5
#define RIO_SEND_LEN 40
#define RIO_RECV_LEN 0x200

#define RIO_ENTER_STORAGE 0x1
#define RIO_LEAVE_STORAGE 0x2
#define RIO_RESET 0xC

struct karma_data {
	int in_storage;
	char *recv;
};

static int rio_karma_init(struct us_data *us);


#define UNUSUAL_DEV(id_vendor, id_product, bcdDeviceMin, bcdDeviceMax, \
		    vendorName, productName, useProtocol, useTransport, \
		    initFunction, flags) \
{ USB_DEVICE_VER(id_vendor, id_product, bcdDeviceMin, bcdDeviceMax), \
  .driver_info = (flags)|(USB_US_TYPE_STOR<<24) }

struct usb_device_id karma_usb_ids[] = {
#	include "unusual_karma.h"
	{ }		/* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, karma_usb_ids);

#undef UNUSUAL_DEV

#define UNUSUAL_DEV(idVendor, idProduct, bcdDeviceMin, bcdDeviceMax, \
		    vendor_name, product_name, use_protocol, use_transport, \
		    init_function, Flags) \
{ \
	.vendorName = vendor_name,	\
	.productName = product_name,	\
	.useProtocol = use_protocol,	\
	.useTransport = use_transport,	\
	.initFunction = init_function,	\
}

static struct us_unusual_dev karma_unusual_dev_list[] = {
#	include "unusual_karma.h"
	{ }		/* Terminating entry */
};

#undef UNUSUAL_DEV


static int rio_karma_send_command(char cmd, struct us_data *us)
{
	int result, partial;
	unsigned long timeout;
	static unsigned char seq = 1;
	struct karma_data *data = (struct karma_data *) us->extra;

	US_DEBUGP("karma: sending command %04x\n", cmd);
	memset(us->iobuf, 0, RIO_SEND_LEN);
	memcpy(us->iobuf, RIO_PREFIX, RIO_PREFIX_LEN);
	us->iobuf[5] = cmd;
	us->iobuf[6] = seq;

	timeout = jiffies + msecs_to_jiffies(6000);
	for (;;) {
		result = usb_stor_bulk_transfer_buf(us, us->send_bulk_pipe,
			us->iobuf, RIO_SEND_LEN, &partial);
		if (result != USB_STOR_XFER_GOOD)
			goto err;

		result = usb_stor_bulk_transfer_buf(us, us->recv_bulk_pipe,
			data->recv, RIO_RECV_LEN, &partial);
		if (result != USB_STOR_XFER_GOOD)
			goto err;

		if (data->recv[5] == seq)
			break;

		if (time_after(jiffies, timeout))
			goto err;

		us->iobuf[4] = 0x80;
		us->iobuf[5] = 0;
		msleep(50);
	}

	seq++;
	if (seq == 0)
		seq = 1;

	US_DEBUGP("karma: sent command %04x\n", cmd);
	return 0;
err:
	US_DEBUGP("karma: command %04x failed\n", cmd);
	return USB_STOR_TRANSPORT_FAILED;
}

static int rio_karma_transport(struct scsi_cmnd *srb, struct us_data *us)
{
	int ret;
	struct karma_data *data = (struct karma_data *) us->extra;

	if (srb->cmnd[0] == READ_10 && !data->in_storage) {
		ret = rio_karma_send_command(RIO_ENTER_STORAGE, us);
		if (ret)
			return ret;

		data->in_storage = 1;
		return usb_stor_Bulk_transport(srb, us);
	} else if (srb->cmnd[0] == START_STOP) {
		ret = rio_karma_send_command(RIO_LEAVE_STORAGE, us);
		if (ret)
			return ret;

		data->in_storage = 0;
		return rio_karma_send_command(RIO_RESET, us);
	}
	return usb_stor_Bulk_transport(srb, us);
}

static void rio_karma_destructor(void *extra)
{
	struct karma_data *data = (struct karma_data *) extra;
	kfree(data->recv);
}

static int rio_karma_init(struct us_data *us)
{
	int ret = 0;
	struct karma_data *data = kzalloc(sizeof(struct karma_data), GFP_NOIO);
	if (!data)
		goto out;

	data->recv = kmalloc(RIO_RECV_LEN, GFP_NOIO);
	if (!data->recv) {
		kfree(data);
		goto out;
	}

	us->extra = data;
	us->extra_destructor = rio_karma_destructor;
	ret = rio_karma_send_command(RIO_ENTER_STORAGE, us);
	data->in_storage = (ret == 0);
out:
	return ret;
}

static int karma_probe(struct usb_interface *intf,
			 const struct usb_device_id *id)
{
	struct us_data *us;
	int result;

	result = usb_stor_probe1(&us, intf, id,
			(id - karma_usb_ids) + karma_unusual_dev_list);
	if (result)
		return result;

	us->transport_name = "Rio Karma/Bulk";
	us->transport = rio_karma_transport;
	us->transport_reset = usb_stor_Bulk_reset;

	result = usb_stor_probe2(us);
	return result;
}

static struct usb_driver karma_driver = {
	.name =		"ums-karma",
	.probe =	karma_probe,
	.disconnect =	usb_stor_disconnect,
	.suspend =	usb_stor_suspend,
	.resume =	usb_stor_resume,
	.reset_resume =	usb_stor_reset_resume,
	.pre_reset =	usb_stor_pre_reset,
	.post_reset =	usb_stor_post_reset,
	.id_table =	karma_usb_ids,
	.soft_unbind =	1,
};

static int __init karma_init(void)
{
	return usb_register(&karma_driver);
}

static void __exit karma_exit(void)
{
	usb_deregister(&karma_driver);
}

module_init(karma_init);
module_exit(karma_exit);
