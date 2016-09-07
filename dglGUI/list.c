//
// list.c
//
// List ADT - Interface implementation
//
// Based on listing 17.5 ( 'list.c' - C Primer Plus - Prata - 5ed )
//

#include <stdio.h>
#include <stdlib.h>
#include "list.h"

/* local function prototype */
static void CopyToNode( Item item, Node * pnode );

/* interface functions   */

/* set the list to empty */
void InitializeList( List* plist )
{
    ( *plist ).head = NULL;
    ( *plist ).end = NULL;
    ( *plist ).iCount = 0;
}

/* returns true if list is empty */
int ListIsEmpty( const List* plist )
{
    if ( ( *plist ).head == NULL )
        return true;
    else
        return false;
}

/* returns true if list is full */
int ListIsFull( const List* plist )
{
    Node* pt;
    int full;

    // Create test node
    pt = ( Node* )malloc( sizeof( Node ) );

    // Validate allocation
    if ( pt == NULL )
        full = true;
    else
        full = false;
    
    // Destroy test node
    free( pt );

    return full;
}

/* returns number of nodes */
unsigned int ListItemCount( const List* plist )
{
    return ( *plist ).iCount;
} 

/* creates node to hold item and adds it to the end of */
/* the list pointed to by plist (slow implementation)  */
int AddItem( Item item, List* plist )
{
    // Declare new node
    Node* pnew;

    // Allocate mem for new node
    pnew = ( Node* )malloc( sizeof( Node ) );
    
    // Validate allocation
    if ( pnew == NULL )
        return false;               // quit function on failure

    // Fill up new node elements
    CopyToNode( item, pnew );
    pnew->next = NULL;              // New node will be the new end

    // Check for empty list
    if ( ( *plist ).head == NULL )
        // List is empty
        ( *plist ).head = pnew;         // Point head to new node
    else
        // List has at least one node
        ( ( *plist ).end )->next = pnew;    // Point current end to new node

    // Update end pointer, point it to new node
    ( *plist ).end = pnew;

    // Increment items count
    ++( *plist ).iCount;

    return true;
}

/* visit each node and execute function pointed to by pfun */
void Traverse( List* plist,
    void ( *pfun )( Item* pItem, HWND hConstLBox ), HWND hConstLBox )
{
    Node* pnode = ( *plist ).head;      /* set to start of list   */

    while ( pnode != NULL )
    {
        ( *pfun )( &pnode->item, hConstLBox );  /* apply function to item */
        pnode = pnode->next;                    /* advance to next item   */
    }
}

/* free memory allocated by malloc() */
/* reset list structure              */
void EmptyTheList( List* plist )
{
    Node* tmp;

    while ( ( *plist ).head != NULL )
    {
        tmp = ( ( *plist ).head )->next;    /* save address of next node */
        free( ( *plist ).head );            /* free current node         */
        ( *plist ).head = tmp;              /* advance to next node      */
    }

    // Reset end pointer
    ( *plist ).end = NULL;

    // Reset items count
    ( *plist ).iCount = 0;
}

/* local function definition  */
/* copies an item into a node */
static void CopyToNode( Item item, Node* pnode )
{
    pnode->item = item;         /* structure copy */
}

/* swap nodes */
void SwapNodes( Node* pN, Node* pM )
{
    Node tmpNode = { 0 };

    // Copy item of node M to node tmp
    CopyToNode( pM->item, &tmpNode );

    // Copy item of node N to node M
    CopyToNode( pN->item, pM );

    // Copy item of node tmp to node N
    CopyToNode( tmpNode.item, pN );
}

/* sort list, sorting order depends on cmpFun */
/* cmpFun compares two items                  */
void SortList( List* plist, int ( *cmpFun )( Item* pItemN, Item* pItemM ) )
{
    Node* topPt = NULL;
    Node* seekPt = NULL;
    unsigned int top, seek;

    topPt = plist->head;

    for ( top = 1; top < plist->iCount; top++ )
    {
        seekPt = topPt->next;

        for ( seek = top + 1; seek <= plist->iCount; seek++ )
        {
            if ( ( *cmpFun )( &seekPt->item, &topPt->item ) > 0 )
                SwapNodes( seekPt, topPt );

            seekPt = seekPt->next;
        }

        topPt = topPt->next;
    }
}