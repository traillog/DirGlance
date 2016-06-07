//
//  options.c
//
//  Based on 'Windows System Programming - Hart - 4Ed'
//  ( '..\WSP4_Examples\UTILITY\OPTIONS.C' )
//
//  Utility function to extract option flags from the command line.

#include <windows.h>
#include <wchar.h>

/*
    argv is the command line.

    The options, if any, start with a '-' in argv[1], argv[2], ...

    OptStr is a text string containing all possible options,
    in one-to-one correspondence with the addresses of Boolean variables
    in the variable argument list (...).

    These flags are set if and only if the corresponding option
    character occurs in argv[1], argv[2], ...

    The return value is the argv index of the first argument
    beyond the options. */
DWORD Options( int argc, LPCWSTR argv[], LPCWSTR OptStr, ... )
{
    int iFlag = 0;                  // Index over string of options
    int iArg = 0;                   // Index of first argv after options
    LPBOOL pFlag = NULL;            // Placeholder for elem taken from arg list
    va_list pFlagList;              // Declare object to hold argument list

    // Initialize pFlagList to argument list
    va_start( pFlagList, OptStr );

    // Iterate over received argument list
    // Use iFlag to iterate over string of possible options
    while (
        // Fetch element from argument list (pointer to BOOL),
        // continue as long as it is not NULL
        ( ( pFlag = va_arg( pFlagList, LPBOOL ) ) != NULL ) &&
        
        // Continue as long as iFlag is less than
        // possible amount of options received
        ( iFlag < (int)wcslen( OptStr ) ) )
    {
            *pFlag = FALSE;             // Initialize current flag

            // Iterate over all argv elements that start with '-'
            for ( iArg = 1;
                  !( *pFlag ) &&
                  ( iArg < argc ) &&
                  ( argv[ iArg ][ 0 ] == TEXT( '-' ) );
                  iArg++ )
            {
                // Search option No. iflag in argv No. iArg
                // (find wchar in buffer)
                *pFlag = wmemchr( argv[ iArg ],
                                  OptStr[ iFlag ],
                                  wcslen( argv[ iArg ] ) ) != NULL;
            }
            
            // Inc index to next posible option to look for
            iFlag++;
    }

    // Clean up
    va_end( pFlagList );

    // Find index of first argv element after options
    for ( iArg = 1;
          ( iArg < argc ) &&
          ( argv[ iArg ][ 0 ] == TEXT( '-' ) );
          iArg++ )
    {
        ;
    }

    return iArg;
}
