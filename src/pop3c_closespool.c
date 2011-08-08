/**
 * @file pop3c_closespool.c
 * @author Bernd Wachter <bwachter-usenet@lart.info>
 * @date 2011
 */

#ifndef __WIN32__
#include <sys/wait.h>
#endif

#include <ibaard_maildir.h>

#include "aardmail-pop3c.h"

extern char* uniqname;

int pop3c_closespool(FDTYPE fd){
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
  fflush(fd);
  if (pop3c.pipeto) {
    if(pclose(fd)==-1) return -1;
  } else mdclose(NULL, &uniqname, fd);
#else
  if (pop3c.pipeto){
    int status;
    close(fd);
    wait(&status);
    return status;
  } else
    mdclose(NULL, &uniqname, fd);
#endif
  return 0;
}
