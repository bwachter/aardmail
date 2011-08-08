/**
 * @file pop3c_quitclose.c
 * @author Bernd Wachter <bwachter-usenet@lart.info>
 * @date 2011
 */

#include "aardmail-pop3c.h"

int pop3c_quitclose(int sd){
  if ((pop3c_oksendline(sd, "quit\r\n")) == -1)
    return -1;
  return (close(sd));
}
