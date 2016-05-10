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

void add_gop_index_parse(index_t * index, gop_index_t * gop_index) {

	/* Save gop index */
	gop_index_t * tmp_gop_index;


	if(index->gop_index == NULL) {
		index->gop_index = gop_index;
	} else {
		tmp_gop_index = index->gop_index;
		while(tmp_gop_index->next) {
			tmp_gop_index = tmp_gop_index->next;
		}
		tmp_gop_index->next = gop_index;
		tmp_gop_index->next->previous = tmp_gop_index;
	}


}



void print_gop_index(const gop_index_t * gop_index) {

	printf("" I64FORMAT "\n", gop_index->gop_number);

	printf("" I64FORMAT "\n", gop_index->chunk_number_seq);
	printf("%i\n", gop_index->seq_rec_nr);

	printf("" I64FORMAT "\n", gop_index->chunk_number_i_frame_pes);
	printf("%i\n", gop_index->i_frame_pes_rec_nr);

	printf("" I64FORMAT "\n", gop_index->chunk_number_i_frame);
	printf("%i\n", gop_index->i_frame_rec_nr);

	printf("" I64FORMAT "\n", gop_index->time_of_iframe);

	printf("%i\n", gop_index->reset_of_pts);

}


void write_gop_index(int fd, const gop_index_t * gop_index) {

	char msgbuf[100];

	snprintf(msgbuf, 98, "" I64FORMAT "\n", gop_index->gop_number);
	write(fd, msgbuf, strlen(msgbuf));

	snprintf(msgbuf, 98, "" I64FORMAT "\n", gop_index->chunk_number_seq);
	write(fd, msgbuf, strlen(msgbuf));
	snprintf(msgbuf, 98, "%i\n", gop_index->seq_rec_nr);
	write(fd, msgbuf, strlen(msgbuf));

	snprintf(msgbuf, 98, "" I64FORMAT "\n", gop_index->chunk_number_i_frame_pes);
	write(fd, msgbuf, strlen(msgbuf));
	snprintf(msgbuf, 98, "%i\n", gop_index->i_frame_pes_rec_nr);
	write(fd, msgbuf, strlen(msgbuf));

	snprintf(msgbuf, 98, "" I64FORMAT "\n", gop_index->chunk_number_i_frame);
	write(fd, msgbuf, strlen(msgbuf));
	snprintf(msgbuf, 98, "%i\n", gop_index->i_frame_rec_nr);
	write(fd, msgbuf, strlen(msgbuf));

	snprintf(msgbuf, 98, "" I64FORMAT "\n", gop_index->time_of_iframe);
	write(fd, msgbuf, strlen(msgbuf));

	snprintf(msgbuf, 98, "%i\n", gop_index->reset_of_pts);
	write(fd, msgbuf, strlen(msgbuf));


}


void swrite_gop_index(SOCKET_FD sockfd, const gop_index_t * gop_index) {

	char msgbuf[100];

	snprintf(msgbuf, 98, "" I64FORMAT "\r\n", gop_index->gop_number);
	write(sockfd, msgbuf, strlen(msgbuf));

	snprintf(msgbuf, 98, "" I64FORMAT "\r\n", gop_index->chunk_number_seq);
	socket_write(sockfd, msgbuf, strlen(msgbuf));
	snprintf(msgbuf, 98, "%i\r\n", gop_index->seq_rec_nr);
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	snprintf(msgbuf, 98, "" I64FORMAT "\r\n", gop_index->chunk_number_i_frame_pes);
	socket_write(sockfd, msgbuf, strlen(msgbuf));
	snprintf(msgbuf, 98, "%i\r\n", gop_index->i_frame_pes_rec_nr);
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	snprintf(msgbuf, 98, "" I64FORMAT "\r\n", gop_index->chunk_number_i_frame);
	socket_write(sockfd, msgbuf, strlen(msgbuf));
	snprintf(msgbuf, 98, "%i\r\n", gop_index->i_frame_rec_nr);
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	snprintf(msgbuf, 98, "" I64FORMAT "\r\n", gop_index->time_of_iframe);
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	snprintf(msgbuf, 98, "%i\r\n", gop_index->reset_of_pts);
	write(sockfd, msgbuf, strlen(msgbuf));

}


void print_gop_index_list(const index_t * index) {

	gop_index_t * start_gop_index;

	start_gop_index = index->gop_index;

	printf("NumberOfEntries@ " I64FORMAT "\n",index->nr_of_gops);


	while(start_gop_index) {
		print_gop_index(start_gop_index);
		start_gop_index = start_gop_index->next;
	}

}


void write_gop_index_list(int fd, const index_t * index) {

	char msgbuf[100];
	gop_index_t * start_gop_index;

	start_gop_index = index->gop_index;

	snprintf(msgbuf, 98,"NumberOfEntries@ " I64FORMAT "\n",index->nr_of_gops);
	write(fd, msgbuf, strlen(msgbuf));


	while(start_gop_index) {
		write_gop_index(fd, start_gop_index);
		start_gop_index = start_gop_index->next;
	}
}

