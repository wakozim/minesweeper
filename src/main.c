#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include "raylib.h"

#include "themes/frappe.h"

#define FACTOR                 100
#define DEFAULT_SCREEN_HEIGHT (FACTOR * 9)
#define DEFAULT_SCREEN_WIDTH  (FACTOR * 16)
#define CELL_SIZE             50
#define CELL_SIZE_PRESSED     (CELL_SIZE - 5)
#define CELL_GAP              5

#define LOGO_FONT_SIZE            80
#define MENU_BUTTON_FONT_SIZE     60
#define FIELD_FONT_SIZE           40
#define INFO_BAR_BUTTON_FONT_SIZE 40
#define END_GAME_BUTTON_FONT_SIZE 40

#define INFO_BAR_WIDTH            200
#define INFO_BAR_GAP              20

#define MAX_FIELD_COLUMNS    100
#define MAX_FIELD_ROWS       100
#define DEFAULT_BOMB_PERCENT 15.625

#define FONT_FILEPATH            "assets/fonts/OpenSans-Regular.ttf"
#define OPEN_CELL_SOUND_FILEPATH "assets/sounds/open_cell.wav"
#define FLAG_ICON_FILEPATH       "assets/images/flag.png"
#define CLOCK_ICON_FILEPATH      "assets/images/clock.png"
#define BOMB_ICON_FILEPATH       "assets/images/bomb.png"


typedef enum {
    MENU = 0,
    CHOOSE_DIFFICULTY,
    GAME,
    PAUSE,
    WIN,
    LOSE,
    QUIT,
} game_state;

game_state current_state = MENU;

int cell_right_pressed_index = -1;
int cell_left_pressed_index = -1;

/* Fonts */
Font logo_font;
Font menu_font;
Font field_font;
Font end_game_button_font;

/* Images and textures */
Image flag_icon_image;
Texture2D flag_icon_texture;

Image clock_icon_image;
Texture2D clock_icon_texture;

Image bomb_icon_image;
Texture2D bomb_icon_texture;

/* Sounds */
Sound open_cell_sound;


float seconds_played = 0;
int score = 0;

bool is_field_generated = false;

typedef enum {
    OPEN = 0,
    CLOSE,
    FLAG,
    STATE_COUNT
} cell_state;

int field_columns = MAX_FIELD_COLUMNS;
int field_rows    = MAX_FIELD_ROWS;
static int field[MAX_FIELD_COLUMNS*MAX_FIELD_ROWS] = {0};
static cell_state state_field[MAX_FIELD_COLUMNS*MAX_FIELD_ROWS] = {0};
float bomb_percent = DEFAULT_BOMB_PERCENT;
int bombs;


int random_number(int minimum_number, int max_number)
{
    return rand() % (max_number + 1 - minimum_number) + minimum_number;
}

bool is_mouse_or_key_released(int mouse_button, int key)
{
    return IsMouseButtonReleased(mouse_button) || IsKeyReleased(key);
}

bool is_mouse_or_key_pressed(int mouse_button, int key)
{
    return IsMouseButtonPressed(mouse_button) || IsKeyPressed(key);
}

bool is_mouse_or_key_down(int mouse_button, int key)
{
    return IsMouseButtonDown(mouse_button) || IsKeyDown(key);
}


Rectangle draw_text_centered(const char *text, Font font, int font_size, Color text_color, Vector2 center)
{
    Vector2 text_size = MeasureTextEx(font, text, font_size, 1);
    Vector2 text_position = {
        center.x - text_size.x/2,
        center.y -  text_size.y/2
    };
    DrawTextEx(font, text, text_position, font_size, 1, text_color);

    Rectangle text_rect = {
        .x = text_position.x,
        .y = text_position.y,
        .width = text_size.x,
        .height = text_size.y
    };
    return text_rect;
}

Rectangle draw_text(const char *text, Font font, int font_size, Color text_color, Vector2 text_position)
{
    Vector2 text_size = MeasureTextEx(font, text, font_size, 1);
    DrawTextEx(font, text, text_position, font_size, 1, text_color);

    Rectangle text_rect = {
        .x = text_position.x,
        .y = text_position.y,
        .width = text_size.x,
        .height = text_size.y
    };
    return text_rect;
}


