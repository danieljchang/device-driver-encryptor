/**************************************************************
* Class:  CSC-415-03 Fall 2022
* Name: Daniel Chang
* Student ID: 921174056
* GitHub UserID: danieljchang
* Project: Assignment 6 – Device Driver
*
* File: EncryptDev.c
*
* Description:
*	This program is creating a device driver that will function
*   as an encryption program. Utilizing xor encryption, we can
*   control the read, write, open, close, and ioctl, to
*   be our custom character device driver.
*
**************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#define MY_MAJOR 415
#define MY_MINOR 0
#define DEVICE_NAME "EncryptDev"
#define SET_KEY 200
#define SET_ENCRYPT 100

int major, minor;
// KernalBuffer might not even be needed since I have a buffer
// inside of the struct.
char *kernalBuffer; // This will hold my encrypted/decrypted strings
struct cdev mycdev;
int actualRXSize = 0;

MODULE_AUTHOR("Daniel Chang");
MODULE_DESCRIPTION("A simple encrypting program");
MODULE_LICENSE("GPL");
struct myds{
    int count;
    int encryptFlag; // 0 if decrypt 1 if encrypt
    int alreadyEncrypted; // 0 if not 1 if it is
    int encryptKey; // what we are encrypting against
    char* buffer;   // to store our new encrypted/decrypted string
}myds;

int myOpen(struct inode * inode, struct file* fs){
    struct myds* ds;
    ds = vmalloc(sizeof(struct myds));
    if (ds == 0){
        printk(KERN_ERR "Failed to vmalloc myds\n");
        return -1;
    } 
    kernalBuffer = vmalloc(512);
    if (kernalBuffer == 0){
        printk(KERN_ERR "failoed to vmalloc kernal buffer\n");
        return -1;
    }
    // Initializing all of my variables in my struct.
    ds->alreadyEncrypted = 0;
    ds->count = 0;
    ds->buffer = NULL;
    ds->encryptFlag = 0;    // automaticall set to decrypt on open
    ds->encryptKey = 10;    // default key is 10, could be 0 since 
                            // program calls it
    fs->private_data = ds;
    printk(KERN_ALERT "SUCCESSFULLY OPENED THE DEVICE DRIVER\n");
    return 0;
}
static ssize_t myRead(struct file * fs, char __user * buffer, size_t hsize, loff_t * off){
    int retValue;
	struct myds* ds;
    ds = (struct myds*)fs->private_data;
    // this will take the encrypted character and change it 
    //  or store it and then return it to the buffer.
	printk("Reading : %p\nOf length : %ld: \n", fs, hsize);
    // taking our kernal and giving it to the user 
    // whether or not it is encrypted or not
    // if (ds->encryptFlag == 0 && ds->alreadyEncrypted == 1){
    //     for (i = 0; i < hsize; i++){
    //         // xor decryption. 
    //         ds->buffer[i] = (ds->buffer[i] ^ ds->encryptKey); 
    //     }
    //     kernalBuffer = ds->buffer;
    // }
    retValue = copy_to_user(buffer, kernalBuffer, hsize);
    if (retValue != 0){
        printk("failed to read\n");
        return -1;
    }
    // for (i = 0; i < hsize; i++){
    //     buffer[i] = ds->buffer[i];
    // }
    return hsize;
}

static ssize_t myWrite(struct file * fs, const char __user * buffer, size_t hsize, loff_t * off){
    struct myds* ds;
	int retValue;
    int i;
    ds = (struct myds*)fs->private_data;
    // Creating a specialized size buffer for each time call
    // this buffer will be vmalloced hsize+1 for \0.
    ds->buffer = vmalloc(hsize+1);
    printk("%s buffer\n", buffer);
    copy_from_user(ds->buffer, buffer, hsize+1);
    printk("%s buffer\n", ds->buffer);
    ds->buffer[hsize] = '\0';
    // this will take the encrypted character and 
    // change it or store it and then return it to the buffer.
    printk(KERN_ALERT "Successfilly getting into write now encrypting %d\n",
        ds->encryptFlag);
    // encrypting a non encrypted message
    if (ds->encryptFlag == 1 && ds->alreadyEncrypted == 0){
        for (i = 0; i < hsize; i++){
            // xor encryption. 
            // if we want to do a string for key,
            // we need to use key % strlen(key) in the xor
            ds->buffer[i] = (ds->buffer[i] ^ ds->encryptKey); 
        }
        // ensuring we do not double encrypt/decrypt
        ds->alreadyEncrypted = 1;    
        printk(KERN_ALERT "encrypted\n");
    // decrypting an encrypted message
    } else if (ds->encryptFlag == 0 && ds->alreadyEncrypted == 1){
        for (i = 0; i < hsize; i++){
            ds->buffer[i] = (ds->buffer[i] ^ ds->encryptKey);
        }
        printk(KERN_ALERT "decrypting\n");
        ds->alreadyEncrypted = 0;

    }else{
        // trying to decrypt and decrypted message or
        // trying to encrypt an encrypted message
        printk(KERN_ALERT "Trying to encrypt an encrypted message or vice versa\n");
        // since we aren't doing anything, we can just
        // give the users buffer and send it to the kernalbuffer
        retValue = copy_from_user(kernalBuffer, buffer, hsize);
        if(retValue != 0){
            printk(KERN_ERR "Failed to copy to user %d\n", retValue);
            vfree(ds->buffer);
            ds->buffer = NULL;
            return -1;
        }
        printk(KERN_ALERT "Successfully copied to user\n");
        return hsize;
    }
    // Since we are not copying the user's buffer we can't user copy from user
    // So we can just do a basic assign here.
    kernalBuffer = ds->buffer;
    // free the stuff vmlloced in this function
    vfree(ds->buffer);
    ds->buffer = NULL;
    return hsize;
}
int myClose(struct inode* inode, struct file* fs){
    // freing all vmalloced memory.
    struct myds* ds;
    ds = (struct myds *) fs->private_data;
    vfree(ds->buffer);
    ds->buffer = NULL;
    vfree(ds);
    ds = NULL;
    vfree(kernalBuffer);
    kernalBuffer = NULL;
    printk("Successfully closed program\n");
    return 0;
}
static long myioctl(struct file* fs, unsigned int command, unsigned long data){
    struct myds * ds;
    ds = (struct myds *) fs->private_data;

    // this means we are attempting to change the multipler for encryption
    // command should be = 200
    if (command == SET_KEY){
        printk(KERN_INFO "The information inside of the setkey change\n");
        if (data > 0){
            //((struct myds*)fs->private_data)->encryptionFlag = data;
            // encrypt key can be a character or a number
            // if we want to do a string for key,
            // we need to use key % strlen(key) in the xor
            ds->encryptKey = data;
            printk("Changing the Encryption Key to %d\n", ds->encryptKey);
        }else{
            // I don't want to risk creating errors with negative keys
            // Maybe we don't need to do this if we do it in the test program
            // but good to have in case no test program.
            printk(KERN_ALERT "Invalid Encryption Key %ld\n", data);
            return -1;
        }
            // command should be 100 for this one to change encrypt/decrypt
    }else if (command == SET_ENCRYPT){  
        printk(KERN_INFO "The information inside of the setencrypt change\n");
        // XOR encryption is just a toggle encrypt 
        // 0 is decrypt
        // 1 is encrypt
        if (data == 0 || data == 1){
            ds->encryptFlag = data;
            printk("Changing the Encryption flag to %d\n", ds->encryptFlag);
        }else{
            printk(KERN_ALERT "Invalid encryption input, Please enter 1 or 0 for encrypt flag\n");
        return -1;
        }
        }else{
            printk(KERN_ALERT "Trying to a non existent command, change 10 for flag 20 for key\n");
    }
    
    printk("----- Current Status ----- Encryption Key : %d Encyption Flag : %d\n",
        ds->encryptKey, ds->encryptFlag);
    return 0;
}

struct file_operations fops = {
    .read = myRead, // will return whatever is in the kernal buffer
    .write = myWrite,   // will encrypt or decrypt the message
    .open = myOpen,     // opens device driver
    .release = myClose, // closes device driver
    .unlocked_ioctl = myioctl,  // sets my key and sets encrypt/decrypt
    .owner = THIS_MODULE

};

int init_module(void){
    int result, registers;
    dev_t devno;
    // our device number will be set to 415,
    // when we want to set it up we need to ensure we use the right
    // major minor number.
    devno = MKDEV(MY_MAJOR, MY_MINOR);
    // registering the device driver
    registers = register_chrdev_region(devno, 1, DEVICE_NAME);
    printk(KERN_INFO "register chardev succeeded 1: %d\n", registers);
    // setting up my custom fops for my custom device driver
    cdev_init(&mycdev, &fops);
    mycdev.owner = THIS_MODULE;
    // adding it to the device list.
    result = cdev_add(&mycdev, devno, 1);

    printk(KERN_INFO "Dev add chardev succeded 2: %d\n", result);
    printk(KERN_INFO "Welcome - Encrypt Driver is loaded.\n");
    if (result < 0 ){
        printk(KERN_ERR "Register chardev failed :%d\n", result);
    }
    return result;
}

void cleanup_module(void){
    dev_t devno;
    devno = MKDEV(MY_MAJOR, MY_MINOR);
    unregister_chrdev_region(devno, 1);
    cdev_del(&mycdev);
}
