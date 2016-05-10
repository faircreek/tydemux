/*
 * playitsam: Play, edit & save a TiVo video stream from file or MFS.
 *
 * readfile.c: Routines to read chunks from various inputs. $Revision: 1.12 $
 *
 * (c) 2002, Warren Toomey wkt@tuhs.org.
 * (c) 2003, 2015 Olof <jinxolina@gmail.com>
 * (c) 2003, 2015 Copyright (C) 2003 Christopher R. Wingert
 *
 * Released under the Gnu General Public License version 2
 */


#include <sys/wait.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include "../libs/libmfs/mfs.h"

#include "tycommon.h"
#include "../tydemux/tydemux.h"


//#include "../libs/libmfs/mfs.h"


#define CHUNKSUNKNOWN	27176654	/* Magic number: indicates that */
					/* we don't know how many chunks */
					/* are in this stream */

#define INPUT_FSID	1		/* TiVo fsid on MFS, or from a */
#define INPUT_FILE	2		/* Unix file  */
#define INPUT_TYSERVER	3		/* Tyserver via TCP */
#define INPUT_TYSERVCHK	4		/* Chunks from tyserver via TCP */
#define MAX_FSID_LIST 50

/* playitsam.c now assumes that chunk 0 from every input file contains
 * valid data. For FSIDs, we now ignore chunk 0 always. For other
 * files, we first read chunk 0 and if its is garbage, we set a seek_offset
 * to CHUNK_SIZE to ensure that future seeks wil work.
 */


#ifdef TIVO
/*
 * For multi-fsid recordings, we store the fsids and their sizes here.
 */
struct fsid_group {
  int fsid;
  int start_chunk;
  int end_chunk;
}  *Fsid_list = NULL;

int Fsid_list_count = 0;

#endif
static fsid_index_t * parse_nowshowing(char * path, int zone);

extern int totalchunks;		/* Total chunks */
int inputtype = 0;		/* Type of file input */
off_t seek_offset = 0;		/* Seek needed to skip useless chunk 0 */
int MFS_INIT=0;


#ifdef TIVO
/*
 * Read from a single fsid or from a set of fsids. Always add 1 so as to skip
 * the chunk #0 in every fsid stream.
 */
static int mfs_multifsid_pread_X(int fsid, char *buf, int chunknum, int chunks, int bytes_read)
{
  int i;
  char * pt1;
  int bytes_read_this_session;

  /* If we only have one fsid, use the one supplied */
  if (Fsid_list_count == 0) {
	if(totalchunks >= chunknum + chunks) {
    		return (mfs_fsid_pread_fast(fsid, buf, ((u64) chunknum + 1) << 17, CHUNK_SIZE * chunks));
    	} else {
		return (-1);
	}
  }

  /* Otherwise, search for an fsid that matches and use it */
  for (i = 0; i < Fsid_list_count; i++) {
    if ((chunknum >= Fsid_list[i].start_chunk) &&
	(chunknum <= Fsid_list[i].end_chunk)) {
      /* fprintf(stderr,"Reading from fsid %d\r\n",Fsid_list[i].fsid); */
	if(Fsid_list[i].end_chunk >= chunknum + chunks) {
		bytes_read_this_session = mfs_fsid_pread_fast(Fsid_list[i].fsid, buf,
			    ((u64) (1 + chunknum - Fsid_list[i].start_chunk)) << 17,
			     CHUNK_SIZE * chunks);
		return(bytes_read_this_session + bytes_read);
	} else {
		if(chunknum + chunks <= totalchunks) {
			int leftover;
			int size;
			size = CHUNK_SIZE * (Fsid_list[i].end_chunk - chunknum);
			leftover = chunknum + chunks - Fsid_list[i].end_chunk;

			if((bytes_read_this_session = mfs_fsid_pread_fast(Fsid_list[i].fsid, buf,
				    ((u64) (1 + chunknum - Fsid_list[i].start_chunk)) << 17,
				     CHUNK_SIZE * (Fsid_list[i].end_chunk - chunknum))) != -1) {

				return(mfs_multifsid_pread_X(fsid, buf + size, Fsid_list[i + 1].start_chunk, leftover,bytes_read_this_session +  bytes_read));
			} else {
				return(-1);
			}
		} else {
			return(-1);
		}
	}



    }
  }

  return (-1);			/* No match */
}


