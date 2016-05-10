
/*
 * Copyright (C) 2002, 2003, 2015  Olof <jinxolina@gmail.com>
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

/* The open socket func is from playitsam (c) 2002, Warren Toomey wkt@tuhs.org */
#include "common.h"
#include "global.h"

#include <string.h>

#ifdef _WIN32
#include "../libs/winsock_gets.c"
#endif

static int okay_or_not(char * buf);
static int open_socket_to_tyserver(char *inhost, int port);

/* open a socket to a tcp remote host with the specified port */
static int open_socket_to_tyserver(char *inhost, int port) {

	int type = SOCK_STREAM;
	struct sockaddr_in sock_out;
	struct hostent * hp;
	SOCKET_FD res;
	int nodelay = 1;
	int window = 65535;
	char *host, *numptr, *numend;
	uint32_t raw_address = 0;
	int one_part;

	host = strdup(inhost);

	if (host == NULL){
		return (-1);
	}


	res = socket(PF_INET, type, 0);

	if (res == -1) {
		free(host);
		return (-1);
	}


//#ifdef CAN_DO_NAME_LOOKUPS
  	/* Can someone tell me how to get /etc/hosts working on the TiVo? */
	hp = gethostbyname(host);
	if (!hp) {
		LOG_ERROR1("unknown host: %s - lets see if it's a IP address\n", host);
		/* lets fall through and see if it's a IP */
		//return (-1);
	} else {
		memcpy(&sock_out.sin_addr, hp->h_addr, hp->h_length);
		sock_out.sin_port = htons((short)port);
		sock_out.sin_family = PF_INET;

		if (connect(res, (struct sockaddr *) & sock_out, sizeof(sock_out))) {
			close(res);
			LOG_ERROR1("failed to connect to %s\n", inhost);
			free(host);
			return (-1);
		}

		setsockopt(res, SOL_SOCKET, SO_RCVBUF, (sockopt_valptr_t) &window, sizeof(int));
		setsockopt(res, IPPROTO_TCP, TCP_NODELAY, (sockopt_valptr_t) &nodelay, sizeof(int));

		free(host);
		return (res);

	}
//#endif

	/* Walk the IP address and convert into a 32-bit integer */
	/* Part one */
	numend = NULL;
	numptr = host;
	numend = strchr(host, '.');

	if (numend == NULL) {
		LOG_ERROR1("%s not an IP address\n", inhost);
		free(host);
		return (-1);
	}

	*numend = '\0';
	one_part = atoi(numptr);



	if ((one_part < 0) || (one_part > 255)) {
		LOG_ERROR1("%s not an IP address\n", inhost);
		free(host);
		return (-1);
	}

	raw_address = one_part << 24;

	/* Part two */

	numptr = ++numend;
	numend = strchr(numptr, '.');

	if (numptr == NULL) {
		LOG_ERROR1("%s not an IP address\n", inhost);
		free(host);
		return (-1);
	}

	*numend = '\0';
	one_part = atoi(numptr);

	if ((one_part < 0) || (one_part > 255)) {
		LOG_ERROR1("%s not an IP address\n", inhost);
		free(host);
		return (-1);
	}

	raw_address += (one_part << 16);

	/* Part three */
	numptr = ++numend;
	numend = strchr(numptr, '.');

	if (numptr == NULL) {
		LOG_ERROR1("%s not an IP address\n", inhost);
		free(host);
		return (-1);
	}

	*numend = '\0';
	one_part = atoi(numptr);

	if ((one_part < 0) || (one_part > 255)) {
		LOG_ERROR1("%s not an IP address\n", inhost);
		free(host);
		return (-1);
	}

	raw_address += (one_part << 8);

	/* Part four */
	numptr = ++numend;
	one_part = atoi(numptr);

	if ((one_part < 0) || (one_part > 255)) {
		LOG_ERROR1("%s not an IP address\n", inhost);
		free(host);
		return (-1);
	}

	raw_address += one_part;

	/* Do the connection */
	sock_out.sin_addr.s_addr = htonl(raw_address);
	sock_out.sin_port = htons((short)port);
	sock_out.sin_family = PF_INET;

	if (connect(res, (struct sockaddr *) & sock_out, sizeof(sock_out))) {
		close(res);
		LOG_ERROR1("failed to connect to %s\n", inhost);
		free(host);
		return (-1);
	}

	setsockopt(res, SOL_SOCKET, SO_RCVBUF, (sockopt_valptr_t) &window, sizeof(int));
	setsockopt(res, IPPROTO_TCP, TCP_NODELAY, (sockopt_valptr_t) &nodelay, sizeof(int));

	free(host);
	return (res);
}