void swrite_gop_index_list(SOCKET_FD sockfd, const index_t * index) {


	gop_index_t * start_gop_index;

	char msgbuf[100];

	char * msg = "200: Printing index\r\n";
	char * msg2 = "200: Finshed printing index\r\n";

	char * msg4 = "300: 0 \r\n";

	start_gop_index = index->gop_index;

	socket_write(sockfd, msg, strlen(msg));

	socket_write(sockfd, msg4, strlen(msg4));

	start_gop_index = index->gop_index;

	snprintf(msgbuf, 98,"NumberOfEntries@ " I64FORMAT "\r\n",index->nr_of_gops);
	socket_write(sockfd, msgbuf, strlen(msgbuf));

	while(start_gop_index) {
		swrite_gop_index(sockfd, start_gop_index);
		start_gop_index = start_gop_index->next;
	}

	socket_write(sockfd, msg2, strlen(msg2));

}


void count_gop_index_list(index_t * index) {

	int64_t nr_index;
	gop_index_t * start_gop_index;
	start_gop_index = index->gop_index;


	nr_index=0;
	while(start_gop_index) {
		start_gop_index = start_gop_index->next;
		nr_index++;
	}

	index->nr_of_gops = nr_index;


}


void add_gop_number(int64_t gop_number, gop_index_t *  gop_index) {

	gop_index->gop_number = gop_number;

}


void add_chunk_number_seq(int64_t chunk_number_seq, gop_index_t *  gop_index) {

	gop_index->chunk_number_seq = chunk_number_seq;

}

void add_seq_rec_nr(int seq_rec_nr, gop_index_t *  gop_index) {

	gop_index->seq_rec_nr = seq_rec_nr;

}



void add_chunk_number_i_frame_pes(int64_t chunk_number_i_frame_pes, gop_index_t *  gop_index) {

	gop_index->chunk_number_i_frame_pes = chunk_number_i_frame_pes;

}

void add_i_frame_pes_rec_nr(int i_frame_pes_rec_nr, gop_index_t *  gop_index) {

	gop_index->i_frame_pes_rec_nr = i_frame_pes_rec_nr;

}



void add_chunk_number_i_frame(int64_t chunk_number_i_frame, gop_index_t *  gop_index) {

	gop_index->chunk_number_i_frame = chunk_number_i_frame;

}

void add_i_frame_rec_nr(int i_frame_rec_nr, gop_index_t *  gop_index) {

	gop_index->i_frame_rec_nr = i_frame_rec_nr;

}


void add_time_of_iframe(ticks_t time_of_iframe, gop_index_t *  gop_index) {

	gop_index->time_of_iframe = time_of_iframe;

}


void add_reset_of_pts(int reset_of_pts, gop_index_t *  gop_index) {

	gop_index->reset_of_pts = reset_of_pts;

}


void  parse_gop_index(SOCKET_HANDLE inpipe, index_t * index, int *progress) {


	char buf[100];
	char * pt1;
	int lines;
	int64_t i64;
	int i32;
	ticks_t ticks;
	int i;
	int nl;

	gop_index_t * gop_index;

	/* Get number of gop_index */
	socket_gets(buf,sizeof(buf), inpipe);
	pt1 = buf + 17;
	lines = atoi(pt1);


	index->nr_of_gops = (int64_t)lines;


	for(i=1; i <= lines; i++) {

		if( progress ) {
			*progress = (i*99)/lines;
		}

		gop_index=new_gop_index();
		index->last_gop_index = gop_index;

		/* GOP number */
		memset(buf, '\0', 99);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		i64 = atoll(buf);
		add_gop_number(i64,gop_index);

		/* SEQ */
		memset(buf, '\0', 99);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		i64 = atoll(buf);
		add_chunk_number_seq(i64,gop_index);


		/* SEQ rec */
		memset(buf, '\0', 99);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		i32 = atoi(buf);
		add_seq_rec_nr(i32,gop_index);

		/* PES */
		memset(buf, '\0', 99);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		i64 = atoll(buf);
		add_chunk_number_i_frame_pes(i64,gop_index);


		/* PES rec */
		memset(buf, '\0', 99);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		i32 = atoi(buf);
		add_i_frame_pes_rec_nr(i32,gop_index);


		/* I */
		memset(buf, '\0', 99);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		i64 = atoll(buf);
		add_chunk_number_i_frame(i64,gop_index);


		/* I rec */
		memset(buf, '\0', 99);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		i32 = atoi(buf);
		add_i_frame_rec_nr(i32,gop_index);


		/* Time */
		memset(buf, '\0', 99);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		ticks = (ticks_t)atoll(buf);
		add_time_of_iframe(ticks,gop_index);

		/* Reset in pts */
		memset(buf, '\0', 99);
		socket_gets(buf,sizeof(buf), inpipe);
		nl = strlen(buf);
		buf[nl - 1] = '\0';
		i32 = atoi(buf);
		add_reset_of_pts(i32,gop_index);

		add_gop_index_parse(index, gop_index);

	}
	return;

}

