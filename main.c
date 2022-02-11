#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include<sys/ioctl.h>
 #include "vigenere.h"
 
int util_lenght(char* string)
{
    int i;
    for(i = 0; string[i] != '\0'; i++);
    return i;
}

int main()
{
        int fd,r1;
        int32_t value, number;
        char str1[] = "SYSTEMSPROGRAMMING";
        char str2[] = "FARUK";
        char key[]="LINUX";
        char *string1 = (char*) malloc(sizeof(char)*util_lenght(str1));
        char *string2 = (char*) malloc(sizeof(char)*util_lenght(str2));
        
        printf("\nOpening Driver\n");
        fd = open("/dev/vigenere0", O_RDWR);
        if(fd < 0) {
                printf("Cannot open device file...\n");
                return 0;
        }
   
 
        r1=write(fd,str1,util_lenght(str1));

        printf("%s written %d\n",str1,r1);

        r1=write(fd,str2,util_lenght(str2));

        printf("%s written %d\n",str2,r1);

       
        lseek(fd,0,SEEK_SET);
        
        r1=read(fd,string1,util_lenght(str1));
        printf("%s read %d\n",string1,r1);
        ioctl(fd, VIGENERE_MODE_DECRYPT, key);

        r1=read(fd,string2,util_lenght(str2));
        printf("%s read %d\n",string2,r1);
        lseek(fd,util_lenght(str1),SEEK_SET);
        ioctl(fd, VIGENERE_MODE_SIMPLE, key);
        r1=read(fd,string2,util_lenght(str2));
        printf("%s read %d\n",string2,r1);

        lseek(fd,0,SEEK_SET);
        ioctl(fd, VIGENERE_MODE_DECRYPT, key);
        read(fd,string1,util_lenght(str1));
        printf("%s\n",string1);

       
  

        free(string1);
        free(string2);
        int d;
        printf("Closing Driver\n");
        scanf("%d",&d);
        close(fd);
}


