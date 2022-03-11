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

#define DISABLE_VERBOSE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <ws.h>

/* Global variables */
const char *db_name = "printers.db";
FILE *f_printers;
char* cwd = "";
size_t cwd_size;
enum PRINTER_STATE {OK = 0, BUSY = 1, NOK = 2};
struct PRINTER {
	char name[16];
	char ip[16];
	enum PRINTER_STATE state;
};

bool open_printer_file(const char *file_name, char *cwd, size_t cwd_size, FILE **ptr) {
	/* Create path for the database */
	char *file_path = malloc(cwd_size + sizeof(file_name));
	strcpy(file_path, cwd);
	strcpy(file_path + strlen(cwd), "/");
	strcpy(file_path + strlen(cwd) + 1, file_name);
	#ifndef DISABLE_VERBOSE
		printf("Database path is: %s\n", file_path);
	#endif
	
	/* Actually open the file now */
	*ptr = fopen(file_path, "r+");
	
	free(file_path);
	if (*ptr == NULL) {
		return false;
	}
	return true;
}
bool reopen_db(FILE **f) {
	if (f_printers != NULL)
		fclose(*f);
	if (open_printer_file(db_name, cwd, cwd_size, f)) {
		#ifndef DISABLE_VERBOSE
			printf("File successfully re-opened\n");
		#endif
		return true;
	} else {
		#ifndef DISABLE_VERBOSE
			printf("Could not open file for read or write\n");
		#endif
		return false;
	}
}
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
			if (j > printer_strlen) // since we add one to j after pushing to the buffer, we want to decrease j with one to prevent stack smashing
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
	memcpy(printer.name, name, printer_strlen);
	memcpy(printer.ip, ip, printer_strlen);
	printer.state = state;
	return printer;
}
bool add_printer(char *printer_txt_old, size_t size) {
	size++; /* we add one since we need to add null char */
	char *printer_txt = calloc(size, sizeof(char));
	memcpy(printer_txt, printer_txt_old, size - 1); /* Old buffer doesn't have room for the null character */
	*(printer_txt + size - 1) = '\0';
	#ifndef DISABLE_VERBOSE
		printf("Adding %s \n", printer_txt);
	#endif
	reopen_db(&f_printers);
	/* First check so it doesn't already exist */
	struct PRINTER line_printer;
	struct PRINTER new_printer = split_line(printer_txt, size);
	const unsigned MAX_LENGTH = 256;
	char line[MAX_LENGTH];
	int i = 0;
	fseek(f_printers, 0L, SEEK_SET);
	while(fgets(line, MAX_LENGTH, f_printers)) {
		i++;
		line_printer = split_line(line, MAX_LENGTH);
		if (strncmp(new_printer.name, line_printer.name, size) == 0) {
			#ifndef DISABLE_VERBOSE
				printf("\nPrinter already exists\n");
			#endif
			free(printer_txt);
			return false;
		}
	}

	fseek(f_printers, 0L, SEEK_END);
	fprintf(f_printers, printer_txt);
	fprintf(f_printers, "\n");
	fflush(f_printers);
	free(printer_txt);
	return true;
}
void get_printers(char **response, size_t *response_size) {
	long unsigned int i = 0;
	char c;
	reopen_db(&f_printers);
	fseek(f_printers, 0L, SEEK_END);
	*response_size = ftell(f_printers);
	fseek(f_printers, 0L, SEEK_SET);

	*response = calloc(*response_size + 1, sizeof(char));
	while(1) {
		c = fgetc(f_printers);
		if (c == EOF)
			break;
		*(*response + i) = c;
		i++;

		printf("%c", c);
	}
	*(response + *response_size) = '\0';
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void onmessage(int fd, const unsigned char *msg, uint64_t size, int type)
{
	char *cli;
	char *response;
	size_t response_size = 0;
	cli = ws_getaddress(fd);
#ifndef DISABLE_VERBOSE
	printf("I receive a message: %s (size: %" PRId64 ", type: %d), from: %s/%d\n",
		msg, size, type, cli, fd);
#endif
	free(cli);

	if (size == 0) {
		ws_sendframe_txt(fd, "PING", false);
		return;
	}

	if (strncmp((char*)msg, "add#", 4) == 0) {
		size_t p_txtsize = size - 4; /* size minus "add#" */
		char *printer_txt = calloc(p_txtsize, sizeof(char)); 
		memcpy(printer_txt, msg + 4, p_txtsize);
		if (add_printer(printer_txt, p_txtsize))
			ws_sendframe_txt(fd, "OK\n", false);
		else
			ws_sendframe_txt(fd, "NOK\n", false);
		free(printer_txt);
	} else if (strncmp((char*)msg, "get#", 4) == 0) {
		get_printers(&response, &response_size);
		ws_sendframe_txt(fd, response, false);
		free(response);
	} else {
		ws_sendframe_txt(fd, "Invalid command\n", false);
		return;
	}

	/**
	 * Mimicks the same frame type received and re-send it again
	 *
	 * Please note that we could just use a ws_sendframe_txt()
	 * or ws_sendframe_bin() here, but we're just being safe
	 * and re-sending the very same frame type and content
	 * again.
	 */
	//ws_sendframe(fd, (char *)msg, size, true, type);
}
#pragma GCC diagnostic pop

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

/**
 * @brief Main routine.
 *
 * @note After invoking @ref ws_socket, this routine never returns,
 * unless if invoked from a different thread.
 */
int main(void)
{
	struct ws_events evs;
	
	/* Get the current working directory */
	if (!get_cwd(&cwd, &cwd_size)) {
		printf("Could not open current working directory");
		return -1;
	}
	/* Open the working files */
	printf("Current working dir: %s\nOpening printers file...\n", cwd);
	if (open_printer_file(db_name, cwd, cwd_size, &f_printers)) {
		printf("File successfully opened\n");
	} else {
		printf("Could not open file for read or write\n");
		return -1;
	}

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
