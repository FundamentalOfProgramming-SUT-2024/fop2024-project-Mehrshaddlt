#include <ncursesw/ncurses.h>
#include <wchar.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h>  
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <string.h>
#include <locale.h>
#include <sqlite3.h>
#define MAXROOMS 9
#define MIN_ROOM_SIZE 6
#define MAX_ROOM_SIZE 10
#define MAX_MESSAGE_LENGTH 80
#define MESSAGE_DURATION 5
#define MAX_LEVELS 5  // 4 regular levels + 1 treasure room
#define TREASURE_LEVEL 5
// Make these variables global and modifiable
int NUMCOLS;
int NUMLINES;
char current_username[50] = "";
static bool fast_travel_mode = false;

typedef struct {
    int x, y;
} Coord;

typedef struct {
    Coord pos;      // position of room on screen
    Coord max;      // size of room
    Coord center;   // center of room
    bool gone;      // room is gone (not there)
    bool connected; // room is connected to another room
    int doors[4];   // doors to this room
} Room;
typedef enum {
    WEAPON_MACE,
    WEAPON_DAGGER,
    WEAPON_WAND,
    WEAPON_ARROW,
    WEAPON_SWORD,
    WEAPON_COUNT
} WeaponType;
typedef struct {
    WeaponType type;
    const char* name;
    const char* symbol;
    bool owned;
} Weapon;
typedef enum {
    TALISMAN_HEALTH,
    TALISMAN_DAMAGE,
    TALISMAN_SPEED,
    TALISMAN_COUNT
} TalismanType;
typedef struct {
    TalismanType type;
    const char* name;
    const char* symbol;
    int color;
    bool owned;
} Talisman;
typedef struct {
    Room rooms[MAXROOMS];
    Room secret_rooms[MAXROOMS];      // Array to store secret rooms
    int num_rooms;
    int num_secret_rooms;             // Number of secret rooms
    Room* stair_room;                 // Room containing the up stairs
    int stair_x, stair_y;             // Position of up stairs
    char **tiles;                     // Each level needs its own tiles
    char **visible_tiles;             // Each level needs its own visibility
    bool **explored;                  // Each level needs its own exploration state
    bool **traps;                     // Each level needs its own traps
    bool **discovered_traps;
    bool **secret_walls;              // Track which walls are secret doors
    Coord stairs_up;                  // Location of stairs going up
    Coord stairs_down;
    Coord secret_entrance;            // Remember where the player entered from
    Room *current_secret_room;        // Track which secret room the player is in
    bool stairs_placed;
    char **backup_tiles;
    char **backup_visible_tiles;  // Add this
    bool **backup_explored;
    bool **secret_stairs;    // Track secret stairs locations
    Room *secret_stair_room; // The room the secret stair leads to
    Coord secret_stair_entrance; 
    bool **coins;          // Make these dynamic arrays
    int **coin_values;
    int talisman_type;
} Level;

typedef struct {
    Level *levels;        // Array of all levels
    int current_level;    // Current level number (1-5)
    int player_x, player_y;
    int prev_stair_x;    // Position of stairs in previous level
    int prev_stair_y;
    bool debug_mode;
    char current_message[MAX_MESSAGE_LENGTH];
    int message_timer;
    int health;
    int strength;
    int gold;
    int armor;
    int exp;
    bool is_treasure_room;
    int character_color;
    int games_played;
    int food_count;        // Current number of food items (max 5)
    int hunger;           // Hunger level (let's say 100 is full, 0 is starving)
    int hunger_timer;     // Timer to track hunger depletion
    bool food_menu_open;
    Weapon weapons[5];
    WeaponType current_weapon;
    bool weapon_menu_open;
    Talisman talismans[3];
    bool talisman_menu_open;
} Map;

// Function declarations
void set_message(Map *map, const char *message);
void add_stairs(Level *level, Room *room, bool is_up);
void copy_room(Room *dest, Room *src);
void transition_to_next_level(Map *map);
void handle_input(Map *map, int input);
Map* create_map();
void free_map(Map* map);
bool rooms_overlap(Room *r1, Room *r2);
void draw_room(Level *level, Room *room);
void connect_rooms(Level *level, Room *r1, Room *r2);
void update_visibility(Map *map);
void generate_map(Map *map);
void generate_remaining_rooms(Map *map);
bool check_for_stairs(Level *level);
bool is_valid_secret_wall(Level *level, int x, int y);
void add_secret_walls_to_room(Level *level, Room *room);
void draw_secret_room(Level *level);
void load_game_settings(Map* map);
void load_username();
void save_user_data(Map* map);
void show_win_screen(Map* map);
void load_user_stats(Map* map);
void add_secret_stairs(Level *level, Room *room);
void eat_food(Map *map);
void display_food_menu(WINDOW *win, Map *map);
void display_weapon_menu(WINDOW *win, Map *map);
void display_talisman_menu(WINDOW *win, Map *map);
// Function delarations
void load_user_stats(Map* map) {
    if (strcmp(current_username, "") == 0 || strcmp(current_username, "Guest") == 0) {
        map->games_played = 0;
        map->exp = 0;  // Initialize exp to 0 for new/guest users
        return;
    }

    FILE *file = fopen("user_score.txt", "r");
    if (file == NULL) {
        map->games_played = 0;
        map->exp = 0;
        return;
    }

    char line[256];
    bool found = false;
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Username:", 9) == 0) {
            char username[50];
            sscanf(line, "Username: %s", username);
            if (strcmp(username, current_username) == 0) {
                found = true;
                // Skip to Experience line
                for (int i = 0; i < 6; i++) {
                    fgets(line, sizeof(line), file);
                    if (i == 5) { // Experience line
                        sscanf(line, "Exp: %d", &map->exp);
                    }
                }
                fgets(line, sizeof(line), file); // Games Played line
                sscanf(line, "Games Played: %d", &map->games_played);
                break;
            }
        }
    }
    
    if (!found) {
        map->games_played = 0;
        map->exp = 0;
    }

    fclose(file);
}
void show_win_screen(Map* map) {
    clear();
    int center_y = NUMLINES / 2;
    int center_x = NUMCOLS / 2;
    int box_width = 40;
    int box_height = 8;
    int start_x = center_x - (box_width / 2);
    int start_y = center_y - (box_height / 2);
    attron(COLOR_PAIR(1));
    mvprintw(start_y, start_x, "╔");
    for (int i = 1; i < box_width - 1; i++) {
        mvprintw(start_y, start_x + i, "═");
    }
    mvprintw(start_y, start_x + box_width - 1, "╗");
    for (int i = 1; i < box_height - 1; i++) {
        mvprintw(start_y + i, start_x, "║");
        mvprintw(start_y + i, start_x + box_width - 1, "║");
    }
    mvprintw(start_y + box_height - 1, start_x, "╚");
    for (int i = 1; i < box_width - 1; i++) {
        mvprintw(start_y + box_height - 1, start_x + i, "═");
    }
    mvprintw(start_y + box_height - 1, start_x + box_width - 1, "╝");
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(start_y + 1, center_x - 4, "VICTORY!");
    attroff(COLOR_PAIR(1) | A_BOLD);
    attron(COLOR_PAIR(4));
    mvprintw(start_y + 3, center_x - 11, "Total Gold Collected: %d", map->gold);
    mvprintw(start_y + 4, center_x - 11, "Final Level Reached: %d", map->current_level);
    mvprintw(start_y + 5, center_x - 11, "Experience so far: %d", map->exp);
    attroff(COLOR_PAIR(4));
    attron(COLOR_PAIR(2));
    mvprintw(start_y + 6, center_x - 10, "Press any key to exit");
    attroff(COLOR_PAIR(2));
    refresh();
    getch();
}
void load_username() {
    FILE *file = fopen("game_settings.txt", "r");
    if (file != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "Username:", 9) == 0) {
                sscanf(line, "Username: %s", current_username);
                break;
            }
        }
        fclose(file);
    }
}
void save_user_data(Map* map) {
    if (strcmp(current_username, "") == 0 || strcmp(current_username, "Guest") == 0) {
        return; // Don't save data for empty username or guest
    }

    FILE *file = fopen("user_score.txt", "r");
    FILE *temp = fopen("temp_score.txt", "w");
    if (file == NULL || temp == NULL) {
        if (temp) fclose(temp);
        if (file) fclose(file);
        return;
    }

    char line[256];
    bool found = false;
    char current_block[2048] = ""; // Buffer to store current user block
    char other_blocks[10240] = ""; // Buffer to store other users' blocks
    bool copying_current_user = false;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Username:", 9) == 0) {
            char username[50];
            sscanf(line, "Username: %s", username);
            
            if (strcmp(username, current_username) == 0) {
                found = true;
                copying_current_user = true;
                // Skip the current user's old block
                for (int i = 0; i < 7; i++) {
                    if (!fgets(line, sizeof(line), file)) break;
                }
            } else {
                // If we were copying the current user's block, stop
                if (copying_current_user) {
                    copying_current_user = false;
                }
                // Store other users' blocks
                strcat(other_blocks, line);
                continue;
            }
        } else if (!copying_current_user) {
            strcat(other_blocks, line);
        }
    }

    // Write the current user's updated data
    fprintf(temp, "Username: %s\n", current_username);
    fprintf(temp, "Level: %d\n", map->current_level);
    fprintf(temp, "Hit: %d\n", map->health);
    fprintf(temp, "Strength: %d\n", map->strength);
    fprintf(temp, "Gold: %d\n", map->gold);
    fprintf(temp, "Armor: %d\n", map->armor);
    fprintf(temp, "Exp: %d\n", map->exp + map->gold); // Add current gold to exp
    fprintf(temp, "Games Played: %d\n\n", map->games_played + 1);

    // Write all other users' data
    fprintf(temp, "%s", other_blocks);

    // If this is a new user, and we haven't written their data yet
    if (!found) {
        fprintf(temp, "Username: %s\n", current_username);
        fprintf(temp, "Level: %d\n", map->current_level);
        fprintf(temp, "Hit: %d\n", map->health);
        fprintf(temp, "Strength: %d\n", map->strength);
        fprintf(temp, "Gold: %d\n", map->gold);
        fprintf(temp, "Armor: %d\n", map->armor);
        fprintf(temp, "Exp: %d\n", map->gold); // Initial exp is their first gold
        fprintf(temp, "Games Played: 1\n\n");
    }

    fclose(file);
    fclose(temp);
    remove("user_score.txt");
    rename("temp_score.txt", "user_score.txt");
}

