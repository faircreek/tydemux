#include "common.h"
int main(int argc, char *argv[]) {

	int in_file, video_file, audio_file;

	off64_t in_file_size;
#if 0
	off64_t byte_offset;
	/* Chunk numbering */
	chunknumber_t chunk_nr;
	off64_t byte_offset;
#endif

	stat64_t file_info;

	/* Tystream */
	tystream_holder_t * tystream;

	/* Audio delay to return */
	int return_value;

	tystream = new_tystream(DEMUX);

	tystream->std_alone=1;

	if(!parse_args(argc, argv, tystream)) {
		exit(tydemux_return(-1000));
	}

		 /* Open the TY file */
	in_file = open(tystream->infile, O_RDONLY | OS_FLAGS);


	if(in_file == -1) {
		perror("Error reading tystream");
		exit(tydemux_return(-1000));
	}

	logger_init(log_info, stdout, 0, 0 );

	fstat64(in_file, &file_info);
	in_file_size = file_info.st_size;

	if(in_file_size < 5242880) {
		LOG_ERROR("TyStream is to small to demux\n" \
		"We need at least 40 chunks\n");
		exit(tydemux_return(-1000));
	}

	tystream_set_in_file(tystream, in_file, in_file_size);



	if(!std_probe_tystream(tystream)) {
		LOG_ERROR("Error in probe - exit\n");
		printf("Probe error\n");
		//exit(tydemux_return(-1000));
	}

	if(tystream->miss_alinged) {
		LOG_ERROR("\nYour stream is not aligned - please use -b to specify byte offset\n\n");
		//printf("\nYour stream is not aligned - please use -b to specify byte offset\n\n");
		exit(tydemux_return(-1000));
	}

	print_probe(tystream);

	get_start_chunk(tystream);

	if(tystream->write) {
		/* Open output files */

		video_file = open(tystream->video_out, O_WRONLY|O_CREAT|O_TRUNC|OS_FLAGS, READWRITE_PERMISSIONS);
		audio_file = open(tystream->audio_out, O_WRONLY|O_CREAT|O_TRUNC|OS_FLAGS, READWRITE_PERMISSIONS);


		if(video_file == -1 || audio_file == -1){
			perror("Error writing output");
			exit(tydemux_return(-1000));
		}
	} else {
		tystream->write = 0;
	}

	if(tystream->write) {
		tystream_set_out_files(tystream, audio_file, video_file);
	}

	/* Indexing test */

#ifdef TEST_INDEX
	print_index(tystream);
	free_tystream(tystream);
	exit(1);
#endif

	LOG_USERDIAG("Starting demux process\n");
#if !defined(TIVO)
	tydemux(tystream, NULL);
#else
	tydemux(tystream);
#endif

#if 0
	for(ul=0, chunk_nr = 0; ul < tystream->in_file_size/CHUNK_SIZE - tystream->start_chunk; ul++, chunk_nr++) {

		tystream->number_of_chunks = chunk_nr;

		if(debug >= 1 && debug < 3) {
			prog_meter(tystream);
		}

		get_chunk(tystream, chunk_nr);

		get_pes_holders(tystream);

		if(!tystream->find_seq) {
			tystream_init(tystream);
		}

		if(tystream->repair) {
			repair_tystream(tystream);
		}


		check_fix_pes_holder_video(tystream);

		write_pes_holders(tystream);

	}
#endif
	/* FIXME we loose chunks plus video and audio pes holders */
	LOG_USERDIAG("\nDemux process finished\n");
	return_value = print_audio_delay(tystream);
	free_tystream(tystream);
	LOG_USERDIAG("\n");

	logger_free();

	return(tydemux_return(return_value));

}


