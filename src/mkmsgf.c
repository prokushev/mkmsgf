/****************************************************************************
 *
 *  mkmsgf.c -- Make Message File Utility (MKMSGF) Clone
 *
 *  ========================================================================
 *
 *    Version 1.1       Michael K Greene <mikeos2@gmail.com>
 *                      September 2023
 *
 *  ========================================================================
 *
 *  Description: Simple clone of the mkmsgf tool.
 *
 *  July 2008 Version 1.0 : Michael K Greene
 *
 *  Based on previous work:
 *      (C) 2002-2008 by Yuri Prokushev
 *      (C) 2001 Veit Kannegieser
 *
 *  ========================================================================
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ***************************************************************************/

/*
   18 May 2024  Things not supported:
       - DBCS
*/

#define INCL_DOSNLS /* National Language Support values */

#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if !defined(_LINUX_SOURCE)
#include <io.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include "mkmsgf.h"
#include "mkmsgerr.h"
#include "version.h"
#include "dlist.h"

#if __WATCOMC__ <= 1290
int getline (char **lineptr, unsigned int *n, FILE *stream);

/* getline.c -- Replacement for GNU C library function getline ()

   Copyright (C) 1992 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */


#define MAX_CANON 64

/* Read up to (and including) a newline from STREAM into *LINEPTR
   (and NUL-terminate it). *LINEPTR is a pointer returned from malloc (or
   NULL), pointing to *N characters of space.  It is realloc'd as
   necessary.  Returns the number of characters read (not including the
   null terminator), or -1 on error or EOF.  */

int
getline (lineptr, n, stream)
  char **lineptr;
  unsigned int *n;
  FILE *stream;
{
  int nchars_avail;
  char *read_pos;

  if (!lineptr || !n || !stream)
    return -1;

  nchars_avail = *n;

  if (!*lineptr)
    {
      if (!(*lineptr = malloc (MAX_CANON)))
	return -1;

      *n = nchars_avail = MAX_CANON;
    }

  read_pos = *lineptr;

  for (;;)
    {
      register char c = getc (stream);

      /* We always want at least one char left in buffer since we
	 always (unless we get an error while reading the first char)
	 NUL-terminate the line buffer. */

      if (nchars_avail < 1)
	{
	  if (*n > MAX_CANON)
	    {
	      nchars_avail = *n;
	      *n *= 2;
	    }
	  else
	    {
	      nchars_avail = MAX_CANON;
	      *n += MAX_CANON;
	    }

	  *lineptr = realloc (*lineptr, *n);
	  read_pos = *lineptr + (*n - nchars_avail);
	}

      /* EOF or error */
      if (feof (stream) || ferror (stream))

	/* Return partial line, if any */
	if (read_pos == *lineptr)
	  return -1;
	else
	  break;

      *read_pos++ = c;
      nchars_avail--;

      /* Return line if NL */
      if (c == '\n')
	break;
    }

  /* Done - NUL terminate and return number of chars read */
  *read_pos = '\0';
  return (*n - nchars_avail);
}

#endif

int parseincludes(MESSAGEINFO *messageinfo);
int setupheader(MESSAGEINFO *messageinfo);
int writemsgfile(MESSAGEINFO *messageinfo);
int writeasmfile(MESSAGEINFO *messageinfo);
int writeheader(MESSAGEINFO *messageinfo);
int DecodeLangOpt(char *dargs, MESSAGEINFO *messageinfo);

// ouput display/helper functions
void usagelong(void);
void prgheading(void);
void helpshort(void);
void helplong(void);
void ProgError(int exnum, char *dispmsg);
void displayinfo(MESSAGEINFO *messageinfo);

