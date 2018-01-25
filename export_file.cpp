/************************** include *****************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <conio.h>
#include <direct.h>
/**************************  prototypes  ***************/
//int _getch(void);

void err (char *errstr);


/***************************/
#define RECEIVE_FILE 1
#define SAVE_DISK_IMAGE 1
#define SHOW_DIR 0
#define SHOW_SECS 0
#define SHOW_UNUSED_SECS 0
#define SHOW_ATR_HEADER 0
#define MAX_LEN 720 * 256

char fs[500];	//filename
char diskimage_name [500];
char atr_filename [500];
char file_filename [500];
char pathstr[500];
char *file_extension;

	short header_len; //XFD: 0, ATR: 16
	short df;		//density
	long lenf;		//length of file
	long filsiz;		//length of Atari file
	long frbytes;	//free bytes
	short mapbyt,mapsec;

	int secno;		//total sectors
	int sectlen;		//length of sector
	int sec;		//present sector
	int secold;
	int filno	;		//present file in directory
	int frsectors;
	int needsecs, seccount;
	int fillen, endfound;
	char ds[1024+1][256];	//whole disk
	char sector_check[1024];
	char as[12]	;	//atari filename
	char *ptr;		/* temporary pointer */
	char *ptr2;		/* temporary pointer */
	
	FILE *stream;
	FILE *stream2;
	FILE *stream3;



/*************************************************************************************/
/********************************* M A I N *******************************************/
/*************************************************************************************/



