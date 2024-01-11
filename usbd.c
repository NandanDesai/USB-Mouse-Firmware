/**
 * @file usbd.c
 * @name This file contains code to control the USB device on the board.
*/

#include<usbd.h>
#include<printk.h>
#include<arm.h>

/** @brief Enable USBD and POWERCLCK interrupts */
#define NVIC_ISER0 (volatile uint32_t*) 0xE000E100
#define NVIC_ISER1 (volatile uint32_t*) 0xE000E104

/** @brief mouse device descriptor */
device_desc_t mouse_dev_desc;
/** @brief mouse configuration descriptor */
configuration_desc_t mouse_config_desc;
/** @brief The "USB host" is ready to receive mouse reports (ie. actions) when this value becomes 0x1 */
volatile uint32_t MOUSE_READY = 0x0;

/**
 * @brief HID report descriptor of our mouse 
 * Reference: https://www.usbmadesimple.co.uk/ums_5.htm
 * 
*/
uint8_t hidReportDescriptor [] = 
{
    0x05, 0x01,    // UsagePage(Generic Desktop Controls)
    0x09, 0x02,    // Usage (Mouse)
    0xA1, 0x01,    //     Collection(Application)
    0x09, 0x01,    //     Usage(Pointer)
    0xA1, 0x00,    //       Collection(Physical)
    0x05, 0x09,    //         Usage Page(Button)
    0x19, 0x01,    //         Usage Minimum(1)
    0x29, 0x05,    //         Usage Maximum(5)
    0x15, 0x00,    //         Logical Minimum(0)
    0x25, 0x01,    //         Logical Maximum(1)
    0x95, 0x05,    //         Report Count (5)
    0x75, 0x01,    //         Report Size (1)
    0x81, 0x02,    //         Input (Data, Variable, Absolute, Bit Field)
    0x95, 0x01,    //         Report Count (1)
    0x75, 0x03,    //         Report Size (3)
    0x81, 0x01,    //         Input (Constant, Array, Absolute, Bit Field)
    0x05, 0x01,    //         Usage Page (Generic Desktop Controls)
    0x09, 0x30,    //         Usage (X)
    0x09, 0x31,    //         Usage (Y)
    0x09, 0x38,    //         Usage (Wheel)
    0x15, 0x81,    //         Logical Minimum (-127)
    0x25, 0x7F,    //         Logical Maximum (127)
    0x75, 0x08,    //         Report Size (8)
    0x95, 0x03,    //         Report Count (3)
    0x81, 0x06,    //         Input (Data, Variable, Relative, Bit Field)
    0xC0,          //       EndCollection()
    0xC0,          //    EndCollection()
};

/** @name usbd_init
 * @brief initialize USBD 
 * 
*/
void usbd_init(){
    // enable Power and Clock interrupt handler
    *NVIC_ISER0 |= (0x1);
    // enable USBD interrupt handler
    *NVIC_ISER1 |= (1 << 7);
    // enable interrupts for USBDETECTED and USBREMOVED events
    *POWER_INTENSET |= (0x3 << 7);

    // enable interrupts for USBRESET, EP0SETUP, USBEVENT //and EPDATA events
    *USBD_INTENSET |= (0x1 | (0x1 << 23) | (0x1 << 22)); //| (0x1 << 24)
}

