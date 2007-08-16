/*
 * Syllable USB support
 *
 * Originally derived from the BSD version, but it has moved considerably
 * to the point where it's really a unique implementation. It's maybe halfway
 * between the Linux & BSD implementations.
 *
 * $Id$
 * $Name$
 *
 * This library is covered by the LGPL, read LICENSE for details.
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <atheos/libusb.h>

#include "usbi.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

static int do_io_request(int request, usb_dev_handle *dev, int ep, char *bytes, int size, int timeout );

#define MAX_CONTROLLERS 10
#define USB_DEVFS	"/dev/usb"

int usb_os_open(usb_dev_handle *dev)
{
  char ctlpath[PATH_MAX + 1];

  snprintf(ctlpath, PATH_MAX, "%s/%s/%s", USB_DEVFS, dev->bus->dirname, dev->device->filename);

  dev->fd = open(ctlpath, O_RDWR);
  if (dev->fd < 0) {
    dev->fd = open(ctlpath, O_RDONLY);
    if (dev->fd < 0) {
      USB_ERROR_STR(-errno, "failed to open %s: %s",
                    ctlpath, strerror(errno));
    }
  }

  return 0;
}

int usb_os_close(usb_dev_handle *dev)
{
  if (dev->fd <= 0)
    return 0;

  if (close(dev->fd) == -1)
    /* Failing trying to close a file really isn't an error, so return 0 */
    USB_ERROR_STR(0, "tried to close device fd %d: %s", dev->fd,
                  strerror(errno));

  return 0;
}

int usb_set_configuration(usb_dev_handle *dev, int configuration)
{
  int ret;

  ret = ioctl(dev->fd, USB_SET_CONFIGURATION, &configuration);
  if (ret < 0)
    USB_ERROR_STR(-errno, "could not set config %d: %s", configuration,
                  strerror(errno));

  dev->config = configuration;

  return 0;
}

int usb_claim_interface(usb_dev_handle *dev, int interface)
{
  /* Syllable doesn't have the corresponding ioctl.  It is
     sufficient to open the relevant endpoints as needed. */

  dev->interface = interface;

  return 0;
}

int usb_release_interface(usb_dev_handle *dev, int interface)
{
  /* See above */
  return 0;
}

int usb_set_altinterface(usb_dev_handle *dev, int alternate)
{
  int ret;
  struct usb_alt_interface intf;

  if (dev->interface < 0)
    USB_ERROR(-EINVAL);

  intf.nIFNum = dev->interface;
  intf.nAlternate = alternate;

  ret = ioctl(dev->fd, USB_SET_ALTINTERFACE, &intf);
  if (ret < 0)
    USB_ERROR_STR(-errno, "could not set alt intf %d/%d: %s",
                  dev->interface, alternate, strerror(errno));

  dev->altsetting = alternate;

  return 0;
}

static int do_io_request(int request, usb_dev_handle *dev, int ep, char *bytes, int size, int timeout )
{
  struct usb_io_request req;
  int ret;

  req.nEndpoint = ep;
  req.pData = bytes;
  req.nSize = size;
  req.nTimeout = timeout * 100;

  ret = ioctl(dev->fd, request, &req);
  if (ret < 0)
    USB_ERROR_STR(-errno, "I/O request failed: %s", strerror(errno));
  else
    ret = size;

  return ret;
}

int usb_bulk_write(usb_dev_handle *dev, int ep, char *bytes, int size,
                   int timeout)
{
  int ret;

  /* Ensure the endpoint address is correct */
  ep &= ~USB_ENDPOINT_IN;

  ret = do_io_request(USB_BULK_WRITE, dev, ep, bytes, size, timeout);
  if (ret < 0)
    USB_ERROR_STR(-errno, "error writing to bulk endpoint %s/%d: %s",
                  dev->device->filename, ep, strerror(errno));

  return ret;
}

int usb_bulk_read(usb_dev_handle *dev, int ep, char *bytes, int size,
                  int timeout)
{
  int ret;

  /* Ensure the endpoint address is correct */
  ep |= USB_ENDPOINT_IN;

  ret = do_io_request(USB_BULK_READ, dev, ep, bytes, size, timeout);
  if (ret < 0)
    USB_ERROR_STR(-errno, "error reading from bulk endpoint %s/%d: %s",
                  dev->device->filename, ep, strerror(errno));

  return ret;
}

int usb_interrupt_write(usb_dev_handle *dev, int ep, char *bytes, int size,
                        int timeout)
{
  int ret, sent = 0;

  /* Ensure the endpoint address is correct */
  ep &= ~USB_ENDPOINT_IN;

  do {
    ret = do_io_request(USB_INT_WRITE, dev, ep, bytes+sent, size-sent, timeout);
    if (ret < 0)
      USB_ERROR_STR(-errno, "error writing to interrupt endpoint %s/%d: %s",
                    dev->device->filename, ep, strerror(errno));

    sent += ret;
  } while (ret > 0 && sent < size);

  return sent;
}

int usb_interrupt_read(usb_dev_handle *dev, int ep, char *bytes, int size,
                       int timeout)
{
  int ret, retrieved = 0;

  /* Ensure the endpoint address is correct */
  ep |= USB_ENDPOINT_IN;

  do {
    ret = do_io_request(USB_INT_READ, dev, ep, bytes+retrieved, size-retrieved, timeout);
    if (ret < 0)
      USB_ERROR_STR(-errno, "error reading from interrupt endpoint %s/%d: %s",
                    dev->device->filename, ep, strerror(errno));

    retrieved += ret;
  } while (ret > 0 && retrieved < size);

  return retrieved;
}

