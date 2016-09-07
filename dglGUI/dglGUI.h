//
//  dglGUI.h
//

// Children IDs
#define     ID_CURLOCLBL            0
#define     ID_CURDIRLBOX           1
#define     ID_PLACESLBL            2
#define     ID_PLACESLBOX           3
#define     ID_CONTSLBL             4
#define     ID_CONTSLBOX            5
#define     ID_SORTLBL              6
#define     ID_NAMEBTN              7
#define     ID_SIZEBTN              8
#define     ID_DATEBTN              9

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