int processparams(int argc, char *argv[])
{
    unsigned long rc = 0; // return code
    unsigned long dlrc = 0; // return code
    int ch = 0; // getopt variable

    // input/output file names and options
    // uint8_t os2ldr = 0;           // here but not used
    uint8_t ibm_format_input = 0; // 1= IBM compatabile input args
    uint8_t outfile_provided = 0; // output file in args

    // getopt options
    uint8_t verbose = 0;   // verbose output
    uint8_t dispquiet = 0; // quiet all display - unless error
    uint8_t proclang = 0;  // lang opt processed

    MESSAGEINFO messageinfo; // holds all the info
    messageinfo.langfamilyID = 0;
    messageinfo.langversionID = 0;
    messageinfo.bytesperchar = 1;
    messageinfo.fakeextend = 0;
    messageinfo.include = NULL;
    messageinfo.identifier[3] = 0;
    messageinfo.asm_format_output = 0; // 1= include is ASM INC
    messageinfo.c_format_output = 0; // 1= include is C H

    /* *********************************************************************
     * The following is to just keep the input options getopt and IBM mkmsgf
     * compatable : MKMSGF infile[.ext] outfile[.ext] [/V]
     * why? Because using getopt so it does not have to match old format
     */

    // is the input file first? yes, make compatable with IBM program
    // so if the first option does not start with / or - then assume it
    // is a filename
    if ((*argv[1] != '-') && (*argv[1] != '/')) // first arg prefix - or / ?
    {
        strncpy(messageinfo.infile, argv[optind], sizeof(messageinfo.infile)-1);
        optind++;

        ++ibm_format_input; // set ibm format

        // we know IBM format so check for output file
        if (argc > 2)
        {
            if ((*argv[2] != '-') && (*argv[2] != '/')) // first arg prefix - or / ?
            {
                strncpy(messageinfo.outfile, argv[optind], sizeof(messageinfo.outfile)-1);
                optind++;

                ++outfile_provided; // have output file
            }
        }
    }

    // just cuz - zero out a couple vars
    messageinfo.codepagesnumber = 0;

    // Get program arguments using getopt()
    while ((ch = getopt(argc, argv, "d:D:eEp:P:l:L:VvHhI:i:AaCcQq")) != -1)
    {
        switch (ch)
        {
        case 'd':
        case 'D':
            ProgError(MKMSG_GETOPT_ERROR, "MKMSGF: Sorry, DBCS not supported");
            break;

        case 'e':
        case 'E':
            messageinfo.fakeextend = 1;
            break;

        case 'p':
        case 'P':
            if (messageinfo.codepagesnumber < 16)
            {
                messageinfo.codepages[messageinfo.codepagesnumber++] = atoi(optarg);
            }
            else
                ProgError(MKMSG_GETOPT_ERROR, "MKMSGF: More than 16 codepages entered");
            break;

        case 'l':
        case 'L':
            // only going to process first L option - only one allowed
            if (proclang)
                ProgError(MKMSG_GETOPT_ERROR, "MKMSGF: Syntax error L option");
            proclang = DecodeLangOpt(optarg, &messageinfo);
            break;

        case 'v':
        case 'V':
            if (!verbose)
                ++verbose;
            break;

        case '?':
        case 'h':
        case 'H':
            prgheading();
            usagelong();
            exit(0);
            break;

            // Undocumented IBM flags

        case 'a': // the real mkmsgf outputs asm file and parses .INC files
        case 'A':
			++messageinfo.asm_format_output;
			break;
        case 'i': // include path, only for A and C
        case 'I':
			messageinfo.include=strdup(optarg);
			break;
        case 'c': // the real mkmsgf outputs asm file and parses .H files
        case 'C':
			++messageinfo.c_format_output;
            break;

        // my added option
        case 'q':
        case 'Q':
            if (!dispquiet)
                ++dispquiet;
            break;

        default:
            ProgError(MKMSG_GETOPT_ERROR, "MKMSGF: Syntax error unknown option");
            break;
        }
    }

    // quiet flag - cancels verbose
    if (dispquiet)
        --verbose;

    // check for input file - getopt compatable cmd line
    // we either have in/out files or it is error
    if ((argc == optind) && !ibm_format_input)
        ProgError(MKMSG_NOINPUT_ERROR, "MKMSGF: no input file");

    // if ibm_format_input is false then using new format
    // so we need to get input file and maybe the output file
    if (!ibm_format_input)
    {
        strncpy(messageinfo.infile, argv[optind], sizeof(messageinfo.infile)-1);
        optind++;

        if (argc != optind)
        {
            strncpy(messageinfo.outfile, argv[optind], sizeof(messageinfo.outfile)-1);
            optind++;

            ++outfile_provided; // have output file
        }
    }

    /* 1. Check exist input file
     * 2. Split the input file parts
     * 3. If no output file, generate out file name
     */

    // setup and check the input / output files
    if (access(messageinfo.infile, F_OK) != 0)
	{
		printf(messageinfo.infile);
        ProgError(MKMSG_INPUT_ERROR, "MKMSGF: Input file does not exist.");
	}

    // splitup input file
    _splitpath(messageinfo.infile,
               messageinfo.indrive,
               messageinfo.indir,
               messageinfo.infname,
               messageinfo.inext);

    if (!outfile_provided)
    {
        for (int x = 0; x < _MAX_PATH; x++)
            messageinfo.outfile[x] = 0x00;
		if (messageinfo.asm_format_output||messageinfo.c_format_output)
			sprintf(messageinfo.outfile, "%s%s", messageinfo.infname, ".asm");
		else
        sprintf(messageinfo.outfile, "%s%s", messageinfo.infname, ".msg");
    }
    // check input == output file
    if (!strcmp(messageinfo.infile, messageinfo.outfile))
        ProgError(MKMSG_IN_OUT_COMPARE, "MKMSGF: Input file same as output file");

    // ************ done with args ************

    // decompile header/ input file info
    rc = setupheader(&messageinfo);
    if (rc != MKMSG_NOERROR)
        ProgError(rc, "MKMSGF: MSG header setup error");

    // display info on screen
    displayinfo(&messageinfo);

	if (messageinfo.asm_format_output||messageinfo.c_format_output)
	{
		messageinfo.msgids=CreateList();

		rc = parseincludes(&messageinfo);
		if (rc != MKMSG_NOERROR)
		{
			DestroyList(&messageinfo.msgids, TRUE, &dlrc);
			if (dlrc != DLIST_SUCCESS)
			{
				ProgError(rc, "MKMSGF: DLIST destroy error");
			}
			ProgError(rc, "MKMSGF: INC file read error");
		}
		rc = writeasmfile(&messageinfo);
		if (rc != MKMSG_NOERROR)
		{
			DestroyList(&messageinfo.msgids, TRUE, &dlrc);
			if (dlrc != DLIST_SUCCESS)
			{
				ProgError(rc, "MKMSGF: DLIST destroy error");
			}
			ProgError(rc, "MKMSGF: ASM file write error");
		}
		DestroyList(&messageinfo.msgids, TRUE, &dlrc);
		if (dlrc != DLIST_SUCCESS)
		{
			ProgError(rc, "MKMSGF: DLIST destroy error");
		}
	} else {
    rc = writeheader(&messageinfo);

    if (rc != MKMSG_NOERROR)
        ProgError(rc, "MKMSGF: MSG Header write error");

		rc = writemsgfile(&messageinfo);
		
    if (rc != MKMSG_NOERROR)
		{
        ProgError(rc, "MKMSGF: MSG file write error");
		}
	}

    // if you don't see this then I screwed up
    printf("\nEnd compile\n");

	return(0);
}

/*************************************************************************
 * Main( )
 *
 * Entry into the program
 *
 * Expects a valid txt filey. Will name the output file using the input
 * file and the MSG extention if an output filename is not provided.
 *
 **********************************/