static int mfs_multifsid_pread(int fsid, char *buf, int chunknum)
{
  int i;

  /* If we only have one fsid, use the one supplied */
  if (Fsid_list_count == 0) {
    return (mfs_fsid_pread_fast(fsid, buf, ((u64) chunknum + 1) << 17, CHUNK_SIZE));
    //return(mfs_read_chunk(fsid, buf, 1 + chunknum - Fsid_list[i].start_chunk, CHUNK_SIZE));
  }

  /* Otherwise, search for an fsid that matches and use it */
  for (i = 0; i < Fsid_list_count; i++) {
    if ((chunknum >= Fsid_list[i].start_chunk) &&
	(chunknum <= Fsid_list[i].end_chunk)) {
      /* fprintf(stderr,"Reading from fsid %d\r\n",Fsid_list[i].fsid); */
      return (mfs_fsid_pread_fast(Fsid_list[i].fsid, buf,
		    ((u64) (1 + chunknum - Fsid_list[i].start_chunk)) << 17,
			     CHUNK_SIZE));
      //return(mfs_read_chunk(Fsid_list[i].fsid, buf, 1 + chunknum - Fsid_list[i].start_chunk, CHUNK_SIZE));
    }
  }
  return (-1);			/* No match */
}


#endif


char *amorpm[] =
{ "am", "pm" };

char *dayofweek[] =
{ "Sun", "Mon", "Tues", "Wed", "Thur", "Fri", "Sat" };

int zone_list[] =
{-5, -6, -7, -8, -9, -10, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, -1, -2, -3, -4, -11, -12};


void fsidtoparts( char *fsid, char *sliceIds[], int *max )
{
   struct mfs_obj_attr *obj;
   int                 pcount;
   int                 index;
   int                 ifsid;
   char                calcFsid[ 80 ];
   int                 iSliceId;
   char                sSliceId[ 16 ];

   ifsid = mfs_resolve( fsid );

   obj = query_object( ifsid, "Part", &pcount );
   for( index = 0 ; index < pcount ; index ++ )
   {
      sprintf( calcFsid, "Part/%d/File", obj[ index ].subobj );
      iSliceId = query_int( ifsid, calcFsid );
      sprintf( sSliceId, "%d", iSliceId );
      sliceIds[ index ] = malloc( strlen( sSliceId ) );
      strcpy( sliceIds[ index ], sSliceId );
   }
   free( obj );
   *max = pcount;
}


static int get_zone(int zone, int daylight) {

	int tz;

	if(zone <= 0) {
		return(0);
	}

	if(daylight == -1) {
		daylight = 0;
	}

	tz = zone_list[zone -1];
	if(daylight) {
		tz++;
	}

	return(tz * 60 * 60);

}



