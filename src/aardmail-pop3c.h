/**
 * @file aardmail-pop3c.h
 * @author Bernd Wachter <bwachter@lart.info>
 * @date 2011
 */

#ifndef _AARDMAIL_POP3C_H
#define _AARDMAIL_POP3C_H

#include <ibaard_authinfo.h>

#include <aardmail.h>

typedef struct _pop3c pop3c_configuration;

extern pop3c_configuration pop3c;

/// Structure holding runtime configuration values for pop3c
struct _pop3c {
    /// Path to the local MDA; if unset, spool to Maildir
    char *pipeto;
    /// Path to the users maildir
    char *maildir;
    /// Flag to indicate if mail should be deleted after retrieval
    int keepmail;
    /// The number of messages on the server
    int msgcount;
    /// The number of messages on the server, as string
    char *msgcount_str;
    /// Limit number of mails to retrieve
    int onlyget;
    /// The hostname or IP address pop3c binds to, if specifiedsm
    char *bindname;
    /// Don't download messages, only return the number of mails
    int onlystat;
};

#if (defined(__WIN32__)) || (defined _BROKEN_IO)
#define FDTYPE FILE*
#define FDINVAL NULL
#else
/// Type of file descriptors on this platform
#define FDTYPE int
/// Return value of open functions on this platform
#define FDINVAL -1
#endif


/** End pop3 session by writing 'quit' to specified file descriptor
 *
 * @param sd The filedescriptor to write to
 * @return -1 on error, close(sd) on success
 */
int pop3c_quitclose(int sd);

/** Send a command expecting a one-line response to sd
 *
 * @param sd The filedescriptor to write to
 * @param msg The command to send
 * @return -1 on error, 0 on success (=server responded with +OK)
 */
int pop3c_oksendline(int sd, char *msg);

/** Ask the server for the number of messages (STAT)
 *
 * @param sd The servers file descriptor
 * @return -1 on error, the number of messages on success
 */
int pop3c_getstat(int sd);

/** Open the spool fd
 *
 * Depending on command line flags, open a file in a Maildir, or a pipe to a
 * local MDA.
 *
 * @return A file descriptor to the mail, or negative numbers on error (return
 *         values of popen(), mdopen() or am_open(), depending on configuration)
 */
FDTYPE pop3c_openspool();

/** Close the fd for a mail
 *
 * Close and flush the fd, and -- for mails spooled into a Maildir -- move
 * the mail to the correct directory
 *
 * @param fd The file descriptor of the mail
 * @return 0 on success, negative numbers on error (return values of clase(),
 *         mdclose() or pclose(), depending on configuration)
 */
int pop3c_closespool(FDTYPE fd);

/** Download one message from server and save to provided file descriptor
 *
 * @param sd The servers file descriptor
 * @param fd The file descriptor of the local mail file
 * @param size unused
 * @return -1 on error, the size of the mail on success
 */
long pop3c_getmessage(int sd, FDTYPE fd, int size);

/** Connect and authenticate to a POP3 server
 *
 * Connect, set up SSL if required, and send user and password
 *
 * @param auth Pointer to an authinfo structure
 * @return -1 on error, file descriptor for the connection on success
 */
int pop3c_connectauth(authinfo *auth);

/** Download message(s) from server
 *
 * Depending on command line switches this will download some or all messages
 * from the server, keeping or deleting it on the server.
 *
 * Messages are either saved in the local Maildir, or sent to a local MDA
 * through a pipe.
 *
 * This function only sets up the environment and takes care of opening
 * an fd to store the message, the dirty work gets done in pop3c_openspool(),
 * pop3c_getmessage() and pop3c_closespool()
 *
 * @param sd The file descriptor to use
 * @return -1 on error, 0 on success
 */
int pop3c_session(int sd);

#endif
