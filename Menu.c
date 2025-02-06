#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>  
#include <sys/stat.h>
#include <locale.h>
#include <wchar.h>
#include <unistd.h>
#define MUSIC_FOLDER "./Music"
#define MAX_MUSIC_FILES 10
#define MAX_FILENAME_LENGTH 100
#define GAME_MENU_SIZE 6
#define COLOR_OPTIONS 5 
#define FILE_NAME "user_data.txt"
#define MENU_SIZE 6
#define LEADERBOARD_FILE "leaderboard.txt"
#define MAX_LEADERBOARD_ENTRIES 100
#define SETTINGS_FILE "game_settings.txt"
#define DIFFICULTY_OPTIONS 3
const char *game_menu[] = {
    "1. Start New Game",
    "2. Resume Previous Game",
    "3. Game Settings",
    "4. Character Color",
    "5. Music",
    "6. Exit to Main Menu"
};
char current_difficulty[20] = "Normal";
char current_user[50] = "";
char current_email[100] = "";
char current_music[MAX_FILENAME_LENGTH] = "None";
int music_enabled = 0;
int logged_in = 0;
int num_accounts = 0;
typedef struct {
    char username[50];
    int level;
    int hit;
    int strength;
    int gold;
    int armor;
    int exp;
    int games_played;
} UserScore;
typedef struct {
    char username[50];
    char email[100];
    char password[50];
    int score;
    int gold;
    int games;
    int experience;
} User;
typedef struct {
    char filename[MAX_FILENAME_LENGTH];
    char display_name[MAX_FILENAME_LENGTH];
    int is_playing;
} MusicTrack;
void draw_double_char(WINDOW *win, int y, int x, char c) {
    wattron(win, A_BOLD);
    mvwaddch(win, y, x, c);
    mvwaddch(win, y, x + 1, c);
    wattroff(win, A_BOLD);
}

const char *menu[] = {
    "1. Create an Account",
    "2. Log In",
    "3. Game Menu",
    "4. Profile Menu",
    "5. Leaderboard Menu",
    "6. Exit"
};
int main();
void handle_winch(int sig);
void display_menu(WINDOW *menu_win, int highlight);
void execute_option(int choice, WINDOW *menu_win);
void clear_line(WINDOW *menu_win, int y, int x, int width);
void create_account(WINDOW *menu_win);
void save_account(User user);
bool check_account_exists(const char *username, const char *email);
bool validate_password(const char *password);
bool validate_email(const char *email);
void generate_random_password(char *password);
void initialize_user_score(const char *username);
UserScore load_user_score(const char *username);
void save_user_score(const UserScore *score);
void login_menu(WINDOW *menu_win);
void trim_newline(char *str);
void forgot_password(WINDOW *menu_win);
void display_game_menu(WINDOW *menu_win, int highlight);
void update_current_difficulty();
void save_difficulty(int difficulty);
int get_current_difficulty();
void show_game_settings(WINDOW *menu_win);
void showGameMenu(WINDOW *menu_win);
int get_difficulty();
void show_character_color_settings(WINDOW *menu_win);
void save_character_color(int color_pair);
int get_character_color();
void showProfileMenu(WINDOW *menu_win);
void stop_current_music();
void play_music(const char* filename);
int check_music_system();
void get_music_files(MusicTrack *tracks, int *count);
void save_music_settings(int enabled, const char* current_track);
void load_music_settings();
void show_music_settings(WINDOW *menu_win);
void init_leaderboard_colors();
void showLeaderboardMenu(WINDOW *menu_win);
int min(int a, int b);
void write_game_state(const char* state);
void update_game_settings_username(const char *username);
void write_game_state(const char* state) {
    FILE* f = fopen("game_state.txt", "w");
    if (f) {
        fprintf(f, "%s", state);
        fclose(f);
    }
}
void handle_winch(int sig) {
    (void)sig;
    endwin();
    refresh();
    clear();
}
void display_menu(WINDOW *menu_win, int highlight) {
    int height, width;
    getmaxyx(stdscr, height, width);
    int x, y = 1;  // Keep original y position
    int max_len = 0;

    // Draw fancy border
    wattron(menu_win, COLOR_PAIR(1));
    
    // Top border
    mvwprintw(menu_win, 0, 0, "╔");
    for (int i = 1; i < width - 1; i++) {
        mvwprintw(menu_win, 0, i, "═");
    }
    mvwprintw(menu_win, 0, width - 1, "╗");

    // Side borders
    for (int i = 1; i < height - 1; i++) {
        mvwprintw(menu_win, i, 0, "║");
        mvwprintw(menu_win, i, width - 1, "║");
    }

    // Bottom border
    mvwprintw(menu_win, height - 1, 0, "╚");
    for (int i = 1; i < width - 1; i++) {
        mvwprintw(menu_win, height - 1, i, "═");
    }
    mvwprintw(menu_win, height - 1, width - 1, "╝");
    wattroff(menu_win, COLOR_PAIR(1));

    // Keep your original menu content
    for (int i = 0; i < MENU_SIZE; i++) {
        int len = strlen(menu[i]);
        if (len > max_len) max_len = len;
    }
    x = (width - max_len) / 2;
    int user_x = 5;
    int user_y = 1;

    mvwprintw(menu_win, 1, 2, "User: ");
    wattron(menu_win, COLOR_PAIR(4) | A_BOLD);
    wprintw(menu_win, "%s", current_user);
    wattroff(menu_win, COLOR_PAIR(4) | A_BOLD);

    mvwprintw(menu_win, 2, 2, "Difficulty: ");
    wattron(menu_win, COLOR_PAIR(5) | A_BOLD);
    wprintw(menu_win, "%s", current_difficulty);
    wattroff(menu_win, COLOR_PAIR(5) | A_BOLD);

    for (int i = 0; i < MENU_SIZE; i++) {
        if (highlight == i + 1) {
            wattron(menu_win, A_REVERSE);
            mvwprintw(menu_win, y, x, "%s", menu[i]);
            wattroff(menu_win, A_REVERSE);
        } else {
            mvwprintw(menu_win, y, x, "%s", menu[i]);
        }
        y++;
    }
    wrefresh(menu_win);
}
int main() {
    unlink("game_state.txt");
    setlocale(LC_ALL, "");
    int highlight = 1;
    int choice = 0;
    int c;
    initscr();
    clear();
    noecho();
    cbreak();
    curs_set(0);
    start_color();
    use_default_colors();
    load_music_settings();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_GREEN, COLOR_BLACK);
    init_pair(5, COLOR_CYAN, COLOR_BLACK);
    if (music_enabled && strcmp(current_music, "None") != 0) {
        play_music(current_music);
    }
    signal(SIGWINCH, handle_winch);
    if (check_music_system() && music_enabled && strcmp(current_music, "None") != 0) {
        play_music(current_music);
    }
    int height, width;
    getmaxyx(stdscr, height, width);
    WINDOW *menu_win = newwin(height, width, 0, 0);
    keypad(menu_win, TRUE);
    while (1) {
        getmaxyx(stdscr, height, width);
        wresize(menu_win, height, width);
        werase(menu_win);
        update_current_difficulty();
        display_menu(menu_win, highlight);
        c = wgetch(menu_win);
        switch (c) {
            case KEY_UP:
                highlight = (highlight == 1) ? MENU_SIZE : highlight - 1;
                break;
            case KEY_DOWN:
                highlight = (highlight == MENU_SIZE) ? 1 : highlight + 1;
                break;
            case 10:
                choice = highlight;
                break;
            case 27:
                endwin();
                exit(0);
        }
        if (choice != 0) {
            execute_option(choice, menu_win);
            choice = 0;
            werase(menu_win);
        }
    }
    endwin();
    return 0;
}
void update_game_settings_username(const char *username) {
    FILE *file = fopen(SETTINGS_FILE, "r");
    char lines[5][256];
    int line_count = 0;

    // Read existing settings
    if (file != NULL) {
        while (line_count < 5 && fgets(lines[line_count], sizeof(lines[0]), file)) {
            // Skip the Username line if it exists
            if (strncmp(lines[line_count], "Username:", 9) != 0) {
                line_count++;
            }
        }
        fclose(file);
    }

    // Write updated settings
    file = fopen(SETTINGS_FILE, "w");
    if (file != NULL) {
        fprintf(file, "Username: %s\n", username);
        for (int i = 0; i < line_count; i++) {
            fprintf(file, "%s", lines[i]);
        }
        fclose(file);
    }
}
void execute_option(int choice, WINDOW *menu_win) {
    werase(menu_win);
    box(menu_win, 0, 0);
    wrefresh(menu_win);

    if (choice == 6) {
        endwin();
        printf("Exiting the program. Goodbye!\n");
        exit(0);
    } else if (choice == 1) {
        create_account(menu_win);
    }
    else if (choice == 2){
        login_menu(menu_win);
    }
    else if (choice == 3){
        showGameMenu(menu_win);
    }
    else if (choice == 4){
        showProfileMenu(menu_win);
    }
    else if (choice == 5){
        showLeaderboardMenu(menu_win);
    }
    else {
        mvwprintw(menu_win, LINES / 2, (COLS - 20) / 2, "Option %d selected", choice);
        wrefresh(menu_win);
        wgetch(menu_win);
    }
}

