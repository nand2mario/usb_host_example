// Simple firmware for testing USB gamepad
// nand2mario, 2024.1
//
// Needs xpack-gcc risc-v gcc: https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/

#include <stdbool.h>
#include <stdint.h>
#include "firmware.h"
#include "minlibc.h"
#include "usb_core.h"
#include "usb_hw.h"

bool status;                // 0: idle, 1: connected and ready, 2: initliazation failure
struct usb_device dev;      // the actual device
int dev_type = 0;           // 0: unknown, 1: Xbox360 wireless, 2: Xbox360 wired
int num_interfaces;

#define XBOX360_WIRELESS 1
#define XBOX360_WIRED 2

#define STATUS_IDLE 0
#define STATUS_READY 1
#define STATUS_FAILURE 2

int enum_interface(struct usb_device *dev, struct usb_interface *intp) {
    // uart_printf("  Interface %d: class=%d, subclass=%d, protocol=%d\n", intp - dev->interfaces, 
    //             intp->if_class, intp->if_subclass, intp->if_protocol);
    num_interfaces++;
    return 1;
}

int enum_class_specific(struct usb_device *dev, struct usb_interface *intp, void *desc) {
    // uart_printf("  Class-specific interface %d, class=%d, subclass=%d, protocol=%d\n", intp - dev->interfaces,
    //             intp->if_class, intp->if_subclass, intp->if_protocol);
    return 1;
}


// Configure and enumerate USB device after it is connected
// return 1 if successful
int init_usb_device() {
    int r = 0;
    uart_print("USB device configuration\n");
    if (usb_configure_device(&dev, 1)) {
        uart_print("USB device configured\n");
    } else {
        uart_print("USB device configuration failed\n");
        goto end;
    }

    uart_print("\nEnumerating interfaces:\n");
    if (!usb_enumerate(&dev, enum_interface, enum_class_specific)) {
        uart_print("USB device enumeration failed\n");
        goto end;
    }

    uart_printf("\nUSB enumeration done.");
    char buf[64];
    char *s = usb_get_string(&dev, dev.str_manufacturer, buf, 64);
    uart_printf("  Manufacturer: %s\n", s);
    s = usb_get_string(&dev, dev.str_product, buf, 64);
    uart_printf("  Product: %s\n", s);

    uart_print("Device interfaces:\n");

    for (int i = 0; i < num_interfaces; i++) {
        struct usb_interface *intp = &dev.interfaces[i];
        uart_printf("Interface %d: class=%d, subclass=%d, protocol=%d\n", i,
                    intp->if_class, intp->if_subclass, intp->if_protocol);
        for (int i = 0; i < intp->if_endpoints; i++) {
            struct usb_endpoint *ep = &intp->endpoint[i];
            char *dir = ep->direction == 0 ? "OUT" : "IN";
            char *type = ep->endpoint_type == 0 ? "Control" : ep->endpoint_type == 1 ? "Isochronous" :
                        ep->endpoint_type == 2 ? "Bulk" : "Interrupt";
            uart_printf("  Endpoint %d: type=%s, dir=%s, max_packet_size=%d, interval=%d\n", ep->endpoint,
                        type, dir, ep->max_packet_size, ep->interval);
        }
    }

    uart_printf("\nDEVICE READY: ");
    if (dev.vid == 0x45e && dev.pid == 0x719) {
        dev_type = XBOX360_WIRELESS;
        uart_printf("Xbox360 wireless\n");
    } else if (dev.vid == 0x45e && dev.pid == 0x28e) {
        dev_type = XBOX360_WIRED;
        uart_printf("Xbox360 wired\n");
    } else {
        uart_printf("Unknown\n");
    }

    r = 1;

end:
    return r;
}

void poll_x360_wireless()
{
    uint8_t buf[32];
    uint8_t response;
    // poll P1
    struct usb_endpoint *ep = &dev.interfaces[0].endpoint[0];
    int r = usbhw_transfer_in(PID_IN, dev.address, ep->endpoint, &response, buf, 32);
    if (r > 2 && buf[1] == 0x1)
    { // P1 status event
        uart_printf("P1: ");
        uart_printf("%c %c %c %c %c %c %c %c %c %c ",
                    buf[6] & 1 ? 'U' : '_', buf[6] & 2 ? 'D' : '_', buf[6] & 4 ? 'L' : '_', buf[6] & 8 ? 'R' : '_',
                    buf[7] & 16 ? 'A' : '_', buf[7] & 32 ? 'B' : '_', buf[7] & 64 ? 'X' : '_', buf[7] & 128 ? 'Y' : '_',
                    buf[6] & 16 ? 'S' : '_', buf[6] & 32 ? 'K' : '_');

        for (int i = 0; i < r; i++)
        {
            uart_print_hex_digits(buf[i], 2);
            uart_putchar(' ');
        }
        uart_print("\r");
    }
}

