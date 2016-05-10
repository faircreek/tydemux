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
#include "tyabout.c"


int ty_debug = 1;
int icount;


void set_debug_level(int debug_level) {
	ty_debug = debug_level;
	icount = 0;
}




int nr_chunks_f(const chunk_t * chunks) {

	int i;

	chunk_t * tmp_chunk;

	tmp_chunk = (chunk_t * )chunks;
	for (i=0;tmp_chunk;tmp_chunk = tmp_chunk->next, i++);
#if 0
	i = 0;
	while(tmp_chunk) {
		i++;
		tmp_chunk = tmp_chunk->next;
	}
#endif

	return(i);


}

void prog_meter(tystream_holder_t * tystream) {

	if(!tystream->cons_junk_chunk) {

		if (tystream->number_of_chunks != 0) {

			if( tystream->std_alone ) {
				if(tystream->number_of_chunks%100 == 0 ) {
					LOG_PROGRESS1("%6lld", tystream->number_of_chunks);
					if( tystream->number_of_chunks%500 == 0 ) {
						LOG_PROGRESS("\n");
					}
				} else if ( tystream->number_of_chunks%10 == 0 ) {
					LOG_PROGRESS(".");
				}
			} else {
				LOG_PROGRESS1("Chunks: %6lld", tystream->number_of_chunks);
			}
		}
		fflush(stdout);
	}
}




void old_prog_meter(tystream_holder_t * tystream) {

	/* Well this can naturally be a whole lot shorter but if you want
	a nicely aliged output then it has to be big a bulky */
	if(!tystream->cons_junk_chunk) {

		if( tystream->number_of_chunks < 1000) {
			if(tystream->number_of_chunks%10 == 0 && tystream->number_of_chunks != 0 && tystream->number_of_chunks%500 != 0 && tystream->number_of_chunks%100 != 0) {
				printf(".");
			}

			if(tystream->number_of_chunks%100 == 0 && tystream->number_of_chunks != 0 && tystream->number_of_chunks%500 != 0) {
				printf("  " I64FORMAT "", tystream->number_of_chunks);
			}

			if(tystream->number_of_chunks%500 == 0 && tystream->number_of_chunks != 0 ) {
				printf("  " I64FORMAT "\n",tystream->number_of_chunks);
			}
		} else if (tystream->number_of_chunks >= 1000 && tystream->number_of_chunks < 10000) {
			if(tystream->number_of_chunks%10 == 0 && tystream->number_of_chunks != 0 && tystream->number_of_chunks%500 != 0 && tystream->number_of_chunks%100 != 0) {
				printf(".");
			}

			if(tystream->number_of_chunks%100 == 0 && tystream->number_of_chunks != 0 && tystream->number_of_chunks%500 != 0) {
				printf(" " I64FORMAT "", tystream->number_of_chunks);
			}

			if(tystream->number_of_chunks%500 == 0 && tystream->number_of_chunks != 0 ) {
				printf(" " I64FORMAT "\n",tystream->number_of_chunks);
			}

		} else {
			if(tystream->number_of_chunks%10 == 0 && tystream->number_of_chunks != 0 && tystream->number_of_chunks%500 != 0 && tystream->number_of_chunks%100 != 0) {
				printf(".");
			}

			if(tystream->number_of_chunks%100 == 0 && tystream->number_of_chunks != 0 && tystream->number_of_chunks%500 != 0) {
				printf("" I64FORMAT "", tystream->number_of_chunks);
			}

			if(tystream->number_of_chunks%500 == 0 && tystream->number_of_chunks != 0 ) {
				printf("" I64FORMAT "\n",tystream->number_of_chunks);
			}

		}
		fflush(stdout);
	}
}

int tydemux_usage(){

 	printf(""NEWLINE"tydemux usage:"NEWLINE""
	       "tydemux -s 1/2 [option] -i infile -a outfile_audio -v outfile_video"NEWLINE""
	       ""NEWLINE""
	       "Where -s 2 is manditory:"NEWLINE""
	       "\t-s 2 \t for Tivo version 2.x and up"NEWLINE""
	       ""NEWLINE""
	       "Where option is:"NEWLINE""
	       "\t -c cutfile\t (Use a TyEditor cut file)"NEWLINE""
	       "\t -h/?\t\t(help - writes this message)"NEWLINE""NEWLINE""
	       "tydemux "TYDEMUX_VERSION" %s", tyabout);




	return(0);

}

