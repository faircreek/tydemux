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

#include "common.h"
#include "global.h"
#ifdef DALI
/* Dali */
#include <dvmbasic.h>
#include <dvmmpeg2.h>
#include <dvmcolor.h>
#include <dvmpnm.h>
#endif /* DALI */
#ifdef MPEG2DEC
#include "mpeg2.h"
#include "convert.h"
#endif



int set_seq_low_delay(tystream_holder_t * tystream, module_t * module) {


	//offset = find_extension_in_payload(tystream, payload, (uint8_t)0x01);

	/* Marker */
	int gotit;

	/* Sizes and offsets */
	int buffer_size;
	int offset;

	/* Buffer */
	uint8_t * buffer;

	/* Pointers to do magic with */
	uint8_t * pt1;

	/* Extensions */
	uint8_t found_extension;

	/* Init */
	buffer = module->data_buffer;
	buffer_size = module->buffer_size;
	offset = 0;

	pt1 = buffer;
	gotit = 0;

	while(offset != -1) {

		offset = find_start_code_offset_in_buffer(MPEG_EXT, buffer_size, pt1, tystream);
		if(offset != -1) {
			/* Okay lets find out if this is the right extension */
			found_extension = pt1[offset + 4] >> 4;
			if(found_extension == 0x01) {
				/* It was */
				gotit = 1;
			} else {
				pt1 = pt1 + offset + 1;
				buffer_size = buffer_size - ((pt1 - buffer) -1);
			}
		} else {
			break;
		}
		if(gotit) {
			break;
		}
	}


	if(!gotit) {
		return(0);
	}

	pt1 = module->data_buffer;
	pt1 = pt1 + offset + 4;

	setbit(pt1, 40, 1);

	return(1);

}





gop_index_t * new_gop_index() {


	gop_index_t * gop_index;

	gop_index = (gop_index_t *)malloc(sizeof(gop_index_t));
	memset(gop_index, 0,sizeof(gop_index_t));
	
	gop_index->gop_number=0;
	gop_index->chunk_number_seq = -1;
	gop_index->seq_rec_nr = -1;
	gop_index->chunk_number_i_frame_pes = -1;
	gop_index->i_frame_pes_rec_nr = -1;
	gop_index->chunk_number_i_frame = -1;
	gop_index->i_frame_rec_nr = -1;
	gop_index->time_of_iframe = 0;
	gop_index->display_time = 0;
	gop_index->reset_of_pts=0;

	gop_index->next = NULL;
	gop_index->previous = NULL;

	return(gop_index);
}