/** @name POWER_CLOCK_IRQHandler
 * @brief Handles POWER or CLOCK related interrupts
 * @note  This is the starting point of our USBD
*/
void POWER_CLOCK_IRQHandler(){
    if(*POWER_EVENTS_USBDETECTED == 1){
        /**
         * Refer: https://infocenter.nordicsemi.com/pdf/nRF52840_Rev_2_Errata_v1.5.pdf
         * Errata [187] USBD: USB cannot be enabled
         * 
        */
        *(volatile uint32_t *)0x4006EC00 = 0x00009375;
        *(volatile uint32_t *)0x4006ED14 = 0x00000003;
        *(volatile uint32_t *)0x4006EC00 = 0x00009375;

        *USBD_ENABLE = 0x1;
        *CLOCK_TASKS_HFCLKSTART = 0x1;
        
        while(*USBD_EVENTS_USBEVENT != 1 && ((*USBD_EVENTCAUSE & (1 << 11)) >> 11) != 1){
            // wait for USBEVENT and EVENTCAUSE=READY event
        }
        // clear the previous events
        *USBD_EVENTS_USBEVENT = 0x0;
        *USBD_EVENTCAUSE = (0x0 << 11);

        *(volatile uint32_t *)0x4006EC00 = 0x00009375;
        *(volatile uint32_t *)0x4006ED14 = 0x00000000;
        *(volatile uint32_t *)0x4006EC00 = 0x00009375;

        while(*POWER_EVENTS_USBPWRRDY != 1){
            // wait for USBPWRRDY event
        }
        *POWER_EVENTS_USBPWRRDY = 0x0;

        while(*CLOCK_EVENTS_HFCLKSTARTED != 1){
            // wait for HFCLKSTARTED event
        }
        *CLOCK_EVENTS_HFCLKSTARTED = 0x0;

        *USBD_USBPULLUP = 0x1; 

        // Endpoint IN enable for endpoint 1
        *USBD_EPINEN |= (0x1 << 1);

        *POWER_EVENTS_USBDETECTED = 0x0;

        MOUSE_READY = 0x0;

        printk("USBD initialized!\n");
    }else if(*POWER_EVENTS_USBREMOVED == 1){
        MOUSE_READY = 0x0;
        printk("USBD removed!\n");
    }
}

/** @name send_data
 * @brief Transfers data from USB "device" to "host"
 * @param endpoint     the endpoint to use for data transfer
 * @param buffer_ptr   pointer to your transfer buffer
 * @param total_size   total size of the buffer being sent  
 * @param data_size    size specified by the host (available in usbd_wlength register)
*/
void send_data(uint8_t endpoint, uint8_t* buffer_ptr, uint32_t total_size, uint16_t data_size){
    // Errata #199: USBD cannot receive tasks during DMA
    *(volatile uint32_t *)0x40027C1C = 0x00000082;

    uint32_t data_sent = 0;
    uint32_t data_remaining = 0;
    if(total_size < data_size){
        data_remaining = total_size;
    }else{
        data_remaining = data_size;
    }
    while(data_remaining > 0){ 
        switch(endpoint){
            case 0:{
                *USBD_EPIN0_PTR = (uint8_t*)((uint32_t)buffer_ptr + data_sent);
                if(data_remaining < MAX_PACKET_SIZE){
                    *USBD_EPIN0_MAXCNT = data_remaining;
                }else{
                    *USBD_EPIN0_MAXCNT = MAX_PACKET_SIZE;
                }
                *USBD_TASKS_STARTEPIN0 = 0x1; // start 'data stage'
                while(*USBD_EVENTS_ENDEPIN0 != 1){
                    //wait for ENDEPIN event
                }
                *USBD_EVENTS_ENDEPIN0 = 0x0;
                while(*USBD_EVENTS_EP0DATADONE != 1){

                }
                *USBD_EVENTS_EP0DATADONE = 0x0;
                data_sent = data_sent + *USBD_EPIN0_AMOUNT;
                data_remaining = data_remaining - *USBD_EPIN0_AMOUNT;
                break;
            }
            case 1:{
                *USBD_EPIN1_PTR = (uint8_t*)((uint32_t)buffer_ptr + data_sent);
                if(data_remaining < MAX_PACKET_SIZE){
                    *USBD_EPIN1_MAXCNT = data_remaining;
                }else{
                    *USBD_EPIN1_MAXCNT = MAX_PACKET_SIZE;
                }
                *USBD_TASKS_STARTEPIN1 = 0x1; // start 'data stage'
                while(*USBD_EVENTS_ENDEPIN1 != 1){
                    //wait for ENDEPIN event
                }
                *USBD_EVENTS_ENDEPIN1 = 0x0;
                while((((*USBD_EVENTS_EPDATASTATUS) & 0x2) >> 1) != 1){
                
                }
                *USBD_EVENTS_EPDATASTATUS |= (0x0 << 1);
                data_sent = data_sent + *USBD_EPIN1_AMOUNT;
                data_remaining = data_remaining - *USBD_EPIN1_AMOUNT;
                break;
            }
            default:
                printk("[Error] Enpoint %d not supported\n", endpoint);
                return;
        }
    }
    if(endpoint == 0){
        // enter 'status' stage for endpoint 0
        *USBD_TASKS_EP0STATUS = 0x1;
    }
    //printk("Data of size %d successfully sent to host on Endpoint %d\n", data_sent, endpoint);
    // Errata #199: USBD cannot receive tasks during DMA
    *(volatile uint32_t *)0x40027C1C = 0x00000000;
}

