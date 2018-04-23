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


int main(int argc, char * argv[])
{
        unsigned short sem_values[2] = { 1, 0 };
        union semun ctlun;
        int consumer_pid=0;
        key_t k=ftok("myfile",11);
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

        int *at_adr=shmat(shmid,0,0);
        /*if((int)at_adr<0)
        {
                perror("shmat failed");
                exit(3);
        }*/

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
                exit (4);
        }


        int fd=open("myfile",O_RDONLY);
        if(fd<0)
        {
                perror("open failed");
                exit(5);
        }

        while(consumer_pid==0)
        {
                char pid[5]={'0'};
                strcpy(pid,(char *)at_adr);
                //printf("%s\n",pid);
                consumer_pid=atoi(pid);
                if(consumer_pid>0)
                        printf("Consumer Pid = %d\n",consumer_pid);
        }


        int ret;
        char buf[100];
        struct sembuf sembuf_array[1];
        while((ret=read(fd,buf,sizeof(buf)))>0)
        {
                memset(&sembuf_array,0,sizeof(sembuf_array));
                sembuf_array[0].sem_num=0;
                sembuf_array[0].sem_op=-1;
                int x=semop(semid,sembuf_array,1);
                if(x<0)
                {
                        perror("semop failed");
                        exit(6);
                }

                memset(at_adr,0,100);
                //printf("%s\n",buf);
                strcpy((char *)at_adr,buf);
                //write(at_adr,buf,ret);                //writes in binary format
                memset(buf,0,100);

                memset(&sembuf_array,0,sizeof(sembuf_array));
                sembuf_array[0].sem_num=1;
                sembuf_array[0].sem_op=1;
                x=semop(semid,sembuf_array,1);
                if(x<0)
                {
                        perror("semop failed");
                        exit(7);
                }

        }

        if(ret<0)
        {
                perror("read failed");
                exit(8);
        }

        if(shmdt(at_adr)<0)
        {
                perror("shmdt failed");
                exit(9);
        }

        close(fd);

        if((kill(consumer_pid,10))<0)
        {
                perror("Kill failed");
                exit(10);
        }

        return 0;
}
