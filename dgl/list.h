//
// list.h
//
// List ADT - Interface declarations
//
// Based on listing 17.3 ( 'list.h' - C Primer Plus - Prata - 5ed )
//

#ifndef LIST_H_
#define LIST_H_

#include <windows.h>

// Boolean definitions
#define     false       0
#define     true        1

//================================================================
// program-specific declarations
//================================================================
struct scanOut
{
    LARGE_INTEGER dirsCount;
    LARGE_INTEGER filesCount;
    WIN32_FIND_DATA findInfo;
};

//================================================================
// general type definitions
//================================================================
typedef struct scanOut Item;

typedef struct node
{
    Item item;
    struct node* next;
} Node;

typedef struct list
{
    Node* head;
    Node* end;
    unsigned int iCount;    // Items count
} List;

//================================================================
// function prototypes
//================================================================

/* operation:        initialize a list                          */
/* preconditions:    plist points to a list                     */
/* postconditions:   the list is initialized to empty           */
void InitializeList( List* plist );

/* operation:        determine if list is empty                 */
/* precondition:     plist points to an initialized list        */
/* postconditions:   function returns True if list is empty     */
/*                   and returns False otherwise                */
int ListIsEmpty( const List* plist); 

/* operation:        determine if list is full                  */
/* precondition:     plist points to an initialized list        */
/* postconditions:   function returns True if list is full      */
/*                   and returns False otherwise                */
int ListIsFull( const List* plist );

/* operation:        determine number of items in list          */
/*                   plist points to an initialized list        */
/* postconditions:   function returns number of items in list   */
unsigned int ListItemCount( const List *plist );

/* operation:        add item to end of list                    */
/* preconditions:    item is an item to be added to list        */
/*                   plist points to an initialized list        */
/* postconditions:   if possible, function adds item to end     */
/*                   of list and returns True; otherwise the    */
/*                   function returns False                     */
int AddItem( Item item, List* plist );

/* operation:        apply a function to each item in list      */
/* preconditions:    plist points to an initialized list        */
/*                   pfun points to a function that takes an    */
/*                   Item argument and has no return value      */
/* postcondition:    the function pointed to by pfun is         */
/*                   executed once for each item in the list    */
void Traverse( List* plist, void ( *pfun )( Item* pItem ) );

/* operation:        free allocated memory, if any              */
/* precondition:     plist points to an initialized list        */
/* postconditions:   any memory allocated for the list is freed */
/*                   and the list is set to empty               */
void EmptyTheList( List* plist );

/* operation:        swap nodes                                 */
/* precondition:     pN, pM valid pointers to nodes             */
/* postconditions:   item component of nodes are swaped         */
void SwapNodes( Node* pN, Node* pM );

/* operation:        sort a list using a comparison function    */
/* precondition:     plist points to non-empty list             */
/*                   cmpFun points to comparison function       */
/* postconditions:   the list is sorted,                        */
/*                   the return values of cmpFun determine the  */
/*                   sorting order                              */
/*                   cmpFun compares two items                  */
void SortList( List* plist, int ( *cmpFun )( Item* pItemN, Item* pItemM ) );

#endif