/* open a socket to a tcp remote host with the specified port */
int open_socket_to_tyserver(char *inhost, int port)
{
  int type = SOCK_STREAM;
  struct sockaddr_in sock_out;
  int res;
  int nodelay = 1;
  int window = 65535;
  char *host, *numptr, *numend;
  u_int32_t raw_address = 0;
  int one_part;

  host = strdup(inhost);
  if (host == NULL) return (-1);

  res = socket(PF_INET, type, 0);
  if (res == -1) { free(host); return (-1); }

#ifdef CAN_DO_NAME_LOOKUPS
  /* Can someone tell me how to get /etc/hosts working on the TiVo? */
  hp = gethostbyname(host);
  if (!hp) {
    fprintf(stderr, "unknown host: %s\n", host);
    return (-1);
  }
  memcpy(&sock_out.sin_addr, hp->h_addr, hp->h_length);

#else

  /* Walk the IP address and convert into a 32-bit integer */
  numptr = host;
  numend = strchr(host, '.');
  if (numend == NULL) {
    fprintf(stderr, "%s not an IP address\n", inhost);
    free(host); return (-1);
  }
  *numend = '\0';
  one_part = atoi(numptr);
  if ((one_part < 0) || (one_part > 255)) {
    fprintf(stderr, "%s not an IP address\n", inhost);
    free(host); return (-1);
  }
  raw_address = one_part << 24;

  numptr = ++numend;
  numend = strchr(numptr, '.');
  if (numptr == NULL) {
    fprintf(stderr, "%s not an IP address\n", inhost);
    free(host); return (-1);
  }
  *numend = '\0';
  one_part = atoi(numptr);
  if ((one_part < 0) || (one_part > 255)) {
    fprintf(stderr, "%s not an IP address\n", inhost);
    free(host); return (-1);
  }
  raw_address += (one_part << 16);

  numptr = ++numend;
  numend = strchr(numptr, '.');
  if (numptr == NULL) {
    fprintf(stderr, "%s not an IP address\n", inhost);
    free(host); return (-1);
  }
  *numend = '\0';
  one_part = atoi(numptr);
  if ((one_part < 0) || (one_part > 255)) {
    fprintf(stderr, "%s not an IP address\n", inhost);
    free(host); return (-1);
  }
  raw_address += (one_part << 8);
  numptr = ++numend;
  one_part = atoi(numptr);
  if ((one_part < 0) || (one_part > 255)) {
    fprintf(stderr, "%s not an IP address\n", inhost);
    free(host); return (-1);
  }
  raw_address += one_part;
  sock_out.sin_addr.s_addr = htonl(raw_address);
#endif

  sock_out.sin_port = htons(port);
  sock_out.sin_family = PF_INET;

  if (connect(res, (struct sockaddr *) & sock_out, sizeof(sock_out))) {
    close(res);
    fprintf(stderr, "failed to connect to %s\n", inhost);
    free(host); return (-1);
  }
  setsockopt(res, SOL_SOCKET, SO_RCVBUF, &window, sizeof(int));
  setsockopt(res, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int));

  free(host); return (res);
}

/*
 * Read a specific chunk from the input stream. Return 1 if ok, 0 if failure
 */
int read_a_chunkX(int fd, int chunknum, char *bptr, int number)
{
  int numread = 0;

    numread = mfs_multifsid_pread_X(fd, bptr, chunknum, number, 0);
    if (numread != CHUNK_SIZE * number) {
      	fprintf(stderr, "Failure reading chunk %d\r\n", chunknum);
      	return(0);
    }
    return (1);
}



int read_a_chunk(int fd, int chunknum, char *bptr)
{
  int numread = 0;
  int numleft = CHUNK_SIZE;
  char ch, buf[100];

  switch (inputtype) {
#ifdef TIVO
  case INPUT_FSID:
    numread = mfs_multifsid_pread(fd, bptr, chunknum);
    if (numread != CHUNK_SIZE) {
      fprintf(stderr, "Failure reading chunk %d\r\n", chunknum);
      return(0);
    }
    return (1);
#endif

  case INPUT_FILE:
    lseek(fd, seek_offset + (CHUNK_SIZE * chunknum), SEEK_SET);
    goto here;				/* Yuk, a goto */

  case INPUT_TYSERVCHK:
	/* We need to send a CHNK command, get the answer, and then
	 * read the 128K of chunk from the TCP connection.
	 */
      sprintf(buf, "CHNK %d\r\n", chunknum);
      numread = write(fd, buf, strlen(buf));
      if (numread < 1) {
	fprintf(stderr, "Failure sending CHNK command\n");
	return(0);
      }
      /* Read the 3-digit result */
      read(fd, buf, 3);
      buf[3] = '\0';
      if (strcmp(buf, "200")) { /* Did not get ok message */
        close(fd);
        fprintf(stderr, "Couldn't read chunk from tyserver\n");
        return(0);
      }
      /* Otherwise, read up to \n from tyserver before we get the chunk */
      while (1) {
        read(fd, &ch, 1);
        if (ch == '\n') break;
      }

	/* Fall into the read 128K below */

here:
  case INPUT_TYSERVER:

    /* Loop until we get CHUNK_SIZE bytes of data */
    while (numleft > 0) {
      numread = read(fd, bptr, numleft);
      if (numread < 1) {
	fprintf(stderr, "Failure Reading chunk %d\r\n", chunknum);
	return(0);
      }
      numleft -= numread;
      bptr += numread;
    }
    return (1);
  default:
    return (0);
  }
}


