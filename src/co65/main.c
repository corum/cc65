/*****************************************************************************/
/*                                                                           */
/*				    main.c				     */
/*                                                                           */
/*              Main program for the co65 object file converter              */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 2003      Ullrich von Bassewitz                                       */
/*               Römerstrasse 52                                             */
/*               D-70794 Filderstadt                                         */
/* EMail:        uz@cc65.org                                                 */
/*                                                                           */
/*                                                                           */
/* This software is provided 'as-is', without any expressed or implied       */
/* warranty.  In no event will the authors be held liable for any damages    */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/* 1. The origin of this software must not be misrepresented; you must not   */
/*    claim that you wrote the original software. If you use this software   */
/*    in a product, an acknowledgment in the product documentation would be  */
/*    appreciated but is not required.                                       */
/* 2. Altered source versions must be plainly marked as such, and must not   */
/*    be misrepresented as being the original software.                      */
/* 3. This notice may not be removed or altered from any source              */
/*    distribution.                                                          */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

/* common */
#include "cmdline.h"
#include "fname.h"
#include "print.h"
#include "segnames.h"
#include "version.h"
#include "xmalloc.h"
#include "xsprintf.h"

/* co65 */
#include "error.h"
#include "global.h"
#include "o65.h"



/*****************************************************************************/
/*     	       	     	       	     Code				     */
/*****************************************************************************/



static void Usage (void)
/* Print usage information and exit */
{
    fprintf (stderr,
    	     "Usage: %s [options] file\n"
    	     "Short options:\n"
       	     "  -V\t\t\tPrint the version number\n"
       	     "  -g\t\t\tAdd debug info to object file\n"
       	     "  -h\t\t\tHelp (this text)\n"
       	     "  -o name\t\tName the output file\n"
       	     "  -v\t\t\tIncrease verbosity\n"
	     "\n"
	     "Long options:\n"
	     "  --bss-name seg\tSet the name of the BSS segment\n"
       	     "  --code-name seg\tSet the name of the CODE segment\n"
       	     "  --data-name seg\tSet the name of the DATA segment\n"
       	     "  --debug-info\t\tAdd debug info to object file\n"
	     "  --help\t\tHelp (this text)\n"
       	     "  --verbose\t\tIncrease verbosity\n"
       	     "  --version\t\tPrint the version number\n"
       	     "  --zeropage-name seg\tSet the name of the ZEROPAGE segment\n",
    	     ProgName);
}



static void CheckSegName (const char* Seg)
/* Abort if the given name is not a valid segment name */
{
    /* Print an error and abort if the name is not ok */
    if (!ValidSegName (Seg)) {
	Error ("Segment name `%s' is invalid", Seg);
    }
}



static void OptBssName (const char* Opt attribute ((unused)), const char* Arg)
/* Handle the --bss-name option */
{
    /* Check for a valid name */
    CheckSegName (Arg);

    /* Set the name */
    BssSeg = xstrdup (Arg);
}



static void OptCodeName (const char* Opt attribute ((unused)), const char* Arg)
/* Handle the --code-name option */
{
    /* Check for a valid name */
    CheckSegName (Arg);

    /* Set the name */
    CodeSeg = xstrdup (Arg);
}



static void OptDataName (const char* Opt attribute ((unused)), const char* Arg)
/* Handle the --data-name option */
{
    /* Check for a valid name */
    CheckSegName (Arg);

    /* Set the name */
    DataSeg = xstrdup (Arg);
}



static void OptDebugInfo (const char* Opt attribute ((unused)),
		    	  const char* Arg attribute ((unused)))
/* Add debug info to the object file */
{
    DebugInfo = 1;
}



static void OptHelp (const char* Opt attribute ((unused)),
	       	     const char* Arg attribute ((unused)))
/* Print usage information and exit */
{
    Usage ();
    exit (EXIT_SUCCESS);
}



static void OptVerbose (const char* Opt attribute ((unused)),
		  	const char* Arg attribute ((unused)))
/* Increase verbosity */
{
    ++Verbosity;
}



static void OptVersion (const char* Opt attribute ((unused)),
		  	const char* Arg attribute ((unused)))
/* Print the assembler version */
{
    fprintf (stderr,
       	     "co65 V%u.%u.%u - (C) Copyright 1998-2003 Ullrich von Bassewitz\n",
       	     VER_MAJOR, VER_MINOR, VER_PATCH);
}



static void OptZeropageName (const char* Opt attribute ((unused)), const char* Arg)
/* Handle the --zeropage-name option */
{
    /* Check for a valid name */
    CheckSegName (Arg);

    /* Set the name */
    ZeropageSeg = xstrdup (Arg);
}



