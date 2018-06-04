#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <stdbool.h>
#include <string.h>

#define BUFFER_SIZE        128
#define READ_END        0
#define WRITE_END        1

bool timeout = false;

void handler(int signal){
    if(signal != SIGALRM) exit(1);
    timeout = true;
    exit(0);
}

int main(){
    fd_set inputs, inputfds; //sets of file description
    char buffer[BUFFER_SIZE], message_in[BUFFER_SIZE];
    int fd[5][2], i, seed, result, message_num = 1;
    float cur_time = 0.0;
    pid_t pid;
    
    FILE* _fd;
    _fd = fopen("output.txt", "w");
    
    struct timeval start, cur;
    struct itimerval t;
    t.it_interval.tv_sec = 0;
    t.it_interval.tv_usec = 0;
    t.it_value.tv_sec = 30; //30 sec timer
    t.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &t, 0);
    gettimeofday(&start, NULL);
    
    signal(SIGALRM, handler);  /* set the Alarm signal capture */
    
    FD_ZERO(&inputfds); //init inputs to the empty
    FD_SET(0, &inputs); //set file descriptor 0
    
    //create 5 pipes
    for(i = 0; i < 5; i++){
        if(pipe(fd[i]) == -1) {
            fprintf(stderr, "pipe(), failed");
            exit(1);
        }
        FD_SET(fd[i][READ_END], &inputfds);
        pid = fork();
        
        seed = rand();
        if(pid == 0) {
            fflush(stdout);
            srand(seed);
            break;
        }
    }
    
    while(!timeout){
        inputs = inputfds;
        
        if(pid == 0){  //child write to pipe
            if(i < 4) { //child 1-4
                gettimeofday(&cur, NULL);
                cur_time = (float)(cur.tv_sec - start.tv_sec); //seconds
                cur_time += (float)(cur.tv_usec - start.tv_usec)/1000000.; //microseconds
                if (cur_time > 30) timeout = true;
                
                fflush(_fd);
                fprintf(_fd, "%2.3f Child %d message %d \n", cur_time, i+1, message_num++);
                
                close(fd[i][READ_END]);
                write(fd[i][WRITE_END], buffer, strlen(buffer)+1);
                sleep(rand() % 3);
                close(fd[i][WRITE_END]);
            }
            else { //child 5
                printf("User input message: \n");
                fgets(message_in, BUFFER_SIZE, stdin);
                
                gettimeofday(&cur, NULL);
                cur_time = (float)(cur.tv_sec - start.tv_sec);
                cur_time += (float)(cur.tv_usec - start.tv_usec)/1000000.;
                if (cur_time > 30) timeout = true;
                
                fflush(_fd);
                fprintf(_fd, "%2.3f Child 5 message %s\n", cur_time, message_in);
                
                close(fd[i][READ_END]);
                write(fd[i][WRITE_END], buffer, strlen(buffer)+1);
                close(fd[i][WRITE_END]);
            }
            
        }
        else{ // parent read from all the pipes
            result = select(FD_SETSIZE, &inputs, (fd_set *) 0, (fd_set *) 0, NULL);
            
            if (result == 0){ //timeout without input
                fflush(stdout);
                break;
            }
            if(result == -1) { //error
                perror("select");
                exit(1);
            }
            
            for(i = 0; i < 5; i++){
                if(FD_ISSET(fd[i][READ_END], &inputs)) {
                    close(fd[i][WRITE_END]);
                    read(fd[i][READ_END], buffer, BUFFER_SIZE);
                }
            }
        }
        
    }
    fclose(_fd);
    return 0;
}