int seq_preset_in_chunk(tystream_holder_t * tystream, chunknumber_t chunk_nr, gop_index_t * gop_index, int i_frame) {

	/* Interation */
	int i;

	/* Chunk header 2.x and up*/
	uint8_t records_nr_1;
	uint8_t records_nr_2;
	uint8_t seq_start_1;
	uint8_t seq_start_2;
	uint16_t records_nr;
	uint16_t seq_start;
	uint16_t test_start;

	/* Variables for seeking
	the start of the chunk */
	off64_t seek_pos;
	off64_t byte_offset;

	/* Chunk related */
	record_header_t * record_header;
	chunk_t * chunk;

	int total_record_size;
	int bad_chunk;
	int return_value;


	vstream_t * vstream;


	/* Init chunk */

	byte_offset = tystream->byte_offset;

#if !defined(TIVOSERVER)
	seek_pos = chunk_nr + tystream->start_chunk;
	seek_pos *= CHUNK_SIZE;
	seek_pos = seek_pos + byte_offset;
#else
	seek_pos = 0;
#endif
	vstream = tystream->vstream;

	if(!vstream) {
#ifdef WIN32
		if( lseek64(tystream->in_file, seek_pos, SEEK_SET) == -1L ) {
			LOG_ERROR("Premature end of file detected\n");
			return(0);  // Seek failed
		}
#else
		if( lseek64(tystream->in_file, seek_pos, SEEK_SET) <= -1 ) {
			LOG_ERROR("File seek failed\n");
			return(0);  // Seek failed
		}
#endif
	} else {
		if(tydemux_seek_vstream(vstream, (int)0, SEEK_SET) == -1) {
			LOG_ERROR("File seek failed\n");
			return(0);  // Seek failed
		}
	}


	if(!vstream) {
		read(tystream->in_file, &records_nr_1, sizeof(uint8_t));
		read(tystream->in_file, &records_nr_2, sizeof(uint8_t));
		read(tystream->in_file, &seq_start_1, sizeof(uint8_t));
		read(tystream->in_file, &seq_start_2, sizeof(uint8_t));
	} else {
		tydemux_read_vstream(&records_nr_1, sizeof(uint8_t), 1, vstream);
		tydemux_read_vstream(&records_nr_2, sizeof(uint8_t), 1, vstream);
		tydemux_read_vstream(&seq_start_1, sizeof(uint8_t), 1, vstream);
		tydemux_read_vstream(&seq_start_2, sizeof(uint8_t), 1, vstream);
	}



	records_nr = ((uint16_t)records_nr_2 << 8) + records_nr_1;
	seq_start = ((uint16_t)seq_start_2 << 8) + seq_start_1;

	test_start = seq_start & 0x8000;
	if(test_start != 0x8000) {
		LOG_ERROR1("seq_preset_in_chunk: chunk " I64FORMAT " - bad chunk header - skipping\n", chunk_nr);
		return(-1);
	}

	seq_start = seq_start & 0x7fff;

	if (seq_start == 0x7fff && !i_frame) {
		/* No seq so return since we don't seek the i-frame*/
		return(0);
	} else if (seq_start == 0x7fff && i_frame) {
		seq_start = 0;
	}

	/* Now we actually need to check if this is a valid chunk - we don't
	want to index an invalid one */

	chunk = (chunk_t *)malloc(sizeof(chunk_t));
	memset(chunk, 0,sizeof(chunk_t));
	chunk->nr_records = records_nr;
	chunk->chunk_number = chunk_nr;

	if(chunk->nr_records > MAX_RECORDS) {
		LOG_ERROR1("seq_preset_in_chunk: chunk " I64FORMAT " - over flow in records - skipping\n", chunk_nr);
		free(chunk);
		return(-1);
	}

	record_header = read_record(chunk, tystream->in_file, vstream);
	chunk->record_header = record_header;

	total_record_size = 0;


	for(i=0; i < chunk->nr_records; i++) {
		total_record_size = total_record_size + record_header[i].size;
	}

	/* Check is the chunk is too big */
	if((total_record_size + (chunk->nr_records * 16) + 4) > CHUNK_SIZE) {
		LOG_ERROR1("seq_present_in_chunk: chunk " I64FORMAT " - chunk is to big - skipping\n", chunk_nr);
		for(i=0; i < chunk->nr_records; i++) {
			if(record_header[i].extended_data != NULL) {
				free(record_header[i].extended_data);
			}
		}
		free(record_header);
		free(chunk);
		return(-1);
	}

	/* Check if chunk is to small */
	if((total_record_size + (chunk->nr_records * 16) + 4) < CHUNK_SIZE/10) {
		LOG_ERROR1("seq_present_in_chunk: chunk " I64FORMAT " - chunk is to small - skipping\n", chunk_nr);
		for(i=0; i < chunk->nr_records; i++) {
			if(record_header[i].extended_data != NULL) {
				free(record_header[i].extended_data);
			}
		}
		free(record_header);
		free(chunk);
		return(-1);
	}


	bad_chunk = 0;

	for(i=0; i < chunk->nr_records; i++){
		switch(chunk->record_header[i].type) {
			case 0x000: /* Good knows what this record is*/
			case 0x2c0: /* Audio Data */
			case 0x3c0: /* Audio PES - MPEG */
			case 0x4c0: /* Audio Data */
			case 0x9c0: /* Audio PES - AC3 */
			case 0x2e0: /* Video Data */
			case 0x6e0: /* Video PES */
			case 0x7e0: /* Video SEQ */
			case 0x8e0: /* Video I Frame/Field */
			case 0xae0: /* Video P Frame/Field */
			case 0xbe0: /* Video B Frame/Field */
			case 0xce0: /* Video GOP */
			case 0xe01: /* CC  (ClosedCaptions) */
			case 0xe02: /* XDS (Extended Data Service)*/
			case 0xe03: /* Ipreview */
			case 0xe05: /* TT (TeleText) ??*/
			case 0xe04:  /* ?? UK record ?? */
			case 0xe06:
				LOG_DEVDIAG3("seq_present_in_chunk: chunk " I64FORMAT " record: %i - junk record type: %03x\n", \
						chunk->chunk_number, i, chunk->record_header[i].type);
				break;
			default:
				LOG_ERROR3("seq_present_in_chunk: chunk " I64FORMAT " record: %i - junk record type: %03x  - skipping\n", \
						chunk->chunk_number, i, chunk->record_header[i].type);
				
				bad_chunk = 1;
				break;

		}
	}


	if(bad_chunk) {
		for(i=0; i < chunk->nr_records; i++) {
			if(record_header[i].extended_data != NULL) {
				free(record_header[i].extended_data);
			}
		}
		free(record_header);
		free(chunk);
		return(-1);
	}

	bad_chunk = 0;

	for(i=0; i < chunk->nr_records; i++){
		if(chunk->record_header[i].type == tystream->wrong_audio) {
			LOG_ERROR2("seq_present_in_chunk: " I64FORMAT " Record %i - wrong type of audio - skipping\n", chunk->chunk_number, i);
			bad_chunk++;
			break;
		}
	}

	if(bad_chunk) {
		for(i=0; i < chunk->nr_records; i++) {
			if(record_header[i].extended_data != NULL) {
				free(record_header[i].extended_data);
			}
		}
		free(record_header);
		free(chunk);
		return(-1);
	}

	if(i_frame) {
		return_value = 0;
	} else {
		return_value = 1;
	}

	/* Okay so far lets see if we can find the I-Frame/PES header - FIXME adapt to S2*/

	for(i = seq_start; i < chunk->nr_records; i++){
		switch(chunk->record_header[i].type) {
			case 0x8e0: /* Video I Frame/Field */
				if(gop_index->i_frame_rec_nr == -1) {
					gop_index->i_frame_rec_nr = i;
					gop_index->chunk_number_i_frame = chunk_nr;
					return_value++;
				}
				break;
			case 0x6e0: /* Video PES */
				if(gop_index->i_frame_rec_nr == -1) {
					gop_index->i_frame_pes_rec_nr = i;
					gop_index->chunk_number_i_frame_pes = chunk_nr;
					return_value++;
				}
				break;
		}
	}

	if(!i_frame) {
		gop_index->seq_rec_nr = seq_start;
		gop_index->chunk_number_seq = chunk_nr;
	}
	
	/*if(return_value < 3) {
		printf("Returvalue %i, chunk " I64FORMAT "\n", return_value, chunk_nr);
		//print_chunk(chunk);
	} else {
		printf("Returvalue %i, chunk " I64FORMAT "\n", return_value, chunk_nr);
	}
	*/
	/* We are okay - free the chunk and return */

	for(i=0; i < chunk->nr_records; i++) {
		if(record_header[i].extended_data != NULL) {
			free(record_header[i].extended_data);
		}
	}
	free(record_header);
	free(chunk);


	return(return_value);


}


void add_gop_index(index_t * index, gop_index_t * gop_index, int64_t gop_number) {

	/* Save gop index */
	gop_index_t * tmp_gop_index;

	gop_index->gop_number = gop_number;

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

	index->nr_of_gops++;

}

void free_index(index_t * index) {

	//int nr;
	gop_index_t * gop_index;
	gop_index_t * gop_index_to_free;

	//nr = 0;
	if(index->gop_index) {
		gop_index = index->gop_index;

		while(gop_index) {
			gop_index_to_free = gop_index;
			gop_index = gop_index->next;
			//printf("Free GOP %i\n", nr);
			//nr++;
			free(gop_index_to_free);
		}
	}
	//printf("Free Index\n");
	free(index);

}




