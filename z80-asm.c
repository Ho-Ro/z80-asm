#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "hash.h"
#include "asm.h"
#include "file.h"

#include "kk_ihex_write.h"


static FILE *input;
static FILE *outbin;
static FILE *outhex;

static unsigned char memory[MAX_MEM];
static unsigned char shadow[MAX_MEM];
static unsigned start = MAX_MEM;

static struct ihex_state *ihex = NULL;
static int verboseMode = 0;


static void MSG( int mode, const char* format, ...);
void ihex_flush_buffer(struct ihex_state *ihex, char *buffer, char *eptr);
static void usage( const char *fullpath );
static int take_line(char *line, int max_chars);
static int compare( const void *left, const void *right );
struct info{ char *label; int value; unsigned lineno; };


int main( int argc, char **argv ) {
    char *filename = 0;
    char line[512];
    
    struct ihex_state ihex_;

    int i, j, result = 0, fill = 0, offset = -1, z80 = 0,
        reference = 0, no_outfile = 0;

    for ( i = 1, j = 0; i < argc; i++ ) {
        if ('-' == argv[i][0]) {
            switch (argv[i][++j]) {
                case 'h':
                    usage( argv[0] );
                    return 0;
                case 'w':
                    printf("Warnings switched on.\n");
                    WARNINGS = 1;
                    break;
                case 'v':
                    ++verboseMode;
                    break;
                case 'l':
                    LISTING = 1;
                    break;
                case 'r':
                    reference = 1;
                    break;
                //case 'c':
                //    offset = com = 0x0100; // CP/M *.com files start at address 0x0100
                //    break;
                case 'z':
                    z80 = 1;
                    break;
                case 'f': // fill 
                    if ( argv[i][++j] ) // "-fXX"
                        result = sscanf( argv[i]+j, "%x", &fill );
                    else if ( i < argc - 1 ) // "-f XX"
                            result = sscanf( argv[++i], "%x", &fill );
                    if ( result )
                        fill &= 0x00FF; // limit to byte size
                    else {
                        fprintf( stderr,"Error: option -f needs a hexadecimal argument\n" );
                        return 1;
                    }
                    j=0; // end of this arg group
                    break;
                case 'o': // offset 
                    if ( argv[i][++j] ) // "-oXXXX"
                        result = sscanf( argv[i]+j, "%x", &offset );
                    else if ( i < argc - 1 ) // "-o XXXX"
                        result = sscanf( argv[++i], "%x", &offset );
                    if ( result )
                        offset &= 0xFFFF; // limit to 64K
                    else {
                        fprintf( stderr,"Error: option -o needs a hexadecimal argument\n" );
                        return 1;
                    }
                    j=0; // end of this arg group
                    break;
                default:
                    usage( argv[0] );
                    return 1;
            }
            if ( j && argv[i][j+1] ) { // one more arg char ..
                --i; // .. in this arg group
                continue;
            }
            j = 0; // start in next arg group
        } else { // no leading '-'
            if ( !filename ) // not yet set
                filename = argv[i];
            else { // error, redef of filename or no filename provided
                usage( argv[0] );
                return 1;
            }
        }
    }
    if ( !filename ) {
        usage( argv[0] );
        return 1;
    }

    disable_pseudo = 0;
    memset( memory, fill, MAX_MEM );
    memset( shadow, 0, MAX_MEM );
    asm_init( (unsigned char)fill );

    ihex = &ihex_; ihex_init( ihex );


    MSG( 1, "Opening input file %s\n", filename );
    input = fopen (filename, "r" );
    if (!input) {
        fprintf( stderr, "Error: can't open input file \"%s\".\n", filename );
        return 1;
    }


    if ( !no_outfile && strlen( filename ) > 4 &&
        !strcmp( filename + strlen( filename ) - 4,".asm") ) {
        // create out file name(s) from in file name
        sprintf( filename + strlen( filename ) - 3, z80 ? "z80" : "bin" );
        MSG( 1, "Creating output file %s\n", filename );
        outbin = fopen( filename, "wb" );
        if ( !outbin ) {    
            asm_close();
            fprintf( stderr,"Error: Can't open output file \"%s\".\n", filename );
            return 1;
        }
        if ( ihex ) {
            sprintf( filename + strlen( filename ) - 3, "hex" );
            MSG( 1, "Creating output file %s\n", filename );
            outhex = fopen( filename, "wb" );
        }
        if ( !outhex ) {
            asm_close();
            fprintf( stderr, "Error: Can't open output file \"%s\".\n", filename );
            return 1;
        }

    } else
        MSG( 1, "No output files created\n" );


    int err = 0;
    MSG( 2, "Compiler pass 1\n" );
    set_compile_pass( 1 );
    set_start_address( 0 );
    while ( !err && !take_line( line, 511 ) )
        err = compile( line );
    if ( err == 8 )
        err = 0;
    if ( !err )
        err = check_cond_nesting();
    if ( !err ) {
        if ( fseek( input, 0, SEEK_SET ) ) {
            asm_close();
            fprintf( stderr, "Can't rewind input file\n" );
            return 2;
        }
        MSG( 2, "Compiler pass 1\n" );
        set_compile_pass( 2 );
        set_start_address( 0 );
        while (!err && !take_line( line, 511 ) )
            err= compile( line );
    }
    if ( err==8 ) err = 0;
    fclose( input );

    if ( !err ) {
        if ( offset >= 0 )
            start = offset & 0xFFFF;
        if ( generated_bytes() && highest_address() >= start ) {
            if ( LISTING )
                printf( "Using memory region $%04X .. $%04X\n", start, highest_address() );
            if ( z80 )
                write_header( outbin, start );
            fwrite( memory + start, 1, highest_address() + 1 - start, outbin );
        }
        if ( ihex ) {
            ihex_end_write( ihex );
            fclose(outhex);
            ihex = NULL;
        }
        fclose( outbin );
    }

    int a, b;
    
    if ( !err && ( LISTING || reference ) )
        printf( "%u bytes code generated and %u labels defined\n",
        generated_bytes(), table_entries());
    if ( !err && reference && ( b = table_entries() ) ) {
        struct info  *ele;
        ele = malloc( b * sizeof( struct info ) );
        for( a = 0; next_table_entry( &ele[a].label, &ele[a].value, &ele[a].lineno ); a++ )
            ;
        qsort( ele, b, sizeof( struct info ),compare );
        printf("       Cross reference:\n");
        printf("      symbol value hexa  line\n");
        for ( a=0; a<b; a++ )
            printf( "%12.12s %5d %4hx %5u\n", ele[a].label, ele[a].value,
                  (unsigned short)ele[a].value, ele[a].lineno );
        free( ele );
        err = 0;
    }
    asm_close();
    return err;
}