static int okay_or_not(char * buf) {

	if(strncmp(buf, "200", 3) == 0) {
		return(1);
	} else if (strncmp(buf, "300", 3) == 0) {
		return(2);
	}

	return(0);
}

remote_holder_t * new_remote_holder(char * hostname) {

	remote_holder_t * remote_holder;
	int size;

	remote_holder = (remote_holder_t *)malloc(sizeof(remote_holder_t));
	memset(remote_holder, 0,sizeof(remote_holder_t));
	size = strlen(hostname);
	remote_holder->hostname = (char *)malloc(sizeof(char) * (size + 100));
	memset(remote_holder->hostname, '\0',size +100);
	strcpy(remote_holder->hostname,hostname);

	remote_holder->fsid_index = NULL;
	remote_holder->inpipe = NULL_SOCKET_HANDLE;
	remote_holder->sockfd = INVALID_SOCKET;

	return(remote_holder);
}


int tydemux_init_remote(remote_holder_t * remote_holder) {

	/* Socket filedes */
	SOCKET_FD sockfd;
	SOCKET_HANDLE inpipe;
	char buf[100];
	fsid_index_t * fsid_index;


	if(!remote_holder) {
		return(0);
	}

#ifdef WIN32
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		wVersionRequested = MAKEWORD( 1, 0 );

		err = WSAStartup( wVersionRequested, &wsaData );
		if ( err != 0 ) {
		 /* Tell the user that we could not find a usable */
		 /* WinSock DLL.   */
		 return 0;
		}
	}
#endif

	sockfd= open_socket_to_tyserver(remote_holder->hostname, 1150);

	if(sockfd == INVALID_SOCKET) {
		LOG_ERROR("Error opening socket to tivo\n");
		return(0);
	}




	remote_holder->sockfd = sockfd;

	if(socket_write(sockfd, "LITY \r\n", 7) != 7) {
		LOG_ERROR("Error sending LITY command\n");
		socket_close(remote_holder->sockfd);
		remote_holder->sockfd = INVALID_SOCKET;
		return(0);
	}

	/* Open a tmp pipe to parse data */
	inpipe = socket_open_handle(sockfd, "r");

	if(!inpipe) {
		LOG_ERROR("Error opening inpipe\n");
		return(0);
	}
	remote_holder->inpipe = inpipe;

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);

	if(!okay_or_not(buf)) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		return(0);
	}

	/* Go fwd to data */
	while(okay_or_not(buf) == 1) {
		memset(buf, '\0', 99);
		socket_gets(buf,sizeof(buf), inpipe);
	}

	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}

	fsid_index = parse_nowshowing_client(inpipe);



	if(!fsid_index) {
		LOG_ERROR("Error no remote recordings avalible\n");
		return(0);
	}

	remote_holder->fsid_index = fsid_index;

	return(1);
}