void print_index(tystream_holder_t * tystream) {

#if !defined(TIVO)
	gop_index_t * gop_index;
	module_t * seq_module = NULL;

#ifdef REAL_WRITE
	FILE * outfile;
#endif
	char outname[100];
	int counter=0;

	module_t * image;

	tystream->index = create_index(tystream, 0, NULL, NULL);

	gop_index = tystream->index->gop_index;

	LOG_DEVDIAG("Init image \n");

	seq_module = init_image(tystream);

	LOG_DEVDIAG("Init image finished\n");


	while(gop_index) {
		sprintf(outname, "%05d.ppm", counter);
		LOG_DEVDIAG1("Writing image %s\n", outname);
		counter++;
		image = get_image(tystream, gop_index, seq_module);

		if(!image) {
			LOG_ERROR("Error in image fetch \n");
			return;
		}
#ifdef REAL_WRITE
		outfile = fopen(outname, "w");
		if(!outfile) {
			perror("");
			LOG_ERROR("Error opening file\n");
			return;
		}

		fwrite(image->data_buffer, 1, image->buffer_size, outfile);
		fclose(outfile);
#endif
		free_module(image);
		image = NULL;
		gop_index = gop_index->next;
	}
	if(seq_module) {
		free_module(seq_module);
	}

#endif

}

module_t * init_image(tystream_holder_t * tystream) {


	module_t * module = NULL;


	if(!tystream->index) {
		return(0);
	}

	if(!tystream->remote_holder) {
		module = (module_t *)malloc(sizeof(module_t));
		memset(module, 0,sizeof(module_t));
		module->data_buffer = NULL;
		module->buffer_size = 0;

		if(!get_first_seq(tystream, tystream->index->gop_index, module)) {
			free_module(module);
			return(0);
		}
	} else {
		module = tydemux_get_remote_seq(tystream->remote_holder, tystream);
		if(!module) {
			return(0);
		}
	}
	return(module);

}

