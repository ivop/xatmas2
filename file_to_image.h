
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#define RECEIVE_FILE 1
#define SAVE_DISK_IMAGE 1
#define SHOW_DIR 0
#define SHOW_SECS 0
#define SHOW_DIR64 0
#define SHOW_UNUSED_SECS 0
#define SHOW_ATR_HEADER 0
#define CREATE_ATR_FROM_XFD 0
#define MAX_LEN 720 * 256




short mapbyt, mapsec;


int sec;                        //present sector
int sectlen;                    //length of sector
char ds[1024 + 1][256];         //whole disk
char sector_check[1024];




//////// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void file_to_image(char file_filename[], char diskimage_name[]) {
    int i, j, cnt, k;
    char *asptr, *fsptr;
    int entryfound, endfound;


    short header_len;           //XFD: 0, ATR: 16
    short df;                   //density
    long lenf;                  //length of file
    long filsiz;                //length of Atari file
    long frbytes;               //free bytes
    char fs[500];               //filename
    char header[16];
    int secno;                  //total sectors
    char as[12];                //atari filename
    int secold;
    int filno;                  //present file in directory
    int frsectors;
    int needsecs, seccount;
    int fillen;
    char *ext;
    char *ptr;
    FILE *filestream;

#if CREATE_ATR_FROM_XFD
    char atr_filename[500];
    FILE *filestream3;
#endif
///////////////////////////////////////////////////////////////////////////////////////////
    as[11] = 0;

    // test validity of extension in disk image file name

    ext = diskimage_name + strlen(diskimage_name) - 4;
    ptr = ext;
    for (i = 0; i <= 3; i++)
        *ptr++ = tolower(*ptr);
    if (strcmp(ext, ".xfd") == 0)
        header_len = 0;
    else if (strcmp(ext, ".atr") == 0)
        header_len = 16;
    else {
        printf("%s Only ATR and XFD disk images are supported.", ext);
        getch();
        exit(1);
    }
    //


    if ((filestream = fopen("savexfd.cfg", "wb")) == NULL) {
        puts("Can't open savexfd.cfg");
        fgets(diskimage_name, 128, stdin);
        getch();
        exit(1);
    } else {
        fputs(diskimage_name, filestream);
        fclose(filestream);
    }




// open disk image file

    if ((filestream = fopen(diskimage_name, "rb")) == NULL) {
#if STAND_ALONE
        file_err("Can't open", diskimage_name);
#else
        printf("Can't open %s. Creating new image from DOS25 data\n",
               diskimage_name);

        if ((filestream = fopen(diskimage_name, "wb")) == NULL)
            file_err("Can't create", diskimage_name);

        fwrite(dos25_block, header_len, 1, filestream);
        fwrite(dos25_block + 16, DOS_BLOCK_SIZE - 16, 1, filestream);
        for (i = DOS_BLOCK_SIZE - 16 + 1; i <= (360 - 1) * 128; i++)
            fputc('\0', filestream);
        fwrite(dos25_votc_dir, DOS25_VOTC_DIR_SIZE, 1, filestream);
        for (i = (720 - 360 - 1) * 128 + DOS25_VOTC_DIR_SIZE; i < 720 * 128;
             i++)
            fputc('\0', filestream);

        fclose(filestream);

        if ((filestream = fopen(diskimage_name, "rb")) == NULL)
            file_err("Can't open", diskimage_name);
#endif
    }


    printf("\nDisk image: %s\n", diskimage_name);


    fseek(filestream, 0, SEEK_END);
    lenf = ftell(filestream);
    if (lenf > MAX_LEN + header_len)
        file_err("File is too long:", diskimage_name);