void load_game_settings(Map* map) {
    FILE* file = fopen("game_settings.txt", "r");
    if (file == NULL) {
        // If file can't be opened, default to color 1 (yellow)
        map->character_color = 1;
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "CharacterColor: ", 15) == 0) {
            map->character_color = atoi(line + 15);
            break;
        }
    }

    fclose(file);
}
void add_secret_stairs(Level *level, Room *room) {
    // 20% chance to add a secret stair in a room
    if (rand() % 10 == 0) {
        int attempts = 0;
        const int MAX_ATTEMPTS = 10;
        
        while (attempts < MAX_ATTEMPTS) {
            // Try to place stairs away from walls and other special tiles
            int stair_x = room->pos.x + 2 + rand() % (room->max.x - 4);
            int stair_y = room->pos.y + 2 + rand() % (room->max.y - 4);
            
            // Check if position is valid (not near doors, other stairs, or traps)
            bool valid = true;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int check_x = stair_x + dx;
                    int check_y = stair_y + dy;
                    if (level->tiles[check_y][check_x] == '+' ||
                        level->tiles[check_y][check_x] == '>' ||
                        level->tiles[check_y][check_x] == '<' ||
                        level->traps[check_y][check_x] ||
                        level->secret_walls[check_y][check_x]) {
                        valid = false;
                        break;
                    }
                }
                if (!valid) break;
            }
            
            if (valid && level->tiles[stair_y][stair_x] == '.') {
                level->secret_stairs[stair_y][stair_x] = true;
                break;
            }
            attempts++;
        }
    }
}
void draw_secret_room(Level *level) {
    // Clear the current level
    for (int y = 0; y < NUMLINES; y++) {
        for (int x = 0; x < NUMCOLS; x++) {
            level->tiles[y][x] = ' ';
            level->visible_tiles[y][x] = ' ';
            level->explored[y][x] = false;
        }
    }
    
    // Make the room bigger (7x7 instead of 5x5)
    int room_size = 7;
    int start_x = NUMCOLS/2 - room_size/2;
    int start_y = NUMLINES/2 - room_size/2;
    
    // Draw the secret room
    for (int y = 0; y < room_size; y++) {
        for (int x = 0; x < room_size; x++) {
            if (y == 0 || y == room_size-1) {
                level->tiles[start_y + y][start_x + x] = '_';
            } else if (x == 0 || x == room_size-1) {
                level->tiles[start_y + y][start_x + x] = '|';
            } else {
                level->tiles[start_y + y][start_x + x] = '.';
            }
            // Make the room always visible
            level->visible_tiles[start_y + y][start_x + x] = level->tiles[start_y + y][start_x + x];
            level->explored[start_y + y][start_x + x] = true;
        }
    }
    
    // Add exit marker in the center
    level->tiles[start_y + room_size/2][start_x + room_size/2] = '?';
    level->visible_tiles[start_y + room_size/2][start_x + room_size/2] = '?';
    
    // Add a single random talisman
    // Choose a random position near but not at the center
    int talisman_y = start_y + 1 + (rand() % (room_size - 2));
    int talisman_x = start_x + 1 + (rand() % (room_size - 2));
    
    // Make sure it's not where the exit marker is
    while (talisman_x == start_x + room_size/2 && talisman_y == start_y + room_size/2) {
        talisman_y = start_y + 1 + (rand() % (room_size - 2));
        talisman_x = start_x + 1 + (rand() % (room_size - 2));
    }
    
    // Place the talisman
    level->tiles[talisman_y][talisman_x] = 'T';
    level->visible_tiles[talisman_y][talisman_x] = 'T';
    level->talisman_type = rand() % TALISMAN_COUNT;  // Random talisman type
}
void add_secret_walls_to_room(Level *level, Room *room) {
    int door_count = 0;
    
    // Count doors in the room
    for (int y = room->pos.y; y < room->pos.y + room->max.y; y++) {
        for (int x = room->pos.x; x < room->pos.x + room->max.x; x++) {
            if (level->tiles[y][x] == '+') {
                door_count++;
            }
        }
    }
    
    // Only add secret walls to rooms with one door (dead ends)
    if (door_count == 1) {
        // Collect valid wall positions
        int wall_x[100], wall_y[100];
        int wall_count = 0;
        
        for (int y = room->pos.y; y < room->pos.y + room->max.y; y++) {
            for (int x = room->pos.x; x < room->pos.x + room->max.x; x++) {
                if (is_valid_secret_wall(level, x, y)) {
                    wall_x[wall_count] = x;
                    wall_y[wall_count] = y;
                    wall_count++;
                }
            }
        }
        
        // If we found valid walls, choose one randomly to be secret
        if (wall_count > 0) {
            int idx = rand() % wall_count;
            level->secret_walls[wall_y[idx]][wall_x[idx]] = true;
            
            // Create the associated secret room
            if (level->num_secret_rooms < MAXROOMS) {
                Room *secret_room = &level->secret_rooms[level->num_secret_rooms];
                secret_room->max.x = 5;
                secret_room->max.y = 5;
                secret_room->center.x = NUMCOLS/2;
                secret_room->center.y = NUMLINES/2;
                level->num_secret_rooms++;
            }
        }
    }
}
// Helper Functions
bool is_valid_secret_wall(Level *level, int x, int y) {
    // Must be a wall tile
    if (level->tiles[y][x] != '|' && level->tiles[y][x] != '_') {
        return false;
    }
    
    // Check if it's near a door
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < NUMCOLS && ny >= 0 && ny < NUMLINES) {
                if (level->tiles[ny][nx] == '+') {
                    return false;
                }
            }
        }
    }
    
    return true;
}
void set_message(Map *map, const char *message) {
    strncpy(map->current_message, message, MAX_MESSAGE_LENGTH - 1);
    map->current_message[MAX_MESSAGE_LENGTH - 1] = '\0';
    map->message_timer = MESSAGE_DURATION;
}
int MIN(int a, int b) {
    return (a < b) ? a : b;
}
bool check_for_stairs(Level *level) {
    for (int y = 0; y < NUMLINES; y++) {
        for (int x = 0; x < NUMCOLS; x++) {
            if (level->tiles[y][x] == '>') {
                return true;
            }
        }
    }
    return false;
}
void add_stairs(Level *level, Room *room, bool is_up) {
    if (is_up) {
        int x, y;
        bool valid_position = false;
        int attempts = 0;
        const int MAX_ATTEMPTS = 100;  // Increased attempts for better placement
        
        while (!valid_position && attempts < MAX_ATTEMPTS) {
            x = room->pos.x + 2 + rand() % (room->max.x - 4);  // Stay further from walls
            y = room->pos.y + 2 + rand() % (room->max.y - 4);  // Stay further from walls
            
            // Check if position is valid (not near doors or traps)
            bool near_door = false;
            
            // Check in a larger radius for doors
            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    if (y + dy >= 0 && y + dy < NUMLINES && 
                        x + dx >= 0 && x + dx < NUMCOLS) {
                        if (level->tiles[y + dy][x + dx] == '+') {
                            near_door = true;
                            break;
                        }
                    }
                }
                if (near_door) break;
            }
            
            if (!near_door && !level->traps[y][x] && level->tiles[y][x] == '.') {
                valid_position = true;
                level->stairs_up.x = x;
                level->stairs_up.y = y;
                level->tiles[y][x] = '>';
                level->stair_x = x;
                level->stair_y = y;
                level->stair_room = room;
            }
            
            attempts++;
        }

        // If we failed to place stairs in this room, try another room
        if (!valid_position) {
            // Try placing stairs in each room until we succeed
            for (int i = 0; i < level->num_rooms; i++) {
                Room *alt_room = &level->rooms[i];
                if (alt_room != room) {  // Don't try the same room again
                    attempts = 0;
                    while (!valid_position && attempts < MAX_ATTEMPTS) {
                        x = alt_room->pos.x + 2 + rand() % (alt_room->max.x - 4);
                        y = alt_room->pos.y + 2 + rand() % (alt_room->max.y - 4);
                        
                        if (!level->traps[y][x] && level->tiles[y][x] == '.') {
                            valid_position = true;
                            level->stairs_up.x = x;
                            level->stairs_up.y = y;
                            level->tiles[y][x] = '>';
                            level->stair_x = x;
                            level->stair_y = y;
                            level->stair_room = alt_room;
                            break;
                        }
                        attempts++;
                    }
                }
                if (valid_position) break;  // Stop if we successfully placed stairs
            }
        }
    } else {
        level->tiles[level->stair_y][level->stair_x] = '<';
    }
}
void copy_room(Room *dest, Room *src) {
    dest->pos = src->pos;
    dest->max = src->max;
    dest->center = src->center;
    dest->gone = src->gone;
    dest->connected = src->connected;
    memcpy(dest->doors, src->doors, sizeof(src->doors));
}