#ifdef DALI
module_t * get_image(tystream_holder_t * tystream, gop_index_t * gop_index, module_t * init_image) {


	/* Dali pointers */
	BitStream * bs;
	BitParser * bp;
	BitParser *outbp;
	BitStream *outbs;
	Mpeg2SeqHdr *sh;
	Mpeg2PicHdr *fh;
	Mpeg2MBInfoArray * ia;
	ByteImage *r, *g, *b, *y, *u, *v;
	ScImage *scy, *scu, *scv;
	VectorImage *fwdmv, *bwdmv;
	PnmHdr *pnmhdr;

	int  len;
	int halfw, halfh, w, h, seqw, seqh, remw, remh, type, ext_type;

	int mb_h, mb_w, h_factor, v_factor, croma, cw, ch;


	/* Tydemux */

	module_t * module;
	module_t * return_module;


	char * tmp_buffer;
	char * pt1;

	int size;

	uint64_t  i;
	uint8_t x;
	uint8_t * oldbuffer;

	return_module = NULL;

	if(!tystream->index) {
		return(0);
	}

	module = (module_t *)malloc(sizeof(module_t));
	memset(module, 0,sizeof(module_t));
	module->data_buffer = NULL;
	module->buffer_size = 0;


	if(!get_image_and_time(tystream, gop_index, module)) {
		free(module);
		return(0);
	}


	set_temporal_reference(module->data_buffer, 0);

	size = module->buffer_size + init_image->buffer_size;
	tmp_buffer =(char *)malloc(sizeof(char) * size);
	memset(tmp_buffer, 0,sizeof(char) * size);
	pt1 = tmp_buffer;

	memcpy(pt1,init_image->data_buffer,init_image->buffer_size);
	pt1 = pt1 + init_image->buffer_size;
	memcpy(pt1,module->data_buffer,module->buffer_size);

	/* Init Dali */

	bs = BitStreamNew(size);
	bp = BitParserNew();



	memcpy(bs->buffer,tmp_buffer, size);
	bs->endDataPtr = bs->buffer + size;
	bs->endBufPtr = bs->buffer + size;

	/* Free tydemux buffers */
	free(tmp_buffer);
	free(module->data_buffer);
	free(module);


	BitParserWrap(bp, bs);

	/* Find the seq FIXME we should parse a new seq for every I-Frame*/

	sh = Mpeg2SeqHdrNew();
	Mpeg2SeqHdrParse(bp, sh);
	Mpeg2SeqHdrExtParse(bp, sh);

	/*
	* Find the width and height of the video frames.  If the width
	* and height are not multiple of 16, round it up to the next
	* multiple of 16.
	*/

	seqw = Mpeg2SeqHdrGetWidth(sh);
	seqh = Mpeg2SeqHdrGetHeight(sh);

	mb_w = Mpeg2SeqHdrGetMBWidth(sh);
	mb_h = Mpeg2SeqHdrGetMBHeight(sh);
	w = mb_w * 16;
	h = mb_h * 16;

	switch(Mpeg2SeqHdrGetChromaFormat(sh)) {
		case 420:
			cw = w/2;
			ch = h/2;
			h_factor = 2;
			v_factor = 2;
			croma = 1;
			//printf("420\n");
			break;
		case 422:
			cw = w/2;
			ch = h;
			h_factor = 2;
			v_factor = 1;
			croma = 0;
			//printf("422\n");
			break;
		case 444:
			cw = w;
			ch = h;
			h_factor = 1;
			v_factor = 1;
			//printf("444\n");	
			croma = 0;
			break;
	}

	/*
	 * Allocates all the ByteImages and ScImages that we need.
	 * y, u, v for decoded frame in YUV color space, r, g, b for
	 * decoded frame in RGB color space. prevy, prevu, prevv are
	 * past frames in YUV color space, futurey, futureu and futurev
	 * are future frames in YUV color space.  scy, scu and scv
	 * are the DCT coded images from the bitstream.  fwdmv and bwdmv
	 * are the forward and backward motion vectors respectively.
	 */

	ia = Mpeg2MBInfoArrayNew(mb_w,mb_h);


	scy     = ScNew (2 * mb_w, 2 * mb_h);
	scu     = ScNew ((2 * mb_w)/h_factor, (2 * mb_h)/v_factor);
	scv     = ScNew ((2 * mb_w)/h_factor, (2 * mb_h)/v_factor);

	y       = ByteNew (w, h);
	u       = ByteNew (w/h_factor, h/v_factor);
	v       = ByteNew (w/h_factor, h/v_factor);

	r       = ByteNew (seqw, seqh);
	g       = ByteNew (seqw, seqh);
	b       = ByteNew (seqw, seqh);




	/*
	 * Create a new PnmHdr and encode it to the BitStream.  We only do
	 * this once, since all frames have the same header.
	 */

	pnmhdr = PnmHdrNew();
	PnmHdrSetType(pnmhdr, PPM_BIN);
	PnmHdrSetWidth(pnmhdr, seqw);
	PnmHdrSetHeight(pnmhdr, seqh);
	PnmHdrSetMaxVal(pnmhdr, 255);
	outbs = BitStreamNew(3*(seqw+1) * (seqh+1)  + 20);
	oldbuffer = outbs->buffer;
	outbp = BitParserNew();
	BitParserWrap(outbp, outbs);
	PnmHdrEncode(pnmhdr, outbp);
	outbs->endBufPtr--;
	PnmHdrFree(pnmhdr);


	/*
	* Create a new Pic header, and advance the cursor to the next
	* pic header. (We are not interested in GOP header here).
	*/

	BitParserSeek(bp, 0);
	fh  = Mpeg2PicHdrNew();
	len = Mpeg2PicHdrFind(bp);


	/*
	 * Reads the pic header, and perform decoding according to the
	 * type.
	 */

	 Mpeg2PicHdrParse(bp, fh);
	 type = Mpeg2PicHdrGetPictureType(fh);

	 /* We only parse I Frames */

	if(type == I_FRAME) {
		if(Mpeg2PicHdrExtFind(bp) == -1) {
			LOG_ERROR("Didn't find Pic ext\n");
		}
		Mpeg2PicHdrExtParse(bp, fh);
		Mpeg2AnyStartCodeFind(bp);
		type = Mpeg2GetCurrStartCode(bp);

		while(type == 0x1b5 || type == 0x1b2) {
			Mpeg2SkipToNextStartCode(bp);
			type = Mpeg2GetCurrStartCode(bp);
		}

		if(type < 0x101 || type > 0x1af) {
			LOG_ERROR("Expected to find a Slice start code \n");
		}

		//printf("\nParse \n");
		Mpeg2ParseI(bp, sh, fh, scy, scu, scv, ia, 1);

		//printf("Byte 1  \n"); 
		Mpeg2ScIFrameToByte(scy, ia, y, 1, 1);
		//printf("Byte 2 \n"); 
		Mpeg2ScIFrameToByte(scu, ia, u, 0, 1);
		//printf("Byte 3 \n");
		Mpeg2ScIFrameToByte(scv, ia, v, 0, 1);
		//printf("YuvToRgb420 \n");
		YuvToRgb420(y, u, v, r, g, b);
		//printf("PpmEncode \n");
		PpmEncode(r, g, b, outbp);

		/* Copy data to our return Image */
		//printf("Copy Data size %i \n", outbs->size);
		if(outbs->endBufPtr < outbs->endDataPtr) {
			LOG_ERROR("Error\n");
			abort();
		}
		return_module = (module_t*)malloc(sizeof(module_t));
		memset(return_module,0,sizeof(module_t));
		return_module->data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * (outbs->size));
		memset(return_module->data_buffer, 0,sizeof(uint8_t) * (outbs->size));
		return_module->buffer_size = outbs->size;
		//printf("Copy Data - init\n");
		fflush(stdout);
		x = outbs->endDataPtr[0];
		//printf("Copy Data - init\n");
		fflush(stdout);
		x = outbs->endBufPtr[0];
		fflush(stdout);
		//printf("Copy Data - loop \n");
		fflush(stdout);
		//for(i=0; i < outbs->size; i++) {
			//printf("" I64FORMAT " of %i\n", i, outbs->size);
		//	x=outbs->buffer[i];
		//}
		fflush(stdout);
		//printf("Copy Data - step 2\n");
		memcpy(return_module->data_buffer, outbs->buffer, return_module->buffer_size);
		//printf("Cleaning up\n");
	} else {
		LOG_WARNING1("Error - NOT A I FRAME %05x\n", type);
	}

	/* Free Dali */

	Mpeg2PicHdrFree(fh);
	Mpeg2SeqHdrFree(sh);
	BitStreamFree(bs);
	BitParserFree(bp);
	BitStreamFree(outbs);
	BitParserFree(outbp);
	ByteFree(r);
	ByteFree(g);
	ByteFree(b);
	ByteFree(y);
	ByteFree(u);
	ByteFree(v);
	ScFree(scy);
	ScFree(scu);
	ScFree(scv);
	Mpeg2MBInfoArrayFree(ia);

	return(return_module);


}

#endif

/***************************************************************************/



#ifdef MPEG2DEC
static module_t * make_ppm (int width, int height, uint8_t * buf)
{
	int size;
	uint8_t header[20];
	uint8_t * pt1;

	module_t * return_module;

	sprintf ((char*)header, "P6\n%d %d\n255\n", width, height);
	size = strlen((char*)header);

	return_module = (module_t*)malloc(sizeof(module_t));
	memset(return_module, 0,sizeof(module_t));
	return_module->data_buffer = (uint8_t *)malloc((sizeof(uint8_t) * 3 * width *  height) + size);
	memset(return_module->data_buffer, 0,(sizeof(uint8_t) * 3 * width *  height) + size);
	return_module->buffer_size = (sizeof(uint8_t) * 3 * width *  height) + size;

	memcpy(return_module->data_buffer, header, size);
	pt1 = return_module->data_buffer + size;
	memcpy(pt1, buf, sizeof(uint8_t) * 3 * width *  height);
	return(return_module);

}



