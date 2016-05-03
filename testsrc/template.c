#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>

#include <signal.h>  
#include <stdio.h>  
#include <unistd.h>  

void ouch(int sig)  
{  
    printf("\nOUCH! - I got signal %d\n", sig);  
    //恢复终端中断信号SIGINT的默认行为  
//    (void) signal(SIGINT, SIG_DFL);  
}  
          
int main()  
{  
    //改变终端中断信号SIGINT的默认行为，使之执行ouch函数  
    //而不是终止程序的执行  
    (void) signal(SIGINT, ouch);  
    while(1)  
    {  
	printf("Hello World!\n");  
	sleep(1);  
    }  
    return 0;  
}  

/*
void thread(void)
{
    int i;
    for(i=0;i<3;i++)
    printf("This is a pthread.n");
}

int main(void)
{
    pthread_t id;
    int i,ret;

    ret=pthread_create(&id,NULL,(void *) thread,NULL);
    if(ret!=0){
	printf ("Create pthread error!n");
	exit (1);
    }
    for(i=0;i<3;i++)
	printf("This is the main process.n");
    pthread_join(id,NULL);
    return (0);
}
*/

/*
int main( void )  
{  
    int filedes[2];  
    char buf[80];  
    pid_t pid;  
      
    pipe( filedes );  
    pid=fork();          
    if (pid > 0)  
    {  
            printf( "This is in the father process,here write a string to the pipe.\n" );  
            read( filedes[0], buf, sizeof(buf) );  
            printf( "%s\n", buf );  
            close( filedes[0] );  
            close( filedes[1] );  
        }  
    else if(pid == 0)  
    {  
            printf( "This is in the child process,here read a string from the pipe.\n" );  
            char s[] = "111 Hello world , this is write by pipe.\n";  
            write( filedes[1], s, sizeof(s) );  
            close( filedes[0] );  
            close( filedes[1] );  
        }  
      
    waitpid( pid, NULL, 0 );  
      
    return 0;  
}  
*/

/*
void createsubprocess(int num)  
{  
    int i;  
    int child;  
    int pid[num];     
    for(i=0;i<num;i++)  
    {  
            if((child = fork()) == -1)  
            {  
	                perror("fork");  
	                exit(EXIT_FAILURE);  
	            }  
            else if(child==0)       //子进程运行  
            {  
	                pid[i]=getpid();  
	                if(execl("/usr/audio/./batchscript","./batchscript",(char *)0) == -1 )  
	                {  
			                perror("execl");  
			                exit(EXIT_FAILURE);  
			            }  
	            }  
              
            else  
            {     
	            }     
        }  
    for(i=0;i<num;i++)  
    {  
            waitpid(pid[i],NULL,0);  
                      
        }  
}  

int main(void)
{
    int num = 5;

    createsubprocess(num);
    printf("In parent");
    return 0;
}
*/

/*
int main(void)  
{  
    pid_t child;  

    printf("flag 1\n");
    printf("flag 2\n");
    if((child = fork()) == -1)  
    {  
            perror("fork");  
            exit(EXIT_FAILURE);  
    }  
    else if(child == 0)                 //子进程中  
    {  
            puts("in child\n");  
            printf("\tchild pid = %d\n",getpid());  
            printf("\tchild ppid = %d\n",getppid());  
            exit(EXIT_SUCCESS);  
    }  
    else      
    {  
            puts("in parent\n");  
            printf("\tparent pid = %d\n",getpid());  
            printf("\tparent ppid = %d\n",getppid());  
    }  

    printf("flag 3\n");
    exit(EXIT_SUCCESS);  
}  
*/

/*
int main(int argc, char **argv)
{
    int err;
    char *cmd = "ls -lht";
    err = system(cmd);
    if (err){
	printf("Error occured!");
    }
    return 0;
}
*/