int tydemux_refresh_fsid(remote_holder_t * remote_holder) {

	/* Socket filedes */
	SOCKET_FD sockfd;
	SOCKET_HANDLE inpipe;
	char buf[100];
	fsid_index_t * fsid_index;


	if(!remote_holder->sockfd == INVALID_SOCKET) {
		LOG_ERROR("You will need to init the remote connection first\n");
		return(0);
	}



	sockfd = remote_holder->sockfd;

	if(socket_write(sockfd, "LITY \r\n", 7) != 7) {
		LOG_ERROR("Error sending LITY command\n");
		socket_close(remote_holder->sockfd);
		remote_holder->sockfd = INVALID_SOCKET;
		return(0);
	}

	/* Open a tmp pipe to parse data */
	inpipe = socket_open_handle(sockfd, "r");

	if(!inpipe) {
		LOG_ERROR("Error opening inpipe\n");
		return(0);
	}
	remote_holder->inpipe = inpipe;

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);

	if(!okay_or_not(buf)) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		return(0);
	}

	/* Go fwd to data */
	while(okay_or_not(buf) == 1) {
		memset(buf, '\0', 99);
		socket_gets(buf,sizeof(buf), inpipe);
	}

	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}

	fsid_index = parse_nowshowing_client(inpipe);



	if(!fsid_index) {
		LOG_ERROR("Error no remote recordings avalible\n");
		return(0);
	}

	if(remote_holder->fsid_index) {
		free_fsid_index_list(remote_holder->fsid_index);
		remote_holder->fsid_index = NULL;
	}

	remote_holder->fsid_index = fsid_index;

	return(1);
}






index_t * tydemux_index_remote(tystream_holder_t * tystream, remote_holder_t * remote_holder, int *progress, char *szProgress ) {

	/* Socket filedes */
	SOCKET_HANDLE inpipe;
	char buf[100];
	int numwrite;
	index_t * index;


	if(!remote_holder) {
		LOG_ERROR("File not opened\n");
		return(0);
	}

	inpipe = remote_holder->inpipe;

	memset(buf, '\0', 99);
	sprintf(buf, "CITY \r\n");
	if( szProgress ) {
		strcpy( szProgress, "Sending Request" );
	}

	numwrite = socket_write(remote_holder->sockfd, buf, strlen(buf));

	if (numwrite < 1) {
		LOG_ERROR("Failure sending CITY command\n");
		return(0);
	}


	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);

	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}


	/* Go fwd to data */
	while(okay_or_not(buf) == 1) {
		memset(buf, '\0', 99);
		socket_gets(buf,sizeof(buf), inpipe);
		LOG_USERDIAG1("%s ",buf);
		if( szProgress ) {
			strcpy( szProgress, buf );
		}
	}

	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}

	index = (index_t *)malloc(sizeof(index_t));
	memset(index, 0,sizeof(index_t));
	index->gop_index = NULL;
	index->nr_of_gops = 0;

	if( szProgress ) {
		strcpy( szProgress, "Fetching data" );
	}
	parse_gop_index(inpipe, index, progress);

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		free_index(index);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}



	if(index->gop_index) {
		/* Uncomment for heavy diag
		print_gop_index_list(index); */
		return(scan_index(tystream, index));
	}

	LOG_ERROR("Error no index\n");
	free_index(index);
	return(0);

}


static int tydemux_read_all(SOCKET_HANDLE inpipe, char *buf, int size) {

	int n;
        while (size) {
 		n = socket_read(buf, 1, size, inpipe);
		if (n <= 0) {
                        // fprintf(stderr,"ERROR: eof in read_all\n");
                        return(0);
                }
                buf += n;
                size -= n;
        }
	return(1);
}


static int tydemux_read_all_play(tystream_holder_t * tystream, int * progress, SOCKET_HANDLE inpipe, int file) {

	char buf[CHUNK_SIZE];
	int numread;
	int64_t total_read;
	int64_t nr_of_chunks;

	total_read = 0;
	while (1) {
		numread = socket_read(buf, 1,CHUNK_SIZE, inpipe);
		if(progress) {
			total_read += (int64_t)numread;
			nr_of_chunks = total_read/(int64_t)CHUNK_SIZE;
			*progress = (int)((nr_of_chunks * 100) / tystream->nr_of_remote_chunks);
		}

		if (numread < 0) {
			return(0);
		}

		if (numread == 0) {
			return(1);
		}

		write(file, buf, numread);
	}

	return(1);

}


