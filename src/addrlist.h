/**
 * @file addrlist.h
 * @author Bernd Wachter <bwachter@lart.info>
 * @date 2005-2011
 */

#ifndef _ADDRLIST_H
#define _ADDRLIST_H

#ifdef __WIN32__
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif

#include "aardmail.h"

/// Data type for storing email addresses
typedef struct _addrlist addrlist;

/// A structure to store a list of email addresses
struct _addrlist {
    /// An email address
    char address[AM_MAXUSER];
    /// Flag for processing notes, currently unused
    int accepted;
    /// Pointer to the next entry
    addrlist *next;
};

/**
 * Append an address to an addrlist list
 *
 * @param addrlist_storage the list to append to
 * @param address the address to append
 * @return 0 on success, -1 on error
 */
int addrlist_append(addrlist **addrlist_storage, char *address);

/**
 * Delete an address to an addrlist list
 *
 * @param addrlist_storage the list to delete from
 * @param address the address to delete
 * @return 0 on success, -1 on error
 */
int addrlist_delete(addrlist **addrlist_storage, char *address);

/**
 * Completely free an addrlist list
 *
 * @param addrlist_storage the list to free
 * @return 0 on success, -1 on error
 */
int addrlist_free(addrlist **addrlist_storage);
#endif
