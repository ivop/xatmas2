
/************************** include *****************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <direct.h>

/**************************  prototypes  ***************/

#define STAND_ALONE 1

void findfree(int);
void err(char *errstr);
void file_err(char *errstr, char filename[]);
void check_sector(int sec);
void file_to_image(char file_filename[], char diskimage_name[]);

/***************************/


int argcount;

char *ptr;                      /* temporary ptr */
char *ptr2;                     /* temporary ptr */






/*************************************************************************************/

/********************************* M A I N *******************************************/

/*************************************************************************************/


void main(int argc, char *argv[500]) {
    char pathstring[500];
    char diskimage_name[500];
    char file_filename[500];

    argcount = argc;
    int c;
    FILE *filestream;

/******************** Load complete disk *********************/


    switch (argc) {
    case 3:                    // diskimage_name passed from CODEGEN

        strcpy(diskimage_name, argv[2]);
        while (strchr(diskimage_name, '?'))
            *strchr(diskimage_name, '?') = ' ';
        strcpy(file_filename, argv[1]);
        break;

    case 2:                    // file_filename passed through drag and drop

        strcpy(file_filename, argv[1]);
        if (strchr(file_filename, '\\'))        //If dir path designated
        {
            //Change directory path to SG dir
            strcpy(pathstring, file_filename);
            ptr = strrchr(pathstring, '\\');
            *++ptr = '\0';
            printf("Directory path: %s\n", pathstring);
            _chdir(pathstring);

            //remove path from file_filename
            ptr = file_filename;
            ptr2 = strrchr(file_filename, '\\');
            ptr2++;
            do
                *ptr++ = *ptr2++;
            while (*ptr2);
            *ptr = '\0';
            printf("File to copy is %s\n", file_filename);
        }

        if ((filestream = fopen("savexfd.cfg", "rb")) == NULL) {
            puts("Enter name of disk image to write to: ");
            gets_s(diskimage_name);

        } else {
            fgets(diskimage_name, 500, filestream);
            fclose(filestream);
            printf("Write to disk image %s? Y/N>", diskimage_name);
            c = _getch();
            if (c != 'y' && c != 'Y') {
                puts("\nEnter name of disk image to write to: ");
                gets_s(diskimage_name);
            }
        }
        break;

    default:
        puts("\a Filename has not been successfully passed to savexfd.\n");
        puts("savexfd.exe saves an ATARI file to an XFD or ATR image file");
        puts("Drag and drop the ATARI file onto the savexfd.exe icon.");
        puts("The disk image specification is  noted in savexfd.cfg.");
        puts("press any key");
        _getch();
        exit(1);
    }

    file_to_image(file_filename, diskimage_name);




    if (argc != 3) {
        printf("Hit any key to exit.");
        _getch();
    }

}

#include "file_to_image.h"