int usb_control_msg(usb_dev_handle *dev, int requesttype, int request,
                     int value, int index, char *bytes, int size, int timeout)
{
  struct usb_ctl_request req;
  int ret;

  if (usb_debug >= 3)
    fprintf(stderr, "usb_control_msg: %d %d %d %d %p %d %d\n",
            requesttype, request, value, index, bytes, size, timeout);

  req.nRequest = request;
  req.nRequestType = requesttype;
  req.nValue = value;
  req.nIndex = index;
  req.pData = bytes;
  req.nSize = size;
  req.nTimeout = timeout * 100;

  ret = ioctl(dev->fd, USB_DO_REQUEST, &req);
  if (ret < 0)
    USB_ERROR_STR(-errno, "error sending control message: %s",
                  strerror(errno));

  return req.nSize;
}

int usb_os_find_busses(struct usb_bus **busses)
{
  struct usb_bus *fbus = NULL;
  int controller;
  char buf[20];

  for (controller = 0; controller < MAX_CONTROLLERS; controller++) {
    struct usb_bus *bus;

    snprintf(buf, sizeof(buf) - 1, "%s/%d", USB_DEVFS, controller);
    if (access(buf, O_RDWR)) {
      if (usb_debug >= 2)
        if (errno != ENXIO && errno != ENOENT)
          fprintf(stderr, "usb_os_find_busses: can't open %s: %s\n",
                  buf, strerror(errno));
      continue;
    }

    bus = malloc(sizeof(*bus));
    if (!bus)
      USB_ERROR(-ENOMEM);

    memset((void *)bus, 0, sizeof(*bus));

    snprintf(bus->dirname, sizeof(bus->dirname) - 1, "%d", controller);
    bus->dirname[sizeof(bus->dirname) - 1] = 0;

    LIST_ADD(fbus, bus);

    if (usb_debug >= 2)
      fprintf(stderr, "usb_os_find_busses: Found %s\n", bus->dirname);
  }

  *busses = fbus;

  return 0;
}

int usb_os_find_devices(struct usb_bus *bus, struct usb_device **devices)
{
  struct usb_device *fdev = NULL;
  char bbuf[PATH_MAX+1], fbuf[PATH_MAX+1];
  DIR *bfd;
  struct dirent *ent;

  snprintf(bbuf, sizeof( bbuf ) - 1, "%s/%s", USB_DEVFS, bus->dirname);

  bfd = opendir(bbuf);
  if(!bfd)
    USB_ERROR_STR(-errno, "couldn't open(%s): %s", bbuf,
                  strerror(errno));

  while((ent = readdir(bfd))) {
    struct usb_device *dev;
	int dfd;
    char buf[PATH_MAX+1];
    unsigned char device_desc[DEVICE_DESC_LENGTH];

    if (ent->d_name[0] == '.' &&
       (ent->d_name[1] == '\0' || ent->d_name[1] == '.' ))
      continue;

    dev = malloc(sizeof(*dev));
    if (!dev)
      USB_ERROR(-ENOMEM);

    memset((void *)dev, 0, sizeof(*dev));

    dev->bus = bus;

    snprintf(dev->filename, sizeof(dev->filename) - 1, "%s", ent->d_name);
    dev->filename[sizeof(dev->filename) - 1] = 0;

    snprintf(fbuf, sizeof( fbuf ) - 1, "%s/%s/%s", USB_DEVFS, bus->dirname, dev->filename);

    /* Open the device */
    dfd = open(fbuf, O_RDONLY);
    if (dfd < 0) {
      if (usb_debug >= 2)
        fprintf(stderr, "usb_os_find_devices: couldn't open device %s: %s\n",
                fbuf, strerror(errno));
      continue;
    }

    if (ioctl(dfd, USB_GET_DEVICE_DESC, device_desc) < 0)
      USB_ERROR_STR(-errno, "couldn't get device descriptor for %s: %s",
                    buf, strerror(errno));

    close(dfd);

    usb_parse_descriptor(device_desc, "bbWbbbbWWWbbbb", &dev->descriptor);

    LIST_ADD(fdev, dev);

    if (usb_debug >= 2)
      fprintf(stderr, "usb_os_find_devices: Found %s\n",
              dev->filename);
  }
  closedir(bfd);

  *devices = fdev;

  return 0;
}

int usb_os_determine_children(struct usb_bus *bus)
{
  /* Nothing yet */
  return 0;
}

void usb_os_init(void)
{
  /* nothing */
}

int usb_resetep(usb_dev_handle *dev, unsigned int ep)
{
  /* Not yet done, because I haven't needed it. */

  USB_ERROR_STR(-ENOSYS, "usb_resetep called, unimplemented on Syllable");
}

int usb_clear_halt(usb_dev_handle *dev, unsigned int ep)
{
  int ret;

  ret = ioctl(dev->fd, USB_CLEAR_HALT, &ep);
  if (ret < 0)
    USB_ERROR_STR(-errno, "could not clear halt status on endpoint %d: %s", ep,
                  strerror(errno));

  return ret;
}

int usb_reset(usb_dev_handle *dev)
{
  /* Not yet done, because I haven't needed it. */

  USB_ERROR_STR(-ENOSYS, "usb_reset called, unimplemented on Syllable");
}