void main(int argc,char *argv[500])
	{
		
	int i,j,cnt;
	char *asptr, *fsptr;
	int entryfound,endfound;
/*********** Ganze Diskette laden *********************/

	
	puts ("***** Exportfile V.1.01 (2010) ***** by Richard Jackman\n");
	puts("exports a file from an XFD or ATR disk image to the current directory.");

	
	
	switch (argc) 
		{


		case 2:												// file_filename passed through drag and drop
		
			strcpy (diskimage_name,	argv[1]);
			if (strchr(diskimage_name,'\\'))					//If dir path designated
				{
				strcpy(pathstr,diskimage_name);				//Change directory path to SG dir
				ptr = strrchr(pathstr,'\\');
				*++ptr = '\0';
				//printf ("Directory path: %s\n",pathstr);
				_chdir(pathstr);

				ptr = diskimage_name;						//remove path from file_filename
				ptr2 = strrchr(diskimage_name,'\\');
				ptr2++;
				do *ptr++ = *ptr2++;while (*ptr2);
				
				*ptr = '\0';
				//printf ("Disk image is %s\n",diskimage_name);
				}

			break;

		default:
			puts("\a Filename has not been successfully passed to exportfile.exe\n");
			puts("To export a file from an XFD or ATR disk image,");
			puts ("drag and drop the disk image icon onto the exportfile.exe icon.");
			puts ("press any key");
			_getch();
			exit(1);
		}

	// test validity of extension in disk image file name

	file_extension = diskimage_name + strlen(diskimage_name)-4;
	if ((strcmp (file_extension, ".XFD") == 0 ) || (strcmp (file_extension, ".xfd") == 0 ))
		header_len = 0;
	else
		if ((strcmp (file_extension, ".ATR") == 0 ) || (strcmp (file_extension, ".atr") == 0 ))
			header_len = 16;
		else
			{
			puts ("Only ATR and XFD disk images are supported.");
			_getch();
			exit(1);
			}
	//

	// open disk image file

	if ((stream=fopen(diskimage_name,"rb")) == NULL)
		{
		printf("Can't open %s\n",diskimage_name);
		puts ("press any key");
		_getch();
		exit(1);
		}
	printf ("\nDisk image: ");
	puts(diskimage_name);
	//puts ("\n");


	

	fseek (stream, 0, SEEK_END);
	lenf = ftell( stream );

	if (lenf > MAX_LEN + header_len) 
		{
		puts ("File is too long");
		_getch();
		exit(1);
		}	
//	printf("Image has %li bytes\n",lenf);

	switch (lenf - header_len)
		{
		case 720 * 128:
			puts ("Single density");
			df=1;
			secno=720;
			sectlen=128;
			mapsec = 360;
			mapbyt = 10;
			break;
		case 1040 * 128:
			printf ("Enhanced density");
			puts ("\nOnly single and double density are supported.");
			exit(1);
			df=2;
			secno=1040;
			sectlen=128;
			mapsec = 1024;
			mapbyt = 0;
			break;
		case 720 * 256:	
			printf ("Double density");
			df=1;
			secno=720;
			sectlen=256;
			mapsec = 360;
			mapbyt = 10;
			break;
		default:
			err ("Size of disk image does not match SD, ED or DD.");
		}
	
	printf (" %i sectors, %i bytes per sector\n\n",secno,sectlen);

	
/************** Read in whole disk image *******************************/
	
	
	fseek (stream, header_len , SEEK_SET);			// Reset file pointer

	for (sec = 1; sec <= secno; sec++ )
		{
		fread(&ds[sec][0], 1, sectlen, stream);
//		printf ("%i: %i    ",sec,ds[sec][0]);
		}
	fclose(stream);
	
	// (secs 1 to 3 are always 128 bytes)



	
	
/******************** Check Integtrity of Disk *************************************/

	
	cnt = endfound = 0;
	filno = 1;
	memset (sector_check,0,1024);
	for (i = 361; i<369; i++)
		{
		for (j=0; j<=112; j += 16)
			{
			if ( ds[i][j] == 0)
				{
				endfound++;
				break;
				}
			if(!(ds[i][j]&128))
				{
				fillen	= (unsigned char)ds[i][j+1] + ((unsigned char)ds[i][j+2]) * 256 ;//File sectlen, not crucial
				sec		= (unsigned char)ds[i][j+3] + ((unsigned char)ds[i][j+4]) * 256 ;//Start sector number
				memcpy (as,&ds[i][j+5],8+3);					//File name

				printf ("%2i> %s %3i  |  ",filno,as,fillen); // Show dir
				if (cnt++ == 2)
					{
					puts ("");
					cnt = 0 ;
					}
	

				do	{
					if ((unsigned char)ds[sec][sectlen-3]/4 != filno-1)
						{
						printf("\a\nDisk defect: file number mismatch in sector %i: %i<>%i\n",sec,(unsigned char)ds[sec][sectlen-3]>>2,filno-1);
						_getch();
						}
#if SHOW_SECS				
					printf("%4i",sec);
#endif
					if (sector_check[sec])
						{
						printf("\a\nDisk defect: sector %i is already in use by file %i\n",sec, sector_check[sec]);
						_getch();
						}
					
					sector_check[sec] = (char) filno;
					//printf(" %-3i",sector_check[sec]);
					sec = ((unsigned char)ds[sec][sectlen-2]) + ((unsigned char)(ds[sec][sectlen-3]&3)) * 256 ;
					}while (sec);
#if SHOW_SECS				
				puts("\n");
#endif				
				}
			filno++;
			}
		if ( endfound )
			break;
		}

frsectors = (unsigned char)ds[360][3] + (256 * (unsigned char)( ds[360][4]));//VOTC
//	printf ("VOTC: %i sectors, ",(unsigned char)ds[360][1] + (256 * (unsigned char)ds[360][2]));
	printf ("  %i free sectors\n",frsectors);
/*******************************************************/
//	do
//	{
	puts("\nPlease enter name and extension of file to export (filename.ext)");
	
	gets (fs);



	/**************** Format file name for directory *******************/
	
	if (fsptr = strrchr(fs, '\\'))
		fsptr++;
	else
		fsptr = fs;

	// Check for correct filename format while moving filename to as.



	strcpy (as, "           ");//8+3 blanks
	asptr = as;

	
	if ( !isalpha (*fsptr) ) //check that first character of filename is a letter
		err("Filename must begin with a letter.");
		

	cnt = 0;
	while (isalnum (*fsptr))
		{
		if (++cnt == 9)
			err("Filename is too long.");
		*fsptr = toupper(*fsptr);
		*asptr++ = *fsptr++;
		}

	if (*fsptr)			// non alphanum character present
		{
		if (*fsptr++ == '.')	// file extention present
			{
			asptr = as + 8;
			cnt = 0;
			while (*fsptr)
				{
				if (++cnt == 4)
					err("Filename extension is too long.");
					
				if (!isalnum (*fsptr))
					err("Only alpha-numerical characters are allowed in extension.");
				
				*fsptr = toupper(*fsptr);
				*asptr++ = *fsptr++;
				}
			}
		else
			err("Only alpha-numerical characters are allowed in filename.");
			
		}

//	printf ("Directory entry in xfd disk: '%s'\n",as);
//	_getch();
//	exit(1);


	


/********************    Search for file in directory   ********************/	

	filno = sec = entryfound = endfound = 0;
		
	for (i = 361; i<369; i++)				// For each Directory sector
		{
		for (j=0; j<=112; j += 16)			// For each directory entry in sector
			{
			if (ds[i][j]==0)				// If byte 0 of entry == 0, end of file list
				{
				endfound++;
				break;
				}
			if ( !(ds[i][j]&128))							// If byte 0 of entry not inverted, valid file present
				{
				if ( memcmp( &(ds[i][j+5]), as, 8+3 )==0 )  //filename entry found
					{
					entryfound++;
					break;
					}
				}
			filno++;
			}
		if ( entryfound || endfound)
			break;
		}

	if ( entryfound == 0)
		err("Entry not found.");
	

	
	
	
/******************** Export File *******************************/

	if ((stream=fopen(fs,"wb")) == NULL)
		{
		printf("Can't open %s\n",fs);
		puts ("press any key");
		_getch();
		exit(1);
		}

	sec = (unsigned char)ds[i][j+3] + (unsigned char)ds[i][j+4] * 256;//Start sector number

	do// won't work if length of file is 0
		{


		fwrite (&ds[sec][0], ((unsigned char) ds[sec][sectlen-1]),1, stream);
		
		sec = ((unsigned char)ds[sec][sectlen-3]&3) * 256   +   (unsigned char) ds[sec][sectlen-2];//next sector
		

		} 
		while (sec);




	fclose (stream);

	printf("\n%s has been exported to the current directory.\nPress any key to exit.",fs);
	_getch();


	}




/*************************************************************************************/
/******************* S U B R O U T I N E S *******************************************/
/*************************************************************************************/



/******************** error *********************************************/

void err (char *errstr)

	{
	printf ("\n\a%s\n",errstr);
	_getch();
	exit(1);
	}

/***********************************************************************************/

