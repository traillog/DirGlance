//
// dgl.c
//
// Command line tool for DirGlance
//
// Usage: dgl [options] [target dir]
//
// Options:
//  -s  :   Sort by size [bytes] (default)
//  -f  :   Sort by files count (descending)
//  -d  :   Sort by dirs count (descending)
//  -m  :   Sort by date modified (latest to earliest)
//  -n  :   Soft by name (a to Z)
//  -t  :   Sort by type (<DIR>, <LIN>, file)
//  -h  :   Print usage
//  -b  :   Extended output (debug purposes)
//
//  If no option is specidied, then '-s' will be used
//  If no target dir is specified, then the current working dir will be used

#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include "list.h"               // Definition list ADT

#define     MAX_OPTIONS     20  // Max # command line options

// Flags indices
#define     FL_SIZE         0   // Sort by size [bytes] (default)
#define     FL_FILES        1   // Sort by files count (descending)
#define     FL_DIRS         2   // Sort by dirs count (descending)
#define     FL_MODIF        3   // Sort by date modified (latest to earliest)
#define     FL_NAME         4   // Soft by name (a to Z)
#define     FL_TYPE         5   // Sort by type (<DIR>, <LIN>, file)
#define     FL_HELP         6   // Print usage
#define     FL_DBG          7   // Extended output (debug purposes)

// Function prototypes
extern DWORD Options( int argc, LPCWSTR argv[], LPCWSTR OptStr, ... );
extern VOID ReportError( LPCTSTR userMsg, DWORD exitCode, BOOL prtErrorMsg );
BOOL scanDir( LPTSTR tDir, List* resList, Item* parentItem, BOOL fstLevel, BOOL dbg );
BOOL ptDir( LPCTSTR inDir );
void showResults( List* resultsList, Item* resultsLevel );
void showItem( Item* pItem );
int cmpItemsLastWriteTime( Item* pItemN, Item* pItemM );
int cmpItemsDirsCount( Item* pItemN, Item* pItemM );
int cmpItemsFilesCount( Item* pItemN, Item* pItemM );
int cmpItemsSizeCount( Item* pItemN, Item* pItemM );
int cmpItemsName( Item* pItemN, Item* pItemM );
void calcDispElapTime( const long long* ticksPt, const long long* freqPt );
void sepThousands( const long long* numPt, TCHAR* acc, size_t elemsAcc );