void render_menu(int screen_width, int screen_height)
{
    /* Draw logo */
    draw_text_centered(
        "Minesweeper",
        logo_font,
        LOGO_FONT_SIZE,
        TEXT_COLOR,
        CLITERAL(Vector2){screen_width/2, screen_height/2 - 100}
    );
    /* Draw buttons */

    /* Draw play button */
    Rectangle play_rect = draw_text_centered(
        "Play",
        menu_font,
        MENU_BUTTON_FONT_SIZE,
        TEXT_COLOR,
        CLITERAL(Vector2){screen_width/2, screen_height/2}
    );

    /* Draw exit button */
    Rectangle exit_rect = draw_text_centered(
        "Exit",
        menu_font,
        MENU_BUTTON_FONT_SIZE,
        TEXT_COLOR,
        CLITERAL(Vector2){screen_width/2, screen_height/2 + 100}
    );

    /* Check mouse click */
    if (is_mouse_or_key_released(MOUSE_BUTTON_LEFT, KEY_Z)) {
        Vector2 mouse_position = GetMousePosition();
        if (CheckCollisionPointRec(mouse_position, play_rect)) {
            current_state = CHOOSE_DIFFICULTY;
        } else if (CheckCollisionPointRec(mouse_position, exit_rect)) {
            current_state = QUIT;
        }
    }
}


void init_field(void)
{
    /* Reset field */
    for (int i = 0; i < MAX_FIELD_ROWS*MAX_FIELD_COLUMNS; i++) {
        field[i] = 0;
        state_field[i] = CLOSE;
    }

    /* Set bombs at field */
    int cells = field_columns*field_rows;;
    bombs = cells*bomb_percent / 100;
    for (int i = 0; i < bombs; i++) {
        int cell_index = random_number(0, cells - 1);
        while (field[cell_index] == -1) cell_index = random_number(0, cells - 1);
        field[cell_index] = -1;
    }

    /* Calc bombs around cells */
    for (int y = 0; y < field_rows; y++) {
        for (int x = 0; x < field_columns; x++) {
            if (field[y*field_columns + x] == -1) continue;

            int bombs_around = 0;
            for (int sy = -1; sy <= 1; sy++) {
                for (int sx = -1; sx <= 1; sx++) {
                     if (sx == 0 && sy == 0) continue;
                     if (x + sx < 0 || x + sx >= field_columns) continue;
                     if (y + sy < 0 || y + sy >= field_rows) continue;

                     if (field[(sy+y)*field_columns + (sx+x)] == -1) bombs_around += 1;
                }
            }
            field[y*field_columns + x] = bombs_around;
        }
    }

#ifdef DEBUG
    for (int y = 0; y < field_rows; y++) {
        for (int x = 0; x < field_columns; x++) {
            printf("%3d ", field[y*field_columns + x]);
        }
        printf("\n");
    }
#endif

}


int flags(void)
{
    int flags = 0;
    for (int i = 0; i < field_rows*field_columns; i++) {
        if (state_field[i] == FLAG) flags++;
    }
    return flags;
}


void open_empty_cells(void)
{
    bool run = true;
    while (run) {
        run = false;
        for (int y = 0; y < field_rows; y++) {
            for (int x = 0; x < field_columns; x++) {
                int cell_index = y * field_columns + x;
                if (state_field[cell_index] != OPEN) continue;
                if (field[cell_index] != 0) continue;

                /* Iterate through the neighboring cells and open it */
                for (int sy = -1; sy <= 1; sy++) {
                    for (int sx = -1; sx <= 1; sx++) {
                        if (sx == 0 && sy == 0) continue;
                        if (x + sx < 0 || x + sx >= field_columns) continue;
                        if (y + sy < 0 || y + sy >= field_rows) continue;
                        int neighbour_cell_index = (y + sy) * field_columns + (x + sx);
                        if (state_field[neighbour_cell_index] != OPEN) {
                            state_field[neighbour_cell_index] = OPEN;
                            run = true;
                            score++;
                        }
                    }
                }
            }
        }
    }
}


bool check_win(void)
{
    int cells = field_columns*field_rows;;
    for (int i = 0; i < cells; i++) {
        if (field[i] == -1) continue;
        else if (state_field[i] != OPEN) return false;
    }
    return true;
}