//      printf("Image has %li bytes\n",lenf);

    switch (lenf - header_len) {
    case 720 * 128:
        printf("Single density:");
        df = 1;
        secno = 720;
        sectlen = 128;
        mapsec = 360;
        mapbyt = 10;
        break;
    case 1040 * 128:
        printf("Enhanced density:");
        err("Only single and double density are supported.");
        exit(1);
        df = 2;
        secno = 1040;
        sectlen = 128;
        mapsec = 1024;
        mapbyt = 0;
        break;
    case 720 * 256:
        printf("Double density:");
        df = 1;
        secno = 720;
        sectlen = 256;
        mapsec = 360;
        mapbyt = 10;
        break;
    default:
        err("Size of disk image does not match SD, ED or DD.");
    }

    printf(" %i sectors of %i bytes\n", secno, sectlen);


/***************************************************************/

    if (header_len == 16) {
        fseek(filestream, 0, SEEK_SET); // Reset file ptr to beginning of ATR-file
        fread(header, 1, header_len, filestream);
    }



/***** Show ATR header **************************************/

#if SHOW_ATR_HEADER
    if (header_len == 16) {



        printf("The size of the ATR disk is $%X without header\n",
               lenf - header_len);
        fseek(filestream, 0, SEEK_SET); // Reset file ptr to beginning of ATR-file
        puts("The standard ATR file format is: (first byte listed as 1, not 0)");

        printf("%2X", fgetc(filestream));
        puts(" * 01: $0296 (sum of NICKATARI) ");
        printf("%2X", fgetc(filestream));
        puts(" * 02: ");
        printf("%2X", fgetc(filestream));
        puts(" * 03: size of this disk image, in paragraphs (size/$10) , low");
        printf("%2X", fgetc(filestream));
        puts(" * 04: size of this disk image, in paragraphs (size/$10) , high");
        printf("%2X", fgetc(filestream));
        puts(" * 05: sector size. ($80 or $100) bytes/sector, low");
        printf("%2X", fgetc(filestream));
        puts(" * 06: sector size. ($80 or $100) bytes/sector, high");
        printf("%2X", fgetc(filestream));
        puts(" * 07: high part of size, in paragraphs (added by REV 3.00) ");
        printf("%2X", fgetc(filestream));
        puts(" * 08..16: Unused");
        printf("%2X", fgetc(filestream));
        puts(" * Remainder of image contains Atari data. ");

    }
//              for (cnt = 1; cnt <= header_len; cnt++ )

#endif



/***** Create ATR header **************************************/

#if CREATE_ATR_FROM_XFD
    if (header_len == 0) {
        strcpy(atr_filename, diskimage_name);
        ptr = strrchr(atr_filename, '.');
        *ptr = '\0';
        strcat(atr_filename, ".ATR");
        if ((filestream3 = fopen(atr_filename, "wb")) == NULL) {
            printf("Can't open %s\n", atr_filename);
            puts("press any key");
            getch();
            exit(1);
        }
        printf("\nDisk image: ");
        puts(atr_filename);

        printf("The size of the ATR disk is $%X without header\n",
               lenf - header_len);
//              fseek (filestream3, 0 , SEEK_SET);                      // Reset file ptr to begenning of ATR-file

        fputc(0x96, filestream3);       // * 01: $0296 (sum of NICKATARI) ");
        fputc(0x02, filestream3);
        fputc((lenf / 0x10) % 256, filestream3);        // * 03: size of this disk image, in paragraphs (size/$10) , low");
        fputc((lenf / 0x10) / 256, filestream3);        // * 04: size of this disk image, in paragraphs (size/$10) , high");
        fputc(sectlen % 256, filestream3);      // * 05: sector size. ($80 or $100) bytes/sector, low");
        fputc(sectlen / 256, filestream3);      // * 06: sector size. ($80 or $100) bytes/sector, high");
        fputc((lenf / 0x10) / 256, filestream3);        // * 07: high part of size, in paragraphs (added by REV 3.00) ");
        for (cnt = 8; cnt <= 16; cnt++) // * 08..16: Unused");
            fputc(0, filestream3);
    }
#endif


/************** Read in whole disk image *******************************/


    fseek(filestream, header_len, SEEK_SET);    // Reset file ptr

    for (sec = 1; sec <= secno; sec++) {
        fread(&ds[sec][0], 1, sectlen, filestream);
//              printf ("%i: %i    ",sec,ds[sec][0]);
    }
    fclose(filestream);

    // (secs 1 to 3 are always 128 bytes)


