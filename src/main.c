/*
 * Copyright (C) 2016-2021 Davidson Francis <davidsondfgl@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <ws.h>

/* Global variables */
FILE *f_printers;
FILE *f_printers_tmp;
enum PRINTER_STATE {OK = 0, BUSY = 1, NOK = 2};
struct PRINTER {
	char name[16];
	char ip[16];
	enum PRINTER_STATE state;
};

struct PRINTER split_line(char *line, long unsigned int size) {
	long unsigned int i = 0;
	long unsigned int j = 0;
	struct PRINTER printer;
	long unsigned int printer_strlen = sizeof(printer.ip);
	char name[printer_strlen];
	char ip[printer_strlen];
	enum PRINTER_STATE state = NOK;
	char buffer[printer_strlen];
	uint8_t stage = 0;
	char c; 

	for (i = 0; i < size; i++) { // size is the size of the array, since one char is one byte, it is also the length of the array
		c = *(line + i);
		if (c == '&' || j >= printer_strlen) {
			if (j > printer_strlen)
				j--;
			buffer[j] = '\0';
			if (stage == 0) {
				memcpy(name, buffer, printer_strlen);
			} else if (stage == 1) {
				memcpy(ip, buffer, printer_strlen);
			} else
				break;
			stage++;
			j = 0;
		} else { // we'll jump over the '&' sign so therefor not add to buffer when c is '&'
			buffer[j] = c;
			j++;
		}
	}
	/* Check if we have filled both the name and the ip */
	if (stage == 2) {
		/* Then add the state as well */
		if (strncmp("OK", buffer, 2) == 0)
			state = OK;
		else if (strncmp("BUSY", buffer, 4) == 0)
			state = BUSY;
		else
			state = NOK;
	}
	/* Now fill the struct with above data */
	// memcpy(printer.name, name, 15);
	// memcpy(printer.ip, ip, 15);
	//printf("%lu \n", sizeof(printer.name));
	memcpy(printer.name, name, printer_strlen);
	memcpy(printer.ip, ip, printer_strlen);
	printer.state = state;
	return printer;
}
void clean_files() {
	const unsigned MAX_LENGTH = 256;
	char line[MAX_LENGTH];
	while(fgets(line, MAX_LENGTH, f_printers)) {
		struct PRINTER printer = split_line(&line[0], MAX_LENGTH);
		printf("%s %s %u \n", printer.name, printer.ip, printer.state);
	}
}
void add_printer(const char *name, const char *ip) {
	const unsigned MAX_LENGTH = 256;
	char line [MAX_LENGTH];

	while(fgets(line, MAX_LENGTH, f_printers)) {
		printf("%s", line);
	}
}


/**
 * @brief Called when a client connects to the server.
 *
 * @param fd File Descriptor belonging to the client. The @p fd parameter
 * is used in order to send messages and retrieve informations
 * about the client.
 */
void onopen(int fd)
{
	char *cli;
	cli = ws_getaddress(fd);
#ifndef DISABLE_VERBOSE
	printf("Connection opened, client: %d | addr: %s\n", fd, cli);
#endif
	free(cli);
}

/**
 * @brief Called when a client disconnects to the server.
 *
 * @param fd File Descriptor belonging to the client. The @p fd parameter
 * is used in order to send messages and retrieve informations
 * about the client.
 */
void onclose(int fd)
{
	char *cli;
	cli = ws_getaddress(fd);
#ifndef DISABLE_VERBOSE
	printf("Connection closed, client: %d | addr: %s\n", fd, cli);
#endif
	free(cli);
}

/**
 * @brief Called when a client connects to the server.
 *
 * @param fd File Descriptor belonging to the client. The
 * @p fd parameter is used in order to send messages and
 * retrieve informations about the client.
 *
 * @param msg Received message, this message can be a text
 * or binary message.
 *
 * @param size Message size (in bytes).
 *
 * @param type Message type.
 */
void onmessage(int fd, const unsigned char *msg, uint64_t size, int type)
{
	char *cli;
	cli = ws_getaddress(fd);
#ifndef DISABLE_VERBOSE
	printf("I receive a message: %s (size: %" PRId64 ", type: %d), from: %s/%d\n",
		msg, size, type, cli, fd);
#endif
	free(cli);

	/**
	 * Mimicks the same frame type received and re-send it again
	 *
	 * Please note that we could just use a ws_sendframe_txt()
	 * or ws_sendframe_bin() here, but we're just being safe
	 * and re-sending the very same frame type and content
	 * again.
	 */
	ws_sendframe(fd, (char *)msg, size, true, type);
	add_printer("hey", "localhost");
}

bool get_cwd(char **cwd, size_t *cwd_size) {
	long path_max;
	char *ptr;

	path_max = pathconf(".", _PC_PATH_MAX);
	if (path_max == -1)
		*cwd_size = 1024;
	else if (path_max > 10240)
		*cwd_size = 10240;
	else
		*cwd_size = path_max;
	


	for (*cwd = ptr = NULL; ptr == NULL; *cwd_size *= 2)
	{
		if ((*cwd = realloc(*cwd, *cwd_size)) == NULL)
		{
			perror("error realloc");
		}


		ptr = getcwd(*cwd, *cwd_size);
		if (ptr == NULL && errno != ERANGE)
		{
			perror("ERANGE");
		}
	}

	if (ptr != NULL) {
		return true;
	}
	return false;
}

bool open_printer_file(const char *file_name, char *cwd, size_t cwd_size, FILE **ptr) {
	/* Create path for the database */
	char *file_path = malloc(cwd_size + sizeof(file_name));
	strcpy(file_path, cwd);
	strcpy(file_path + strlen(cwd), "/");
	strcpy(file_path + strlen(cwd) + 1, file_name);

	printf("Database path is: %s\n", file_path);
	
	/* Actually open the file now */
	*ptr = fopen(file_path, "r+");
	
	free(file_path);
	if (*ptr == NULL) {
		return false;
	}
	return true;
}
/**
 * @brief Main routine.
 *
 * @note After invoking @ref ws_socket, this routine never returns,
 * unless if invoked from a different thread.
 */
int main(void)
{
	struct ws_events evs;
	char* cwd = "";
	size_t cwd_size;
	
	/* Get the current working directory */
	if (!get_cwd(&cwd, &cwd_size)) {
		printf("Could not open current working directory");
		return -1;
	}
	/* Open the working files */
	printf("Current working dir: %s\nOpening printers file...\n", cwd);
	if (open_printer_file("printers.db", cwd, cwd_size, &f_printers) &&
	    open_printer_file("printers.db.tmp", cwd, cwd_size, &f_printers_tmp)) {
		printf("File successfully opened\n");
	} else {
		printf("Could not open file for read or write\n");
		return -1;
	}
	/* Clean the files */
	clean_files();

	evs.onopen    = &onopen;
	evs.onclose   = &onclose;
	evs.onmessage = &onmessage;
	ws_socket(&evs, 8080, 0); /* Never returns. */

	/*
	 * If you want to execute code past ws_socket, invoke it like:
	 *   ws_socket(&evs, 8080, 1)
	 */

	fclose(f_printers);
	return (0);
}