int main(int argc, char *argv[])
{
    // these I found in the disassembled code, here for reference
    // but not used - just for reference
    // uint8_t *mkmsgfprog = getenv("MKMSGF_PROG");
    // if (mkmsgfprog != NULL)
    //    if (!strncmp(mkmsgfprog, "OS2LDR", 6))
    //        os2ldr = 1;

    // no args - print usage and exit
    if (argc == 1)
    {
        prgheading(); // display program heading
        helpshort();
        exit(MKMSG_NOERROR);
    }

	// Control file
	if ((*argv[1] == '@'))
	{
		char *line=NULL;
		unsigned int line_size=sizeof(line);
		char *fakeargv[10];	// 10 args
		char *param;
		int fakeargc;
		FILE* file = fopen(argv[1]+1, "r");

		while (!feof(file))
		{
			getline(&line, &line_size, file);
			if (feof(file)) break;
			printf("%s\n", line);
			flushall();
			enum { kMaxArgs = 64 };
			int fakeargc = 0;
			char *fakeargv[kMaxArgs];
			
			fakeargv[fakeargc++] = argv[0];

			char *p2 = strtok(line, " ");
			while (p2 && fakeargc < kMaxArgs-1)
			{
				fakeargv[fakeargc++] = p2;
				p2 = strtok(0, " ");
			}
			fakeargv[fakeargc] = 0;
			processparams(fakeargc, fakeargv);
			optind=1;
		}
		fclose(file);
        exit(MKMSG_NOERROR);
	}

	// single file
	processparams(argc, argv);
    exit(MKMSG_NOERROR);
}

/*************************************************************************
 * Function:  setupheader( )
 *
 * Gets and stores header info in MESSAGEINFO structure
 *
 * 1. Open input file
 * 2. Read past comments
 * 3. Get identifer and save file read position
 * 1. Get start message number
 * 2. Get number of messages
 * 3. determine index pointer and size uint8/uint32
 *
 * Return:    returns error code or 0 for all good
 *************************************************************************/

int setupheader(MESSAGEINFO *messageinfo)
{
    size_t buffer_size = 0;
    char *read_buffer = NULL;
    char msgnum[5] = {0};
    int first = 1;

    messageinfo->msgstartline = 0;

    // open input file
    FILE *fpi = fopen(messageinfo->infile, "rb");
    if (fpi == NULL)
        return (MKMSG_OPEN_ERROR);

    // get identifer and save, keep track of identifer line
    // number
    while (TRUE)
    {
        size_t n = 0;
        char *line = NULL;

        getline(&line, &n, fpi);

        if (line[0] != ';')
        {
            if (strlen(line) > 5) // identifer (3) + 0x0D 0x0A (2)
                exit(99);
            messageinfo->identifier[0] = line[0];
            messageinfo->identifier[1] = line[1];
            messageinfo->identifier[2] = line[2];

            fgetpos(fpi, &messageinfo->msgstartline);
            break;
        }
    }

    // make sure number of messages is 0
    messageinfo->numbermsg = 0;

    // Get message start number and number of messages
    while (TRUE)
    {
        // this should be the first message line
        getline(&read_buffer, &buffer_size, fpi);

        // if end of file get out of loop
        if (feof(fpi))
            break;

        if (strncmp(messageinfo->identifier, read_buffer, 3) == 0)
        {
            messageinfo->numbermsg++;

            if (first) // first message - get start number
            {
                first = 0;
                sprintf(msgnum, "%c%c%c%c",
                        read_buffer[3],
                        read_buffer[4],
                        read_buffer[5],
                        read_buffer[6]);

                messageinfo->firstmsg = atoi(msgnum);
            }
        }
    }

    fclose(fpi);
    free(read_buffer);

    // calculate whether to use uint16 or uint32 for index
    int handle = open(messageinfo->infile, O_RDONLY | O_TEXT);
    if (handle == -1)
        return (MKMSG_OFFID_ERR);
    else
    {
        if (_filelength(handle) <= 40000) // using 50K as pointer tripwire
            messageinfo->offsetid = 1;
        else
            messageinfo->offsetid = 0;
        close(handle);
    }
    // size in bytes of index
    if (messageinfo->offsetid)
        messageinfo->indexsize = messageinfo->numbermsg * 2;
    else
        messageinfo->indexsize = messageinfo->numbermsg * 4;

    // assign header values for standard v2 MSG file
    messageinfo->version = 0x0002;                     // set version
    messageinfo->hdroffset = 0x001F;                   // header offset
    messageinfo->indexoffset = messageinfo->hdroffset; // okay dup of hdroffset
    messageinfo->reserved[0] = 0x4D;                   // put this in to mark MKG clone compiled
    messageinfo->reserved[1] = 0x4B;
    messageinfo->reserved[2] = 0x47;
    messageinfo->reserved[3] = 0x00;
    messageinfo->reserved[4] = 0x00;

    // hdrsize/offset + the size of index will locate the country info
    messageinfo->countryinfo = messageinfo->hdroffset +
                               messageinfo->indexsize;

    // messages start after the FILECOUNTRYINFO block
    messageinfo->msgoffset = messageinfo->countryinfo +
                             sizeof(FILECOUNTRYINFO);

    // remains 0 for now
    messageinfo->extenblock = 0;

    // TEMP stuff
    strncpy(messageinfo->filename,
            messageinfo->outfile,
            sizeof(messageinfo->filename)-1);
    messageinfo->country = 0;

    return (MKMSG_NOERROR);
}

/*************************************************************************
 * Function:  writeasmfile( )
 *
 * Reads in all the MSG file info and stores in MESSAGEINFO structure
 *
 * 1 Open input file in read and out put file in update mode
 * 2 Allocate read buffer
 * 3 Set input file positions
 * 4 *** start main loop ***
 * 4.1 Clear read buffer
 * 4.2 Get current read poistion and store in index
 * 4.3 Read in message line
 * 4.4 Check for comment - if true read next line -> 5.1
 * 4.5 Line starts with msg ID? Yes new message else -> 5.9
 * 4.6 Check if valid msg type - E, I, W, H, P, ? --> error if not
 * 4.7 Increment message tracking number
 * 4.8.1 Check for ? message if true then generate full ? message to save
 *     time else process E, I, W, H, P msg types and set scratch pointer
 * 4.8.2.1 Check for the mandatory space after : exit if not present
 * 4.8.2.2 Move msg type to space and set scratch pointer
 * 4.9 Continuation line -- set scratch pointer
 *
 * 5 Get message length
 * 6 Check/fix 0x0D 0x0A ending of line
 * 7 Check and fix for %0 lines
 * 8 Write message line to output file
 * ** end main loop
 * 9 Seek to index start and write index
 *
 *
 * Return:    returns error code or 0 for all good
 *************************************************************************/
