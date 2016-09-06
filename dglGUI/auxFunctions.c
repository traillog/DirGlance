//
// auxFunctions.c
//
// Auxiliary Functions Subdirectory Analysis

#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include "list.h"               // Definition list ADT

#define     MAX_OPTIONS     20  // Max # command line options

// Sorting methodes
#define     BY_SIZE         0   // Sort by size [bytes] (default)
#define     BY_FILES        1   // Sort by files count (descending)
#define     BY_DIRS         2   // Sort by dirs count (descending)
#define     BY_MODIF        3   // Sort by date modified (latest to earliest)
#define     BY_NAME         4   // Soft by name (a to Z)
#define     BY_TYPE         5   // Sort by type (<DIR>, <LIN>, file)

// Function prototypes
extern DWORD Options( int argc, LPCWSTR argv[], LPCWSTR OptStr, ... );
extern VOID ReportError( LPCTSTR userMsg, DWORD exitCode, BOOL prtErrorMsg );
BOOL scanDir( LPTSTR tDir, List* resList, Item* parentItem, BOOL fstLevel, BOOL* pReset );
BOOL ptDir( LPCTSTR inDir );
void sortResults( List* plist, int sortMethod );
void showResults( List* resultsList, Item* resultsLevel, HWND hConstLBox );
void showItem( Item* pItem, HWND hConstLBox );
int cmpItemsLastWriteTime( Item* pItemN, Item* pItemM );
int cmpItemsDirsCount( Item* pItemN, Item* pItemM );
int cmpItemsFilesCount( Item* pItemN, Item* pItemM );
int cmpItemsSizeCount( Item* pItemN, Item* pItemM );
int cmpItemsName( Item* pItemN, Item* pItemM );
void calcDispElapTime( const long long* ticksPt, const long long* freqPt );
void sepThousands( const long long* numPt, TCHAR* acc, size_t elemsAcc );

BOOL scanDir( LPTSTR tDir, List* resList, Item* parentItem, BOOL fstLevel, BOOL* pReset )
{
    TCHAR dirStr[ MAX_PATH ] = { 0 };
    HANDLE hFind = INVALID_HANDLE_VALUE;

    Item currentItem = { 0 };
    
    LARGE_INTEGER parentSize = { 0 };
    LARGE_INTEGER currentSize = { 0 };

    // Abort current scan
    if ( *pReset == TRUE )
        return FALSE;

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
            ReportError( TEXT( "FindFirstFile failed." ), 0, TRUE );

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

                // Scan subdir
                scanDir( dirStr, resList, &currentItem, FALSE, pReset );

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
        ReportError( TEXT( "\nFindNextFile failed.\n" ), 0, TRUE );
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

void sortResults( List* plist, int sortMethod )
{
    if ( ListIsEmpty( plist ) )
        MessageBox( NULL, TEXT( "No results." ),
            TEXT( "Dir Glance" ), MB_ICONERROR );
    else
    {
        // Sort results
        switch( sortMethod )
        {
        case BY_MODIF :
            SortList( plist, cmpItemsLastWriteTime );
            break;

        case BY_FILES :
            SortList( plist, cmpItemsFilesCount );
            break;

        case BY_DIRS :
            SortList( plist, cmpItemsDirsCount );
            break;

        case BY_NAME :
            SortList( plist, cmpItemsName );
            break;

        case BY_SIZE :
        default :
            SortList( plist, cmpItemsSizeCount );
            break;
        }
    }
}

void showResults( List* resultsList, Item* resultsLevel, HWND hConstLBox )
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
    Traverse( resultsList, showItem, hConstLBox );

    // Display totals
    wprintf_s( TEXT( "    %19s %5s %10s %10s %18s %s\n" ),
        TEXT( "-------------------" ),
        TEXT( "-----" ),
        TEXT( "----------" ),
        TEXT( "----------" ),
        TEXT( "------------------" ),
        TEXT( "---------------------------------------------" ) );

    showItem( resultsLevel, hConstLBox );
}

void showItem( Item* pItem, HWND hContsLBox )
{
    FILETIME lastWriteFTIME;
    SYSTEMTIME lastWriteSYSTIME;
    LARGE_INTEGER entrySize;
    static TCHAR dirsCtStr[ 32 ] = { 0 };
    static TCHAR filesCtStr[ 32 ] = { 0 };
    static TCHAR sizeStr[ 32 ] = { 0 };
    static TCHAR dateStr[ 32 ] = { 0 };
    static TCHAR typeStr[ 32 ] = { 0 };
    static TCHAR detailsStr[ 512 ] = { 0 };
    static TCHAR outStr[ 512 ] = { 0 };

    // Fetch and prepare entry last modification date & time

    // UTC time (FILETIME) to local time (FILETIME)
    FileTimeToLocalFileTime( &pItem->findInfo.ftLastWriteTime,
        &lastWriteFTIME );

    // local time (FILETIME) to local time (SYSTIME)
    FileTimeToSystemTime( &lastWriteFTIME, &lastWriteSYSTIME );

    // Disp entry last modification date & time
    swprintf_s( dateStr, _countof( dateStr ),
        TEXT( "    %04d-%02d-%02d %02d:%02d:%02d" ),
        lastWriteSYSTIME.wYear,
        lastWriteSYSTIME.wMonth,
        lastWriteSYSTIME.wDay,
        lastWriteSYSTIME.wHour,
        lastWriteSYSTIME.wMinute,
        lastWriteSYSTIME.wSecond );

    // Disp entry type (dir/file)
    if ( pItem->findInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
        swprintf_s( typeStr, _countof( typeStr ),
            TEXT( " %5s" ), TEXT( "<LIN>" ) );
    else if ( pItem->findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        swprintf_s( typeStr, _countof( typeStr ),
            TEXT( " %5s" ), TEXT( "<DIR>" ) );
    else
        swprintf_s( typeStr, _countof( typeStr ),
            TEXT( " %5s" ), TEXT( "     " ) );

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
    swprintf_s( detailsStr, _countof( detailsStr ),
        TEXT( " %10s %10s %18s %s\n" ),
        dirsCtStr,
        filesCtStr,
        sizeStr,
        pItem->findInfo.cFileName );

    // Ensamble output line
    wcscpy_s( outStr, _countof( outStr ), dateStr );
    wcscat_s( outStr, _countof( outStr ), typeStr );
    wcscat_s( outStr, _countof( outStr ), detailsStr );

    // Append output line to target listbox
    SendMessage( hContsLBox, LB_ADDSTRING, 0, ( LPARAM )outStr );
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