module_t * get_image(tystream_holder_t * tystream, gop_index_t * gop_index, module_t * init_image) {


	/* MPEG2DEC */
	#define BUFFER_SIZE 4096
	uint8_t buffer[BUFFER_SIZE];
	mpeg2dec_t * mpeg2dec;
	const mpeg2_info_t * info;
	int state;
	int size;

	/* Tydemux */

	module_t * module = NULL;
	module_t * return_module;
	vstream_t * vstream;
	int gotit;

	return_module = NULL;

	if(!tystream->index || !init_image || !init_image->data_buffer) {
		return(0);
	}

	if(!tystream->remote_holder) {
		module = (module_t *)malloc(sizeof(module_t));
		memset(module, 0,sizeof(module_t));
		module->data_buffer = NULL;
		module->buffer_size = 0;

		if(!get_image_and_time(tystream, gop_index, module)) {
			free(module);
			return(0);
		}

	} else {
		module = tydemux_get_remote_i_frame(tystream->remote_holder, tystream, gop_index->gop_number);
		if(!module) {
			return(0);
		}
	}

	/* Create a MPEG stream */

	vstream = new_vstream();
	tydemux_write_vstream(init_image->data_buffer, 1, init_image->buffer_size, vstream);

	set_temporal_reference(module->data_buffer, 0);
	tydemux_write_vstream(module->data_buffer, 1, module->buffer_size, vstream);

	/* We need to pictures since we the decoding doesn't start until we reash the start
	of the second picture */
	set_temporal_reference(module->data_buffer, 1);
	tydemux_write_vstream(module->data_buffer, 1, module->buffer_size, vstream);

	//printf("Into decode\n");
	//fflush(stdout);
	//sleep(5);
	mpeg2dec = mpeg2_init ();

	if (mpeg2dec == NULL) {
		return(0);
	}

	info = mpeg2_info (mpeg2dec);

	size = BUFFER_SIZE;
	gotit = 0;

	do {
		state = mpeg2_parse (mpeg2dec);
		switch (state) {
			case -1:
				size = tydemux_read_vstream(buffer, 1, BUFFER_SIZE, vstream);
				mpeg2_buffer (mpeg2dec, buffer, buffer + size);
				break;
			case STATE_SEQUENCE:
				mpeg2_convert (mpeg2dec, convert_rgb24, NULL);
				break;
			case STATE_SLICE:
			case STATE_END:
				if (info->display_fbuf)
					return_module = make_ppm (info->sequence->width, info->sequence->height,
						info->display_fbuf->buf[0]);
				gotit=1;
				break;
		}

		if(gotit) {
			break;
		}

	} while (size);

	mpeg2_close (mpeg2dec);
	//printf("Out decode\n");
	fflush(stdout);

	free_vstream(vstream);
	free(module->data_buffer);
	free(module);
#ifdef WIN32
	assert( tystream->vstream != (vstream_t *)0xdddddddd );
#endif
	return(return_module);


}

#endif
/**************************************************************/


int get_first_seq(tystream_holder_t * tystream, gop_index_t * gop_index, module_t * module) {

	/* Iteration */
	int i;


	/* The chunk we read */
	chunk_t * chunk = NULL;

	/* The chunk linked list */
	chunk_t * chunks = NULL;

	int chunks_to_read;
	int result;

	if(!gop_index) {
		return(0);
	}


	chunks_to_read = 2;


	for(i=0; i < chunks_to_read; i++) {
		chunk = read_chunk(tystream, gop_index->chunk_number_seq + i, 1);
		if(chunk) {
			chunks = add_chunk(tystream, chunk, chunks);
		} else {
			chunks_to_read++;
		}
	}


	LOG_DEVDIAG("getting seq\n");
	if(gop_index->seq_rec_nr <= chunks->nr_records) {
		result = get_video(gop_index->seq_rec_nr, module, chunks, MPEG_SEQ, tystream);
	} else {
		result = 0;
		LOG_ERROR2("WE TRY TO GET A RECORD NOT IN THE CHUNK %i - %i\n", gop_index->seq_rec_nr, chunk->nr_records);
	}



	if(!result) {
		LOG_ERROR2("parse_chunk_video: ERROR - seq-frame - chunk " I64FORMAT ", record %i\n", \
			chunks->chunk_number, gop_index->seq_rec_nr);
		free_junk_chunks(chunks);
		return(get_first_seq(tystream,gop_index->next,module));
	}

	LOG_DEVDIAG("setting seq\n");
	if(!tystream->remote_holder) {
		/* This is already done on the tivo side */
		set_seq_low_delay(tystream, module);
	}
	LOG_DEVDIAG("getting seq done\n");
	free_junk_chunks(chunks);
	return(1);


}

int get_index_time(tystream_holder_t * tystream, gop_index_t * gop_index) {

	/* Iteration */
	int i;


	/* The chunk we read */
	chunk_t * chunk = NULL;

	/* The chunk linked list */
	chunk_t * chunks = NULL;

	int chunks_to_read;


	chunks_to_read = 2;

	/* Find out how many chunks we need to read
	if pes and i frame is in the same chunk then two chunks if not three chunks
	*/


	for(i=0; i < chunks_to_read; i++) {
		chunk = read_chunk(tystream, gop_index->chunk_number_i_frame_pes + i, 1);
		if(chunk) {
			chunks = add_chunk(tystream, chunk, chunks);
		} else {
			chunks_to_read++;
		}
	}



	gop_index->time_of_iframe = get_time_Y_video(gop_index->i_frame_pes_rec_nr, chunks, tystream);

	free_junk_chunks(chunks);

	return(1);
}


