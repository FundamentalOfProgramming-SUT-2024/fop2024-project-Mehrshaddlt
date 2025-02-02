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
#define MAXROOMS 9
#define MIN_ROOM_SIZE 6
#define MAX_ROOM_SIZE 10
#define MAX_MESSAGE_LENGTH 8000
#define MESSAGE_DURATION 5
#define MAX_LEVELS 5 
#define TREASURE_LEVEL 5

int NUMCOLS;
int NUMLINES;
char current_username[50] = "";
static bool fast_travel_mode = false;

typedef struct {
    int x, y;
} Coord;
typedef enum {
    MONSTER_DEMON,
    MONSTER_FIRE,
    MONSTER_GIANT,
    MONSTER_SNAKE,
    MONSTER_UNDEAD,
    MONSTER_COUNT
} MonsterType;
typedef struct {
    MonsterType type;
    const char* name;
    char symbol;
    int health;
    int max_health;
    int damage;
    int x, y;
    bool active;
    bool aggressive; 
    bool was_attacked;
    bool immobilized;
} Monster;
typedef struct {
    Coord pos;     
    Coord max;   
    Coord center; 
    bool gone; 
    bool connected; 
    int doors[4]; 
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
    int ammo; 
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
    int count;                
    int active_durations[5];   
} Talisman;
typedef struct {
    Room rooms[MAXROOMS];
    Room secret_rooms[MAXROOMS];    
    int num_rooms;
    int num_secret_rooms;          
    Room* stair_room;           
    int stair_x, stair_y;         
    char **tiles;                 
    char **visible_tiles;         
    bool **explored;             
    bool **traps;                    
    bool **discovered_traps;
    bool **secret_walls;         
    Coord stairs_up;                  
    Coord stairs_down;
    Coord secret_entrance;          
    Room *current_secret_room;      
    bool stairs_placed;
    char **backup_tiles;
    char **backup_visible_tiles; 
    bool **backup_explored;
    bool **secret_stairs;  
    Room *secret_stair_room; 
    Coord secret_stair_entrance; 
    bool **coins;     
    int **coin_values;
    int talisman_type;
    Monster monsters[MONSTER_COUNT];
    int num_monsters;
} Level;

typedef struct {
    Level *levels;        
    int current_level;   
    int player_x, player_y;
    int prev_stair_x;  
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
    int food_count;     
    int hunger;          
    int hunger_timer;   
    bool food_menu_open;
    Weapon weapons[5];
    WeaponType current_weapon;
    bool weapon_menu_open;
    Talisman talismans[3];
    bool talisman_menu_open;
    bool health_regen_doubled;
    bool damage_doubled;
    bool speed_doubled;
    int active_talisman_count;
} Map;

// Function declarations
void update_monsters(Map *map);
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
void show_lose_screen(Map* map);
bool is_in_same_room(Level *level, int x1, int y1, int x2, int y2);
bool is_monster_at(Level *level, int x, int y);
void throw_dagger(Map *map, int dir_x, int dir_y);
void draw_weapon_symbol(WINDOW *win, int y, int x, char symbol);
void cast_magic_wand(Map *map, int dir_x, int dir_y);
void shoot_arrow(Map *map, int dir_x, int dir_y);
int calculate_damage(Map *map, int base_damage);
void display_talisman_menu(WINDOW *win, Map *map);
// Function delarations
void display_talisman_menu(WINDOW *win, Map *map) {
    int center_y = NUMLINES / 2;
    int center_x = NUMCOLS / 2;
    int box_width = 40;
    int box_height = 12;
    int start_y = center_y - (box_height / 2);
    int start_x = center_x - (box_width / 2);
    WINDOW *menu_win = newwin(box_height, box_width, start_y, start_x);
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
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, 1, 2, "Talisman Collection");
    mvwprintw(menu_win, 2, 2, "Press 1-3 to use a talisman");
    for (int i = 0; i < TALISMAN_COUNT; i++) {
        wattron(menu_win, map->talismans[i].color);
        mvwprintw(menu_win, 4 + i * 2, 2, "%d: %s %s [%d owned]", 
                  i + 1,
                  map->talismans[i].symbol,
                  map->talismans[i].name,
                  map->talismans[i].count);
        int active_count = 0;
        for (int j = 0; j < 5; j++) {
            if (map->talismans[i].active_durations[j] > 0) {
                active_count++;
            }
        }
        if (active_count > 0) {
            mvwprintw(menu_win, 4 + i * 2 + 1, 4, "Active: %d talisman(s) - Effects:", active_count);
            switch(i) {
                case TALISMAN_HEALTH:
                    wprintw(menu_win, " Double HP regen");
                    break;
                case TALISMAN_DAMAGE:
                    wprintw(menu_win, " Double damage");
                    break;
                case TALISMAN_SPEED:
                    wprintw(menu_win, " Double movement");
                    break;
            }
        }
        wattroff(menu_win, map->talismans[i].color);
    }
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, box_height - 2, 2, "Press any key to close");
    wattroff(menu_win, COLOR_PAIR(2));
    wrefresh(menu_win);
    int ch = getch();
    if (ch >= '1' && ch <= '3') {
        int talisman_type = ch - '1';
        if (map->talismans[talisman_type].owned && map->talismans[talisman_type].count > 0) {
            for (int i = 0; i < 5; i++) {
                if (map->talismans[talisman_type].active_durations[i] <= 0) {
                    map->talismans[talisman_type].active_durations[i] = 10;
                    map->talismans[talisman_type].count--;
                    
                    char msg[MAX_MESSAGE_LENGTH];
                    snprintf(msg, MAX_MESSAGE_LENGTH, "Activated %s! (%d remaining)", 
                            map->talismans[talisman_type].name, 
                            map->talismans[talisman_type].count);
                    set_message(map, msg);
                    break;
                }
            }
        }
    }
    werase(menu_win);
    wrefresh(menu_win);
    delwin(menu_win);
    redrawwin(stdscr);
    refresh();
}
int calculate_damage(Map *map, int base_damage) {
    return map->damage_doubled ? base_damage * 2 : base_damage;
}
void cast_magic_wand(Map *map, int dir_x, int dir_y) {
    Level *current = &map->levels[map->current_level - 1];
    bool spell_hit = false;
    if (map->weapons[WEAPON_WAND].ammo <= 0) {
        set_message(map, "No magic charges left!");
        return;
    }
    char spell_symbol;
    if (dir_x == 0) spell_symbol = '|';
    else if (dir_y == 0) spell_symbol = '-';
    else spell_symbol = '*';
    int current_x = map->player_x;
    int current_y = map->player_y;
    for (int dist = 1; dist <= 5 && !spell_hit; dist++) {
        int new_x = map->player_x + (dir_x * dist);
        int new_y = map->player_y + (dir_y * dist);
        if (new_x < 0 || new_x >= NUMCOLS || new_y < 0 || new_y >= NUMLINES) break;
        if (current->tiles[new_y][new_x] == '|' || 
            current->tiles[new_y][new_x] == '_' || 
            current->tiles[new_y][new_x] == ' ') {
            break;
        }
        if (current_x != map->player_x || current_y != map->player_y) {
            current->visible_tiles[current_y][current_x] = current->tiles[current_y][current_x];
        }
        for (int i = 0; i < MONSTER_COUNT; i++) {
            Monster *monster = &current->monsters[i];
            if (monster->active && monster->x == new_x && monster->y == new_y) {
                current->visible_tiles[new_y][new_x] = '*';
                refresh();
                napms(100);
                int damage = 15;
                monster->health -= damage;
                monster->was_attacked = true;
                monster->aggressive = true;
                monster->immobilized = true;
                if (monster->health <= 0) {
                    monster->active = false;
                    current->tiles[new_y][new_x] = '.';
                    set_message(map, "Your magic spell defeated the monster!");
                } 
                else {
                    char msg[MAX_MESSAGE_LENGTH];
                    snprintf(msg, MAX_MESSAGE_LENGTH, 
                            "Your spell hits the %s for %d damage and freezes it in place! (Monster HP: %d/%d)", 
                            monster->name, damage, monster->health, monster->max_health);
                    set_message(map, msg);
                }
                spell_hit = true;
                break;
            }
        }
        if (!spell_hit) {
            current->visible_tiles[new_y][new_x] = spell_symbol;
            refresh();
            napms(50);
            current_x = new_x;
            current_y = new_y;
        }
    }
    map->weapons[WEAPON_WAND].ammo--;
    refresh();
}