static void usage( const char *fullpath ) {
    const char *progname = 0;
    char c;
    while ( c = *fullpath++ )
        if ( c == '/' || c == '\\' )
            progname = fullpath;
    printf(
        "Usage:\n"
        "  %s [-h] [-w] [-l] [-r] [-n] [-z] [-o XXXX] [-f XX] <inputfile.asm>\n"
        "    -h       Show this help\n"
        "    -w       Enable warnings\n"
        "    -l       Create listing\n"
        "    -r       Create cross reference\n"
        "    -n       Do not create output files, just analyse the source code\n"
        //"    -c       Create a *.com binary file with file offset of $0100\n"
        "    -z       Create a *.z80 bimary file with offset address in header\n"
        "    -o XXXX  Set the binary file offset to $XXXX\n"
        "    -f XX    Fill unused memory with $XX\n",
        progname
    );
}


static void MSG( int mode, const char* format, ...) {
    if ( verboseMode >= mode ) {
        while ( mode-- )
            fprintf( stderr, " " );
        va_list argptr;
        va_start(argptr, format);
        vfprintf(stderr, format, argptr);
        va_end(argptr);
    }
}


void error( int l, char *line, char *txt ) {
    fprintf( stderr, "%s\nline %d: %s\n", line, l, txt );
}


// fill memory from assembler output, create ihex on the fly
unsigned char write_to_memory( unsigned short index, unsigned char byte ) {
    unsigned char state = shadow[ index ]; // was this cell already written?
    memory[ index ] = byte;
    shadow[ index ] = 1;
    if ( index < start )
        start = index;
    if ( ihex ) {
        static unsigned address = MAX_MEM;   
        if ( index != address ) // start new ihex line
            ihex_write_at_address( ihex, address = index ); 
        ihex_write_byte( ihex, byte );
        ++address;
    }
    return state;
}


void ihex_flush_buffer(struct ihex_state *ihex, char *buffer, char *eptr) {
    (void) ihex;
    *eptr = '\0';
    (void) fputs(buffer, outhex);
}


static int take_line( char *line, int max_chars ) {
    if ( !fgets( line, max_chars-1, input ) ) { line[0] = 0; return 1; }  /* end of file */
#ifdef UNIX
    if ( strlen( line ) && line[ strlen( line ) - 1 ] == '\n' )
#endif
        line[ strlen( line ) - 1 ] = 0;
#ifdef UNIX  /* reading DOS-files */
    if ( strlen( line ) && line[ strlen( line ) - 1 ] == '\r' )
        line[strlen(line)-1]=0;
#endif
#ifdef DOS
    if ( !strlen( line ) )
        take_line( line, max_chars );
#endif
    return 0;
}


static int compare( const void *left, const void *right ) {
    return  strcmp( ((const struct info *)left)->label, ((const struct info *)right)->label );
}


