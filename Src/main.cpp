#include <unistd.h>
#include <stdio.h>

#include "log.h"
#include "httpserver.h"

int main(int argc, char **argv)
{
	Http::HttpServer httpserver("0.0.0.0", 8080);
	if(httpserver.start(1024) < 0){
		errsys("server create failed\n");
		return -1;
	}

	printf("************************************\n");
	printf("* List of classes of commands:\n\n");
	printf("* CTRL^D -- exit\n");
	printf("************************************\n");
	
	for(;;){
		char buff[1024];
		char *cmd = fgets(buff, sizeof(buff), stdin);
		if(cmd == NULL)
			break;
	}

	trace("terminate ...\n");
	return 0;
}
