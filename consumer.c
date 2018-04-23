#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ipc.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/shm.h>
union semun
{
        int val;                      /* Value for SETVAL */
        struct semid_ds *buf;         /* Buffer for IPC_STAT, IPC_SET */
        unsigned short *array;        /* Array for GETALL, SETALL */
        struct seminfo *__buf;        /* Buffer for IPC_INFO(Linux-specific) */
};

int flag=0;
int *at_adr;

void compare_file();

void myhandler()
{
        if(shmdt(at_adr)<0)
        {
                perror("shmdt failed");
                exit(10);
        }
        compare_file();
        exit(0);
}

void compare_file()
{
        int f1=open("myfile",O_RDONLY);
        if(f1<0)
        {
                perror("Open 1 failed");
                exit(11);
        }
        int f2=open("myfile_new",O_RDONLY);
        if(f1<0)
        {
                perror("Open 2 failed");
                exit(12);
        }

        int f=0;
        int ret1,ret2;
        char ch1='0', ch2='0';
        while(f==0)
        {
                if((ret1=read(f1,&ch1,1))<0)
                {
                        perror("Read 1 failed");
                        exit(13);
                }
                if((ret2=read(f2,&ch2,1))<0)
                {
                        perror("Read 2 failed");
                        exit(14);
                }
                if(ch1!=ch2)
                {
                        printf("\n\n----------Files don't match !!!----------\n\n");
                        return;
                }
                if(ret1==0 && ret2==0)
                {
                        printf("\n\n----------Files Match.----------\n\n");
                        return;
                }
        }
        return;
}

int main(int argc, char * argv[])
{
        unsigned short sem_values[2] = { 1, 0 };
        union semun ctlun;

        key_t k=ftok("myfile",11);
        printf("cons pid = %d\n",getpid());
        printf("Key is: %d\n",k);
        if(k<0)
        {
                perror("ftok failed");
                exit(1);
        }

        int shmid=shmget(k,100,IPC_CREAT|S_IRWXU);
        printf("Shmid is: %d\n",shmid);
        if(shmid<0)
        {
                perror("shmget failed");
                exit(2);
        }

        at_adr=shmat(shmid,0,0);

        int semid=semget(k,2,IPC_CREAT|S_IRWXU);
        printf("Semid is: %d\n",semid);
        if(semid<0)
        {
                perror("semget failed");
                exit(4);
        }

        ctlun.array = sem_values;

        int ret1 = semctl (semid, 0, SETALL, ctlun);
        if (ret1 < 0)
        {
                perror ("semctl failed");
                exit (3);
        }


        int fd=open("myfile_new",O_CREAT|O_WRONLY,S_IRWXU);
        if(fd<0)
        {
                perror("open failed");
                exit(5);
        }


        memset(at_adr,0,100);
        sprintf((char *)at_adr,"%d",getpid());


        int ret;
        char buf[100];
        struct sembuf sembuf_array[1];
        while(flag==0)
        {
                memset(&sembuf_array,0,sizeof(sembuf_array));
                sembuf_array[0].sem_num=1;
                sembuf_array[0].sem_op=-1;
                int x=semop(semid,sembuf_array,1);
                if(x<0)
                {
                        perror("semop 6 failed");
                        exit(6);
                }

                memset(buf,0,100);
                //read(at_adr,buf,100);                 //creates myfile_new in binary form
                strcpy(buf,(char *)at_adr);
                //printf("%s\n",(char *)at_adr);
                //printf("from buf %s\n",buf);
                ret=write(fd,buf,strlen(buf));
                printf("---%s---\n",buf);
                memset(at_adr,0,100);
                if(ret<0)
                {
                        perror("write failed");
                        exit(7);
                }

                //char pid[10]={'0'};
        //      memset(at_adr,0,100);
        //      sprintf((char *)at_adr,"%d",getpid());
                //strcpy(at_adr,pid);

                memset(&sembuf_array,0,sizeof(sembuf_array));
                sembuf_array[0].sem_num=0;
                sembuf_array[0].sem_op=1;
                x=semop(semid,sembuf_array,1);
                if(x<0)
                {
                        perror("semop 8 failed");
                        exit(8);
                }

                signal(10,myhandler);
        }


        return 0;
}
