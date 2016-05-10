

int open_socket_to_tyserver(char *inhost, int port);
int read_a_chunk(int fd, int chunknum, char *bptr);
int read_a_chunkX(int fd, int chunknum, char *bptr, int number);
int open_input(char *inputsource, int play_only);
void close_input() ;
fsid_index_t * parse_nowshowing_tivo();