/*
 * Given the input argument from the user, work out which one it is and open
 * it. Returns a file descriptor to the input, or a fsid if the input is on
 * MFS. If play_only is true, then we open tyserver streams with PLAY, and
 * we don't use the CHNK command - NOTE you must have done a mfs_init at this point!!
 */

int open_input(char *inputsource, int play_only)
{
	int fsid;
	int i;
	char *onefsid;

	int fd;
	struct stat statbuf;
	char ch, *tyfile;
	char result[4];
	unsigned char buf[4];




	/**** BIG CHANGE FROM PREVOUS VERSIONS ****/
	/* We now always ignore chunk 0 from every fsid stream */

	/* inputsource could be a single fsid or a list of fsids */
	/* First, check if it has commas in it */
	/* If so, split the list into fsids, store them */
	/* and work out their size */
	if (strchr(inputsource, ',') || atoi(inputsource) != 0) {

		if(!Fsid_list) {
			/* Malloc room for MAX_FSID_LIST entries */
			Fsid_list = (struct fsid_group *) malloc(
						MAX_FSID_LIST * sizeof(struct fsid_group));
			printf("Malloc the FSID \n");
			if (Fsid_list == NULL) {
				fprintf(stderr, "Can't malloc fsid list\n");
				return(0);
			}
			if(!MFS_INIT) {
				mfs_init();
				MFS_INIT=1;
			}
		} else {
			Fsid_list_count = 0;
			seek_offset = 0;
			/* reset all FSID numbers */
			for(i=0; i < MAX_FSID_LIST; i++) {
				Fsid_list[i].fsid = 0;
				Fsid_list[i].start_chunk = 0;
				Fsid_list[i].start_chunk = 0;
			}
		}



		totalchunks = 0;
		fsid = 0;
		//printf("Input source: %s\n",inputsource);
		if(strchr(inputsource, ',')) {
			/* Extract each fsid */
			for (i=0;(i<MAX_FSID_LIST)&&((onefsid=strsep(&inputsource,","))!=NULL);i++){
				fsid = atoi(onefsid);
				if (fsid <= 0) {
					continue;
				}

			Fsid_list[i].fsid = fsid;
			//printf(":%d:\n", fsid);
			Fsid_list[i].start_chunk = totalchunks;
			totalchunks += mfs_fsid_numchunks(fsid) - 2;   /* Don't count chunk #0 */
									/* Nor last chunk */
			Fsid_list[i].end_chunk = totalchunks - 1;
			Fsid_list_count++;
#if 1
			fprintf(stderr, "fsid %d has %d chunks [%d to %d]\r\n", fsid, totalchunks,
				Fsid_list[i].start_chunk, Fsid_list[i].end_chunk);
#endif
			}

			inputtype = INPUT_FSID;
    			return (fsid);
    		} else {

			/* Okay a single fsid */
			fsid = atoi(inputsource);
			if (fsid <= 0) {
				return(0);
			}

			Fsid_list[0].fsid = fsid;
			Fsid_list[0].start_chunk = totalchunks;
			totalchunks += mfs_fsid_numchunks(fsid) - 2;   /* Don't count chunk #0 */
									/* Nor last chunk */
			Fsid_list[0].end_chunk = totalchunks - 1;
			Fsid_list_count++;
			inputtype = INPUT_FSID;
			return (fsid);
		}
	}

	return(0);
}


