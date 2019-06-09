/*
 *--------------------------------------
 * Program Name: A Game Of A Game Of Life
 * Author: slimeenergy
 * License: None, please don't steal it.
 * Description: https://www.cemetech.net/forum/viewtopic.php?p=280363
 *--------------------------------------
*/

/* Keep these headers */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

/* Standard headers (recommended) */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <graphx.h>
#include <keypadc.h>

#define GAME_NAME "Game of Game of Life"
#define FONT_HEIGHT 8
#define EMPTY_FUNC_MENU_OPTION "\0"

typedef struct {
    uint8_t width;
    uint8_t height;
    bool* cells;
    bool* selected_cells;
} level_t;

/* Put your function prototypes here */
void update();
void update_scene();

void render();
void render_scene();
void render_background();
void render_centered_string(char* str, uint16_t x, uint8_t y);
void render_level();

level_t* generate_level(uint8_t width, uint8_t height, uint16_t cell_count);
void create_new_level(uint8_t width, uint8_t height, uint16_t cell_count);
void destroy_level();
void next_level();

bool key_up_pressed();
bool key_down_pressed();
bool key_left_pressed();
bool key_right_pressed();
bool key_enter_pressed();
bool key_second_pressed();

uint8_t random_number(uint8_t minimum,uint8_t maximum);

int16_t min(int16_t a, int16_t b);
int16_t max(int16_t a, int16_t b);
/* Put all your globals here */
bool running = true; //Whether or not the main loop should keep running.
level_t* playing_level = NULL; //The level that is currently being played.
uint8_t scene = 0; //Scenes: 0 Main Menu, 1 Level Start Screen, 2 Play, 3 Win, 4 Credits, 5 How To
uint8_t cursor_pos = 0;
uint8_t cursor_pos2 = 0;
uint8_t level = 0;
//If keys are being pressed
bool up = false;
bool down = false;
bool left = false;
bool right = false;
bool enter = false;
bool second = false;

void main() {
    ti_CloseAll();
    gfx_Begin();
    while(running) {
        kb_Scan();

        update();
        render();

        gfx_SwapDraw();
    }
    destroy_level();
    gfx_End();
}

/* Put other functions here */
void update() {
    if(kb_Data[6] == kb_Clear) {
        running = false;
    }
    update_scene();

    if(kb_Data[7] == kb_Down) {
        down = true;
    } else {
        down = false;
    }
    if(kb_Data[7] == kb_Left) {
        left = true;
    } else {
        left = false;
    }
    if(kb_Data[7] == kb_Right) {
        right = true;
    } else {
        right = false;
    }
    if(kb_Data[7] == kb_Up) {
        up = true;
    } else {
        up = false;
    }
    if(kb_Data[6] == kb_Enter) {
        enter = true;
    } else {
        enter = false;
    }
    if(kb_Data[1] == kb_2nd) {
        second = true;
    } else {
        second = false;
    }
}
void update_scene() {
    if(scene == 0) {
        //Main menu options are "Play" "Credits" "Exit"
        if(key_down_pressed()) {
            cursor_pos = min(2,cursor_pos+1);
        }
        if(key_up_pressed()) {
            cursor_pos = max(0,cursor_pos-1);
        }
        if(key_enter_pressed()) {
            if(cursor_pos == 0) {
                scene = 1;
            }
            if(cursor_pos == 2) {
                running = false;
            }
        }
    } else if(scene == 1) {
        if(key_enter_pressed()) {
            gfx_SetTextScale(2,2);
            render_centered_string("Generating Level",LCD_WIDTH/2,LCD_HEIGHT/2-FONT_HEIGHT);
            render_centered_string("Please wait...",LCD_WIDTH/2,LCD_HEIGHT/2+FONT_HEIGHT);
            gfx_SetTextScale(1,1);
            gfx_SwapDraw();
            next_level();
            scene = 2;
            //todo: start timer here
            cursor_pos = 0;
        }
    } else if(scene == 2) {
        if(key_up_pressed()) {
            cursor_pos = max(0,cursor_pos-1);
        }
        if(key_left_pressed()) {
            cursor_pos2 = max(0,cursor_pos2-1);
        }
        if(key_right_pressed()) {
            cursor_pos2 = min(playing_level->width,cursor_pos2+1);
        }
        if(key_down_pressed()) {
            cursor_pos = min(playing_level->height,cursor_pos+1);
        }
        if(key_second_pressed()) {
            playing_level->selected_cells[cursor_pos*playing_level->width+cursor_pos2] = !playing_level->selected_cells[cursor_pos*playing_level->width+cursor_pos2];
        }
        if(key_enter_pressed()) {
            //todo: stop timer here
        }
    }
}