void poll_x360_wired()
{
    uint8_t buf[32];
    uint8_t response;
    // poll P1
    struct usb_endpoint *ep = &dev.interfaces[0].endpoint[0];
    int r = usbhw_transfer_in(PID_IN, dev.address, ep->endpoint, &response, buf, 32);
    if (r > 10 /* && buf[1] == 0x14 */)
    { // P1 status event
        uart_printf("P1: ");
        uart_printf("%c %c %c %c %c %c %c %c %c %c ",
                    buf[2] & 1 ? 'U' : '_', buf[2] & 2 ? 'D' : '_', buf[2] & 4 ? 'L' : '_', buf[2] & 8 ? 'R' : '_',
                    buf[3] & 16 ? 'A' : '_', buf[3] & 32 ? 'B' : '_', buf[3] & 64 ? 'X' : '_', buf[3] & 128 ? 'Y' : '_',
                    buf[2] & 16 ? 'S' : '_', buf[2] & 32 ? 'K' : '_');

        for (int i = 0; i < r; i++)
        {
            uart_print_hex_digits(buf[i], 2);
            uart_putchar(' ');
        }
        uart_print("\r");
    }
}


void set_led()
{
    uart_printf("Xbox360 wireless LED setup\n");
    /*
    0x00	 All off
    0x01	 All blinking
    0x02	 1 flashes, then on
    0x03	 2 flashes, then on
    0x04	 3 flashes, then on
    0x05	 4 flashes, then on
    0x06	 1 on
    0x07	 2 on
    0x08	 3 on
    0x09	 4 on
    0x0A	 Rotating (e.g. 1-2-4-3)
    0x0B	 Blinking*
    0x0C	 Slow blinking*
    0x0D	 Alternating (e.g. 1+4-2+3), then back to previous*
    */

    struct usb_endpoint *ep = &dev.interfaces[0].endpoint[1]; // P1 control port
    uint8_t buf[] = {0x00, 0x00, 0x08, 0x40 + 6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int r = usbhw_transfer_out(PID_OUT, dev.address, ep->endpoint, 1, ep->data_toggle ? PID_DATA1 : PID_DATA0, buf, sizeof(buf));
    ep->data_toggle = !ep->data_toggle;
    // int r = usb_bulk_write(ep, buf, sizeof(buf), 2);

    ep = &dev.interfaces[2].endpoint[1]; // P2 control port
    buf[3] = 0x40 + 7;
    // r = usb_bulk_write(ep, buf, sizeof(buf), 2);
    r = usbhw_transfer_out(PID_OUT, dev.address, ep->endpoint, 1, ep->data_toggle ? PID_DATA1 : PID_DATA0, buf, sizeof(buf));
    ep->data_toggle = !ep->data_toggle;
}

void request_endp1()
{
    uint8_t buf[64];
    uint8_t response;
    // interface 0, endpoint 0
    // int r = usb_bulk_read(&dev.interfaces[0].endpoint[0], buf, 32, 2);
    uart_print("USB data request\n");
    struct usb_endpoint *ep = &dev.interfaces[0].endpoint[0];
    int r = usbhw_transfer_in(PID_IN, dev.address, ep->endpoint, &response, buf, 32);
    uart_printf("  r=%d, response=%d\n", r, response);

    if (r >= USB_RES_OK)
    {
        uart_print("  USB data received: ");
        for (int i = 0; i < r; i++)
        {
            uart_print_hex_digits(buf[i], 2);
            uart_putchar(' ');
        }
        uart_putchar('\n');
    }
}



// main

int main() {

    reg_uart_clkdiv = 417; // 48000000 / 115200;
    uart_print("Firmware started\n");

    usbhw_init(0x02000100);
    usbhw_reset();
    uart_print("USB host hardware initialized\n");

    bool polling = true;

    uart_print("\nUSB Gamepad Test Program. The following devices have been tested.\n");
    uart_print("  Xbox360 wireless controller\n");
    uart_print("  Xbox360 wired controller\n");
    uart_print("  8bitdo 3SN30pro wired controller\n");
    uart_print("Press 1 to request USB data, 2 to set LED, 3 to start/stop polling\n");
    int c;
    int led = 1;

    while (1) {                 // main loop

        if (status == STATUS_IDLE && usbhw_hub_device_detected()) {
            uart_printf("USB device detected\n");
            if (init_usb_device()) {
                status = STATUS_READY;
            } else {
                status = STATUS_FAILURE;
            }
        } else if (status != STATUS_IDLE && !usbhw_hub_device_detected()) {
            uart_printf("USB device removed\n");
            usbhw_reset();
            status = STATUS_IDLE;
            dev_type = 0;
        }

        if (status != STATUS_READY) {
            delay(10);
            continue;
        }

        c = uart_getchar();
        if (c == '1') {
            request_endp1();
        } else if (c == '2') {
            set_led();
        } else if (c == '3') {
            if (dev_type > 0) {
                polling = !polling;
                if (polling)
                    uart_printf("polling started\n");
                else
                    uart_printf("polling stopped\n");
            } else
                uart_printf("Unknown device, vid=%04x, pid=%04x\n", dev.vid, dev.pid);
        }

        if (polling) {
            if (dev_type == XBOX360_WIRELESS)
                poll_x360_wireless();
            else if (dev_type == XBOX360_WIRED)
                poll_x360_wired();
        }
    }

    uart_printf("Program ended\n");
    while (1) {}
}
