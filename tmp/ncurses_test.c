#include <ncurses.h>
#include <dirent.h>
#include <string.h>

#define MAX_ITEMS 50

int get_scripts(const char *dir, char scripts[][256]){
    DIR *d = opendir(dir);
    if(!d) return 1;

    struct dirent *ent;
    int count = 0;
    while((ent = readdir(d)) != NULL && count < MAX_ITEMS){
        if(strstr(ent->d_name, ".js")){
            snprintf(scripts[count], 256, "%s/%s", dir, ent->d_name);
            count++;
        }
    }
    closedir(d);
    return count;
}

int menu(char items[][256], int count, const char *title){
    int highlight = 0;
    int choice = -1;
    int ch;

    while(1){
        clear();
        mvprintw(1, 2, "%s", title);
        for(int i=0; i<count; i++){
            if(i == highlight){
                attron(A_REVERSE);
            }
            
            mvprintw(i + 3, 4, "%s", items[i]);

            if(i == highlight){
                attroff(A_REVERSE);
            }
        }

        refresh();
        ch = getch();
        switch(ch){
            case KEY_UP:
                if(highlight > 0) highlight--;
                break;
            case KEY_DOWN:
                if(highlight < count - 1) highlight++;
                break;
            case 10:
                choice = highlight;
                return choice;
        }
    }
}

void manual_input(char *buffer, int size, const char *prompt){
    echo();
    clear();
    mvprintw(2, 2, "%s", prompt);
    mvprintw(4, 2, "> ");
    refresh();
    getnstr(buffer, size - 1);
    noecho();
}

int main(){
    char scripts[MAX_ITEMS][256];
    char selected_script[256];

    printf("\n\n---------- mobilka :3 ----------\n\n");

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    int script_count = get_scripts("/home/user0o1/frida", scripts);
    //get_scripts("/home/user0o1/frida", scripts);
    //size_t script_count = sizeof(scripts)/sizeof(scripts[0]);
    //int script_count = 4;
    script_count++;
    strcpy(scripts[script_count], "[*] manual input");
    script_count++;

    int script_choice = menu(scripts, script_count, "[*] please, choose frida script:");
    if(script_choice == script_count - 1){
        manual_input(selected_script, sizeof(selected_script), "enter script path");
    } else {
        strcpy(selected_script, scripts[script_choice]);
    }

    endwin();

    printf("\n\n[+] your choice: %s\n\n", selected_script);
    return 0;
}