void print_probe(tystream_holder_t * tystream) {

	LOG_USERDIAG("\n");
	LOG_USERDIAG("Tystream recorded on:\n");
	if(tystream->tivo_series == S2) {
		if(tystream->tivo_type == SA) {
			LOG_USERDIAG("\t\tSA Tivo Series 2\n");
		} else {
			LOG_USERDIAG("\t\tDTivo Series 2\n");
		}
	} else {
		if(tystream->tivo_type == SA) {
			LOG_USERDIAG("\t\tSA Tivo Series 1\n");
		} else {
			LOG_USERDIAG("\t\tDTivo Series 1\n");
		}
	}
	if(tystream->tivo_version == V_13) {
		LOG_USERDIAG("\t\tSoftware rev 1.3\n");
	} else {
		if(tystream->tivo_series == S2) {
			LOG_USERDIAG("\t\tSoftware rev 3.0 or higher\n");
		} else {
			LOG_USERDIAG("\t\tSoftware rev 2.0 or higher\n");
		}
	}
	LOG_USERDIAG("\n");
	LOG_USERDIAG("Tystream recoding audio stats:\n");
	if(tystream->audio_type > DTIVO_AC3) {
		LOG_USERDIAG("\t\tMPEG Layer II audio\n");
	} else {
		LOG_USERDIAG("\t\tAC3 (Dolby Digital) audio\n");
	}
	LOG_USERDIAG1("\t\tAverage tyrecord (audio) size: %i\n", tystream->std_audio_size);
	LOG_USERDIAG1("\t\tAudio frame size: %i\n", tystream->audio_frame_size);
	LOG_USERDIAG1("\t\tAudio frame time: " I64FORMAT " (ticks)\n", tystream->audio_median_tick_diff);
	LOG_USERDIAG("\n");
	LOG_USERDIAG("Tystream recoding video stats:\n");
	if(tystream->tivo_version == V_2X) {
		switch(tystream->frame_rate){
			case 1:
				LOG_USERDIAG("\t\tFrame rate: 23.976 frames/sec\n");
				break;
			case 2:
				LOG_USERDIAG("\t\tFrame rate: 24 frames/sec\n");
				break;
			case 3:
				LOG_USERDIAG("\t\tFrame rate: 25 frames/sec\n");
				break;
			case 4:
				LOG_USERDIAG("\t\tFrame rate: 29.97 frames/sec\n");
				break;
			case 5:
				LOG_USERDIAG("\t\tFrame rate: 30 frames/sec\n");
				break;
			case 6:
				LOG_USERDIAG("\t\tFrame rate: 50 frames/sec\n");
				break;
			case 7:
				LOG_USERDIAG("\t\tFrame rate: 59.94 frames/sec\n");
				break;
			case 8:
				LOG_USERDIAG("\t\tFrame rate: 60 frames/sec\n");
				break;
			default:
				LOG_WARNING("WARNING: Unknown frame rate!!\n");
		}
	} else if (tystream->tivo_version == V_13) {
		LOG_USERDIAG("\t\tFrame rate: 29.97 frames/sec\n");
	}
	LOG_USERDIAG("\n");


}

#ifdef WIN32
int print_audio_delay(tystream_holder_t * tystream) {

	long time_diff;
	int return_value;

	time_diff = (unsigned long) (tystream->time_diff/90);

	return_value = (int) time_diff;


	if(time_diff ==0) {
		LOG_USERDIAG("\nA/V Sync Offset: 0ms (The -O option is not needed in mplex)\n" );

	} else if(tystream->indicator == POSITIVE) {

		LOG_USERDIAG3("\nA/V Sync Offset: %03ldms (i.e. audio plays %03ldms early use -O -%03ld in mplex)\n", \
		time_diff,time_diff, time_diff);
	} else {
		LOG_USERDIAG3("\nA/V Sync Offset: -%03ldms (i.e. audio plays %03ldms late use -O %03ld in mplex)\n", \
		time_diff,time_diff, time_diff);
	}

	LOG_USERDIAG1("Return value %d \n", return_value);

    	return(return_value);

}

#else

int print_audio_delay(tystream_holder_t * tystream) {
	ticks_t time_diff;
	int return_value;


	printf("" I64FORMAT "\n",tystream->time_diff);

	time_diff = tystream->time_diff/90;
	return_value = (int)time_diff;

	if(time_diff ==0) {
		printf("\nA/V Sync Offset: 0ms (The -O option is not needed in mplex)\n" );

	} else if (tystream->indicator == POSITIVE) {
		printf("\nA/V Sync Offset: %03lldms (i.e. audio plays %03lldms early use -O -%03lld in mplex)\n",
			time_diff,time_diff, time_diff);
	} else {
		printf("\nA/V Sync Offset: %03lldms (i.e. audio plays %03lldms late use -O %03lld in mplex)\n",
			time_diff,(ticks_t)-1 * time_diff, (ticks_t)-1 * time_diff);
			return_value = return_value * -1;
	}
	return(return_value);

}

#endif




int  tydemux_return(int return_value) {

	//printf("Return is %d\n", return_value);

#ifdef WIN32

	return(return_value);
#else
	if(return_value == -1000) {
		return(2);
	} else if (return_value == 1000) {
		return(1);
	} else {
		return(0);
	}
#endif
}



