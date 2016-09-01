//
//  repError.c
//
//  Based on 'Windows System Programming - Hart - 4Ed'
//  ( '..\WSP4_Examples\UTILITY\REPRTERR.C' )
//
//  General-purpose function for reporting system errors.
//
//  Obtain the error number and convert it to the system error message.
//  Display this information and the user-specified message
//  to the standard error device.
//
//  userMsg     : Message to be displayed to standard error device.
//  exitCode    :  0   - Return.
//                >0   - ExitProcess with this code.
//  prtErrorMsg : Display the last system error message
//                if this flag is set.

#include <windows.h>
#include <stdio.h>

VOID ReportError( LPCTSTR userMsg, DWORD exitCode, BOOL prtErrorMsg )
{
    DWORD eMsgLen, errNum;
    LPTSTR lpvSysMsg = NULL;

    errNum = GetLastError();

    fwprintf( stderr, TEXT( "%s\n" ), userMsg );

    if ( prtErrorMsg )
    {
        eMsgLen = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            errNum,
            MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
            ( LPTSTR )&lpvSysMsg,
            0,
            NULL );

        if ( eMsgLen > 0 )
            fwprintf( stderr, TEXT( "%s\n" ), lpvSysMsg );
        else
            fwprintf( stderr, TEXT( "Last Error Number: %d.\n"), errNum );

        if ( lpvSysMsg != NULL )
            LocalFree( lpvSysMsg );
    }

    if ( exitCode > 0 )
        ExitProcess( exitCode );

    return;
}
