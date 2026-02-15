#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

int start_adb(){
    // create new child; if child: pid == 0; if parent: pid == <child_pid>; if fork failed: pid == <0;
    pid_t pid = fork();

    // child branch, since if child, then pid == 0
    if(pid == 0){
        printf("[*] child proc started\n");
        //execl("/usr/bin/adb", "adb", "start-server", NULL);
        execlp("adb", "adb", "start-server", NULL);
        perror("execlp failed");
        _exit(1);
    // parent branch, since if parent, then pid == <child_pid> and its >0
    } else if(pid > 0){
        printf("[*] parent proc waiting for child\n");
        int status;
        // waiting for child proccess
        waitpid(pid, &status, 0);
        printf("[+] child proc died\n");
        
        // confirming that child died without errors, since 0 == success
        if(WIFEXITED(status) && WEXITSTATUS(status) == 0){
            printf("[+] adb server started successfully\n");
            return 0;
        } else{
            printf("[x] adb server failed\n");
            return -1;
        }
    } else{
        perror("fork failed\n");
        return -1;
    }
}

int get_adb_devices(char devices[][128]){
    //system("adb start-server");
    //
    start_adb();
    printf("\n[*] looking for devices...\n");

    // create socket in order to connect to adb server
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5037);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0){
        printf("[+] successfully connected to the server\n");
    } else{
        printf("[x] error\n");
        close(sock);
        return -1;
    }
    
    //int out = send(sock, "000CHost:devices", 16, 0);
    //
    // dynamic format
    char cmd[] = "host:devices";
    char req[256];

    sprintf(req, "%04X%s", (int)strlen(cmd), cmd);
    send(sock, req, strlen(req), 0);
    
    // adb server's response format:
    //
    // <4 bytes status>        // "OKAY" or "FAIL"
    // [if FAIL → 4 hex len + error msg]
    // [if OKAY → 4 hex len + payload]
    //
    // example:
    //
    // OKAY
    // 000F (ASCII hex lenght)
    // <15 data bytes>
    // 
    // or:
    //
    // OKAY
    // 0022
    // 3f81ac39    device
    char status[5];
    recv(sock, status, 4, 0);
    printf("[*] ADB server response: %s\n", status);
    
    char hex_len[5];
    recv(sock, hex_len, 4, 0);
    int len = strtol(hex_len, NULL, 16);
    printf("[*] ADB server response: %s\n", hex_len);

    char *buffer = malloc(len + 1);
    recv(sock, buffer, len, 0);
    buffer[len] = '\0';
    printf("[+] found device: %s\n", buffer);

    //char *line = strtok(buffer, "\n");
    //int i = 0;
    /*while(line && i < 30){
        printf("[+] found device: %s\n", buffer[i]);
        i++;
    }*/

    free(buffer);
    close(sock);
    return 0;
}

int main(){
    char devices[30][128];

    printf("[+] started\n\n");
    get_adb_devices(devices);
    printf("\n[+] done\n");

    return 0;
}