fsid_index_t * parse_nowshowing_tivo() {


	char *path;
	fsid_index_t * fsid_index_list=NULL;
	int fsid;
	int fsid2;
	char 	txtbuf[100];
	int version = 0;
	int zone;
	int daylight;
	
	if(!MFS_INIT) {
		mfs_init();
		MFS_INIT = 1;
	}

	if ( mfs_resolve( "/Recording/NowShowingByClassic" ) != 0 ) {
		//printf("Classic\n");
		path = "/Recording/NowShowingByClassic";

	} else if (mfs_resolve( "/Recording/NowShowing") != 0) {
		//printf("Normal\n");
		path = "/Recording/NowShowing";

	} else {
		//printf("Complete\n");
		path = "/Recording/Complete";
	}

	fsid = mfs_resolve("/SwSystem/ACTIVE");

	memset(txtbuf, '\0', 99);
	sprintf(txtbuf ,"%s", query_string(fsid, "Name"));

	if(txtbuf[0] == '3') {
		version++;
	}

	if(version) {
		fsid2 = mfs_resolve("/State/LocationConfig");
		zone = query_int(fsid2, "TimeZoneOld");
		daylight = query_int(fsid2, "DaylightSavingsPolicy");
	} else {
		fsid2 = mfs_resolve("/Setup");
		zone = query_int(fsid2, "TimeZone");
		daylight = query_int(fsid2, "DaylightSavingsPolicy");
	}
	zone = get_zone(zone, daylight);

	fsid_index_list = parse_nowshowing(path, zone);
	return(fsid_index_list);
}