typedef
struct tagParam
{
	int num;
	FILE * f;
	int type;
	int skip;
} Param;

void handleitem(ADDRESS Object, TAG ObjectTag, CARDINAL32 ObjectSize, ADDRESS ObjectHandle, ADDRESS Parameters, CARDINAL32 * Error)
{
	
	
	if (((Param *)Parameters)->num==ObjectTag)
	{
		char line[81]={0};
		if (((Param *)Parameters)->type==0)
		  fprintf(((Param *)Parameters)->f, "\tPUBLIC TXT_%s\r\nTXT_%s\tLABEL\tWORD\r\n", Object, Object);
		if ((((Param *)Parameters)->type==1)&(((Param *)Parameters)->skip==0))
		{
		  fprintf(((Param *)Parameters)->f, "\tDW\tEND_%s - TXT_%s - 2\r\n", Object, Object);
		  ((Param *)Parameters)->skip=1;
		}
		if ((((Param *)Parameters)->type==2)&(((Param *)Parameters)->skip==0))
		{
		  fprintf(((Param *)Parameters)->f, "END_%s\tLABEL\tWORD\r\n\tDB\t0\n\r", Object);
		  ((Param *)Parameters)->skip=1;
		}
	}

}

int writeasmfile(MESSAGEINFO *messageinfo)
{
    // open input file
    FILE *fpi = fopen(messageinfo->infile, "rb");
    if (fpi == NULL)
        return (MKMSG_OPEN_ERROR);

    // write output file open for write
    FILE *fpo = fopen(messageinfo->outfile, "wb");
    if (fpo == NULL)
        return (MKMSG_OPEN_ERROR);

    // buffer to read in a message - use a 256 byte buffer which
    // is overkill but not a big deal - I do not expect lines >256
    // in length
    size_t read_buff_size = 0;
    char *read_buffer = (char *)calloc(256, sizeof(char));
    if (read_buffer == NULL)
        return (MKMSG_MEM_ERROR2);

    // return to previous position
    fsetpos(fpi, &messageinfo->msgstartline); // input after id line

    int current_msg_len = 0;
    int msg_num_check = messageinfo->firstmsg;
    char *readptr = NULL;

    while (TRUE)
    {
        // clear the read_buffer -- set all to 0x00 this will
        // give me a clean strlen return
        memset(read_buffer, 0x00, _msize(read_buffer));

        // here is the line read in
        getline(&read_buffer, &read_buff_size, fpi);
        if (feof(fpi))
            break;

        // find message start - skip comments
        // this is the main loop if not a comment line
        if (read_buffer[0] != ';')
        {
            // check if ID which indicates message start
            if (strncmp(messageinfo->identifier, read_buffer, 3) == 0)
            {
                // First check - is the message type valid
                if (read_buffer[7] != 'E' && read_buffer[7] != 'H' &&
                    read_buffer[7] != 'I' && read_buffer[7] != 'P' &&
                    read_buffer[7] != 'W' && read_buffer[7] != '?')
                {
                    fclose(fpi);
                    fclose(fpo);
                    free(read_buffer);
                    ProgError(MKMSG_BAD_TYPE, "MKMSGF: Bad message type.");
                }


                // shortcut and make sure the format is correct
                // for ? messages
                if (read_buffer[7] == '?')
                {
                    memset(read_buffer, 0x00, _msize(read_buffer));
                    read_buffer[0] = 0x0D;
                    read_buffer[1] = 0x0A;
                    read_buffer[2] = 0x00;

                    readptr = read_buffer;
                }
                else
                {
                    // check if followed instructions with a space after colon
                    // I assume this was why (maybe not) -- just copy the ID
                    // to the space position and change pointer to the new start
                    if (read_buffer[9] != 0x20)
                        return (MKMSG_BAD_TYPE);
                    else
                    {
                        // move message type to front of message
                        // and set buffer position
                        //read_buffer[9] = read_buffer[7];
                        readptr = &read_buffer[10];
						//readptr = read_buffer;
                    }
                }
            }
            else // no ID - continues previous line
            {
                // this is a continuation line so just
                // set the readptr
                readptr = read_buffer;
            }

            current_msg_len = strlen(readptr);

            // Second check - check and setup correct message ending
            // - the message ending needs to be 0x0D 0x0A, if the text
            // input file was done in a modern text editor the ending
            // is probably just 0x0A so add 0x0D 0x0A 0x00

            if (readptr[(current_msg_len - 1)] != 0x0A &&
                readptr[(current_msg_len - 2)] != 0x0D)
            {
                readptr[(current_msg_len - 1)] = 0x0D;
                readptr[(current_msg_len)] = 0x0A;
                readptr[(current_msg_len + 1)] = 0x00;

                // new length
                current_msg_len = strlen(readptr);
            }

            // At this point the message is ready in reference to where
            // readptr points and each line ends 0x0D 0x0A 0x00

            // Third and final check - fix end if it is a %0 line
            // if ends in [ % 0 0x0D 0x0A 0x00] remove all four
            // poistions and make 0x00 - all four just as a easy
            // way to spot problems in a hex dump
            if (readptr[(current_msg_len - 3)] == '0' &&
                readptr[(current_msg_len - 4)] == '%')
            {
                readptr[(current_msg_len - 1)] = 0x00;
                readptr[(current_msg_len - 2)] = 0x00;
                readptr[(current_msg_len - 3)] = 0x00;
                readptr[(current_msg_len - 4)] = 0x00;

                // new length -- up to first 0x00
                current_msg_len = strlen(readptr);
            }

			// Write out message labels
			Param p;
			unsigned long rc;
			p.num=msg_num_check;
			p.f=fpo;
			p.type=0;
			ForEachItem(messageinfo->msgids,
				&handleitem,
				(ADDRESS)&p,
				TRUE,
				&rc);

			// Write out message length
			p.type=1;
			p.skip=0;
			ForEachItem(messageinfo->msgids,
				&handleitem,
				(ADDRESS)&p,
				TRUE,
				&rc);

            // write out the current message
			fprintf(fpo, "\tDB\t'%c%c%c%04d: '\r\n\tDB\t'",
			messageinfo->identifier[0], messageinfo->identifier[1],
			messageinfo->identifier[2], msg_num_check);
			int outlen=1;
            while (*readptr)
			{
				if (outlen>ASM_MSG_SIZE) 
				{
					fprintf(fpo,"'\r\n\tDB\t'");
					outlen=0;
				}

				if (strncmp("\r\n", readptr, 2) == 0)				
				{
					fprintf(fpo, "', 0DH, 0AH\r\n");
					readptr++;
					outlen=-1;
				}
				else 
				{
					fputc(*readptr, fpo);
				}
				readptr++;
				outlen++;
			}
			if (outlen) fprintf(fpo,"'\r\n");

			// Write out message end label and NULL
			p.type=2;
			p.skip=0;
			ForEachItem(messageinfo->msgids,
				&handleitem,
				(ADDRESS)&p,
				TRUE,
				&rc);

			msg_num_check++;
        }
    }


    printf("Done\n");

    // close up and get out
    fclose(fpo);
    fclose(fpi);
    free(read_buffer); 

    return (MKMSG_NOERROR);
}