/************** Show complete Directory - all 64 entries ****************/


#if SHOW_DIR64

    for (i = 361; i < 369; i++) {
        for (j = 0; j <= 112; j += 16) {
            memcpy(as, &ds[i][j + 5], 8 + 3);   //File name

            printf("%-3i%11s Byte0:%3i%4i%4i\n", filno, as,
                   (unsigned char)ds[i][j], i, j);


            filno++;

        }
    }
    getch();

#endif



/******************** Check Integtrity of Disk *************************************/

    endfound = 0;
    filno = 1;
    memset(sector_check, 0, 1024);
    for (i = 361; i < 369; i++) // For each Directory sector
    {
        for (j = 0; j <= 112; j += 16)  // For each directory entry in sector
        {
            if (ds[i][j] == 0)  // If byte 0 of entry == 0, end of file list
            {
                endfound++;
                break;
            }
            if (!(ds[i][j] & 128))      // If byte 0 of entry not inverted, valid file present
            {
                fillen = (unsigned char)ds[i][j + 1] + ((unsigned char)ds[i][j + 2]) * 256;     //bytes 1,2: File length (not crucial)
                sec = (unsigned char)ds[i][j + 3] + ((unsigned char)ds[i][j + 4]) * 256;        //bytes 3,4: first data sector of file
                memcpy(as, &ds[i][j + 5], 8 + 3);       //bytes 5,15: File name
#if STAND_ALONE
                if (argcount != 3)
                    printf("%2i> %s %3i\n", filno, as, fillen);
#endif
                do {
                    if ((unsigned char)ds[sec][sectlen - 3] / 4 != filno - 1) {
                        printf
                            ("\a\nDisk defect: file number mismatch in sector %i: %i<>%i\n",
                             sec, (unsigned char)ds[sec][sectlen - 3] >> 2,
                             filno - 1);
                        getch();
                    }
#if SHOW_SECS
                    printf("%4i", sec);
#endif
                    if (sector_check[sec]) {
                        printf
                            ("\a\nDisk defect: sector %i is already in use by file %i\n",
                             sec, sector_check[sec]);
                        getch();
                    }

                    sector_check[sec] = (char)filno;
//printf(" %-3i",sector_check[sec]);
                    sec = ((unsigned char)ds[sec][sectlen - 2]) + ((unsigned char)(ds[sec][sectlen - 3] & 3)) * 256;    //Last two bytes: next sector
                } while (sec);
#if SHOW_SECS
                puts("\n");
#endif
            }
            filno++;
        }
        if (endfound)
            break;
    }

//

/*********** Show unused sectors *****************/

#if SHOW_UNUSED_SECS

    for (sec = 0; sec < secno; sec++) {
        if (sector_check[sec] == 0 && (sec < 368 || sec >= 1024)) {
            for (j = 0; j < sectlen; j++) {
                if (ds[sec][j]) {
                    printf("Data in sector %i\n", sec);
                    for (j = 0; j < sectlen; j++)
                        printf("%4i", (unsigned char)ds[sec][j]);
                    puts("\n");
                    break;
                }
            }
        }
    }
    getch();
#endif

/*******************************************************/
#if STAND_ALONE == 0
    if (sectlen == 256)         //TURBODOS -- this is not a complete test for TURBODOS
    {

        strcpy(fs, file_filename);
        for (i = 0; i < strlen(fs); i++)
            fs[i] = toupper(fs[i]);
        if (strcmp(fs, "AUTORUN.SYS") == 0) {
            puts("Can't use AUTORUN.SYS with TURBODOS");
            strcpy(file_filename, src_filename);
            strcpy(strchr(file_filename, '.'), ".LOA");
            remove(file_filename);
            if (rename("AUTORUN.SYS", file_filename) == 0)
                printf("AUTORUN.SYS has been renamed to %s\n", file_filename);
            else
                err("AUTORUN.SYS could not be renamed");
        }
        if (strcmp(fs, "AUTORUN.SMT") == 0) {
            puts("Can't use AUTORUN.SMT with TURBODOS");
            strcpy(file_filename, src_filename);
            strcpy(strchr(file_filename, '.'), ".SMT");
            remove(file_filename);
            if (rename("AUTORUN.SMT", file_filename) == 0)
                printf("AUTORUN.SMT has been renamed to %s\n", file_filename);
            else
                err("AUTORUN.SMT could not be renamed");
        }
    }