static const char* SegReloc (const O65Data* D, const O65Reloc* R, unsigned long Val)
{
    static char Buf[256];
    const O65Import* Import;

    switch (R->SegID) {

        case O65_SEGID_UNDEF:
            if (R->SymIdx >= CollCount (&D->Imports)) {
                Error ("Import index out of range (input file corrupt)");
            }
            Import = CollConstAt (&D->Imports, R->SymIdx);
            xsprintf (Buf, sizeof (Buf), "%s%+ld", Import->Name, (long) Val);
            break;

        case O65_SEGID_TEXT:
            xsprintf (Buf, sizeof (Buf), "%s%+ld", CodeSeg, (long) (Val - D->Header.tbase));
            break;

        case O65_SEGID_DATA:
            xsprintf (Buf, sizeof (Buf), "%s%+ld", DataSeg, (long) (Val - D->Header.dbase));
            break;

        case O65_SEGID_BSS:
            xsprintf (Buf, sizeof (Buf), "%s%+ld", BssSeg, (long) (Val - D->Header.bbase));
            break;

        case O65_SEGID_ZP:
            xsprintf (Buf, sizeof (Buf), "%s%+ld", ZeropageSeg, (long) Val - D->Header.zbase);
            break;

        case O65_SEGID_ABS:
            Error ("Relocation entry contains O65_SEGID_ABS");
            break;

        default:
            Internal ("Cannot handle this segment reference in reloc entry");
    }

    return Buf;
}



static void ConvertSeg (FILE* F, const O65Data* D, const Collection* Relocs,
                        const unsigned char* Data, unsigned long Size)
/* Convert one segment */
{
    const O65Reloc* R;
    unsigned        RIdx;
    unsigned long   Byte;

    /* Get the pointer to the first relocation entry if there are any */
    R = (CollCount (Relocs) > 0)? CollConstAt (Relocs, 0) : 0;

    /* Initialize for the loop */
    RIdx = 0;
    Byte = 0;

    /* Walk over the segment data */
    while (Byte < Size) {

        if (R && R->Offs == Byte) {
            /* We've reached an entry that must be relocated */
            unsigned long Val;
            switch (R->Type) {

                case O65_RTYPE_WORD:
                    if (Byte >= Size - 1) {
                        Error ("Found WORD relocation, but not enough bytes left");
                    } else {
                        Val = (Data[Byte+1] << 8) + Data[Byte];
                        Byte += 2;
                        fprintf (F, "\t.word\t%s\n", SegReloc (D, R, Val));
                    }
                    break;

                case O65_RTYPE_HIGH:
                    Val = (Data[Byte++] << 8) + R->Val;
                    fprintf (F, "\t.byte\t>(%s)\n", SegReloc (D, R, Val));
                    break;

                case O65_RTYPE_LOW:
                    Val = Data[Byte++];
                    fprintf (F, "\t.byte\t<(%s)\n", SegReloc (D, R, Val));
                    break;

                case O65_RTYPE_SEGADDR:
                    if (Byte >= Size - 2) {
                        Error ("Found SEGADDR relocation, but not enough bytes left");
                    } else {
                        Val = (((unsigned long) Data[Byte+2]) << 16) +
                              (((unsigned long) Data[Byte+1]) <<  8) +
                              (((unsigned long) Data[Byte+0]) <<  0) +
                              R->Val;
                        Byte += 3;
                        fprintf (F, "\t.faraddr\t%s\n", SegReloc (D, R, Val));
                    }
                    break;
                case O65_RTYPE_SEG:
                default:
                    Internal ("Invalid relocation type at %lu", Byte);
            }

            /* Get the next relocation entry */
            if (++RIdx < CollCount (Relocs)) {
                R = CollConstAt (Relocs, RIdx);
            } else {
                R = 0;
            }

        } else {
            /* Just a constant value */
            fprintf (F, "\t.byte\t$%02X\n", Data[Byte++]);
        }
    }

    fprintf (F, "\n");
}



