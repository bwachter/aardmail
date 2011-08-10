/**
 * @file pop3c_openspool.c
 * @author Bernd Wachter <bwachter-usenet@lart.info>
 * @date 2011
 */

#include <ibaard_maildir.h>

#include "aardmail-pop3c.h"

/// The unique name of a file inside a Maildir (i.e., where pop3c spools to)
char *uniqname;

FDTYPE pop3c_openspool(){
  FDTYPE fd;
  if (pop3c.pipeto){
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
    fd=popen(pop3c.pipeto, "w");
#else
    fd=am_pipe(pop3c.pipeto);
#endif
  } else
    fd=mdopen(NULL, &uniqname);

  if (fd == FDINVAL)
    logmsg(L_ERROR, F_GENERAL, "opening spool failed", NULL);
  return fd;
}