Map* create_map() {
    Map* map = malloc(sizeof(Map));
    if (map == NULL) return NULL;
    
    map->levels = malloc(MAX_LEVELS * sizeof(Level));
    if (map->levels == NULL) {
        free(map);
        return NULL;
    }

    for (int l = 0; l < MAX_LEVELS; l++) {
        Level *level = &map->levels[l];
        level->coins = malloc(NUMLINES * sizeof(bool*));
        level->coin_values = malloc(NUMLINES * sizeof(int*));
        if (!level->coins || !level->coin_values) {
            free_map(map);
            return NULL;
        }
        // Initialize all arrays first
        level->tiles = malloc(NUMLINES * sizeof(char*));
        level->visible_tiles = malloc(NUMLINES * sizeof(char*));
        level->explored = malloc(NUMLINES * sizeof(bool*));
        level->traps = malloc(NUMLINES * sizeof(bool*));
        level->discovered_traps = malloc(NUMLINES * sizeof(bool*));
        level->secret_walls = malloc(NUMLINES * sizeof(bool*));
        level->backup_tiles = malloc(NUMLINES * sizeof(char*));
        level->backup_visible_tiles = malloc(NUMLINES * sizeof(char*));
        level->backup_explored = malloc(NUMLINES * sizeof(bool*));
        level->secret_stairs = malloc(NUMLINES * sizeof(bool*));

        if (!level->tiles || !level->visible_tiles || !level->explored || 
            !level->traps || !level->discovered_traps || !level->secret_walls ||
            !level->backup_tiles || !level->backup_visible_tiles || 
            !level->backup_explored || !level->secret_stairs) {
            free_map(map);
            return NULL;
        }
        for (int i = 0; i < NUMLINES; i++) {
            // Add these allocations
            level->coins[i] = malloc(NUMCOLS * sizeof(bool));
            level->coin_values[i] = malloc(NUMCOLS * sizeof(int));
            
            if (!level->coins[i] || !level->coin_values[i]) {
                free_map(map);
                return NULL;
            }

            // Initialize the arrays
            for (int j = 0; j < NUMCOLS; j++) {
                level->coins[i][j] = false;
                level->coin_values[i][j] = 0;
            }
        }
        for (int i = 0; i < NUMLINES; i++) {
            level->tiles[i] = malloc(NUMCOLS * sizeof(char));
            level->visible_tiles[i] = malloc(NUMCOLS * sizeof(char));
            level->explored[i] = malloc(NUMCOLS * sizeof(bool));
            level->traps[i] = malloc(NUMCOLS * sizeof(bool));
            level->discovered_traps[i] = malloc(NUMCOLS * sizeof(bool));
            level->secret_walls[i] = malloc(NUMCOLS * sizeof(bool));
            level->backup_tiles[i] = malloc(NUMCOLS * sizeof(char));
            level->backup_visible_tiles[i] = malloc(NUMCOLS * sizeof(char));
            level->backup_explored[i] = malloc(NUMCOLS * sizeof(bool));
            level->secret_stairs[i] = malloc(NUMCOLS * sizeof(bool));

            if (!level->tiles[i] || !level->visible_tiles[i] || !level->explored[i] ||
                !level->traps[i] || !level->discovered_traps[i] || !level->secret_walls[i] ||
                !level->backup_tiles[i] || !level->backup_visible_tiles[i] ||
                !level->backup_explored[i] || !level->secret_stairs[i]) {
                free_map(map);
                return NULL;
            }

            // Initialize all arrays
            for (int j = 0; j < NUMCOLS; j++) {
                level->tiles[i][j] = ' ';
                level->visible_tiles[i][j] = ' ';
                level->explored[i][j] = false;
                level->traps[i][j] = false;
                level->discovered_traps[i][j] = false;
                level->secret_walls[i][j] = false;
                level->backup_tiles[i][j] = ' ';
                level->backup_visible_tiles[i][j] = ' ';
                level->backup_explored[i][j] = false;
                level->secret_stairs[i][j] = false;
                level->coins[i][j] = false;          // Initialize coins array
                level->coin_values[i][j] = 0;        // Initialize coin values array
            }
        }

        // Initialize other level properties
        level->num_rooms = 0;
        level->num_secret_rooms = 0;
        level->stair_room = NULL;
        level->current_secret_room = NULL;
        level->stair_x = -1;
        level->stair_y = -1;
        level->secret_entrance.x = -1;
        level->secret_entrance.y = -1;
    }
    map->food_count = 0;
    map->hunger = 100;
    map->hunger_timer = 0;
    map->food_menu_open = false;
    map->current_level = 1;
    map->player_x = 0;
    map->player_y = 0;
    map->debug_mode = false;
    map->current_message[0] = '\0';
    map->message_timer = 0;
    map->health = 12;
    map->strength = 16;
    map->gold = 0;
    map->armor = 0;
    map->exp = 0;
    map->is_treasure_room = false;
    map->prev_stair_x = -1;
    map->prev_stair_y = -1;
    map->weapons[WEAPON_MACE] = (Weapon){WEAPON_MACE, "Mace", "⚒", true};  // Hammer and pick
    map->weapons[WEAPON_DAGGER] = (Weapon){WEAPON_DAGGER, "Dagger", "🗡", false};  // Dagger
    map->weapons[WEAPON_WAND] = (Weapon){WEAPON_WAND, "Magic Wand", "⚚", false};  // Staff of Aesculapius
    map->weapons[WEAPON_ARROW] = (Weapon){WEAPON_ARROW, "Normal Arrow", "➳", false};  // Arrow
    map->weapons[WEAPON_SWORD] = (Weapon){WEAPON_SWORD, "Sword", "⚔", false};  // Crossed swords
    map->current_weapon = WEAPON_MACE;
    map->weapon_menu_open = false;
    map->talismans[TALISMAN_HEALTH] = (Talisman){TALISMAN_HEALTH, "Health Talisman", "◆", COLOR_PAIR(4), false};
    map->talismans[TALISMAN_DAMAGE] = (Talisman){TALISMAN_DAMAGE, "Damage Talisman", "◆", COLOR_PAIR(3), false};
    map->talismans[TALISMAN_SPEED] = (Talisman){TALISMAN_SPEED, "Speed Talisman", "◆", COLOR_PAIR(5), false};
    map->talisman_menu_open = false;
    load_game_settings(map);
    load_user_stats(map);
    return map;
}
void display_talisman_menu(WINDOW *win, Map *map) {
    int center_y = NUMLINES / 2;
    int center_x = NUMCOLS / 2;
    int box_width = 40;
    int box_height = 10;
    int start_y = center_y - (box_height / 2);
    int start_x = center_x - (box_width / 2);

    WINDOW *menu_win = newwin(box_height, box_width, start_y, start_x);

    // Draw box
    wattron(menu_win, COLOR_PAIR(1));
    box(menu_win, 0, 0);
    mvwprintw(menu_win, 0, 0, "╔");
    for (int i = 1; i < box_width - 1; i++) {
        mvwprintw(menu_win, 0, i, "═");
    }
    mvwprintw(menu_win, 0, box_width - 1, "╗");
    
    for (int i = 1; i < box_height - 1; i++) {
        mvwprintw(menu_win, i, 0, "║");
        mvwprintw(menu_win, i, box_width - 1, "║");
    }
    
    mvwprintw(menu_win, box_height - 1, 0, "╚");
    for (int i = 1; i < box_width - 1; i++) {
        mvwprintw(menu_win, box_height - 1, i, "═");
    }
    mvwprintw(menu_win, box_height - 1, box_width - 1, "╝");
    wattroff(menu_win, COLOR_PAIR(1));

    // Display talisman menu contents
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, 1, 2, "Talisman Collection");

    // List all talismans
    for (int i = 0; i < TALISMAN_COUNT; i++) {
        wattron(menu_win, map->talismans[i].color);
        mvwprintw(menu_win, 3 + i, 2, "%s %s %s", 
                  map->talismans[i].symbol,
                  map->talismans[i].name,
                  map->talismans[i].owned ? "[ACTIVE]" : "[NOT FOUND]");
        wattroff(menu_win, map->talismans[i].color);
    }

    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, box_height - 2, 2, "Press any key to close");
    wattroff(menu_win, COLOR_PAIR(2));

    wrefresh(menu_win);
    getch();

    // Clean up
    werase(menu_win);
    wrefresh(menu_win);
    delwin(menu_win);
    redrawwin(stdscr);
    refresh();
}
void display_weapon_menu(WINDOW *win, Map *map) {
    int center_y = NUMLINES / 2;
    int center_x = NUMCOLS / 2;
    int box_width = 40;
    int box_height = 12;
    int start_y = center_y - (box_height / 2);
    int start_x = center_x - (box_width / 2);

    WINDOW *menu_win = newwin(box_height, box_width, start_y, start_x);

    // Draw box
    wattron(menu_win, COLOR_PAIR(1));
    mvwprintw(menu_win, 0, 0, "╔");
    for (int i = 1; i < box_width - 1; i++) {
        mvwprintw(menu_win, 0, i, "═");
    }
    mvwprintw(menu_win, 0, box_width - 1, "╗");
    
    for (int i = 1; i < box_height - 1; i++) {
        mvwprintw(menu_win, i, 0, "║");
        mvwprintw(menu_win, i, box_width - 1, "║");
    }
    
    mvwprintw(menu_win, box_height - 1, 0, "╚");
    for (int i = 1; i < box_width - 1; i++) {
        mvwprintw(menu_win, box_height - 1, i, "═");
    }
    mvwprintw(menu_win, box_height - 1, box_width - 1, "╝");
    wattroff(menu_win, COLOR_PAIR(1));

    // Display weapon menu contents
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, 1, 2, "Weapon Inventory");
    mvwprintw(menu_win, 2, 2, "Current Weapon: %s %s", 
              map->weapons[map->current_weapon].name,
              map->weapons[map->current_weapon].symbol);
    
    // List all weapons
    for (int i = 0; i < WEAPON_COUNT; i++) {
        wattron(menu_win, map->weapons[i].owned ? COLOR_PAIR(4) : COLOR_PAIR(3));
        mvwprintw(menu_win, 4 + i, 2, "%d. %s %s %s", 
                  i + 1, 
                  map->weapons[i].symbol,
                  map->weapons[i].name,
                  map->weapons[i].owned ? "[OWNED]" : "[NOT FOUND]");
    }
    
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, box_height - 3, 2, "Press 1-5 to equip weapon");
    mvwprintw(menu_win, box_height - 2, 2, "Press any other key to close");
    wattroff(menu_win, COLOR_PAIR(2));
    
    wrefresh(menu_win);
    
    // Get input
    int ch = getch();
    
    // Handle weapon selection
    if (ch >= '1' && ch <= '5') {
        int weapon_index = ch - '1';
        if (map->weapons[weapon_index].owned) {
            map->current_weapon = weapon_index;
            char msg[MAX_MESSAGE_LENGTH];
            snprintf(msg, MAX_MESSAGE_LENGTH, "Equipped %s %s", 
                    map->weapons[weapon_index].name,
                    map->weapons[weapon_index].symbol);
            set_message(map, msg);
        } else {
            set_message(map, "You don't have this weapon yet!");
        }
    }
    
    // Clean up
    werase(menu_win);
    wrefresh(menu_win);
    delwin(menu_win);
    redrawwin(stdscr);
    refresh();
}
void display_food_menu(WINDOW *win, Map *map) {
    int center_y = NUMLINES / 2;
    int center_x = NUMCOLS / 2;
    int box_width = 40;
    int box_height = 10;
    int start_y = center_y - (box_height / 2);
    int start_x = center_x - (box_width / 2);

    // Create a new window for the menu
    WINDOW *menu_win = newwin(box_height, box_width, start_y, start_x);

    // Draw box
    wattron(menu_win, COLOR_PAIR(1));
    mvwprintw(menu_win, 0, 0, "╔");
    for (int i = 1; i < box_width - 1; i++) {
        mvwprintw(menu_win, 0, i, "═");
    }
    mvwprintw(menu_win, 0, box_width - 1, "╗");
    
    for (int i = 1; i < box_height - 1; i++) {
        mvwprintw(menu_win, i, 0, "║");
        mvwprintw(menu_win, i, box_width - 1, "║");
    }
    
    mvwprintw(menu_win, box_height - 1, 0, "╚");
    for (int i = 1; i < box_width - 1; i++) {
        mvwprintw(menu_win, box_height - 1, i, "═");
    }
    mvwprintw(menu_win, box_height - 1, box_width - 1, "╝");
    wattroff(menu_win, COLOR_PAIR(1));

    // Display food menu contents
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, 1, 2, "Food Menu");
    mvwprintw(menu_win, 2, 2, "Hunger: ");
    
    // Draw hunger bar
    wattron(menu_win, COLOR_PAIR(map->hunger > 30 ? 4 : 3));
    for (int i = 0; i < map->hunger / 5; i++) {
        mvwprintw(menu_win, 2, 10 + i, "█");
    }
    wattroff(menu_win, COLOR_PAIR(map->hunger > 30 ? 4 : 3));
    
    mvwprintw(menu_win, 4, 2, "Food Items: %d/5", map->food_count);
    mvwprintw(menu_win, 6, 2, "Press E to eat food");
    mvwprintw(menu_win, 7, 2, "Press any other key to close");
    wattroff(menu_win, COLOR_PAIR(2));
    
    // Show the window and wait for input
    wrefresh(menu_win);
    
    // Get input
    int ch = getch();
    
    // Clean up
    werase(menu_win);
    wrefresh(menu_win);
    delwin(menu_win);
    redrawwin(stdscr);  // Ensure background is redrawn properly
    refresh();

    // Handle eating food if 'E' was pressed
    if (ch == 'e' || ch == 'E') {
        if (map->food_count > 0 && map->hunger < 50) {
            map->food_count--;
            map->hunger = MIN(map->hunger + 10, 50);
            set_message(map, "You ate some food!");
        }
    }
}