vstream_t *  tydemux_get_remote_play_chunk(remote_holder_t * remote_holder, tystream_holder_t * tystream) {

	vstream_t * vstream = NULL;

	vstream = new_vstream();
	/* Lets fake it and do a read buffer */
	vstream->start_stream = (uint8_t *)malloc(sizeof(uint8_t) * CHUNK_SIZE);
	memset(vstream->start_stream, 0,sizeof(uint8_t) * CHUNK_SIZE);
	vstream->size = sizeof(uint8_t) * CHUNK_SIZE;
	vstream->current_pos = vstream->start_stream;
	vstream->end_stream = vstream->start_stream + vstream->size;

	if(!tydemux_read_all(remote_holder->inpipe, vstream->start_stream, CHUNK_SIZE)) {
		free_vstream(vstream);
#ifdef WIN32
		assert( tystream->vstream != (vstream_t *)0xdddddddd );
#endif
		LOG_ERROR("Error reading remote chunk\n");
		return(0);
	}

	return(vstream);
}




int tydemux_remote_play_stream(tystream_holder_t * tystream, chunknumber_t chunk_nr) {

	char buf[100];
	int numwrite;
	SOCKET_HANDLE inpipe;
	remote_holder_t * remote_holder;

	inpipe = tystream->remote_holder->inpipe;
	remote_holder = tystream->remote_holder;

	memset(buf, '\0', 99);
	sprintf(buf, "PLTY " I64FORMAT "\r\n", chunk_nr);
	numwrite = socket_write(remote_holder->sockfd, buf, strlen(buf));

	if (numwrite < 1) {
		LOG_ERROR("Failure sending PLTY command\n");
		return(0);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}

	return(1);

}




int tydemux_get_remote_stream(tystream_holder_t * tystream, int * progress, remote_holder_t * remote_holder, int file) {

	char buf[100];
	int numwrite;
	SOCKET_HANDLE inpipe;

	inpipe = remote_holder->inpipe;


	memset(buf, '\0', 99);
	sprintf(buf, "PLTY 0\r\n");
	numwrite = socket_write(remote_holder->sockfd, buf, strlen(buf));

	if (numwrite < 1) {
		LOG_ERROR("Failure sending PLTY command\n");
		return(0);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}

	return(tydemux_read_all_play(tystream, progress, inpipe, file));


}

vstream_t * tydemux_get_remote_chunk(remote_holder_t * remote_holder, tystream_holder_t * tystream, int64_t chunk_number) {

	char buf[100];
	char * pt1;
	int numwrite;
	SOCKET_HANDLE inpipe;
	int size;
	vstream_t * vstream;

	inpipe = remote_holder->inpipe;

	if(!tystream) {
		LOG_ERROR("Error remote file not opened\n");
		return(0);
	}


	memset(buf, '\0', 99);
	sprintf(buf, "SCTY " I64FORMAT "\r\n", chunk_number);
	numwrite = socket_write(remote_holder->sockfd, buf, strlen(buf));

	if (numwrite < 1) {
		LOG_ERROR("Failure sending SCTY command\n");
		return(0);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}

	/* Lets look at the size of the data */
	pt1 = buf + 4;

	size = atoi(pt1);

	if(size != CHUNK_SIZE) {
		LOG_ERROR("ERROR: Chunk is not of valid size discard chunk!!\n");
	}

	vstream = new_vstream();
	/* Lets fake it and do a read buffer */
	vstream->start_stream = (uint8_t *)malloc(sizeof(uint8_t) * size);
	memset(vstream->start_stream, 0,sizeof(uint8_t) * size);
	vstream->size = sizeof(uint8_t) * size ;
	vstream->current_pos = vstream->start_stream;
	vstream->end_stream = vstream->start_stream + vstream->size;

	if(!tydemux_read_all(remote_holder->inpipe, vstream->start_stream, size)) {
		free_vstream(vstream);
#ifdef WIN32
		assert( tystream->vstream != (vstream_t *)0xdddddddd );
#endif
		LOG_ERROR("Error reading remote chunk\n");
		return(0);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		free_vstream(vstream);
		LOG_ERROR1("Error getting okay message - %s\n", buf);
#ifdef WIN32
		assert( tystream->vstream != (vstream_t *)0xdddddddd );
#endif
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}


	return(vstream);
}



