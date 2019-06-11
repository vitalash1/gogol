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
#include <debug.h>

#define GAME_NAME "Game of Game of Life"
#define FONT_HEIGHT 8
#define EMPTY_FUNC_MENU_OPTION "\0"
#define WIN_TEXT_LESS_FIVE "I believe this is below average, level %d!"
#define WIN_TEXT_LESS_TEN "Nice, you did well! You made it to level %d."
#define WIN_TEXT_MORE_TEN "Amazing job! Level %d, double factorial!!"
#define WIN_TEXT_HIGHSCORE "You beat your highscore with level %d!"
#define WIN_TEXT_NO_TIME "We all run out of time eventually. Level %d."

#define ALIVE_CELLS_PER_LEVEL 3
#define ALIVE_CELLS_BASE 6

//All of the timer code was stolen from the "second_counter2" example because I barely have an idea on how it works.
#define SECOND 32768/1
//The counter will count upwards until it reaches TIME_LIMIT. It is displayed as TIME_LIMIT-counter
#define TIME_LIMIT 40

typedef struct {
    uint8_t width;
    uint8_t height;
    bool* cells;
    bool* selected_cells;
} level_t;

typedef struct {
    bool running; //Whether or not the main game loop should loop
    bool timing; //Whether or not a timer should be counting the number of seconds that have gone by since it started.
    level_t* playing_level; //Current level structure being played.
    uint8_t scene; //Current game.scene. game.scenes: 0 Main Menu, 1 game.level Start Screen, 2 Play, 3 Win/Lose, 4 Credits, 5 How To
    uint8_t cursor_pos; //Y cursor position
    uint8_t cursor_pos2; //X cursor position
    uint8_t level; //The current level being played.
    uint24_t timer_seconds; //Amount of seconds counted by timer so far.
} game_t;

/* Put your function prototypes here */
void update();
void update_scene();

void render();
void render_scene();
void render_background();
void render_centered_string(char* str, uint24_t x, uint8_t y);
void render_level();

level_t* generate_level(uint8_t width, uint8_t height, uint24_t cell_count);
void create_new_level(uint8_t width, uint8_t height, uint24_t cell_count);
void destroy_level();
void next_level();
void iterate_level();
void clear_level(bool cells,bool selected);
bool compare_selection_level();

bool key_up_pressed();
bool key_down_pressed();
bool key_left_pressed();
bool key_right_pressed();
bool key_enter_pressed();
bool key_second_pressed();

void start_timer();
void update_timer();
void end_timer();

uint8_t random_number(uint8_t minimum,uint8_t maximum);

int24_t min(int24_t a, int24_t b);
int24_t max(int24_t a, int24_t b);
/* Put all your globals here */
/*bool game.running = true; //Whether or not the main loop should keep game.running.
level_t* game.playing_level = NULL; //The game.level that is currently being played.
uint8_t game.scene = 0; //game.scenes: 0 Main Menu, 1 game.level Start Screen, 2 Play, 3 Win, 4 Credits, 5 How To
uint8_t game.cursor_pos = 0;
uint8_t game.cursor_pos2 = 0;
uint8_t game.level = 0;*/

game_t game = {true,false,NULL,0,0,0,0,0};