void eat_food(Map *map) {
    if (map->food_count > 0) {
        map->food_count--;
        map->hunger = MIN(100, map->hunger + 30); // Restore 30 hunger
        map->health = MIN(20, map->health + 5);   // Restore 5 health
        set_message(map, "You eat some food. It was tasty!");
    } else {
        set_message(map, "You don't have any food!");
    }
}

void free_map(Map* map) {
    if (map == NULL) return;
    
    if (map->levels != NULL) {
        for (int l = 0; l < MAX_LEVELS; l++) {
            Level *level = &map->levels[l];
            
            if (level->tiles != NULL) {
                for (int i = 0; i < NUMLINES; i++) {
                    if (level->tiles[i]) free(level->tiles[i]);
                    if (level->coins[i]) free(level->coins[i]);
                    if (level->visible_tiles[i]) free(level->visible_tiles[i]);
                    if (level->explored[i]) free(level->explored[i]);
                    if (level->traps[i]) free(level->traps[i]);
                    if (level->discovered_traps[i]) free(level->discovered_traps[i]);
                    if (level->secret_walls[i]) free(level->secret_walls[i]);
                    if (level->backup_tiles[i]) free(level->backup_tiles[i]);
                    if (level->backup_visible_tiles[i]) free(level->backup_visible_tiles[i]);
                    if (level->backup_explored[i]) free(level->backup_explored[i]);
                }
                free(level->coins);
                free(level->tiles);
                free(level->visible_tiles);
                free(level->explored);
                free(level->traps);
                free(level->discovered_traps);
                free(level->secret_walls);
                free(level->backup_tiles);
                free(level->backup_visible_tiles);
                free(level->backup_explored);
                if (level->secret_stairs != NULL) {
                    for (int i = 0; i < NUMLINES; i++) {
                        if (level->secret_stairs[i]) free(level->secret_stairs[i]);
                    }
                    free(level->secret_stairs);
                }
                if (level->coin_values != NULL) {
                    for (int i = 0; i < NUMLINES; i++) {
                        if (level->coin_values[i]) free(level->coin_values[i]);
                    }
                    free(level->coin_values);
                }
            }
        }
        free(map->levels);
    }
    
    free(map);
}
bool rooms_overlap(Room *r1, Room *r2) {
    return !(r1->pos.x + r1->max.x + 1 < r2->pos.x ||
             r2->pos.x + r2->max.x + 1 < r1->pos.x ||
             r1->pos.y + r1->max.y + 1 < r2->pos.y ||
             r2->pos.y + r2->max.y + 1 < r1->pos.y);
}

void draw_room(Level *level, Room *room) {
    if (room->pos.x < 0 || room->pos.x + room->max.x >= NUMCOLS ||
        room->pos.y < 0 || room->pos.y + room->max.y >= NUMLINES) {
        return;
    }
    
    // Draw horizontal walls
    for (int x = room->pos.x + 1; x < room->pos.x + room->max.x - 1; x++) {
        level->tiles[room->pos.y][x] = '_';
        level->tiles[room->pos.y + room->max.y - 1][x] = '_';
    }
    
    // Draw vertical walls
    for (int y = room->pos.y + 1; y < room->pos.y + room->max.y - 1; y++) {
        level->tiles[y][room->pos.x] = '|';
        level->tiles[y][room->pos.x + room->max.x - 1] = '|';
    }
    
    // Fill floor and add features
    for (int y = room->pos.y + 1; y < room->pos.y + room->max.y - 1; y++) {
        for (int x = room->pos.x + 1; x < room->pos.x + room->max.x - 1; x++) {
            level->tiles[y][x] = '.';
            
            // Add coins (30% chance for regular coin, 5% for black coin)
            if (rand() % 100 < 3) {
                level->coins[y][x] = true;
                level->coin_values[y][x] = 1;
                level->tiles[y][x] = '$';
            } else if (rand() % 100 < 1) {
                level->coins[y][x] = true;
                level->coin_values[y][x] = 5;
                level->tiles[y][x] = '&';
            }
        }
    }
    
    // Add traps after coins
    int num_traps = rand() % 2;  // 0-1 traps per room
    for (int i = 0; i < num_traps; i++) {
        int trap_x = room->pos.x + 1 + (rand() % (room->max.x - 2));
        int trap_y = room->pos.y + 1 + (rand() % (room->max.y - 2));
        if (level->tiles[trap_y][trap_x] == '.' &&
            level->tiles[trap_y][trap_x] != '>' && 
            level->tiles[trap_y][trap_x] != '<') {
            level->traps[trap_y][trap_x] = true;
        }
    }
    if (rand() % 5 == 0) {
        int food_x = room->pos.x + 1 + (rand() % (room->max.x - 2));
        int food_y = room->pos.y + 1 + (rand() % (room->max.y - 2));
        if (level->tiles[food_y][food_x] == '.' && 
            !level->traps[food_y][food_x]) {
            level->tiles[food_y][food_x] = '*';
        }
    }
    if (level->num_rooms == 0) {
        int weapon_x = room->pos.x + 1 + (rand() % (room->max.x - 2));
        int weapon_y = room->pos.y + 1 + (rand() % (room->max.y - 2));
        if (level->tiles[weapon_y][weapon_x] == '.') {
            level->tiles[weapon_y][weapon_x] = 'W';
        }
    }
}

void connect_rooms(Level *level, Room *r1, Room *r2) {
    if (r1 == NULL || r2 == NULL) return;
    
    int start_x = r1->center.x;
    int start_y = r1->center.y;
    int end_x = r2->center.x;
    int end_y = r2->center.y;
    
    start_x = (start_x < 0) ? 0 : (start_x >= NUMCOLS ? NUMCOLS-1 : start_x);
    start_y = (start_y < 0) ? 0 : (start_y >= NUMLINES ? NUMLINES-1 : start_y);
    end_x = (end_x < 0) ? 0 : (end_x >= NUMCOLS ? NUMCOLS-1 : end_x);
    end_y = (end_y < 0) ? 0 : (end_y >= NUMLINES ? NUMLINES-1 : end_y);
    
    int current_x = start_x;
    int current_y = start_y;
    
    while (current_x != end_x) {
        if (current_x >= 0 && current_x < NUMCOLS && current_y >= 0 && current_y < NUMLINES) {
            char current_tile = level->tiles[current_y][current_x];
            if (level->tiles[current_y][current_x] == ' ' || 
                level->tiles[current_y][current_x] == '#') {
                level->tiles[current_y][current_x] = '#';
            }
        }
        current_x += (end_x > current_x) ? 1 : -1;
    }
    
    while (current_y != end_y) {
        if (current_x >= 0 && current_x < NUMCOLS && current_y >= 0 && current_y < NUMLINES) {
            if (level->tiles[current_y][current_x] == ' ' || 
                level->tiles[current_y][current_x] == '#') {
                level->tiles[current_y][current_x] = '#';
            }
        }
        current_y += (end_y > current_y) ? 1 : -1;
    }
    
    // Place doors
    for (int y = 0; y < NUMLINES; y++) {
        for (int x = 0; x < NUMCOLS; x++) {
            if (level->tiles[y][x] == '|' || level->tiles[y][x] == '_') {
                bool is_edge = (x == 0 || x == NUMCOLS-1 || y == 0 || y == NUMLINES-1);
                if (!is_edge && (
                    (x > 0 && level->tiles[y][x-1] == '#') ||
                    (x < NUMCOLS-1 && level->tiles[y][x+1] == '#') ||
                    (y > 0 && level->tiles[y-1][x] == '#') ||
                    (y < NUMLINES-1 && level->tiles[y+1][x] == '#'))) {
                    level->tiles[y][x] = '+';
                }
            }
        }
    }
}

void update_visibility(Map *map) {
    Level *current = &map->levels[map->current_level - 1];
    
    // First, copy explored areas to visible_tiles
    for (int y = 0; y < NUMLINES; y++) {
        for (int x = 0; x < NUMCOLS; x++) {
            if (current->explored[y][x] || map->debug_mode) {
                current->visible_tiles[y][x] = current->tiles[y][x];
                // Show secret walls only in debug mode
                if (map->debug_mode && current->secret_walls[y][x] && 
                    (current->tiles[y][x] == '|' || current->tiles[y][x] == '_')) {
                    current->visible_tiles[y][x] = '?';
                }
            } else {
                current->visible_tiles[y][x] = ' ';
            }
        }
    }

    // Check if player is in a room
    bool in_room = false;
    Room *current_room = NULL;
    
    for (int i = 0; i < current->num_rooms; i++) {
        Room *room = &current->rooms[i];
        if (map->player_x >= room->pos.x && 
            map->player_x < room->pos.x + room->max.x &&
            map->player_y >= room->pos.y && 
            map->player_y < room->pos.y + room->max.y) {
            in_room = true;
            current_room = room;
            break;
        }
    }

    // If we're in a room, reveal the whole room
    if (in_room && current_room != NULL) {
        for (int y = current_room->pos.y; y < current_room->pos.y + current_room->max.y; y++) {
            for (int x = current_room->pos.x; x < current_room->pos.x + current_room->max.x; x++) {
                current->explored[y][x] = true;
                current->visible_tiles[y][x] = current->tiles[y][x];
            }
        }
    } 
    // If we're in a corridor
    else if (current->tiles[map->player_y][map->player_x] == '#' || 
             current->tiles[map->player_y][map->player_x] == '+') {
        // Mark current position as explored
        current->explored[map->player_y][map->player_x] = true;
        
        // Only show the corridor in the direction we're moving
        bool horizontal_corridor = false;
        bool vertical_corridor = false;
        
        // Check if we're in a horizontal corridor
        if ((map->player_x > 0 && (current->tiles[map->player_y][map->player_x-1] == '#' || 
                                  current->tiles[map->player_y][map->player_x-1] == '+')) ||
            (map->player_x < NUMCOLS-1 && (current->tiles[map->player_y][map->player_x+1] == '#' || 
                                         current->tiles[map->player_y][map->player_x+1] == '+'))) {
            horizontal_corridor = true;
        }
        
        // Check if we're in a vertical corridor
        if ((map->player_y > 0 && (current->tiles[map->player_y-1][map->player_x] == '#' || 
                                  current->tiles[map->player_y-1][map->player_x] == '+')) ||
            (map->player_y < NUMLINES-1 && (current->tiles[map->player_y+1][map->player_x] == '#' || 
                                          current->tiles[map->player_y+1][map->player_x] == '+'))) {
            vertical_corridor = true;
        }
        
        // Only show corridor in the appropriate direction(s)
        if (horizontal_corridor) {
            // West
            for (int dx = 0; dx >= -5; dx--) {
                int x = map->player_x + dx;
                if (x >= 0 && x < NUMCOLS) {
                    if (current->tiles[map->player_y][x] == '#' || 
                        current->tiles[map->player_y][x] == '+') {
                        current->explored[map->player_y][x] = true;
                    } else break;
                }
            }
            // East
            for (int dx = 0; dx <= 5; dx++) {
                int x = map->player_x + dx;
                if (x >= 0 && x < NUMCOLS) {
                    if (current->tiles[map->player_y][x] == '#' || 
                        current->tiles[map->player_y][x] == '+') {
                        current->explored[map->player_y][x] = true;
                    } else break;
                }
            }
        }
        
        if (vertical_corridor) {
            // North
            for (int dy = 0; dy >= -5; dy--) {
                int y = map->player_y + dy;
                if (y >= 0 && y < NUMLINES) {
                    if (current->tiles[y][map->player_x] == '#' || 
                        current->tiles[y][map->player_x] == '+') {
                        current->explored[y][map->player_x] = true;
                    } else break;
                }
            }
            // South
            for (int dy = 0; dy <= 5; dy++) {
                int y = map->player_y + dy;
                if (y >= 0 && y < NUMLINES) {
                    if (current->tiles[y][map->player_x] == '#' || 
                        current->tiles[y][map->player_x] == '+') {
                        current->explored[y][map->player_x] = true;
                    } else break;
                }
            }
        }
    }
}

