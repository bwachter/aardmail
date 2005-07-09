#include <stdlib.h>
#include <string.h>

#ifdef __WIN32__
#include <io.h>
#else
#include <unistd.h>
#endif

#include "addrlist.h"
#include "aardlog.h"
#include "cat.h"
#include "fs.h"

static int i=0;

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
	if (*addrlist_storage == NULL){
		if ((*addrlist_storage = malloc(sizeof(addrlist))) == NULL) {
			logmsg(L_ERROR, F_ADDRLIST, "unable to malloc() memory for first addrlist element", NULL);
			return -1;
		}
		i++;
		strcpy((*addrlist_storage)->address,address);
		(*addrlist_storage)->next=NULL;
	} else {
		p=*addrlist_storage;
		while (p->next != NULL) p=p->next;
		if ((p->next=malloc(sizeof(addrlist))) == NULL) {
			logmsg(L_ERROR, F_ADDRLIST, "unable to malloc() memory for new addrlist element", NULL);
			return -1;
		}
		strcpy(p->next->address,address);
		p->next->next=NULL;
		i++;
	}
	return 0;
}
