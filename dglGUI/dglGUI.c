//
//  dglGUI.c
//  DirGlance GUI
//

#include <windows.h>
#include <process.h>
#include "dglGUI.h"
#include "list.h"               // Definition list ADT

// Worker thread data
typedef struct paramsTag
{
     HWND*  hElem;
     HANDLE hEvent;
     int    iStatus;
     BOOL*  bNewLoc;
     TCHAR* curDir;
     List*  pResList;
     Item*  pResItem;
     int*   sortMethod;
}
PARAMS, *PPARAMS;

extern BOOL scanDir( LPTSTR tDir, List* resList, Item* parentItem, BOOL fstLevel, BOOL* pReset );
extern void showResults( List* resultsList, Item* resultsLevel, HWND hConstLBox );
extern void sortResults( List* plist, int sortMethod );

LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

void analyseSubdir( PPARAMS pparams );
void applySorting( HWND* hElem, int* pSortMeth, int newSort, List* pResList, Item* pResItem );
void createChildren( HWND* hElem, HWND hwnd, LPARAM lParam );
void sizeChildren( HWND* hElem, int cxCh, int cyCh, int cxCl, int cyCl );
BOOL fetchSelSubdir( HWND* hElem, TCHAR* currentDir );
void updateCurDirPlaces( HWND* hElem, TCHAR* currentDir );
void setupFontConts( HWND* hElem, LOGFONT* pLogfont, HFONT* pHfont );
void setupLBoxHorzExt( HWND* hElem );
void updateStateSortBtns( HWND* hElem, BOOL state );

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR szCmdLine, int iCmdShow )
{
    static TCHAR szAppName[] = TEXT( "dirGlance" );
    HWND         hwnd = 0;
    WNDCLASS     wndclass = { 0 };
    MSG          msg = { 0 };

    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = WndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon( NULL, IDI_APPLICATION );
    wndclass.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wndclass.hbrBackground = ( HBRUSH )GetStockObject( WHITE_BRUSH );
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = szAppName;

    if ( !RegisterClass( &wndclass ) )
    {
        MessageBox( NULL, TEXT( "This program requires Windows NT!" ),
            szAppName, MB_ICONERROR) ;
        return 0;
    }

    hwnd = CreateWindow(
        szAppName,
        TEXT( "Dir Glance v0.1" ),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1100, 640,
        NULL,
        NULL,
        hInstance,
        NULL );

    ShowWindow( hwnd, iCmdShow );
    UpdateWindow( hwnd );

    while ( GetMessage( &msg, NULL, 0, 0 ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    return msg.wParam;
}

void Thread( PVOID pvoid )
{
    volatile PPARAMS pparams;

    pparams = ( PPARAMS )pvoid;

    while ( TRUE )
    {
        // Wait until the event gets signaled
        WaitForSingleObject( pparams->hEvent, INFINITE );

        // Perform lengthy task
        // as long as you are allowed to
        while ( TRUE )
        {
            // Analyze current subdirectory
            analyseSubdir( pparams );

            // Check for completed task or restart
            if ( *( pparams->bNewLoc ) == TRUE )
            {
                // Start all over again
                OutputDebugString( TEXT( "Restart thread\n" ) );
                *( pparams->bNewLoc ) = FALSE;
                SendMessage( pparams->hElem[ ID_CONTSLBOX ],
                    LB_RESETCONTENT, 0, 0 );
            }
            else
            {
                // Sort results
                sortResults( pparams->pResList, *( pparams->sortMethod ) );

                // Display results
                showResults( pparams->pResList, pparams->pResItem,
                    pparams->hElem[ ID_CONTSLBOX ] );

                // Update status
                pparams->iStatus = STATUS_DONE;

                // Enalbe sorting buttons
                updateStateSortBtns( pparams->hElem, TRUE );
                
                break;
            }
        }
    }
}

LRESULT CALLBACK WndProc( HWND hwnd, UINT message,
    WPARAM wParam, LPARAM lParam )
{
    static HWND hElem[ 12 ] = { 0 };
    static int cxChar, cyChar;
    static int cxClient, cyClient;
    static TCHAR currentDir[ MAX_PATH + 1 ] = { 0 };
    static LOGFONT logfont;
    static HFONT hFont;

    static HANDLE hEvent;
    static PARAMS params;
    static BOOL bResetTask;
    static List resultsList = { 0 };
    static Item resultsItem = { 0 };
    static int sortMethod = BY_SIZE;

    switch ( message )
    {
    case WM_CREATE :
        // Get char metrics --> used as a basic children alignment unit
        cxChar = LOWORD( GetDialogBaseUnits() );
        cyChar = HIWORD( GetDialogBaseUnits() );

        // Create child windows
        createChildren( hElem, hwnd, lParam );

        // Init font of the 'Contents' listbox
        setupFontConts( hElem, &logfont, &hFont );

        // Setup listboxes horizontal extents
        setupLBoxHorzExt( hElem );

        // Update current dir & places
        updateCurDirPlaces( hElem, currentDir );

        // Disable sorting buttons
        updateStateSortBtns( hElem, FALSE );

        // Initialize results list
        InitializeList( &resultsList );

        // Seutp worker thread infos
        hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

        bResetTask = FALSE;
        sortMethod = BY_SIZE;

        params.hElem = hElem;
        params.hEvent = hEvent;
        params.iStatus = STATUS_WORKING;
        params.bNewLoc = &bResetTask;
        params.curDir = currentDir;
        params.pResList = &resultsList;
        params.pResItem = &resultsItem;
        params.sortMethod = &sortMethod;

        // Start worker thread (paused)
        _beginthread( Thread, 0, &params );

        // Trigger initial dir scan
        SetEvent( hEvent );
        return 0;

    case WM_COMMAND :
        if ( LOWORD( wParam ) == ID_PLACESLBOX &&
             HIWORD( wParam ) == LBN_DBLCLK )
        {
            // Fetch selected subdir and change to it
            if ( !( fetchSelSubdir( hElem, currentDir ) ) )
                break;

            // Update 'Contents' list box
            // Trigger a new scan
            bResetTask = TRUE;
            params.curDir = currentDir;

            if ( params.iStatus == STATUS_DONE )
            {
                // Update status
                params.iStatus = STATUS_WORKING;

                // Disable sorting buttons
                updateStateSortBtns( hElem, FALSE );

                // Trigger next dir scan
                SetEvent( hEvent );
            }
        }

        if ( LOWORD( wParam ) == ID_NAMEBTN &&      
             HIWORD( wParam ) == BN_CLICKED )
        {
            // Sort results by Name
            applySorting( hElem, &sortMethod, BY_NAME, &resultsList, &resultsItem );
        }

        if ( LOWORD( wParam ) == ID_SIZEBTN &&      
             HIWORD( wParam ) == BN_CLICKED )
        {
            // Sort results by Size
            applySorting( hElem, &sortMethod, BY_SIZE, &resultsList, &resultsItem );
        }

        if ( LOWORD( wParam ) == ID_DATEBTN &&      
             HIWORD( wParam ) == BN_CLICKED )
        {
            // Sort results by Date
            applySorting( hElem, &sortMethod, BY_MODIF, &resultsList, &resultsItem );
        }

        if ( LOWORD( wParam ) == ID_DIRSBTN &&      
             HIWORD( wParam ) == BN_CLICKED )
        {
            // Sort results by Dirs
            applySorting( hElem, &sortMethod, BY_DIRS, &resultsList, &resultsItem );
        }

        if ( LOWORD( wParam ) == ID_FILESBTN &&      
             HIWORD( wParam ) == BN_CLICKED )
        {
            // Sort results by Files
            applySorting( hElem, &sortMethod, BY_FILES, &resultsList, &resultsItem );
        }

        return 0;

    case WM_SIZE :
        // Get client size
        cxClient = LOWORD( lParam );
        cyClient = HIWORD( lParam );

        // Size children
        sizeChildren( hElem, cxChar, cyChar, cxClient, cyClient );
        return 0;

    case WM_DESTROY :
        // Housekeeping
        EmptyTheList( &resultsList );
        DeleteObject( hFont );

        // Close the application
        PostQuitMessage( 0 );
        return 0;
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}

void analyseSubdir( PPARAMS pparams )
{
    PVOID oldValueWow64 = NULL;
    BOOL wow64Disabled = FALSE;

    // Reset results list
    EmptyTheList( pparams->pResList );

    // Reset results item
    ZeroMemory( pparams->pResItem, sizeof( Item ) );

    // Disable file system redirection
    wow64Disabled = Wow64DisableWow64FsRedirection( &oldValueWow64 );

    // Scan target dir
    scanDir( pparams->curDir, pparams->pResList, pparams->pResItem,
        TRUE, pparams->bNewLoc );

    // Re-enable redirection
    if ( wow64Disabled )
    {
        if ( !( Wow64RevertWow64FsRedirection( oldValueWow64 ) ) )
            MessageBox( NULL, TEXT( "Re-enable redirection failed." ),
            TEXT( "Dir Glance" ), MB_ICONERROR);
    }
}

void applySorting( HWND* hElem, int* pSortMeth, int newSort,
    List* pResList, Item* pResItem )
{
    // Disable sorting buttons
    updateStateSortBtns( hElem, FALSE );

    // Delete displayed contents
    SendMessage( hElem[ ID_CONTSLBOX ], LB_RESETCONTENT, 0, 0 );

    // Update sorting method
    *pSortMeth = newSort;

    // Sort results
    sortResults( pResList, newSort );

    // Display results
    showResults( pResList, pResItem, hElem[ ID_CONTSLBOX ] );

    // Enable sorting buttons
    updateStateSortBtns( hElem, TRUE );
}

void createChildren( HWND* hElem, HWND hwnd, LPARAM lParam )
{
    // Create 'Current Location' label
    hElem[ ID_CURLOCLBL ] = CreateWindow(
        TEXT( "static" ), TEXT( "Current Location :" ),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, 0, 0,
        hwnd, ( HMENU )( ID_CURLOCLBL ),
        ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

    // Create 'Current Dir' list box
    hElem[ ID_CURDIRLBOX ] = CreateWindow(
        TEXT( "listbox" ), NULL,
        WS_CHILD | WS_VISIBLE | LBS_STANDARD | WS_HSCROLL,
        0, 0, 0, 0,
        hwnd, ( HMENU )( ID_CURDIRLBOX ),
        ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

    // Create 'Places' label
    hElem[ ID_PLACESLBL ] = CreateWindow(
        TEXT( "static" ), TEXT( "Places :" ),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, 0, 0,
        hwnd, ( HMENU )( ID_PLACESLBL ),
        ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

    // Create 'Places' list box
    hElem[ ID_PLACESLBOX ] = CreateWindow(
        TEXT( "listbox" ), NULL,
        WS_CHILD | WS_VISIBLE | LBS_STANDARD | WS_HSCROLL,
        0, 0, 0, 0,
        hwnd, ( HMENU )( ID_PLACESLBOX ),
        ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

    // Create 'Contents' label
    hElem[ ID_CONTSLBL ] = CreateWindow(
        TEXT( "static" ), TEXT( "Contents :" ),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, 0, 0,
        hwnd, ( HMENU )( ID_CONTSLBL ),
        ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

    // Create 'Contents' list box
    hElem[ ID_CONTSLBOX ] = CreateWindow(
        TEXT( "listbox" ), NULL,
        WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_BORDER |
        WS_VSCROLL | WS_HSCROLL,
        0, 0, 0, 0,
        hwnd, ( HMENU )( ID_CONTSLBOX ),
        ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

    // Create 'Sort' label
    hElem[ ID_SORTLBL ] = CreateWindow(
        TEXT( "static" ), TEXT( "Sort :" ),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, 0, 0,
        hwnd, ( HMENU )( ID_SORTLBL ),
        ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

    // Create 'By Name' button
    hElem[ ID_NAMEBTN ] = CreateWindow(
        TEXT( "button" ), TEXT( "By Name" ),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hwnd, ( HMENU )( ID_NAMEBTN ),
        ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

    // Create 'By Size' button
    hElem[ ID_SIZEBTN ] = CreateWindow(
        TEXT( "button" ), TEXT( "By Size" ),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hwnd, ( HMENU )( ID_SIZEBTN ),
        ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

    // Create 'By Date' button
    hElem[ ID_DATEBTN ] = CreateWindow(
        TEXT( "button" ), TEXT( "By Date" ),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hwnd, ( HMENU )( ID_DATEBTN ),
        ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

    // Create 'By Dirs' button
    hElem[ ID_DIRSBTN ] = CreateWindow(
        TEXT( "button" ), TEXT( "By Dirs" ),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hwnd, ( HMENU )( ID_DIRSBTN ),
        ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

    // Create 'By Files' button
    hElem[ ID_FILESBTN ] = CreateWindow(
        TEXT( "button" ), TEXT( "By Files" ),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hwnd, ( HMENU )( ID_FILESBTN ),
        ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );
}

void sizeChildren( HWND* hElem, int cxCh, int cyCh, int cxCl, int cyCl )
{
    // Size and position 'Current Location' label
    MoveWindow( hElem[ ID_CURLOCLBL ],
        cyCh, cyCh,
        24 * cxCh, cyCh, TRUE);

    // Size and position 'Current Dir' list box
    MoveWindow( hElem[ ID_CURDIRLBOX ],
        cyCh, 2 * cyCh,
        cxCl - 3 * cyCh, 3 * cyCh, TRUE);

    // Size and position 'Places' label
    MoveWindow( hElem[ ID_PLACESLBL ],
        cyCh, 5 * cyCh,
        24 * cxCh, cyCh, TRUE);

    // Size and position 'Places' list box
    MoveWindow( hElem[ ID_PLACESLBOX ],
        cyCh, 6 * cyCh,
        30 * cxCh, cyCl - 7 * cyCh, TRUE);

    // Size and position 'Contents' label
    MoveWindow( hElem[ ID_CONTSLBL ],
        cyCh + 30 * cxCh + cyCh, 5 * cyCh,
        24 * cxCh, cyCh, TRUE);

    // Size and position 'Contents' list box
    MoveWindow( hElem[ ID_CONTSLBOX ],
        cyCh + 30 * cxCh + cyCh, 6 * cyCh,
        cxCl - ( cyCh + 30 * cxCh + cyCh + cyCh),
        cyCl - 12 * cyCh, TRUE);

    // Size and position 'Sort' label
    MoveWindow( hElem[ ID_SORTLBL ],
        cyCh + 30 * cxCh + cyCh, cyCl - 5 * cyCh,
        24 * cxCh, cyCh, TRUE);

    // Size and position 'By Date' button
    MoveWindow( hElem[ ID_DATEBTN ],
        cyCh + 30 * cxCh + cyCh,
        cyCl - 4 * cyCh,
        12 * cxCh, 2 * cyCh, TRUE);

    // Size and position 'By Dirs' button
    MoveWindow( hElem[ ID_DIRSBTN ],
        cyCh + 30 * cxCh + cyCh + 1 * 13 * cxCh,
        cyCl - 4 * cyCh,
        12 * cxCh, 2 * cyCh, TRUE);

    // Size and position 'By Files' button
    MoveWindow( hElem[ ID_FILESBTN ],
        cyCh + 30 * cxCh + cyCh + 2 * 13 * cxCh,
        cyCl - 4 * cyCh,
        12 * cxCh, 2 * cyCh, TRUE);

    // Size and position 'By Size' button
    MoveWindow( hElem[ ID_SIZEBTN ],
        cyCh + 30 * cxCh + cyCh + 3 * 13 * cxCh,
        cyCl - 4 * cyCh,
        12 * cxCh, 2 * cyCh, TRUE);

    // Size and position 'By Name' button
    MoveWindow( hElem[ ID_NAMEBTN ],
        cyCh + 30 * cxCh + cyCh + 4 * 13 * cxCh,
        cyCl - 4 * cyCh,
        12 * cxCh, 2 * cyCh, TRUE);
}

BOOL fetchSelSubdir( HWND* hElem, TCHAR* currentDir )
{
    int currentSelIndex = 0;
    TCHAR selSubDir[ MAX_PATH + 1 ] = { 0 };

    // Fetch selected subdir and change to it
    if ( LB_ERR == ( currentSelIndex =
        SendMessage( hElem[ ID_PLACESLBOX ], LB_GETCURSEL, 0, 0 ) ) )
        return FALSE;

    SendMessage( hElem[ ID_PLACESLBOX ], LB_GETTEXT, currentSelIndex,
        ( LPARAM )selSubDir );

    selSubDir[ lstrlen( selSubDir ) - 1 ] = '\0';

    // Try to go to selected subdirectory,
    // if that doesn't work it has to be a drive
    if ( !SetCurrentDirectory( selSubDir + 1 ) )
    {
        selSubDir[ 3 ] = ':';
        selSubDir[ 4 ] = '\0';
        SetCurrentDirectory( selSubDir + 2 );
    }

    // Update current dir & places
    updateCurDirPlaces( hElem, currentDir );

    return TRUE;
}

void updateCurDirPlaces( HWND* hElem, TCHAR* currentDir )
{
    // Get current dir
    memset( currentDir, 0, sizeof( TCHAR ) * ( MAX_PATH + 1 ) );
    GetCurrentDirectory( MAX_PATH + 1, currentDir );

    // Update 'Current Dir' list box
    SendMessage( hElem[ ID_CURDIRLBOX ], LB_RESETCONTENT, 0, 0 );
    SendMessage( hElem[ ID_CURDIRLBOX ], LB_ADDSTRING, 0, ( LPARAM )currentDir );

    // Reload the 'Places' list box
    SendMessage( hElem[ ID_PLACESLBOX ], LB_RESETCONTENT, 0, 0 );
    SendMessage( hElem[ ID_PLACESLBOX ], LB_DIR,
        DDL_DRIVES | DDL_DIRECTORY | DDL_EXCLUSIVE,
        ( LPARAM )TEXT( "*" ) );
}

void setupFontConts( HWND* hElem, LOGFONT* pLogfont, HFONT* pHfont )
{
    GetObject( GetStockObject( SYSTEM_FIXED_FONT ), sizeof( LOGFONT ), 
        ( PTSTR )pLogfont );

    *pHfont = CreateFontIndirect( pLogfont );

    SendMessage( hElem[ ID_CONTSLBOX ], WM_SETFONT, ( WPARAM )(*pHfont), 0 );
}

void setupLBoxHorzExt( HWND* hElem )
{
    SendMessage( hElem[ ID_CURDIRLBOX ], LB_SETHORIZONTALEXTENT, 1000, 0 );
    SendMessage( hElem[ ID_PLACESLBOX ], LB_SETHORIZONTALEXTENT, 1000, 0 );
    SendMessage( hElem[ ID_CONTSLBOX ], LB_SETHORIZONTALEXTENT, 1000, 0 );
}

void updateStateSortBtns( HWND* hElem, BOOL state )
{
    EnableWindow( hElem[ ID_NAMEBTN ], state );
    EnableWindow( hElem[ ID_SIZEBTN ], state );
    EnableWindow( hElem[ ID_DATEBTN ], state );
    EnableWindow( hElem[ ID_DIRSBTN ], state );
    EnableWindow( hElem[ ID_FILESBTN ], state );
}