#endif

/*******************************************************/


    strcpy(fs, file_filename);
    while (strchr(fs, '?'))
        *strchr(fs, '?') = ' ';



        /**************** Format file name for directory *******************/

    if ((fsptr = strrchr(fs, '\\')) != 0)
        fsptr++;
    else
        fsptr = fs;

    // Check for correct filename format while moving filename to as.



    strcpy(as, "           ");  //8+3 blanks
    asptr = as;


    if (!isalpha(*fsptr))       //check that first character of filename is a letter
        err("Filename must begin with a letter.");


    cnt = 0;
    while (isalnum(*fsptr)) {
        if (++cnt == 9)
            err("Filename is too long.");
        *asptr++ = toupper(*fsptr++);
    }

    if (*fsptr)                 // non alphanum character present
    {
        if (*fsptr++ == '.')    // file extention present
        {
            asptr = as + 8;
            cnt = 0;
            while (*fsptr) {
                if (++cnt == 4)
                    err("Filename extension is too long.");

                if (!isalnum(*fsptr))
                    err("Only alpha-numerical characters are allowed in extension.");

                *asptr++ = toupper(*fsptr++);
            }
        } else
            err("Only alpha-numerical characters are allowed in filename.");

    }
//      printf ("Directory entry in xfd disk: '%s'\n",as);
//      getch();
//      exit(1);

        /**************** Open file for reading ***************************/


    if ((filestream = fopen(fs, "rb")) == NULL) {
        printf("Can't open %s\n", fs);
        puts("press any key");
        getch();
        exit(1);
    }



    fseek(filestream, 0, SEEK_END);     // find size of file (filsiz)
    filsiz = ftell(filestream);
    needsecs = filsiz / (sectlen - 3);
    if (filsiz % (sectlen - 3))
        needsecs++;

    fseek(filestream, 0, SEEK_SET);     // Reset file ptr

    frsectors = (unsigned char)ds[360][3] + (256 * (unsigned char)(ds[360][4]));        //VOTC





//      printf ("VOTC: %i sectors, ",(unsigned char)ds[360][1] + (256 * (unsigned char)ds[360][2]));
    printf("%i free sectors, %i needed, ", frsectors, needsecs);




/********************    Search for file in directory   ********************/


    filno = sec = entryfound = endfound = 0;

    for (i = 361; i < 369; i++) // For each Directory sector
    {
        for (j = 0; j <= 112; j += 16)  // For each directory entry in sector
        {
            if (ds[i][j] == 0)  // If byte 0 of entry == 0, end of file list
            {
                endfound++;
                break;
            }
            if (!(ds[i][j] & 128))      // If byte 0 of entry not inverted, valid file present
            {
                if (memcmp(&(ds[i][j + 5]), as, 8 + 3) == 0)    // filename entry found
                {
                    entryfound++;
                    break;
                }
            }
            filno++;
        }
        if (entryfound || endfound)
            break;
    }


    if (entryfound) {
        //printf ("The entry '%s' has been found\n",as);
        frsectors += (unsigned char)ds[i][j + 1] + (unsigned char)ds[i][j + 2] * 0xff;  // add size of file already on disk
        //printf ("Entry found in position %i\n",filno);
    }
    //getch();
    frbytes = (sectlen - 3) * frsectors;

    printf("%i available for %s\n", frsectors, fs);

    if (filsiz > frbytes)
        err("The file is too large to fit on this disk");

