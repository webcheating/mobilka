#include <dirent.h>
#include <stdio.h>
#include <string.h>

int get_scripts(const char *dir, char scripts[][256]){
    DIR *d = opendir(dir);
    if(!d){
        printf("[x] no dir found\n");
        return 1;
    } else{
        printf("\n[+] found scripts:\n\n");
    }
    struct dirent *ent;
    int count = 0;
    while((ent = readdir(d)) != NULL){
        if(strstr(ent->d_name, ".js")){
            snprintf(scripts[count], 256, "%s/%s", dir, ent->d_name);
            printf("[*] %s\n", scripts[count]);
            count++;
        }
    }
    closedir(d);
    return count;
}

int main(){
    char scripts[30][256];

    get_scripts("/home/$USER/frida", scripts);
    return 0;
}