/*************************************************************************
 * Function:  writemsgfile( )
 *
 * Reads in all the MSG file info and stores in MESSAGEINFO structure
 *
 * 1 Open input file in read and out put file in update mode
 * 2 Allocate index and read buffer
 * 3 Set input and output file positions
 * 4 Set index pointer based on uint8/uint32 sizes
 * 5 *** start main loop ***
 * 5.1 Clear read buffer
 * 5.2 Get current read poistion and store in index
 * 5.3 Read in message line
 * 5.4 Check for comment - if true read next line -> 5.1
 * 5.5 Line starts with msg ID? Yes new message else -> 5.9
 * 5.6 Check if valid msg type - E, I, W, H, P, ? --> error if not
 * 5.7 Increment message tracking number
 * 5.8.1 Check for ? message if true then generate full ? message to save
 *     time else process E, I, W, H, P msg types and set scratch pointer
 * 5.8.2.1 Check for the mandatory space after : exit if not present
 * 5.8.2.2 Move msg type to space or colon and set scratch pointer
 * 5.9 Continuation line -- set scratch pointer
 *
 * 6 Get message length
 * 7 Check/fix 0x0D 0x0A ending of line
 * 8 Check and fix for %0 lines
 * 9 Write message line to output file
 * ** end main loop
 * 10 Seek to index start and write index
 *
 *
 * Return:    returns error code or 0 for all good
 *************************************************************************/

