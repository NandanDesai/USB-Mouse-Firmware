/** @file   main.c
 *  @note   User-space Mouse control code for nRF52840 board
**/

#include <lib642.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNUSED __attribute__((unused))

/** @brief size of the user input buffer */
#define USER_BUF_MAX 2

/** @brief thread user space stack size - 4KB */
#define USR_STACK_WORDS 512
#define NUM_THREADS 2
#define NUM_MUTEXES 0
#define CLOCK_FREQUENCY 600

/** @brief mouse actions impelemented:
 * 'a': move mouse pointer left
 * 'd': move mouse pointer right
 * 'w': move mouse pointer up
 * 's': move mouse pointer down
 * 't': scroll mouse wheel up
 * 'g': scroll mouse wheel down
 * 'q': click left mouse button
 * 'e': click right mouse button
 */
char mouse_actions[][10] = {
    "a", "d", "w", "s", "t", "g", "q", "e"
};

enum ENUM_MOUSE_ACTIONS{LEFT = 0, RIGHT, UP, DOWN, SUP, SDOWN, LCLICK, RCLICK};

/** @brief buffer to hold user input */
char user_cmd_buffer[USER_BUF_MAX] = "z";
/** @brief current index in user buffer */
int user_cmd_i = 0;

/**
 * @name clear_user_buffer
 * @brief empty the user buffer 
 */
void clear_user_buffer(){
    for(int i = 0; i < USER_BUF_MAX; i++){
        user_cmd_buffer[i] = '\0';
    }
    user_cmd_i = 0;
}

/** @name thread_0_keypress
 * @brief thread which reads user input  
 * @note T0:(10, 65)
 */
void thread_0_keypress() {
    while(1){
        if(user_cmd_i == USER_BUF_MAX){
            clear_user_buffer();   
        }
        unsigned char input = 0;
        int status = read(STDIN_FILENO, &input, 1);
        if(input == '\n'){
            clear_user_buffer();
        }
        if(status > 0 && (char) input >= 'a' && (char) input <= 'z'){
            user_cmd_buffer[user_cmd_i] = (char) input;
            user_cmd_i++;
            user_cmd_buffer[user_cmd_i] = '\0';
        }
        wait_until_next_period();
    }
}

/** @name thread_1_mouse_evt
 * @brief thread which performs mouse action based on user input  
 * @note T1:(40, 65)
 */
void thread_1_mouse_evt() {
    while(1){
        for(int i = 0; i < 8; i++){
            if(user_cmd_buffer[0] != mouse_actions[i][0]){
                continue;
            }
            switch(i){
                case LEFT:
                    mouse_move(-10, 0);
                    break;
                case RIGHT:
                    mouse_move(10, 0);
                    break;
                case UP:
                    mouse_move(0, -10);
                    break;
                case DOWN:
                    mouse_move(0, 10);
                    break;
                case SUP:
                    mouse_scroll(3);
                    break;
                case SDOWN:
                    mouse_scroll(-3);
                    break;
                case LCLICK:
                    mouse_click(1); // mouse button pressed
                    mouse_click(0); // mouse button unpressed (pressing and unpressing emulates a "click")
                    break;
                case RCLICK:
                    mouse_click(2);
                    mouse_click(0);
                    break;
                default:
                    break;
            }
            clear_user_buffer();
            break;
        }
        wait_until_next_period();
    }
}

/**
 * @name main
 * @brief runs the mouse application control logic 
 */
int main(UNUSED int argc, UNUSED const char* argv[]) {
    ABORT_ON_ERROR(thread_init(NUM_THREADS, USR_STACK_WORDS, NULL, KERNEL_ONLY, NUM_MUTEXES));

    ABORT_ON_ERROR(thread_create(&thread_0_keypress, 0, 10, 65, NULL));
    ABORT_ON_ERROR(thread_create(&thread_1_mouse_evt, 1, 40, 65, NULL));

    printf("Starting scheduler...\n");

    ABORT_ON_ERROR(scheduler_start(CLOCK_FREQUENCY));

    return 0;
}
