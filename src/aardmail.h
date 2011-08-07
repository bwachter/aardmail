/**
 * @file aardmail.h
 * @author Bernd Wachter <bwachter@lart.info>
 * @date 2005-2011
 */

#ifndef _AARDMAIL_H
#define _AARDMAIL_H

#include <ibaard_types.h>
#include <ibaard.h>

#include "version.h"

/// General log messages
#define F_GENERAL "GENERAL"
/// Network related log messages
#define F_NET "NET"
/// Log messages from Maildir handling
#define F_MAILDIR "MAILDIR"
/// Log messages related to authinfo parsing
#define F_AUTHINFO "AUTHINFO"
/// Log messages from addrlist handling
#define F_ADDRLIST "ADDRLIST"
/// Log messages in the SSL code
#define F_SSL "SSL"

/** Execute a program to test exit status
 *
 * This function is used to determine based on the exit status of a program
 * if emails should be retreived/sent. A possible way to use it would be
 * writing a small program querying the connection status from connection
 * managers.
 *
 * @param program a path to the program to execute
 * @return 0 if program executed successfully, -1 on error
 */
int am_checkprogram(char *program);
#if (!defined(__WIN32__)) && (!defined _BROKEN_IO)
/** Open a pipe and redirect stdio
 *
 * @param pipeto
 * @return the file descriptor connected to pipetos STDIN
 */
int am_pipe(char *pipeto);
#endif

/** Placeholder function for unimplemented features
 *
 * @return nothing, but exits the program
 */
void am_unimplemented();
#endif