void clear_line(WINDOW *menu_win, int y, int x, int width) {
    mvwprintw(menu_win, y, x, "%*s", width, " ");
    wmove(menu_win, y, x);
}

void create_account(WINDOW *menu_win) {
    User new_user;
    int height, width;

    getmaxyx(stdscr, height, width);
    int base_y = 3;
    int x = 5;
    char generated_password[50] = "";
    bool username_valid = false;
    bool email_valid = false;

    new_user.username[0] = '\0';
    new_user.email[0] = '\0';
    new_user.password[0] = '\0';

    while (1) {
        werase(menu_win);
        wattron(menu_win, COLOR_PAIR(1));
        mvwprintw(menu_win, 0, 0, "╔");
        for (int i = 1; i < width - 1; i++) {
            mvwprintw(menu_win, 0, i, "═");
        }
        mvwprintw(menu_win, 0, width - 1, "╗");
        for (int i = 1; i < height - 1; i++) {
            mvwprintw(menu_win, i, 0, "║");
            mvwprintw(menu_win, i, width - 1, "║");
        }
        mvwprintw(menu_win, height - 1, 0, "╚");
        for (int i = 1; i < width - 1; i++) {
            mvwprintw(menu_win, height - 1, i, "═");
        }
        mvwprintw(menu_win, height - 1, width - 1, "╝");
        wattroff(menu_win, COLOR_PAIR(1));
        mvwprintw(menu_win, 1, (width - strlen("Create Account")) / 2, "Create Account");
        mvwprintw(menu_win, base_y + 5, (width - strlen("Press Tab to generate a random password.")) / 2, "Press Tab to generate a random password.");
        
        // Username field
        if (!username_valid) {
            mvwprintw(menu_win, base_y, (width - strlen("Username: ")) / 2 - 15, "Username: ");
            wmove(menu_win, base_y, (width - strlen("Username: ")) / 2 + strlen("Username: ") - 15);
            wrefresh(menu_win);
            echo();
            int ch = wgetch(menu_win);
            if (ch == 27) {
                noecho();
                return;
            }
            ungetch(ch);
            mvwgetstr(menu_win, base_y, (width - strlen("Username: ")) / 2 + strlen("Username: ") - 15, new_user.username);
            noecho();

            if (strcmp(new_user.username, "") == 0) {
                mvwprintw(menu_win, base_y + 1, (width - strlen("Username cannot be empty.")) / 2, "Username cannot be empty.");
                wrefresh(menu_win);
                wgetch(menu_win);
                // Clear only the error message and username field
                clear_line(menu_win, base_y, (width - strlen("Username: ")) / 2 + strlen("Username: ") - 15, width - 2 * x);
                clear_line(menu_win, base_y + 1, (width - strlen("Username: ")) / 2 - 15, width - 2 * (width - strlen("Username: ")) / 2 - 15);
                continue;
            }
            username_valid = true;
        } else {
            mvwprintw(menu_win, base_y, (width - strlen("Username: ")) / 2 - 15, "Username: %s", new_user.username);
        }

        // Email field
        if (!email_valid) {
            mvwprintw(menu_win, base_y + 2, (width - strlen("Email: ")) / 2 - 15, "Email: ");
            wmove(menu_win, base_y + 2, (width - strlen("Email: ")) / 2 + strlen("Email: ") - 15);
            wrefresh(menu_win);
            echo();
            int ch = wgetch(menu_win);
            if (ch == 27) {
                noecho();
                return;
            }
            ungetch(ch);
            mvwgetstr(menu_win, base_y + 2, (width - strlen("Email: ")) / 2 + strlen("Email: ") - 15, new_user.email);
            noecho();

            if (check_account_exists(new_user.username, new_user.email)) {
                mvwprintw(menu_win, base_y + 3, (width - strlen("Username or Email already exists.")) / 2, "Username or Email already exists.");
                wrefresh(menu_win);
                wgetch(menu_win);
                // Clear only the error message and email field
                clear_line(menu_win, base_y + 2, (width - strlen("Email: ")) / 2 + strlen("Email: ") - 15, width - 2 * x);
                clear_line(menu_win, base_y + 3, (width - strlen("Email: ")) / 2 - 15, width - 2 * x - 15);
                clear_line(menu_win, base_y, (width - strlen("Username: ")) / 2 + strlen("Username: ") - 15, width - 2 * x);
                clear_line(menu_win, base_y + 1, (width - strlen("Username: ")) / 2 - 15, width - 2 * (width - strlen("Username: ")) / 2 - 15);
                continue;
            }
            if (!validate_email(new_user.email)) {
                mvwprintw(menu_win, base_y + 3, (width - strlen("Invalid Email format.")) / 2, "Invalid Email format.");
                wrefresh(menu_win);
                wgetch(menu_win);
                // Clear only the error message and email field
                clear_line(menu_win, base_y + 2, (width - strlen("Email: ")) / 2 + strlen("Email: ") - 15, width - 2 * x);
                clear_line(menu_win, base_y + 3, (width - strlen("Email: ")) / 2 - 15, width - 2 * (width - strlen("Email: ")) / 2 - 15);
                clear_line(menu_win, base_y, (width - strlen("Username: ")) / 2 + strlen("Username: ") - 15, width - 2 * x);
                clear_line(menu_win, base_y + 1, (width - strlen("Username: ")) / 2 - 15, width - 2 * (width - strlen("Username: ")) / 2 - 15);
                continue;
            }
            email_valid = true;
        } else {
            mvwprintw(menu_win, base_y + 2, (width - strlen("Email: ")) / 2 - 15, "Email: %s", new_user.email);
        }

        // Password field
        mvwprintw(menu_win, base_y + 4, (width - strlen("Password: ")) / 2 - 15, "Password: ");
        wmove(menu_win, base_y + 4, (width - strlen("Password: ")) / 2 + strlen("Password: ") - 15);
        wrefresh(menu_win);
        echo();
        int ch = wgetch(menu_win);
        noecho();

        if (ch == 27) {
            return;
        } else if (ch == 9) { // Tab key
            generate_random_password(generated_password);
            mvwprintw(menu_win, base_y + 4, (width - strlen("Password: ")) / 2 + strlen("Password: ") - 15, "%s", generated_password);
            wrefresh(menu_win);
            strcpy(new_user.password, generated_password);
            wgetch(menu_win);
        } else {
            ungetch(ch);
            echo();
            mvwgetstr(menu_win, base_y + 4, (width - strlen("Password: ")) / 2 + strlen("Password: ") - 15, new_user.password);
            noecho();
        }

        if (!validate_password(new_user.password)) {
            mvwprintw(menu_win, base_y + 5, (width - strlen("Password must have at least 7 characters, 1 uppercase, 1 lowercase, and 1 digit.")) / 2, "Password must have at least 7 characters, 1 uppercase, 1 lowercase, and 1 digit.");
            wrefresh(menu_win);
            wgetch(menu_win);
            // Clear only the error message and password field
            clear_line(menu_win, base_y + 4, (width - strlen("Password: ")) / 2 + strlen("Password: ") - 15, width - 2 * (width - strlen("Password: ")) / 2 + strlen("Password: "));
            clear_line(menu_win, base_y + 5, (width - strlen("Password: ")) / 2 - 15, width - 2 * x);
            continue;
        }

        strcpy(current_user, new_user.username);
        strcpy(current_email, new_user.email);
        save_account(new_user);
        initialize_user_score(new_user.username); 
        update_game_settings_username(current_user);
        mvwprintw(menu_win, base_y + 6, (width - strlen("Account created successfully!")) / 2, "Account created successfully!");
        wrefresh(menu_win);
        wgetch(menu_win);
        break;
    }
}

void save_account(User user) {
    FILE *file = fopen(FILE_NAME, "a");
    if (file == NULL) {
        printw("Error saving account information.\n");
        exit(1);
    }

    fprintf(file, "Username: %s\n", user.username);
    fprintf(file, "Password: %s\n", user.password);
    fprintf(file, "Email: %s\n", user.email);
    fprintf(file, "\n");

    fclose(file);
}

