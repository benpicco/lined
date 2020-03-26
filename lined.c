#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LINE_BUFFER 128

typedef struct {
	int fd_old;
	int fd_new;
	int line;
} editor_t;

typedef int edit_function_t(editor_t*, void*, const char*);

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

static bool insert_line(editor_t* file, const char* text, size_t len) {
	static const char nl = '\n';

	printf("%d\t%s\n", file->line, text);

	file->line++;

	if (file->fd_new == file->fd_old) // hack for append - print only
		return true;
	if (write(file->fd_new, text, len) != len)
		return false;
	if (write(file->fd_new, &nl, 1) != 1)
		return false;

	return true;
}

static int copy_file(editor_t* file, edit_function_t* function, void* condition, const char* text) {

	char buffer[LINE_BUFFER];
	int len = 0;
	while (len >= 0) {

		len = _getline(buffer, sizeof(buffer), file->fd_old);

		int res = function(file, condition, text);

		if (res > 0)
			continue;
		if (res < 0)
			return res;

		if (len >= 0) {
			if (!insert_line(file, buffer, len))
				return -1;
		}
	}

	return 0;
}

static int _delete(editor_t* file, void* line, const char* text) {
	if (file->line != *(int*)line)
		return 0;

	*(int*)line = -1;
	return 1; // skip line
}

static int _insert(editor_t* file, void* line, const char* text) {
	if (file->line != *(int*)line)
		return 0;
	if (!insert_line(file, text, strlen(text)))
		return -1;
	return 0;
}

static int _replace(editor_t* file, void* line, const char* text) {
	if (file->line != *(int*)line)
		return 0;
	if (!insert_line(file, text, strlen(text)))
		return -1;

	*(int*)line = -1;
	return 1; // skip line
}

static int _substitute(editor_t* file, void* searchstring, const char* text) {
	printf("s[%s]", searchstring);

	if (strncmp(text, searchstring, strlen(searchstring)))
		return 0;
	printf("match!\n");
	if (!insert_line(file, text, strlen(text)))
		return -1;
	return 1; // skip line
}

static int _no_op(editor_t* file, void* condition, const char* text) {
	return 0;
}

static int file_append(const char* file, const char* text) {

	editor_t file_state;
	file_state.fd_old = open(file, O_RDWR | O_CREAT, 0666);
	file_state.fd_new = file_state.fd_old;
	file_state.line = 0;

	if (file_state.fd_old < 0)
		return -1;

	copy_file(&file_state, _no_op, NULL, NULL);

	if (text != NULL)
		insert_line(&file_state, text, strlen(text));

	close(file_state.fd_old);

	return 0;
}

static int file_edit(const char* file, edit_function_t* function, void* condition, const char* text) {

	editor_t file_state;
	file_state.fd_old = open(file, O_RDONLY);
	if (file_state.fd_old < 0) {
		printf("can't open %s\n", file);
		return -1;
	}

	char file_tmp[32];
	strcpy(file_tmp, file);
	strcat(file_tmp, ".t");

	file_state.fd_new = open(file_tmp, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (file_state.fd_new < 0) {
		printf("can't open tmp file %s\n", file_tmp);

		close(file_state.fd_old);
		return -1;
	}

	file_state.line = 0;

	int ret = copy_file(&file_state, function, condition, text);

	close(file_state.fd_old);
	close(file_state.fd_new);

	if (ret < 0) {
		printf("error writing file\n");
		return ret;
	}

	rename(file_tmp, file);

	return 0;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("usage: %s [file] [cmd] [text]\n", argv[0]);
		return -1;
	}

	char* file = argv[1];
	char* cmd = argc > 2 ? argv[2] : "p";
	char* text = argc > 3 ? concat_args(argc - 3, &argv[3]) : NULL;
	int line = atoi(cmd + 1);
	char mode = cmd[0];

	if (text == NULL && (mode == 'i' || mode == 'r' || mode == 'a'))
		return -1;

	switch (mode) {
	case 'a':  // append
		return file_append(file, text);
	case 'p':  // print
		return file_append(file, NULL);
	case 'd': // delete
		return file_edit(file, _delete, &line, NULL);
	case 'r': // replace
		return file_edit(file, _replace, &line, text);
	case 'i': // insert
		return file_edit(file, _insert, &line, text);
	case 's': // substitute
		++cmd;
		return file_edit(file, _substitute, cmd, text);
	}

	return 0;
}