int get_image_and_time(tystream_holder_t * tystream, gop_index_t * gop_index, module_t * module) {

	/* Iteration */
	int i;


	/* The chunk we read */
	chunk_t * chunk = NULL;

	/* The chunk linked list */
	chunk_t * chunks = NULL;

	int chunks_to_read;
	int result;


	chunks_to_read = 2;

	/* Find out how many chunks we need to read
	if pes and i frame is in the same chunk then two chunks if not three chunks
	*/


	for(i=0; i < chunks_to_read; i++) {
		chunk = read_chunk(tystream, gop_index->chunk_number_i_frame + i, 1);
		if(chunk) {
			chunks = add_chunk(tystream, chunk, chunks);
		} else {
			chunks_to_read++;
		}
	}

	if(gop_index->i_frame_rec_nr <= chunks->nr_records) {
		result = get_video(gop_index->i_frame_rec_nr, module, chunks, MPEG_I, tystream);
	} else {
		result = 0;
		LOG_ERROR2("WE TRY TO GET A RECORD NOT IN THE CHUNK %i - %i\n", gop_index->i_frame_rec_nr, chunk->nr_records);
	}


	if(!result) {
		LOG_ERROR2("parse_chunk_video: ERROR - i-frame - chunk " I64FORMAT ", record %i\n", \
			chunks->chunk_number,gop_index->i_frame_rec_nr );
		free_junk_chunks(chunks);
		return(0);
	}


	free_junk_chunks(chunks);

	return(1);

}


index_t * create_index(tystream_holder_t * tystream, int interactive, int *pcprogress, char *szprogress ) {

	/* Iteration */
	int64_t ul;
	int64_t last_chunk;

	/* Gop number */
	int64_t gop_number;

	/* Markers */
	int reuse;

	/* Results of funcs */
	int result;

	/* Chunk numbering */
	chunknumber_t chunk_nr;

	index_t * index;
	gop_index_t * gop_index;
	gop_index_t * last_gop_index;

	index = (index_t *)malloc(sizeof(index_t));
	memset(index,0,sizeof(index_t));

	index->nr_of_gops = 0;
	index->gop_index = NULL;

	reuse = 0;

	LOG_USERDIAG("Starting index process\n");
	last_chunk = (int64_t)( tystream->in_file_size/CHUNK_SIZE - tystream->start_chunk );

	if( szprogress ) {
		strcpy( szprogress, "Indexing..." );
	}

	gop_number = 1;
	for(ul=0, chunk_nr = 0; ul < last_chunk; ul++, chunk_nr++) {


		if( pcprogress ) {
			*pcprogress = (int)((ul*50)/last_chunk);
		}

		if(!reuse) {
			gop_index = new_gop_index();
		} else {
			/* reset gop index */
			gop_index->gop_number=0;
			gop_index->chunk_number_seq = -1;
			gop_index->seq_rec_nr = -1;
			gop_index->chunk_number_i_frame_pes = -1;
			gop_index->i_frame_pes_rec_nr = -1;
			gop_index->chunk_number_i_frame = -1;
			gop_index->i_frame_rec_nr = -1;
			gop_index->time_of_iframe = 0;
			gop_index->display_time = 0;
			gop_index->reset_of_pts=0;
		}

		result = seq_preset_in_chunk(tystream, chunk_nr, gop_index, 0);

		if(result < 1) {
			reuse = 1;
			continue;
		}

		if(result == 3) {
			/* We got all of them SEQ, PES and I Frame
			add the gop_index to the linked list */
			last_gop_index = gop_index;
			add_gop_index(index, gop_index, gop_number);
			gop_number++;
			reuse = 0;
			continue;
		}

		if(result < 3) {
			//printf("Second try\n");
			result = seq_preset_in_chunk(tystream, chunk_nr + 1, gop_index, 1);

			if(result < 1) {
				/* Giving up on this index */
				//printf("Failed to create index\n");
				reuse = 1;
				continue;
			}

			if(gop_index->i_frame_pes_rec_nr != -1 && gop_index->i_frame_rec_nr != -1 ) {
				last_gop_index = gop_index;
				add_gop_index(index, gop_index, gop_number);
				gop_number++;
				reuse = 0;
				continue;
			} else {
				//printf("Failed to create index\n");
				/* Scrap it and start over */
				reuse = 1;
			}
		}
	}

	index->last_gop_index = last_gop_index->previous;

	/* We may other wise hang on the last record FIXME Workaround */
	last_gop_index->previous->next = NULL;
	free(last_gop_index);



	if( szprogress ) {
		strcpy( szprogress, "Getting index times..." );
	}
	if( pcprogress ) {
		*pcprogress = 0;
	}

	gop_index = index->gop_index;
	ul = 0;
	while(gop_index) {
		++ul;
		if( pcprogress ) {
			*pcprogress = (int)((ul*50)/index->nr_of_gops)+49;
		}
		get_index_time(tystream, gop_index);
		gop_index = gop_index->next;
	}

	LOG_USERDIAG1("Indexing finished " I64FORMAT " entries in index\n", index->nr_of_gops);

	return(scan_index(tystream, index));

}

#if 0
index_t * scan_index(tystream_holder_t * tystream, index_t * index) {

	/* Create display_times for all the GOPS */
	gop_index_t * tmp_gop_index;
	ticks_t display_time = 0;
	ticks_t max_diff;
	ticks_t time_since_last;
	ticks_t avg_gop_time_diff;

	/* 1 min gap then we definitely have had a PTS reset */
	/* Nope it's not but you can make it 10min and we can
	be sure - the thing is when we hit gigantic holes
	in the stream */
	max_diff = tystream->frame_tick * (int64_t)3600;

	/* Start at the second index - the first one is displayed at display_time=0 */
	tmp_gop_index = index->gop_index->next;
	while(tmp_gop_index) {
		time_since_last = tmp_gop_index->time_of_iframe - tmp_gop_index->previous->time_of_iframe;
		if( time_since_last <= 0 || time_since_last > max_diff ) {
			tmp_gop_index->reset_of_pts = 1;
			/* Set the display time of this GOP to the previous GOP plus a bit */
			tmp_gop_index->display_time = tmp_gop_index->previous->display_time + avg_gop_time_diff;
		} else {
			tmp_gop_index->display_time = tmp_gop_index->previous->display_time + time_since_last;

			/* Could be more clever working this out but assuming only one or two */
			/* resets occur in the stream, this is good enough for now */
			avg_gop_time_diff = time_since_last;
		}
		tmp_gop_index = tmp_gop_index->next;
	}

	/* Leave these in in case we need them in the future
	 * the editor uses them as the first and last frames the user can see */
	index->real_start_gop_index = index->gop_index;
	index->real_end_gop_index = index->last_gop_index;

	return(index);
}


