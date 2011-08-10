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
#else
      write(fd, buf, i-2);
#endif
      return(fsize);
    } else {
      if (delayrn){
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
        fwrite("\n", 1, 1, fd);
#else
        write(fd, "\n", 1);
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
        } else
          fwrite(buf, 1, i+1, fd);
#else
        write(fd, tmp, i);
      } else
          write(fd, buf, i+1);
#endif
    }
  }
}
}
