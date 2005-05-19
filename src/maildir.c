#ifdef __WIN32__
#include <windows.h>
#include <winbase.h>
#include <io.h>
#else 
#include <unistd.h>
#endif

#include "cat.h"

int maildirgname(char **uniqname){
#ifdef _POSIX_SOURCE

#endif
}

// opens a file in maildir/ 
int maildiropen(char *maildir, char **uniqname){
	char *tmpname;
	int fd;

	cat(&tmpname, maildir, 
}

int maildirclose(char *maildir, char **uniqname, int fd){
	char *tmpname;
	int status;

	if (!close(fd)){
#ifdef __WIN32__
		status=MoveFile("", "");
#else
		status=link("uniqname", "");
		unlink("uniqname");
#endif
}

