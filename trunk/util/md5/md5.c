/* MD5.C - main file
 */

/* Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
rights reserved.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */
#ifndef MD
#define MD 5
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "md5_global.h"
#include "md5.h"

/* Length of test block, number of test blocks.
 */
#define TEST_BLOCK_LEN 1000
#define TEST_BLOCK_COUNT 1000

static void MDString PROTO_LIST ((unsigned char[16], char *));
static void MDTimeTrial PROTO_LIST ((void));
void MDFile PROTO_LIST ((unsigned char[16], char *));
static void MDPartOfFile PROTO_LIST ((unsigned char[16], char *, int, int));
static void MDFilter PROTO_LIST ((void));
static void MDPrint PROTO_LIST ((unsigned char[16]));

/* Main driver.

Arguments (may be any combination):
  -sstring - digests string
  -t       - runs time trial
  filename - digests file
  (none)   - digests standard input
 */
#if 0
int main (int argc, char *argv[]) {
    int i;
    unsigned char digest[16];

    if (argc > 1)
    {
	for (i = 1; i < argc; i++) 
        {
	    if (argv[i][0] == '-' && argv[i][1] == 's') 
            {
		MDString (&digest, argv[i] + 2);
                /*
                printf ("\nMD%d (\"%s\") = ", MD, argv[i] + 2);
                MDPrint (digest);
                printf ("\n");
                */
            }
	    else if (strcmp (argv[i], "-t") == 0)
	        MDTimeTrial ();
	    else 
            {
		MDFile (&digest, argv[i]);
                //MDPartOfFile (&digest, argv[i], 1000, 1003);
                
                //printf ("\nMD%d (%s) = ", MD, argv[i]);
                //fprintf (stderr, "xxxx %02x\n", digest[0]);
                unsigned int lol;
                char ans[32];
                for (lol = 0; lol < 16; lol++)
                {
                 //   fprintf (stderr, "Wrote %02x\n", digest[lol]);
                    sprintf (ans+2*lol, "%02x", digest[lol]);
                }
                #if 1
                fprintf (stderr, "\n===================\n");
                fprintf (stderr, "ANS : %s\n", ans);
                fprintf (stderr, "\n====================\n");
                MDPrint (digest);
                printf ("\n");
                #endif
            }
        }
    }
    else
        MDFilter ();

    return (0);
}
#endif
/* Digests a string and puts the result into digest[16].
 */
static void MDString (unsigned char digest[16], char *string) {
    MD5_CTX context;
    unsigned int len = strlen (string);

    MD5Init (&context);
    MD5Update (&context, string, len);
    MD5Final (digest, &context);

    /*
    printf ("MD%d (\"%s\") = ", MD, string);
    MDPrint (digest);
    printf ("\n");
    */
}

/* Measures the time to digest TEST_BLOCK_COUNT TEST_BLOCK_LEN-byte
  blocks.
 */
static void MDTimeTrial () {
    MD5_CTX context;
    time_t endTime, startTime;
    unsigned char block[TEST_BLOCK_LEN], digest[16];
    unsigned int i;
    printf
	("MD%d time trial. Digesting %d %d-byte blocks ...", MD,
	 TEST_BLOCK_LEN, TEST_BLOCK_COUNT);

    /* Initialize block */
    for (i = 0; i < TEST_BLOCK_LEN; i++)
	    block[i] = (unsigned char) (i & 0xff);

    /* Start timer */
    time (&startTime);

    /* Digest blocks */
    MD5Init (&context);
    for (i = 0; i < TEST_BLOCK_COUNT; i++)
	    MD5Update (&context, block, TEST_BLOCK_LEN);
    MD5Final (digest, &context);

    /* Stop timer */
    time (&endTime);

    printf (" done\n");
    printf ("Digest = ");
    MDPrint (digest);
    printf ("\nTime = %ld seconds\n", (long) (endTime - startTime));
    printf
	("Speed = %ld bytes/second\n",
	 (long) TEST_BLOCK_LEN * (long) TEST_BLOCK_COUNT / (endTime -
							    startTime));
}

/* Digests an entire file and puts the result into digest.
 */
void MDFile (unsigned char digest[16], char *filename) {
    FILE *file;
    MD5_CTX context;
    int len;
    unsigned char buffer[1024];

    if ((file = fopen (filename, "rb")) == NULL)
	    printf ("%s can't be opened\n", filename);

    else {
	    MD5Init (&context);
	    while ((len = fread (buffer, 1, 1024, file)) != 0)
	        MD5Update (&context, buffer, len);
	    MD5Final (digest, &context);

	    fclose (file);
            /*
	    printf ("MD%d (%s) = ", MD, filename);
	    MDPrint (digest);
	    printf ("\n");
            */
        
    }
}

/* Digests a file between 'beginning' and 'end' octets and puts the result into digest.
 */
static void MDPartOfFile (unsigned char digest[16], char *filename, int beginning, int end) {
    FILE *file;
    MD5_CTX context;
    int len;
    unsigned char buffer[1024];
    int size = end - beginning;

    if ((file = fopen (filename, "rb")) == NULL) {
	    perror ("file can't be opened\n");
        exit (EXIT_FAILURE);
        }
    else if (size < 0) {
        perror ("Negative size");
        exit (EXIT_FAILURE);
    }
    else {
	    MD5Init (&context);
        
        /* Puts cursor at the beginning octet */
        while (beginning > 1024) {
            if ((len = fread (buffer, 1, 1024, file)) == 0) {
                perror ("End of file");
                exit (EXIT_FAILURE);
            }
            beginning -= 1024;
        }
        if ((len = fread (buffer, 1, beginning-1, file)) == 0) {
            perror ("End of file");
            exit (EXIT_FAILURE);
        }

        /* Reads end-beginning octets */
        while (size > 1024) {
            if ((len = fread(buffer, 1, 1024, file)) == 0) {
                perror ("End of file");
                exit (EXIT_FAILURE);
                }
            MD5Update (&context, buffer, len);
            size -= 1024;
        }
        if ((len = fread (buffer, 1, size, file)) == 0) {
            perror ("End of file before end");
            exit (EXIT_FAILURE);
        }

        MD5Update (&context, buffer, len);
            
	    MD5Final (digest, &context);

	    fclose (file);
        /*
	    printf ("MD%d (%s) = ", MD, filename);
	    MDPrint (digest);
	    printf ("\n");
        */
    }
}

/* Digests the standard input and prints the result.
 */
static void MDFilter () {
    MD5_CTX context;
    int len;
    unsigned char buffer[16], digest[16];

    MD5Init (&context);
    while ((len = fread (buffer, 1, 16, stdin)) != 0)
        MD5Update (&context, buffer, len);
    MD5Final (digest, &context);

    MDPrint (digest);
    printf ("\n");
}

/* Prints a message digest in hexadecimal.
 */
static void MDPrint (unsigned char digest[16]) {
    unsigned int i;
    for (i = 0; i < 16; i++)
        fprintf (stderr, "%02x ", digest[i]);
}