static fsid_index_t * parse_nowshowing(char * path, int zone) {

	struct mfs_dirent	*dir;
	u32			count;
	int			index;
	u32			i;
	time_t			recTime;
	struct tm		recTimetm;
	struct tm		airTimetm;
	int			ampm = 0;
	char			*sliceIds[ 32 ];
	int			numSlices;
	char			fsid[ 32 ];
	int			duration, duration_m, duration_h;
	int			state;
	char 			txtbuf[600];
	char			*pt1;
	int 			size;

	fsid_index_t * fsid_index_list=NULL;
	fsid_index_t * fsid_index;
	struct stat fileinfo;

	dir = mfs_dir(mfs_resolve(path), &count);

	for ( i = 0 ; i < count ; i++ ) {

		state = query_int(dir[i].fsid, "State");

		/* The recoding is done so parse the info */
		if(state == 4) {

			fsid_index=new_fsid_index();

			recTime = (query_int(dir[i].fsid, "StartDate") * 86400);
			recTime += query_int(dir[i].fsid, "StartTime") + 60;
			recTime += zone;
			localtime_r( &recTime, &recTimetm );
			recTimetm.tm_year += 1900;
			ampm = 0;

			if ( recTimetm.tm_hour > 12 ) {
				ampm = 1;
				recTimetm.tm_hour -= 12;
			}


			recTime = (query_int(dir[i].fsid, "Showing/Program/OriginalAirDate") * 86400);
			localtime_r( &recTime, &airTimetm );
			airTimetm.tm_year += 1900;

			/* The REC FSID */
			add_fsid(query_int(dir[i].fsid, "Part"), fsid_index);
			sprintf( fsid, "%d", query_int( dir[ i ].fsid, "Part" ) );

			/* The Tystream FILE NAME */
			fsidtoparts( fsid, sliceIds, &numSlices );
			pt1 = txtbuf;
			memset(txtbuf, '\0', 599);

			for( index = 0 ; index < numSlices ; index++ ) {

				size = strlen(sliceIds[index]);
				strncpy(pt1,sliceIds[index],size);
				pt1 = pt1 + size;

				if (index != ( numSlices - 1 )) {
					pt1[0]=',';
					pt1++;
				}
				free( sliceIds[ index ] );
			}
			add_tystream(txtbuf, fsid_index);


			/* The State */
			add_state(state,fsid_index);

			/* YEAR */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf, "%d", recTimetm.tm_year);
			if(strcmp("(null)", txtbuf)) {
				add_year(txtbuf, fsid_index);
			}

			/* Airdate */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf , "%d/%d/%d", airTimetm.tm_mon + 1, airTimetm.tm_mday, airTimetm.tm_year );
			if(strcmp("(null)", txtbuf)) {
				add_air(txtbuf, fsid_index);
			}

			/* Day */
			add_day(dayofweek[recTimetm.tm_wday],fsid_index);

			/* Date */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf , "%d/%d", recTimetm.tm_mon + 1, recTimetm.tm_mday );
			if(strcmp("(null)", txtbuf)) {
				add_date(txtbuf, fsid_index);
			}

			/* Time */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf , "%d:%2.2d %s", recTimetm.tm_hour, recTimetm.tm_min, amorpm[ ampm ] );
			if(strcmp("(null)", txtbuf)) {
				add_rectime(txtbuf, fsid_index);
			}

			/* Duration */
			duration = query_int(dir[i].fsid, "StopDate") -
				query_int(dir[i].fsid, "StartDate");
			duration *= 86400;

			duration += ( query_int(dir[i].fsid, "StopTime") -
				query_int(dir[i].fsid, "StartTime"));
			memset(txtbuf, '\0', 599);
			duration_h = duration / 3600;
			duration_m = (duration % 3600)/ 60;
			sprintf(txtbuf , "%d:%02d", duration_h, duration_m );
			if(strcmp("(null)", txtbuf)) {
				add_duration(txtbuf, fsid_index);
			}

			/* Title */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf , "%s", query_string(dir[i].fsid, "Showing/Program/Title"));
			if(strcmp("(null)", txtbuf)) {
				add_title(txtbuf, fsid_index);
			}

			/* Episode */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf, "%s", query_string(dir[i].fsid, "Showing/Program/EpisodeTitle"));
			if(strcmp("(null)", txtbuf)) {
				add_episode(txtbuf, fsid_index);
			}

			/* Episode Number */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf, "%d", query_int(dir[i].fsid, "Showing/Program/EpisodeNum"));
			if(strcmp("-1", txtbuf)) {
				add_episodenr(txtbuf, fsid_index);
			}

			/* Description */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf, "%s", query_string(dir[i].fsid, "Showing/Program/Description"));
			if(strcmp("(null)", txtbuf)) {
				add_description(txtbuf, fsid_index);
			}

			/* Actors */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf, "%s", query_string(dir[i].fsid, "Showing/Program/Actor"));
			if(strcmp("(null)", txtbuf)) {
				add_actors(txtbuf, fsid_index);
			}

			/* Guest stars */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf, "%s", query_string(dir[i].fsid, "Showing/Program/GuestStar"));
			if(strcmp("(null)", txtbuf)) {
				add_gstar(txtbuf, fsid_index);
			}

			/* Host */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf, "%s", query_string(dir[i].fsid, "Showing/Program/Host"));
			if(strcmp("(null)", txtbuf)) {
				add_host(txtbuf, fsid_index);
			}

			/* Director */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf, "%s", query_string(dir[i].fsid, "Showing/Program/Director"));
			if(strcmp("(null)", txtbuf)) {
				add_director(txtbuf, fsid_index);
			}

			/* Exec Prod */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf, "%s", query_string(dir[i].fsid, "Showing/Program/ExecProducer"));
			if(strcmp("(null)", txtbuf)) {
				add_eprod(txtbuf, fsid_index);
			}

			/* Producer */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf, "%s", query_string(dir[i].fsid, "Showing/Program/Producer"));
			if(strcmp("(null)", txtbuf)) {
				add_prod(txtbuf, fsid_index);
			}

			/* Writer */
			memset(txtbuf, '\0', 599);
			sprintf(txtbuf, "%s", query_string(dir[i].fsid, "Showing/Program/Writer"));
			if(strcmp("(null)", txtbuf)) {
				add_writer(txtbuf, fsid_index);
			}

			/* Index present or not */
			memset(txtbuf, '\0', 599);
			snprintf(txtbuf, 598,"/var/index/index/%i", fsid_index->fsid);
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

			fsid_index_list = add_fsid_index(fsid_index_list, fsid_index);

		}
	}

	if(dir) {
		mfs_dir_free(dir);
	}
	return(fsid_index_list);
}