vstream_t * tydemux_get_remote_chunk_X(remote_holder_t * remote_holder, tystream_holder_t * tystream, int64_t chunk_number) {

	char buf[100];
	char * pt1;
	int numwrite;
	SOCKET_HANDLE inpipe;
	int size;
	vstream_t * vstream;

	inpipe = remote_holder->inpipe;

	if(!tystream) {
		LOG_ERROR("Error remote file not opened\n");
		return(0);
	}


	memset(buf, '\0', 99);
	sprintf(buf, "SCTX " I64FORMAT "\r\n", chunk_number);
	numwrite = socket_write(remote_holder->sockfd, buf, strlen(buf));

	if (numwrite < 1) {
		LOG_ERROR("Failure sending SCTX command\n");
		return(0);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}

	/* Lets look at the size of the data */
	pt1 = buf + 4;

	size = atoi(pt1);

	if(size != CHUNK_SIZE *  10) {
		LOG_ERROR1("ERROR: Chunk is not of valid size discard chunk %i!!\n", size);
		return(0);
	}

	vstream = new_vstream();
	/* Lets fake it and do a read buffer */
	vstream->start_stream = (uint8_t *)malloc(sizeof(uint8_t) * size);
	memset(vstream->start_stream, 0,sizeof(uint8_t) * size);
	vstream->size = sizeof(uint8_t) * size ;
	vstream->current_pos = vstream->start_stream;
	vstream->end_stream = vstream->start_stream + vstream->size;

	if(!tydemux_read_all(remote_holder->inpipe, vstream->start_stream, size)) {
		free_vstream(vstream);
#ifdef WIN32
		assert( tystream->vstream != (vstream_t *)0xdddddddd );
#endif
		LOG_ERROR("Error reading remote chunk\n");
		return(0);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		free_vstream(vstream);
		LOG_ERROR1("Error getting okay message - %s\n", buf);
#ifdef WIN32
		assert( tystream->vstream != (vstream_t *)0xdddddddd );
#endif
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}


	return(vstream);
}



module_t * tydemux_get_remote_seq(remote_holder_t * remote_holder, tystream_holder_t * tystream) {

	char buf[100];
	char * pt1;
	int numwrite;
	SOCKET_HANDLE inpipe;
	int size;
	module_t * module;

	inpipe = remote_holder->inpipe;

	if(!tystream) {
		LOG_ERROR("Error remote file not opened\n");
		return(0);
	}


	memset(buf, '\0', 99);
	sprintf(buf, "GSTY \r\n");
	numwrite = socket_write(remote_holder->sockfd, buf, strlen(buf));

	if (numwrite < 1) {
		LOG_ERROR("Failure sending GSTY command\n");
		return(0);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}

	/* Lets look at the size of the data */
	pt1 = buf + 4;

	size = atoi(pt1);

	module = (module_t *)malloc(sizeof(module_t));
	memset(module, 0,sizeof(module_t));
	module->data_buffer = NULL;
	module->buffer_size = 0;

	/* Lets fake it and do a read buffer */
	module->data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * size);
	memset(module->data_buffer, 0,sizeof(uint8_t) * size);
	module->buffer_size = sizeof(uint8_t) * size ;

	if(!tydemux_read_all(remote_holder->inpipe, module->data_buffer, size)) {
		free_module(module);
		LOG_ERROR("Error reading remote SEQ record\n");
		return(0);
	}


	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		free_module(module);
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}


	return(module);
}




module_t * tydemux_get_remote_i_frame(remote_holder_t * remote_holder, tystream_holder_t * tystream, int64_t gop_number) {

	char buf[100];
	char * pt1;
	int numwrite;
	SOCKET_HANDLE inpipe;
	int size;
	module_t * module;

	inpipe = remote_holder->inpipe;

	if(!tystream) {
		LOG_ERROR("Error remote file not opened\n");
		return(0);
	}


	memset(buf, '\0', 99);
	sprintf(buf, "GITY " I64FORMAT "\r\n", gop_number);
	numwrite = socket_write(remote_holder->sockfd, buf, strlen(buf));

	if (numwrite < 1) {
		LOG_ERROR("Failure sending GITY command\n");
		return(0);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}

	/* Lets look at the size of the data */
	pt1 = buf + 4;

	size = atoi(pt1);

	module = (module_t *)malloc(sizeof(module_t));
	memset(module, 0,sizeof(module_t));
	module->data_buffer = NULL;
	module->buffer_size = 0;

	/* Lets fake it and do a read buffer */
	module->data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * size);
	memset(module->data_buffer, 0,sizeof(uint8_t) * size);
	module->buffer_size = sizeof(uint8_t) * size ;

	if(!tydemux_read_all(remote_holder->inpipe, module->data_buffer, size)) {
		free_module(module);
		LOG_ERROR("Error reading remote I-Frame record\n");
		return(0);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		free_module(module);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}


	return(module);
}