index_t * scan_index(tystream_holder_t * tystream, index_t * index) {

	/* Okay this is a first try to find resets in PTS at the start and end of a tyrecoding */

	/* We speculate the the max number of frames between two gops are 100 hence the max diff
	in time should be tystream->frame_tick * 100. We start in the middle of the index and work
	both ways if we bounce into bigger diff in either direction well then we know we have a rest */

	int64_t middle_gop_index_nr;
	ticks_t middle_time;
	ticks_t max_diff;

	gop_index_t * tmp_gop_index;
	gop_index_t * middle_gop_index;
	gop_index_t * end_reset_index = NULL;
	gop_index_t * start_reset_index = NULL;



	if(index->last_gop_index->gop_number <= (int64_t)200) {
		index->real_end_gop_index = index->last_gop_index;
		index->real_start_gop_index = index->gop_index;
		return(index);
	}

	/* Okay 10 min gap and we definitly out of range :) */
	max_diff = tystream->frame_tick * (int64_t)1080000;

	tmp_gop_index = index->gop_index;
	middle_gop_index_nr = index->last_gop_index->gop_number / (int64_t)2;

	while(tmp_gop_index && (tmp_gop_index->gop_number <= middle_gop_index_nr)) {
		tmp_gop_index = tmp_gop_index->next;
	}

	middle_gop_index = tmp_gop_index;
	middle_time = middle_gop_index->time_of_iframe;

	tmp_gop_index = middle_gop_index->next;

	/* Second half */
	while(tmp_gop_index) {
		if(tmp_gop_index->time_of_iframe <= middle_time ||
		tmp_gop_index->time_of_iframe > middle_time + max_diff) {
			tmp_gop_index->reset_of_pts = 1;
			//end_reset = 1;
			end_reset_index = tmp_gop_index;
			break;
		}
		middle_time = tmp_gop_index->time_of_iframe;
		tmp_gop_index = tmp_gop_index->next;
	}


	middle_time = middle_gop_index->time_of_iframe;
	tmp_gop_index = middle_gop_index->previous;

	/* First half */
	while(tmp_gop_index) {
		if(tmp_gop_index->time_of_iframe >= middle_time ||
		tmp_gop_index->time_of_iframe < middle_time - max_diff) {
			tmp_gop_index->reset_of_pts = 1;
			//start_reset = 1;
			start_reset_index = tmp_gop_index;
			break;
		}
		middle_time = tmp_gop_index->time_of_iframe;
		tmp_gop_index = tmp_gop_index->previous;
	}

	if(end_reset_index) {
		/* we need to cut this a bit before */
		end_reset_index = end_reset_index->previous->previous->previous->previous;
		index->real_end_gop_index = end_reset_index;
	} else {
		index->real_end_gop_index = index->last_gop_index;
	}

	if(start_reset_index) {
		/* we need to cut this a but after */
		start_reset_index = start_reset_index->next->next->next->next;
		index->real_start_gop_index = start_reset_index;
	} else {
		index->real_start_gop_index = index->gop_index;
	}

	return(index);

}
#endif



static index_t * scan_index_display_time(tystream_holder_t * tystream, index_t * index) {

	/* Create display_times for all the GOPS */
	gop_index_t * tmp_gop_index;
//	ticks_t display_time = 0;
	ticks_t max_diff;
	ticks_t time_since_last;
	ticks_t avg_gop_time_diff;
	ticks_t frame_rate;

	/* 1 min gap then we definitely have had a PTS reset */
	/* Nope it's not but you can make it 10min and we can
	be sure - the thing is when we hit gigantic holes
	in the stream */
	if(tystream->frame_rate == 3) {
		frame_rate = 25;
	} else {
		frame_rate = 30;
	}

	max_diff = tystream->frame_tick * frame_rate * (int64_t)60;

	/* Start at the second index - the first one is displayed at display_time=0 */
	tmp_gop_index = index->real_start_gop_index->next;

	/* It doesn't really matter if we fix display times beyon the real end */
	while(tmp_gop_index) {
		time_since_last = tmp_gop_index->time_of_iframe - tmp_gop_index->previous->time_of_iframe;
		if( time_since_last <= 0 || time_since_last > max_diff ) {
			tmp_gop_index->reset_of_pts = 1;
			/* Set the display time of this GOP to the previous GOP plus a bit */
			tmp_gop_index->display_time = tmp_gop_index->previous->display_time + avg_gop_time_diff;
		} else {
			tmp_gop_index->display_time = tmp_gop_index->previous->display_time + time_since_last;

			/* Could be more clever working this out but assuming only one or two */
			/* resets occur in the stream, this is good enough for now */
			avg_gop_time_diff = time_since_last;
		}
		tmp_gop_index = tmp_gop_index->next;
	}


	return(index);
}


