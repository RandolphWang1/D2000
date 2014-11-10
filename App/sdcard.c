#include<unistd.h>
#include <termios.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>

void sdcard_server()
{
    int fd;
    char string[20];
    memset(string,0,20);
    char string1[]="update.tar: OK";
    if(open("/dev/mmcblk0",O_RDONLY,0666)==-1)
    {
        lcd_clear();
        text_out(0,0," NOT FOUND SDCARD£¡");
        sleep(2);
    }
    else 
    {
        lcd_clear();
        system("mount /dev/mmcblk0 /mnt/sd");
        text_out(0,0," mounted");
        sleep(2);
        text_out(0,16," checking");
        sleep(2);
        system("cp /mnt/sd/update.* ./");
        system("md5sum -c update.md5 > /mnt/key.txt");
        system("rm ./update.*");
        fd=open("/mnt/key.txt",O_RDONLY);
        read(fd,string,14);
        if(strcmp(string,string1)==0)
        {
            text_out(0,32," check ok rebooting");
            system("umount /mnt/sd");
            while(1);
        }
        else text_out(0,32," no update file!");
        system("umount /mnt/sd");
        sleep(2);
    }
}