tystream_holder_t * tydemux_open_probe_remote(remote_holder_t * remote_holder, int fsid) {

	tystream_holder_t * tystream;
	char buf[100];
	int numwrite;
	SOCKET_HANDLE inpipe;
	int i;

	inpipe = remote_holder->inpipe;


	memset(buf, '\0', 99);
	sprintf(buf, "OPTY %d\r\n", fsid);
	numwrite = socket_write(remote_holder->sockfd, buf, strlen(buf));

	if (numwrite < 1) {
		LOG_ERROR("Failure sending OPTY command\n");
		return(0);
	}

	/* Lets see if we got a first okay */

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}

	/* We have four okay strings */
	for(i=0; i < 4; i++) {
		memset(buf, '\0', 99);
		socket_gets(buf,sizeof(buf), inpipe);
		if(okay_or_not(buf) != 1) {
			LOG_ERROR1("Error getting okay message - %s\n", buf);
			return(0);
		} else {
			LOG_DEVDIAG1("%s ",buf);
		}
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}

	/* Okay we are now on to the real probe info
	create a tystream */
	tystream = new_tystream(DEMUX);

	/* The start chunk is always zero when
	doing remote demux - since tyserver is
	not delivering chunks with the wrong
	audio */

	tystream->start_chunk = 0;
	/* We only support V2_X and up!! */
	tystream->tivo_version = V_2X;

	/* Okay lets get the tivo type! */
	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(strncmp("SA", buf, 2) == 0) {
		tystream->tivo_type = SA;
	} else if(strncmp("DT", buf, 2) == 0){
		tystream->tivo_type = DTIVO;
	} else {
		LOG_ERROR1("Wrong Tivo type %s\n", buf);
		free_tystream(tystream);
		return(0);
	}

	/* We should have a 200 again */
	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		free_tystream(tystream);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}


	/* Okay lets get the tivo series */
	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);

	if(atoi(buf) == 1) {
		tystream->tivo_series = S1;
	} else if (atoi(buf) == 2) {
		tystream->tivo_series = S2;
	} else {
		LOG_ERROR("Wrong Tivo Series\n");
		free_tystream(tystream);
		return(0);
	}

	/* We should have a 200 again */
	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		free_tystream(tystream);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}


	/* Okay lets get the tivo audio type */
	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);

	if(strncmp("AC3", buf, 3) == 0) {
		tystream->wrong_audio = 0x3c0;
		tystream->right_audio = 0x9c0;
		tystream->audio_startcode = AC3_PES_AUDIO;
		tystream->DTIVO_MPEG = 0;

	} else if(strncmp("MPG", buf, 3) == 0){
		tystream->wrong_audio = 0x9c0;
		tystream->right_audio = 0x3c0;
		tystream->audio_startcode = MPEG_PES_AUDIO;
		if(tystream->tivo_type == DTIVO) {
			tystream->DTIVO_MPEG = 3;
		} else {
			tystream->DTIVO_MPEG = 0;
		}

	} else {
		LOG_ERROR1("Wrong Audio Type %s\n", buf);
		free_tystream(tystream);
		return(0);
	}

	/* We should have a 200 again */
	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		free_tystream(tystream);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}


	/* Okay lets get the audio size */
	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	tystream->audio_frame_size = atoi(buf);

	switch(tystream->audio_frame_size) {
		case 1536:
			tystream->std_audio_size = 1552;
			tystream->med_size = 1552;
			tystream->audio_type = DTIVO_AC3;
			break;
		case 864:
			tystream->std_audio_size = 880;
			tystream->med_size = 880;
			tystream->audio_type = SA_MPEG;
			break;
		case 768:
			tystream->std_audio_size = 780;
			tystream->med_size = 780;
			tystream->audio_type = DTIVO_MPEG_1;
			break;
		case 576:
			tystream->std_audio_size = 588;
			tystream->med_size = 588;
			tystream->audio_type = DTIVO_MPEG_2;
			break;
		case 480:
			tystream->std_audio_size = 492;
			tystream->med_size = 492;
			tystream->audio_type = DTIVO_MPEG_3;
			break;
		case 336:
			tystream->std_audio_size = 348;
			tystream->med_size = 348;
			tystream->audio_type = DTIVO_MPEG_4;
			break;
		default:
			LOG_ERROR1("Wrong audio size! %i\n", tystream->audio_frame_size);
			free_tystream(tystream);
			return(0);
	}

	/* We should have a 200 again */
	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		free_tystream(tystream);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}


	/* Okay lets get the audio tick */
	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	tystream->audio_median_tick_diff = atoll(buf);

	switch(tystream->audio_median_tick_diff) {
		case 3240:
		case 2160:
		case 2880:
			break;
		default:
			LOG_ERROR1("Wrong audio tickdiff! " I64FORMAT "\n", tystream->audio_median_tick_diff);
			free_tystream(tystream);
			return(0);
	}



	/* We should have a 200 again */
	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		free_tystream(tystream);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}




	/* Okay lets get the framerate */
	/* Get number of chunks */
	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	tystream->frame_rate = atoi(buf);

	switch(tystream->frame_rate) {
		case 4:
			tystream->tick_diff = 6006;
			tystream->frame_tick = 3003;
			tystream->drift_threshold = (tystream->frame_tick/2) + (tystream->frame_tick/20);
			break;
		case 3:
			tystream->tick_diff = 7200;
			tystream->frame_tick = 3600;
			tystream->drift_threshold = (tystream->frame_tick/2) + (tystream->frame_tick/20);
			break;
		default:
			LOG_ERROR("Invalid framerate!\n");
			free_tystream(tystream);
			return(0);
	}

	/* We should have a 200 again */
	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		free_tystream(tystream);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}

	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 2) {
		LOG_ERROR1("Error no data returned - %s\n", buf);
		return(0);
	}

	/* Get number of chunks */
	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	tystream->nr_of_remote_chunks = atoll(buf);


	/* We should have a 200 again */
	memset(buf, '\0', 99);
	socket_gets(buf,sizeof(buf), inpipe);
	if(okay_or_not(buf) != 1) {
		LOG_ERROR1("Error getting okay message - %s\n", buf);
		free_tystream(tystream);
		return(0);
	} else {
		LOG_DEVDIAG1("%s ",buf);
	}


	return(tystream);

}