int writemsgfile(MESSAGEINFO *messageinfo)
{
    // open input file
    FILE *fpi = fopen(messageinfo->infile, "rb");
    if (fpi == NULL)
        return (MKMSG_OPEN_ERROR);

    // write output file open for update
    FILE *fpo = fopen(messageinfo->outfile, "r+b");
    if (fpo == NULL)
        return (MKMSG_OPEN_ERROR);

    // buffer to read in index - this reserves memory of the
    // size calculated earlier to hold the index which is
    // dumped to the file at end of message write
    char *index_buffer = (char *)calloc(messageinfo->indexsize, sizeof(char));
    if (index_buffer == NULL)
        return (MKMSG_MEM_ERROR1);

    // buffer to read in a message - use a 256 byte buffer which
    // is overkill but not a big deal - I do not expect lines >256
    // in length
    size_t read_buff_size = 0;
    char *read_buffer = (char *)calloc(256, sizeof(char));
    if (read_buffer == NULL)
        return (MKMSG_MEM_ERROR2);

    // return to previous position
    fsetpos(fpi, &messageinfo->msgstartline); // input after id line
    fsetpos(fpo, &messageinfo->msgoffset);    // move to start msg area

    int msg_num_check = 0;
    int current_msg_len = 0;
    char *readptr = NULL;
    fpos_t index_position;

    // index pointers
    uint16_t *small_index = NULL; // used if index pointers uint16
    uint32_t *large_index = NULL; // used if index pointers uint32

    // pick the pointer based on index uint16 or uint32
    if (messageinfo->offsetid)
        small_index = (uint16_t *)index_buffer;
    else
        large_index = (uint32_t *)index_buffer;

    while (!feof(fpi))
    {
        // clear the read_buffer -- set all to 0x00 this will
        // give me a clean strlen return
        memset(read_buffer, 0x00, _msize(read_buffer));

        // get current position in outfile -- onentry will be
        // set to first message after fsetpos above
        fgetpos(fpo, &index_position);

        // handle the uint16 and uint32 index differences
        if (messageinfo->offsetid)
            small_index[msg_num_check] = (uint16_t)index_position;
        else
            large_index[msg_num_check] = (uint32_t)index_position;

        // here is the line read in
        getline(&read_buffer, &read_buff_size, fpi);
        if (!getline(&read_buffer, &read_buff_size, fpi) & feof(fpi))
            break;

        // find message start - skip comments
        // this is the main loop if not a comment line
        if (read_buffer[0] != ';')
        {
            // check if ID which indicates message start
            if (strncmp(messageinfo->identifier, read_buffer, 3) == 0)
            {
                // First check - is the message type valid
                if (read_buffer[7] != 'E' && read_buffer[7] != 'H' &&
                    read_buffer[7] != 'I' && read_buffer[7] != 'P' &&
                    read_buffer[7] != 'W' && read_buffer[7] != '?')
                {
                    fclose(fpi);
                    fclose(fpo);
                    free(read_buffer);
                    free(index_buffer);
                    ProgError(1, "MKMSGF: Bad message type."); // fix error
                }

                // keep track of messages processed
                msg_num_check += 1;

                // shortcut and make sure the format is correct
                // for ? messages
                if (read_buffer[7] == '?')
                {
                    memset(read_buffer, 0x00, _msize(read_buffer));
                    read_buffer[0] = '?';
                    read_buffer[1] = 0x0D;
                    read_buffer[2] = 0x0A;
                    read_buffer[3] = 0x00;

                    readptr = read_buffer;
                }
                else
                {
                    // check if followed instructions with a space after colon
                    // I assume this was why (maybe not) -- just copy the ID
                    // to the space position and change pointer to the new start
                    if (read_buffer[9] != 0x20)
					{
                        read_buffer[8] = read_buffer[7];
                        readptr = &read_buffer[8];
//                        return (MKMSG_BAD_TYPE);
					}
                    else
                    {
                        // move message type to front of message
                        // and set buffer position
                        read_buffer[9] = read_buffer[7];
                        readptr = &read_buffer[9];
                    }
                }
            }
            else // no ID - continues previous line
            {
                // this is a continuation line so just
                // set the readptr
                readptr = read_buffer;
            }

            current_msg_len = strlen(readptr);

            // Second check - check and setup correct message ending
            // - the message ending needs to be 0x0D 0x0A, if the text
            // input file was done in a modern text editor the ending
            // is probably just 0x0A so add 0x0D 0x0A 0x00

            if (current_msg_len && readptr[(current_msg_len - 1)] != 0x0A &&
                readptr[(current_msg_len - 2)] != 0x0D)
            {
                readptr[(current_msg_len - 1)] = 0x0D;
                readptr[(current_msg_len)] = 0x0A;
                readptr[(current_msg_len + 1)] = 0x00;

                // new length
                current_msg_len = strlen(readptr);
            }

            // At this point the message is ready in reference to where
            // readptr points and each line ends 0x0D 0x0A 0x00

            // Third and final check - fix end if it is a %0 line
            // if ends in [ % 0 0x0D 0x0A 0x00] remove all four
            // poistions and make 0x00 - all four just as a easy
            // way to spot problems in a hex dump
            if (readptr[(current_msg_len - 3)] == '0' &&
                readptr[(current_msg_len - 4)] == '%')
            {
                readptr[(current_msg_len - 1)] = 0x00;
                readptr[(current_msg_len - 2)] = 0x00;
                readptr[(current_msg_len - 3)] = 0x00;
                readptr[(current_msg_len - 4)] = 0x00;

                // new length -- up to first 0x00
                current_msg_len = strlen(readptr);
            }

            // write out the current message
            fwrite(readptr, sizeof(char), current_msg_len, fpo);
			
        }
    }

    // position to index start and write index -- I could
    // used fwrite - just decided to write index like this:
    fseek(fpo, messageinfo->indexoffset, SEEK_SET);
    for (int x = 0; x < messageinfo->indexsize; x++)
        fputc(index_buffer[x], fpo);

    // check the wiki for a description of the extended
    // header -- add header if passed -e option
    // So this is step by step just to make sure it
    // is correct - there is nothing like doing somethng
    // not needed and screwing (bug) the entire program
    if (messageinfo->fakeextend)
    {
        // close
        fclose(fpo);

        // write output file open for append
        FILE *fpo = fopen(messageinfo->outfile, "r+b");
        if (fpo == NULL)
            return (MKMSG_OPEN_ERROR);

        // move to end of file
        fseek(fpo, 0L, SEEK_END);

        // save location
        uint32_t extenblock = (uint32_t)ftell(fpo);

        // tack on the fake ext header
        for (int x = 0; x < 4; x++)
            fputc(extfake[x], fpo);

        fclose(fpo);

        // write output file open for update
        fpo = fopen(messageinfo->outfile, "r+b");
        if (fpo == NULL)
            return (MKMSG_OPEN_ERROR);

        // move to position in header
        fseek(fpo, 0x16L, SEEK_SET);

        // write out extenblock
        fwrite(&extenblock, sizeof(uint32_t), 1, fpo);
    }

    printf("Done\n");

    // close up and get out
    fclose(fpo);
    fclose(fpi);
    free(read_buffer);
    free(index_buffer);  // why it traps???

    return (MKMSG_NOERROR);
}


int writecountryblock(MESSAGEINFO *messageinfo, FILE * fpo)
{
    FILECOUNTRYINFO *cntryheader = NULL;

    // generate and write empty country block
    char *write_buffer = (char *)calloc(1, sizeof(FILECOUNTRYINFO));
    if (write_buffer == NULL)
        return (MKMSG_MEM_ERROR5);

    memset(write_buffer, 0x00, _msize(write_buffer));
    cntryheader = (FILECOUNTRYINFO *)write_buffer;

    cntryheader->bytesperchar = messageinfo->bytesperchar;
    cntryheader->country = messageinfo->country;
    cntryheader->langfamilyID = messageinfo->langfamilyID;
    cntryheader->langversionID = messageinfo->langversionID;
    cntryheader->codepagesnumber = messageinfo->codepagesnumber;

    for (int x = 0; x < cntryheader->codepagesnumber; x++)
        cntryheader->codepages[x] = messageinfo->codepages[x];

    strncpy(cntryheader->filename,
            messageinfo->filename,
            sizeof(cntryheader->filename)-1);

    cntryheader->filler = 0x00;

    fwrite(cntryheader, sizeof(char), sizeof(FILECOUNTRYINFO), fpo);

    free(write_buffer);

    return (0);
}