void render() {
    render_background(false);
    render_scene();
}
void render_scene () {
    if(scene == 0) {
        uint8_t i;
        gfx_SetTextFGColor(0);
        gfx_SetTextScale(2,2);
        render_centered_string(GAME_NAME,LCD_WIDTH/2,LCD_HEIGHT/4);
        for(i = 0; i < 3; i++) {
            if(i == cursor_pos) {
                gfx_SetTextFGColor(0xC0);
            } else {
                gfx_SetTextFGColor(0);
            }
            if(i == 0) {
                render_centered_string("Play",LCD_WIDTH/2,LCD_HEIGHT/4+FONT_HEIGHT*4);
            }
            if(i == 1) {
                render_centered_string("Credits",LCD_WIDTH/2,LCD_HEIGHT/4+FONT_HEIGHT*6);
            }
            if(i == 2) {
                render_centered_string("Exit",LCD_WIDTH/2,LCD_HEIGHT/4+FONT_HEIGHT*8);
            }
        }
    } else if(scene == 1) {
        char* fsa = malloc(sizeof("Level 255"));
        gfx_SetTextScale(1,1);
        sprintf(fsa,"Level %d",level+1);
        render_centered_string(fsa,LCD_WIDTH/2,LCD_HEIGHT/4);
        free(fsa);
        render_centered_string("Press Enter To Start",LCD_WIDTH/2,LCD_HEIGHT-LCD_HEIGHT/4);
    } else if(scene == 2) {
        uint16_t w,h;
        w = LCD_WIDTH/playing_level->width;
        h = LCD_HEIGHT/playing_level->height;
        render_level();
        gfx_SetColor(0xC1);
        gfx_Rectangle(cursor_pos2*w,cursor_pos*h,w,h);
    }
}
//If fancy is true, a fancy and distracting background will be made. TODO: Make the fancy background. My idea would be a grid with tiles fading in and out.
void render_background(bool fancy) {
    gfx_FillScreen(0xFF);
}
void render_centered_string(char* str,uint16_t x, uint8_t y) {
    gfx_PrintStringXY(str,x - (gfx_GetStringWidth(str)/2),y - (FONT_HEIGHT/2));
}

//Returns a new level.
level_t* generate_level(uint8_t width, uint8_t height, uint16_t cell_count) {
    uint16_t i;
    level_t* out = malloc(sizeof(uint8_t)*2 + (sizeof(width*height*sizeof(bool))*2));
    out->width = width;
    out->height = height;
    out->cells = malloc(width*height*sizeof(bool));
    out->selected_cells = malloc(width*height*sizeof(bool));
    for(i = 0; i < width*height; i++) {
        out->cells[i] = false;
    }

    for(i = 0; i < cell_count; i++) {
        //Whether or not the cell has generated yet. The program will try to generate cells at random locations but won't generate if there's already a cell there. If done is false then there is already a cell there.
        bool done = false;

        while(!done) {
            uint8_t x,y;
            x = random_number(0,width);
            y = random_number(0,height);
            if(!out->cells[y*width+x]) {
                out->cells[y*width+x] = true;
                done = true;
            }
        }
    }
    return out;
}
//Frees the memory of the current playing level.
void destroy_level() {
    if(playing_level != NULL) {
        free(playing_level->cells);
        free(playing_level->selected_cells);
        free(playing_level);
    }
}
//Frees the memory of the current level, then generates a new level and sets "playing_level" to that new level.
void create_new_level(uint8_t width, uint8_t height, uint16_t cell_count) {
    destroy_level();
    playing_level = generate_level(width,height,cell_count);
}
void render_level() {
    uint16_t tile_width, j;
    uint8_t tile_height, i;
    tile_width = LCD_WIDTH/playing_level->width;
    tile_height = LCD_HEIGHT/playing_level->height;
    for(i = 0; i < playing_level->height; i++) {
        for(j = 0; j < playing_level->width; j++) {
            bool state = playing_level->cells[i*playing_level->width+j];
            bool state2 = playing_level->selected_cells[i*playing_level->width+j];
            //If state is true, then the cell is alive.
            if(state) {
                gfx_SetColor(0x1C);
                if(state2) {
                    gfx_SetColor(0x80);
                }
                gfx_FillRectangle(j*tile_width,i*tile_height,tile_width,tile_height);
            } else if(state2) {
                gfx_SetColor(0xC0);
                gfx_FillRectangle(j*tile_width,i*tile_height,tile_width,tile_height);
            }
            gfx_SetColor(0x00);
            gfx_Rectangle(j*tile_width,i*tile_height,tile_width,tile_height);
        }
    }
}

int16_t min(int16_t a, int16_t b) {
    if(a < b) {
        return a;
    }
    return b;
}
int16_t max(int16_t a, int16_t b) {
    if(a > b) {
        return a;
    }
    return b;
}
bool key_up_pressed() {
    if((kb_Data[7] == kb_Up) && !up) {
        up = true;
        return true;
    }
    return false;
}
bool key_down_pressed() {
    if((kb_Data[7] == kb_Down) && !down) {
        down = true;
        return true;
    }
    return false;
}
bool key_left_pressed() {
    if((kb_Data[7] == kb_Left) && !left) {
        left = true;
        return true;
    }
    return false;
}
bool key_right_pressed() {
    if((kb_Data[7] == kb_Right) && !right) {
        right = true;
        return true;
    }
    return false;
}
bool key_enter_pressed() {
    if((kb_Data[6] == kb_Enter) && !enter) {
        enter = true;
        return true;
    }
    return false;
}
bool key_second_pressed() {
    if((kb_Data[1] == kb_2nd) && !second) {
        second = true;
        return true;
    }
    return false;
}
void next_level() {
    level++;
    create_new_level(16,12,level*8);
}
uint8_t random_number(uint8_t minimum,uint8_t maximum) {
    srandom(rtc_Time());
    return (random() % (maximum-minimum)) + minimum;
}