void generate_map(Map *map) {
    Level *current = &map->levels[map->current_level - 1];
        if (map->current_level == 5) {
        // Clear the level
        for (int y = 0; y < NUMLINES; y++) {
            for (int x = 0; x < NUMCOLS; x++) {
                current->tiles[y][x] = ' ';
                current->visible_tiles[y][x] = ' ';
                current->explored[y][x] = false;
                current->traps[y][x] = false;
                current->discovered_traps[y][x] = false;
                current->secret_walls[y][x] = false;
                current->secret_stairs[y][x] = false;
                current->coins[y][x] = false;           // Add this
                current->coin_values[y][x] = 0; 
            }
        }
        current->num_rooms = 0;
        current->num_secret_rooms = 0;
        current->current_secret_room = NULL;
        // Create one large treasure room
        Room treasure_room;
        treasure_room.pos.x = NUMCOLS / 4;
        treasure_room.pos.y = NUMLINES / 4;
        treasure_room.max.x = NUMCOLS / 2;
        treasure_room.max.y = NUMLINES / 2;
        treasure_room.center.x = treasure_room.pos.x + treasure_room.max.x / 2;
        treasure_room.center.y = treasure_room.pos.y + treasure_room.max.y / 2;
        
        // Draw the treasure room
        for (int y = treasure_room.pos.y; y < treasure_room.pos.y + treasure_room.max.y; y++) {
            for (int x = treasure_room.pos.x; x < treasure_room.pos.x + treasure_room.max.x; x++) {
                if (y == treasure_room.pos.y || y == treasure_room.pos.y + treasure_room.max.y - 1)
                    current->tiles[y][x] = '_';
                else if (x == treasure_room.pos.x || x == treasure_room.pos.x + treasure_room.max.x - 1)
                    current->tiles[y][x] = '|';
                else
                    current->tiles[y][x] = '.';
            }
        }
        for (int y = treasure_room.pos.y + 1; y < treasure_room.pos.y + treasure_room.max.y - 1; y++) {
            for (int x = treasure_room.pos.x + 1; x < treasure_room.pos.x + treasure_room.max.x - 1; x++) {
                if (current->tiles[y][x] == '.' && !current->traps[y][x]) {
                    // 60% chance for normal coin in treasure room (more coins since it's the treasure room!)
                    if (rand() % 100 < 60) {
                        current->coins[y][x] = true;
                        current->coin_values[y][x] = 1;
                        current->tiles[y][x] = '$';
                    }
                    // 20% chance for black coin in treasure room (higher chance since it's the treasure room)
                    else if (rand() % 100 < 20) {
                        current->coins[y][x] = true;
                        current->coin_values[y][x] = 5;
                        current->tiles[y][x] = '&';
                    }
                }
            }
        }
        // Add down stairs
        if (map->current_level < MAX_LEVELS && current->num_rooms > 1) {
            // Try to place stairs in each room except the first room
            bool stairs_placed = false;
            int max_placement_attempts = 10;  // Try each room multiple times
            
            for (int attempts = 0; attempts < max_placement_attempts && !stairs_placed; attempts++) {
                for (int i = 1; i < current->num_rooms && !stairs_placed; i++) {  // Start from 1 to skip first room
                    Room *room = &current->rooms[i];
                    add_stairs(current, room, true);
                    
                    // Verify if stairs were actually placed
                    if (check_for_stairs(current)) {
                        current->stair_room = room;
                        stairs_placed = true;
                        break;
                    }
                }
            }

            // If still no stairs, regenerate the map
            if (!stairs_placed) {
                current->num_rooms = 0;
                for (int y = 0; y < NUMLINES; y++) {
                    for (int x = 0; x < NUMCOLS; x++) {
                        current->tiles[y][x] = ' ';
                    }
                }
                generate_map(map);  // Recursive call to try again with a new map
                return;
            }
        }
        
        // Store the room
        current->rooms[0] = treasure_room;
        current->num_rooms = 1;
        
        // Set player position
        map->player_x = treasure_room.center.x;
        map->player_y = treasure_room.center.y;
        
        return;
    }
    current->num_rooms = 0;
    int attempts = 0;
    const int MAX_ATTEMPTS = 50;
    int target_rooms = 6 + (rand() % 4);
    
    // Clear the level
    for (int y = 0; y < NUMLINES; y++) {
        for (int x = 0; x < NUMCOLS; x++) {
            current->tiles[y][x] = ' ';
            current->visible_tiles[y][x] = ' ';
            current->explored[y][x] = false;
            current->traps[y][x] = false;
            current->discovered_traps[y][x] = false;
            current->secret_stairs[y][x] = false; 
            current->secret_walls[y][x] = false;
            current->coins[y][x] = false;           // Add this
            current->coin_values[y][x] = 0; 

        }
    }
    
    int cell_width = NUMCOLS / 3;
    int cell_height = NUMLINES / 3;
    
    if (cell_width < MIN_ROOM_SIZE + 2 || cell_height < MIN_ROOM_SIZE + 2) {
        return;
    }
    
    while (current->num_rooms < target_rooms && attempts < MAX_ATTEMPTS) {
        Room new_room;
        
        int grid_x = current->num_rooms % 3;
        int grid_y = current->num_rooms / 3;
        
        new_room.max.x = MIN_ROOM_SIZE + rand() % (MIN(MAX_ROOM_SIZE - MIN_ROOM_SIZE + 1, cell_width - 2));
        new_room.max.y = MIN_ROOM_SIZE + rand() % (MIN(MAX_ROOM_SIZE - MIN_ROOM_SIZE + 1, cell_height - 2));
        
        new_room.pos.x = grid_x * cell_width + rand() % (cell_width - new_room.max.x - 1) + 1;
        new_room.pos.y = grid_y * cell_height + rand() % (cell_height - new_room.max.y - 1) + 1;
        
        // Add this check to prevent rooms from being generated too low
        if (new_room.pos.y + new_room.max.y > NUMLINES - 6) {  // Leave space for stats
            attempts++;
            continue;  // Skip this room and try again
        }
        
        new_room.center.x = new_room.pos.x + new_room.max.x / 2;
        new_room.center.y = new_room.pos.y + new_room.max.y / 2;
        
        bool valid = true;
        for (int j = 0; j < current->num_rooms; j++) {
            if (rooms_overlap(&new_room, &current->rooms[j])) {
                valid = false;
                break;
            }
        }
        
        if (valid) {
            current->rooms[current->num_rooms] = new_room;
            draw_room(current, &new_room);
            current->num_rooms++;
        }
        attempts++;
        
        if (attempts >= MAX_ATTEMPTS && current->num_rooms < 6) {
            current->num_rooms = 0;
            attempts = 0;
            for (int y = 0; y < NUMLINES; y++) {
                for (int x = 0; x < NUMCOLS; x++) {
                    current->tiles[y][x] = ' ';
                }
            }
        }
    }
    for (int i = 0; i < current->num_rooms; i++) {
        add_secret_stairs(current, &current->rooms[i]);
    }
    // Connect rooms
    for (int i = 1; i < current->num_rooms; i++) {
        connect_rooms(current, &current->rooms[i-1], &current->rooms[i]);
    }
    for (int i = 0; i < current->num_rooms; i++) {
        add_secret_walls_to_room(current, &current->rooms[i]);
    }
    // Add stairs if not first level
    if (map->current_level < MAX_LEVELS && current->num_rooms > 1) {
        // Find a room that's not the first room for up stairs
        for (int i = 1; i < current->num_rooms; i++) {
            Room *room = &current->rooms[i];
            add_stairs(current, room, true);
            current->stair_room = room;
            break;
        }
    }
        
    // Set player position in first room
    if (current->num_rooms > 0) {
        Room *first_room = &current->rooms[0];
        map->player_x = first_room->center.x;
        map->player_y = first_room->center.y;
        
        for (int y = first_room->pos.y; y < first_room->pos.y + first_room->max.y; y++) {
            for (int x = first_room->pos.x; x < first_room->pos.x + first_room->max.x; x++) {
                current->explored[y][x] = true;
            }
        }
    }
}

