#ifndef _RK_USB_DISK_H_
#define _RK_USB_DISK_H_

#define DEF_MNT_DIR "/mnt"

#define FILE_DEV_CHECK "/proc/scsi/scsi"
#define FILE_MNT_CHECK "/proc/mounts"
#define DEV_PART_FILE "/proc/partitions"
#define USB_DEV_CHECK "/proc/scsi/usb-storage"

typedef struct usbdisk{
	char mntdir[128];
	char devnode[128];
} usbdisk_t;

#ifdef __cplusplus
extern "C" {
#endif

int rkUsbDiskCheck(struct usbdisk *dev);
int rkUsbDiskMnt(struct usbdisk *dev);
int rkUsbDiskRemove(struct usbdisk *dev);

#ifdef __cplusplus
}
#endif

#endif /* _RK_USB_DISK_H_ */
