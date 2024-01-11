/** @file   usbd.h
 *  @brief  function prototypes for USBD
**/

#include <unistd.h>

#ifndef _USBD_H_
#define _USBD_H_

/** @brief types of USB device descriptors */
enum DESC_TYPE{DEVICE = 1, CONFIG, STRING, INTERFACE, ENDPOINT};

/** @brief indicates whether the "USB host" is ready to receive mouse reports (ie. actions) */
extern volatile uint32_t MOUSE_READY;

/** @brief struct for DEVICE descriptor 
 * 
 * NOTE: "__attribute__((__packed__))" is an attribute specific to GCC. 
 * Using this here to not allow padding/alignment for the structs.
 * Hosts accepts the value without padding
*/
typedef struct __attribute__((__packed__)){
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
}device_desc_t;

/** @brief struct for CONFIGURATION descriptor */
typedef struct __attribute__((__packed__)){
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
}_config_desc_t;

/** @brief struct for INTERFACE descriptor */
typedef struct __attribute__((__packed__)){
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
}_interface_desc_t;

/** @brief struct for HID descriptor */
typedef struct __attribute__((__packed__)){
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdHID;
    uint8_t bCountryCode;
    uint8_t bNumDescriptors;
    uint8_t bDescriptorType2;
    uint16_t wDescriptorLength;
}_hid_desc_t;

/** @brief struct for ENDPOINT descriptor */
typedef struct __attribute__((__packed__)){
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
}_endpoint_desc_t;

/** @brief combined struct which includes CONFIG, INTERFACE, HID, and ENDPOINT descriptors */
typedef struct __attribute__((__packed__)){
    _config_desc_t config;
    _interface_desc_t interface;
    _hid_desc_t hid;
    _endpoint_desc_t endpoint;
}configuration_desc_t;

/** @brief struct for each HID input report (ie. the mouse actions) */
typedef struct __attribute__((__packed__)){
    uint8_t buttons;
    int8_t X;
    int8_t Y;
    int8_t Wheel;
}input_report_t;

/** @brief maximum packet size for the USB communication */
#define MAX_PACKET_SIZE 64

/********************************** USBD Global **********************************/

/** @brief USBD registers */
#define USBD (volatile uint32_t*) 0x40027000
#define USBD_ENABLE (volatile uint32_t*) (0x40027000 + 0x500)
#define USBD_USBPULLUP (volatile uint32_t*) (0x40027000 + 0x504)
#define USBD_EPINEN (volatile uint32_t*) (0x40027000 + 0x510)
#define USBD_EPOUTEN (volatile uint32_t*) (0x40027000 + 0x514)
#define USBD_INTENSET (volatile uint32_t*) (0x40027000 + 0x304)
#define USBD_INTENCLR (volatile uint32_t*) (0x40027000 + 0x308)
#define USBD_USBADDR (volatile uint32_t*) (0x40027000 + 0x470)

/** @brief USBD Generic Events (ie. for all endpoints) */
#define USBD_EVENTS_USBEVENT (volatile uint32_t*) (0x40027000 + 0x158)
#define USBD_EVENTCAUSE (volatile uint32_t*) (0x40027000 + 0x400)
#define USBD_EVENTS_STARTED (volatile uint32_t*) (0x40027000 + 0x104)
#define USBD_EVENTS_EPDATASTATUS (volatile uint32_t*) (0x40027000 + 0x46C)
#define USBD_EVENTS_EPDATA (volatile uint32_t*) (0x40027000 + 0x160)


/********************************** CONTROL TRANSFER **********************************/

/** @brief ENDPOINT-0 registers */
#define USBD_EPIN0_PTR (volatile uint8_t**) (0x40027000 + 0x600 + (0 * 0x14))
#define USBD_EPIN0_MAXCNT (volatile uint32_t*) (0x40027000 + 0x604 + (0 * 0x14))
#define USBD_EPIN0_AMOUNT (volatile uint32_t*) (0x40027000 + 0x608 + (0 * 0x14))

