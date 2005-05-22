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

static authinfo *authinfo_start, *authinfo_end;
static int i=0;

static authinfo_key authinfo_keys[] = {
	{"machine", 1},
	{"login", 1},
	{"password", 1},
	{"default", 0},
	{"force", 0},
	{"port", 1},
	{NULL, 0}
};

int authinfo_init(){
	int err, iskey=0;
	char *home, *authinfo_content=0, *tmp;
	char *path=NULL;
	unsigned long authinfo_len, l;
	authinfo authinfo_tmp;
	authinfo_key *key;

	if ((home=getenv("HOME"))==NULL){
		logmsg(L_WARNING, F_GENERAL, "unable to find the home directory", NULL);
		return -1;
	}

	if (cat(&path, home, "/.authinfo", NULL)) return -1;
	if ((err=tf(path))==-1){
		logmsg(L_INFO, F_GENERAL, "no authinfo found: ", strerror(err), NULL);
		free(path);
		return -1;
	}
	if ((err=openreadclose(path, &authinfo_content, &authinfo_len))){
		logmsg(L_WARNING, F_GENERAL, "unable to read authinfo: ", strerror(err), NULL);
		free(path);
		return -1;
	}

	logmsg(L_INFO, F_GENERAL, "using authinfo file in ", path, NULL);
	free(path);

	tmp=authinfo_content;
	l=0;
	while (l<authinfo_len){
		iskey=0;
		for (key=authinfo_keys; key->name && !iskey; key++){
			if (!strncmp(authinfo_content+l, key->name, strlen(key->name))){
				logmsg(L_INFO, F_GENERAL, "found a key: ", key->name, NULL);
				l+=strlen(key->name)+1;
				if (key->hasargs){
					//authinfo_tmp.password="foobar";
					for (tmp=authinfo_content+l;*tmp!=' ' && *tmp!='\n';*tmp++,l++) 
						write(1, tmp, 1);
				} else l--;
				iskey=1;
				break;
			}
		}
		if (!strncmp(authinfo_content+l, "\n", 1)){
			logmsg(L_INFO, F_GENERAL, "record completed", NULL);
			authinfo_append(&authinfo_tmp);
			memset(&authinfo_tmp, 0, sizeof(authinfo));
		}
		l++;
	}
	return 0;
}

static int authinfo_append(authinfo *authinfo_add){
	authinfo *p, *p1;

	if ((authinfo_end = NULL)){
		if ((authinfo_end = malloc(sizeof(authinfo))) == NULL) {
			logmsg(L_ERROR, F_GENERAL, "unable to malloc() memory for authinfo structures", NULL);
			return -1;
		}
	}

	// is there already an element?
	if (authinfo_start == NULL){
		if ((authinfo_start = malloc(sizeof(authinfo))) == NULL) {
			logmsg(L_ERROR, F_GENERAL, "unable to malloc() memory for first authinfo element", NULL);
			return -1;
		}
		i++;
		// set up elements
		authinfo_start->next=NULL;
		authinfo_end=authinfo_start;
		authinfo_end->prev=NULL;
	} else {
		p=authinfo_start;
		while (p->next != NULL) p=p->next;

		if ((p->next=malloc(sizeof(authinfo))) == NULL) {
			logmsg(L_ERROR, F_GENERAL, "unable to malloc() memory for new authinfo element", NULL);
			return -1;
		}

		p1=p;
		p=p->next; //pointer to newly allocated memory
		i++;
		// set up elements

		//set up pointers
		authinfo_end=p;
		p->prev=p1;
		p1->next=p;
	}
	return 0;
}