void render_difficulty_menu(int screen_width, int screen_height)
{
    /* Draw 8 x 8 button */
    Rectangle easy_rect = draw_text_centered(
        "8x8, 10 bombs",
        menu_font,
        MENU_BUTTON_FONT_SIZE,
        TEXT_COLOR,
        CLITERAL(Vector2){screen_width/2, screen_height/2 - 100}
    );

    /* Draw 16 x 16 button */
    Rectangle medium_rect = draw_text_centered(
        "16x16, 40 bombs",
        menu_font,
        MENU_BUTTON_FONT_SIZE,
        TEXT_COLOR,
        CLITERAL(Vector2){screen_width/2, screen_height/2}
    );

    /* Draw 25 x 16 button */
    Rectangle hard_rect = draw_text_centered(
        "25x16, 63 bombs",
        menu_font,
        MENU_BUTTON_FONT_SIZE,
        TEXT_COLOR,
        CLITERAL(Vector2){screen_width/2, screen_height/2 + 100}
    );

    /* Check mouse click */
    if (is_mouse_or_key_released(MOUSE_BUTTON_LEFT, KEY_Z)) {
        Vector2 mouse_position = GetMousePosition();
        if (CheckCollisionPointRec(mouse_position, easy_rect)) {
            field_columns = 8;
            field_rows = 8;
            current_state = GAME;
        } else if (CheckCollisionPointRec(mouse_position, medium_rect)) {
            field_columns = 16;
            field_rows = 16;
            current_state = GAME;
        } else if (CheckCollisionPointRec(mouse_position, hard_rect)) {
            field_columns = 25;
            field_rows = 16;
            current_state = GAME;
        }

        if (current_state == GAME) {
            score = 0;
            seconds_played = 0;
            init_field();
            is_field_generated = true;
        }
    }
}

void process_lose(void)
{
    for (int i = 0; i < field_columns*field_rows; i++) {
        if (field[i] == -1) state_field[i] = OPEN;
    }
    current_state = LOSE;
}

// TODO: Simplify render_field()
void render_field(Vector2 field_position, bool interactive)
{
    for (int y = 0; y < field_rows; y++) {
        for (int x = 0; x < field_columns; x++) {
            int cell_index = y * field_columns + x;
            int cell_size = CELL_SIZE;
            int cell_x = field_position.x + x*CELL_SIZE + x*CELL_GAP;
            int cell_y = field_position.y + y*CELL_SIZE + y*CELL_GAP;

            Rectangle cell_rect = {
                .x = cell_x,
                .y = cell_y,
                .height = CELL_SIZE,
                .width = CELL_SIZE
            };
            bool is_cell_hovered = CheckCollisionPointRec(GetMousePosition(), cell_rect);

            /* Set cell color */
            Color cell_color = CELL_COLOR;
            if (state_field[cell_index] == OPEN && field[cell_index] == 0) cell_color = EMPTY_CELL_COLOR;
            else if (state_field[cell_index] == OPEN && field[cell_index] == -1) cell_color = BOMB_CELL_COLOR;
            else if (state_field[cell_index] == OPEN) cell_color = OPEN_CELL_COLOR;
            else if (is_cell_hovered && interactive) cell_color = CELL_COLOR_HOVER;
            /* Set cell size */
            if (interactive &&
                state_field[cell_index] != OPEN &&
                is_cell_hovered &&
                (is_mouse_or_key_down(MOUSE_BUTTON_LEFT, KEY_Z) &&
                 cell_left_pressed_index == cell_index))
            {
                cell_size = CELL_SIZE_PRESSED;
            }
            /* Draw cell */
            int visible_cell_x = cell_x + (CELL_SIZE - cell_size) / 2;
            int visible_cell_y = cell_y + (CELL_SIZE - cell_size) / 2;
            DrawRectangle(visible_cell_x, visible_cell_y, cell_size, cell_size, cell_color);

            /* If cell is open and its not flaged, draw the count of bombs around it */
            if (state_field[cell_index] == OPEN && field[cell_index] > 0) {
                const char *cell_text = TextFormat("%i", field[y*field_columns + x]);
                Vector2 cell_text_size = MeasureTextEx(field_font, cell_text, FIELD_FONT_SIZE, 1);
                Vector2 cell_text_position = {
                    (cell_x + CELL_SIZE/2) - cell_text_size.x/2,
                    (cell_y + CELL_SIZE/2) - cell_text_size.y/2
                };
                DrawTextEx(field_font, cell_text, cell_text_position, FIELD_FONT_SIZE, 1, CELL_TEXT_COLOR);
            } else if (state_field[cell_index] == OPEN && field[cell_index] == -1) {
                /* Draw bomb icon */
                float scale = ((float)cell_size - 10) / (float)bomb_icon_image.width;
                DrawTextureEx(
                    bomb_icon_texture,
                    CLITERAL(Vector2){visible_cell_x + 5, visible_cell_y + 5},
                    0,
                    scale,
                    TEXT_COLOR
                );
            } else if (state_field[cell_index] == FLAG) {
                /* Draw flag icon */
                float scale = ((float)cell_size - 10) / (float)flag_icon_image.width;
                DrawTextureEx(
                    flag_icon_texture,
                    CLITERAL(Vector2){visible_cell_x + 5, visible_cell_y + 5},
                    0,
                    scale,
                    TEXT_COLOR
                );
            }

            /* Process events */
            if (!interactive) continue;

            /* Check if mouse was pressed on cell */
            if (is_mouse_or_key_pressed(MOUSE_BUTTON_LEFT, KEY_Z) && is_cell_hovered) {
                cell_left_pressed_index = cell_index;
            } else if (is_mouse_or_key_pressed(MOUSE_BUTTON_RIGHT, KEY_X) && is_cell_hovered) {
                cell_right_pressed_index = cell_index;
            }

            /* Check if mouse was unpressed and pressed on cell */
            if (is_cell_hovered) {
                if (state_field[cell_index] == CLOSE &&
                    is_mouse_or_key_released(MOUSE_BUTTON_LEFT, KEY_Z) &&
                    cell_left_pressed_index == cell_index)
                {
                    state_field[cell_index] = OPEN;
                    PlaySound(open_cell_sound);
                    if (field[cell_index] == 0) open_empty_cells();
                    else if (field[cell_index] != -1) score++;

                    if (field[cell_index] == -1) process_lose();
                    else if (check_win()) current_state = WIN;
                } else if (is_mouse_or_key_released(MOUSE_BUTTON_RIGHT, KEY_X) &&
                           cell_right_pressed_index == cell_index &&
                           state_field[cell_index] != OPEN)
                {
                    if (state_field[cell_index] == CLOSE &&
                        flags() < bombs) {
                      state_field[cell_index] = FLAG;
                    }
                    else {
                      state_field[cell_index] = CLOSE;
                    }
                }
            }
        }
    }
}


