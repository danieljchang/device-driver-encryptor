/**************************************************************
* Description:
*	This project is to build a device driver.
*    In this file we are running a simple input output user program
*    utilizing our device driver "EncryptDev".
*    Taking our users input, encrypting and decyrpting it
*    and then printing it out.
*
**************************************************************/

#include <sys/fcntl.h> 
#include <sys/stat.h>
#include <sys/ioctl.h>      
#include <unistd.h>     
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define SET_KEY 200
#define SET_ENCRYPT 100

int main(int argc, char *argv[]){
    char again[1];
    char* text;
    char* result;
    text = malloc(256);
    result = malloc(256);
    long key = 0;
    printf("___________________USER PROGRAM_________________\n");
    while(1){
        printf("Opening encryption \n");
        // Mkaing it 0_RDWR because we want to write and read 
        // from the device driver
        int fd = open("/dev/EncryptDev", O_RDWR);
        if (fd < 0){
            printf("Device Open Error\n");
            perror("Device File open Error");
            return -1;
        }
        // using iocontrol to switch to encrypt, standard is on decrypt
        ioctl(fd, SET_ENCRYPT, 1);
        printf("Enter a key for encryption (Enter an integer between 0 and 80)\n");
        scanf("%li", &key);
        getc(stdin);
        if (key > 80){
            printf("enter a key between 0 and 80");
            int closeRet = close(fd);
            if (closeRet != 0){
                printf("\nfailed to close Device driver\n");
                return -1;
            }
            printf("\nClosing Encryption Device Driver\n");
            continue;
        }
        printf("setting key %li\n", key);
        ioctl(fd, SET_KEY, key);
        
        printf("Enter text to encrypt: \n");
        // reading all characters until we reach a \n or eof
        scanf("%[^\n]s", text);
        getc(stdin);

        printf("\n-------encrypting--------\n");
        // read only takes whatever is in the kernalBuffer
        // so we need to write it to the kernal buffer and encrypt/decrypt
        // whatever the message is
        write(fd, text, strlen(text));
        read(fd, result, strlen(text));
        printf("Here is your encrypted text :\n%s\n", result);

        ioctl(fd, SET_ENCRYPT, 0);
        printf("\n------changing to decrypt--------\n");
        printf("\n------decrypting-------\n");
        // now we have to write and read.
        write(fd, result, strlen(result));
        read(fd, result, strlen(result));
        printf("here is the decrypted text :\n%s\n",result);

        int closeRet = close(fd);
        if (closeRet == 0){
            printf("\nClosing Encryption Device Driver\n");
        }
        printf("\nWould you like to encrypt or decrypt again? ENTER:   Y/N \n");
        scanf("%1s", again);

        
        if (toupper(again[0]) != 'Y'){
            break;
        }
        for (int i = 0; i <256 ;i++){
            text[i] = '\0';
            result[i] = '\0';
        }
        
    }
    free(text);
    text = NULL;
    free(result);
    result = NULL;

    return 0;
}