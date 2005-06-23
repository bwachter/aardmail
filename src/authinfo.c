#include <stdlib.h>
#include <string.h>

#ifdef __WIN32__
#include <io.h>
#else
#include <unistd.h>
#endif

#include "authinfo.h"
#include "aardlog.h"
#include "cat.h"
#include "fs.h"

static int authinfo_append(authinfo *authinfo_add);

static authinfo *authinfo_storage;
static int i=0;

static authinfo_key authinfo_keys[] = {
	{"machine", 1},
	{"login", 1},
	{"password", 1},
	{"default", 0},
	{"force", 1},
	{"port", 1},
	{NULL, 0}
};

int authinfo_init(){
	int err, nextrecord=0;
	char *home, *authinfo_content=0, *tmp, *tmp1;
	char *path=NULL;
	unsigned long authinfo_len, l;
	authinfo authinfo_tmp;
	authinfo_key *key;

	if (((home=getenv("HOME"))==NULL) && ((home=getenv("USERPROFILE"))==NULL)){
		logmsg(L_WARNING, F_AUTHINFO, "unable to find the home directory", NULL);
		return -1;
	}

	if (cat(&path, home, "/.authinfo", NULL)) return -1;
	if ((err=tf(path))==-1){
		logmsg(L_INFO, F_AUTHINFO, "no authinfo found: ", strerror(err), NULL);
		free(path);
		return -1;
	}
	if ((err=openreadclose(path, &authinfo_content, &authinfo_len))){
		logmsg(L_WARNING, F_AUTHINFO, "unable to read authinfo: ", strerror(err), NULL);
		free(path);
		return -1;
	}

	logmsg(L_INFO, F_AUTHINFO, "using authinfo file in ", path, NULL);
	free(path);

	tmp=authinfo_content;
	l=0;
	memset(&authinfo_tmp, 0, sizeof(authinfo));
	while (l<authinfo_len){
		for (key=authinfo_keys; key->name; key++){
			if (!strncmp(authinfo_content+l, key->name, strlen(key->name))){
				l+=strlen(key->name)+1;
				if (key->hasargs){
					for (tmp=authinfo_content+l,tmp1=tmp;*tmp!=' ' && *tmp!='\n';*tmp++,l++);
					if (*tmp=='\n') nextrecord=1;
					*tmp=0;
					if (!strcmp(key->name, "machine")) strcpy(authinfo_tmp.machine, tmp1);
					if (!strcmp(key->name, "port")) strcpy(authinfo_tmp.port, tmp1);
					if (!strcmp(key->name, "login")) strcpy(authinfo_tmp.login, tmp1);
					if (!strcmp(key->name, "password")) strcpy(authinfo_tmp.password, tmp1);
					if (!strcmp(key->name, "force")) 
						if (!strcmp(tmp1, "yes")) authinfo_tmp.force=1;
				} else {
					if (!strcmp(key->name, "default")) authinfo_tmp.defaultauth=1;
					l--;
				}
				break;
			}
		}
		if ((!strncmp(authinfo_content+l, "\n", 1)) || nextrecord){
			authinfo_append(&authinfo_tmp);
			memset(&authinfo_tmp, 0, sizeof(authinfo));
			nextrecord=0;
		}
		l++;
	}
	return 0;
}

int authinfo_lookup(authinfo *authinfo_keys){
	authinfo *p, defaultauth;
	memset(&defaultauth, 0, sizeof(authinfo));
	for (p=authinfo_storage; p!=NULL; p=p->next){
		if (p->defaultauth){
			memcpy(&defaultauth, p, sizeof(authinfo));
			continue;
		}
		if (strcmp(authinfo_keys->machine, ""))
			if (strcmp(authinfo_keys->machine, p->machine))
				continue;
		if (strcmp(authinfo_keys->port, ""))
			if (strcmp(authinfo_keys->port, p->port))
				continue;
		if (strcmp(authinfo_keys->login, ""))
			if (strcmp(authinfo_keys->login, p->login))
				continue;
		if (strcmp(authinfo_keys->password, ""))
			if (strcmp(authinfo_keys->password, p->password))
				continue;
		// if we got that far we either found a valid key, or
		// all fields are set to NULL
		memcpy(authinfo_keys, p, sizeof(authinfo));
		authinfo_keys->next=NULL;
		return 0;
	}
	return -1; // we did not find a key
}

static int authinfo_append(authinfo *authinfo_add){
	authinfo *p;

	logmsg(L_DEBUG, F_AUTHINFO, "adding ", authinfo_add->login, "@", authinfo_add->machine, ":",
				 authinfo_add->port, " to authinfo structure",  NULL);
	if (authinfo_storage == NULL){
		if ((authinfo_storage = malloc(sizeof(authinfo))) == NULL) {
			logmsg(L_ERROR, F_AUTHINFO, "unable to malloc() memory for first authinfo element", NULL);
			return -1;
		}
		i++;
		memcpy(authinfo_storage, authinfo_add, sizeof(authinfo));
		authinfo_storage->next=NULL;
	} else {
		p=authinfo_storage;
		while (p->next != NULL) p=p->next;

		if ((p->next=malloc(sizeof(authinfo))) == NULL) {
			logmsg(L_ERROR, F_AUTHINFO, "unable to malloc() memory for new authinfo element", NULL);
			return -1;
		}
		memcpy(p->next, authinfo_add, sizeof(authinfo));
		i++;
	}
	return 0;
}
