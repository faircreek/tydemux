/*
 *
 * (c) 2003, 2015 Olaf Beck olaf_sc@yahoo,com
 *
 * Released under the Gnu General Public License version 2
 */

void get_start_chunk_tivo(tystream_holder_t * tystream);
int get_index_time_tivo(tystream_holder_t * tystream, gop_index_t * gop_index);
void create_index_tivo(tystream_holder_t * tystream);
int open_file_tydemux(tystream_holder_t * tystream, char *filename);
void usage(void);
int make_index(fsid_index_t * fsid_index);
dir_index_t * remove_old_files(fsid_index_t * fsid_index);
int file_present(dir_index_t * dir_index, fsid_index_t * fsid_index);
int create_index_files(fsid_index_t * fsid_index,dir_index_t * dir_index, int files);
int index_present(fsid_index_t * fsid_index, dir_index_t * dir_index);
int write_now_showing(fsid_index_t * fsid_index);