/** @name get_device_desc
 * @brief responds to GET_DEVICE_DESCRIPTOR request from the host
 * @param data_size    size specified by the host (available in usbd_wlength register)
*/
void get_device_desc(uint16_t data_size){
    printk("Inside get_device_desc\n");
    mouse_dev_desc.bLength = sizeof(device_desc_t);
    mouse_dev_desc.bDescriptorType = 1;
    mouse_dev_desc.bcdUSB = 0x0110;
    mouse_dev_desc.bDeviceClass = 0x00;
    mouse_dev_desc.bDeviceSubClass = 0x00;
    mouse_dev_desc.bDeviceProtocol = 0x00;
    mouse_dev_desc.bMaxPacketSize0 = MAX_PACKET_SIZE;
    mouse_dev_desc.idVendor = 0x0F62;
    mouse_dev_desc.idProduct = 0x1001;
    mouse_dev_desc.bcdDevice = 0x0001;
    mouse_dev_desc.iManufacturer = 0;
    mouse_dev_desc.iProduct = 0;
    mouse_dev_desc.iSerialNumber = 0;
    mouse_dev_desc.bNumConfigurations = 1;

    send_data(0, (uint8_t*)&mouse_dev_desc, sizeof(device_desc_t), data_size);

    printk("Finished get_device_desc\n");
}

/** @name get_config_desc
 * @brief responds to GET_CONFIG_DESCRIPTOR request from the host
 * @param data_size    size specified by the host (available in usbd_wlength register)
*/
void get_config_desc(uint16_t data_size){
    printk("Inside get_config_desc\n");
    // config descriptor
    mouse_config_desc.config.bLength = sizeof(_config_desc_t);
    mouse_config_desc.config.bDescriptorType = 2;
    mouse_config_desc.config.wTotalLength = (uint16_t) sizeof(configuration_desc_t);
    mouse_config_desc.config.bNumInterfaces = 1;
    mouse_config_desc.config.bConfigurationValue = 1;
    mouse_config_desc.config.iConfiguration = 0;
    mouse_config_desc.config.bmAttributes = 0b11000000;
    mouse_config_desc.config.bMaxPower = 0;

    // interface descriptor
    mouse_config_desc.interface.bLength = sizeof(_interface_desc_t);
    mouse_config_desc.interface.bDescriptorType = 4;
    mouse_config_desc.interface.bInterfaceNumber = 0;
    mouse_config_desc.interface.bAlternateSetting = 0;
    mouse_config_desc.interface.bNumEndpoints = 1;
    mouse_config_desc.interface.bInterfaceClass = 0x03; //HID
    mouse_config_desc.interface.bInterfaceSubClass = 0x01;
    mouse_config_desc.interface.bInterfaceProtocol = 0x02; //Mouse
    mouse_config_desc.interface.iInterface = 0;

    // HID descriptor
    mouse_config_desc.hid.bLength = sizeof(_hid_desc_t);
    mouse_config_desc.hid.bDescriptorType = 0x21;
    mouse_config_desc.hid.bcdHID = 0x0110;
    mouse_config_desc.hid.bCountryCode = 0;
    mouse_config_desc.hid.bNumDescriptors = 1;
    mouse_config_desc.hid.bDescriptorType2 = 34;
    mouse_config_desc.hid.wDescriptorLength = sizeof(hidReportDescriptor);

    // endpoint descriptor
    mouse_config_desc.endpoint.bLength = sizeof(_endpoint_desc_t);
    mouse_config_desc.endpoint.bDescriptorType = 5;
    mouse_config_desc.endpoint.bEndpointAddress = 0x81;
    mouse_config_desc.endpoint.bmAttributes = 0x03;
    mouse_config_desc.endpoint.wMaxPacketSize = sizeof(input_report_t); // size of mouse REPORT packet
    mouse_config_desc.endpoint.bInterval = 0x0A; //10ms

    send_data(0, (uint8_t*)&mouse_config_desc, sizeof(configuration_desc_t), data_size);

    printk("Finished get_config_desc\n");
}