bool check_account_exists(const char *username, const char *email) {
    FILE *file = fopen(FILE_NAME, "r");
    char line[200];
    char stored_username[50];
    char stored_email[100];

    if (file == NULL) return false;

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "Username: %49s", stored_username) == 1) {
            if (strcmp(stored_username, username) == 0) {
                fclose(file);
                return true;
            }
        } else if (sscanf(line, "Email: %99s", stored_email) == 1) {
            if (strcmp(stored_email, email) == 0) {
                fclose(file);
                return true;
            }
        }
    }

    fclose(file);
    return false;
}

bool validate_password(const char *password) {
    int has_upper = 0, has_lower = 0, has_digit = 0;
    if (strlen(password) < 7) return false;

    for (int i = 0; password[i] != '\0'; i++) {
        if (isupper(password[i])) has_upper = 1;
        if (islower(password[i])) has_lower = 1;
        if (isdigit(password[i])) has_digit = 1;
    }

    return has_upper && has_lower && has_digit;
}

bool validate_email(const char *email) {
    const char *at = strchr(email, '@');
    const char *dot = strrchr(email, '.');

    if (!at || !dot || at > dot) return false;
    return true;
}

void generate_random_password(char *password) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const int length = 12;
    srand(time(NULL));

    for (int i = 0; i < length; i++) {
        password[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    password[length] = '\0';
}
void initialize_user_score(const char *username) {
    FILE *file = fopen("user_score.txt", "a+");
    if (file == NULL) {
        return;
    }

    // Check if user already exists
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, username) != NULL) {
            fclose(file);
            return; // User already exists
        }
    }

    // Add new user with default values
    fprintf(file, "Username: %s\n", username);
    fprintf(file, "Level: 1\n");
    fprintf(file, "Hit: 12\n");
    fprintf(file, "Strength: 16\n");
    fprintf(file, "Gold: 0\n");
    fprintf(file, "Armor: 0\n");
    fprintf(file, "Exp: 0\n");
    fprintf(file, "Games Played: 0\n\n");

    fclose(file);
}
UserScore load_user_score(const char *username) {
    UserScore score;
    // Initialize with default values
    strcpy(score.username, username);
    score.level = 1;
    score.hit = 12;
    score.strength = 16;
    score.gold = 0;
    score.armor = 0;
    score.exp = 0;
    score.games_played = 0;

    FILE *file = fopen("user_score.txt", "r");
    if (file == NULL) {
        return score;
    }

    char line[256];
    bool reading_user = false;

    while (fgets(line, sizeof(line), file)) {
        // Check for username line
        if (strncmp(line, "Username: ", 10) == 0) {
            char current_user[50];
            sscanf(line, "Username: %s", current_user);
            if (strcmp(current_user, username) == 0) {
                reading_user = true;
                continue;
            }
            reading_user = false;
        }

        // If we're reading the correct user's data, parse the stats
        if (reading_user) {
            if (strncmp(line, "Level: ", 7) == 0) 
                sscanf(line, "Level: %d", &score.level);
            else if (strncmp(line, "Hit: ", 5) == 0) 
                sscanf(line, "Hit: %d", &score.hit);
            else if (strncmp(line, "Strength: ", 10) == 0) 
                sscanf(line, "Strength: %d", &score.strength);
            else if (strncmp(line, "Gold: ", 6) == 0) 
                sscanf(line, "Gold: %d", &score.gold);
            else if (strncmp(line, "Armor: ", 7) == 0) 
                sscanf(line, "Armor: %d", &score.armor);
            else if (strncmp(line, "Exp: ", 5) == 0) 
                sscanf(line, "Exp: %d", &score.exp);
            else if (strncmp(line, "Games Played: ", 14) == 0) 
                sscanf(line, "Games Played: %d", &score.games_played);
        }
    }
    fclose(file);
    return score;
}
void save_user_score(const UserScore *score) {
    FILE *file = fopen("user_score.txt", "r");
    FILE *temp = fopen("temp_score.txt", "w");
    if (file == NULL || temp == NULL) {
        if (temp) fclose(temp);
        if (file) fclose(file);
        return;
    }
    char line[256];
    bool copying_user = true;
    bool user_found = false;
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Username: ", 10) == 0) {
            char current_user[50];
            sscanf(line, "Username: %s", current_user);
            if (strcmp(current_user, score->username) == 0) {
                fprintf(temp, "Username: %s\n", score->username);
                fprintf(temp, "Level: %d\n", score->level);
                fprintf(temp, "Hit: %d\n", score->hit);
                fprintf(temp, "Strength: %d\n", score->strength);
                fprintf(temp, "Gold: %d\n", score->gold);
                fprintf(temp, "Armor: %d\n", score->armor);
                fprintf(temp, "Exp: %d\n", score->exp);
                fprintf(temp, "Games Played: %d\n\n", score->games_played);              
                copying_user = false;
                user_found = true;
                for (int i = 0; i < 8; i++) {
                    fgets(line, sizeof(line), file);
                }
            }
        }
        if (copying_user) {
            fprintf(temp, "%s", line);
        }
    }
    if (!user_found) {
        fprintf(temp, "Username: %s\n", score->username);
        fprintf(temp, "Level: %d\n", score->level);
        fprintf(temp, "Hit: %d\n", score->hit);
        fprintf(temp, "Strength: %d\n", score->strength);
        fprintf(temp, "Gold: %d\n", score->gold);
        fprintf(temp, "Armor: %d\n", score->armor);
        fprintf(temp, "Exp: %d\n", score->exp);
        fprintf(temp, "Games Played: %d\n\n", score->games_played);
    }
    fclose(file);
    fclose(temp);
    remove("user_score.txt");
    rename("temp_score.txt", "user_score.txt");
}
void login_menu(WINDOW *menu_win) {
    char username[50];
    char password[50];
    int height, width;
    getmaxyx(stdscr, height, width);
    int y = 3;
    int x = 5;
    bool logged_in = false;
    bool forgot_password_selected = false;
    int input_x;

    while (!logged_in && !forgot_password_selected) {
        werase(menu_win);
        werase(menu_win);
        wattron(menu_win, COLOR_PAIR(1));
        mvwprintw(menu_win, 0, 0, "╔");
        for (int i = 1; i < width - 1; i++) {
            mvwprintw(menu_win, 0, i, "═");
        }
        mvwprintw(menu_win, 0, width - 1, "╗");
        for (int i = 1; i < height - 1; i++) {
            mvwprintw(menu_win, i, 0, "║");
            mvwprintw(menu_win, i, width - 1, "║");
        }
        mvwprintw(menu_win, height - 1, 0, "╚");
        for (int i = 1; i < width - 1; i++) {
            mvwprintw(menu_win, height - 1, i, "═");
        }
        mvwprintw(menu_win, height - 1, width - 1, "╝");
        wattroff(menu_win, COLOR_PAIR(1));
        mvwprintw(menu_win, 1, (width - strlen("Login Menu")) / 2, "Login Menu");
        mvwprintw(menu_win, 7, (width - strlen("Press ↓ Arrow for Forgot Password")) / 2, "Press ↓ Arrow for Forgot Password"); // Added instruction
        wrefresh(menu_win);
        y = 3;

        mvwprintw(menu_win, y, (width - strlen("Username: ")) / 2 - 15, "Username: ");
        input_x = (width - strlen("Username: ")) / 2 - 15 + strlen("Username: ");
        wmove(menu_win, y, input_x);
        wrefresh(menu_win);
        echo();
        int ch = wgetch(menu_win);
        if (ch == 27) {
            noecho();
            return;
        }
        ungetch(ch);
        mvwgetstr(menu_win, y, input_x, username);
        noecho();
        y++;

        if (strcmp(username, "guest") == 0) {
            strcpy(current_user, "Guest");
            update_game_settings_username("Guest");  // Add this line
            mvwprintw(menu_win, y, (width - strlen("Logged in as guest!")) / 2, "Logged in as guest!");
            wrefresh(menu_win);
            wgetch(menu_win);
            logged_in = true;
            break;
        }
        mvwprintw(menu_win, y, (width - strlen("Password: ")) / 2 - 15, "Password: ");
        input_x = (width - strlen("Password: ")) / 2 - 15 + strlen("Password: ");
        wmove(menu_win, y, input_x);
        wrefresh(menu_win);
        echo();
        ch = wgetch(menu_win);
        if (ch == 27) {
            noecho();
            return;
        } 
        else if (ch == KEY_DOWN) {
            forgot_password_selected = true;
            continue;
        }
        ungetch(ch);
        mvwgetstr(menu_win, y, input_x, password);
        noecho();
        y++;

        FILE *file = fopen(FILE_NAME, "r");
        if (file == NULL) {
            mvwprintw(menu_win, y, (width - strlen("Invalid username or password.")) / 2, "Invalid username or password.");
            wrefresh(menu_win);
            wgetch(menu_win);
            continue;
        }

        char line[200];
        char stored_username[50];
        char stored_password[50];
        bool found = false;

        while (fgets(line, sizeof(line), file)) {
            if (sscanf(line, "Username: %49s", stored_username) == 1) {
                if (strcmp(stored_username, username) == 0) {
                    fgets(line, sizeof(line), file);
                    if (sscanf(line, "Password: %49s", stored_password) == 1) {
                        if (strcmp(stored_password, password) == 0) {
                            found = true;
                            break;
                        }
                    }
                }
            }
        }
        fclose(file);

        if (found) {
            strcpy(current_user, username);
            UserScore user_score = load_user_score(username);
            update_game_settings_username(username);  // Add this line
            mvwprintw(menu_win, y, (width - strlen("Login successful!")) / 2, "Login successful!");
            wrefresh(menu_win);
            wgetch(menu_win);
            logged_in = true;
        }

        else {
            mvwprintw(menu_win, y, (width - strlen("Invalid username or password.")) / 2, "Invalid username or password.");
            wrefresh(menu_win);
            wgetch(menu_win);
        }
    }

    if (forgot_password_selected) {
        forgot_password(menu_win);
    }
}
void trim_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
    if (len > 1 && str[len - 2] == '\r') {
        str[len - 2] = '\0';
    }
}

