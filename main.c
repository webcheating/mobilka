#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ncurses.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define MAX_DEVICES 5
#define MAX_ITEMS 30

char *exec_cmd(const char *cmd) {
    static char buffer[8192];
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;

    memset(buffer, 0, sizeof(buffer));
    fread(buffer, 1, sizeof(buffer) - 1, fp);
    pclose(fp);
    return buffer;
}

int get_installed_apps(char apps[][256]){
    char *app_list = exec_cmd("adb shell pm list packages -3");
    if(!app_list) return 0;

    int count = 0;
    char *line = strtok(app_list, "\n");
    while(line && count < MAX_ITEMS){
        if(strncmp(line, "package:", 8) == 0){
            strncpy(apps[count], line + 8, 255);
            count++;
        }
        line = strtok(NULL, "\n");
    }
    return count;
}

int get_scripts(const char *dir, char scripts[][256]){
    DIR *d = opendir(dir);
    if(!d) return -1;

    struct dirent *ent;
    int count = 0;
    while((ent = readdir(d)) != NULL && count < MAX_ITEMS){
        //if(strstr(ent->d_name, ".js")){
        char *dot = strrchr(ent->d_name, '.');
        if(dot && strcmp(dot, ".js") == 0){
            snprintf(scripts[count], 256, "%s/%s", dir, ent->d_name);
            count++;
        }
    }
    closedir(d);
    return count;
}

void get_local_ip(char *buf, int size) {
    char *out = exec_cmd(
        "ip -4 addr show scope global | grep inet | awk '{print $2}' | cut -d/ -f1"
    );

    if (!out || strlen(out) == 0) {
        strncpy(buf, "127.0.0.1", size - 1);
        return;
    }
    sscanf(out, "%63s", buf);
}
/*
int get_adb_devices(char devices[][128]){
    int sock = socket(AF_INET, SOCKSTREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5037);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    send(sock, "000CHost:devices", 16, 0);
}*/


int get_adb_devices(char devices[][128]) {
    char *out = exec_cmd("adb devices");
    if (!out) return 0;

    int count = 0;
    char *line = strtok(out, "\n");

    while (line && count < MAX_DEVICES) {
        if (strstr(line, "\tdevice")) {
            sscanf(line, "%127s", devices[count]);
            count++;
        }
        line = strtok(NULL, "\n");
    }
    return count;
}

int menu_select(char items[][256], int count, const char *title){
    int highlight = 0;
    int choice = -1;
    int ch;

    while(1){
        clear();
        mvprintw(0, 2, "%s", title);
        for(int i=0; i<count; i++){
            if(i == highlight){
                attron(A_REVERSE);
            }
            mvprintw(i+2, 4, "%s", items[i]);
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

int main() {
    char apps[MAX_ITEMS][256];
    char selected_app[256];
    char scripts[MAX_ITEMS][256];
    char selected_script[256];
    char devices[MAX_DEVICES][128];
    char device[128];
    char local_ip[64];
    char app[256];
    char script[256];
    int choice;
    
    /*
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    */

    printf("\n========== mobilka :33 ==========\n\n");

    int dev_count = get_adb_devices(devices);

    /*
    if (dev_count == 0) {
        printf("[!] No adb devices found\n");
        return 1;
    }*/
    printf("[*] looking for adb devices...\n");
    if (dev_count == 0){
        printf("[!] no adb devices found\n\n");
    }
    while (1) {
    dev_count = get_adb_devices(devices);
    
    
    if (dev_count > 0) {
    //    printf("\n[+] foud device\n\n");
        break;
    }

    printf("\r[*] waiting for device connection...");
    fflush(stdout);
    sleep(2);
}

    if (dev_count == 1) {
        strcpy(device, devices[0]);
        printf("\n[+] found device: %s\n", device);
    } else {
        printf("[!] multiple devices found:\n");
        for (int i = 0; i < dev_count; i++)
            printf(" [%d] %s\n", i, devices[i]);

        printf("\n[*] choose device: ");
        scanf("%d", &choice);

        if (choice < 0 || choice >= dev_count) {
            printf("[!] invalid choice\n");
            return 1;
        }
        strcpy(device, devices[choice]);
    }

    printf("[*] using device: %s\n\n", device);

    // get local IP
    //char *ip = exec_cmd("ip route get 1 | awk '{print $7}'");
    //sscanf(ip, "%63s", local_ip);
    get_local_ip(local_ip, sizeof(local_ip));

    printf("[+] found local IP: %s\n", local_ip);

    // -------------------- proxy -----------------------------
    char proxy_cmd[512];
    snprintf(proxy_cmd, sizeof(proxy_cmd), "adb -s %s shell settings put global http_proxy %s:8080", device, local_ip);

    system(proxy_cmd);
    printf("[+] proxy set\n\n");

    // ------------------- scrcpy -----------------------------
    char scrcpy_cmd[256];
    snprintf(scrcpy_cmd, sizeof(scrcpy_cmd), "scrcpy -s %s &", device);
    system(scrcpy_cmd);

    printf("[+] scrcpy started\n");

    // ------------------ frida-server -------------------------
    char frida_srv_cmd[512];
    snprintf(frida_srv_cmd, sizeof(frida_srv_cmd), "adb -s %s shell \"su -c '/data/local/tmp/frida/frida-server* &'\"", device);
    system(frida_srv_cmd);

    printf("[+] frida-server started\n\n");

    //// app
    //printf("\n[*] enter app package (e.g. com.example.app): ");
    //scanf("%255s", app);
    //// script
    //printf("[*] enter frida script path: ");
    //scanf("%255s", script);
    //
    // ---------------------------------------------------------------------------
    //
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    int app_count = get_installed_apps(apps);
    strcpy(apps[app_count], "[*] manual input");
    app_count++;
    
    int app_choice = menu_select(apps, app_count, "[*] choose an app to run:");
    if(app_choice == app_count - 1){
        manual_input(selected_app, sizeof(selected_app), "enter package name:");
    } else{
        strcpy(selected_app, apps[app_choice]);
    }

    int script_count = get_scripts("/home/user0o1/frida", scripts);
    strcpy(scripts[script_count], "[*] manual input");
    script_count++;

    int script_choice = menu_select(scripts, script_count, "[*] choose frida script:");
    if(script_choice == script_count - 1){
        manual_input(selected_script, sizeof(selected_script), "enter script path:");
    } else{
        strcpy(selected_script, scripts[script_choice]);
    }

    endwin();

    char frida_cmd[1024];
    snprintf(frida_cmd, sizeof(frida_cmd), "frida -U -f %s -l %s", selected_app, selected_script);

    printf("\n[+] running: \n%s\n\n", frida_cmd);
    system(frida_cmd);
    //snprintf(frida_cmd, sizeof(frida_cmd), "frida -U -f %s -l %s --no-pause", selected_app, selected_script);
    
    //snprintf(proxy_cmd, sizeof(proxy_cmd), "adb -s %s shell settings put global http_proxy :0", device, local_ip);
    //system(proxy_cmd);
    return 0;
}

