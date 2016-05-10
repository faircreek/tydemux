/*
 * Copyright (C) 2003, 2015  Olof <jinxolina@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "common.h"
#include "global.h"

#ifdef _WIN32
#include "../libs/winsock_gets.c"
#endif

fsid_index_t * new_fsid_index() {

	fsid_index_t * fsid_index;

	fsid_index = (fsid_index_t *)malloc(sizeof(fsid_index_t));
	memset(fsid_index, 0,sizeof(fsid_index_t));

	fsid_index->fsid = -1;
	fsid_index->tystream=NULL;
	fsid_index->state = 0;
	fsid_index->year=NULL;
	fsid_index->air=NULL;
	fsid_index->day=NULL;
	fsid_index->date=NULL;
	fsid_index->rectime=NULL;
	fsid_index->duration=NULL;
	fsid_index->title=NULL;
	fsid_index->episode=NULL;
	fsid_index->episodenr=NULL;
	fsid_index->description=NULL;
	fsid_index->actors=NULL;
	fsid_index->gstar=NULL;
	fsid_index->host=NULL;
	fsid_index->director=NULL;
	fsid_index->eprod=NULL;
	fsid_index->prod=NULL;
	fsid_index->writer=NULL;
	fsid_index->index_avalible = 0;

	fsid_index->next = NULL;
	fsid_index->previous = NULL;

	return(fsid_index);
}

void free_fsid_index(fsid_index_t * fsid_index) {


	if(fsid_index->tystream) {
		free(fsid_index->tystream);
	}

	if(fsid_index->year) {
		free(fsid_index->year);
	}

	if(fsid_index->air) {
		free(fsid_index->air);
	}

	if(fsid_index->day) {
		free(fsid_index->day);
	}

	if(fsid_index->date) {
		free(fsid_index->date);
	}

	if(fsid_index->rectime) {
		free(fsid_index->rectime);
	}

	if(fsid_index->duration) {
		free(fsid_index->duration);
	}

	if(fsid_index->title) {
		free(fsid_index->title);
	}

	if(fsid_index->episode) {
		free(fsid_index->episode);
	}

	if(fsid_index->episodenr) {
		free(fsid_index->episodenr);
	}

	if(fsid_index->description) {
		free(fsid_index->description);
	}

	if(fsid_index->actors) {
		free(fsid_index->actors);
	}

	if(fsid_index->gstar) {
		free(fsid_index->gstar);
	}

	if(fsid_index->host) {
		free(fsid_index->host);
	}

	if(fsid_index->director) {
		free(fsid_index->director);
	}

	if(fsid_index->eprod) {
		free(fsid_index->eprod);
	}

	if(fsid_index->prod) {
		free(fsid_index->prod);
	}

	if(fsid_index->writer) {
		free(fsid_index->writer);
	}

	free(fsid_index);

}




void print_fsid_index(const fsid_index_t * fsid_index) {


	printf("RecObjFsid@ %i\n", fsid_index->fsid);

	if(fsid_index->tystream) {
		printf("FSID@ %s\n", fsid_index->tystream);
	} else {
		printf("FSID@\n");
	}

	printf("State@ %i\n", fsid_index->state);

	if(fsid_index->year) {
		printf("Year@ %s\n", fsid_index->year);
	} else {
		printf("Year@\n");
	}


	if(fsid_index->air) {
		printf("AirDate@ %s\n", fsid_index->air);
	} else {
		printf("AirDate@\n");
	}


	if(fsid_index->day) {
		printf("Day@ %s\n", fsid_index->day);
	} else {
		printf("Day@\n");
	}


	if(fsid_index->date) {
		printf("Date@ %s\n", fsid_index->date);
	} else {
		printf("Date@\n");
	}


	if(fsid_index->rectime) {
		printf("Time@ %s\n", fsid_index->rectime);
	} else {
		printf("Time@\n");
	}


	if(fsid_index->duration) {
		printf("Duration@ %s\n", fsid_index->duration);
	} else {
		printf("Duration@\n");
	}


	if(fsid_index->title) {
		printf("Title@ %s\n", fsid_index->title);
	} else {
		printf("Title@\n");
	}


	if(fsid_index->episode) {
		printf("Episode@ %s\n", fsid_index->episode);
	} else {
		printf("Episode@\n");
	}


	if(fsid_index->episodenr) {
		printf("Episodenr@ %s\n", fsid_index->episodenr);
	} else {
		printf("Episodenr@\n");
	}


	if(fsid_index->description) {
		printf("Description@ %s\n", fsid_index->description);
	} else {
		printf("Description@\n");
	}


	if(fsid_index->actors) {
		printf("Actors@ %s\n", fsid_index->actors);
	} else {
		printf("Actors@\n");
	}


	if(fsid_index->gstar) {
		printf("GuestStars@ %s\n", fsid_index->gstar);
	} else {
		printf("GuestStars@\n");
	}


	if(fsid_index->host) {
		printf("Host@ %s\n", fsid_index->host);
	} else {
		printf("Host@\n");
	}


	if(fsid_index->director) {
		printf("Director@ %s\n", fsid_index->director);
	} else {
		printf("Director@\n");
	}


	if(fsid_index->eprod) {
		printf("ExecProd@ %s\n", fsid_index->eprod);
	} else {
		printf("ExecProd@\n");
	}


	if(fsid_index->prod) {
		printf("Producer@ %s\n", fsid_index->prod);
	} else {
		printf("Producer@\n");
	}


	if(fsid_index->writer) {
		printf("Writer@ %s\n", fsid_index->writer);
	} else {
		printf("Writer@\n");
	}

	printf("Remote@ %i\n", fsid_index->index_avalible);



}


void write_fsid_index(int fd, const fsid_index_t * fsid_index) {

	char msgbuf[600];

	snprintf(msgbuf, 598,"RecObjFsid@ %i\n", fsid_index->fsid);
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->tystream) {
		snprintf(msgbuf, 598,"FSID@ %s\n", fsid_index->tystream);
	} else {
		snprintf(msgbuf, 598,"FSID@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));


	snprintf(msgbuf, 598,"State@ %i\n", fsid_index->state);
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->year) {
		snprintf(msgbuf, 598,"Year@ %s\n", fsid_index->year);
	} else {
		snprintf(msgbuf, 598,"Year@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->air) {
		snprintf(msgbuf, 598,"AirDate@ %s\n", fsid_index->air);
	} else {
		snprintf(msgbuf, 598,"AirDate@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->day) {
		snprintf(msgbuf, 598,"Day@ %s\n", fsid_index->day);
	} else {
		snprintf(msgbuf, 598,"Day@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->date) {
		snprintf(msgbuf, 598,"Date@ %s\n", fsid_index->date);
	} else {
		snprintf(msgbuf, 598,"Date@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->rectime) {
		snprintf(msgbuf, 598,"Time@ %s\n", fsid_index->rectime);
	} else {
		snprintf(msgbuf, 598,"Time@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->duration) {
		snprintf(msgbuf, 598,"Duration@ %s\n", fsid_index->duration);
	} else {
		snprintf(msgbuf, 598,"Duration@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->title) {
		snprintf(msgbuf, 598,"Title@ %s\n", fsid_index->title);
	} else {
		snprintf(msgbuf, 598,"Title@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->episode) {
		snprintf(msgbuf, 598,"Episode@ %s\n", fsid_index->episode);
	} else {
		snprintf(msgbuf, 598,"Episode@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->episodenr) {
		snprintf(msgbuf, 598,"Episodenr@ %s\n", fsid_index->episodenr);
	} else {
		snprintf(msgbuf, 598,"Episodenr@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->description) {
		snprintf(msgbuf, 598,"Description@ %s\n", fsid_index->description);
	} else {
		snprintf(msgbuf, 598,"Description@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->actors) {
		snprintf(msgbuf, 598,"Actors@ %s\n", fsid_index->actors);
	} else {
		snprintf(msgbuf, 598,"Actors@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->gstar) {
		snprintf(msgbuf, 598,"GuestStars@ %s\n", fsid_index->gstar);
	} else {
		snprintf(msgbuf, 598,"GuestStars@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->host) {
		snprintf(msgbuf, 598,"Host@ %s\n", fsid_index->host);
	} else {
		snprintf(msgbuf, 598,"Host@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->director) {
		snprintf(msgbuf, 598,"Director@ %s\n", fsid_index->director);
	} else {
		snprintf(msgbuf, 598,"Director@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->eprod) {
		snprintf(msgbuf, 598,"ExecProd@ %s\n", fsid_index->eprod);
	} else {
		snprintf(msgbuf, 598,"ExecProd@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->prod) {
		snprintf(msgbuf, 598,"Producer@ %s\n", fsid_index->prod);
	} else {
		snprintf(msgbuf, 598,"Producer@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

	if(fsid_index->writer) {
		snprintf(msgbuf, 598,"Writer@ %s\n", fsid_index->writer);
	} else {
		snprintf(msgbuf, 598,"Writer@\n");
	}
	write(fd, msgbuf, strlen(msgbuf));

#if !defined(TIVO)
	snprintf(msgbuf, 598,"Remote@ %i\n", fsid_index->index_avalible);
	write(fd, msgbuf, strlen(msgbuf));
#endif

}



void swrite_fsid_index(SOCKET_FD sockfd, const fsid_index_t * fsid_index) {

	char msgbuf[600];

	snprintf(msgbuf, 598,"RecObjFsid@ %i\r\n", fsid_index->fsid);
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->tystream) {
		snprintf(msgbuf, 598,"FSID@ %s\r\n", fsid_index->tystream);
	} else {
		snprintf(msgbuf, 598,"FSID@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));


	snprintf(msgbuf, 598,"State@ %i\r\n", fsid_index->state);
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->year) {
		snprintf(msgbuf, 598,"Year@ %s\r\n", fsid_index->year);
	} else {
		snprintf(msgbuf, 598,"Year@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->air) {
		snprintf(msgbuf, 598,"AirDate@ %s\r\n", fsid_index->air);
	} else {
		snprintf(msgbuf, 598,"AirDate@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->day) {
		snprintf(msgbuf, 598,"Day@ %s\r\n", fsid_index->day);
	} else {
		snprintf(msgbuf, 598,"Day@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->date) {
		snprintf(msgbuf, 598,"Date@ %s\r\n", fsid_index->date);
	} else {
		snprintf(msgbuf, 598,"Date@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->rectime) {
		snprintf(msgbuf, 598,"Time@ %s\r\n", fsid_index->rectime);
	} else {
		snprintf(msgbuf, 598,"Time@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->duration) {
		snprintf(msgbuf, 598,"Duration@ %s\r\n", fsid_index->duration);
	} else {
		snprintf(msgbuf, 598,"Duration@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->title) {
		snprintf(msgbuf, 598,"Title@ %s\r\n", fsid_index->title);
	} else {
		snprintf(msgbuf, 598,"Title@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->episode) {
		snprintf(msgbuf, 598,"Episode@ %s\r\n", fsid_index->episode);
	} else {
		snprintf(msgbuf, 598,"Episode@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->episodenr) {
		snprintf(msgbuf, 598,"Episodenr@ %s\r\n", fsid_index->episodenr);
	} else {
		snprintf(msgbuf, 598,"Episodenr@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->description) {
		snprintf(msgbuf, 598,"Description@ %s\r\n", fsid_index->description);
	} else {
		snprintf(msgbuf, 598,"Description@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->actors) {
		snprintf(msgbuf, 598,"Actors@ %s\r\n", fsid_index->actors);
	} else {
		snprintf(msgbuf, 598,"Actors@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->gstar) {
		snprintf(msgbuf, 598,"GuestStars@ %s\r\n", fsid_index->gstar);
	} else {
		snprintf(msgbuf, 598,"GuestStars@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->host) {
		snprintf(msgbuf, 598,"Host@ %s\r\n", fsid_index->host);
	} else {
		snprintf(msgbuf, 598,"Host@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->director) {
		snprintf(msgbuf, 598,"Director@ %s\r\n", fsid_index->director);
	} else {
		snprintf(msgbuf, 598,"Director@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->eprod) {
		snprintf(msgbuf, 598,"ExecProd@ %s\r\n", fsid_index->eprod);
	} else {
		snprintf(msgbuf, 598,"ExecProd@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->prod) {
		snprintf(msgbuf, 598,"Producer@ %s\r\n", fsid_index->prod);
	} else {
		snprintf(msgbuf, 598,"Producer@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	if(fsid_index->writer) {
		snprintf(msgbuf, 598,"Writer@ %s\r\n", fsid_index->writer);
	} else {
		snprintf(msgbuf, 598,"Writer@\r\n");
	}
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	snprintf(msgbuf, 598,"Remote@ %i\r\n", fsid_index->index_avalible);
	socket_write(sockfd, msgbuf, strlen(msgbuf));

}


void free_fsid_index_list(fsid_index_t * fsid_index_list) {


	fsid_index_t * fsid_index;

	while(fsid_index_list) {
		fsid_index = fsid_index_list;
		fsid_index_list = fsid_index_list->next;
		free_fsid_index(fsid_index);
	}

}


void print_fsid_index_list(const fsid_index_t * fsid_index_list) {

	int nr_index;
	fsid_index_t * start_fsid_index;

	start_fsid_index = (fsid_index_t *)fsid_index_list;


	nr_index=0;
	while(start_fsid_index) {
		start_fsid_index = start_fsid_index->next;
		nr_index++;
	}

	printf("NumberOfEntries@ %i\n",nr_index);


	start_fsid_index = (fsid_index_t *)fsid_index_list;


	nr_index=1;
	while(start_fsid_index) {
		printf("StartEntry@ %i\n", nr_index);
		print_fsid_index(start_fsid_index);
		printf("EndEntry@ %i\n",nr_index);
		start_fsid_index = start_fsid_index->next;
		nr_index++;
	}



}


void write_fsid_index_list(int fd, const fsid_index_t * fsid_index_list) {

	int nr_index;
	fsid_index_t * start_fsid_index;
	char msgbuf[600];
	start_fsid_index = (fsid_index_t *)fsid_index_list;


	nr_index=0;
	while(start_fsid_index) {
		start_fsid_index = start_fsid_index->next;
		nr_index++;
	}

	snprintf(msgbuf, 598,"NumberOfEntries@ %i\n",nr_index);
	write(fd, msgbuf, strlen(msgbuf));

	start_fsid_index = (fsid_index_t *)fsid_index_list;


	nr_index=1;
	while(start_fsid_index) {
		snprintf(msgbuf, 598,"StartEntry@ %i\n", nr_index);
		write(fd, msgbuf, strlen(msgbuf));
		write_fsid_index(fd, start_fsid_index);
		snprintf(msgbuf, 598,"EndEntry@ %i\n",nr_index);
		write(fd, msgbuf, strlen(msgbuf));
		start_fsid_index = start_fsid_index->next;
		nr_index++;
	}



}


void swrite_fsid_index_list(SOCKET_FD sockfd, const fsid_index_t * fsid_index_list) {

	int nr_index;
	fsid_index_t * start_fsid_index;
	char msgbuf[600];

	char *okmsg = "200 Showdata\r\n";
	char *okmsg1 = "300 0\r\n";



	start_fsid_index = (fsid_index_t *)fsid_index_list;


	socket_write(sockfd, okmsg, strlen(okmsg));
	socket_write(sockfd, okmsg1, strlen(okmsg1));

	nr_index=0;
	while(start_fsid_index) {
		start_fsid_index = start_fsid_index->next;
		nr_index++;
	}

	snprintf(msgbuf, 598,"NumberOfEntries@ %i\r\n",nr_index);
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	start_fsid_index = (fsid_index_t *)fsid_index_list;


	nr_index=1;
	while(start_fsid_index) {
		snprintf(msgbuf, 598,"StartEntry@ %i\r\n", nr_index);
		socket_write(sockfd, msgbuf, strlen(msgbuf));
		swrite_fsid_index(sockfd, start_fsid_index);
		snprintf(msgbuf, 598,"EndEntry@ %i\r\n",nr_index);
		socket_write(sockfd, msgbuf, strlen(msgbuf));
		start_fsid_index = start_fsid_index->next;
		nr_index++;
	}
}



fsid_index_t * add_fsid_index(fsid_index_t * fsid_index_list, fsid_index_t * fsid_index) {

	fsid_index_t * tmp_fsid_index;

	if(!fsid_index_list) {
		return(fsid_index);
	}

	tmp_fsid_index = (fsid_index_t *)fsid_index_list;

	while(tmp_fsid_index->next) {
		tmp_fsid_index = tmp_fsid_index->next;
	}

	tmp_fsid_index->next = fsid_index;
	fsid_index->previous = tmp_fsid_index;

	return(fsid_index_list);
}




void add_fsid(int fsid, fsid_index_t *  fsid_index) {

	fsid_index->fsid = fsid;

}

void add_tystream(char * tystream, fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(tystream);
	fsid_index->tystream = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->tystream, '\0',size +1);
	strcpy(fsid_index->tystream,tystream);

	for (i = 0; i < size; i++) {
		if ((fsid_index->tystream[i] == '\r') || (fsid_index->tystream[i] == '\n')) {
			fsid_index->tystream[i] = '\0';
			break;
		}
	}

}

void add_state(int state, fsid_index_t *  fsid_index) {

	fsid_index->state = state;

}

void add_year(char * year,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(year);
	fsid_index->year = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->year, '\0',size +1);
	strcpy(fsid_index->year,year);

	for (i = 0; i < size; i++) {
		if ((fsid_index->year[i] == '\r') || (fsid_index->year[i] == '\n')) {
			fsid_index->year[i] = '\0';
			break;
		}
	}


}

void add_air(char * air,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(air);
	fsid_index->air = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->air, '\0',size +1);
	strcpy(fsid_index->air,air);

	for (i = 0; i < size; i++) {
		if ((fsid_index->air[i] == '\r') || (fsid_index->air[i] == '\n')) {
			fsid_index->air[i] = '\0';
			break;
		}
	}


}

void add_day(char * day,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(day);
	fsid_index->day = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->day, '\0',size +1);
	strcpy(fsid_index->day,day);

	for (i = 0; i < size; i++) {
		if ((fsid_index->day[i] == '\r') || (fsid_index->day[i] == '\n')) {
			fsid_index->day[i] = '\0';
			break;
		}
	}



}

void add_date(char * date,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(date);
	fsid_index->date = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->date, '\0',size +1);
	strcpy(fsid_index->date,date);

	for (i = 0; i < size; i++) {
		if ((fsid_index->date[i] == '\r') || (fsid_index->date[i] == '\n')) {
			fsid_index->date[i] = '\0';
			break;
		}
	}



}

void add_rectime(char * rectime,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(rectime);
	fsid_index->rectime = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->rectime, '\0',size +1);
	strcpy(fsid_index->rectime,rectime);

	for (i = 0; i < size; i++) {
		if ((fsid_index->rectime[i] == '\r') || (fsid_index->rectime[i] == '\n')) {
			fsid_index->rectime[i] = '\0';
			break;
		}
	}


}

void add_duration(char * duration,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(duration);
	fsid_index->duration = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->duration, '\0',size +1);
	strcpy(fsid_index->duration,duration);

	for (i = 0; i < size; i++) {
		if ((fsid_index->duration[i] == '\r') || (fsid_index->duration[i] == '\n')) {
			fsid_index->duration[i] = '\0';
			break;
		}
	}


}


void add_title(char * title,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(title);
	fsid_index->title = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->title, '\0',size +1);
	strcpy(fsid_index->title,title);

	for (i = 0; i < size; i++) {
		if ((fsid_index->title[i] == '\r') || (fsid_index->title[i] == '\n')) {
			fsid_index->title[i] = '\0';
			break;
		}
	}


}

void add_episode(char * episode,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(episode);
	fsid_index->episode = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->episode, '\0',size +1);
	strcpy(fsid_index->episode,episode);

	for (i = 0; i < size; i++) {
		if ((fsid_index->episode[i] == '\r') || (fsid_index->episode[i] == '\n')) {
			fsid_index->episode[i] = '\0';
			break;
		}
	}


}

void add_episodenr(char * episodenr,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(episodenr);
	fsid_index->episodenr = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->episodenr, '\0',size +1);
	strcpy(fsid_index->episodenr,episodenr);

	for (i = 0; i < size; i++) {
		if ((fsid_index->episodenr[i] == '\r') || (fsid_index->episodenr[i] == '\n')) {
			fsid_index->episodenr[i] = '\0';
			break;
		}
	}


}



void add_description(char * description,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(description);
	fsid_index->description = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->description, '\0',size +1);
	strcpy(fsid_index->description,description);

	for (i = 0; i < size; i++) {
		if ((fsid_index->description[i] == '\r') || (fsid_index->description[i] == '\n')) {
			fsid_index->description[i] = '\0';
			break;
		}
	}



}

void add_actors(char * actors,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(actors);
	fsid_index->actors = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->actors, '\0',size +1);
	strcpy(fsid_index->actors,actors);

	for (i = 0; i < size; i++) {
		if ((fsid_index->actors[i] == '\r') || (fsid_index->actors[i] == '\n')) {
			fsid_index->actors[i] = '\0';
			break;
		}
	}

}


void add_gstar(char * gstar,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(gstar);
	fsid_index->gstar = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->gstar, '\0',size +1);
	strcpy(fsid_index->gstar,gstar);

	for (i = 0; i < size; i++) {
		if ((fsid_index->gstar[i] == '\r') || (fsid_index->gstar[i] == '\n')) {
			fsid_index->gstar[i] = '\0';
			break;
		}
	}


}

void add_host(char * host,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(host);
	fsid_index->host = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->host, '\0',size +1);
	strcpy(fsid_index->host,host);

	for (i = 0; i < size; i++) {
		if ((fsid_index->host[i] == '\r') || (fsid_index->host[i] == '\n')) {
			fsid_index->host[i] = '\0';
			break;
		}
	}


}

void add_director(char * director,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(director);
	fsid_index->director = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->director, '\0',size +1);
	strcpy(fsid_index->director,director);

	for (i = 0; i < size; i++) {
		if ((fsid_index->director[i] == '\r') || (fsid_index->director[i] == '\n')) {
			fsid_index->director[i] = '\0';
			break;
		}
	}



}

void add_eprod(char * eprod,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(eprod);
	fsid_index->eprod = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->eprod, '\0',size +1);
	strcpy(fsid_index->eprod,eprod);

	for (i = 0; i < size; i++) {
		if ((fsid_index->eprod[i] == '\r') || (fsid_index->eprod[i] == '\n')) {
			fsid_index->eprod[i] = '\0';
			break;
		}
	}



}

void add_prod(char * prod,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(prod);
	fsid_index->prod = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->prod, '\0',size +1);
	strcpy(fsid_index->prod,prod);

	for (i = 0; i < size; i++) {
		if ((fsid_index->prod[i] == '\r') || (fsid_index->prod[i] == '\n')) {
			fsid_index->prod[i] = '\0';
			break;
		}
	}


}

void add_writer(char * writer,fsid_index_t *  fsid_index) {

	int i;
	int size;

	size = strlen(writer);
	fsid_index->writer = (char *)malloc(sizeof(char) * (size +1));
	memset(fsid_index->writer, '\0',size +1);
	strcpy(fsid_index->writer,writer);

	for (i = 0; i < size; i++) {
		if ((fsid_index->writer[i] == '\r') || (fsid_index->writer[i] == '\n')) {
			fsid_index->writer[i] = '\0';
			break;
		}
	}



}


void add_index_avalible(int index_avalible, fsid_index_t *  fsid_index) {

	fsid_index->index_avalible = index_avalible;

}



fsid_index_t * parse_nowshowing_client(SOCKET_HANDLE inpipe) {


	char buf[600];
	char * pt1;
	int lines;
	int fsid;
	int index_avalible;
	int nl;
	int i;
	int state;

	fsid_index_t * fsid_index_list=NULL;
	fsid_index_t * fsid_index;


	socket_gets(buf,sizeof(buf), inpipe);
	pt1 = buf + 17;
	lines = atoi(pt1);

	for(i=1; i <= lines; i++) {

		fsid_index=new_fsid_index();

		/* THE REC FSID */
		socket_gets(buf,sizeof(buf), inpipe); /* Dump the Entry line */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 12;
		fsid = atoi(pt1);
		add_fsid(fsid,fsid_index);

		/* TYSTREAM FILE NAME */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 6;
		add_tystream(pt1,fsid_index);

		/* THE State */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 7;
		state = atoi(pt1);
		add_state(state,fsid_index);


		/* YEAR */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 6;
		if(strlen(pt1)) {
			add_year(pt1,fsid_index);
		}

		/* Airdate */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 9;
		if(strlen(pt1)) {
			add_air(pt1,fsid_index);
		}

		/* Day */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 5;
		if(strlen(pt1)) {
			add_day(pt1,fsid_index);
		}

		/* Date */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 6;
		if(strlen(pt1)) {
			add_date(pt1,fsid_index);
		}

		/* Time */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 6;
		if(strlen(pt1)) {
			add_rectime(pt1,fsid_index);
		}

		/* Duration */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 10;
		if(strlen(pt1)) {
			add_duration(pt1,fsid_index);
		}



		/* TITLE */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 7;
		if(strlen(pt1)) {
			add_title(pt1,fsid_index);
		}

		/* Episode */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 9;
		if(strlen(pt1)) {
			add_episode(pt1,fsid_index);
		}

		/* Episodenr */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 11;
		if(strlen(pt1)) {
			add_episodenr(pt1,fsid_index);
		}

		/* Description */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		pt1 = buf + 13;
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		if(strlen(pt1)) {
			add_description(pt1,fsid_index);
		}

		/* Actor */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 8;
		if(strlen(pt1)) {
			add_actors(pt1,fsid_index);
		}

		/* GStar */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 12;
		if(strlen(pt1)) {
			add_gstar(pt1,fsid_index);
		}

		/* Host */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 6;
		if(strlen(pt1)) {
			add_host(pt1,fsid_index);
		}

		/* Director */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 10;
		if(strlen(pt1)) {
			add_director(pt1,fsid_index);
		}

		/* ExecProd */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 10;
		if(strlen(pt1)) {
			add_eprod(pt1,fsid_index);
		}


		/* ExecProd */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 10;
		if(strlen(pt1)) {
			add_prod(pt1,fsid_index);
		}

		/* Writer */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 8;
		if(strlen(pt1)) {
			add_writer(pt1,fsid_index);
		}
		
		/* Remote index avalible */
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 8;
		index_avalible = atoi(pt1);
		add_index_avalible(index_avalible,fsid_index);

		socket_gets(buf,sizeof(buf), inpipe); /* Dump the Entry line */
		fsid_index_list = add_fsid_index(fsid_index_list, fsid_index);

	}

	return(fsid_index_list);

}