int wmain( int argc, LPTSTR argv[] )
{
    // Declare vars
    TCHAR targetDir[ MAX_PATH ] = { 0 };
    TCHAR workDir[ MAX_PATH ] = { 0 };
    DWORD targetLength = 0;
    DWORD workLength = 0;
    Item resultsItem = { 0 };
    List resultsList = { 0 };
    LARGE_INTEGER freq;
    LARGE_INTEGER startingT, endingT, elapsedTicks;
    BOOL flags[ MAX_OPTIONS ] = { 0 };
    int targetDirInd = 0;

    // Fetch frec & initial ticks count
    QueryPerformanceFrequency( &freq );
    QueryPerformanceCounter( &startingT );

    // Get index of first argument after options
    // Also determine which options are active
    targetDirInd = Options( argc, argv,
        TEXT( "sfdmnthb" ),
        &flags[ FL_SIZE ], &flags[ FL_FILES ], &flags[ FL_DIRS ],
        &flags[ FL_MODIF ], &flags[ FL_NAME ], &flags[ FL_TYPE ],
        &flags[ FL_HELP ], &flags[ FL_DBG ], NULL );

    // Get current working dir
    workLength = GetCurrentDirectory( _countof( workDir ), workDir );

    // Validate target dir
    if ( ( argc > targetDirInd + 1 ) || flags[ FL_HELP ] )
    {
        // More than one target or
        // target with gaps (no quotes) specified or
        // asked for help

        // Print usage
        wprintf_s( TEXT( "\n    Usage:    dgl [options] [target dir]\n\n" ) );
        wprintf_s( TEXT( "    Options:\n\n" ) );
        wprintf_s( TEXT( "      -s   :  Sort by size [bytes] (default)\n" ) );
        wprintf_s( TEXT( "      -f   :  Sort by files count (descending)\n" ) );
        wprintf_s( TEXT( "      -d   :  Sort by dirs count (descending)\n" ) );
        wprintf_s( TEXT( "      -m   :  Sort by date modified (latest to earliest)\n" ) );
        wprintf_s( TEXT( "      -n   :  Soft by name (a to Z)\n" ) );
        wprintf_s( TEXT( "      -t   :  Sort by type (<DIR>, <LIN>, file)\n" ) );
        wprintf_s( TEXT( "      -h   :  Print usage\n" ) );
        wprintf_s( TEXT( "      -b   :  Extended output (debug purposes)\n\n" ) );
        wprintf_s( TEXT( "    If no option is specidied, then '-s' will be used\n" ) );
        wprintf_s( TEXT( "    If no target dir is specified, then the current working dir will be used\n" ) );

        return 1;
    }
    else if ( ( argc < targetDirInd + 1 ) && ( workLength <= MAX_PATH - 3 ) )
    {
        // No target specified --> assume current dir
        wcscpy_s( targetDir, MAX_PATH, workDir );
    }
    else if ( argc == targetDirInd + 1 )
    {
        // One target specified

        // Validate target dir starting with '\'
        if ( argv[ targetDirInd ][ 0 ] == '\\' )
        {
            // Fetch drive letter from working dir
            wcsncpy_s( targetDir, MAX_PATH, workDir, 2 );
        }

        // Append passed dir to target dir
        wcscat_s( targetDir, MAX_PATH, argv[ targetDirInd ] );
    }

    // Set up absolute target dir --> resolve '.' and '..' in target dir
    if ( !SetCurrentDirectory( targetDir ) )
    {
        ReportError( TEXT( "\nTarget directory not found.\n" ), 0, TRUE );
        return 1;
    }

    // Display absolute target dir
    GetCurrentDirectory( _countof( targetDir ), targetDir );
    wprintf_s( TEXT( "\n    Target dir: \"%s\"\n\n" ), targetDir );

    // Initialize results list
    InitializeList( &resultsList );
    if ( ListIsFull( &resultsList ) )
    {
        wprintf_s( TEXT( "\nNo memory available!\n" ) );
        return 1;
    }

    // Debug output
    if ( flags[ FL_DBG ] )
        wprintf_s( TEXT( "    %s\n" ), targetDir );

    // Scan target dir
    scanDir( targetDir, &resultsList, &resultsItem, TRUE, flags[ FL_DBG ] );

    // Display results
    if ( ListIsEmpty( &resultsList ) )
        wprintf_s( TEXT( "\nNo data.\n\n" ) );
    else
    {
        // Sort results
        // if-else chain determines sorting priority
        // one sorting type high prio excludes low prio types
        if ( flags[ FL_SIZE ] )
            // Sort by size (descending)
            SortList( &resultsList, cmpItemsSizeCount );
        else if ( flags[ FL_FILES ] )
            // Sort by files count (descending)
            SortList( &resultsList, cmpItemsFilesCount );
        else if ( flags[ FL_DIRS ] )
            // Sort by dirs count (descending)
            SortList( &resultsList, cmpItemsDirsCount );
        else if ( flags[ FL_MODIF ] )
            // Sort by modification date (latest to earliest)
            SortList( &resultsList, cmpItemsLastWriteTime );
        else if ( flags[ FL_NAME ] )
            // Sort by name (a to Z)
            SortList( &resultsList, cmpItemsName );
        else
            // Default: sort by size (descending)
            SortList( &resultsList, cmpItemsSizeCount );

        // Debug output
        if ( flags[ FL_DBG ] )
            wprintf_s( TEXT( "\n" ) );

        // Display sorted results
        showResults( &resultsList, &resultsItem );
    }

    // Housekeeping
    EmptyTheList( &resultsList );

    // Fetch final ticks count
    QueryPerformanceCounter( &endingT );

    // Calc elapsed ticks
    elapsedTicks.QuadPart = endingT.QuadPart - startingT.QuadPart;

    // Calc and display elapsed time
    calcDispElapTime( &elapsedTicks.QuadPart, &freq.QuadPart );

    return 0;
}

