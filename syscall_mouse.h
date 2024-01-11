/** @file   mouse.h
 *
 *  @brief  function prototypes for Mouse actions
 *  @note   Not for public release, do not share
 *
**/
#include <unistd.h>

#ifndef _SYSCALL_MOUSE_H_
#define _SYSCALL_MOUSE_H_

/** @brief syscall to move mouse by specified coordinates */
void sys_mouse_move(int8_t x, int8_t y);

/** @brief syscall to scroll mouse */
void sys_mouse_scroll(int8_t wheel);

/** @brief syscall to emulate mouse left or right click */
void sys_mouse_click(uint8_t button);

#endif /* _SYSCALL_MOUSE_H_ */