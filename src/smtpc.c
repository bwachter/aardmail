
#include "aardmail.h"
#include "aardlog.h"
#include "authinfo.h"
#include "network.h"
#include "cat.h"
#include "maildir.h"

int main(int argc, char **argv){
	(void) argc;
	(void) argv;
	maildir_init(NULL, ".spool", 0);
	return 0;
}