void forgot_password(WINDOW *menu_win) {
    char username[50];
    char email[100];
    char new_password[50];
    int height, width;
    getmaxyx(stdscr, height, width);
    int y = 3;
    int x = 5;
    int input_x;

    while (1) {
        werase(menu_win);
        box(menu_win, 0, 0);
        mvwprintw(menu_win, 1, (width - strlen("Forgot Password")) / 2, "Forgot Password");
        wrefresh(menu_win);
        y = 3;
        // Get username
        mvwprintw(menu_win, y, (width - strlen("Username: ")) / 2 - 15, "Username: ");
        int ch = wgetch(menu_win);
        if (ch == 27) {
            noecho();
            return;
        }
        ungetch(ch);
        input_x = (width - strlen("Username: ")) / 2 - 15 + strlen("Username: ");
        wmove(menu_win, y, input_x);
        wrefresh(menu_win);
        echo();
        mvwgetstr(menu_win, y, input_x, username);
        noecho();
        trim_newline(username);
        y++;
        if (strcmp(username, "") == 0) {
            mvwprintw(menu_win, y, (width - strlen("Username cannot be empty.")) / 2, "Username cannot be empty.");
            wrefresh(menu_win);
            wgetch(menu_win);
            continue;
        }

        // Get email
        mvwprintw(menu_win, y, (width - strlen("Email: ")) / 2 - 15, "Email: ");
        input_x = (width - strlen("Email: ")) / 2 - 15 + strlen("Email: ");
        wmove(menu_win, y, input_x);
        wrefresh(menu_win);
        echo();
        mvwgetstr(menu_win, y, input_x, email);
        noecho();
        trim_newline(email);
        y++;

        if (strcmp(email, "") == 0) {
            mvwprintw(menu_win, y, (width - strlen("Email cannot be empty.")) / 2, "Email cannot be empty.");
            wrefresh(menu_win);
            wgetch(menu_win);
            continue;
        }

        // Check credentials in file
        FILE *file = fopen(FILE_NAME, "r");
        if (file == NULL) {
            mvwprintw(menu_win, y, (width - strlen("No accounts found.")) / 2, "No accounts found.");
            wrefresh(menu_win);
            wgetch(menu_win);
            continue;
        }

        char line[200];
        char stored_username[50];
        char stored_email[100];
        bool match_found = false;

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "Username: %49s", stored_username) == 1) {
            trim_newline(stored_username);
            if (strcmp(stored_username, username) == 0) {
                if (fgets(line, sizeof(line), file)) {
                    if (fgets(line, sizeof(line), file)) {
                        if (sscanf(line, "Email: %99s", stored_email) == 1) {
                            trim_newline(stored_email);
                            if (strcmp(stored_email, email) == 0) {
                                match_found = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    fclose(file);
        if (!match_found) {
            mvwprintw(menu_win, y, (width - strlen("Username or Email not found.")) / 2, "Username or Email not found.");
            wrefresh(menu_win);
            wgetch(menu_win);
            continue;
        }
        mvwprintw(menu_win, y, (width - strlen("Enter New Password: ")) / 2 - 15, "Enter New Password: ");
        input_x = (width - strlen("Enter New Password: ")) / 2 - 15 + strlen("Enter New Password: ");
        wmove(menu_win, y, input_x);
        wrefresh(menu_win);
        echo();
        mvwgetstr(menu_win, y, input_x, new_password);
        noecho();
        trim_newline(new_password);

        if (!validate_password(new_password)) {
            mvwprintw(menu_win, y + 1, (width - strlen("Invalid Password (min 7 chars, 1 upper, 1 lower, 1 digit).")) / 2, "Invalid Password (min 7 chars, 1 upper, 1 lower, 1 digit).");
            wrefresh(menu_win);
            wgetch(menu_win);
            continue;
        }

        // Save new password
        FILE *temp_file = fopen("temp_user_data.txt", "w");
        file = fopen(FILE_NAME, "r");
        if (file == NULL || temp_file == NULL) {
            mvwprintw(menu_win, y, (width - strlen("Error updating password.")) / 2, "Error updating password.");
            wrefresh(menu_win);
            wgetch(menu_win);
            continue;
        }

        bool password_updated = false;
        while (fgets(line, sizeof(line), file)) {
            fputs(line, temp_file);
            if (sscanf(line, "Username: %49s", stored_username) == 1) {
                if (strcmp(stored_username, username) == 0) {
                    fgets(line, sizeof(line), file); // Skip old password
                    fprintf(temp_file, "Password: %s\n", new_password);
                    password_updated = true;
                }
            }
        }
        fclose(file);
        fclose(temp_file);
        rename("temp_user_data.txt", FILE_NAME);

        if (password_updated) {
            mvwprintw(menu_win, y + 1, (width - strlen("Password updated successfully!")) / 2, "Password updated successfully!");
            strcpy(current_user, username);
            strcpy(current_email, email);
            wrefresh(menu_win);
            wgetch(menu_win);
            return;
        } else {
            mvwprintw(menu_win, y + 1, (width - strlen("Error updating password.")) / 2, "Error updating password.");
            wrefresh(menu_win);
            wgetch(menu_win);
        }
    }
}

void display_game_menu(WINDOW *menu_win, int highlight) {
    int height, width;
    getmaxyx(stdscr, height, width);
    int x, y = 1;
    int max_len = 0;

    for (int i = 0; i < GAME_MENU_SIZE; i++) {
        int len = strlen(game_menu[i]);
        if (len > max_len) max_len = len;
    }

    x = (width - max_len) / 2;
    werase(menu_win);
    wattron(menu_win, COLOR_PAIR(1));
    mvwprintw(menu_win, 0, 0, "╔");
    for (int i = 1; i < width - 1; i++) {
        mvwprintw(menu_win, 0, i, "═");
    }
    mvwprintw(menu_win, 0, width - 1, "╗");
    for (int i = 1; i < height - 1; i++) {
        mvwprintw(menu_win, i, 0, "║");
        mvwprintw(menu_win, i, width - 1, "║");
    }
    mvwprintw(menu_win, height - 1, 0, "╚");
    for (int i = 1; i < width - 1; i++) {
        mvwprintw(menu_win, height - 1, i, "═");
    }
    mvwprintw(menu_win, height - 1, width - 1, "╝");
    wattroff(menu_win, COLOR_PAIR(1));
    mvwprintw(menu_win, 1, 2, "User: ");
    wattron(menu_win, COLOR_PAIR(4) | A_BOLD);
    wprintw(menu_win, "%s", current_user);
    wattroff(menu_win, COLOR_PAIR(4) | A_BOLD);

    mvwprintw(menu_win, 2, 2, "Difficulty: ");
    wattron(menu_win, COLOR_PAIR(5) | A_BOLD);
    wprintw(menu_win, "%s", current_difficulty);
    wattroff(menu_win, COLOR_PAIR(5) | A_BOLD);
    for (int i = 0; i < GAME_MENU_SIZE; i++) {
        if (highlight == i + 1) {
            wattron(menu_win, A_REVERSE);
            mvwprintw(menu_win, y, x, "%s", game_menu[i]);
            wattroff(menu_win, A_REVERSE);
        } else {
            mvwprintw(menu_win, y, x, "%s", game_menu[i]);
        }
        y++;
    }
    wrefresh(menu_win);
}
void update_current_difficulty() {
    int diff = get_current_difficulty();
    switch(diff) {
        case 1:
            strcpy(current_difficulty, "Easy");
            break;
        case 2:
            strcpy(current_difficulty, "Normal");
            break;
        case 3:
            strcpy(current_difficulty, "Hard");
            break;
        default:
            strcpy(current_difficulty, "Normal");
    }
}
void save_difficulty(int difficulty) {
    FILE *file = fopen(SETTINGS_FILE, "w");
    if (file != NULL) {
        fprintf(file, "Difficulty: %d\n", difficulty);
        fclose(file);
    }
    update_current_difficulty();
}
int get_current_difficulty() {
    FILE *file = fopen(SETTINGS_FILE, "r");
    int difficulty = 2; // Default to Normal
    if (file != NULL) {
        char line[100];
        if (fgets(line, sizeof(line), file)) {
            sscanf(line, "Difficulty: %d", &difficulty);
        }
        fclose(file);
    }
    return difficulty;
}
void show_game_settings(WINDOW *menu_win) {
    int height, width;
    getmaxyx(stdscr, height, width);
    int current_choice = get_current_difficulty();
    const char *difficulty_names[] = {"Easy", "Normal", "Hard"};
    
    while (1) {
        werase(menu_win);
        werase(menu_win);
        wattron(menu_win, COLOR_PAIR(1));
        mvwprintw(menu_win, 0, 0, "╔");
        for (int i = 1; i < width - 1; i++) {
            mvwprintw(menu_win, 0, i, "═");
        }
        mvwprintw(menu_win, 0, width - 1, "╗");
        for (int i = 1; i < height - 1; i++) {
            mvwprintw(menu_win, i, 0, "║");
            mvwprintw(menu_win, i, width - 1, "║");
        }
        mvwprintw(menu_win, height - 1, 0, "╚");
        for (int i = 1; i < width - 1; i++) {
            mvwprintw(menu_win, height - 1, i, "═");
        }
        mvwprintw(menu_win, height - 1, width - 1, "╝");
        wattroff(menu_win, COLOR_PAIR(1));
        
        // Title
        mvwprintw(menu_win, 1, (width - strlen("Game Settings")) / 2, "Game Settings");
        
        // Difficulty options
        mvwprintw(menu_win, 3, (width - strlen("Difficulty Level")) / 2, "Difficulty Level");
        
        for (int i = 0; i < DIFFICULTY_OPTIONS; i++) {
            if (i + 1 == current_choice) {
                wattron(menu_win, A_REVERSE);
            }
            mvwprintw(menu_win, 5 + i, (width - strlen(difficulty_names[i])) / 2, "%s", difficulty_names[i]);
            if (i + 1 == current_choice) {
                wattroff(menu_win, A_REVERSE);
            }
        }
        
        // Instructions
        mvwprintw(menu_win, height - 3, 2, "↑/↓: Select  Enter: Confirm  ESC: Back");
        
        wrefresh(menu_win);
        
        int c = wgetch(menu_win);
        switch (c) {
            case KEY_UP:
                current_choice = (current_choice == 1) ? DIFFICULTY_OPTIONS : current_choice - 1;
                break;
            case KEY_DOWN:
                current_choice = (current_choice == DIFFICULTY_OPTIONS) ? 1 : current_choice + 1;
                break;
            case 10: // Enter key
                save_difficulty(current_choice);
                mvwprintw(menu_win, height - 2, 2, "Difficulty saved!");
                wrefresh(menu_win);
                napms(1000); // Show message for 1 second
                return;
            case 27: // Escape key
                return;
        }
    }
}
void showGameMenu(WINDOW *menu_win) {
    int highlight = 1;
    int choice = 0;
    int c;
    int height, width;
    getmaxyx(stdscr, height, width);
    wresize(menu_win, height, width);

    while (1) {
        werase(menu_win);
        werase(menu_win);
        wattron(menu_win, COLOR_PAIR(1));
        mvwprintw(menu_win, 0, 0, "╔");
        for (int i = 1; i < width - 1; i++) {
            mvwprintw(menu_win, 0, i, "═");
        }
        mvwprintw(menu_win, 0, width - 1, "╗");
        for (int i = 1; i < height - 1; i++) {
            mvwprintw(menu_win, i, 0, "║");
            mvwprintw(menu_win, i, width - 1, "║");
        }
        mvwprintw(menu_win, height - 1, 0, "╚");
        for (int i = 1; i < width - 1; i++) {
            mvwprintw(menu_win, height - 1, i, "═");
        }
        mvwprintw(menu_win, height - 1, width - 1, "╝");
        wattroff(menu_win, COLOR_PAIR(1));
        display_game_menu(menu_win, highlight);
        c = wgetch(menu_win);

        switch (c) {
            case KEY_UP:
                highlight = (highlight == 1) ? GAME_MENU_SIZE : highlight - 1;
                break;
            case KEY_DOWN:
                highlight = (highlight == GAME_MENU_SIZE) ? 1 : highlight + 1;
                break;
            case 10:
                choice = highlight;
                break;
            case 27:
                return; // Exit game menu and return to main menu
        }

        if (choice != 0) {
            switch (choice) {
                case 1: // New Game
                {
                    clear();
                    refresh();
                    endwin();
                    system("clear");
                    remove("savegame.json");  // Remove existing save
                    char *args[] = {"./Map", NULL};
                    execv("./Map", args);
                    fprintf(stderr, "Failed to start new game\n");
                    exit(1);
                }
                break;

                case 2: // Resume Game
                {
                    // First check if save file exists
                    FILE *test = fopen("savegame.json", "r");
                    if (test == NULL) {
                        mvprintw(LINES/2, (COLS-30)/2, "No saved game found!");
                        refresh();
                        napms(2000);  // Show message for 2 seconds
                        break;
                    }
                    fclose(test);
                    
                    clear();
                    refresh();
                    endwin();
                    system("clear");
                    
                    // Start Map with a special flag to indicate it should wait for 'L'
                    char *args[] = {"./Map", "resume_wait", NULL};
                    execv("./Map", args);
                    
                    // In case execv fails
                    fprintf(stderr, "Failed to resume game\n");
                    exit(1);
                }
                break;
                case 3:
                    // Game Settings logic
                    show_game_settings(menu_win);
                    break;
                case 4:
                    show_character_color_settings(menu_win);
                    break;
                case 5:
                    show_music_settings(menu_win);
                    break;
                case 6:
                    return; // Exit to main menu
            }
            choice = 0;
            werase(menu_win);
            box(menu_win, 0, 0);
        }
    }
}
int get_difficulty() {
    FILE *file = fopen("game_settings.txt", "r");
    int difficulty = 2; // Default to Normal
    if (file != NULL) {
        char line[100];
        if (fgets(line, sizeof(line), file)) {
            sscanf(line, "Difficulty: %d", &difficulty);
        }
        fclose(file);
    }
    
    return difficulty;
}
void show_character_color_settings(WINDOW *menu_win) {
    int height, width;
    getmaxyx(stdscr, height, width);
    int current_choice = get_character_color();
    
    // Color pairs and names using your existing init_pairs
    const int color_pairs[] = {1, 2, 3, 4, 5};
    const char *color_names[] = {
        "Yellow",
        "White",
        "Red",
        "Green",
        "Cyan"
    };

    while (1) {
        werase(menu_win);
        werase(menu_win);
        wattron(menu_win, COLOR_PAIR(1));
        mvwprintw(menu_win, 0, 0, "╔");
        for (int i = 1; i < width - 1; i++) {
            mvwprintw(menu_win, 0, i, "═");
        }
        mvwprintw(menu_win, 0, width - 1, "╗");
        for (int i = 1; i < height - 1; i++) {
            mvwprintw(menu_win, i, 0, "║");
            mvwprintw(menu_win, i, width - 1, "║");
        }
        mvwprintw(menu_win, height - 1, 0, "╚");
        for (int i = 1; i < width - 1; i++) {
            mvwprintw(menu_win, height - 1, i, "═");
        }
        mvwprintw(menu_win, height - 1, width - 1, "╝");
        wattroff(menu_win, COLOR_PAIR(1));
        
        // Title
        mvwprintw(menu_win, 1, (width - strlen("Character Color Settings")) / 2, "Character Color Settings");
        
        // Preview current character
        mvwprintw(menu_win, 3, (width - strlen("Preview: ")) / 2, "Preview: ");
        wattron(menu_win, COLOR_PAIR(current_choice));
        mvwaddch(menu_win, 3, (width + strlen("Preview: ")) / 2, '@');
        wattroff(menu_win, COLOR_PAIR(current_choice));

        // Color options
        for (int i = 0; i < COLOR_OPTIONS; i++) {
            if (color_pairs[i] == current_choice) {
                wattron(menu_win, A_REVERSE);
            }
            
            // Display color name and preview
            mvwprintw(menu_win, 5 + i, (width - 20) / 2, "%s ", color_names[i]);
            wattron(menu_win, COLOR_PAIR(color_pairs[i]));
            mvwaddch(menu_win, 5 + i, (width + 20) / 2, '@');
            wattroff(menu_win, COLOR_PAIR(color_pairs[i]));
            
            if (color_pairs[i] == current_choice) {
                wattroff(menu_win, A_REVERSE);
            }
        }
        
        // Instructions
        mvwprintw(menu_win, height - 3, 2, "↑/↓: Select  Enter: Confirm  ESC: Back");
        
        wrefresh(menu_win);
        
        int c = wgetch(menu_win);
        switch (c) {
            case KEY_UP:
                for (int i = 0; i < COLOR_OPTIONS; i++) {
                    if (color_pairs[i] == current_choice) {
                        current_choice = color_pairs[(i == 0) ? COLOR_OPTIONS - 1 : i - 1];
                        break;
                    }
                }
                break;
            case KEY_DOWN:
                for (int i = 0; i < COLOR_OPTIONS; i++) {
                    if (color_pairs[i] == current_choice) {
                        current_choice = color_pairs[(i == COLOR_OPTIONS - 1) ? 0 : i + 1];
                        break;
                    }
                }
                break;
            case 10: // Enter key
                save_character_color(current_choice);
                mvwprintw(menu_win, height - 2, 2, "Color saved!");
                wrefresh(menu_win);
                napms(1000); // Show message for 1 second
                return;
            case 27: // Escape key
                return;
        }
    }
}
void save_character_color(int color_pair) {
    FILE *file = fopen(SETTINGS_FILE, "r");
    int difficulty = 2; // Default
    if (file != NULL) {
        char line[100];
        if (fgets(line, sizeof(line), file)) {
            sscanf(line, "Difficulty: %d", &difficulty);
        }
        fclose(file);
    }

    // Save both settings
    file = fopen(SETTINGS_FILE, "w");
    if (file != NULL) {
        fprintf(file, "Difficulty: %d\n", difficulty);
        fprintf(file, "CharacterColor: %d\n", color_pair);
        fclose(file);
    }
}

int get_character_color() {
    FILE *file = fopen(SETTINGS_FILE, "r");
    int color_pair = 2; // Default to white (pair 2)
    if (file != NULL) {
        char line[100];
        // Skip difficulty line
        fgets(line, sizeof(line), file);
        // Read color if it exists
        if (fgets(line, sizeof(line), file)) {
            sscanf(line, "CharacterColor: %d", &color_pair);
        }
        fclose(file);
    }
    return color_pair;
}
void showProfileMenu(WINDOW *menu_win) {
    int height, width;
    getmaxyx(stdscr, height, width);

    // Calculate box dimensions for centering
    int box_width = 60;  // Fixed width for our content boxes
    int box_start_x = (width - box_width) / 2;  // Center horizontally
    int y = height / 4;  // Start from 1/4 of screen height

    // Main window box
    werase(menu_win);
    wattron(menu_win, COLOR_PAIR(5));
    box(menu_win, 0, 0);
    mvwhline(menu_win, 2, 1, ACS_HLINE, width - 2);  // Separator line under title
    wattroff(menu_win, COLOR_PAIR(5));

    // Title centered at top
    wattron(menu_win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(menu_win, 1, (width - strlen("Profile Menu")) / 2, "Profile Menu");
    wattroff(menu_win, COLOR_PAIR(2) | A_BOLD);

    if (strcmp(current_user, "") == 0) {
        mvwprintw(menu_win, y, (width - strlen("Please log in to view your profile.")) / 2, 
                  "Please log in to view your profile.");
        wrefresh(menu_win);
        wgetch(menu_win);
        return;
    }
    werase(menu_win);
    wattron(menu_win, COLOR_PAIR(1));
    mvwprintw(menu_win, 0, 0, "╔");
    for (int i = 1; i < width - 1; i++) {
        mvwprintw(menu_win, 0, i, "═");
    }
    mvwprintw(menu_win, 0, width - 1, "╗");
    for (int i = 1; i < height - 1; i++) {
        mvwprintw(menu_win, i, 0, "║");
        mvwprintw(menu_win, i, width - 1, "║");
    }
    mvwprintw(menu_win, height - 1, 0, "╚");
    for (int i = 1; i < width - 1; i++) {
        mvwprintw(menu_win, height - 1, i, "═");
    }
    mvwprintw(menu_win, height - 1, width - 1, "╝");
    wattroff(menu_win, COLOR_PAIR(1));

    // Title with fancy border
    wattron(menu_win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(menu_win, 1, (width - strlen("Profile Menu")) / 2, "Profile Menu");
    wattroff(menu_win, COLOR_PAIR(2) | A_BOLD);

    if (strcmp(current_user, "") == 0) {
        mvwprintw(menu_win, y, (width - strlen("Please log in to view your profile.")) / 2, 
                  "Please log in to view your profile.");
        wrefresh(menu_win);
        wgetch(menu_win);
        return;
    }
    // Read user data from file
    FILE *user_data = fopen("user_data.txt", "r");
    char username[100] = "", password[100] = "", email[100] = "";
    char line[256];
    int found_user = 0;
    
    if (user_data) {
        while (fgets(line, sizeof(line), user_data)) {
            if (strncmp(line, "Username: ", 10) == 0) {
                char temp_user[100];
                strcpy(temp_user, line + 10);
                temp_user[strcspn(temp_user, "\n")] = 0;
                
                // If this is our user, get their data
                if (strcmp(temp_user, current_user) == 0) {
                    strcpy(username, temp_user);
                    
                    // Get password
                    if (fgets(line, sizeof(line), user_data)) {
                        if (strncmp(line, "Password: ", 10) == 0) {
                            strcpy(password, line + 10);
                            password[strcspn(password, "\n")] = 0;
                        }
                    }
                    
                    // Get email
                    if (fgets(line, sizeof(line), user_data)) {
                        if (strncmp(line, "Email: ", 7) == 0) {
                            strcpy(email, line + 7);
                            email[strcspn(email, "\n")] = 0;
                        }
                    }
                    
                    found_user = 1;
                    break;
                }
            }
        }
        fclose(user_data);
    }

    // Read user score from file
    FILE *user_score = fopen("user_score.txt", "r");
    int level = 0, hit = 0, strength = 0, gold = 0, armor = 0, exp = 0, games = 0;
    int found_score = 0;
    
    if (user_score) {
        while (fgets(line, sizeof(line), user_score)) {
            if (strncmp(line, "Username: ", 10) == 0) {
                char temp_user[100];
                strcpy(temp_user, line + 10);
                temp_user[strcspn(temp_user, "\n")] = 0;
                
                // If this is our user, get their stats
                if (strcmp(temp_user, current_user) == 0) {
                    while (fgets(line, sizeof(line), user_score)) {
                        if (line[0] == '\n' || strncmp(line, "Username:", 9) == 0) break;
                        
                        if (strstr(line, "Level: ")) sscanf(line, "Level: %d", &level);
                        else if (strstr(line, "Hit: ")) sscanf(line, "Hit: %d", &hit);
                        else if (strstr(line, "Strength: ")) sscanf(line, "Strength: %d", &strength);
                        else if (strstr(line, "Gold: ")) sscanf(line, "Gold: %d", &gold);
                        else if (strstr(line, "Armor: ")) sscanf(line, "Armor: %d", &armor);
                        else if (strstr(line, "Exp: ")) sscanf(line, "Exp: %d", &exp);
                        else if (strstr(line, "Games Played: ")) sscanf(line, "Games Played: %d", &games);
                    }
                    found_score = 1;
                    break;
                }
            }
        }
        fclose(user_score);
    }

    wattron(menu_win, COLOR_PAIR(3));
    mvwprintw(menu_win, y, box_start_x, "╔════════════ User Information ════════════╗");
    for(int i = 1; i <= 4; i++) {
        mvwprintw(menu_win, y+i, box_start_x, "║");
    }
    mvwprintw(menu_win, y+4, box_start_x, "╚══════════════════════════════════════════╝");
    wattroff(menu_win, COLOR_PAIR(3));

    // User info content
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, y+1, box_start_x + 2, "Username  : ");
    wattron(menu_win, COLOR_PAIR(1) | A_BOLD);
    wprintw(menu_win, "%s", username);
    
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, y+2, box_start_x + 2, "Email     : ");
    wattron(menu_win, COLOR_PAIR(1) | A_BOLD);
    wprintw(menu_win, "%s", email);
    
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, y+3, box_start_x + 2, "Password  : ");
    wattron(menu_win, COLOR_PAIR(1) | A_BOLD);
    char masked_pass[100] = "";
    for (int i = 0; i < strlen(password); i++) masked_pass[i] = '*';
    wprintw(menu_win, "%s", masked_pass);
    wattroff(menu_win, COLOR_PAIR(1) | A_BOLD);

    // Game Statistics Box
    y += 6;  // Space between boxes
    wattron(menu_win, COLOR_PAIR(4));
    mvwprintw(menu_win, y, box_start_x, "╔════════════ Game Statistics ════════════╗");
    for(int i = 1; i <= 8; i++) {
        mvwprintw(menu_win, y+i, box_start_x, "║");
    }
    mvwprintw(menu_win, y+8, box_start_x, "╚══════════════════════════════════════════╝");
    wattroff(menu_win, COLOR_PAIR(4));

    // Stats content
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, y+1, box_start_x + 2, "⚔  Level        : ");
    wattron(menu_win, COLOR_PAIR(1) | A_BOLD);
    wprintw(menu_win, "%d", level);
    
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, y+2, box_start_x + 2, "♥  Hit Points   : ");
    wattron(menu_win, COLOR_PAIR(1) | A_BOLD);
    wprintw(menu_win, "%d", hit);
    
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, y+3, box_start_x + 2, "⚡ Strength     : ");
    wattron(menu_win, COLOR_PAIR(1) | A_BOLD);
    wprintw(menu_win, "%d", strength);
    
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, y+4, box_start_x + 2, "$ Gold         : ");
    wattron(menu_win, COLOR_PAIR(1) | A_BOLD);
    wprintw(menu_win, "%d", gold);
    
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, y+5, box_start_x + 2, "🛡  Armor        : ");
    wattron(menu_win, COLOR_PAIR(1) | A_BOLD);
    wprintw(menu_win, "%d", armor);
    
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, y+6, box_start_x + 2, "✨ Experience   : ");
    wattron(menu_win, COLOR_PAIR(1) | A_BOLD);
    wprintw(menu_win, "%d", exp);
    
    wattron(menu_win, COLOR_PAIR(2));
    mvwprintw(menu_win, y+7, box_start_x + 2, "🎮 Games Played : ");
    wattron(menu_win, COLOR_PAIR(1) | A_BOLD);
    wprintw(menu_win, "%d", games);
    wattroff(menu_win, COLOR_PAIR(1) | A_BOLD);

    // Current date/time and login info at bottom
    y = height - 4;
    wattron(menu_win, COLOR_PAIR(5));
    mvwprintw(menu_win, y, (width - strlen("Current User:")) / 2 - 5, "Current User: %s", current_user);
    mvwprintw(menu_win, y+1, (width - strlen("Date/Time:")) / 2 - 10, "Date/Time: %s", "2025-02-05 11:00:00");
    wattroff(menu_win, COLOR_PAIR(5));

    // Bottom instruction
    wattron(menu_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(menu_win, height-2, (width - strlen("Press any key to return")) / 2, 
              "Press any key to return");
    wattroff(menu_win, COLOR_PAIR(3) | A_BOLD);

    wrefresh(menu_win);
    wgetch(menu_win);
}
void stop_current_music() {
    system("pkill mpg123 2>/dev/null");
    usleep(100000);
}
void play_music(const char* filename) {
    char command[256];
    stop_current_music();
    snprintf(command, sizeof(command), 
            "mpg123 -q --buffer 1024 -C --loop -1 --quiet -o pulse \"%s/%s\" > /dev/null 2>&1 &", 
            MUSIC_FOLDER, filename);
    
    if (system(command) != 0) {
        // If PulseAudio fails, try with default output
        snprintf(command, sizeof(command), 
                "mpg123 -q --buffer 1024 -C --loop -1 --quiet \"%s/%s\" > /dev/null 2>&1 &", 
                MUSIC_FOLDER, filename);
        system(command);
    }
}
int check_music_system() {
    int status = system("which mpg123 > /dev/null 2>&1");
    if (status != 0) {
        return 0; // mpg123 not found
    }
    return 1; // mpg123 is available
}
void get_music_files(MusicTrack *tracks, int *count) {
    DIR *dir;
    struct dirent *ent;
    *count = 0;

    dir = opendir(MUSIC_FOLDER);
    if (dir == NULL) {
        return;
    }

    while ((ent = readdir(dir)) != NULL && *count < MAX_MUSIC_FILES) {
        if (strstr(ent->d_name, ".mp3") || strstr(ent->d_name, ".MP3")) {
            strcpy(tracks[*count].filename, ent->d_name);
            // Create display name (remove .mp3 extension)
            strncpy(tracks[*count].display_name, ent->d_name, strlen(ent->d_name) - 4);
            tracks[*count].display_name[strlen(ent->d_name) - 4] = '\0';
            tracks[*count].is_playing = 0;
            (*count)++;
        }
    }
    closedir(dir);
}