void shoot_arrow(Map *map, int dir_x, int dir_y) {
    Level *current = &map->levels[map->current_level - 1];
    bool arrow_hit = false;
    if (map->weapons[WEAPON_ARROW].ammo <= 0) {
        set_message(map, "No arrows left!");
        return;
    }
    char arrow_symbol;
    if (dir_x == 0 && dir_y == -1) arrow_symbol = '^';
    else if (dir_x == 0 && dir_y == 1) arrow_symbol = '^';
    else if (dir_x == -1 && dir_y == 0) arrow_symbol = '<';
    else if (dir_x == 1 && dir_y == 0) arrow_symbol = '>';
    else return;
    int current_x = map->player_x;
    int current_y = map->player_y;
    for (int dist = 1; dist <= 5 && !arrow_hit; dist++) {
        int new_x = map->player_x + (dir_x * dist);
        int new_y = map->player_y + (dir_y * dist);
        
        if (new_x < 0 || new_x >= NUMCOLS || new_y < 0 || new_y >= NUMLINES) {
            break;
        }
        if (current->tiles[new_y][new_x] == '|' || 
            current->tiles[new_y][new_x] == '_' || 
            current->tiles[new_y][new_x] == ' ') {
            arrow_hit = true;
            if (dist > 1) {
                new_x = map->player_x + (dir_x * (dist - 1));
                new_y = map->player_y + (dir_y * (dist - 1));
                if (current->tiles[new_y][new_x] == '.') {
                    current->tiles[new_y][new_x] = 'a';
                }
            }
            break;
        }
        if (current_x != map->player_x || current_y != map->player_y) {
            current->visible_tiles[current_y][current_x] = current->tiles[current_y][current_x];
        }
        for (int i = 0; i < MONSTER_COUNT; i++) {
            Monster *monster = &current->monsters[i];
            if (monster->active && monster->x == new_x && monster->y == new_y) {
                current->visible_tiles[new_y][new_x] = 'x';
                refresh();
                napms(100);
                int damage = 5;
                monster->health -= damage;
                monster->was_attacked = true;
                monster->aggressive = true;
                if (monster->health <= 0) {
                    monster->active = false;
                    current->tiles[new_y][new_x] = 'a';
                    set_message(map, "Your arrow defeated the monster!");
                } else {
                    char msg[MAX_MESSAGE_LENGTH];
                    snprintf(msg, MAX_MESSAGE_LENGTH, 
                            "Your arrow hits the %s for %d damage! (Monster HP: %d/%d)", 
                            monster->name, damage, monster->health, monster->max_health);
                    set_message(map, msg);
                    if (current->tiles[new_y][new_x] == '.') {
                        current->tiles[new_y][new_x] = 'a';
                    }
                }
                arrow_hit = true;
                break;
            }
        }
        if (!arrow_hit) {
            current->visible_tiles[new_y][new_x] = arrow_symbol;
            refresh();
            napms(50);
            current_x = new_x;
            current_y = new_y;
            if (dist == 5) {
                if (current->tiles[new_y][new_x] == '.') {
                    current->tiles[new_y][new_x] = 'a';
                }
            }
        }
    }
    map->weapons[WEAPON_ARROW].ammo--;
    refresh();
}
void load_user_stats(Map* map) {
    if (strcmp(current_username, "") == 0 || strcmp(current_username, "Guest") == 0) {
        map->games_played = 0;
        map->exp = 0;
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
                for (int i = 0; i < 6; i++) {
                    fgets(line, sizeof(line), file);
                    if (i == 5) {
                        sscanf(line, "Exp: %d", &map->exp);
                    }
                }
                fgets(line, sizeof(line), file);
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
void show_lose_screen(Map* map) {
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
    mvprintw(start_y + 1, center_x - 4, "YOU LOST!");
    attroff(COLOR_PAIR(1) | A_BOLD);
    attron(COLOR_PAIR(4));
    mvprintw(start_y + 3, center_x - 9, "Username : %s", current_username);
    mvprintw(start_y + 4, center_x - 11, "Final Level Reached: %d", map->current_level);
    mvprintw(start_y + 5, center_x - 11, "Experience so far: %d", map->exp);
    attroff(COLOR_PAIR(4));
    attron(COLOR_PAIR(2));
    mvprintw(start_y + 6, center_x - 10, "Press any key to exit");
    attroff(COLOR_PAIR(2));
    refresh();
    getch();
}
void throw_dagger(Map *map, int dir_x, int dir_y) {
    Level *current = &map->levels[map->current_level - 1];
    bool dagger_stopped = false;
    if (map->weapons[WEAPON_DAGGER].ammo <= 0) {
        set_message(map, "No daggers left!");
        return;
    }
    char dagger_direction;
    if (dir_x == 0 && dir_y == -1) dagger_direction = '|';   
    else if (dir_x == 0 && dir_y == 1) dagger_direction = '|';  
    else if (dir_x == -1 && dir_y == 0) dagger_direction = '-'; 
    else if (dir_x == 1 && dir_y == 0) dagger_direction = '-'; 
    else return;
    int current_x = map->player_x;
    int current_y = map->player_y;
    for (int dist = 1; dist <= 5 && !dagger_stopped; dist++) {
        int new_x = map->player_x + (dir_x * dist);
        int new_y = map->player_y + (dir_y * dist);
        if (new_x < 0 || new_x >= NUMCOLS || new_y < 0 || new_y >= NUMLINES) {
            break;
        }
        if (current->tiles[new_y][new_x] == '|' || 
            current->tiles[new_y][new_x] == '_' || 
            current->tiles[new_y][new_x] == ' ') {
            dagger_stopped = true;
            if (dist > 1) {
                new_x = map->player_x + (dir_x * (dist - 1));
                new_y = map->player_y + (dir_y * (dist - 1));
                if (current->tiles[new_y][new_x] == '.') {
                    current->tiles[new_y][new_x] = 'd';
                }
            }
            break;
        }
        if (current_x != map->player_x || current_y != map->player_y) {
            current->visible_tiles[current_y][current_x] = current->tiles[current_y][current_x];
        }
        bool hit_monster = false;
        for (int i = 0; i < MONSTER_COUNT; i++) {
            Monster *monster = &current->monsters[i];
            if (monster->active && monster->x == new_x && monster->y == new_y) {
                current->visible_tiles[new_y][new_x] = '*';
                refresh();
                napms(100);
                int damage = 12;
                monster->health -= damage;
                monster->was_attacked = true;
                monster->aggressive = true;
                
                if (monster->health <= 0) {
                    monster->active = false;
                    current->tiles[new_y][new_x] = 'd';
                    set_message(map, "Your thrown dagger defeated the monster!");
                } else {
                    char msg[MAX_MESSAGE_LENGTH];
                    snprintf(msg, MAX_MESSAGE_LENGTH, 
                            "Your thrown dagger hits the %s for %d damage! (Monster HP: %d/%d)", 
                            monster->name, damage, monster->health, monster->max_health);
                    set_message(map, msg);
                    if (current->tiles[new_y][new_x] == '.') {
                        current->tiles[new_y][new_x] = 'd';
                    }
                }
                hit_monster = true;
                dagger_stopped = true;
                break;
            }
        }
        if (!hit_monster && !dagger_stopped) {
            current->visible_tiles[new_y][new_x] = dagger_direction;
            refresh();
            napms(50);
            current_x = new_x;
            current_y = new_y;
            if (dist == 5) {
                if (current->tiles[new_y][new_x] == '.') {
                    current->tiles[new_y][new_x] = 'd';
                }
            }
        }
    }
    map->weapons[WEAPON_DAGGER].ammo--;
    refresh();
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
        return;
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
    char current_block[2048] = "";
    char other_blocks[10240] = "";
    bool copying_current_user = false;
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Username:", 9) == 0) {
            char username[50];
            sscanf(line, "Username: %s", username);
            if (strcmp(username, current_username) == 0) {
                found = true;
                copying_current_user = true;
                for (int i = 0; i < 7; i++) {
                    if (!fgets(line, sizeof(line), file)) break;
                }
            } 
            else {
                if (copying_current_user) {
                    copying_current_user = false;
                }
                strcat(other_blocks, line);
                continue;
            }
        } else if (!copying_current_user) {
            strcat(other_blocks, line);
        }
    }
    fprintf(temp, "Username: %s\n", current_username);
    fprintf(temp, "Level: %d\n", map->current_level);
    fprintf(temp, "Hit: %d\n", map->health);
    fprintf(temp, "Strength: %d\n", map->strength);
    fprintf(temp, "Gold: %d\n", map->gold);
    fprintf(temp, "Armor: %d\n", map->armor);
    fprintf(temp, "Exp: %d\n", map->exp + map->gold);
    fprintf(temp, "Games Played: %d\n\n", map->games_played + 1);
    fprintf(temp, "%s", other_blocks);
    if (!found) {
        fprintf(temp, "Username: %s\n", current_username);
        fprintf(temp, "Level: %d\n", map->current_level);
        fprintf(temp, "Hit: %d\n", map->health);
        fprintf(temp, "Strength: %d\n", map->strength);
        fprintf(temp, "Gold: %d\n", map->gold);
        fprintf(temp, "Armor: %d\n", map->armor);
        fprintf(temp, "Exp: %d\n", map->gold);
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
    if (rand() % 10 == 0) {
        int attempts = 0;
        const int MAX_ATTEMPTS = 10;  
        while (attempts < MAX_ATTEMPTS) {
            int stair_x = room->pos.x + 2 + rand() % (room->max.x - 4);
            int stair_y = room->pos.y + 2 + rand() % (room->max.y - 4);
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
    for (int y = 0; y < NUMLINES; y++) {
        for (int x = 0; x < NUMCOLS; x++) {
            level->tiles[y][x] = ' ';
            level->visible_tiles[y][x] = ' ';
            level->explored[y][x] = false;
        }
    }
    int room_size = 7;
    int start_x = NUMCOLS/2 - room_size/2;
    int start_y = NUMLINES/2 - room_size/2;
    for (int y = 0; y < room_size; y++) {
        for (int x = 0; x < room_size; x++) {
            if (y == 0 || y == room_size-1) {
                level->tiles[start_y + y][start_x + x] = '_';
            } else if (x == 0 || x == room_size-1) {
                level->tiles[start_y + y][start_x + x] = '|';
            } else {
                level->tiles[start_y + y][start_x + x] = '.';
            }
            level->visible_tiles[start_y + y][start_x + x] = level->tiles[start_y + y][start_x + x];
            level->explored[start_y + y][start_x + x] = true;
        }
    }
    level->tiles[start_y + room_size/2][start_x + room_size/2] = '?';
    level->visible_tiles[start_y + room_size/2][start_x + room_size/2] = '?';
    int talisman_y = start_y + 1 + (rand() % (room_size - 2));
    int talisman_x = start_x + 1 + (rand() % (room_size - 2));
    while (talisman_x == start_x + room_size/2 && talisman_y == start_y + room_size/2) {
        talisman_y = start_y + 1 + (rand() % (room_size - 2));
        talisman_x = start_x + 1 + (rand() % (room_size - 2));
    }
    level->tiles[talisman_y][talisman_x] = 'T';
    level->visible_tiles[talisman_y][talisman_x] = 'T';
    level->talisman_type = rand() % TALISMAN_COUNT;
}
void add_secret_walls_to_room(Level *level, Room *room) {
    int door_count = 0;
    for (int y = room->pos.y; y < room->pos.y + room->max.y; y++) {
        for (int x = room->pos.x; x < room->pos.x + room->max.x; x++) {
            if (level->tiles[y][x] == '+') {
                door_count++;
            }
        }
    }
    if (door_count == 1) {
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
        if (wall_count > 0) {
            int idx = rand() % wall_count;
            level->secret_walls[wall_y[idx]][wall_x[idx]] = true;
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

bool is_valid_secret_wall(Level *level, int x, int y) {
    // Must be a wall tile
    if (level->tiles[y][x] != '|' && level->tiles[y][x] != '_') {
        return false;
    }
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
        const int MAX_ATTEMPTS = 100;
        while (!valid_position && attempts < MAX_ATTEMPTS) {
            x = room->pos.x + 2 + rand() % (room->max.x - 4);
            y = room->pos.y + 2 + rand() % (room->max.y - 4);
            bool near_door = false;
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
        if (!valid_position) {
            for (int i = 0; i < level->num_rooms; i++) {
                Room *alt_room = &level->rooms[i];
                if (alt_room != room) { 
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
                if (valid_position) break;
            }
        }
    } 
    else {
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
            level->coins[i] = malloc(NUMCOLS * sizeof(bool));
            level->coin_values[i] = malloc(NUMCOLS * sizeof(int));
            
            if (!level->coins[i] || !level->coin_values[i]) {
                free_map(map);
                return NULL;
            }
            for (int j = 0; j < NUMCOLS; j++) {
                level->coins[i][j] = false;
                level->coin_values[i][j] = 0;
            }
        }
        for (int l = 0; l < MAX_LEVELS; l++) {
            Level *level = &map->levels[l];
            level->num_monsters = MONSTER_COUNT;
            level->monsters[MONSTER_DEMON] = (Monster){MONSTER_DEMON, "Demon", 'D', 5, 5, 1, 0, 0, false, false, false, false};
            level->monsters[MONSTER_FIRE] = (Monster){MONSTER_FIRE, "Fire Breather", 'F', 10, 10, 2, 0, 0, false, false, false, false};
            level->monsters[MONSTER_GIANT] = (Monster){MONSTER_GIANT, "Giant", 'G', 15, 15, 3, 0, 0, false, false, false , false};
            level->monsters[MONSTER_SNAKE] = (Monster){MONSTER_SNAKE, "Snake", 'S', 20, 20, 4, 0, 0, false, false, false, false};
            level->monsters[MONSTER_UNDEAD] = (Monster){MONSTER_UNDEAD, "Undead", 'U', 30, 30, 5, 0, 0, false, true, false, false};
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
                level->coins[i][j] = false;       
                level->coin_values[i][j] = 0;    
            }
        }
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
    map->health = 25;
    map->strength = 16;
    map->gold = 0;
    map->armor = 0;
    map->exp = 0;
    map->is_treasure_room = false;
    map->prev_stair_x = -1;
    map->prev_stair_y = -1;
    map->weapons[WEAPON_MACE] = (Weapon){WEAPON_MACE, "Mace", "⚒", true, -1}; 
    map->weapons[WEAPON_DAGGER] = (Weapon){WEAPON_DAGGER, "Dagger", "🗡", false, 12};  
    map->weapons[WEAPON_WAND] = (Weapon){WEAPON_WAND, "Magic Wand", "⚚", false, 8}; 
    map->weapons[WEAPON_ARROW] = (Weapon){WEAPON_ARROW, "Normal Arrow", "➳", false, 20}; 
    map->weapons[WEAPON_SWORD] = (Weapon){WEAPON_SWORD, "Sword", "⚔", false, -1}; 
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

void display_weapon_menu(WINDOW *win, Map *map) {
    int center_y = NUMLINES / 2;
    int center_x = NUMCOLS / 2;
    int box_width = 40;
    int box_height = 12;
    int start_y = center_y - (box_height / 2);
    int start_x = center_x - (box_width / 2);
    WINDOW *menu_win = newwin(box_height, box_width, start_y, start_x);
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
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, 1, 2, "Weapon Inventory");
    mvwprintw(menu_win, 2, 2, "Current Weapon: %s %s", 
              map->weapons[map->current_weapon].name,
              map->weapons[map->current_weapon].symbol);
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, 1, 2, "Weapon Inventory");
    mvwprintw(menu_win, 2, 2, "Current Weapon: %s %s", 
              map->weapons[map->current_weapon].name,
              map->weapons[map->current_weapon].symbol);
    for (int i = 0; i < WEAPON_COUNT; i++) {
        wattron(menu_win, COLOR_PAIR(6));
        mvwprintw(menu_win, 4 + i, 2, "%d. %s ", 
                i + 1, 
                map->weapons[i].symbol);
        wattroff(menu_win, COLOR_PAIR(6));
        wattron(menu_win, map->weapons[i].owned ? COLOR_PAIR(4) : COLOR_PAIR(3));
        if (map->weapons[i].owned) {
            switch(i) {
                case WEAPON_DAGGER:
                    wprintw(menu_win, "%s [%d daggers]", 
                        map->weapons[i].name, map->weapons[i].ammo);
                    break;
                case WEAPON_WAND:
                    wprintw(menu_win, "%s [%d charges]", 
                        map->weapons[i].name, map->weapons[i].ammo);
                    break;
                case WEAPON_ARROW:
                    wprintw(menu_win, "%s [%d arrows]", 
                        map->weapons[i].name, map->weapons[i].ammo);
                    break;
                default:
                    wprintw(menu_win, "%s %s", 
                        map->weapons[i].name,
                        (i == WEAPON_MACE || i == WEAPON_SWORD) ? "[MELEE]" : "[OWNED]");
            }
        } 
        else {
            wprintw(menu_win, "%s [NOT FOUND]", map->weapons[i].name);
        }
    }
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, box_height - 3, 2, "Press 1-5 to equip weapon");
    mvwprintw(menu_win, box_height - 2, 2, "Press any other key to close");
    wattroff(menu_win, COLOR_PAIR(2)); 
    wrefresh(menu_win);
    int ch = getch();
    if (ch >= '1' && ch <= '5') {
        int weapon_index = ch - '1';
        if (map->weapons[weapon_index].owned) {
            map->current_weapon = weapon_index;
            char msg[MAX_MESSAGE_LENGTH];
            snprintf(msg, MAX_MESSAGE_LENGTH, "Equipped %s %s", 
                    map->weapons[weapon_index].name,
                    map->weapons[weapon_index].symbol);
            set_message(map, msg);
        }
        else {
            set_message(map, "You don't have this weapon yet!");
        }
    }
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
    WINDOW *menu_win = newwin(box_height, box_width, start_y, start_x);
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
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, 1, 2, "Food Menu");
    mvwprintw(menu_win, 2, 2, "Hunger: ");
    wattron(menu_win, COLOR_PAIR(map->hunger > 30 ? 4 : 3));
    for (int i = 0; i < map->hunger / 5; i++) {
        mvwprintw(menu_win, 2, 10 + i, "█");
    }
    wattroff(menu_win, COLOR_PAIR(map->hunger > 30 ? 4 : 3));
    mvwprintw(menu_win, 4, 2, "Food Items: %d/5", map->food_count);
    mvwprintw(menu_win, 6, 2, "Press E to eat food");
    mvwprintw(menu_win, 7, 2, "Press any other key to close");
    wattroff(menu_win, COLOR_PAIR(2));
    wrefresh(menu_win);
    int ch = getch();
    werase(menu_win);
    wrefresh(menu_win);
    delwin(menu_win);
    redrawwin(stdscr);
    refresh();
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
        map->hunger = MIN(100, map->hunger + 30); 
        map->health = MIN(25, map->health + 5); 
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
    for (int x = room->pos.x + 1; x < room->pos.x + room->max.x - 1; x++) {
        level->tiles[room->pos.y][x] = '_';
        level->tiles[room->pos.y + room->max.y - 1][x] = '_';
    }
    for (int y = room->pos.y + 1; y < room->pos.y + room->max.y - 1; y++) {
        level->tiles[y][room->pos.x] = '|';
        level->tiles[y][room->pos.x + room->max.x - 1] = '|';
    }
    for (int y = room->pos.y + 1; y < room->pos.y + room->max.y - 1; y++) {
        for (int x = room->pos.x + 1; x < room->pos.x + room->max.x - 1; x++) {
            level->tiles[y][x] = '.';
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
    int num_traps = rand() % 2;
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
            int weapon_type = WEAPON_DAGGER + (rand() % (WEAPON_COUNT - 1));
            switch(weapon_type) {
                case WEAPON_DAGGER:
                    level->tiles[weapon_y][weapon_x] = 'd';
                    break;
                case WEAPON_WAND:
                    level->tiles[weapon_y][weapon_x] = 'm';
                    break;
                case WEAPON_ARROW:
                    level->tiles[weapon_y][weapon_x] = 'a';
                    break;
                case WEAPON_SWORD:
                    level->tiles[weapon_y][weapon_x] = 's';
                    break;
            }
        }
        int attempts = 0;
        while (attempts < 50) {
            int weapon2_x = room->pos.x + 1 + (rand() % (room->max.x - 2));
            int weapon2_y = room->pos.y + 1 + (rand() % (room->max.y - 2));
            int weapon_type = WEAPON_DAGGER + (rand() % (WEAPON_COUNT - 1));
            if (level->tiles[weapon2_y][weapon2_x] == '.' && 
                (weapon2_x != weapon_x || weapon2_y != weapon_y)) {
                int weapon2_type;
                do {
                    weapon2_type = WEAPON_DAGGER + (rand() % (WEAPON_COUNT - 1));
                } 
                while (weapon2_type == weapon_type);
                switch(weapon2_type) {
                    case WEAPON_DAGGER:
                        level->tiles[weapon2_y][weapon2_x] = 'd';
                        break;
                    case WEAPON_WAND:
                        level->tiles[weapon2_y][weapon2_x] = 'm';
                        break;
                    case WEAPON_ARROW:
                        level->tiles[weapon2_y][weapon2_x] = 'a';
                        break;
                    case WEAPON_SWORD:
                        level->tiles[weapon2_y][weapon2_x] = 's';
                        break;
                }
                break;
            }
            attempts++;
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
    for (int y = 0; y < NUMLINES; y++) {
        for (int x = 0; x < NUMCOLS; x++) {
            if (current->explored[y][x] || map->debug_mode) {
                current->visible_tiles[y][x] = current->tiles[y][x];
                if (map->debug_mode && current->secret_walls[y][x] && 
                    (current->tiles[y][x] == '|' || current->tiles[y][x] == '_')) {
                    current->visible_tiles[y][x] = '?';
                }
            } 
            else {
                current->visible_tiles[y][x] = ' ';
            }
        }
    }
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
    if (in_room && current_room != NULL) {
        for (int y = current_room->pos.y; y < current_room->pos.y + current_room->max.y; y++) {
            for (int x = current_room->pos.x; x < current_room->pos.x + current_room->max.x; x++) {
                current->explored[y][x] = true;
                current->visible_tiles[y][x] = current->tiles[y][x];
            }
        }
    } 
    else if (current->tiles[map->player_y][map->player_x] == '#' || 
             current->tiles[map->player_y][map->player_x] == '+') {
        current->explored[map->player_y][map->player_x] = true;
        bool horizontal_corridor = false;
        bool vertical_corridor = false;
        if ((map->player_x > 0 && (current->tiles[map->player_y][map->player_x-1] == '#' || 
                                  current->tiles[map->player_y][map->player_x-1] == '+')) ||
            (map->player_x < NUMCOLS-1 && (current->tiles[map->player_y][map->player_x+1] == '#' || 
                                         current->tiles[map->player_y][map->player_x+1] == '+'))) {
            horizontal_corridor = true;
        }
        if ((map->player_y > 0 && (current->tiles[map->player_y-1][map->player_x] == '#' || 
                                  current->tiles[map->player_y-1][map->player_x] == '+')) ||
            (map->player_y < NUMLINES-1 && (current->tiles[map->player_y+1][map->player_x] == '#' || 
                                          current->tiles[map->player_y+1][map->player_x] == '+'))) {
            vertical_corridor = true;
        }
        if (horizontal_corridor) {
            for (int dx = 0; dx >= -5; dx--) {
                int x = map->player_x + dx;
                if (x >= 0 && x < NUMCOLS) {
                    if (current->tiles[map->player_y][x] == '#' || 
                        current->tiles[map->player_y][x] == '+') {
                        current->explored[map->player_y][x] = true;
                    } else break;
                }
            }
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
            for (int dy = 0; dy >= -5; dy--) {
                int y = map->player_y + dy;
                if (y >= 0 && y < NUMLINES) {
                    if (current->tiles[y][map->player_x] == '#' || 
                        current->tiles[y][map->player_x] == '+') {
                        current->explored[y][map->player_x] = true;
                    } else break;
                }
            }
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
        for (int y = 0; y < NUMLINES; y++) {
            for (int x = 0; x < NUMCOLS; x++) {
                current->tiles[y][x] = ' ';
                current->visible_tiles[y][x] = ' ';
                current->explored[y][x] = false;
                current->traps[y][x] = false;
                current->discovered_traps[y][x] = false;
                current->secret_walls[y][x] = false;
                current->secret_stairs[y][x] = false;
                current->coins[y][x] = false;
                current->coin_values[y][x] = 0; 
            }
        }
        current->num_rooms = 0;
        current->num_secret_rooms = 0;
        current->current_secret_room = NULL;
        Room treasure_room;
        treasure_room.pos.x = NUMCOLS / 4;
        treasure_room.pos.y = NUMLINES / 4;
        treasure_room.max.x = NUMCOLS / 2;
        treasure_room.max.y = NUMLINES / 2;
        treasure_room.center.x = treasure_room.pos.x + treasure_room.max.x / 2;
        treasure_room.center.y = treasure_room.pos.y + treasure_room.max.y / 2;
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
                    if (rand() % 100 < 60) {
                        current->coins[y][x] = true;
                        current->coin_values[y][x] = 1;
                        current->tiles[y][x] = '$';
                    }
                    else if (rand() % 100 < 20) {
                        current->coins[y][x] = true;
                        current->coin_values[y][x] = 5;
                        current->tiles[y][x] = '&';
                    }
                }
            }
        }
        if (map->current_level < MAX_LEVELS && current->num_rooms > 1) {
            bool stairs_placed = false;
            int max_placement_attempts = 10;
            for (int attempts = 0; attempts < max_placement_attempts && !stairs_placed; attempts++) {
                for (int i = 1; i < current->num_rooms && !stairs_placed; i++) {
                    Room *room = &current->rooms[i];
                    add_stairs(current, room, true);
                    if (check_for_stairs(current)) {
                        current->stair_room = room;
                        stairs_placed = true;
                        break;
                    }
                }
            }
            if (!stairs_placed) {
                current->num_rooms = 0;
                for (int y = 0; y < NUMLINES; y++) {
                    for (int x = 0; x < NUMCOLS; x++) {
                        current->tiles[y][x] = ' ';
                    }
                }
                generate_map(map);
                return;
            }
        }
        Monster *undead = &current->monsters[MONSTER_UNDEAD];
        undead->active = true;
        undead->aggressive = true;
        undead->health = undead->max_health;
        undead->was_attacked = false;
        int tries = 0;
        do {
            undead->x = treasure_room.pos.x + 1 + (rand() % (treasure_room.max.x - 2));
            undead->y = treasure_room.pos.y + 1 + (rand() % (treasure_room.max.y - 2));
            tries++;
        } 
        while ((current->tiles[undead->y][undead->x] != '.' || 
                current->traps[undead->y][undead->x]) && 
                tries < 100);

        if (tries < 100) {
            current->tiles[undead->y][undead->x] = 'U';
        }
        for (int i = 0; i < MONSTER_COUNT; i++) {
            if (i != MONSTER_UNDEAD) {
                current->monsters[i].active = false;
            }
        }
        current->rooms[0] = treasure_room;
        current->num_rooms = 1;
        map->player_x = treasure_room.center.x;
        map->player_y = treasure_room.center.y;
        
        return;
    }
    current->num_rooms = 0;
    int attempts = 0;
    const int MAX_ATTEMPTS = 50;
    int target_rooms = 6 + (rand() % 4);
    for (int y = 0; y < NUMLINES; y++) {
        for (int x = 0; x < NUMCOLS; x++) {
            current->tiles[y][x] = ' ';
            current->visible_tiles[y][x] = ' ';
            current->explored[y][x] = false;
            current->traps[y][x] = false;
            current->discovered_traps[y][x] = false;
            current->secret_stairs[y][x] = false; 
            current->secret_walls[y][x] = false;
            current->coins[y][x] = false;    
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
        if (new_room.pos.y + new_room.max.y > NUMLINES - 6) { 
            attempts++;
            continue;  
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
    for (int i = 1; i < current->num_rooms; i++) {
        connect_rooms(current, &current->rooms[i-1], &current->rooms[i]);
    }
    for (int i = 0; i < current->num_rooms; i++) {
        add_secret_walls_to_room(current, &current->rooms[i]);
    }
    if (map->current_level < MAX_LEVELS && current->num_rooms > 1) {
        for (int i = 1; i < current->num_rooms; i++) {
            Room *room = &current->rooms[i];
            add_stairs(current, room, true);
            current->stair_room = room;
            break;
        }
    }
    if (map->current_level < 5) {
        Level *current = &map->levels[map->current_level - 1];
        for (int i = 0; i < MONSTER_COUNT; i++) {
            if (i + 1 >= current->num_rooms) break;
            Room *room = &current->rooms[i + 1];
            Monster *monster = &current->monsters[i];
            int tries = 0;
            do {
                monster->x = room->pos.x + 1 + (rand() % (room->max.x - 2));
                monster->y = room->pos.y + 1 + (rand() % (room->max.y - 2));
                tries++;
            } 
            while ((current->tiles[monster->y][monster->x] != '.' || 
                     current->traps[monster->y][monster->x]) && 
                    tries < 100);
            if (tries < 100) {
                monster->active = true;
                current->tiles[monster->y][monster->x] = monster->symbol;
            }
        }
    }
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
            Room *current_room = NULL;
            for (int i = 0; i < current->num_rooms; i++) {
                Room *room = &current->rooms[i];
                if (map->player_x >= room->pos.x && map->player_x < room->pos.x + room->max.x &&
                    map->player_y >= room->pos.y && map->player_y < room->pos.y + room->max.y) {
                    current_room = room;
                    break;
                }
            }
            map->current_level++;
            Level *next = &map->levels[map->current_level - 1]; 
            if (map->current_level == 5) {
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
                Room treasure_room;
                int center_x = NUMCOLS / 2;
                int center_y = NUMLINES / 2;
                int room_width = NUMCOLS / 4;
                int room_height = NUMLINES / 4; 
                treasure_room.pos.x = center_x - (room_width / 2);
                treasure_room.pos.y = center_y - (room_height / 2);
                treasure_room.max.x = room_width;
                treasure_room.max.y = room_height;
                treasure_room.center.x = treasure_room.pos.x + treasure_room.max.x / 2;
                treasure_room.center.y = treasure_room.pos.y + treasure_room.max.y / 2;
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
                for (int y = treasure_room.pos.y + 1; y < treasure_room.pos.y + treasure_room.max.y - 1; y++) {
                    for (int x = treasure_room.pos.x + 1; x < treasure_room.pos.x + treasure_room.max.x - 1; x++) {
                        if (next->tiles[y][x] == '.' && next->tiles[y][x] != '>') {
                            if (rand() % 100 < 5) {
                                next->coins[y][x] = true;
                                next->coin_values[y][x] = 1;
                                next->tiles[y][x] = '$';
                            }
                            else if (rand() % 100 < 3) {
                                next->coins[y][x] = true;
                                next->coin_values[y][x] = 5;
                                next->tiles[y][x] = '&';
                            }
                        }
                    }
                }
                int num_traps = 8 + rand() % 5;
                for (int i = 0; i < num_traps; i++) {
                    int trap_x = treasure_room.pos.x + 1 + rand() % (treasure_room.max.x - 2);
                    int trap_y = treasure_room.pos.y + 1 + rand() % (treasure_room.max.y - 2);
                    if (next->tiles[trap_y][trap_x] == '.' || 
                        next->tiles[trap_y][trap_x] == '$' || 
                        next->tiles[trap_y][trap_x] == '&' && next->tiles[trap_y][trap_x] != '>') {
                        next->traps[trap_y][trap_x] = true;
                    }
                }
                next->stairs_down.x = treasure_room.center.x;
                next->stairs_down.y = treasure_room.pos.y + 1;
                next->tiles[next->stairs_down.y][next->stairs_down.x] = '<';
                int victory_x = treasure_room.center.x;
                int victory_y = treasure_room.pos.y + treasure_room.max.y - 2;
                next->tiles[victory_y][victory_x] = '>';
                next->rooms[0] = treasure_room;
                next->num_rooms = 1;
                map->player_x = next->stairs_down.x;
                map->player_y = next->stairs_down.y;
                set_message(map, "You've reached the Treasure Room! Be careful of traps!");
                next->stairs_placed = true;
            }
            else if (!next->stairs_placed) {
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
                    next->rooms[0] = *current_room;
                    next->num_rooms = 1;
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
                    generate_remaining_rooms(map);
                    next->stairs_placed = true;
                }
            }
            map->player_x = next->stairs_down.x;
            map->player_y = next->stairs_down.y;
            
            char msg[MAX_MESSAGE_LENGTH];
            snprintf(msg, MAX_MESSAGE_LENGTH, "Ascending to level %d", map->current_level);
            set_message(map, msg);
        }
    }
    update_visibility(map);
}

void generate_remaining_rooms(Map *map) {
    Level *current = &map->levels[map->current_level - 1];
    int attempts = 0;
    const int MAX_ATTEMPTS = 50;
    bool weapon_placed = false; 
    while (current->num_rooms < MAXROOMS && attempts < MAX_ATTEMPTS) {
        Room new_room;
        new_room.max.x = MIN_ROOM_SIZE + rand() % (MAX_ROOM_SIZE - MIN_ROOM_SIZE + 1);
        new_room.max.y = MIN_ROOM_SIZE + rand() % (MAX_ROOM_SIZE - MIN_ROOM_SIZE + 1);
        new_room.pos.x = 1 + rand() % (NUMCOLS - new_room.max.x - 2);
        new_room.pos.y = 1 + rand() % (NUMLINES - new_room.max.y - 2);
        if (new_room.pos.y + new_room.max.y > NUMLINES - 6) {
            attempts++;
            continue;
        }
        new_room.center.x = new_room.pos.x + new_room.max.x / 2;
        new_room.center.y = new_room.pos.y + new_room.max.y / 2;
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
            if (current->num_rooms > 0) {
                connect_rooms(current, &current->rooms[current->num_rooms - 1], &new_room);
                if (!weapon_placed && current->num_rooms >= 2 && (rand() % 3 == 0)) {
                    int weapon_x = new_room.pos.x + 1 + (rand() % (new_room.max.x - 2));
                    int weapon_y = new_room.pos.y + 1 + (rand() % (new_room.max.y - 2));
                    if (current->tiles[weapon_y][weapon_x] == '.') {
                        int weapon_type = WEAPON_DAGGER + (rand() % (WEAPON_COUNT - 1));
                        switch(weapon_type) {
                            case WEAPON_DAGGER:
                                current->tiles[weapon_y][weapon_x] = 'd';
                                break;
                            case WEAPON_WAND:
                                current->tiles[weapon_y][weapon_x] = 'm';
                                break;
                            case WEAPON_ARROW:
                                current->tiles[weapon_y][weapon_x] = 'a';
                                break;
                            case WEAPON_SWORD:
                                current->tiles[weapon_y][weapon_x] = 's';
                                break;
                        }
                        weapon_placed = true;
                    }
                }
            }
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
    if (!weapon_placed && current->num_rooms > 1) {
        int room_index = 1 + (rand() % (current->num_rooms - 1));
        Room *random_room = &current->rooms[room_index];
        int tries = 0;
        while (tries < 50) {
            int weapon_x = random_room->pos.x + 1 + (rand() % (random_room->max.x - 2));
            int weapon_y = random_room->pos.y + 1 + (rand() % (random_room->max.y - 2));
            if (current->tiles[weapon_y][weapon_x] == '.') {
                int weapon_type = WEAPON_DAGGER + (rand() % (WEAPON_COUNT - 1));
                switch(weapon_type) {
                    case WEAPON_DAGGER:
                        current->tiles[weapon_y][weapon_x] = 'd';
                        break;
                    case WEAPON_WAND:
                        current->tiles[weapon_y][weapon_x] = 'm';
                        break;
                    case WEAPON_ARROW:
                        current->tiles[weapon_y][weapon_x] = 'a';
                        break;
                    case WEAPON_SWORD:
                        current->tiles[weapon_y][weapon_x] = 's';
                        break;
                }
                break;
            }
            tries++;
        }
    }
    if (map->current_level < 5) {
        Level *current = &map->levels[map->current_level - 1];
        for (int i = 0; i < MONSTER_COUNT; i++) {
            current->monsters[i].active = false;
            current->monsters[i].was_attacked = false;
            current->monsters[i].aggressive = (i == MONSTER_UNDEAD);
            current->monsters[i].health = current->monsters[i].max_health;
        }
        for (int i = 0; i < MONSTER_COUNT; i++) {
            if (i + 1 >= current->num_rooms) break; 
            Room *room = &current->rooms[i + 1];
            Monster *monster = &current->monsters[i];
            int tries = 0;
            do {
                monster->x = room->pos.x + 1 + (rand() % (room->max.x - 2));
                monster->y = room->pos.y + 1 + (rand() % (room->max.y - 2));
                tries++;
            } 
            while ((current->tiles[monster->y][monster->x] != '.' || 
                     current->traps[monster->y][monster->x]) && 
                    tries < 100);
            
            if (tries < 100) {
                monster->active = true;
                current->tiles[monster->y][monster->x] = monster->symbol;
            }
        }
    }
}

bool is_in_same_room(Level *level, int x1, int y1, int x2, int y2) {
    if ((level->tiles[y1][x1] != '.' && level->tiles[y1][x1] != '@') ||
        (level->tiles[y2][x2] != '.' && level->tiles[y2][x2] != '@')) {
        return false;
    }
    Room *room1 = NULL;
    for (int i = 0; i < level->num_rooms; i++) {
        Room *room = &level->rooms[i];
        if (x1 >= room->pos.x && x1 < room->pos.x + room->max.x &&
            y1 >= room->pos.y && y1 < room->pos.y + room->max.y) {
            room1 = room;
            break;
        }
    }
    if (!room1) return false;
    return (x2 >= room1->pos.x && x2 < room1->pos.x + room1->max.x &&
            y2 >= room1->pos.y && y2 < room1->pos.y + room1->max.y);
}

void update_monsters(Map *map) {
    Level *current = &map->levels[map->current_level - 1];
    for (int i = 0; i < MONSTER_COUNT; i++) {
        Monster *monster = &current->monsters[i];
        if (!monster->active) continue;
        if (monster->immobilized) {
            if (monster->aggressive && 
                abs(monster->x - map->player_x) <= 1 && 
                abs(monster->y - map->player_y) <= 1) {
                map->health -= monster->damage;
                char msg[MAX_MESSAGE_LENGTH];
                snprintf(msg, MAX_MESSAGE_LENGTH, 
                        "The frozen %s still manages to hit you for %d damage! (Your HP: %d)", 
                        monster->name, monster->damage, map->health);
                set_message(map, msg);
            }
            continue;
        }
        int orig_x = monster->x;
        int orig_y = monster->y;
        bool room_discovered = false;
        Room *monster_room = NULL;
        for (int i = 0; i < current->num_rooms; i++) {
            Room *room = &current->rooms[i];
            if (monster->x >= room->pos.x && monster->x < room->pos.x + room->max.x &&
                monster->y >= room->pos.y && monster->y < room->pos.y + room->max.y) {
                monster_room = room;
                for (int y = room->pos.y; y < room->pos.y + room->max.y; y++) {
                    for (int x = room->pos.x; x < room->pos.x + room->max.x; x++) {
                        if (current->explored[y][x]) {
                            room_discovered = true;
                            break;
                        }
                    }
                    if (room_discovered) break;
                }
                break;
            }
        }
        if (!room_discovered || monster_room == NULL) continue;
        bool should_follow = false;
        int dx = 0, dy = 0;
        if (monster->type == MONSTER_UNDEAD || monster->type == MONSTER_SNAKE) {
            should_follow = true;
            if (monster->x < map->player_x) dx = 1;
            else if (monster->x > map->player_x) dx = -1;
            if (monster->y < map->player_y) dy = 1;
            else if (monster->y > map->player_y) dy = -1;
        } 
        else {
            bool in_same_room = (map->player_x >= monster_room->pos.x && 
                               map->player_x < monster_room->pos.x + monster_room->max.x &&
                               map->player_y >= monster_room->pos.y && 
                               map->player_y < monster_room->pos.y + monster_room->max.y);
            if (monster->was_attacked && in_same_room) {
                should_follow = true;
                if (monster->x < map->player_x) dx = 1;
                else if (monster->x > map->player_x) dx = -1;
                if (monster->y < map->player_y) dy = 1;
                else if (monster->y > map->player_y) dy = -1;
            } 
            else if (in_same_room) {
                should_follow = true;
                int random_dir = rand() % 4;
                switch(random_dir) {
                    case 0: dx = 1; break;
                    case 1: dx = -1; break;
                    case 2: dy = 1; break;
                    case 3: dy = -1; break;
                }
            }
        }
        if (should_follow) {
            int new_x = monster->x + dx;
            int new_y = monster->y + dy;
            bool valid_move = false;
            if (new_x >= monster_room->pos.x && new_x < monster_room->pos.x + monster_room->max.x &&
                new_y >= monster_room->pos.y && new_y < monster_room->pos.y + monster_room->max.y) {
                if (!(new_x == map->player_x && new_y == map->player_y) && 
                    current->tiles[new_y][new_x] == '.' && 
                    !is_monster_at(current, new_x, new_y)) {
                    valid_move = true;
                }
            }
            if (valid_move) {
                monster->x = new_x;
                monster->y = new_y;
                current->tiles[orig_y][orig_x] = '.';
                current->tiles[monster->y][monster->x] = monster->symbol;
            }
            if (monster->type == MONSTER_UNDEAD || monster->was_attacked) {
                if (abs(monster->x - map->player_x) <= 1 && 
                    abs(monster->y - map->player_y) <= 1) {
                    monster->aggressive = true;
                }
            }
        }
    }
}

bool is_monster_at(Level *level, int x, int y) {
    for (int i = 0; i < MONSTER_COUNT; i++) {
        if (level->monsters[i].active && 
            level->monsters[i].x == x && 
            level->monsters[i].y == y) {
            return true;
        }
    }
    return false;
}

void handle_input(Map *map, int input) {
    Level *current = &map->levels[map->current_level - 1];
    int new_x = map->player_x;
    int new_y = map->player_y;
    if (input >= '1' && input <= '3') {
        int talisman_type = input - '1';
        if (map->talismans[talisman_type].owned && map->talismans[talisman_type].count > 0) {
            for (int i = 0; i < 5; i++) {
                if (map->talismans[talisman_type].active_durations[i] <= 0) {
                    map->talismans[talisman_type].active_durations[i] = 10;
                    map->talismans[talisman_type].count--;
                    
                    switch(talisman_type) {
                        case TALISMAN_HEALTH:
                            map->health_regen_doubled = true;
                            break;
                        case TALISMAN_DAMAGE:
                            map->damage_doubled = true;
                            break;
                        case TALISMAN_SPEED:
                            map->speed_doubled = true;
                            break;
                    }
                    char msg[MAX_MESSAGE_LENGTH];
                    snprintf(msg, MAX_MESSAGE_LENGTH, "Activated %s! (%d remaining)", 
                            map->talismans[talisman_type].name, 
                            map->talismans[talisman_type].count);
                    set_message(map, msg);
                    return;
                }
            }
            set_message(map, "Too many active talismans of this type!");
        } 
        else {
            set_message(map, "You don't have any of these talismans!");
        }
        return;
    }
    if (map->current_weapon == WEAPON_DAGGER || 
        map->current_weapon == WEAPON_WAND || 
        map->current_weapon == WEAPON_ARROW) {
        int dir_x = 0, dir_y = 0;
        switch(input) {
            case KEY_UP: dir_y = -1; break;
            case KEY_DOWN: dir_y = 1; break;
            case KEY_LEFT: dir_x = -1; break;
            case KEY_RIGHT: dir_x = 1; break;
        }
        if (dir_x != 0 || dir_y != 0) {
            switch(map->current_weapon) {
                case WEAPON_DAGGER: throw_dagger(map, dir_x, dir_y); break;
                case WEAPON_WAND: cast_magic_wand(map, dir_x, dir_y); break;
                case WEAPON_ARROW: shoot_arrow(map, dir_x, dir_y); break;
            }
            return;
        }
    }
    if (input == 'v' || input == 'V') {
        for (int i = 1; i < WEAPON_COUNT; i++) {
            if (!map->weapons[i].owned) {
                map->weapons[i].owned = true;
                if (i == WEAPON_DAGGER) {
                    map->weapons[i].ammo = 12;
                } else if (i == WEAPON_ARROW) {
                    map->weapons[i].ammo = 20;
                } else if (i == WEAPON_WAND) {
                    map->weapons[i].ammo = 8;
                }
                char msg[MAX_MESSAGE_LENGTH];
                snprintf(msg, MAX_MESSAGE_LENGTH, "Debug: Added %s to inventory", 
                        map->weapons[i].name);
                set_message(map, msg);
            }
        }
        return;
    }
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
    if (input == 'i' || input == 'I') {
        display_weapon_menu(stdscr, map);
        clear();
        refresh();
        update_visibility(map);
        return;
    }
    if (input == 'e' || input == 'E') {
        display_food_menu(stdscr, map);
        int menu_input = getch();
        if (menu_input == 'e' || menu_input == 'E') {
            eat_food(map);
        }
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
        while (true) {
            int test_x = map->player_x + dx;
            int test_y = map->player_y + dy;
            if (test_x < 0 || test_x >= NUMCOLS || 
                test_y < 0 || test_y >= NUMLINES ||
                current->tiles[test_y][test_x] == '|' || 
                current->tiles[test_y][test_x] == '_' || 
                current->tiles[test_y][test_x] == ' ' ||
                current->tiles[test_y][test_x] == '>' ||
                current->tiles[test_y][test_x] == '<' ||
                current->secret_walls[test_y][test_x] ||
                current->tiles[test_y][test_x] == 'D' ||
                current->tiles[test_y][test_x] == 'F' ||
                current->tiles[test_y][test_x] == 'G' ||
                current->tiles[test_y][test_x] == 'S' ||
                current->tiles[test_y][test_x] == 'U') {
                break;
            }
            map->player_x = test_x;
            map->player_y = test_y;
            if (current->traps[test_y][test_x] && !current->discovered_traps[test_y][test_x]) {
                int damage = 2 + (rand() % 3);
                map->health -= damage;
                current->discovered_traps[test_y][test_x] = true;
                char msg[MAX_MESSAGE_LENGTH];
                snprintf(msg, MAX_MESSAGE_LENGTH, "You triggered a trap! Lost %d health!", damage);
                set_message(map, msg);
                break;
            }
            update_visibility(map);
        }
        fast_travel_mode = false;
        return;
    }
    static int last_arrow = -1;
    static clock_t last_time = 0;
    clock_t current_time = clock();
    const double delay = 0.2;
    if (input == KEY_UP || input == KEY_DOWN || input == KEY_LEFT || input == KEY_RIGHT) {
        if (last_arrow != -1 && (current_time - last_time) / CLOCKS_PER_SEC < delay) {
            if ((last_arrow == KEY_UP && input == KEY_LEFT) || 
                (last_arrow == KEY_LEFT && input == KEY_UP)) {
                new_x--;
                new_y--;
                last_arrow = -1;
            } 
            else if ((last_arrow == KEY_UP && input == KEY_RIGHT) || 
                      (last_arrow == KEY_RIGHT && input == KEY_UP)) {
                new_x++;
                new_y--;
                last_arrow = -1;
            } 
            else if ((last_arrow == KEY_DOWN && input == KEY_LEFT) || 
                      (last_arrow == KEY_LEFT && input == KEY_DOWN)) {
                new_x--;
                new_y++;
                last_arrow = -1;
            } 
            else if ((last_arrow == KEY_DOWN && input == KEY_RIGHT) || 
                      (last_arrow == KEY_RIGHT && input == KEY_DOWN)) {
                new_x++;
                new_y++;
                last_arrow = -1;
            }
        } 
        else {
            last_arrow = input;
            last_time = current_time;
            return;
        }
    }
    if (current->current_secret_room != NULL) {
        switch (input) {
            case 'w': case 'W': new_y--; break;
            case 's': case 'S': new_y++; break;
            case 'a': case 'A': new_x--; break;
            case 'd': case 'D': new_x++; break;
            case '\n': case '\r':
                if (current->tiles[map->player_y][map->player_x] == 'T') {
                    int talisman_type = current->talisman_type;
                    if (!map->talismans[talisman_type].owned) {
                        map->talismans[talisman_type].owned = true;
                        current->tiles[map->player_y][map->player_x] = '.';
                        char msg[MAX_MESSAGE_LENGTH];
                        snprintf(msg, MAX_MESSAGE_LENGTH, "You obtained the %s!", 
                                map->talismans[talisman_type].name);
                        set_message(map, msg);
                        switch(talisman_type) {
                            case TALISMAN_HEALTH:
                                map->health += 10;
                                break;
                            case TALISMAN_DAMAGE:
                                map->strength += 5;
                                break;
                            case TALISMAN_SPEED:
                                break;
                        }
                    }
                }
                if (abs(map->player_x - current->current_secret_room->center.x) <= 1 &&
                    abs(map->player_y - current->current_secret_room->center.y) <= 1) {
                    for (int y = 0; y < NUMLINES; y++) {
                        for (int x = 0; x < NUMCOLS; x++) {
                            current->tiles[y][x] = current->backup_tiles[y][x];
                            current->visible_tiles[y][x] = current->backup_visible_tiles[y][x];
                            current->explored[y][x] = current->backup_explored[y][x];
                        }
                    }
                    map->player_x = current->secret_entrance.x;
                    map->player_y = current->secret_entrance.y;
                    if (current->secret_walls[map->player_y][map->player_x]) {
                        current->visible_tiles[map->player_y][map->player_x] = '?';
                        current->explored[map->player_y][map->player_x] = true;
                    }
                    current->current_secret_room = NULL;
                    set_message(map, "You return from the secret room.");
                    return;
                }
                break;
        }
        if (new_x >= 0 && new_x < NUMCOLS && new_y >= 0 && new_y < NUMLINES) {
            if (abs(new_x - current->current_secret_room->center.x) <= 3 &&
                abs(new_y - current->current_secret_room->center.y) <= 3) {
                char next_tile = current->tiles[new_y][new_x];
                if (next_tile != '|' && next_tile != '_') {
                    map->player_x = new_x;
                    map->player_y = new_y;
                }
            }
        }
        return;
    }
    switch (input) {
        case 'w': case 'W': new_y--; break;
        case 's': case 'S': new_y++; break;
        case 'a': case 'A': new_x--; break;
        case 'd': case 'D': new_x++; break;
        case '\n': case '\r':
            if (current->tiles[map->player_y][map->player_x] == 's' ||
                current->tiles[map->player_y][map->player_x] == 'd' || 
                current->tiles[map->player_y][map->player_x] == 'm' || 
                current->tiles[map->player_y][map->player_x] == 'a') {
                WeaponType weapon_type;
                switch(current->tiles[map->player_y][map->player_x]) {
                    case 's': weapon_type = WEAPON_SWORD; break;
                    case 'd': weapon_type = WEAPON_DAGGER; break;
                    case 'm': weapon_type = WEAPON_WAND; break;
                    case 'a': weapon_type = WEAPON_ARROW; break;
                    default: return;
                }
                if (current->tiles[map->player_y][map->player_x] == 'a') {
                    if (!map->weapons[WEAPON_ARROW].owned) {
                        map->weapons[WEAPON_ARROW].owned = true;
                        map->weapons[WEAPON_ARROW].ammo = 20;
                        set_message(map, "You picked up 20 arrows!");
                        current->tiles[map->player_y][map->player_x] = '.';
                    } else if (map->weapons[WEAPON_ARROW].ammo < 20) {
                        map->weapons[WEAPON_ARROW].ammo++;
                        set_message(map, "You replenished 1 arrow.");
                        current->tiles[map->player_y][map->player_x] = '.';
                    } else {
                        set_message(map, "You already have full arrows.");
                        return;
                    }
                }
                else if (weapon_type == WEAPON_DAGGER) {
                    if (!map->weapons[weapon_type].owned) {
                        map->weapons[weapon_type].owned = true;
                        map->weapons[weapon_type].ammo = 12;
                        set_message(map, "You picked up 12 daggers!");
                        current->tiles[map->player_y][map->player_x] = '.';
                    } else if (map->weapons[weapon_type].ammo < 12) {
                        map->weapons[weapon_type].ammo++;
                        set_message(map, "You replenished 1 dagger.");
                        current->tiles[map->player_y][map->player_x] = '.';
                    } else {
                        set_message(map, "You already have full daggers.");
                        return;
                    }
                }
                else {
                    if (!map->weapons[weapon_type].owned) {
                        map->weapons[weapon_type].owned = true;
                        char msg[MAX_MESSAGE_LENGTH];
                        snprintf(msg, MAX_MESSAGE_LENGTH, "You found a %s!", 
                                map->weapons[weapon_type].name);
                        set_message(map, msg);
                        current->tiles[map->player_y][map->player_x] = '.';
                    }
                }
                return;
            }
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
                    switch(talisman_type) {
                        case TALISMAN_HEALTH:
                            map->health += 10;
                            break;
                        case TALISMAN_DAMAGE:
                            map->strength += 5;
                            break;
                        case TALISMAN_SPEED:
                            break;
                    }
                }
            }
            if (current->tiles[new_y][new_x] == 'W') {
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
                    snprintf(msg, MAX_MESSAGE_LENGTH, "You found a %s!", 
                            map->weapons[weapon_index].name);
                    set_message(map, msg);
                }
            }
            if (current->tiles[map->player_y][map->player_x] == '>') {
                set_message(map, "Press Enter to go up to next level");
                if (map->current_level == 5) {
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
            if (current->secret_stairs[map->player_y][map->player_x]) {
                for (int y = 0; y < NUMLINES; y++) {
                    for (int x = 0; x < NUMCOLS; x++) {
                        current->backup_tiles[y][x] = current->tiles[y][x];
                        current->backup_visible_tiles[y][x] = current->visible_tiles[y][x];
                        current->backup_explored[y][x] = current->explored[y][x];
                    }
                }
                current->secret_entrance.x = map->player_x;
                current->secret_entrance.y = map->player_y;
                current->current_secret_room = &current->secret_rooms[0];
                map->player_x = current->current_secret_room->center.x;
                map->player_y = current->current_secret_room->center.y;
                draw_secret_room(current);
                set_message(map, "You descend the mysterious stairs into a secret Talisman room!");
                return;
            }
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int check_x = map->player_x + dx;
                    int check_y = map->player_y + dy; 
                    if (check_x >= 0 && check_x < NUMCOLS && 
                        check_y >= 0 && check_y < NUMLINES) {
                        if (current->secret_walls[check_y][check_x]) {
                            for (int y = 0; y < NUMLINES; y++) {
                                for (int x = 0; x < NUMCOLS; x++) {
                                    current->backup_tiles[y][x] = current->tiles[y][x];
                                    current->backup_visible_tiles[y][x] = current->visible_tiles[y][x];
                                    current->backup_explored[y][x] = current->explored[y][x];
                                }
                            }
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
        default:
            return;
    }
    if (new_x < 0 || new_x >= NUMCOLS || new_y < 0 || new_y >= NUMLINES) {
        return;
    }
    bool hasCombat = false;
    for (int i = 0; i < MONSTER_COUNT; i++) {
        Monster *monster = &current->monsters[i];
        if (monster->active && monster->x == new_x && monster->y == new_y) {
            hasCombat = true;
            int damage = (map->current_weapon == WEAPON_MACE) ? 5 : 
                        (map->current_weapon == WEAPON_SWORD) ? 10 : 3;
            monster->health -= damage;
            monster->was_attacked = true;
            monster->aggressive = true;
            if (monster->health <= 0) {
                monster->active = false;
                current->tiles[monster->y][monster->x] = '.';
                char msg[MAX_MESSAGE_LENGTH];
                snprintf(msg, MAX_MESSAGE_LENGTH, "You defeated a monster by bumping into it with your %s!", 
                        map->weapons[map->current_weapon].name);
                set_message(map, msg);
            } else {
                char msg[MAX_MESSAGE_LENGTH];
                snprintf(msg, MAX_MESSAGE_LENGTH, 
                        "You bump into the %s dealing %d damage! (Monster HP: %d/%d)", 
                        monster->name, damage, monster->health, monster->max_health);
                set_message(map, msg);
            }
            new_x = map->player_x;
            new_y = map->player_y;
            break;
        }
    }
    if ((map->current_weapon == WEAPON_MACE || map->current_weapon == WEAPON_SWORD) &&
        (new_x != map->player_x || new_y != map->player_y)) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int attack_x = new_x + dx;
                int attack_y = new_y + dy;
                if (attack_x >= 0 && attack_x < NUMCOLS && 
                    attack_y >= 0 && attack_y < NUMLINES) {
                    for (int i = 0; i < MONSTER_COUNT; i++) {
                        Monster *monster = &current->monsters[i];
                        if (monster->active && monster->x == attack_x && monster->y == attack_y) {
                            hasCombat = true;
                            int damage = calculate_damage(map, (map->current_weapon == WEAPON_MACE) ? 5 : 10);
                            monster->health -= damage;
                            monster->was_attacked = true;
                            monster->aggressive = true;
                            if (monster->health <= 0) {
                                monster->active = false;
                                current->tiles[monster->y][monster->x] = '.';
                                char msg[MAX_MESSAGE_LENGTH];
                                snprintf(msg, MAX_MESSAGE_LENGTH, "You defeated a monster with your %s!", 
                                        map->weapons[map->current_weapon].name);
                                set_message(map, msg);
                            } 
                            else {
                                char msg[MAX_MESSAGE_LENGTH];
                                snprintf(msg, MAX_MESSAGE_LENGTH, 
                                        "Your %s hits the %s for %d damage! (Monster HP: %d/%d)", 
                                        map->weapons[map->current_weapon].name,
                                        monster->name, damage, monster->health, monster->max_health);
                                set_message(map, msg);
                            }
                        }
                    }
                }
            }
        }
    }
    char next_tile = current->tiles[new_y][new_x];
    if (next_tile != '|' && next_tile != '_' && next_tile != ' ' &&
        next_tile != 'D' && next_tile != 'F' && next_tile != 'G' && 
        next_tile != 'S' && next_tile != 'U') {
        if (current->secret_walls[new_y][new_x]) {
            set_message(map, "You sense something strange about this wall. Press Enter to investigate.");
            return;
        }
        map->player_x = new_x;
        map->player_y = new_y;
        if (current->traps[new_y][new_x] && !current->discovered_traps[new_y][new_x]) {
            int damage = 2 + (rand() % 3);
            map->health -= damage;
            current->discovered_traps[new_y][new_x] = true;
            char msg[MAX_MESSAGE_LENGTH];
            snprintf(msg, MAX_MESSAGE_LENGTH, "You triggered a trap! Lost %d health!", damage);
            set_message(map, msg);
        }
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
    if (hasCombat) {
        for (int i = 0; i < MONSTER_COUNT; i++) {
            Monster *monster = &current->monsters[i];
            if (monster->active && (monster->aggressive || monster->type == MONSTER_UNDEAD)) {
                if (abs(monster->x - map->player_x) <= 1 && 
                    abs(monster->y - map->player_y) <= 1) {
                    map->health -= monster->damage;
                    char additional_msg[MAX_MESSAGE_LENGTH];
                    snprintf(additional_msg, MAX_MESSAGE_LENGTH, 
                            " | The %s counter-attacks for %d damage! (Your HP: %d)", 
                            monster->name, monster->damage, map->health);
                    strncat(map->current_message, additional_msg, 
                        MAX_MESSAGE_LENGTH - strlen(map->current_message) - 1);
                }
            }
        }
    }
    for (int type = 0; type < TALISMAN_COUNT; type++) {
        bool still_active = false;
        for (int i = 0; i < 5; i++) {
            if (map->talismans[type].active_durations[i] > 0) {
                map->talismans[type].active_durations[i]--;
                if (map->talismans[type].active_durations[i] > 0) {
                    still_active = true;
                }
            }
        }
        switch(type) {
            case TALISMAN_HEALTH:
                map->health_regen_doubled = still_active;
                break;
            case TALISMAN_DAMAGE:
                map->damage_doubled = still_active;
                break;
            case TALISMAN_SPEED:
                map->speed_doubled = still_active;
                break;
        }
    }
    if (map->health_regen_doubled && map->hunger > 50 && map->health < 25) {
        if (map->health < 25) {
            map->health++;
        }
    }
    if (map->speed_doubled) {
        if (map->message_timer <= 0) {
            set_message(map, "Speed boost active!");
        }
    }
    if (map->damage_doubled) {
        if (hasCombat && map->message_timer > 0) {
            char additional_msg[MAX_MESSAGE_LENGTH];
            snprintf(additional_msg, MAX_MESSAGE_LENGTH, " (Damage doubled by talisman!)");
            strncat(map->current_message, additional_msg, 
                MAX_MESSAGE_LENGTH - strlen(map->current_message) - 1);
        }
    }
    if (map->speed_doubled && (new_x != map->player_x || new_y != map->player_y)) {
        int dx = new_x - map->player_x;
        int dy = new_y - map->player_y;
        new_x += dx;
        new_y += dy;
        if (new_x >= 0 && new_x < NUMCOLS && new_y >= 0 && new_y < NUMLINES) {
            if (current->tiles[new_y][new_x] != '|' && 
                current->tiles[new_y][new_x] != '_' && 
                current->tiles[new_y][new_x] != ' ') {
                map->player_x = new_x;
                map->player_y = new_y;
            }
        }
    }
    update_monsters(map);
}
int main() {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    load_username();
    getmaxyx(stdscr, NUMLINES, NUMCOLS);
    NUMLINES -= 4;
    curs_set(0);
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_YELLOW, COLOR_BLACK); 
        init_pair(2, COLOR_WHITE, COLOR_BLACK); 
        init_pair(3, COLOR_RED, COLOR_BLACK);   
        init_pair(4, COLOR_GREEN, COLOR_BLACK); 
        init_pair(5, COLOR_CYAN, COLOR_BLACK);   
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK); 
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
                                color = 5;  
                                attron(COLOR_PAIR(color));
                                mvprintw(y + 4, x + 1, "●");
                                attroff(COLOR_PAIR(color));
                                continue;
                            case '$': 
                                color = 1;  
                                attron(COLOR_PAIR(color));
                                mvprintw(y + 4, x + 1, "▲");
                                attroff(COLOR_PAIR(color));
                                continue;
                            case '&': 
                                color = 6;  
                                attron(COLOR_PAIR(color));
                                mvprintw(y + 4, x + 1, "△");
                                attroff(COLOR_PAIR(color));
                                continue;
                            case 'T': 
                                {
                                    int color;
                                    switch(current->talisman_type) {
                                        case TALISMAN_HEALTH:
                                            color = COLOR_PAIR(4); 
                                            break;
                                        case TALISMAN_DAMAGE:
                                            color = COLOR_PAIR(3); 
                                            break;
                                        case TALISMAN_SPEED:
                                            color = COLOR_PAIR(5); 
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
                            case 'D': case 'F': case 'G': case 'S': case 'U':
                                {
                                    int color;
                                    switch(current->visible_tiles[y][x]) {
                                        case 'D': color = 3; break; 
                                        case 'F': color = 1; break;  
                                        case 'G': color = 4; break; 
                                        case 'S': color = 5; break;
                                        case 'U': color = 6; break;  
                                        default: color = 2;
                                    }
                                    attron(COLOR_PAIR(color));
                                    mvaddch(y + 4, x + 1, current->visible_tiles[y][x]);
                                    attroff(COLOR_PAIR(color));
                                }
                                continue;
                            case 's': case 'd': case 'm': case 'a':
                            {
                                attron(COLOR_PAIR(5));  
                                mvaddch(y + 4, x + 1, current->visible_tiles[y][x]);
                                attroff(COLOR_PAIR(5));
                                continue;
                            }
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
            mvprintw(NUMLINES + 2, 2, "Level: ");
            attron(COLOR_PAIR(4));  
            printw("%d", map->current_level);
            attroff(COLOR_PAIR(4));

            attron(COLOR_PAIR(2));
            printw("  Health: ");
            attron(COLOR_PAIR(3)); 
            printw("%d", map->health);
            attroff(COLOR_PAIR(3));

            attron(COLOR_PAIR(2));
            printw("  Str: ");
            attron(COLOR_PAIR(3)); 
            printw("%d", map->strength);
            attroff(COLOR_PAIR(3));

            attron(COLOR_PAIR(2));
            printw("  Gold: ");
            attron(COLOR_PAIR(1));  
            printw("%d", map->gold);
            attroff(COLOR_PAIR(1));

            attron(COLOR_PAIR(2));
            printw("  Armor: ");
            attron(COLOR_PAIR(5));
            printw("%d", map->armor);
            attroff(COLOR_PAIR(5));

            attron(COLOR_PAIR(2));
            printw("  Exp: ");
            attron(COLOR_PAIR(4));  
            printw("%d", map->exp);
            attroff(COLOR_PAIR(4));
            attroff(COLOR_PAIR(2));

            attron(COLOR_PAIR(2));
            printw("  Current User: ");
            attron(COLOR_PAIR(4));  
            printw("%s", current_username);
            attroff(COLOR_PAIR(4));
            attroff(COLOR_PAIR(2));
            if (map->health_regen_doubled || map->damage_doubled || map->speed_doubled) {
                attron(COLOR_PAIR(6));  
                mvprintw(NUMLINES + 2, NUMCOLS - 30, "Active Talismans: ");
                if (map->health_regen_doubled) printw("H ");
                if (map->damage_doubled) printw("D ");
                if (map->speed_doubled) printw("S ");
                attroff(COLOR_PAIR(6));
            }
        attroff(COLOR_PAIR(2));
        refresh();
        int ch = getch();
        if (++map->hunger_timer >= 100) {  
            map->hunger_timer = 0;
            if (map->hunger > 0) {
                map->hunger--;
            }
            if (map->hunger <= 20 && map->health > 0) {
                map->health--;
                set_message(map, "You are starving!");
            }
            if (map->hunger > 50 && map->health < 25) { 
                Level *current = &map->levels[map->current_level - 1];
                bool monster_nearby = false;
                
                for (int i = 0; i < MONSTER_COUNT; i++) {
                    Monster *monster = &current->monsters[i];
                    if (monster->active && 
                        abs(monster->x - map->player_x) <= 2 && 
                        abs(monster->y - map->player_y) <= 2) {
                        monster_nearby = true;
                        break;
                    }
                }
                if (!monster_nearby && map->health < 25) {
                    map->health++;
                    if (map->message_timer <= 0) { 
                        set_message(map, "You feel your health returning...");
                    }
                }
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
            show_lose_screen(map);
            free_map(map);
            clear();
            refresh();
            endwin();
            curs_set(1);
            system("clear");
            exit(0);
        }
    }
    free_map(map);
    endwin();
    return 0;
}