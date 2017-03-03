#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LINE_BUFFER 128

int _getline(char* buffer, size_t size, int fd) {

    int in = -1;
    while (read(fd, buffer, 1) > 0 && --size) {
        ++in;

        if (*buffer == '\n') {
            *buffer = 0;
            break;
        }

        ++buffer;
    }

    return in;
}

static char* concat_args(int argc, char** argv) {
    size_t len = 0;
    char *_all_args, *all_args;

    for(int i=0; i<argc; i++) {
        len += strlen(argv[i]);
    }

    _all_args = all_args = (char *)malloc(len+argc-1);

    for(int i=0; i<argc; i++) {
        memcpy(_all_args, argv[i], strlen(argv[i]));
        _all_args += strlen(argv[i])+1;
        *(_all_args-1) = ' ';
    }

    *(_all_args-1) = 0;

	return all_args;
}

static inline void insert_line(int fd, const char* text, size_t len, int line) {
    static const char nl = '\n';

    write(fd, text, len);
    write(fd, &nl, 1);

    printf("%d\t%s\n", line, text);
}

static void copy_file(int fd_old, int fd_new, char mode, int line, const char* text) {
    int current_line = -1;

    char buffer[LINE_BUFFER];
    int len = 0;
    while (len >= 0) {

        len = _getline(buffer, sizeof(buffer), fd_old);
        ++current_line;

        // delete - skip line
        if (mode == 'd' && current_line == line) {
            continue;
        }

        // insert line
        if (mode == 'i' && current_line == line)  {
            insert_line(fd_new, text, strlen(text), current_line++);
        }

        // replace line
        if (mode == 'r' && current_line == line)  {
            insert_line(fd_new, text, strlen(text), current_line);

            continue;
        }

        if (len >= 0)
            insert_line(fd_new, buffer, len, current_line);
    }
}

static int file_append(const char* file, const char* text) {

    int fd = open(file, O_RDWR | O_CREAT, 0666);

    if (fd < 0)
        return -1;

    if (lseek(fd, 0, SEEK_END) < 0) {
        printf("seek error\n");
        return -1;
    }

    insert_line(fd, text, strlen(text), 0);

    close(fd);

    return 0;
}

static int file_edit(const char* file, char mode, int line, const char* text) {

    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        printf("can't open %s\n", file);
        return -1;
    }

    char file_tmp[32];
    strcpy(file_tmp, file);
    strcat(file_tmp, ".t");

    int fd_new = open(file_tmp, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_new < 0) {
        printf("can't open tmp file %s\n", file_tmp);

        close(fd);
        return -1;
    }

    copy_file(fd, fd_new, mode, line, text);

    close(fd);
    close(fd_new);

    rename(file_tmp, file);

    return 0;
}

int main(int argc, char** argv) {
	if (argc < 3) {
		printf("usage: %s [file] [cmd] [text]\n", argv[0]);
		return -1;
	}

	char* file = argv[1];
	char* cmd = argv[2];
	char* text = argc > 3 ? concat_args(argc - 3, &argv[3]) : NULL;
	int line = atoi(cmd + 1);
    char mode = cmd[0];

    if (text == NULL && (mode == 'i' || mode == 'r' || mode == 'a'))
        return -1;

    if (mode == 'a')
        return file_append(file, text);
    else
        return file_edit(file, mode, line, text);

	return 0;
}