void free_remote_holder(remote_holder_t * remote_holder) {

	sleep(1);
	if(remote_holder->inpipe) {
		socket_close_handle(remote_holder->inpipe);
	}
	if(remote_holder->sockfd != INVALID_SOCKET) {
		socket_close(remote_holder->sockfd);
	}
	if(remote_holder->fsid_index) {
		free_fsid_index(remote_holder->fsid_index);
	}
	if(remote_holder->hostname) {
		free(remote_holder->hostname);
	}
	free(remote_holder);
}


void tydemux_close_remote_tystream(remote_holder_t * remote_holder) {
	
	char buf[100];

	if(remote_holder->sockfd != INVALID_SOCKET) {
		socket_write(remote_holder->sockfd, "COTY \r\n", 7);
		
		memset(buf, '\0', 99);
		socket_gets(buf,sizeof(buf), remote_holder->inpipe);

		if(okay_or_not(buf) != 1) {
			LOG_ERROR1("Error getting okay message - %s\n", buf);
		} else {
			LOG_DEVDIAG1("%s ",buf);
		}
	}


}

void tydemux_close_tyserver(remote_holder_t * remote_holder) {

	if(remote_holder->sockfd != INVALID_SOCKET ) {
		socket_write(remote_holder->sockfd, "EXIT \r\n", 7);
	}
}