BOOL scanDir( LPTSTR tDir, List* resList, Item* parentItem, BOOL fstLevel, BOOL dbg )
{
    TCHAR dirStr[ MAX_PATH ] = { 0 };
    HANDLE hFind = INVALID_HANDLE_VALUE;

    Item currentItem = { 0 };
    
    LARGE_INTEGER parentSize = { 0 };
    LARGE_INTEGER currentSize = { 0 };

    // Prepare string for use with FindFile functions.  First, copy the
    // string to a buffer, then append '\*' to the directory name.
    wcscpy_s( dirStr, MAX_PATH, tDir );

    if ( dirStr[ wcslen( dirStr ) - 1 ] == TEXT( '\\' ) )
        wcscat_s( dirStr, MAX_PATH, TEXT( "*" ) );
    else
        wcscat_s( dirStr, MAX_PATH, TEXT( "\\*" ) );

    // Find the first file in the directory.
    hFind = FindFirstFile( dirStr, &currentItem.findInfo );

    // Validate search handle
    if ( INVALID_HANDLE_VALUE == hFind )
    {
        // Only report error if different from 'Access Denied'.
        // For example, system symbolic links report 'access denied'.
        // If a handle is obtained and the size is requested,
        // Win32 reports 0 bytes.
        // See results using '..\progsDev\others\TestGetFileSizeEx\'
        if ( GetLastError() != ERROR_ACCESS_DENIED )
            ReportError( TEXT( "FindFirstFile failed" ), 0, TRUE );

        // Exit in any case
        return FALSE;
    }

    // List all the files/subdirs in the directory
    do
    {
        // Do not follow symbolic links
        // Symbolic links are listed but,
        // they do not affect dirs nor files count
        // Their size (0 bytes) and date are taken into account
        if ( !( currentItem.findInfo.dwFileAttributes &
                FILE_ATTRIBUTE_REPARSE_POINT ) )
        {
            // Subdirectory found ?
            if ( currentItem.findInfo.dwFileAttributes &
                 FILE_ATTRIBUTE_DIRECTORY )
            {
                // Dir found

                // Skip '.' or '..'
                if ( ptDir( currentItem.findInfo.cFileName ) )
                    continue;

                // Increment dirs count of parent
                ++parentItem->dirsCount.QuadPart;

                // Prepare the recursive call
                wcscpy_s( dirStr, MAX_PATH, tDir );
                wcscat_s( dirStr, MAX_PATH, TEXT( "\\" ) );
                wcscat_s( dirStr, MAX_PATH, currentItem.findInfo.cFileName );

                // Debug output
                if ( dbg )
                    wprintf_s( TEXT( "    %s\n" ), dirStr );

                // Scan subdir
                scanDir( dirStr, resList, &currentItem, FALSE, dbg );

                // Update dirs count of parent using found subdirs
                parentItem->dirsCount.QuadPart +=
                    currentItem.dirsCount.QuadPart;

                // Update files count of parent using found files
                parentItem->filesCount.QuadPart +=
                    currentItem.filesCount.QuadPart;
            }
            else
            {
                // File found

                // Increment files count of parent
                ++parentItem->filesCount.QuadPart;

                // Current item has one file
                currentItem.filesCount.QuadPart = 1;
            }
        }

        // Update last write time information of the parent.
        // Is current item's LastWriteTime later
        // than parent's LastWriteTime ?
        if ( CompareFileTime( &currentItem.findInfo.ftLastWriteTime,
            &parentItem->findInfo.ftLastWriteTime ) == 1 )
        {
            // Parent gets the LastWriteTime from current item
            parentItem->findInfo.ftLastWriteTime =
                currentItem.findInfo.ftLastWriteTime;
        }

        // Get size of current found file(s)
        currentSize.LowPart = currentItem.findInfo.nFileSizeLow;
        currentSize.HighPart = currentItem.findInfo.nFileSizeHigh;

        // Get size of parent so far
        parentSize.LowPart = parentItem->findInfo.nFileSizeLow;
        parentSize.HighPart = parentItem->findInfo.nFileSizeHigh;

        // Add current size to parent size (64-bit addition)
        parentSize.QuadPart += currentSize.QuadPart;

        // Update parent file size
        parentItem->findInfo.nFileSizeLow = parentSize.LowPart;
        parentItem->findInfo.nFileSizeHigh = parentSize.HighPart;

        // Update list
        if ( fstLevel )
        {
            // Append current item to results list
            if ( AddItem( currentItem, resList ) == false )
            {
                wprintf_s( TEXT( "Problem allocating memory\n" ) );
                return FALSE;
            }
        }

        // Reset current item
        memset( &currentItem, 0, sizeof( Item ) );

    } while ( FindNextFile( hFind, &currentItem.findInfo ) != 0 );

    // Validate end of search
    if ( GetLastError() != ERROR_NO_MORE_FILES )
    {
        ReportError( TEXT( "\nFindNextFile failed\n" ), 0, TRUE );
        return FALSE;
    }

    // Close search handle
    FindClose( hFind );

    return TRUE;
}