Vector2 render_flags(Vector2 position)
{
    /* Calculate flags text size */
    const char *flags_text = TextFormat("%d/%d", flags(), bombs);
    Vector2 flags_text_size = MeasureTextEx(field_font, flags_text, 60, 1);

    /* Draw flag texture */
    float scale = ((float)flags_text_size.y - 10) / (float)flag_icon_image.height;
    DrawTextureEx(
        flag_icon_texture,
        CLITERAL(Vector2) {position.x, position.y + 5},
        0,
        scale,
        TEXT_COLOR
    );

    /* Draw flags count */
    Vector2 flags_text_position = {
        position.x + 10 + flag_icon_image.width * scale,
        position.y
    };
    DrawTextEx(field_font, flags_text, flags_text_position, 60, 1, CELL_TEXT_COLOR);
    return flags_text_size;
}


Vector2 render_clock(int seconds, Vector2 position)
{
    const char *time_text = TextFormat("%02d:%02d", (int)seconds / 60, (int)seconds % 60);
    Vector2 time_text_size = MeasureTextEx(field_font, time_text, 60, 1);

    /* Draw flag texture */
    float scale = ((float)time_text_size.y - 10) / (float)clock_icon_image.height;
    DrawTextureEx(
        clock_icon_texture,
        CLITERAL(Vector2) {position.x, position.y + 5},
        0,
        scale,
        TEXT_COLOR
    );

    /* Draw flags count */
    Vector2 time_text_position = {
        position.x + 10 + clock_icon_image.width * scale,
        position.y
    };

    DrawTextEx(field_font, time_text, time_text_position, 60, 1, CELL_TEXT_COLOR);
    return time_text_size;
}


void render_game(int screen_width, int screen_height)
{
    int field_width = field_columns*CELL_SIZE + ((field_columns - 1) * CELL_GAP);
    int field_height = field_rows*CELL_SIZE + ((field_rows - 1) * CELL_GAP);

    int field_start_x = screen_width/2 - field_width/2 - 200;
    int field_start_y = screen_height/2 - field_height/2;

    /* Render score */
    Vector2 flags_size = render_flags(
        CLITERAL(Vector2){field_start_x + field_width + INFO_BAR_GAP, field_start_y}
    );

    /* Render time */
    seconds_played += GetFrameTime();
    render_clock(
        seconds_played,
        CLITERAL(Vector2){field_start_x + field_width + INFO_BAR_GAP, field_start_y + 10 + flags_size.y}
    );

    /* Render field */
    render_field(
        CLITERAL(Vector2){field_start_x, field_start_y},
        true
    );
}