void transition_level(Map *map, bool going_up) {
    Level *current = &map->levels[map->current_level - 1];
    
    if (going_up) {
        if (map->current_level <= MAX_LEVELS) {
            // Store current room information before transitioning
            Room *current_room = NULL;
            // Find the room containing the stairs
            for (int i = 0; i < current->num_rooms; i++) {
                Room *room = &current->rooms[i];
                if (map->player_x >= room->pos.x && map->player_x < room->pos.x + room->max.x &&
                    map->player_y >= room->pos.y && map->player_y < room->pos.y + room->max.y) {
                    current_room = room;
                    break;
                }
            }
            
            // Move to next level
            map->current_level++;
            Level *next = &map->levels[map->current_level - 1]; 

            // Handle treasure room (level 5)
            if (map->current_level == 5) {
                // Clear the next level completely
                for (int y = 0; y < NUMLINES; y++) {
                    for (int x = 0; x < NUMCOLS; x++) {
                        next->tiles[y][x] = ' ';
                        next->visible_tiles[y][x] = ' ';
                        next->explored[y][x] = false;
                        next->traps[y][x] = false;
                        next->discovered_traps[y][x] = false;
                        next->coins[y][x] = false;
                        next->coin_values[y][x] = 0;
                        next->secret_walls[y][x] = false;
                        next->secret_stairs[y][x] = false;
                    }
                }

                // Create treasure room
                Room treasure_room;
                int center_x = NUMCOLS / 2;
                int center_y = NUMLINES / 2;
                int room_width = NUMCOLS / 4;   // Instead of NUMCOLS/2
                int room_height = NUMLINES / 4; 
                treasure_room.pos.x = center_x - (room_width / 2);
                treasure_room.pos.y = center_y - (room_height / 2);
                treasure_room.max.x = room_width;
                treasure_room.max.y = room_height;
                treasure_room.center.x = treasure_room.pos.x + treasure_room.max.x / 2;
                treasure_room.center.y = treasure_room.pos.y + treasure_room.max.y / 2;

                // Draw the treasure room
                for (int y = treasure_room.pos.y; y < treasure_room.pos.y + treasure_room.max.y; y++) {
                    for (int x = treasure_room.pos.x; x < treasure_room.pos.x + treasure_room.max.x; x++) {
                        if (y == treasure_room.pos.y || y == treasure_room.pos.y + treasure_room.max.y - 1)
                            next->tiles[y][x] = '_';
                        else if (x == treasure_room.pos.x || x == treasure_room.pos.x + treasure_room.max.x - 1)
                            next->tiles[y][x] = '|';
                        else
                            next->tiles[y][x] = '.';
                    }
                }

                // Generate coins in treasure room (more generous distribution)
                for (int y = treasure_room.pos.y + 1; y < treasure_room.pos.y + treasure_room.max.y - 1; y++) {
                    for (int x = treasure_room.pos.x + 1; x < treasure_room.pos.x + treasure_room.max.x - 1; x++) {
                        if (next->tiles[y][x] == '.' && next->tiles[y][x] != '>') {
                            // 60% chance for normal coin
                            if (rand() % 100 < 5) {
                                next->coins[y][x] = true;
                                next->coin_values[y][x] = 1;
                                next->tiles[y][x] = '$';
                            }
                            // 20% chance for black coin
                            else if (rand() % 100 < 3) {
                                next->coins[y][x] = true;
                                next->coin_values[y][x] = 5;
                                next->tiles[y][x] = '&';
                            }
                        }
                    }
                }

                // Add traps after coins
                int num_traps = 8 + rand() % 5;  // 8-12 traps
                for (int i = 0; i < num_traps; i++) {
                    int trap_x = treasure_room.pos.x + 1 + rand() % (treasure_room.max.x - 2);
                    int trap_y = treasure_room.pos.y + 1 + rand() % (treasure_room.max.y - 2);
                    if (next->tiles[trap_y][trap_x] == '.' || 
                        next->tiles[trap_y][trap_x] == '$' || 
                        next->tiles[trap_y][trap_x] == '&' && next->tiles[trap_y][trap_x] != '>') {
                        next->traps[trap_y][trap_x] = true;
                    }
                }

                // Add stairs
                next->stairs_down.x = treasure_room.center.x;
                next->stairs_down.y = treasure_room.pos.y + 1;
                next->tiles[next->stairs_down.y][next->stairs_down.x] = '<';

                int victory_x = treasure_room.center.x;
                int victory_y = treasure_room.pos.y + treasure_room.max.y - 2;
                next->tiles[victory_y][victory_x] = '>';

                // Store room and set player position
                next->rooms[0] = treasure_room;
                next->num_rooms = 1;
                map->player_x = next->stairs_down.x;
                map->player_y = next->stairs_down.y;

                set_message(map, "You've reached the Treasure Room! Be careful of traps!");
                next->stairs_placed = true;
            }
            // Handle regular level transition
            else if (!next->stairs_placed) {
                // Clear the next level
                for (int y = 0; y < NUMLINES; y++) {
                    for (int x = 0; x < NUMCOLS; x++) {
                        next->tiles[y][x] = ' ';
                        next->visible_tiles[y][x] = ' ';
                        next->explored[y][x] = false;
                        next->traps[y][x] = false;
                        next->discovered_traps[y][x] = false;
                        next->coins[y][x] = false;
                        next->coin_values[y][x] = 0;
                        next->secret_walls[y][x] = false;
                        next->secret_stairs[y][x] = false;
                    }
                }
                
                if (current_room != NULL) {
                    // Copy the room structure
                    next->rooms[0] = *current_room;
                    next->num_rooms = 1;
                    
                    // Copy room contents
                    for (int y = current_room->pos.y; y < current_room->pos.y + current_room->max.y; y++) {
                        for (int x = current_room->pos.x; x < current_room->pos.x + current_room->max.x; x++) {
                            next->tiles[y][x] = current->tiles[y][x];
                            if (x == map->player_x && y == map->player_y) {
                                next->tiles[y][x] = '<';
                                next->stairs_down.x = x;
                                next->stairs_down.y = y;
                            }
                        }
                    }
                    
                    // Generate remaining rooms and features
                    generate_remaining_rooms(map);
                    next->stairs_placed = true;
                }
            }
            
            // Place player at stairs
            map->player_x = next->stairs_down.x;
            map->player_y = next->stairs_down.y;
            
            char msg[MAX_MESSAGE_LENGTH];
            snprintf(msg, MAX_MESSAGE_LENGTH, "Ascending to level %d", map->current_level);
            set_message(map, msg);
        }
    }
    update_visibility(map);
}
// New function to generate remaining rooms after copying the first room
// In generate_remaining_rooms function:
void generate_remaining_rooms(Map *map) {
    Level *current = &map->levels[map->current_level - 1];
    int attempts = 0;
    const int MAX_ATTEMPTS = 50;
    
    while (current->num_rooms < MAXROOMS && attempts < MAX_ATTEMPTS) {
        Room new_room;
        new_room.max.x = MIN_ROOM_SIZE + rand() % (MAX_ROOM_SIZE - MIN_ROOM_SIZE + 1);
        new_room.max.y = MIN_ROOM_SIZE + rand() % (MAX_ROOM_SIZE - MIN_ROOM_SIZE + 1);
        
        new_room.pos.x = 1 + rand() % (NUMCOLS - new_room.max.x - 2);
        new_room.pos.y = 1 + rand() % (NUMLINES - new_room.max.y - 2);
        
        // Add this check to prevent rooms from being generated too low
        if (new_room.pos.y + new_room.max.y > NUMLINES - 6) {  // Leave space for stats
            attempts++;
            continue;  // Skip this room and try again
        }
        
        new_room.center.x = new_room.pos.x + new_room.max.x / 2;
        new_room.center.y = new_room.pos.y + new_room.max.y / 2;
        
        // Check for overlap with existing rooms
        bool valid = true;
        for (int i = 0; i < current->num_rooms; i++) {
            if (rooms_overlap(&new_room, &current->rooms[i])) {
                valid = false;
                break;
            }
        }
        
        if (valid) {
            current->rooms[current->num_rooms] = new_room;
            draw_room(current, &new_room);
            
            // Connect to previous room
            if (current->num_rooms > 0) {
                connect_rooms(current, &current->rooms[current->num_rooms - 1], &new_room);
            }
            
            // If this is the last room, add up stairs
            if (current->num_rooms == MAXROOMS - 1) {
                int stair_x = new_room.pos.x + 1 + rand() % (new_room.max.x - 2);
                int stair_y = new_room.pos.y + 1 + rand() % (new_room.max.y - 2);
                current->tiles[stair_y][stair_x] = '>';
                current->stairs_up.x = stair_x;
                current->stairs_up.y = stair_y;
            }
            
            current->num_rooms++;
        }
        attempts++;
    }
}

