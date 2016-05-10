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

void free_module(module_t * module) {
	free(module->data_buffer);
	free(module);
}

/* Returns nmemb read  - use tydemux_feof to see if we have reached the end */
/* Will always reset current bit! */
size_t tydemux_read_vstream(void * prt, size_t size, size_t nmemb, vstream_t * vstream) {

	int bytes_to_read;
	uint8_t * to_much_pos;

	bytes_to_read = nmemb * size;

	vstream->current_bit = 0;

	to_much_pos = vstream->current_pos + bytes_to_read;

	if(to_much_pos > vstream->end_stream) {
		bytes_to_read = vstream->end_stream - vstream->current_pos;
		if(!bytes_to_read) {
			vstream->eof=1;
		}
	}

	memcpy(prt, vstream->current_pos, bytes_to_read);

	vstream->current_pos = vstream->current_pos + bytes_to_read;

	return((size_t)(bytes_to_read/size));

}

int tydemux_seek_vstream(vstream_t * vstream, int offset, int whence) {


	vstream->current_bit = 0;

	if(whence == SEEK_END) {
		if(offset == 0) {
			vstream->eof=1;
			vstream->current_pos = vstream->end_stream;
			return(0);
		} else if (offset > 0) {
			return(-1);
		} else {
			if(vstream->end_stream + offset < vstream->start_stream) {
				return(-1);
			}
			vstream->eof=0;
			vstream->current_pos = vstream->end_stream + offset;
			return(0);
		}

	} else if (whence == SEEK_CUR) {
		if(offset == 0) {
			if(vstream->current_pos == vstream->end_stream) {
				vstream->eof=1;
			} else {
				vstream->eof=0;
			}
			return(0);

		} else if (offset > 0) {
			if(vstream->current_pos + offset > vstream->end_stream) {
				return(-1);
			} else if (vstream->current_pos + offset == vstream->end_stream) {
				vstream->eof=1;
				vstream->current_pos = vstream->end_stream;
				return(0);
			}
			vstream->current_pos = vstream->current_pos + offset;
			return(0);

		} else {
			if(vstream->current_pos + offset < vstream->start_stream) {
				return(-1);
			} else if (vstream->current_pos + offset == vstream->start_stream) {
				vstream->eof=0;
				vstream->current_pos = vstream->start_stream;
				return(0);
			}
			vstream->current_pos = vstream->current_pos + offset;
			vstream->eof=0;
			return(0);
		}

	} else if (whence == SEEK_SET) {
		if(offset == 0) {
			vstream->eof=0;
			vstream->current_pos = vstream->start_stream;
			return(0);
		} else if (offset < 0) {
			return(-1);
		} else {
			if(vstream->start_stream + offset > vstream->end_stream) {
				return(-1);
			}
			vstream->eof=0;
			vstream->current_pos = vstream->start_stream + offset;
			return(0);
		}
	} else {
		return(-1);
	}

}

int tydemux_tell_vstream(vstream_t * vstream) {
	return(vstream->current_pos - vstream->start_stream);
}


vstream_t * new_vstream() {

	vstream_t * vstream;

	vstream = (vstream_t *)malloc(sizeof(vstream_t));
	memset(vstream, 0,sizeof(vstream_t));
	vstream->eof=0;
	vstream->current_bit = 0;
	vstream->size=0;
	vstream->start_stream=NULL;
	vstream->end_stream=NULL;
	vstream->current_pos=NULL;
	return(vstream);
}

/* Will not affects reads the current pos will still be the same */

size_t tydemux_write_vstream(const void * ptr,size_t size, size_t nmemb, vstream_t * vstream) {

	int current_pos;

	if(!vstream->start_stream) {
		current_pos = vstream->current_pos - vstream->start_stream;
	} else {
		current_pos = 0;
	}

	vstream->start_stream = (uint8_t *)realloc(vstream->start_stream, (size_t)(vstream->size + size * nmemb));
	memcpy(vstream->start_stream + vstream->size, ptr, size * nmemb);

	vstream->size = vstream->size + size * nmemb;
	vstream->end_stream = vstream->start_stream + vstream->size;
	vstream->current_pos = vstream->start_stream + current_pos;
	vstream->eof = 0;
	return(nmemb);

}

void free_vstream(vstream_t * vstream) {
	free(vstream->start_stream);
	free(vstream);
	return;
}
#if 0


void tydemux_bits_seek_start(vstream_t * vstream) {
	vstream->current_pos = vstream->start_stream;
	vstream->eof = 0;
	vstream->current_bit = 0;
	return;
}


uint32_t tydemux_bits_getbyte_noptr(vstream_t * vstream) {

	return(tydemux_bits_getbits(vstream, 8));

}


uint32_t tydemux_bits_getbit_noptr(vstream_t * vstream) {

	return(tydemux_bits_getbits(vstream, 1));

}

int tydemux_feof(vstream_t * vstream) {
	if(vstream->eof) {
		return(1);
	} else {
		return(0);
	}
}

uint32_t tydemux_bits_showbits32_noptr(vstream_t * vstream) {

	return(tydemux_bits_showbits(vstream, 32));
}


uint32_t tydemux_bits_getbits(vstream_t * vstream, int nr_of_bits) {


	int i;
	uint8_t * current_pos;

	int current_bit;
	int bit;

	int to_much_bit;
	uint8_t * to_much_pos;

	uint32_t storage;
	int decrement;

	current_pos = vstream->current_pos;
	current_bit = vstream->current_bit;

	to_much_bit = current_bit + nr_of_bits;
	to_much_pos = current_pos +to_much_bit/8;
	to_much_bit = to_much_bit%8;



	if((to_much_pos > vstream->end_stream) ||
		(to_much_pos == vstream->end_stream  && to_much_bit)) {

		vstream->eof = 1;
		return(0);
	}

	decrement = nr_of_bits - 1;
	storage = 0;

	for(i=0; i < nr_of_bits; i++) {

		bit = getbit(current_pos, i + current_bit);
		bit = bit << decrement;
		storage |= bit;
		decrement--;
	}

	current_bit = current_bit + nr_of_bits;
	current_pos = current_pos + current_bit/8;
	current_bit = current_bit%8;

	vstream->current_pos = current_pos;
	vstream->current_bit = current_bit;
	return(storage);

}


uint32_t tydemux_bits_showbits(vstream_t * vstream, int nr_of_bits) {


	int i;
	uint8_t * current_pos;

	int current_bit;
	int bit;

	int to_much_bit;
	uint8_t * to_much_pos;

	uint32_t storage;
	int decrement;

	current_pos = vstream->current_pos;
	current_bit = vstream->current_bit;

	to_much_bit = current_bit + nr_of_bits;
	to_much_pos = current_pos +to_much_bit/8;
	to_much_bit = to_much_bit%8;



	if((to_much_pos > vstream->end_stream) ||
		(to_much_pos == vstream->end_stream  && to_much_bit)) {

		vstream->eof = 1;
		return(0);
	}

	decrement = nr_of_bits - 1;
	storage = 0;

	for(i=0; i < nr_of_bits; i++) {

		bit = getbit(current_pos, i + current_bit);
		bit = bit << decrement;
		storage |= bit;
		decrement--;
	}

	return(storage);

}
#endif
