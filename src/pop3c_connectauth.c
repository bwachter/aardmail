/**
 * @file pop3c_connectauth.c
 * @author Bernd Wachter <bwachter-usenet@lart.info>
 * @date 2011
 */

#include <ibaard_network.h>
#include <ibaard_cat.h>

#include "aardmail-pop3c.h"

int pop3c_connectauth(authinfo *auth){
  int i, sd;
  char buf[1024];
  char *tmpstring=NULL;

  // ugly hack to get rid of the `label defined but not used' warning when building without SSL
  goto connect;

  connect:
  if ((sd=netconnect2(auth->machine, auth->port, pop3c.bindname)) == -1)
    return -1;
  if ((i=netreadline(sd, buf)) == -1)
    return -1;
  if (i==0){
    logmsg(L_ERROR, F_NET, "peer closed connection: ", strerror(errno), NULL);
    return -1;
  }

#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
  // check if we have to use starttls. abort if USETLS is already set
  if ((am_sslconf & AM_SSL_STARTTLS) && !(am_sslconf & AM_SSL_USETLS)){
    if ((pop3c_oksendline(sd, "stls\r\n")) == -1) {
      if (am_sslconf & AM_SSL_ALLOWPLAIN){
        pop3c_quitclose(sd);
        am_sslconf = 0;
        logmsg(L_WARNING, F_NET, "Reconnecting using plaintext (you allowed this!)", NULL);
        goto connect;
      } else return -1;
    }

    am_sslconf ^= AM_SSL_USETLS;
    if ((i=netsslstart(sd))) {
      logmsg(L_ERROR, F_SSL, "unable to open tls-connection using starttls", NULL);
      if (am_sslconf & AM_SSL_ALLOWPLAIN){
        pop3c_quitclose(sd);
        am_sslconf = 0;
        logmsg(L_WARNING, F_NET, "Reconnecting using plaintext (you allowed this!)", NULL);
        goto connect;
      } else return -1;
    }
  }
#endif

  if (!cat(&tmpstring, "user ", auth->login, "\r\n", NULL))
    if ((pop3c_oksendline(sd, tmpstring)) == -1)
      return -1;

#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
  // we never send a password if we got a ssl key
  if (strcmp(am_sslkey, "")) return sd;
#endif

  if (!cat(&tmpstring, "pass ", auth->password, "\r\n", NULL)){
    if ((pop3c_oksendline(sd, tmpstring)) == -1)
      return -1;
  }

  return sd;
}
