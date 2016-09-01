//
//  dglGUI.c
//  DirGlance GUI
//

#include <windows.h>

#define     ID_CurLocLbl            1
#define     ID_CurDirLBox           2
#define     ID_PlacesLbl            3
#define     ID_PlacesLBox           4
#define     ID_ContsLbl             5
#define     ID_ContsLBox            6
#define     ID_SortLbl              7
#define     ID_NameBtn              8
#define     ID_SizeBtn              9
#define     ID_DateBtn              10

extern void analyseSubDir( TCHAR* targetDir, HWND hContsLBox );

LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

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

LRESULT CALLBACK WndProc( HWND hwnd, UINT message,
    WPARAM wParam, LPARAM lParam )
{
    static HWND hCurLocLbl, hCurDirLBox;
    static HWND hPlacesLbl, hPlacesLBox;
    static HWND hContsLbl, hContsLBox;
    static HWND hSortLbl, hNameBtn, hSizeBtn, hDateBtn;
    static int cxChar, cyChar;
    static int cxClient, cyClient;
    TCHAR currentDir[ MAX_PATH + 1 ] = { 0 };
    TCHAR selSubDir[ MAX_PATH + 1 ] = { 0 };
    int currentSelIndex;
    static LOGFONT logfont;
    static HFONT hFont;

    switch ( message )
    {
    case WM_CREATE :
        // Get char metrics --> used as a basic children alignment unit
        cxChar = LOWORD( GetDialogBaseUnits() );
        cyChar = HIWORD( GetDialogBaseUnits() );

        // Create 'Current Location' label
        hCurLocLbl = CreateWindow(
            TEXT( "static" ), TEXT( "Current Location :" ),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            hwnd, ( HMENU )( ID_CurLocLbl ),
            ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

        // Create 'Current Dir' list box
        hCurDirLBox = CreateWindow(
            TEXT( "listbox" ), NULL,
            WS_CHILD | WS_VISIBLE | LBS_STANDARD | WS_HSCROLL,
            0, 0, 0, 0,
            hwnd, ( HMENU )( ID_CurDirLBox ),
            ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

        // Create 'Places' label
        hPlacesLbl = CreateWindow(
            TEXT( "static" ), TEXT( "Places :" ),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            hwnd, ( HMENU )( ID_PlacesLbl ),
            ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

        // Create 'Places' list box
        hPlacesLBox = CreateWindow(
            TEXT( "listbox" ), NULL,
            WS_CHILD | WS_VISIBLE | LBS_STANDARD | WS_HSCROLL,
            0, 0, 0, 0,
            hwnd, ( HMENU )( ID_PlacesLBox ),
            ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

        // Create 'Contents' label
        hContsLbl = CreateWindow(
            TEXT( "static" ), TEXT( "Contents :" ),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            hwnd, ( HMENU )( ID_ContsLbl ),
            ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

        // Create 'Contents' list box
        hContsLBox = CreateWindow(
            TEXT( "listbox" ), NULL,
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_BORDER |
                WS_VSCROLL | WS_HSCROLL,
            0, 0, 0, 0,
            hwnd, ( HMENU )( ID_ContsLBox ),
            ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

        // Create 'Sort' label
        hSortLbl = CreateWindow(
            TEXT( "static" ), TEXT( "Sort :" ),
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            hwnd, ( HMENU )( ID_SortLbl ),
            ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

        // Create 'By Name' button
        hNameBtn = CreateWindow(
            TEXT( "button" ), TEXT( "By Name" ),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0,
            hwnd, ( HMENU )( ID_NameBtn ),
            ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

        // Create 'By Size' button
        hSizeBtn = CreateWindow(
            TEXT( "button" ), TEXT( "By Size" ),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0,
            hwnd, ( HMENU )( ID_SizeBtn ),
            ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

        // Create 'By Date' button
        hDateBtn = CreateWindow(
            TEXT( "button" ), TEXT( "By Date" ),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0,
            hwnd, ( HMENU )( ID_DateBtn ),
            ( ( LPCREATESTRUCT )lParam )->hInstance, NULL );

        // Initialize font of the 'Contents' listbox
        GetObject( GetStockObject( SYSTEM_FIXED_FONT ), sizeof( LOGFONT ), 
            ( PTSTR )&logfont );
        hFont = CreateFontIndirect( &logfont );
        SendMessage( hContsLBox, WM_SETFONT, ( WPARAM )hFont, 0 );

        // Setup listboxes horizontal extents
        SendMessage( hCurDirLBox, LB_SETHORIZONTALEXTENT, 1000, 0 );
        SendMessage( hPlacesLBox, LB_SETHORIZONTALEXTENT, 1000, 0 );
        SendMessage( hContsLBox, LB_SETHORIZONTALEXTENT, 1000, 0 );

        // Get current dir
        GetCurrentDirectory( MAX_PATH + 1, currentDir );

        // Show current dir in list box
        SendMessage( hCurDirLBox, LB_ADDSTRING, 0, ( LPARAM )currentDir );

        // Populate the 'Places' list box
        SendMessage( hPlacesLBox, LB_DIR,
            DDL_DRIVES | DDL_DIRECTORY | DDL_EXCLUSIVE,
            ( LPARAM )TEXT( "*" ) );

        // Update 'Contents' list box
        analyseSubDir( currentDir, hContsLBox );

        return 0;

    case WM_SIZE :
        // Get client size
        cxClient = LOWORD( lParam );
        cyClient = HIWORD( lParam );

        // Size and position 'Current Location' label
        MoveWindow( hCurLocLbl,
            cyChar, cyChar,
            24 * cxChar, cyChar, TRUE);

        // Size and position 'Current Dir' list box
        MoveWindow( hCurDirLBox,
            cyChar, 2 * cyChar,
            cxClient - 3 * cyChar, 3 * cyChar, TRUE);

        // Size and position 'Places' label
        MoveWindow( hPlacesLbl,
            cyChar, 5 * cyChar,
            24 * cxChar, cyChar, TRUE);

        // Size and position 'Places' list box
        MoveWindow( hPlacesLBox,
            cyChar, 6 * cyChar,
            30 * cxChar, cyClient - 7 * cyChar, TRUE);

        // Size and position 'Contents' label
        MoveWindow( hContsLbl,
            cyChar + 30 * cxChar + cyChar, 5 * cyChar,
            24 * cxChar, cyChar, TRUE);

        // Size and position 'Contents' list box
        MoveWindow( hContsLBox,
            cyChar + 30 * cxChar + cyChar, 6 * cyChar,
            cxClient - ( cyChar + 30 * cxChar + cyChar + cyChar),
            cyClient - 12 * cyChar, TRUE);

        // Size and position 'Sort' label
        MoveWindow( hSortLbl,
            cyChar + 30 * cxChar + cyChar, cyClient - 5 * cyChar,
            24 * cxChar, cyChar, TRUE);

        // Size and position 'By Name' button
        MoveWindow( hNameBtn,
            cyChar + 30 * cxChar + cyChar, cyClient - 4 * cyChar,
            12 * cxChar, 2 * cyChar, TRUE);

        // Size and position 'By Size' button
        MoveWindow( hSizeBtn,
            cyChar + 30 * cxChar + cyChar + 13 * cxChar, cyClient - 4 * cyChar,
            12 * cxChar, 2 * cyChar, TRUE);

        // Size and position 'By Date' button
        MoveWindow( hDateBtn,
            cyChar + 30 * cxChar + cyChar + 13 * cxChar + 13 * cxChar,
            cyClient - 4 * cyChar,
            12 * cxChar, 2 * cyChar, TRUE);

        return 0;

    case WM_COMMAND :
        if ( LOWORD( wParam ) == ID_PlacesLBox &&
             HIWORD( wParam ) == LBN_DBLCLK )
        {
            if ( LB_ERR == ( currentSelIndex =
                SendMessage( hPlacesLBox, LB_GETCURSEL, 0, 0 ) ) )
                break;

            SendMessage( hPlacesLBox, LB_GETTEXT, currentSelIndex,
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

            // Update 'Current Dir' list box
            SendMessage( hCurDirLBox, LB_RESETCONTENT, 0, 0 );
            SendMessage( hCurDirLBox, LB_ADDSTRING, 0, ( LPARAM )currentDir );

            // Reload the 'Places' list box
            SendMessage( hPlacesLBox, LB_RESETCONTENT, 0, 0 );
            SendMessage( hPlacesLBox, LB_DIR,
                DDL_DRIVES | DDL_DIRECTORY | DDL_EXCLUSIVE,
                ( LPARAM )TEXT( "*" ) );

            // Update 'Contents' list box
            SendMessage( hContsLBox, LB_RESETCONTENT, 0, 0 );
            analyseSubDir( currentDir, hContsLBox );

            InvalidateRect (hwnd, NULL, TRUE) ;
        }
        return 0;

    case WM_DESTROY :
        // Close the application
        PostQuitMessage( 0 );
        return 0;
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}