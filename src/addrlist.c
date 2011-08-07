/**
 * @file addrlist.c
 * @author Bernd Wachter <bwachter@lart.info>
 * @date 2005-2011
 */

#include <stdlib.h>
#include <string.h>

#ifdef __WIN32__
#include <io.h>
#else
#include <unistd.h>
#endif

#include <ibaard_log.h>
#include <ibaard_fs.h>
#include <ibaard_cat.h>

#include "addrlist.h"

int addrlist_free(addrlist **addrlist_storage){
  addrlist *p, *next_p;
  if (*addrlist_storage == NULL){
    logmsg(L_WARNING, F_ADDRLIST, "trying to empty an empty addrlist", NULL);
    return -1;
  } else {
    p=*addrlist_storage;
    while(1){
      if (p->next==NULL){
        logmsg(L_DEBUG, F_ADDRLIST, "removing ", p->address, NULL);
        free(p);
        return 0;
      } else {
        logmsg(L_DEBUG, F_ADDRLIST, "removing ", p->address, NULL);
        next_p=p->next;
        free(p);
        p=next_p;
      }
    }
  }
}

int addrlist_delete(addrlist **addrlist_storage, char *address){
  addrlist *p, *prev_p;
  logmsg(L_DEBUG, F_ADDRLIST, "removing ", address, " from addrlist structure",  NULL);
  if (*addrlist_storage == NULL){
    logmsg(L_WARNING, F_ADDRLIST, "trying to remove an element from empty addrlist", NULL);
    return -1;
  } else {
    for (p=prev_p=*addrlist_storage; p->next != NULL; prev_p=p, p=p->next){
      if (!strcmp(p->address, address)){ // we found a matching entry
        prev_p->next=p->next;
        free(p);
        return 0;
      }
    }
  }
  return 0; // no key found, but no error either
}

int addrlist_append(addrlist **addrlist_storage, char *address){
  addrlist *p;

  logmsg(L_DEBUG, F_ADDRLIST, "adding ", address, " to addrlist structure",  NULL);
  p=*addrlist_storage;
  if (!strcmp(p->address, "")){
    strcpy(p->address,address);
    p->next=NULL;
  } else {
    while (p->next != NULL) p=p->next;
    if ((p->next=malloc(sizeof(addrlist))) == NULL) {
      logmsg(L_ERROR, F_ADDRLIST, "unable to malloc() memory for new addrlist element", NULL);
      return -1;
    }
    strcpy(p->next->address,address);
    p->next->next=NULL;
  }
  return 0;
}