void save_music_settings(int enabled, const char* current_track) {
    FILE *file = fopen(SETTINGS_FILE, "r");
    int difficulty = 2;
    int char_color = 2;
    
    // Read existing settings
    if (file != NULL) {
        char line[100];
        if (fgets(line, sizeof(line), file)) {
            sscanf(line, "Difficulty: %d", &difficulty);
        }
        if (fgets(line, sizeof(line), file)) {
            sscanf(line, "CharacterColor: %d", &char_color);
        }
        fclose(file);
    }

    // Save all settings
    file = fopen(SETTINGS_FILE, "w");
    if (file != NULL) {
        fprintf(file, "Difficulty: %d\n", difficulty);
        fprintf(file, "CharacterColor: %d\n", char_color);
        fprintf(file, "MusicEnabled: %d\n", enabled);
        fprintf(file, "CurrentMusic: %s\n", current_track);
        fclose(file);
    }
}

void load_music_settings() {
    FILE *file = fopen(SETTINGS_FILE, "r");
    if (file != NULL) {
        char line[100];
        // Skip difficulty and color lines
        fgets(line, sizeof(line), file);
        fgets(line, sizeof(line), file);
        
        // Read music settings
        if (fgets(line, sizeof(line), file)) {
            sscanf(line, "MusicEnabled: %d", &music_enabled);
        }
        if (fgets(line, sizeof(line), file)) {
            char music_name[MAX_FILENAME_LENGTH];
            sscanf(line, "CurrentMusic: %s", music_name);
            strcpy(current_music, music_name);
        }
        fclose(file);
    }
}