void main() {
    timer_1_MatchValue_1 = SECOND;
    srandom(rtc_Time());
    ti_CloseAll();
    gfx_Begin();
    while(game.running) {
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
    if(game.timing) {
        update_timer();
    }
    if(kb_Data[6] == kb_Clear) {
        game.running = false;
    }
    update_scene();
}
void update_scene() {
    static bool timer_finished = false;
    if(game.scene == 0) {
        //Main menu options are "Play" "Credits" "Exit"
        if(key_down_pressed()) {
            game.cursor_pos = min(2,game.cursor_pos+1);
        }
        if(key_up_pressed()) {
            game.cursor_pos = max(0,game.cursor_pos-1);
        }
        if(key_enter_pressed()) {
            if(game.cursor_pos == 0) {
                game.scene = 1;
            }
            if(game.cursor_pos == 2) {
                game.running = false;
            }
        }
    } else if(game.scene == 1) {
        if(key_enter_pressed()) {
            gfx_SetTextScale(2,2);
            render_centered_string("Generating level",LCD_WIDTH/2,LCD_HEIGHT/2-FONT_HEIGHT);
            render_centered_string("Please wait...",LCD_WIDTH/2,LCD_HEIGHT/2+FONT_HEIGHT);
            gfx_SetTextScale(1,1);
            gfx_SwapDraw();
            next_level();
            game.scene = 2;
            game.cursor_pos = 0;
            game.cursor_pos2 = 0;
            start_timer();
            timer_finished = false;
        }
    } else if(game.scene == 2) {
        if(!timer_finished) {
            if(key_up_pressed()) {
                game.cursor_pos = max(0,game.cursor_pos-1);
            }
            if(key_left_pressed()) {
                game.cursor_pos2 = max(0,game.cursor_pos2-1);
            }
            if(key_right_pressed()) {
                game.cursor_pos2 = min(game.playing_level->width-1,game.cursor_pos2+1);
            }
            if(key_down_pressed()) {
                game.cursor_pos = min(game.playing_level->height-1,game.cursor_pos+1);
            }
            if(key_second_pressed()) {
                game.playing_level->selected_cells[game.cursor_pos*game.playing_level->width+game.cursor_pos2] = !game.playing_level->selected_cells[game.cursor_pos*game.playing_level->width+game.cursor_pos2];
            }
            if(game.timer_seconds > TIME_LIMIT) {
                game.scene = 3;
                //loser
            }
        }
        if(key_enter_pressed()) {
            if(!timer_finished) {
                end_timer();
                iterate_level();
                timer_finished = true;
            } else {
                if(compare_selection_level()) {
                    //Win. Your selection is all on alive cells, not on dead.
                    game.scene = 1;
                } else {
                    //Lose.
                    game.scene = 3;
                }
            }
        }
    } else if(game.scene == 3) {
        if(key_enter_pressed()) {
            //Set the scene to main menu
            game.scene = 0;
            //Set the cursor position to 0.
            game.cursor_pos = 0;
            game.level = 0;
        }
    }
}

void render() {
    render_background(false);
    render_scene();
}
void render_scene () {
    if(game.scene == 0) {
        uint8_t i;
        gfx_SetTextFGColor(0);
        gfx_SetTextScale(2,2);
        render_centered_string(GAME_NAME,LCD_WIDTH/2,LCD_HEIGHT/4);
        for(i = 0; i < 3; i++) {
            if(i == game.cursor_pos) {
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
        gfx_SetTextScale(1,1);
    } else if(game.scene == 1) {
        char* fsa = malloc(strlen("Level 255"));
        gfx_SetTextScale(2,2);
        sprintf(fsa,"Level %d",game.level+1);
        render_centered_string(fsa,LCD_WIDTH/2,LCD_HEIGHT/4);
        free(fsa);
        render_centered_string("Press Enter To Start",LCD_WIDTH/2,LCD_HEIGHT-LCD_HEIGHT/4);
        gfx_SetTextScale(1,1);
    } else if(game.scene == 2) {
        uint24_t w,h;
        char* seconds;
        if(game.timing)  {
            //If the timer ever gets past 9999s then something must have gone severely wrong in this game's development.
            seconds = malloc(strlen("1337s"));
            sprintf(seconds,"%d",TIME_LIMIT-game.timer_seconds);
        } else {
            seconds = "ENTER";
        }
        w = LCD_WIDTH/game.playing_level->width;
        h = LCD_HEIGHT/game.playing_level->height;
        render_level();
        gfx_SetColor(0xC1);
        gfx_Rectangle(game.cursor_pos2*w,game.cursor_pos*h,w,h);

        gfx_SetColor(0xC1);
        
        gfx_PrintStringXY(seconds,3,3);
        if(game.timing) {
            free(seconds);
        }
    } else if(game.scene == 3) {
        char* fsa;

        if(game.timer_seconds > TIME_LIMIT) {
            fsa = malloc(strlen(WIN_TEXT_NO_TIME)+1);
            sprintf(fsa,WIN_TEXT_NO_TIME,game.level);
            if(game.timing) {
                end_timer();
            }
        } else {
            //todo: add checking for highscores that will take the place of the normal texts.
            if(game.level <= 5) {
                //Plus one because the level could be more than two digits, it will be at most three.
                fsa = malloc(strlen(WIN_TEXT_LESS_FIVE)+1);
                sprintf(fsa,WIN_TEXT_LESS_FIVE,game.level);
            } else if(game.level <= 10) {
                fsa = malloc(strlen(WIN_TEXT_LESS_TEN)+1);
                sprintf(fsa,WIN_TEXT_LESS_TEN,game.level);
            } else { // if(game.level > 10). Just did an else rather than an else if to avoid compiler warnings
                fsa = malloc(strlen(WIN_TEXT_MORE_TEN)+1);
                sprintf(fsa,WIN_TEXT_MORE_TEN,game.level);
            }
        }
        render_centered_string(fsa,LCD_WIDTH/2,LCD_HEIGHT/4);
        render_centered_string("Press Enter",LCD_WIDTH/2,LCD_HEIGHT*3/4);
        free(fsa);
    }
}
//If fancy is true, a fancy and distracting background will be made. TODO: Make the fancy background. My idea would be a grid with tiles fading in and out.
void render_background(bool fancy) {
    gfx_FillScreen(0xFF);
}
void render_centered_string(char* str,uint24_t x, uint8_t y) {
    gfx_PrintStringXY(str,x - (gfx_GetStringWidth(str)/2),y - (FONT_HEIGHT/2));
}
//This does one generation on the game.level currently being played. <!> It may not work <!>
void iterate_level() {
    //A bunch of iterators
    int24_t i,j,xp,yp;
    //A buffer to be read for counting neighbors
    bool* cells_buffer = malloc(game.playing_level->width * game.playing_level->height * sizeof(bool));

    //Copying cells to buffer
    //Hopefully this method works, I'm paranoid of memory overflows now.
    for(i = 0; i < game.playing_level->width*game.playing_level->height; i++) {
        cells_buffer[i] = game.playing_level->cells[i];
    }
    //Looping through cells
    for(i = 0; i < game.playing_level->height; i++) {
        for(j = 0; j < game.playing_level->width; j++) {
            uint8_t neighbors = 0;
            //Counting neighbors
            for(yp = max(0,i-1); yp < min(game.playing_level->height,i+2); yp++) {
                for(xp = max(0,j-1); xp < min(game.playing_level->width,j+2); xp++) {
                    if(!(xp == j && yp == i)) {
                        if(cells_buffer[yp*game.playing_level->width+xp] == true) {
                            neighbors++;
                        }
                    }
                }
            }
            //Killing underpopulated/overpopulated areas
            if(neighbors < 2 || neighbors > 3) {
                game.playing_level->cells[i*game.playing_level->width+j] = false;
            }
            //New births, thanks to three parents!
            if(neighbors == 3) {
                game.playing_level->cells[i*game.playing_level->width+j] = true;
            }
        }
    }
    //Free the buffer that was used.
    free(cells_buffer);
}
void clear_level(bool cells, bool selected) {
    uint24_t i;
    for(i = 0; i < game.playing_level->height*game.playing_level->width; i++) {
        if(cells) {
            game.playing_level->cells[i] = false;
        }
        if(selected) {
            game.playing_level->selected_cells[i] = false;
        }
    }
}
//Checks if all of the selected cells are alive, and that no dead cells are selected. 
bool compare_selection_level() {
    uint24_t i;
    for(i = 0; i < game.playing_level->height*game.playing_level->width; i++) {
        if(game.playing_level->cells[i] != game.playing_level->selected_cells[i]) {
            return false;
        }
    }
    return true;
}
//Returns a new game.level.
level_t* generate_level(uint8_t width, uint8_t height, uint24_t cell_count) {
    uint24_t i;
    level_t* out = malloc(sizeof(level_t));
    out->width = width;
    out->height = height;
    out->cells = malloc(width*height*sizeof(bool));
    out->selected_cells = malloc(width*height*sizeof(bool));

    for(i = 0; i < width*height; i++) {
        out->cells[i] = false;
        out->selected_cells[i] = false;
    }
    for(i = 0; i < cell_count; i++) {
        //Whether or not the cell has generated yet. The program will try to generate cells at random locations but won't generate if there's already a cell there. If done is false then there is already a cell there.
        bool done = false;

        while(!done) {
            uint8_t x,y;
            x = randInt(0,width);
            y = randInt(0,height);
            if(!out->cells[y*(width-1)+x]) {
                out->cells[y*(width-1)+x] = true;
                done = true;
            }
        }
    }
    return out;
}
//Frees the memory of the current playing level.
void destroy_level() {
    if(game.playing_level != NULL) {
        free(game.playing_level->selected_cells);
        free(game.playing_level->cells);
        free(game.playing_level);
    }
}
//Frees the memory of the current game.level, then generates a new game.level and sets "game.playing_level" to that new game.level.
void create_new_level(uint8_t width, uint8_t height, uint24_t cell_count) {
    destroy_level();
    game.playing_level = generate_level(width,height,cell_count);
}
void render_level() {
    uint24_t tile_width, j;
    uint8_t tile_height, i;
    tile_width = LCD_WIDTH/game.playing_level->width;
    tile_height = LCD_HEIGHT/game.playing_level->height;
    for(i = 0; i < game.playing_level->height; i++) {
        for(j = 0; j < game.playing_level->width; j++) {
            bool state = game.playing_level->cells[i*game.playing_level->width+j];
            bool state2 = game.playing_level->selected_cells[i*game.playing_level->width+j];
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

int24_t min(int24_t a, int24_t b) {
    if(a < b) {
        return a;
    }
    return b;
}
int24_t max(int24_t a, int24_t b) {
    if(a > b) {
        return a;
    }
    return b;
}
bool key_up_pressed() {
    static bool up = false;
    if(kb_Data[7] == kb_Up) {
        if(!up) {
            up = true;
            return true;
        }
        //The reason I'm returning false before it reaches the end, is that I don't want UP to be false while the key is pressed.
        return false;
    }
    up = false;
    return false;
}
bool key_down_pressed() {
    static bool down = false;
    if(kb_Data[7] == kb_Down) {
        if(!down) {
            down = true;
            return true;
        }
        return false;
    }
    down = false;
    return false;
}
bool key_left_pressed() {
    static bool left = false;
    if(kb_Data[7] == kb_Left) {
        if(!left) {
            left = true;
            return true;
        }
        return false;
    }
    left = false;
    return false;
}
bool key_right_pressed() {
    static bool right = false;
    if(kb_Data[7] == kb_Right) {
        if(!right) {
            right = true;
            return true;
        }
        return false;
    }
    right = false;
    return false;
}
bool key_enter_pressed() {
    static bool enter = false;
    if(kb_Data[6] == kb_Enter) {
        if(!enter) {
            enter = true;
            return true;
        }
        return false;
    }
    enter = false;
    return false;
}
bool key_second_pressed() {
    static bool second = false;
    if(kb_Data[1] == kb_2nd) {
        if(!second) {
            second = true;
            return true;
        }
        return false;
    }
    second = false;
    return false;
}
void next_level() {
    game.level++;
    create_new_level(16,12,game.level*ALIVE_CELLS_PER_LEVEL + ALIVE_CELLS_BASE);
}

void start_timer() {
    game.timing = true;
    game.timer_seconds = 0;
    timer_Control = TIMER1_DISABLE;
    timer_1_Counter = 0;
    timer_Control = TIMER1_ENABLE | TIMER1_32K | TIMER1_NOINT | TIMER1_UP;
}
void update_timer() {
    if(timer_IntStatus & TIMER1_MATCH1) {
        game.timer_seconds++;
        timer_IntStatus = TIMER1_MATCH1;
        timer_Control = TIMER1_DISABLE;
        timer_1_Counter = 0;
        timer_Control = TIMER1_ENABLE | TIMER1_32K | TIMER1_NOINT | TIMER1_UP;
    }
}
void end_timer() {
    game.timing = false;
    timer_Control = TIMER1_DISABLE;
    timer_1_Counter = 0;
}

/*
Obselete; Function "randInt" provided by C libs.
uint8_t random_number(uint8_t minimum,uint8_t maximum) {
    return (random() % (maximum-minimum)) + minimum;
}*/