BOOL ptDir( LPCTSTR inDir )
{
    if ( ( ( wcslen( inDir ) == 1 ) &&
           ( wcscmp( inDir, TEXT( "." ) ) == 0 ) ) ||
         ( ( wcslen( inDir ) == 2 ) &&
           ( wcscmp( inDir, TEXT( ".." ) ) == 0 ) ) )
    {
        // inDir == "." || inDir == ".."
        return TRUE;
    }
    else
    {
        // otherwise
        return FALSE;
    }
}

void showResults( List* resultsList, Item* resultsLevel )
{
    // Display header
    wprintf_s( TEXT( "    %19s %5s %10s %10s %18s %s\n" ),
        TEXT( "Date Modified" ),
        TEXT( "Type" ),
        TEXT( "Dirs" ),
        TEXT( "Files" ),
        TEXT( "Size" ),
        TEXT( "Name" ) );

    wprintf_s( TEXT( "    %19s %5s %10s %10s %18s %s\n" ),
        TEXT( "-------------------" ),
        TEXT( "-----" ),
        TEXT( "----------" ),
        TEXT( "----------" ),
        TEXT( "------------------" ),
        TEXT( "---------------------------------------------" ) );

    // Display founded entries
    Traverse( resultsList, showItem );

    // Display totals
    wprintf_s( TEXT( "    %19s %5s %10s %10s %18s %s\n" ),
        TEXT( "-------------------" ),
        TEXT( "-----" ),
        TEXT( "----------" ),
        TEXT( "----------" ),
        TEXT( "------------------" ),
        TEXT( "---------------------------------------------" ) );

    showItem( resultsLevel );
}