void handle_input(Map *map, int input) {
    Level *current = &map->levels[map->current_level - 1];
    int new_x = map->player_x;
    int new_y = map->player_y;

    // Handle fast travel first, before any other input processing
    if (input == 'f' || input == 'F') {
        set_message(map, "Fast travel mode: Press direction key (W/A/S/D)");
        fast_travel_mode = true;
        refresh();
        return;
    }
    if (input == 't' || input == 'T') {
        display_talisman_menu(stdscr, map);
        clear();
        refresh();
        update_visibility(map);
        return;
    }
    if (input == '\n' || input == '\r') {
        // Check the current tile for talisman
        if (current->tiles[map->player_y][map->player_x] == 'T') {
            int talisman_type = current->talisman_type;
            if (!map->talismans[talisman_type].owned) {
                map->talismans[talisman_type].owned = true;
                current->tiles[map->player_y][map->player_x] = '.';
                
                char msg[MAX_MESSAGE_LENGTH];
                switch(talisman_type) {
                    case TALISMAN_HEALTH:
                        map->health += 10;
                        set_message(map, "You obtained the Health Talisman! +10 Health");
                        break;
                    case TALISMAN_DAMAGE:
                        map->strength += 5;
                        set_message(map, "You obtained the Damage Talisman! +5 Strength");
                        break;
                    case TALISMAN_SPEED:
                        set_message(map, "You obtained the Speed Talisman!");
                        break;
                }
            }
        }
        
        // Also check adjacent tiles for talismans
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int check_x = map->player_x + dx;
                int check_y = map->player_y + dy;
                
                if (check_x >= 0 && check_x < NUMCOLS && check_y >= 0 && check_y < NUMLINES) {
                    if (current->tiles[check_y][check_x] == 'T') {
                        int talisman_type = current->talisman_type;
                        if (!map->talismans[talisman_type].owned) {
                            map->talismans[talisman_type].owned = true;
                            current->tiles[check_y][check_x] = '.';
                            
                            switch(talisman_type) {
                                case TALISMAN_HEALTH:
                                    map->health += 10;
                                    set_message(map, "You obtained the Health Talisman! +10 Health");
                                    break;
                                case TALISMAN_DAMAGE:
                                    map->strength += 5;
                                    set_message(map, "You obtained the Damage Talisman! +5 Strength");
                                    break;
                                case TALISMAN_SPEED:
                                    set_message(map, "You obtained the Speed Talisman!");
                                    break;
                            }
                        }
                    }
                }
            }
        }
    }
    if (input == 'i' || input == 'I') {
        display_weapon_menu(stdscr, map);
        clear();
        refresh();
        update_visibility(map);
        return;
    }
    if (fast_travel_mode) {
        int dx = 0, dy = 0;
        switch (tolower(input)) {
            case 'w': dy = -1; break;
            case 's': dy = 1; break;
            case 'a': dx = -1; break;
            case 'd': dx = 1; break;
            default: 
                fast_travel_mode = false;
                return;
        }

        // Keep moving until hitting a wall or special tile
        while (true) {
            int test_x = map->player_x + dx;
            int test_y = map->player_y + dy;

            // Check boundaries and walls
            if (test_x < 0 || test_x >= NUMCOLS || 
                test_y < 0 || test_y >= NUMLINES ||
                current->tiles[test_y][test_x] == '|' || 
                current->tiles[test_y][test_x] == '_' || 
                current->tiles[test_y][test_x] == ' ' ||
                current->tiles[test_y][test_x] == '>' ||
                current->tiles[test_y][test_x] == '<' ||
                current->secret_walls[test_y][test_x]) {
                break;
            }

            // Move to new position
            map->player_x = test_x;
            map->player_y = test_y;

            // Handle traps
            if (current->traps[test_y][test_x] && !current->discovered_traps[test_y][test_x]) {
                int damage = 2 + (rand() % 3);
                map->health -= damage;
                current->discovered_traps[test_y][test_x] = true;
                char msg[MAX_MESSAGE_LENGTH];
                snprintf(msg, MAX_MESSAGE_LENGTH, "You triggered a trap! Lost %d health!", damage);
                set_message(map, msg);
                break;  // Stop fast travel if we hit a trap
            }
            update_visibility(map);
        }

        fast_travel_mode = false;
        return;
    }
    static int last_arrow = -1;
    static clock_t last_time = 0;
    clock_t current_time = clock();
    const double delay = 0.2; // 200ms delay for arrow combination
    if (input == 'e' || input == 'E') {
        display_food_menu(stdscr, map);
        int menu_input = getch();
        if (menu_input == 'e' || menu_input == 'E') {
            eat_food(map);
        }
        // Redraw the screen after closing menu
        clear();
        refresh();
        update_visibility(map);
    }
    // Handle arrow keys for diagonal movement
    if (input == KEY_UP || input == KEY_DOWN || input == KEY_LEFT || input == KEY_RIGHT) {
        if (last_arrow != -1 && (current_time - last_time) / CLOCKS_PER_SEC < delay) {
            // Handle diagonal combinations
            if ((last_arrow == KEY_UP && input == KEY_LEFT) || 
                (last_arrow == KEY_LEFT && input == KEY_UP)) {
                // Northwest
                new_x--;
                new_y--;
                last_arrow = -1;
            } else if ((last_arrow == KEY_UP && input == KEY_RIGHT) || 
                      (last_arrow == KEY_RIGHT && input == KEY_UP)) {
                // Northeast
                new_x++;
                new_y--;
                last_arrow = -1;
            } else if ((last_arrow == KEY_DOWN && input == KEY_LEFT) || 
                      (last_arrow == KEY_LEFT && input == KEY_DOWN)) {
                // Southwest
                new_x--;
                new_y++;
                last_arrow = -1;
            } else if ((last_arrow == KEY_DOWN && input == KEY_RIGHT) || 
                      (last_arrow == KEY_RIGHT && input == KEY_DOWN)) {
                // Southeast
                new_x++;
                new_y++;
                last_arrow = -1;
            }
        } else {
            last_arrow = input;
            last_time = current_time;
            return;
        }
    }
    
    // If we're in a secret room (from either secret wall or secret stairs)
    if (current->current_secret_room != NULL) {
        switch (input) {
            case 'w': case 'W': new_y--; break;
            case 's': case 'S': new_y++; break;
            case 'a': case 'A': new_x--; break;
            case 'd': case 'D': new_x++; break;
            case '\n': case '\r':
                // If near the secret entrance (center of room), exit
                if (abs(map->player_x - current->current_secret_room->center.x) <= 1 &&
                    abs(map->player_y - current->current_secret_room->center.y) <= 1) {
                    // Restore everything
                    for (int y = 0; y < NUMLINES; y++) {
                        for (int x = 0; x < NUMCOLS; x++) {
                            current->tiles[y][x] = current->backup_tiles[y][x];
                            current->visible_tiles[y][x] = current->backup_visible_tiles[y][x];
                            current->explored[y][x] = current->backup_explored[y][x];
                        }
                    }
                    
                    // Return to the entrance
                    map->player_x = current->secret_entrance.x;
                    map->player_y = current->secret_entrance.y;
                    
                    // If we came from secret stairs, mark them as discovered
                    if (current->secret_stairs[map->player_y][map->player_x]) {
                        current->visible_tiles[map->player_y][map->player_x] = '%';
                        current->explored[map->player_y][map->player_x] = true;
                    }
                    // If we came from secret walls, mark them as discovered
                    else if (current->secret_walls[map->player_y][map->player_x]) {
                        current->visible_tiles[map->player_y][map->player_x] = '?';
                        current->explored[map->player_y][map->player_x] = true;
                    }
                    
                    current->current_secret_room = NULL;
                    set_message(map, "You return from the secret room.");
                    return;
                }
                break;
        }
        
        // Validate movement within secret room
        if (new_x >= 0 && new_x < NUMCOLS && new_y >= 0 && new_y < NUMLINES) {
            // Allow movement within the 7x7 secret room area
            if (abs(new_x - current->current_secret_room->center.x) <= 3 &&
                abs(new_y - current->current_secret_room->center.y) <= 3) {
                char next_tile = current->tiles[new_y][new_x];
                if (next_tile != '|' && next_tile != '_') {
                    map->player_x = new_x;
                    map->player_y = new_y;
                }
            }
        }
    }
    // Regular room handling
    else {
        switch (input) {
            case 'w': case 'W': new_y--; break;
            case 's': case 'S': new_y++; break;
            case 'a': case 'A': new_x--; break;
            case 'd': case 'D': new_x++; break;
            case '\n': case '\r':
                if (current->tiles[new_y][new_x] == '*') {
                    if (map->food_count < 5) {
                        map->food_count++;
                        current->tiles[new_y][new_x] = '.';
                        set_message(map, "You found some food!");
                    } else {
                        set_message(map, "You can't carry any more food!");
                    }
                }
                if (current->tiles[new_y][new_x] == 'T') {
                    int talisman_type = current->talisman_type;
                    if (!map->talismans[talisman_type].owned) {
                        map->talismans[talisman_type].owned = true;
                        current->tiles[new_y][new_x] = '.';
                        
                        char msg[MAX_MESSAGE_LENGTH];
                        snprintf(msg, MAX_MESSAGE_LENGTH, "You obtained the %s!", 
                                map->talismans[talisman_type].name);
                        set_message(map, msg);
                        
                        // Apply talisman effects
                        switch(talisman_type) {
                            case TALISMAN_HEALTH:
                                map->health += 10;
                                break;
                            case TALISMAN_DAMAGE:
                                map->strength += 5;
                                break;
                            case TALISMAN_SPEED:
                                // Could implement a speed boost effect here
                                break;
                        }
                    }
                }
                if (current->tiles[new_y][new_x] == 'W') {
                    // Pick a random weapon that player doesn't have yet
                    int available_weapons[WEAPON_COUNT];
                    int count = 0;
                    for (int i = 0; i < WEAPON_COUNT; i++) {
                        if (!map->weapons[i].owned) {
                            available_weapons[count++] = i;
                        }
                    }
                    if (count > 0) {
                        int weapon_index = available_weapons[rand() % count];
                        map->weapons[weapon_index].owned = true;
                        current->tiles[new_y][new_x] = '.';
                        char msg[MAX_MESSAGE_LENGTH];
                        snprintf(msg, MAX_MESSAGE_LENGTH, "You found a %s!", map->weapons[weapon_index].name);
                        set_message(map, msg);
                    }
                }
                // Handle stairs
                if (current->tiles[map->player_y][map->player_x] == '>') {
                    set_message(map, "Press Enter to go up to next level");
                    if (map->current_level == 5) {
                        // Victory condition - reached the stairs in treasure room
                        save_user_data(map);
                        show_win_screen(map);
                        free_map(map);
                        clear();
                        refresh();
                        endwin();
                        curs_set(1);
                        system("clear");
                        exit(0);
                    }
                    transition_level(map, true);
                    return;
                } else if (current->tiles[map->player_y][map->player_x] == '<') {
                    set_message(map, "Press Enter to go down to previous level");
                    if (map->current_level > 1) {
                        transition_level(map, false);
                    }
                    return;
                }
                
                // Check for secret stairs
                if (current->secret_stairs[map->player_y][map->player_x]) {
                    // Backup everything before entering secret room
                    for (int y = 0; y < NUMLINES; y++) {
                        for (int x = 0; x < NUMCOLS; x++) {
                            current->backup_tiles[y][x] = current->tiles[y][x];
                            current->backup_visible_tiles[y][x] = current->visible_tiles[y][x];
                            current->backup_explored[y][x] = current->explored[y][x];
                        }
                    }
                    
                    // Enter secret stair room
                    current->secret_entrance.x = map->player_x;
                    current->secret_entrance.y = map->player_y;
                    current->current_secret_room = &current->secret_rooms[0];
                    map->player_x = current->current_secret_room->center.x;
                    map->player_y = current->current_secret_room->center.y;
                    draw_secret_room(current);
                    set_message(map, "You descend the mysterious stairs into a secret Talisman room!");
                    return;
                }
                
                // Check if player is adjacent to a secret wall
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int check_x = map->player_x + dx;
                        int check_y = map->player_y + dy;
                        
                        if (check_x >= 0 && check_x < NUMCOLS && 
                            check_y >= 0 && check_y < NUMLINES) {
                            if (current->secret_walls[check_y][check_x]) {
                                // Backup everything before entering secret room
                                for (int y = 0; y < NUMLINES; y++) {
                                    for (int x = 0; x < NUMCOLS; x++) {
                                        current->backup_tiles[y][x] = current->tiles[y][x];
                                        current->backup_visible_tiles[y][x] = current->visible_tiles[y][x];
                                        current->backup_explored[y][x] = current->explored[y][x];
                                    }
                                }
                                
                                // Enter secret room
                                current->secret_entrance.x = map->player_x;
                                current->secret_entrance.y = map->player_y;
                                current->current_secret_room = &current->secret_rooms[0];
                                map->player_x = current->current_secret_room->center.x;
                                map->player_y = current->current_secret_room->center.y;
                                draw_secret_room(current);
                                set_message(map, "You enter a secret Talisman room!");
                                return;
                            }
                        }
                    }
                }
                break;
        }
        
        // Handle regular movement
        if (new_x >= 0 && new_x < NUMCOLS && new_y >= 0 && new_y < NUMLINES) {
            // Check if trying to move into a secret wall
            if (current->secret_walls[new_y][new_x]) {
                set_message(map, "You sense something strange about this wall. Press Enter to investigate.");
                return;
            }
            
            char next_tile = current->tiles[new_y][new_x];
            if (next_tile != '|' && next_tile != '_' && next_tile != ' ') {
                // Handle traps
                if (current->traps[new_y][new_x] && !current->discovered_traps[new_y][new_x]) {
                    int damage = 2 + (rand() % 3);
                    map->health -= damage;
                    current->discovered_traps[new_y][new_x] = true;
                    char msg[MAX_MESSAGE_LENGTH];
                    snprintf(msg, MAX_MESSAGE_LENGTH, "You triggered a trap! Lost %d health!", damage);
                    set_message(map, msg);
                }
                if (new_x >= 0 && new_x < NUMCOLS && new_y >= 0 && new_y < NUMLINES) {
                    if (current->coins[new_y][new_x]) {
                        int coin_value = current->coin_values[new_y][new_x];
                        map->gold += coin_value;
                        current->coins[new_y][new_x] = false;
                        current->coin_values[new_y][new_x] = 0;
                        current->tiles[new_y][new_x] = '.';
                        
                        char msg[MAX_MESSAGE_LENGTH];
                        if (coin_value == 1) {
                            snprintf(msg, MAX_MESSAGE_LENGTH, "You found a gold coin! (+1 gold)");
                        } else {
                            snprintf(msg, MAX_MESSAGE_LENGTH, "You found a rare black coin! (+5 gold)");
                        }
                        set_message(map, msg);
                    }
                }
                map->player_x = new_x;
                map->player_y = new_y;
            }
        }
    }
}
int main() {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    load_username();
    getmaxyx(stdscr, NUMLINES, NUMCOLS);
    NUMLINES -= 4;  // Adjusted for message area and stats only
    
    curs_set(0);
    
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_YELLOW, COLOR_BLACK);  // Player
        init_pair(2, COLOR_WHITE, COLOR_BLACK);   // Regular text
        init_pair(3, COLOR_RED, COLOR_BLACK);     // Traps/Doors
        init_pair(4, COLOR_GREEN, COLOR_BLACK);   // Floor
        init_pair(5, COLOR_CYAN, COLOR_BLACK);    // Borders/Stairs
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);  //MEGENTA
        //init_pair(7, COLOR_WHITE | A_BOLD, COLOR_BLACK); 
        init_pair(8, COLOR_YELLOW, COLOR_BLACK);
    }
    
    srand(time(NULL));
    
    Map *map = create_map();
    if (map == NULL) {
        endwin();
        fprintf(stderr, "Failed to create map\n");
        return 1;
    }
    generate_map(map);
    while (1) {
        clear();
        update_visibility(map);
        Level *current = &map->levels[map->current_level - 1];
        attron(COLOR_PAIR(5));
        mvprintw(0, 0, "╔");
        for (int x = 1; x < NUMCOLS - 1; x++) {
            mvprintw(0, x, "═");
        }
        mvprintw(0, NUMCOLS - 1, "╗");
        attroff(COLOR_PAIR(5));
        attron(COLOR_PAIR(3));
        mvprintw(1, 1, "╔");
        for (int x = 2; x < NUMCOLS - 2; x++) {
            mvprintw(1, x, "═");
        }
        mvprintw(1, NUMCOLS - 2, "╗");
        mvprintw(2, 1, "║");
        mvprintw(2, NUMCOLS - 2, "║");
        mvprintw(3, 1, "╚");
        for (int x = 2; x < NUMCOLS - 2; x++) {
            mvprintw(3, x, "═");
        }
        mvprintw(3, NUMCOLS - 2, "╝");
        attroff(COLOR_PAIR(3));
        attron(COLOR_PAIR(5));
        for (int y = 1; y < NUMLINES; y++) {
            mvprintw(y, 0, "║");
            mvprintw(y, NUMCOLS - 1, "║");
        }
        mvprintw(NUMLINES, 0, "╚");
        for (int x = 1; x < NUMCOLS - 1; x++) {
            mvprintw(NUMLINES, x, "═");
        }
        mvprintw(NUMLINES, NUMCOLS - 1, "╝");
        mvprintw(NUMLINES + 1, 0, "╔");
        for (int x = 1; x < NUMCOLS - 1; x++) {
            mvprintw(NUMLINES + 1, x, "═");
        }
        mvprintw(NUMLINES + 1, NUMCOLS - 1, "╗");
        mvprintw(NUMLINES + 2, 0, "║");
        mvprintw(NUMLINES + 2, NUMCOLS - 1, "║");
        mvprintw(NUMLINES + 3, 0, "╚");
        for (int x = 1; x < NUMCOLS - 1; x++) {
            mvprintw(NUMLINES + 3, x, "═");
        }
        mvprintw(NUMLINES + 3, NUMCOLS - 1, "╝");
        attroff(COLOR_PAIR(5));
        for (int y = 0; y < NUMLINES - 4; y++) {
            for (int x = 0; x < NUMCOLS - 2; x++) {
                if (map->debug_mode || current->visible_tiles[y][x] != ' ') {
                    if ((map->debug_mode && current->traps[y][x]) || 
                        (current->discovered_traps[y][x] && current->explored[y][x])) {
                        attron(COLOR_PAIR(3));
                        mvaddch(y + 4, x + 1, '^');
                        attroff(COLOR_PAIR(3));
                    } else {
                        int color = 2;
                        switch(current->visible_tiles[y][x]) {
                            case '>': case '<': color = 5; break;
                            case '+': color = 3; break;
                            case '|': case '_': color = 1; break;
                            case '.': color = 4; break;
                            case '#': color = 2; break;
                            case '*': 
                                color = 5;  // Magenta color for food
                                attron(COLOR_PAIR(color));
                                mvprintw(y + 4, x + 1, "●");
                                attroff(COLOR_PAIR(color));
                                continue;
                            case '$': 
                                color = 1;  // Yellow for gold coins
                                attron(COLOR_PAIR(color));
                                mvprintw(y + 4, x + 1, "▲");
                                attroff(COLOR_PAIR(color));
                                continue;
                            case '&': 
                                color = 6;  // White/bold for black coins
                                attron(COLOR_PAIR(color));
                                mvprintw(y + 4, x + 1, "△");
                                attroff(COLOR_PAIR(color));
                                continue;
                            case 'T': 
                                {
                                    int color;
                                    switch(current->talisman_type) {
                                        case TALISMAN_HEALTH:
                                            color = COLOR_PAIR(4);  // Green for health
                                            break;
                                        case TALISMAN_DAMAGE:
                                            color = COLOR_PAIR(3);  // Red for damage
                                            break;
                                        case TALISMAN_SPEED:
                                            color = COLOR_PAIR(5);  // Cyan for speed
                                            break;
                                        default:
                                            color = COLOR_PAIR(2);
                                            break;
                                    }
                                    attron(color);
                                    mvprintw(y + 4, x + 1, "◆");
                                    attroff(color);
                                }
                                continue;
                        }
                        attron(COLOR_PAIR(color));
                        mvaddch(y + 4, x + 1, current->visible_tiles[y][x]);
                        attroff(COLOR_PAIR(color));
                        attron(COLOR_PAIR(3)); 
                        if (current->secret_stairs[y][x] && (map->debug_mode || current->explored[y][x])) {
                            attron(COLOR_PAIR(3));
                            mvaddch(y + 4, x + 1, '%');
                            attroff(COLOR_PAIR(3));
                        }
                        attroff(COLOR_PAIR(3));
                    }
                }
            }
        }
        attron(COLOR_PAIR(map->character_color));
        mvaddch(map->player_y + 4, map->player_x + 1, '@');
        attroff(COLOR_PAIR(map->character_color));
        if (map->message_timer > 0) {
            attron(COLOR_PAIR(2));
            mvprintw(2, 2, "%s", map->current_message);
            attroff(COLOR_PAIR(2));
            map->message_timer--;
        }
        attron(COLOR_PAIR(2));
            attron(COLOR_PAIR(2));
            mvprintw(2, NUMCOLS/2 - 15, "Current User's Login: ");
            attron(COLOR_PAIR(4));
            printw("%s", current_username);
            attroff(COLOR_PAIR(4));

            // Draw stats with different colors
            attron(COLOR_PAIR(2));
            mvprintw(NUMLINES + 2, 2, "Level: ");
            attron(COLOR_PAIR(4));  // Green for level
            printw("%d", map->current_level);
            attroff(COLOR_PAIR(4));

            attron(COLOR_PAIR(2));
            printw("  Health: ");
            attron(COLOR_PAIR(3));  // Red for health
            printw("%d", map->health);
            attroff(COLOR_PAIR(3));

            attron(COLOR_PAIR(2));
            printw("  Str: ");
            attron(COLOR_PAIR(3));  // Red for strength
            printw("%d", map->strength);
            attroff(COLOR_PAIR(3));

            attron(COLOR_PAIR(2));
            printw("  Gold: ");
            attron(COLOR_PAIR(1));  // Yellow/Gold for gold
            printw("%d", map->gold);
            attroff(COLOR_PAIR(1));

            attron(COLOR_PAIR(2));
            printw("  Armor: ");
            attron(COLOR_PAIR(5));  // Cyan for armor
            printw("%d", map->armor);
            attroff(COLOR_PAIR(5));

            attron(COLOR_PAIR(2));
            printw("  Exp: ");
            attron(COLOR_PAIR(4));  // Green for exp
            printw("%d", map->exp);
            attroff(COLOR_PAIR(4));
            attroff(COLOR_PAIR(2));
        attroff(COLOR_PAIR(2));
        refresh();
        int ch = getch();
        if (++map->hunger_timer >= 100) {  // Adjust timing as needed
            map->hunger_timer = 0;
            if (map->hunger > 0) {
                map->hunger--;
            }
            // Damage player if starving
            if (map->hunger <= 20 && map->health > 0) {
                map->health--;
                set_message(map, "You are starving!");
            }
        }
        if (ch == 'q' || ch == 'Q'){
            save_user_data(map);
            free_map(map);
            clear();
            refresh();
            endwin();
            curs_set(1);
            system("clear");
            exit(0);
        }
        if (ch == 'r' || ch == 'R') {
            generate_map(map);
        } else if (ch == 'm' || ch == 'M') {
            map->debug_mode = !map->debug_mode;
            set_message(map, map->debug_mode ? "Debug mode activated." : "Debug mode deactivated.");
        } else {
            handle_input(map, ch);
        }
        if (map->health <= 0) {
            set_message(map, "Game Over! You died!");
            refresh();
            napms(2000);
            break;
        }
    }
    
    free_map(map);
    endwin();
    return 0;
}