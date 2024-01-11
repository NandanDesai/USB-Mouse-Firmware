/** 
 *  @file   syscall_mouse.c
 *  @note   This file contains the REPORT protocol to control the mouse.
**/

#include <usbd.h>
#include <syscall_mouse.h>
#include <printk.h>

/** @name sys_mouse_move
 * @brief syscall to move mouse by specified coordinates 
 * @param  x  coordinates to move mouse horizontally
 * @param  y  coordinates to move mouse vertically
 */
void sys_mouse_move(int8_t x, int8_t y){
    while(MOUSE_READY != 1){
        // wait for the USB to initialize and mouse to get ready
    }
    input_report_t input_report;
    input_report.buttons = (0x0);
    input_report.X = x;
    input_report.Y = y;
    input_report.Wheel = 0;
    send_data(1, (uint8_t*)&input_report, sizeof(input_report_t), sizeof(input_report_t));
}

/** @name sys_mouse_scroll
 * @brief syscall to perform mouse scroll action 
 * @param  wheel  +ve value to scroll mouse up; -ve value to scroll down
 */
void sys_mouse_scroll(int8_t wheel){
    while(MOUSE_READY != 1){
        // wait for the USB to initialize and mouse to get ready
    }
    input_report_t input_report;
    input_report.buttons = (0x0);
    input_report.X = 0;
    input_report.Y = 0;
    input_report.Wheel = wheel;
    send_data(1, (uint8_t*)&input_report, sizeof(input_report_t), sizeof(input_report_t));
}

/** @name sys_mouse_click
 * @brief syscall to perform a mouse button click action 
 * @param  button  1 to left click; 2 to right click
 *          
 */
void sys_mouse_click(uint8_t button){
    while(MOUSE_READY != 1){
        // wait for the USB to initialize and mouse to get ready
    }
    input_report_t input_report;
    input_report.buttons = button; // | 0x8
    input_report.X = 0;
    input_report.Y = 0;
    input_report.Wheel = 0;
    send_data(1, (uint8_t*)&input_report, sizeof(input_report_t), sizeof(input_report_t));
}