/*************************************************************************
 * Function:  writeheader( )
 *
 * Reads in all the MSG file info and stores in MESSAGEINFO structure
 *
 * 1 Open input file in read and out put file in update mode
 * 2 Allocate index and read buffer
 * 3 Set input and output file positions
 * 4 Set index pointer based on uint8/uint32 sizes
 *
 *
 * Return:    returns error code or 0 for all good
 *************************************************************************/

int writeheader(MESSAGEINFO *messageinfo)
{
    MSGHEADER *msgheader = NULL;

    // write output file open for append
    FILE *fpo = fopen(messageinfo->outfile, "wb");
    if (fpo == NULL)
        return (MKMSG_OPEN_ERROR);

    // buffer to write in a message - start with a 80 size buffer
    // if for some reason bigger is needed realloc latter
    char *write_buffer = (char *)calloc(messageinfo->hdroffset, sizeof(char));
    if (write_buffer == NULL)
        return (MKMSG_MEM_ERROR3); // fix

    msgheader = (MSGHEADER *)write_buffer;

    // load MKMSG signature
    for (int x = 0; x < 8; x++)
        msgheader->magic_sig[x] = signature[x];

    for (int x = 0; x < 3; x++)
        msgheader->identifier[x] = messageinfo->identifier[x];

    msgheader->numbermsg = messageinfo->numbermsg;
    msgheader->firstmsg = messageinfo->firstmsg;
    msgheader->offset16bit = messageinfo->offsetid;
    msgheader->version = messageinfo->version;
    msgheader->hdroffset = messageinfo->hdroffset;
    msgheader->countryinfo = messageinfo->countryinfo;
    msgheader->extenblock = messageinfo->extenblock;

    for (int x = 0; x < 3; x++)
        msgheader->reserved[x] = messageinfo->reserved[x];

    fwrite(msgheader, sizeof(char), messageinfo->hdroffset, fpo);

    // generate and write empty index
    write_buffer = (char *)realloc(write_buffer, messageinfo->indexsize);
    if (write_buffer == NULL)
        return (MKMSG_MEM_ERROR4);

    memset(write_buffer, 0x00, _msize(write_buffer));
    fwrite(write_buffer, sizeof(char), messageinfo->indexsize, fpo);

	writecountryblock(messageinfo, fpo);

    fclose(fpo);
    free(write_buffer);

    return (0);
}

int parseincfile(MESSAGEINFO *messageinfo, char *s)
{
	FILE *fpi;
	char *line=NULL;
	unsigned int line_size=sizeof(line);
	char id[81]={0};
	char equ[4]={0};
	int num;
	unsigned long rc;

    // open input file
    fpi = fopen(s, "rb");
    if (fpi == NULL)
        return (MKMSG_OPEN_ERROR);

	while (!feof(fpi))
	{
		getline(&line, &line_size, fpi);
		if (feof(fpi)) break;
		if (line_size)
		{
			if (3==sscanf(line, "%80s %3s %d", id, equ, &num ))
			{
				InsertItem (messageinfo->msgids,
							81,
							id,
							num,
							NULL,
							AppendToList,
							FALSE,
							&rc);
			}
		}
	}

	fclose(fpi);
    return (0);
}

int parsehfile(MESSAGEINFO *messageinfo, char *s)
{
	FILE *fpi;
	char *line=NULL;
	unsigned int line_size=sizeof(line);
	char id[81]={0};
	char define[10]={0};
	int num;
	unsigned long rc;

    // open input file
    fpi = fopen(s, "rb");
    if (fpi == NULL)
        return (MKMSG_OPEN_ERROR);

	while (!feof(fpi))
	{
		getline(&line, &line_size, fpi);
		if (feof(fpi)) break;
		if (line_size)
		{
			if (3==sscanf(line, "%9s %80s %d", define, id, &num ))
			{
				InsertItem (messageinfo->msgids,
							81,
							id,
							num,
							NULL,
							AppendToList,
							FALSE,
							&rc);
			}
		}
	}

	fclose(fpi);
    return (0);
}

/*************************************************************************
 * Function:  parseincludes( )
 *
 * Reads in all the INC and H files info and stores in MESSAGEINFO structure
 *
 * 1 Search predefined include files in INCLUDE and current directory
 *
 *
 * Return:    returns error code or 0 for all good
 *************************************************************************/

int parseincludes(MESSAGEINFO *messageinfo)
{
	char dup[1024] = {0};
	char filename[_MAX_PATH]={0};
	char *ev;
	
	if (messageinfo->include) strcat(dup, messageinfo->include);
	ev=getenv("INCLUDE");
	if (ev) strcat(dup, ";");
	if (ev) strcat(dup, ev);
	if (!strlen(dup))
	{
		strcat(dup, ".");
	}

	char *s = dup;
	char *p = NULL;

	if (dup)
	do {
		p = strchr(s, ';');
		if (p != NULL) {
			p[0] = 0;
		}
		
		char searchfiles[2][8+3+1+1];
		
		if (messageinfo->asm_format_output)
		{
			strcpy(searchfiles[0], "BASEMID.INC");
			strcpy(searchfiles[1], "UTILMD*.INC");
		}

		if (messageinfo->c_format_output)
		{
			strcpy(searchfiles[0], "BASEMID.H");
			strcpy(searchfiles[1], "UTILMD*.H");
		}

		for (size_t i = 0; i < sizeof(searchfiles) / sizeof(searchfiles[0]); i++)
		{
			filename[0]=0;
			strcat(filename, s);
			strcat(filename, "\\");
			strcat(filename, searchfiles[i]);

			printf("test=%s\n",filename);
			struct _finddata_t c_file;
			int hFile;

			if( (hFile = _findfirst( filename, &c_file )) == -1L )
			{
				do {
					char buffer[30];

					if (messageinfo->asm_format_output)	parseincfile(messageinfo, c_file.name);
					if (messageinfo->c_format_output)	parsehfile(messageinfo, c_file.name);
				} while( _findnext( hFile, &c_file ) == 0 );
			_findclose( hFile );
			}
		}
   
		s = p + 1;
	} while (p != NULL);

    return (0);
}