void showItem( Item* pItem )
{
    FILETIME lastWriteFTIME;
    SYSTEMTIME lastWriteSYSTIME;
    LARGE_INTEGER entrySize;
    TCHAR dirsCtStr[ 32 ] = { 0 };
    TCHAR filesCtStr[ 32 ] = { 0 };
    TCHAR sizeStr[ 32 ] = { 0 };

    // Fetch and prepare entry last modification date & time

    // UTC time (FILETIME) to local time (FILETIME)
    FileTimeToLocalFileTime( &pItem->findInfo.ftLastWriteTime,
        &lastWriteFTIME );

    // local time (FILETIME) to local time (SYSTIME)
    FileTimeToSystemTime( &lastWriteFTIME, &lastWriteSYSTIME );

    // Disp entry last modification date & time
    wprintf_s( TEXT( "    %04d-%02d-%02d %02d:%02d:%02d" ),
        lastWriteSYSTIME.wYear,
        lastWriteSYSTIME.wMonth,
        lastWriteSYSTIME.wDay,
        lastWriteSYSTIME.wHour,
        lastWriteSYSTIME.wMinute,
        lastWriteSYSTIME.wSecond );

    // Disp entry type (dir/file)
    if ( pItem->findInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
        wprintf_s( TEXT( " %5s" ), TEXT( "<LIN>" ) );
    else if ( pItem->findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        wprintf_s( TEXT( " %5s" ), TEXT( "<DIR>" ) );
    else
        wprintf_s( TEXT( " %5s" ), TEXT( "     " ) );

    // Fetch entry size
    entrySize.LowPart = pItem->findInfo.nFileSizeLow;
    entrySize.HighPart = pItem->findInfo.nFileSizeHigh;

    // Convert nums to strings (thousands separated)
    sepThousands( &pItem->dirsCount.QuadPart, dirsCtStr,
        _countof( dirsCtStr ) );
    sepThousands( &pItem->filesCount.QuadPart, filesCtStr,
        _countof( filesCtStr ) );
    sepThousands( &entrySize.QuadPart, sizeStr,
        _countof( sizeStr ) );

    // Disp entry details
    wprintf_s( TEXT( " %10s %10s %18s %s\n" ),
        dirsCtStr,
        filesCtStr,
        sizeStr,
        pItem->findInfo.cFileName );
}

// compares LastWriteTime of two items
//   1 : item N later than item M
//   0 : item N same time item M
//  -1 : item N earlier item M
int cmpItemsLastWriteTime( Item* pItemN, Item* pItemM )
{
    return CompareFileTime( &pItemN->findInfo.ftLastWriteTime,
        &pItemM->findInfo.ftLastWriteTime );
}

// compares dirs count of two items
//   1 : item N > item M
//   0 : item N = item M
//  -1 : item N < item M
int cmpItemsDirsCount( Item* pItemN, Item* pItemM )
{
    // Def vars
    LARGE_INTEGER valN = { 0 };
    LARGE_INTEGER valM = { 0 };

    // Fetch N value
    valN.HighPart = pItemN->dirsCount.HighPart;
    valN.LowPart = pItemN->dirsCount.LowPart;

    // Fetch M value
    valM.HighPart = pItemM->dirsCount.HighPart;
    valM.LowPart = pItemM->dirsCount.LowPart;

    // Perform 64-bit comparison
    if ( valN.QuadPart > valM.QuadPart )
        return 1;
    else if ( valN.QuadPart == valM.QuadPart )
        return 0;
    else
        return -1;
}

// compares files count of two items
//   1 : item N > item M
//   0 : item N = item M
//  -1 : item N < item M
int cmpItemsFilesCount( Item* pItemN, Item* pItemM )
{
    // Def vars
    LARGE_INTEGER valN = { 0 };
    LARGE_INTEGER valM = { 0 };

    // Fetch N value
    valN.HighPart = pItemN->filesCount.HighPart;
    valN.LowPart = pItemN->filesCount.LowPart;

    // Fetch M value
    valM.HighPart = pItemM->filesCount.HighPart;
    valM.LowPart = pItemM->filesCount.LowPart;

    // Perform 64-bit comparison
    if ( valN.QuadPart > valM.QuadPart )
        return 1;
    else if ( valN.QuadPart == valM.QuadPart )
        return 0;
    else
        return -1;
}

// compares size count of two items
//   1 : item N > item M
//   0 : item N = item M
//  -1 : item N < item M
int cmpItemsSizeCount( Item* pItemN, Item* pItemM )
{
    // Def vars
    LARGE_INTEGER valN = { 0 };
    LARGE_INTEGER valM = { 0 };

    // Fetch N value
    valN.HighPart = pItemN->findInfo.nFileSizeHigh;
    valN.LowPart = pItemN->findInfo.nFileSizeLow;

    // Fetch M value
    valM.HighPart = pItemM->findInfo.nFileSizeHigh;
    valM.LowPart = pItemM->findInfo.nFileSizeLow;

    // Perform 64-bit comparison
    if ( valN.QuadPart > valM.QuadPart )
        return 1;
    else if ( valN.QuadPart == valM.QuadPart )
        return 0;
    else
        return -1;
}

// compare names of two items
//  >0 : item N before item M
//   0 : item N same place as item M
//  <0 : item N after item M
int cmpItemsName( Item* pItemN, Item* pItemM )
{
    int result;

    // case-insensitive comparison
    result = _wcsicmp( pItemM->findInfo.cFileName, pItemN->findInfo.cFileName );

    return result;
}

void calcDispElapTime( const long long* ticksPt, const long long* freqPt )
{
    unsigned long long totalMicros = 0, restMicros = 0;
    unsigned long long millis = 0, secs = 0, mins = 0;
    
    totalMicros = *ticksPt * 1000000 / *freqPt;

    millis = totalMicros / 1000;        // total millis

    restMicros = totalMicros % 1000;

    if ( restMicros >= 500 )            // round to millis
        ++millis;

    secs = millis / 1000;       // total secs

    millis = millis % 1000;     // rest millis

    mins = secs / 60;           // total mins

    secs = secs % 60;           // rest secs

    // Display results
    wprintf( TEXT( "\n    Elapsed time [mm:ss.mil] : " ) );
    wprintf( TEXT( "%02I64d:%02I64d.%03I64d\n" ),
        mins, secs, millis );
}

void sepThousands( const long long* numPt, TCHAR* acc, size_t elemsAcc )
{
    static TCHAR app[ 32 ] = { 0 }; // append data (last three digits as chars)
    long long numIn = *numPt;       // remaining num
    long long res = 0;              // append data (last three digits as nums)    

    // Reset accumulated result (output str)
    wmemset( acc, 0, elemsAcc );

    // Iterate as long as there are digits left
    if ( numIn == 0 )
        wcscpy_s( acc, elemsAcc, TEXT( "0" ) );
    else
    {
        while ( numIn > 0 )
        {
            // Reset append storage
            wmemset( app, 0, _countof( app ) );

            // Get three right most digits
            res = numIn % 1000;

            // Get the remaining num
            numIn = numIn / 1000;

            // Build up string to be appended in front of accumulated result
            swprintf_s( app, _countof( app ),
                ( numIn > 0 ) ? TEXT( ",%03d" ) : TEXT( "%3d" ), res );

            // Append accumulated result to new digits
            wcscat_s( app, _countof( app ), acc );

            // Update accumulated result
            wcscpy_s( acc, elemsAcc, app );
        }
    }
}