void show_music_settings(WINDOW *menu_win) {
    MusicTrack tracks[MAX_MUSIC_FILES];
    int track_count = 0;
    int current_choice = 0;
    int height, width;
    
    get_music_files(tracks, &track_count);
    if (track_count == 0) {
        mvwprintw(menu_win, LINES/2, (COLS-strlen("No music files found in Music folder"))/2,
                  "No music files found in Music folder");
        wrefresh(menu_win);
        wgetch(menu_win);
        return;
    }

    while (1) {
        getmaxyx(stdscr, height, width);
        werase(menu_win);
        wattron(menu_win, COLOR_PAIR(1));
        mvwprintw(menu_win, 0, 0, "╔");
        for (int i = 1; i < width - 1; i++) {
            mvwprintw(menu_win, 0, i, "═");
        }
        mvwprintw(menu_win, 0, width - 1, "╗");
        for (int i = 1; i < height - 1; i++) {
            mvwprintw(menu_win, i, 0, "║");
            mvwprintw(menu_win, i, width - 1, "║");
        }
        mvwprintw(menu_win, height - 1, 0, "╚");
        for (int i = 1; i < width - 1; i++) {
            mvwprintw(menu_win, height - 1, i, "═");
        }
        mvwprintw(menu_win, height - 1, width - 1, "╝");
        wattroff(menu_win, COLOR_PAIR(1));

        // Title
        mvwprintw(menu_win, 1, (width - strlen("Music Settings")) / 2, "Music Settings");
        
        // Music Status
        wattron(menu_win, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(menu_win, 3, 2, "Music: %s", music_enabled ? "ON" : "OFF");
        wattroff(menu_win, COLOR_PAIR(4) | A_BOLD);
        
        // Current Track
        mvwprintw(menu_win, 4, 2, "Current Track: ");
        wattron(menu_win, COLOR_PAIR(5) | A_BOLD);
        wprintw(menu_win, "%s", current_music);
        wattroff(menu_win, COLOR_PAIR(5) | A_BOLD);

        // Display tracks
        mvwprintw(menu_win, 6, (width - strlen("Available Tracks:")) / 2, "Available Tracks:");
        for (int i = 0; i < track_count; i++) {
            if (i == current_choice) {
                wattron(menu_win, A_REVERSE);
            }
            mvwprintw(menu_win, 8 + i, (width - strlen(tracks[i].display_name)) / 2, 
                     "%s", tracks[i].display_name);
            if (i == current_choice) {
                wattroff(menu_win, A_REVERSE);
            }
        }

        // Instructions
        mvwprintw(menu_win, height - 3, 2, "↑/↓: Select  Enter: Play/Stop  Space: Toggle Music  ESC: Back");
        
        wrefresh(menu_win);
        
        int c = wgetch(menu_win);
        switch (c) {
            case KEY_UP:
                current_choice = (current_choice == 0) ? track_count - 1 : current_choice - 1;
                break;
                
            case KEY_DOWN:
                current_choice = (current_choice == track_count - 1) ? 0 : current_choice + 1;
                break;
                
            case ' ': // Toggle music on/off
                music_enabled = !music_enabled;
                if (!music_enabled) {
                    stop_current_music();
                } else if (strcmp(current_music, "None") != 0) {
                    play_music(current_music);
                }
                save_music_settings(music_enabled, current_music);
                break;
                
            case 10: // Enter key
                if (music_enabled) {
                    strcpy(current_music, tracks[current_choice].filename);
                    play_music(current_music);
                    save_music_settings(music_enabled, current_music);
                }
                break;
                
            case 27: // Escape key
                return;
        }
    }
}
void init_leaderboard_colors() {
    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK); 
    init_pair(2, COLOR_WHITE, COLOR_BLACK);   
    init_pair(3, COLOR_RED, COLOR_BLACK);    
    init_pair(4, COLOR_GREEN, COLOR_BLACK); 
    init_pair(5, COLOR_CYAN, COLOR_BLACK);  
}

