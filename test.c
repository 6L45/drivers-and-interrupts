#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>


int main()
{
	int fd = open("/dev/kbdlog", O_RDONLY);
	if (fd < 0)
	{
		perror("hey non gaston\n");
		return 1;
	}

	char *buffer = NULL;
	buffer = malloc(50);

	int i = 2;
	int ret;
	while ((ret =read(fd, buffer, 50)))
	{
		if (ret < 0)
		{
			perror("trop marrant bertrand\n");
			close(fd);
			free(buffer);
			return 1;
		}
		buffer[ret] = 0;
		buffer = realloc(buffer, 50 * i + 1);
	}
	close(fd);
	printf("%s\n", buffer);

	free(buffer);
	return 0;
}
