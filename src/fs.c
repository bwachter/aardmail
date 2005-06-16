#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#ifdef __WIN32__
#include <stdio.h>
#include <io.h>
#else
#include <unistd.h>
#endif


int tf(char *name){
	int fd;
	if ((fd=open(name, O_RDONLY))==-1) return errno;
	close(fd);
	return 0;
}

int td(char *name){
	struct stat dirstat;
	if (!stat(name, &dirstat))
		if (S_ISDIR(dirstat.st_mode)) return 0;
	return -1;
}

int openreadclose(char *fn, char **buf, unsigned long *len) {
  int fd=open(fn,O_RDONLY);
  if (fd<0) return -1;
  if (!*buf) {
    *len=lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);
    *buf=(char*)malloc(*len+1);
    if (!*buf) {
      close(fd);
      return -1;
    }
  }
  *len=read(fd,*buf,*len);
  if (*len != (unsigned long)-1)
    (*buf)[*len]=0;
  return close(fd);
}

