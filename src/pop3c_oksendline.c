/**
 * @file pop3c_oksendline.c
 * @author Bernd Wachter <bwachter-usenet@lart.info>
 * @date 2011
 */

#include <ibaard_network.h>
#include "aardmail-pop3c.h"

int pop3c_oksendline(int sd, char *msg){
  char buf[MAXNETBUF];
  int i;

  if ((i=netwriteline(sd, msg)) == -1){
    logmsg(L_ERROR, F_NET, "unable to write line to network: ", strerror(errno), NULL);
    return -1;
  }
  if ((i=netreadline(sd, buf)) == -1){
    logmsg(L_ERROR, F_NET, "unable to read line from network: ", strerror(errno), NULL);
    return -1;
  }
  if (i==0){
    logmsg(L_ERROR, F_NET, "peer closed connection: ", strerror(errno), NULL);
    return -1;
  }
  if (!strncmp(buf, "+OK", 3))
    return 0;
  logmsg(L_ERROR, F_NET, "bad response: '", buf, "' after '", msg, "' from me", NULL);
  return -1;
}
