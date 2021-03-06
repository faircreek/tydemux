#define CHUNK_SIZE (256*512)		/* Size of a chunk */
#define CHUNKSUNKNOWN	27176654	/* Magic number: indicates that */
					/* we don't know how many chunks */
					/* are in this stream */

#define INPUT_FSID	1		/* TiVo fsid on MFS, or from a */
#define INPUT_FILE	2		/* Unix file  */
#define INPUT_TYSERVER	3		/* Tyserver via TCP */
#define INPUT_TYSERVCHK	4		/* Chunks from tyserver via TCP */
/* This file is automatically generated with "make proto". DO NOT EDIT */

void get_start_chunk_tivo();
int get_index_time_tivo(gop_index_t * gop_index);
void print_index_tivo(int sockfd, index_t * index);
void create_index_tivo(int sockfd);
void print_probe_tivo(int sockfd);
void Exit(int num);
void exit_server(int sockfd);
void play_file(int sockfd, char *filename);
void list_recordings_tydemux(int sockfd);
void open_file(int sockfd, char *filename);
void open_file_tydemux(int sockfd, char *filename);
void send_chunk_tydemux(int sockfd, int chk);
void send_chunk(int sockfd, int chk);
void handle_request(int sockfd);
void usage(void);
int main(int argc, char *argv[]);