static void Convert (void)
/* Do file conversion */
{
    FILE* F;
    unsigned  I;

    /* Read the o65 file into memory */
    O65Data* D = ReadO65File (InFilename);

    /* For now, we do only accept o65 files generated by the ld65 linker which
     * have a specific format.
     */
    if (D->Header.mode != O65_MODE_CC65) {
        Error ("Cannot convert o65 files of this type");
    }
                     
    /* Output statistics */
    Print (stdout, 1, "Size of text segment:               %5lu\n", D->Header.tlen);
    Print (stdout, 1, "Size of data segment:               %5lu\n", D->Header.dlen);
    Print (stdout, 1, "Size of bss segment:                %5lu\n", D->Header.blen);
    Print (stdout, 1, "Size of zeropage segment:           %5lu\n", D->Header.zlen);
    Print (stdout, 1, "Number of imports:                  %5u\n", CollCount (&D->Imports));
    Print (stdout, 1, "Number of exports:                  %5u\n", CollCount (&D->Exports));
    Print (stdout, 1, "Number of text segment relocations: %5u\n", CollCount (&D->TextReloc));
    Print (stdout, 1, "Number of data segment relocations: %5u\n", CollCount (&D->DataReloc));

    /* Open the output file */
    F = fopen (OutFilename, "wb");
    if (F == 0) {
        Error ("Cannot open `%s': %s", OutFilename, strerror (errno));
    }

    /* Create a header */
    if ((D->Header.mode & O65_CPU_MASK) == O65_CPU_65816) {
	fprintf (F, "\t.p816\n");
    }
    fprintf (F, ";\n; File generated by co65 v %u.%u.%u\n;\n",
             VER_MAJOR, VER_MINOR, VER_PATCH);
    fprintf (F, "\t.fopt\t\tcompiler,\"co65 v %u.%u.%u\"\n",
             VER_MAJOR, VER_MINOR, VER_PATCH);
    fprintf (F, "\t.case\t\ton\n");
    fprintf (F, "\t.debuginfo\t%s\n", (DebugInfo != 0)? "on" : "off");
    fprintf (F, "\n");

    /* Imported identifiers */
    if (CollCount (&D->Imports) > 0) {
        for (I = 0; I < CollCount (&D->Imports); ++I) {

            /* Get the next import */
            O65Import* Import = CollAtUnchecked (&D->Imports, I);

            /* Import it by name */
            fprintf (F, "\t.import\t%s\n", Import->Name);
        }
        fprintf (F, "\n");
    }

    /* Exported identifiers */
    if (CollCount (&D->Exports) > 0) {
        for (I = 0; I < CollCount (&D->Exports); ++I) {

            /* Get the next import */
            O65Export* Export = CollAtUnchecked (&D->Exports, I);

            /* Import it by name */
            fprintf (F, "\t.export\t%s\n", Export->Name);
        }
        fprintf (F, "\n");
    }

    /* Code segment */
    fprintf (F, ".segment\t\"%s\"\n", CodeSeg);
    fprintf (F, "%s:\n", CodeSeg);
    ConvertSeg (F, D, &D->TextReloc, D->Text, D->Header.tlen);

    /* Data segment */
    fprintf (F, ".segment\t\"%s\"\n", DataSeg);
    fprintf (F, "%s:\n", DataSeg);
    ConvertSeg (F, D, &D->DataReloc, D->Data, D->Header.dlen);

    /* BSS segment */
    fprintf (F, ".segment\t\"%s\"\n", BssSeg);
    fprintf (F, "%s:\n", BssSeg);
    fprintf (F, "\t.res\t%lu\n", D->Header.blen);
    fprintf (F, "\n");

    fprintf (F, "\t.end\n");
    fclose (F);
}



int main (int argc, char* argv [])
/* Converter main program */
{
    /* Program long options */
    static const LongOpt OptTab[] = {
	{ "--bss-name",		1, 	OptBssName   		},
	{ "--code-name",	1, 	OptCodeName  		},
	{ "--data-name",	1, 	OptDataName  		},
	{ "--debug-info",      	0, 	OptDebugInfo 		},
	{ "--help",    		0,	OptHelp			},
	{ "--verbose", 	       	0,	OptVerbose		},
	{ "--version", 	       	0,	OptVersion		},
       	{ "--zeropage-name",   	1,     	OptZeropageName         },
    };

    unsigned I;

    /* Initialize the cmdline module */
    InitCmdLine (&argc, &argv, "co65");

    /* Check the parameters */
    I = 1;
    while (I < ArgCount) {

       	/* Get the argument */
       	const char* Arg = ArgVec [I];

       	/* Check for an option */
       	if (Arg [0] == '-') {
       	    switch (Arg [1]) {

		case '-':
		    LongOption (&I, OptTab, sizeof(OptTab)/sizeof(OptTab[0]));
		    break;

	    	case 'g':
	    	    OptDebugInfo (Arg, 0);
	    	    break;

		case 'h':
		    OptHelp (Arg, 0);
		    break;

       	        case 'o':
       	  	    OutFilename = GetArg (&I, 2);
       	       	    break;

       	       	case 'v':
	       	    OptVerbose (Arg, 0);
       	       	    break;

       	        case 'V':
    	       	    OptVersion (Arg, 0);
       	       	    break;

       	       	default:
       	       	    UnknownOption (Arg);
	       	    break;

     	    }
       	} else {
    	    /* Filename. Check if we already had one */
    	    if (InFilename) {
    	       	Error ("Don't know what to do with `%s'\n", Arg);
    	    } else {
	       	InFilename = Arg;
	    }
     	}

	/* Next argument */
	++I;
    }

    /* Do we have an input file? */
    if (InFilename == 0) {
       	Error ("No input file\n");
    }

    /* Generate the name of the output file if none was specified */
    if (OutFilename == 0) {
        OutFilename = MakeFilename (InFilename, AsmExt);
    }

    /* Do the conversion */
    Convert ();

    /* Return an apropriate exit code */
    return EXIT_SUCCESS;
}