/** @brief ENDPOINT-0 Tasks & Events */
#define USBD_TASKS_STARTEPIN0 (volatile uint32_t*) (0x40027000 + 0x004 + (0 * 0x4))
#define USBD_TASKS_EP0STATUS (volatile uint32_t*) (0x40027000 + 0x050)
#define USBD_EVENTS_USBRESET (volatile uint32_t*) (0x40027000 + 0x100)
#define USBD_EVENTS_ENDEPIN0 (volatile uint32_t*) (0x40027000 + 0x108 + (0 * 0x4))
#define USBD_EVENTS_EP0SETUP (volatile uint32_t*) (0x40027000 + 0x15C)
#define USBD_EVENTS_EP0DATADONE (volatile uint32_t*) (0x40027000 + 0x128)

/** @brief ENDPOINT-0 registers that store SETUP data */
#define USBD_BMREQUESTTYPE (volatile uint32_t*) (0x40027000 + 0x480)
#define USBD_BREQUEST (volatile uint32_t*) (0x40027000 + 0x484)
#define USBD_WVALUEL (volatile uint32_t*) (0x40027000 + 0x488)
#define USBD_WVALUEH (volatile uint32_t*) (0x40027000 + 0x48C)
#define USBD_WLENGTHL (volatile uint32_t*) (0x40027000 + 0x498)
#define USBD_WLENGTHH (volatile uint32_t*) (0x40027000 + 0x49C)
#define USBD_WINDEXL (volatile uint32_t*) (0x40027000 + 0x494)
#define USBD_WINDEXH (volatile uint32_t*) (0x40027000 + 0x49C)

/********************************** INTERRUPT TRANSFER **********************************/

/** @brief ENDPOINT-1 Tasks & Events */
#define USBD_TASKS_STARTEPIN1 (volatile uint32_t*) (0x40027000 + 0x004 + (1 * 0x4))
#define USBD_EVENTS_ENDEPIN1 (volatile uint32_t*) (0x40027000 + 0x108 + (1 * 0x4))

/** @brief ENDPOINT-1 registers */
#define USBD_EPIN1_PTR (volatile uint8_t**) (0x40027000 + 0x600 + (1 * 0x14))
#define USBD_EPIN1_MAXCNT (volatile uint32_t*) (0x40027000 + 0x604 + (1 * 0x14))
#define USBD_EPIN1_AMOUNT (volatile uint32_t*) (0x40027000 + 0x608 + (1 * 0x14))

/********************************** POWER & CLOCK **********************************/

/** @brief POWER registers */
#define POWER (volatile uint32_t*) 0x40000000
#define POWER_EVENTS_USBDETECTED (volatile uint32_t*) (0x40000000 + 0x11C)
#define POWER_EVENTS_USBREMOVED (volatile uint32_t*) (0x40000000 + 0x120)
#define POWER_EVENTS_USBPWRRDY (volatile uint32_t*) (0x40000000 + 0x124)
#define POWER_INTENSET (volatile uint32_t*) (0x40000000 + 0x304)
#define POWER_INTENCLR (volatile uint32_t*) (0x40000000 + 0x308)

/** @brief CLOCK registers */
#define CLOCK (volatile uint32_t*) 0x40000000
#define CLOCK_TASKS_HFCLKSTART (volatile uint32_t*) (0x40000000 + 0x000)
#define CLOCK_TASKS_HFCLKSTOP (volatile uint32_t*) (0x40000000 + 0x004)
#define CLOCK_EVENTS_HFCLKSTARTED (volatile uint32_t*) (0x40000000 + 0x100)
#define CLOCK_HFCLKSTAT (volatile uint32_t*) (0x40000000 + 0x40C)

/** @brief initialize USBD */
void usbd_init();

/** @brief send data from USB device to USB */
void send_data(uint8_t endpoint, uint8_t* buffer_ptr, uint32_t total_size, uint16_t data_size);

#endif