fsid_index_t * parse_nowshowing_server(SOCKET_HANDLE inpipe) {


	char buf[600];
	char txtbuf[100];
	char * pt1;
	int lines;
	int fsid;
	int nl;
	int i;
	int state;
	struct stat fileinfo;

	fsid_index_t * fsid_index_list=NULL;
	fsid_index_t * fsid_index;


	socket_gets(buf,sizeof(buf), inpipe);
	pt1 = buf + 17;
	lines = atoi(pt1);

	for(i=1; i <= lines; i++) {

		fsid_index=new_fsid_index();

		/* THE REC FSID */
		socket_gets(buf,sizeof(buf), inpipe); /* Dump the Entry line */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 12;
		fsid = atoi(pt1);
		add_fsid(fsid,fsid_index);

		/* TYSTREAM FILE NAME */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 6;
		add_tystream(pt1,fsid_index);

		/* THE State */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 7;
		state = atoi(pt1);
		add_state(state,fsid_index);


		/* YEAR */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 6;
		if(strlen(pt1)) {
			add_year(pt1,fsid_index);
		}

		/* Airdate */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 9;
		if(strlen(pt1)) {
			add_air(pt1,fsid_index);
		}

		/* Day */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 5;
		if(strlen(pt1)) {
			add_day(pt1,fsid_index);
		}

		/* Date */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 6;
		if(strlen(pt1)) {
			add_date(pt1,fsid_index);
		}

		/* Time */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 6;
		if(strlen(pt1)) {
			add_rectime(pt1,fsid_index);
		}

		/* Duration */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 10;
		if(strlen(pt1)) {
			add_duration(pt1,fsid_index);
		}



		/* TITLE */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 7;
		if(strlen(pt1)) {
			add_title(pt1,fsid_index);
		}

		/* Episode */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 9;
		if(strlen(pt1)) {
			add_episode(pt1,fsid_index);
		}

		/* Episodenr */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 11;
		if(strlen(pt1)) {
			add_episodenr(pt1,fsid_index);
		}

		/* Description */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		pt1 = buf + 13;
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		if(strlen(pt1)) {
			add_description(pt1,fsid_index);
		}

		/* Actor */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 8;
		if(strlen(pt1)) {
			add_actors(pt1,fsid_index);
		}

		/* GStar */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 12;
		if(strlen(pt1)) {
			add_gstar(pt1,fsid_index);
		}

		/* Host */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 6;
		if(strlen(pt1)) {
			add_host(pt1,fsid_index);
		}

		/* Director */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 10;
		if(strlen(pt1)) {
			add_director(pt1,fsid_index);
		}

		/* ExecProd */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 10;
		if(strlen(pt1)) {
			add_eprod(pt1,fsid_index);
		}


		/* ExecProd */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 10;
		if(strlen(pt1)) {
			add_prod(pt1,fsid_index);
		}

		/* Writer */
		memset(buf, '\0', 599);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		pt1 = buf + 8;
		if(strlen(pt1)) {
			add_writer(pt1,fsid_index);
		}

		memset(txtbuf, '\0', 99);
		snprintf(txtbuf, 98,"/var/index/index/%i", fsid_index->fsid);
		if(!stat(txtbuf, &fileinfo)) {
			/* okay something is there - lets check if reg file */
			if(S_ISREG(fileinfo.st_mode)) {
				add_index_avalible(1, fsid_index);
			} else {
				printf("Warning indexfile not a regluar file - /var/index/index/%i\n", fsid_index->fsid);
			}
		} else {
			//perror("");
			//printf("/var/index/index/%i not avalible\n", fsid_index->fsid);
		}
		socket_gets(buf,sizeof(buf), inpipe); /* Dump the Entry line */
		fsid_index_list = add_fsid_index(fsid_index_list, fsid_index);

	}

	return(fsid_index_list);

}

