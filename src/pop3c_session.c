/**
 * @file pop3c_session.c
 * @author Bernd Wachter <bwachter-usenet@lart.info>
 * @date 2011
 */

#include <stdlib.h>
#include <stdio.h>

#include <ibaard_cat.h>

#include "aardmail-pop3c.h"

int pop3c_session(int sd){
  FDTYPE fd;
  int i;
  char *pop3_msgnrbuf[1024];
  char *tmpstring=NULL;

  if ((pop3c.msgcount=pop3c_getstat(sd)) == -1)
    exit(-1);

  if (pop3c.onlystat){
    lmsg("There are ", pop3c.msgcount_str, " messages on server", NULL);
    return 0;
  }

  if (pop3c.onlyget == 0 || pop3c.onlyget > pop3c.msgcount)
    pop3c.onlyget = pop3c.msgcount;

  for (i=1;i<=pop3c.onlyget;i++){
    if (sprintf((char *)pop3_msgnrbuf, "%i", i)==0){
      logmsg(L_ERROR, F_GENERAL, "sprintf() failed", NULL);
      return -1;
    }
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
    if ((fd=pop3c_openspool())==NULL) return -1;
#else
    if ((fd=pop3c_openspool())==-1) return -1;
#endif
    //pop3c_getsize(sd, i);
    if (!cat(&tmpstring, "retr ", pop3_msgnrbuf, "\r\n", NULL)){
      int del_error=0;
      if ((pop3c_oksendline(sd, tmpstring)) == -1) return -1;
      if (pop3c_getmessage(sd, fd, 0) < 0) del_error=1;
      if (pop3c_closespool(fd)!=0 || del_error==1) {
        logmsg(L_ERROR, F_GENERAL, "Unable to save message", NULL);
        return -1;
      }
      if (!pop3c.keepmail) {
        if (cat(&tmpstring, "dele ", pop3_msgnrbuf, "\r\n", NULL))
          return -1;
        else
          if ((pop3c_oksendline(sd, tmpstring)) == -1) {
            free(tmpstring);
            return -1;
          }
      }
    }
  }

  // cat() takes care of freeing used buffers, so freeing it
  // is only required at the end
  free(tmpstring);
  return 0;
}