/** @name get_descriptor
 * @brief When GET_DESCRIPTOR request is received, calls the specific descriptor being requested
 * @param desc_type    type of descriptor requested by host
 * @param data_size    descriptor size specified by the host (available in usbd_wlength register)
*/
void get_descriptor(uint16_t desc_type, uint16_t data_size){
    printk("Desc_Type: %d, Data Size: %d\n", desc_type, data_size);
    switch(desc_type){
        case DEVICE:
            get_device_desc(data_size);
            break;
        case CONFIG:
            //breakpoint();
            get_config_desc(data_size);
            break;
        case STRING:
            //breakpoint();
            // no strings to send. Proceed to STATUS stage
            *USBD_TASKS_EP0STATUS = 0x1;
            break;
        case INTERFACE:
            breakpoint();
            break;
        case ENDPOINT:
            breakpoint();
            break;
        default:
            break;
    }

}

/** @name set_address
 * @brief SET_ADDRESS request is received from host
 * @param address    the new address assigned to the USB device
*/
void set_address(uint16_t address){
    printk("Set Address: %d\n", address);
    printk("Device Addr: %d\n", *USBD_USBADDR);
    *USBD_EVENTCAUSE |= *USBD_EVENTCAUSE;
    *USBD_EVENTS_USBEVENT = 0x0;
}

/** @name  usbd_enumeration
 * @brief Initialize USBD stack (USB enumeration) 
*/
void usbd_enumeration(){
    uint8_t request_type = (*USBD_BMREQUESTTYPE & 0xFF);
    uint8_t request = (*USBD_BREQUEST & 0xFF);
    uint16_t w_value = 0;
    uint16_t w_length = (*USBD_WLENGTHL & 0xFF)  | ((*USBD_WLENGTHH & 0xFF) << 8);
    printk("device_addr: %d, request_type: 0x%x, request: 0x%x, w_valueh (descriptor_type): 0x%x, w_valuel (descriptor_index): %d, w_length (descriptor_length): %d, w_index (interface_number): %d\n",*USBD_USBADDR, request_type, request, (*USBD_WVALUEH & 0xFF), ((*USBD_WVALUEL & 0xFF)), w_length, (*USBD_WINDEXL & 0xFF)  | ((*USBD_WINDEXH & 0xFF) << 8));
    if(request_type == 0x80){
        // respond to GET_DESCRIPTOR
        if(request == 0x6){
            w_value = (*USBD_WVALUEH & 0xFF);
            get_descriptor(w_value, w_length);
        }else{
            printk("REQUEST (Device to Host): %d\n", request);
        }
    }else if(request_type == 0x0){
        if(request == 0x5){
            // SET_ADDRESS
            w_value = (*USBD_WVALUEL & 0xFF)  | ((*USBD_WVALUEH & 0xFF) << 8);
            set_address(w_value);
        }else{
            printk("REQUEST (Host to Device): %d\n", request);
            // No data to send. Proceed to STATUS stage
            *USBD_TASKS_EP0STATUS = 0x1;
        }
    }else if(request_type == 0x81 && request == 0x6){
        printk("Request received for HID Report Descriptor\n");
        send_data(0, (uint8_t*)hidReportDescriptor, sizeof(hidReportDescriptor), w_length);
        MOUSE_READY = 0x1;
    }else{
        printk("[ERROR] Unrecognized request_type: %d\n", request_type);
        // Unrecognized request type, proceed to STATUS stage and see what happens
        *USBD_TASKS_EP0STATUS = 0x1;
    }
}

/** @name  USBD_IRQHandler
 * @brief USDB interrupt handler
*/
void USBD_IRQHandler(){
    if(*USBD_EVENTS_USBRESET == 1){
        *USBD_EVENTS_USBRESET = 0x0;
        //usbd_enumeration();
        printk("\nUSB_RESET received!\n");
    }else if(*USBD_EVENTS_EP0SETUP == 1){
        printk("\nEP0SETUP received!\n");
        *USBD_EVENTS_EP0SETUP = 0x0;
        usbd_enumeration();
    }else if(*USBD_EVENTS_USBEVENT == 1){
        *USBD_EVENTS_USBEVENT = 0x0;
        printk("\nUSB EVENT received! EVENTCAUSE: 0x%x\n", *USBD_EVENTCAUSE);
    }else if(*USBD_EVENTS_EPDATA == 1){
        *USBD_EVENTS_EPDATA = 0x0;
        printk("\nEPDATA received! EPDATASTATUS: %d\n", *USBD_EVENTS_EPDATASTATUS);
    }

    // clear the events after they're consumed
    *USBD_EVENTCAUSE |= *USBD_EVENTCAUSE;
    *USBD_EVENTS_USBEVENT = 0x0;
}

