//
//  dglGUI.c
//  DirGlance GUI
//

#include <windows.h>
#include <process.h>
#include "list.h"               // Definition list ADT

// Children IDs
#define     ID_CURLOCLBL            1
#define     ID_CURDIRLBOX           2
#define     ID_PLACESLBL            3
#define     ID_PLACESLBOX           4
#define     ID_CONTSLBL             5
#define     ID_CONTSLBOX            6
#define     ID_SORTLBL              7
#define     ID_NAMEBTN              8
#define     ID_SIZEBTN              9
#define     ID_DATEBTN              10

// Status IDs
#define     STATUS_READY            0
#define     STATUS_WORKING          1
#define     STATUS_DONE             2

// Sorting methods
#define     BY_SIZE         0   // Sort by size [bytes] (default)
#define     BY_FILES        1   // Sort by files count (descending)
#define     BY_DIRS         2   // Sort by dirs count (descending)
#define     BY_MODIF        3   // Sort by date modified (latest to earliest)
#define     BY_NAME         4   // Soft by name (a to Z)
#define     BY_TYPE         5   // Sort by type (<DIR>, <LIN>, file)

typedef struct
{
     HWND   hwndLBox;
     HANDLE hEvent;
     int    iStatus;
     BOOL*  bNewLoc;
     TCHAR* curDir;
     List*  pResList;
     Item*  pResItem;
     int    sortMethod;
}
PARAMS, *PPARAMS;

extern BOOL scanDir( LPTSTR tDir, List* resList, Item* parentItem, BOOL fstLevel, BOOL* pReset );
extern void showResults( List* resultsList, Item* resultsLevel, HWND hConstLBox );
extern void sortResults( List* plist, int sortMethod );

LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

void createChildren( HWND* hElem, HWND hwnd, LPARAM lParam );
void sizeChildren( HWND* hElem, int cxCh, int cyCh, int cxCl, int cyCl );
void updateCurDirPlaces( HWND* hElem, TCHAR* currentDir );
void setupFontConts( HWND* hElem, LOGFONT* pLogfont, HFONT* pHfont );
void setupLBoxHorzExt( HWND* hElem );

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
    PVOID oldValueWow64 = NULL;
    BOOL wow64Disabled = FALSE;

    pparams = ( PPARAMS )pvoid;

    while ( TRUE )
    {
        // Wait until the event gets signaled
        WaitForSingleObject( pparams->hEvent, INFINITE );

        // Perform lengthy task
        // as long as you are allowed to
        while ( TRUE )
        {
            // Disable file system redirection
            wow64Disabled = Wow64DisableWow64FsRedirection( &oldValueWow64 );

            // Reset results list
            EmptyTheList( pparams->pResList );

            // Reset results item
            ZeroMemory( pparams->pResItem, sizeof( Item ) );

            // Scan target dir
            scanDir( pparams->curDir, pparams->pResList, pparams->pResItem,
                TRUE, pparams->bNewLoc );

            // Re-enable redirection
            if ( wow64Disabled )
            {
                if ( !( Wow64RevertWow64FsRedirection( oldValueWow64 ) ) )
                    MessageBox( NULL, TEXT( "Re-enable redirection failed." ),
                        TEXT( "Dir Glance" ), MB_ICONERROR) ;
            }

            // Sort results
            if ( *( pparams->bNewLoc ) == FALSE )
                sortResults( pparams->pResList, pparams->sortMethod );

            // Display results
            if ( *( pparams->bNewLoc ) == FALSE )
                showResults( pparams->pResList, pparams->pResItem,
                    pparams->hwndLBox );


            if ( *( pparams->bNewLoc ) == TRUE )
            {
                // Start all over again
                OutputDebugString( TEXT( "Restart thread\n" ) );
                *( pparams->bNewLoc ) = FALSE;
                SendMessage( pparams->hwndLBox, LB_RESETCONTENT, 0, 0 );
            }
            else
            {
                pparams->iStatus = STATUS_DONE;
                break;
            }
        }
    }
}

LRESULT CALLBACK WndProc( HWND hwnd, UINT message,
    WPARAM wParam, LPARAM lParam )
{
    static HWND hElem[ 10 ] = { 0 };
    static int cxChar, cyChar;
    static int cxClient, cyClient;
    static TCHAR currentDir[ MAX_PATH + 1 ] = { 0 };
    static TCHAR selSubDir[ MAX_PATH + 1 ] = { 0 };
    int currentSelIndex;
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

        // Get current dir
        GetCurrentDirectory( MAX_PATH + 1, currentDir );

        // Update current dir & places
        updateCurDirPlaces( hElem, currentDir );

        // Update 'Contents' list box ---> lengthy task !!!!
        InitializeList( &resultsList );

        // Seutp worker thread infos
        hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

        bResetTask = FALSE;
        sortMethod = BY_SIZE;

        params.hwndLBox = hElem[ ID_CONTSLBOX ];
        params.hEvent = hEvent;
        params.iStatus = STATUS_WORKING;
        params.bNewLoc = &bResetTask;
        params.curDir = currentDir;
        params.pResList = &resultsList;
        params.pResItem = &resultsItem;
        params.sortMethod = sortMethod;

        // Start worker thread (paused)
        _beginthread( Thread, 0, &params );

        // Trigger initial dir scan
        SetEvent( hEvent );
        return 0;

    case WM_SIZE :
        // Get client size
        cxClient = LOWORD( lParam );
        cyClient = HIWORD( lParam );

        // Size children
        sizeChildren( hElem, cxChar, cyChar, cxClient, cyClient );
        return 0;

    case WM_COMMAND :
        if ( LOWORD( wParam ) == ID_PLACESLBOX &&
             HIWORD( wParam ) == LBN_DBLCLK )
        {
            if ( LB_ERR == ( currentSelIndex =
                SendMessage( hElem[ ID_PLACESLBOX ], LB_GETCURSEL, 0, 0 ) ) )
                break;

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

            // Get the new directory name and fill the list box.
            GetCurrentDirectory( MAX_PATH + 1, currentDir );

            // Update current dir & places
            updateCurDirPlaces( hElem, currentDir );

            // Update 'Contents' list box
            // Trigger a new scan
            bResetTask = TRUE;
            params.curDir = currentDir;

            if ( params.iStatus == STATUS_DONE )
            {
                params.iStatus = STATUS_WORKING;
                SetEvent( hEvent );
            }

            InvalidateRect (hwnd, NULL, TRUE) ;
        }
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

        // Size and position 'By Name' button
        MoveWindow( hElem[ ID_NAMEBTN ],
            cyCh + 30 * cxCh + cyCh, cyCl - 4 * cyCh,
            12 * cxCh, 2 * cyCh, TRUE);

        // Size and position 'By Size' button
        MoveWindow( hElem[ ID_SIZEBTN ],
            cyCh + 30 * cxCh + cyCh + 13 * cxCh, cyCl - 4 * cyCh,
            12 * cxCh, 2 * cyCh, TRUE);

        // Size and position 'By Date' button
        MoveWindow( hElem[ ID_DATEBTN ],
            cyCh + 30 * cxCh + cyCh + 13 * cxCh + 13 * cxCh,
            cyCl - 4 * cyCh,
            12 * cxCh, 2 * cyCh, TRUE);
}

void updateCurDirPlaces( HWND* hElem, TCHAR* currentDir )
{
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