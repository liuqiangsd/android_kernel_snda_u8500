/*
 * Copyright (C) ST-Ericsson AB 2010
 * Author: Sjur Brendeland / sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 */

#ifndef XSHM_PDEV_H_
#define XSHM_PDEV_H_
#include <linux/platform_device.h>

#define XSHM_NAMESZ 16

/**
 * struct xshm_udchannel - Unidirectional channel for xshm driver.
 *
 * @addr: Base address of the channel, address must be
 *		  a kernel logical address.
 * @buffers: The number of buffers in the channel.
 * @ch_size: The size of data area for the channel in one direction.
 * @xfer_bit: GENI/O bit used when sending data (write pointer move)
 * @xfer_done_bit: GENI/O bit used to indicate avilable buffers
 *	(read pointer move).
 * @alignment: Alignment used in payload protocol.
 * @mtu: Maxium Transfer Unit used for packet in a buffer (Packet mode).
 * @packets: Maxium number of packets in a buffer (Packet mode).
 * @state: State of the device 0 - Closed, 1 - Open
 * @read: Specify the read index for a channel. In packed mode
 *	this index will at any time refer to the next buffer available for read.
 *	In stream mode, this will be the read index in the ring-buffer.
 * @write: Specify the write index for a channel. In packed mode
 *	this index will at any time refer to the next buffer available for
 *	write. In stream mode, this will be the write index in the ring-buffer.
 * @buf_size: In packet mode, this array contains the size of each buffer.
 *	In stream mode this is unused.
 *
 * This external shared memory channel configuration is exported from the
 * platform device. It gives the platform driver the
 * necessary information for running the shared memory protocol
 * between modem and host.
 *
 * Note that two instances of this configuration is needed in order to run a
 * bi-directional channel.
 */
struct xshm_udchannel {
	void *addr;
	u32 buffers;
	u32 ch_size;
	u8 xfer_done_bit;
	u8 xfer_bit;
	u32 mtu;
	u32 alignment;
	u32 packets;
	__le32 *state;
	__le32 *read;
	__le32 *write;
	__le32 *buf_size;
/* private: */
	struct kobject kobj; /* kobj must be located at the end */
};

/**
 * struct xshm_channel - Channel definition for xshm driver.
 * @rx: Configuration for RX channel
 * @tx: Configuration for TX channel
 * @excl_group: Only channels with the same group ID can be open simultaneously.
 * @mode: Configuring type of channel PACKET(1), STREAM(2)
 * @name: Name of interface.
 * @priority: Priority of the channel.
 * @latency: Latency of the channel.
 */
struct xshm_channel {
	struct xshm_udchannel rx, tx;
	u32 excl_group;
	u32 mode;
	char name[XSHM_NAMESZ];
	u32 priority;
	u32 latency;
};

#define XSHM_OPEN   1
#define XSHM_CLOSED 0

enum xshm_dev_state {
	XSHM_DEV_CLOSED = 0,
	XSHM_DEV_OPENING,
	XSHM_DEV_OPEN,
	XSHM_DEV_ACTIVE,
};

/**
 * struct xshm_dev - Device definition for xshm platform device.
 *
 * @pdev: Platform device
 * @cfg: Configuration for the Channel
 * @state: State of the device: Closed - No user space client is using it,
 *	Open - Open but no payload queued, Active - Payload queued on device.
 *
 * @open: The driver calls open() when channel is taken into use.
 *	This function will fail if channel configuration is inconsistent,
 *	or upon resource conflicts with other channels.
 *
 * @open_cb: The device calls open_cb() when is ready for use.
 *
 * @close: Called by the driver when a channel is no longer in use.
 *
 * @close_cb: The device calls close_cb() to notify about remote side closure.
 *
 * @ipc_tx_release_cb: This callback is triggered by the modem when a
 *	transmit operation has completed and the buffer can be reused.
 *	This function must be set by the driver upon device registration.
 *	The "more" flag is set if ipc_rx_cb() call is coming immediately
 *	after this call to ipc_tx_release_cb().
 *
 * @ipc_rx_cb: The driver gets this callback when the modem sends a buffer
 *	from the modem. The driver must call ipc_rx_release()
 *	to make the buffer available again when the received buffer has been
 *	processed.
 *	This function pointer must be set by the driver upon device
 *	registration.
 *
 * @ipc_rx_release: Called by the driver when a RX operation has completed
 *	and that the rx-buffer is released.
 *
 * @ipc_tx: Called by the driver when a TX buffer shall be sent to the modem.
 *
 * @driver_data: pointer to driver specific data.
 *
 * When communicating between two systems (e.g. modem and host),
 * external shared memory can bused (e.g. C2C or DPRAM).
 *
 * This structure is used by the platform device representing the
 * External Shared Memory.
 *
 * The this structure contains configuration data for the platform device and
 * functions pointers for IPC communication between Linux host and modem.
 * The external shared memory protocol memory e.g. C2C or DPRAM
 * together is a IPC mechanism for transporting small commands such as
 * Mailbox or GENI/O.
 *
 * This data structure is initiated by the xshm platform device, except
 * for the functions ipc_rx_cb() and ipc_tx_release_cb(). They must be set by
 * the platform driver when device is registering.
 */

struct xshm_dev {
	struct platform_device pdev;
	struct xshm_channel cfg;
	enum xshm_dev_state state;
	int (*open)(struct xshm_dev *dev);
	void (*close)(struct xshm_dev *dev);
	int (*ipc_rx_release)(struct xshm_dev *dev, bool more);
	int (*ipc_tx)(struct xshm_dev *dev);
	int (*open_cb)(void *drv);
	void (*close_cb)(void *drv);
	int (*ipc_rx_cb)(void *drv);
	int (*ipc_tx_release_cb)(void *drv);
	void *driver_data;
	/* private: */
	struct list_head node;
	void *priv;
};

/**
 * xshm_create_dev() - Create an instance of the xshm platform device.
 * @shmdev: Device configuration data.
 */
int xshm_register_dev(struct xshm_dev *shmdev);

/**
 * xshm_ipc_ready() - Notify that GENO bit READY_FOR_IPC is set
 */
void xshm_ipc_ready(void);

/**
 * xshm_caif_ready() - Notify that GENO bit READY_FOR_CAIF is set
 */
void xshm_caif_ready(void);

extern bool ready_for_ipc;
extern bool ready_for_caif;
void xshm_put(struct xshm_dev *shmdev);
struct xshm_dev *xshmdev_alloc(void);
void xshmdev_free(struct xshm_dev *dev);
void remove_devices(void);
void close_devices(void);
void xshm_boot_exit(void);
int xshm_boot_init(void);
void xshm_reset(void);
void genio_ipc_ready_cb(void);
#endif