index_t * scan_index(tystream_holder_t * tystream, index_t * index) {

	/* Okay this is a first try to find resets in PTS at the start and end of a tyrecoding */


	int64_t middle_gop_index_nr;
	ticks_t middle_time;
	ticks_t max_diff;
	ticks_t min_diff;
	ticks_t frame_rate;
	ticks_t time_since_last;
	ticks_t test_time_since_last;
	int counter;
	int okay;

	gop_index_t * tmp_gop_index;
	gop_index_t * middle_gop_index;
	gop_index_t * end_reset_index = NULL;
	gop_index_t * start_reset_index = NULL;
	gop_index_t * test_gop_index;
	gop_index_t * junk_gop_index;
	gop_index_t * save_gop_index;


	if(index->last_gop_index->gop_number <= (int64_t)6) {
		index->real_end_gop_index = index->last_gop_index;
		index->real_start_gop_index = index->gop_index;
		return(scan_index_display_time(tystream, index));
	}

	if(tystream->frame_rate == 3) {
		frame_rate = 25;
	} else {
		frame_rate = 30;
	}

	/* Okay 5 min gap and we definitly out of range :) - ie a real pts reset
	fram_rate (f/s) * time_per_frame * 60sec * 10min */
	max_diff = frame_rate * tystream->frame_tick * (int64_t)60 * (int64_t)5;
	min_diff = (int64_t)-1 * max_diff;

	tmp_gop_index = index->gop_index;
	middle_gop_index_nr = index->last_gop_index->gop_number / (int64_t)2;

	while(tmp_gop_index && (tmp_gop_index->gop_number <= middle_gop_index_nr)) {
		tmp_gop_index = tmp_gop_index->next;
	}

	/*tmp_gop_index = index->gop_index;

	while(tmp_gop_index) {
		printf("Time: " I64FORMAT "\n", tmp_gop_index->time_of_iframe);
		tmp_gop_index = tmp_gop_index->next;
	}
	*/
	tmp_gop_index = index->gop_index;

	middle_gop_index = tmp_gop_index;
	middle_time = middle_gop_index->time_of_iframe;

	tmp_gop_index = middle_gop_index->next;

	/* Second half */
	while(tmp_gop_index) {

		time_since_last = tmp_gop_index->time_of_iframe - tmp_gop_index->previous->time_of_iframe;
		if(time_since_last < min_diff || time_since_last > max_diff ) {
			/* okay lets see if this is the real reset of just
			a garbled pts timestamp */
			okay = 0;
			counter = 0;
			test_gop_index = tmp_gop_index->next;
			while(test_gop_index && counter < 10) {
				counter++;
				test_time_since_last = test_gop_index->time_of_iframe - tmp_gop_index->previous->time_of_iframe;
				if(test_time_since_last < min_diff || test_time_since_last > max_diff ) {
					/* Still funky - remember thers is about 1 to 2 sec per gop */
					okay = 0;
				} else {
					/* This one is okay */
					/* OLAF to John - this should indicate a stary frame that should not be in the index */
					/* I think we should remove it - see bug report of frames not present in show */
					/* In other words this is a out of sync chunk so we need to scrap it */
					//printf("We got an okay " I64FORMAT " - %i\n",tmp_gop_index->previous->chunk_number_i_frame, counter);
					okay = 1;
					junk_gop_index = tmp_gop_index;
					save_gop_index = tmp_gop_index->previous;

					test_gop_index->previous->next = NULL; /* Seal of the end of the junk */
					junk_gop_index->previous = NULL; /* Seal of the start */

					/* Tie together */
					save_gop_index->next = test_gop_index;
					test_gop_index->previous = save_gop_index;

					/* Free the junk */
					while(junk_gop_index) {
						tmp_gop_index = junk_gop_index;
						junk_gop_index = junk_gop_index->next;
						free(tmp_gop_index);
					}

					tmp_gop_index = test_gop_index->next;
					break;
				}
				test_gop_index = test_gop_index->next;
			}
			if(!okay) {
				tmp_gop_index->reset_of_pts = 1;
				//end_reset = 1;
				end_reset_index = tmp_gop_index;
				break;
			} else {
				continue;
			}
		}
		middle_time = tmp_gop_index->time_of_iframe;
		tmp_gop_index = tmp_gop_index->next;

	}


	middle_time = middle_gop_index->time_of_iframe;
	tmp_gop_index = middle_gop_index->previous;

	/* First half */
	while(tmp_gop_index) {
		time_since_last = tmp_gop_index->next->time_of_iframe - tmp_gop_index->time_of_iframe;
		if(time_since_last < min_diff || time_since_last > max_diff ) {
			/* okay lets see if this is the real reset of just
			a garbled pts timestamp */
			okay = 0;
			counter = 0;
			test_gop_index = tmp_gop_index->previous;
			while(test_gop_index && counter < 10) {
				counter++;
				test_time_since_last = tmp_gop_index->next->time_of_iframe - test_gop_index->time_of_iframe;
				if(test_time_since_last < min_diff || test_time_since_last > max_diff ) {
					/* Still funky - remember thers is about 1 to 2 sec per gop */
					okay = 0;
				} else {
					/* This one is okay */
					//printf("We got an okay " I64FORMAT " - %i\n",tmp_gop_index->previous->chunk_number_i_frame, counter);

					junk_gop_index = tmp_gop_index;
					save_gop_index = tmp_gop_index->next;

					test_gop_index->next->previous = NULL; /* Seal of the "start" of the junk */
					junk_gop_index->next = NULL; /* Seal of the "end" */

					/* Tie together */
					save_gop_index->previous = test_gop_index;
					test_gop_index->next = save_gop_index;

					/* Free the junk */
					while(junk_gop_index) {
						tmp_gop_index = junk_gop_index;
						junk_gop_index = junk_gop_index->previous;
						free(tmp_gop_index);
					}

					tmp_gop_index = test_gop_index->previous;
					okay = 1;
					break;
				}
				test_gop_index = test_gop_index->previous;
			}
			if(!okay) {
				tmp_gop_index->reset_of_pts = 1;
				//end_reset = 1;
				start_reset_index = tmp_gop_index;
				break;
			} else {
				continue;
			}
		}
		middle_time = tmp_gop_index->time_of_iframe;
		tmp_gop_index = tmp_gop_index->previous;
	}

	if(end_reset_index) {
		/* we need to cut this a bit before */
		end_reset_index = end_reset_index->previous->previous->previous->previous;
		index->real_end_gop_index = end_reset_index;
	} else {
		index->real_end_gop_index = index->last_gop_index;
	}

	if(start_reset_index) {
		/* we need to cut this a but after */
		start_reset_index = start_reset_index->next->next->next->next;
		index->real_start_gop_index = start_reset_index;
	} else {
		index->real_start_gop_index = index->gop_index;
	}

	return(scan_index_display_time(tystream, index));

}




























