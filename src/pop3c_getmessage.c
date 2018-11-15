/**
 * @file pop3c_getmessage.c
 * @author Bernd Wachter <bwachter-usenet@lart.info>
 * @date 2011
 */

#include <ibaard_network.h>

#include "aardmail-pop3c.h"

long pop3c_getmessage(int sd, FDTYPE fd, int size){
  char *tmp;
  char *buf[MAXNETBUF];
  int i, delayrn=0;
  long fsize=0;
#ifdef __GNUC__
  (void) size;
#endif

  for(;;){
    if ((i=netreadline(sd, (char *)buf)) == -1) {
      logmsg(L_ERROR, F_NET, "unable to read line from network", NULL);
      return -1;
    }
    /*
      if (i==0){
      logmsg(L_ERROR, F_NET, "peer closed connection: ", strerror(errno), NULL);
      return -1;
      }
    */
    //fsize+=i;
    tmp=(char *)buf;
    if (!(strcmp((char *)buf, ".\r\n"))){
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
      fwrite(buf, 1, i-2, fd);
      if (ferror(fd)){
        logmsg(L_DEADLY, F_GENERAL, "writing to spool failed: ", strerror(errno), NULL);
      }
#else
      if (write(fd, buf, i-2) == -1){
        logmsg(L_DEADLY, F_GENERAL, "writing to spool failed: ", strerror(errno), NULL);
      }
#endif
      return(fsize);
    } else {
      if (delayrn){
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
        fwrite("\n", 1, 1, fd);
        if (ferror(fd)){
          logmsg(L_DEADLY, F_GENERAL, "writing to spool failed: ", strerror(errno), NULL);
        }
#else
        if (write(fd, "\n", 1) == -1){
          logmsg(L_DEADLY, F_GENERAL, "writing to spool failed: ", strerror(errno), NULL);
        }
#endif
        delayrn=0;
      }
      // we'll delay writing \r\n till we get here the next time in case of \r\n.\r\n as ending
      if (!strcmp((char *)buf, "\r\n")) delayrn=1;
      else {
        tmp=(char *)buf;
        if (!strncmp(tmp+i-1, "\r\n", 2)){
          strncpy(tmp+i-1, "\n", 1);
          i--;
        }
        if (!strncmp((char *)buf, ".", 1)){
          tmp=(char *)buf;
          tmp++;
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
          fwrite(tmp, 1, i, fd);
          if (ferror(fd)){
            logmsg(L_DEADLY, F_GENERAL, "writing to spool failed: ", strerror(errno), NULL);
          }
#else
          if (write(fd, tmp, i) == -1){
            logmsg(L_DEADLY, F_GENERAL, "writing to spool failed: ", strerror(errno), NULL);
          }
#endif
        } else {
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
          fwrite(buf, 1, i+1, fd);
          if (ferror(fd)){
            logmsg(L_DEADLY, F_GENERAL, "writing to spool failed: ", strerror(errno), NULL);
          }
#else
          if (write(fd, buf, i+1) == -1){
            logmsg(L_DEADLY, F_GENERAL, "writing to spool failed: ", strerror(errno), NULL);
          }
#endif
        }
      }
    }
  }
}
