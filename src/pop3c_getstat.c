/**
 * @file pop3c_getstat.c
 * @author Bernd Wachter <bwachter-usenet@lart.info>
 * @date 2011
 */

#include <errno.h>

#include <ibaard_network.h>

#include "aardmail-pop3c.h"

int pop3c_getstat(int sd){
  char *tmp;
  char buf[MAXNETBUF];
  int i;

  if ((i=netwriteline(sd,"stat\r\n")) == -1){
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
  if (strncmp(buf, "+OK", 3)){
    logmsg(L_ERROR, F_NET, "bad response: '", buf, "'after 'stat' from me", NULL);
    return -1;
  }

  if (pop3c.msgcount_str != NULL)
    free(pop3c.msgcount_str);

  for (tmp=buf+4,i=0;*tmp!=' ';tmp++,i++){}
  if((pop3c.msgcount_str=malloc((i+1)*sizeof(char))) == NULL){
    logmsg(L_ERROR, F_GENERAL, "malloc() failed at pop3c_getstat()", NULL);
    return -1;
  }
  strncpy(pop3c.msgcount_str, buf+4, i);
  pop3c.msgcount_str[i]='\0';
  logmsg(L_INFO, F_NET, "There are ", pop3c.msgcount_str, " messages on server", NULL);
  pop3c.msgcount=atoi(pop3c.msgcount_str);
  return pop3c.msgcount;
}