#define ENTRIES_PER_PAGE 10

void showLeaderboardMenu(WINDOW *menu_win) {
    int height, width;
    getmaxyx(stdscr, height, width);
    int current_page = 0;
    int selected_index = 0;
    int total_pages;
    
    // Array to store user scores
    UserScore users[MAX_LEADERBOARD_ENTRIES];
    int user_count = 0;
    
    // Read all user scores from user_score.txt
    FILE *file = fopen("user_score.txt", "r");
    if (file != NULL) {
        char line[200];
        while (fgets(line, sizeof(line), file) && user_count < MAX_LEADERBOARD_ENTRIES) {
            if (strncmp(line, "Username: ", 10) == 0) {
                sscanf(line, "Username: %s", users[user_count].username);
                // Read all stats for this user
                fgets(line, sizeof(line), file); // Level
                sscanf(line, "Level: %d", &users[user_count].level);
                fgets(line, sizeof(line), file); // Hit
                sscanf(line, "Hit: %d", &users[user_count].hit);
                fgets(line, sizeof(line), file); // Strength
                sscanf(line, "Strength: %d", &users[user_count].strength);
                fgets(line, sizeof(line), file); // Gold
                sscanf(line, "Gold: %d", &users[user_count].gold);
                fgets(line, sizeof(line), file); // Armor
                sscanf(line, "Armor: %d", &users[user_count].armor);
                fgets(line, sizeof(line), file); // Exp
                sscanf(line, "Exp: %d", &users[user_count].exp);
                fgets(line, sizeof(line), file); // Games Played
                sscanf(line, "Games Played: %d", &users[user_count].games_played);
                fgets(line, sizeof(line), file); // Blank line
                user_count++;
            }
        }
        fclose(file);
    }
    
    // Sort users by experience (or gold, depending on your preference)
    for (int i = 0; i < user_count - 1; i++) {
        for (int j = 0; j < user_count - i - 1; j++) {
            if (users[j].exp < users[j + 1].exp) {
                UserScore temp = users[j];
                users[j] = users[j + 1];
                users[j + 1] = temp;
            }
        }
    }

    total_pages = (user_count + ENTRIES_PER_PAGE - 1) / ENTRIES_PER_PAGE;

    while (1) {
        werase(menu_win);
        wattron(menu_win, COLOR_PAIR(1));
        mvwprintw(menu_win, 0, 0, "╔");
        for (int i = 1; i < width - 1; i++) {
            mvwprintw(menu_win, 0, i, "═");
        }
        mvwprintw(menu_win, 0, width - 1, "╗");
        for (int i = 1; i < height - 1; i++) {
            mvwprintw(menu_win, i, 0, "║");
            mvwprintw(menu_win, i, width - 1, "║");
        }
        mvwprintw(menu_win, height - 1, 0, "╚");
        for (int i = 1; i < width - 1; i++) {
            mvwprintw(menu_win, height - 1, i, "═");
        }
        mvwprintw(menu_win, height - 1, width - 1, "╝");
        wattroff(menu_win, COLOR_PAIR(1));
        mvwprintw(menu_win, 1, (width - strlen("LEADERBOARD")) / 2, "LEADERBOARD");
        mvwprintw(menu_win, 2, 2, "Logged in as: ");
        wattron(menu_win, COLOR_PAIR(4) | A_BOLD);
        wprintw(menu_win, "%s", current_user);
        wattroff(menu_win, COLOR_PAIR(4) | A_BOLD);

        // Column headers with proper spacing
        mvwprintw(menu_win, 4, 2, "Rank  Title      Player Name          Level  Gold    Exp     Games");
        mvwprintw(menu_win, 5, 2, "--------------------------------------------------------------------");

        int start_y = 6;
        int start_index = current_page * ENTRIES_PER_PAGE;
        int end_index = min(start_index + ENTRIES_PER_PAGE, user_count);

        for (int i = start_index; i < end_index; i++) {
            // Apply different styles based on rank
            if (i < 3) {
                wattron(menu_win, COLOR_PAIR(i + 1) | A_BOLD);
            } else if (strcmp(users[i].username, current_user) == 0) {
                wattron(menu_win, COLOR_PAIR(4) | A_BOLD);
            } else {
                wattron(menu_win, COLOR_PAIR(5));
            }

            // Assign titles and symbols based on rank
            const char *title;
            if (i == 0) title = "\U0001F451 Legend  ";
            else if (i == 1) title = "\U00002694 GOAT    ";
            else if (i == 2) title = "\U0001F6E1 Warrior ";
            else title = "Player   ";

            // Highlight selected row
            if (i == selected_index) {
                wattron(menu_win, A_REVERSE);
            }

            mvwprintw(menu_win, start_y + (i - start_index), 2,
                     "%-4d %-10s%-18s %-6d %-7d %-7d %d",
                     i + 1,
                     title,
                     users[i].username,
                     users[i].level,
                     users[i].gold,
                     users[i].exp,
                     users[i].games_played);

            if (i == selected_index) {
                wattroff(menu_win, A_REVERSE);
            }

            if (i < 3) {
                wattroff(menu_win, COLOR_PAIR(i + 1) | A_BOLD);
            } else if (strcmp(users[i].username, current_user) == 0) {
                wattroff(menu_win, COLOR_PAIR(4) | A_BOLD);
            } else {
                wattroff(menu_win, COLOR_PAIR(5));
            }
        }

        // Navigation info
        mvwprintw(menu_win, height - 4, 2, "Page %d/%d", current_page + 1, total_pages);
        mvwprintw(menu_win, height - 3, 2, "↑/↓: Navigate  ←/→: Change Page  ESC: Exit");

        // Refresh and handle input
        wrefresh(menu_win);

        int c = wgetch(menu_win);
        switch (c) {
            case KEY_UP:
                if (selected_index > 0) {
                    selected_index--;
                    if (selected_index < start_index) {
                        current_page--;
                    }
                }
                break;
            case KEY_DOWN:
                if (selected_index < user_count - 1) {
                    selected_index++;
                    if (selected_index >= end_index) {
                        current_page++;
                    }
                }
                break;
            case KEY_LEFT:
                if (current_page > 0) {
                    current_page--;
                    selected_index = current_page * ENTRIES_PER_PAGE;
                }
                break;
            case KEY_RIGHT:
                if (current_page < total_pages - 1) {
                    current_page++;
                    selected_index = current_page * ENTRIES_PER_PAGE;
                }
                break;
            case 27: // ESC
                return;
        }
    }
}

int min(int a, int b) {
    return (a < b) ? a : b;
}