/******************** If entry found: Delete file in directory and VOTC *******************************/

    if (entryfound) {
        ds[i][j] |= 0x80;       // deleted in directory

        sec = (unsigned char)ds[i][j + 3] + (unsigned char)ds[i][j + 4] * 256;  //Start sector number
        cnt = 0;
        do                      // won't work if length of file is 0
        {
            sector_check[sec] = 0;
            ds[mapsec][mapbyt + sec / 8] |= (unsigned char)(128 >> (sec % 8));  //adjust VOTC
            //printf ("deleting sector %i\n",sec);
            //printf ("sec:%i byte:%i %i\n",sec,mapbyt + sec/ 8,(unsigned char)ds[mapsec][mapbyt + sec/ 8] );

            sec = ((unsigned char)ds[sec][sectlen - 3] & 3) * 256 + (unsigned char)ds[sec][sectlen - 2];        //next sector
            cnt++;
        } while (sec);

        sec = 360;
        frsectors += cnt;
        ds[sec][3] = (char)(frsectors % 256);   //update no. of free sectors in VOTC
        ds[sec][4] = (char)(frsectors / 256);


    }


/********************    Find free directory position (filno)   ********************/

    filno = sec = entryfound = 0;

    for (i = 361; i < 369; i++) // For each Directory sector
    {
        for (j = 0; j <= 112; j += 16)  // For each directory entry in sector
        {
            if ((ds[i][j] & 128) || (ds[i][j] == 0))    //erased or fresh
            {
                entryfound++;
                break;
            }
            filno++;
            //printf ("filno = %i, first byte = %i\n",filno,ds[i][j]);
        }
        if (entryfound)
            break;
    }



/**************** Write directory entry *************************************/


    printf("Writing %s to disk image\n", fs);

    if (filno == 64)
        err("Directory is full with 64 files.");

    findfree(0);                // determine sec

    ds[i][j] = 'B';             //Flag
    ds[i][j + 1] = (char)(filsiz / (sectlen - 3) + 1);  //+0.99 File sectlen, not crucial
    ds[i][j + 2] = (char)((filsiz / (sectlen - 3) + 1) / 256);  //+0.99
    ds[i][j + 3] = (char)sec;   //Start sector number
    ds[i][j + 4] = (char)(sec / 256);
    memcpy(&ds[i][j + 5], as, 8 + 3);   //File name

    //printf ("Start sector low: %X    hi: %X\n\n",(unsigned char)ds[i][j+3],(unsigned char)ds[i][j+4]);
    //printf ("Start sector: %i\n",(unsigned char)ds[i][j+3]+(unsigned char)ds[i][j+4] * 256);



/**************** Read in full sectors **************************************/

    k = 0;
    seccount = 0;
    for (i = 1; i <= filsiz - (sectlen - 3); i += (sectlen - 3)) {

        fread(&ds[sec][0], (sectlen - 3), 1, filestream);       //read sectlen-3 bytes into d
        secold = sec;
        findfree(sec);
        seccount++;
        /*if (sec == 127)
           printf ("%i %i\n",k++,sec); */

        ds[secold][sectlen - 3] = (char)(sec / 256 + 4 * filno);        //7D: 
        ds[secold][sectlen - 2] = (char)sec;    //7E:Low byte des folgenden Sectors
        ds[secold][sectlen - 1] = (char)(sectlen - 3);  //7F:Anzahl der Datenbytes
    }



/**************** Read in final (partial) sector *****************************/


    i = filsiz % (sectlen - 3);
    if (i) {
        fread(&ds[sec][0], i, 1, filestream);
        ds[sec][sectlen - 3] = (char)(filno * 4);       //7D:hhhhh
        ds[sec][sectlen - 2] = (char)0; //7E:Low byte des folgenden Sectors: 0
        ds[sec][sectlen - 1] = (char)i; //7F:Anzahl der Datenbytes
        seccount++;
    }
    fclose(filestream);

    if (seccount - needsecs)    //this is just a check
    {
        printf("\aRequire %i sectors, but %i were written", needsecs,
               seccount);
        getch();
    }