void render_end_game_screen(int screen_width, int screen_height)
{
    int field_width = field_columns*CELL_SIZE + ((field_columns - 1) * CELL_GAP);
    int field_height = field_rows*CELL_SIZE + ((field_rows - 1) * CELL_GAP);

    int field_start_x = screen_width/2 - field_width/2 - 200;
    int field_start_y = screen_height/2 - field_height/2;

    /* Render field */
    render_field(
        CLITERAL(Vector2){field_start_x, field_start_y},
        false
    );

    /* Render score */
    Vector2 flags_size = render_flags(
        CLITERAL(Vector2){field_start_x + field_width + INFO_BAR_GAP, field_start_y}
    );

    /* Render time */
    render_clock(
        seconds_played,
        CLITERAL(Vector2){field_start_x + field_width + INFO_BAR_GAP, field_start_y + 10 + flags_size.y}
    );

    /* Render buttons */
    Rectangle play_again_rect = draw_text(
        "Play again",
        end_game_button_font,
        END_GAME_BUTTON_FONT_SIZE,
        TEXT_COLOR,
        CLITERAL(Vector2){field_start_x + field_width + 25, field_start_y + field_height - 150}
    );

    Rectangle difficulty_rect = draw_text(
        "Change difficulty",
        end_game_button_font,
        END_GAME_BUTTON_FONT_SIZE,
        TEXT_COLOR,
        CLITERAL(Vector2){field_start_x + field_width + 25, play_again_rect.y + play_again_rect.height + 10}
    );

    Rectangle exit_rect = draw_text(
        "Exit",
        end_game_button_font,
        END_GAME_BUTTON_FONT_SIZE,
        TEXT_COLOR,
        CLITERAL(Vector2){field_start_x + field_width + 25, difficulty_rect.y + difficulty_rect.height + 10}
    );

    if (is_mouse_or_key_released(MOUSE_BUTTON_LEFT, KEY_Z)) {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, play_again_rect)) {
            score = 0;
            seconds_played = 0;
            init_field();
            is_field_generated = true;
            current_state = GAME;
        } else if (CheckCollisionPointRec(mouse, difficulty_rect)) {
            current_state = CHOOSE_DIFFICULTY;
        } else if (CheckCollisionPointRec(mouse, exit_rect)) {
            current_state = QUIT;
        }
    }
}


int main(void)
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT, "Minesweeper");
    InitAudioDevice();
    SetTargetFPS(30);
    srand(time(NULL));

    logo_font            = LoadFontEx(FONT_FILEPATH, LOGO_FONT_SIZE, NULL, 0);
    menu_font            = LoadFontEx(FONT_FILEPATH, MENU_BUTTON_FONT_SIZE, NULL, 0);
    field_font           = LoadFontEx(FONT_FILEPATH, FIELD_FONT_SIZE, NULL, 0);
    end_game_button_font = LoadFontEx(FONT_FILEPATH, END_GAME_BUTTON_FONT_SIZE, NULL, 0);

    open_cell_sound = LoadSound(OPEN_CELL_SOUND_FILEPATH);

    flag_icon_image   = LoadImage(FLAG_ICON_FILEPATH);
    flag_icon_texture = LoadTextureFromImage(flag_icon_image);

    clock_icon_image   = LoadImage(CLOCK_ICON_FILEPATH);
    clock_icon_texture = LoadTextureFromImage(clock_icon_image);

    bomb_icon_image   = LoadImage(BOMB_ICON_FILEPATH);
    bomb_icon_texture = LoadTextureFromImage(bomb_icon_image);

    bool exit_window = false;
    while (!exit_window) {
        if (WindowShouldClose()) exit_window = true;

        BeginDrawing();
            int screen_width  = GetScreenWidth();
            int screen_height = GetScreenHeight();
            ClearBackground(BACKGROUND_COLOR);

            switch (current_state) {
            case MENU:
                render_menu(screen_width, screen_height); break;
            case CHOOSE_DIFFICULTY:
                render_difficulty_menu(screen_width, screen_height); break;
            case GAME:
                render_game(screen_width, screen_height); break;
            case WIN:
            case LOSE:
                render_end_game_screen(screen_width, screen_height); break;
            case QUIT:
                exit_window = true; break;
            default:
                break;
            }
        EndDrawing();
        if (IsKeyReleased(KEY_R)) {
                TakeScreenshot("screen.png");
            }
    }

    UnloadSound(open_cell_sound);
    CloseAudioDevice();

    UnloadTexture(bomb_icon_texture);
    UnloadImage(bomb_icon_image);

    UnloadTexture(clock_icon_texture);
    UnloadImage(clock_icon_image);

    UnloadTexture(flag_icon_texture);
    UnloadImage(flag_icon_image);

    UnloadFont(end_game_button_font);
    UnloadFont(field_font);
    UnloadFont(menu_font);
    UnloadFont(logo_font);
    CloseWindow();

    return 0;
}


// TODO: Generate field after first click
// TODO: Change sound when open cell
// TODO: Change flag icon color
// TODO: Change clock icon color