/* DecodeLangOpt( )
 *
 * get and check cmd line /L option
 */
int DecodeLangOpt(char *dargs, MESSAGEINFO *messageinfo)
{
    messageinfo->langfamilyIDcode = 0;

    if (strchr(dargs, ',') == NULL)
    {
        ProgError(-1, "MKMSGF: No sub id using 1 default");
        messageinfo->langversionID = 1;
        messageinfo->langfamilyID = atoi(dargs);
    }
    else
    {
        messageinfo->langfamilyID = atoi(strtok(dargs, ","));
        messageinfo->langversionID = atoi(strtok(NULL, ","));
    }

    // Language family > 1 and < 35
    if (messageinfo->langfamilyID < 1 || messageinfo->langfamilyID > 34)
        return (MKMSG_LANG_OUT_RANGE);

    for (int i = 1; i < 35; i++)
    {
        if (langinfo[i].langfam == messageinfo->langfamilyID)
            if (langinfo[i].langsub == messageinfo->langversionID)
                messageinfo->langfamilyIDcode = i;
    }

    if (!messageinfo->langfamilyIDcode)
        return (MKMSG_SUBID_OUT_RANGE);

    return 1;
}

/*
 * User message functions
 */
void usagelong(void)
{
    helpshort();
    helplong();
}

void helpshort(void)
{
    printf("\nMKMSGF infile[.ext] outfile[.ext] [-V]\n");
    printf("[-D <DBCS range or country>] [-P <code page>] [-L <language id,sub id>]\n");
}

void helplong(void)
{
    printf("\nUse MKMSGF as follows:\n");
    printf("        MKMSGF <inputfile> <outputfile> [/V]\n");
    printf("                [/D <DBCS range or country>] [/P <code page>]\n");
    printf("                [/L <language family id,sub id>]\n");
    printf("        where the default values are:\n");
    printf("           code page  -  none\n");
    printf("           DBCS range -  none\n");
    printf("        A valid DBCS range is: n10,n11,n20,n21,...,nn0,nn1\n");
    printf("        A single number is taken as a DBCS country code.\n");
    printf("        The valid OS/2 language/sublanguage ID values are:\n\n");
    printf("\tLanguage ID:\n");
    printf("\tCode\tFamily\tSub\tLanguage\tPrincipal country\n");
    printf("\t----\t------\t---\t--------\t-----------------\n");
    for (int i = 0; langinfo[i].langfam != 0; i++)
        printf("\t%s\t%d\t%d\t%-20s\t%s\n", langinfo[i].langcode,
               langinfo[i].langfam, langinfo[i].langsub, langinfo[i].lang, langinfo[i].country);
}

void prgheading(void)
{
    printf("\nOperating System/2 Make Message File Utility (MKMSGF) Clone\n");
    printf("Version %s  Michael Greene <mikeos2@gmail.com>\n", SYSLVERSION);
    printf("Compiled with Open Watcom %d.%d  %s\n", OWMAJOR, OWMINOR, __DATE__);
}

/*************************************************************************
 * Function:  displayinfo()
 *
 * Display MESSAGEINFO data to screen
 *
 *************************************************************************/

void displayinfo(MESSAGEINFO *messageinfo)
{
    printf("\n*********** Header Info ***********\n\n");

    printf("Input filename         %s\n", messageinfo->infile);
    printf("Component Identifier:  %c%c%c\n", messageinfo->identifier[0],
           messageinfo->identifier[1], messageinfo->identifier[2]);
    printf("Number of messages:    %d\n", messageinfo->numbermsg);
    printf("First message number:  %d\n", messageinfo->firstmsg);
    printf("OffsetID:              %d  (Offset %s)\n", messageinfo->offsetid,
           (messageinfo->offsetid ? "uint16_t" : "uint32_t"));
    printf("MSG File Version:      %d\n", messageinfo->version);
    printf("Header offset:         0x%02X (%d)\n",
           messageinfo->hdroffset, messageinfo->hdroffset);
    printf("Country Info:          0x%02X (%d)\n",
           messageinfo->countryinfo, messageinfo->countryinfo);
    printf("Extended Header:       0x%02X (%lu)\n",
           messageinfo->extenblock, messageinfo->extenblock);
    printf("Reserved area:         ");
    for (int x = 0; x < 5; x++)
        printf("%02X ", messageinfo->reserved[x]);
    printf("\n");

    if (messageinfo->version == 2)
    {
        printf("\n*********** Country Info  ***********\n\n");
        printf("Bytes per character:       %d\n", messageinfo->bytesperchar);
        printf("Country Code:              %d\n", messageinfo->country);
        printf("Language family ID:        %d\n", messageinfo->langfamilyID);
        printf("Language version ID:       %d\n", messageinfo->langversionID);
        printf("Number of codepages:       %d\n", messageinfo->codepagesnumber);
        for (int x = 0; x < messageinfo->codepagesnumber; x++)
            printf("0x%02X (%d)  ", messageinfo->codepages[x], messageinfo->codepages[x]);
        printf("\n");
        printf("File name:                 %s\n\n", messageinfo->filename);
        if (messageinfo->extenblock)
        {
            printf("** Has an extended header **\n");
            printf("Ext header length:        %d\n", messageinfo->extlength);
            printf("Number ext blocks:        %d\n\n", messageinfo->extnumblocks);
        }
        else
            printf("** No an extended header **\n\n");
    }
    return;
}

/* ProgError( )
 *
 * stardard message print
 *
 * if exnum < 0 then print heading (if not yet displayed) and
 * return.
 * else do the above and exit
 *
 */

void ProgError(int exnum, char *dispmsg)
{
    char buffer[80] = {0};

    sprintf(buffer, "\n%s (%d)\n", dispmsg, exnum);

    if (exnum < 0)
    {
        printf("%s", buffer);
        return;
    }
    else
    {
        helpshort();
        printf("%s", buffer);
        exit(exnum);
    }
}