/****************	Wtite # free sectors to VTOC (sec 360 ) ********************/

    sec = 360;
    frbytes -= filsiz;
    ds[sec][3] = (char)((frbytes / (sectlen - 3)) % 256);
    ds[sec][4] = (char)((frbytes / (sectlen - 3)) / 256);

/****************    Check and show free sectors    ***************************/
    frsectors = (unsigned char)ds[360][3] + (256 * (unsigned char)(ds[360][4]));        //VOTC
    printf("%i free sectors\n", frsectors);




        /******************      Save disk image          ***********************************/

#if SAVE_DISK_IMAGE
    if ((strcmp(ext, ".XFD") == 0) || (strcmp(ext, ".xfd") == 0))
        header_len = 0;
    else
        header_len = 16;
    if ((filestream = fopen(diskimage_name, "wb")) == NULL) {
        printf("Can't open %s\n", diskimage_name);
        puts("press any key");
        getch();
        exit(1);
    }
    printf("Saving %s", diskimage_name);
    fwrite(header, 1, header_len, filestream);
//      fseek (filestream, header_len , SEEK_SET);                      // Reset file ptr

    if (df == 1)
        secno = 720;
    if (df == 2)
        secno = 1040;
    for (sec = 1; sec <= secno; sec++)
        if (fwrite(&ds[sec][0], sectlen, 1, filestream) != 1)
            puts("disk write error");
    fclose(filestream);
    printf("...... done.\n\n");
#endif

/****************     Create ATR image from XFD image  ******************************************/


#if CREATE_ATR_FROM_XFD
    if (header_len == 0) {

        printf("Saving %s\n\n", atr_filename);


        if (df == 1)
            secno = 720;
        if (df == 2)
            secno = 1040;
        for (sec = 1; sec <= secno; sec++)
            if (fwrite(&ds[sec][0], sectlen, 1, filestream3) != 1)
                puts("disk write error");
        fclose(filestream3);
    }
#endif

/********************************/




}




/*************************************************************************************/

/******************* S U B R O U T I N E S *******************************************/

/*************************************************************************************/





/****************     Subroutine  findfree  ******************************************/

void findfree(int sc) {
    short i, j;
    unsigned char byt, bitcor;

//      short bitcor;



    //sec = 0;
    for (i = mapbyt + sc / 8; i < sectlen; i++) //freies byte suchen
    {
        if (ds[mapsec][i])      //at least one bit is set
            break;
    }

    byt = ds[mapsec][i];
    bitcor = 127;               //%01111111
    for (j = 0; j <= 7; j++)    //freies bit suchen
    {
        if (byt & 128)
            break;
        byt <<= 1;
        bitcor >>= 1;           //%00111111
        bitcor |= 128;          //%10111111
    }

    sec = (i - mapbyt) * 8 + j;
    check_sector(sec);
    ds[mapsec][i] &= bitcor;    //sector legen
    //printf ("sector: %i\t i= %i\tvalue = %i\n",sec,i,ds[mapsec][i]);
}



/******************** error *********************************************/

void err(char *errstr)
{
    printf("\n\n\aERROR: %s\n", errstr);
    getch();
    exit(1);
}

/******************** error *********************************************/

void file_err(char *errstr, char filename[])
{
    printf("\n\n\aERROR: %s %s\n", errstr, filename);
    getch();
    exit(1);
}

/***********************************************************************************/

void check_sector(int sec) {

/*	if (ds[sec][sectlen-3]>>2 != filno)
		{
		printf("\a\nWrite error: file number mismatch in sector %i %i<>%i\n",sec,ds[sec][sectlen-3]>>2, filno);
		getch();
		exit(1);
		}*/

    //printf("%4i",sector_check[sec]);
    if (sector_check[sec]) {
        printf("\a\nCan't write: sector %i is already in use by file %i\n",
               sec, sector_check[sec]);
        getch();
        exit(1);
    }
    if ((sec >= 360) && (sec <= 368)) {
        puts("\aTried to write file sector to sectors 360 to 368 (VTOC and directory");
        getch();
        exit(1);
    }
}
