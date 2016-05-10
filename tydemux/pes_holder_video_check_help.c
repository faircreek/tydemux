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


int is_okay(const pes_holder_t * pes_holder, pes_holder_t * to_check_pes) {


	pes_holder_t * tmp_pes_holder;
	int counter;
	int gotit;

	counter = 0;
	tmp_pes_holder = (pes_holder_t *)pes_holder;

	if(!to_check_pes || !pes_holder) {
		return(0);
	}

	if(!pes_holder->seq_present) {
		LOG_DEVDIAG("No seq in pes\n");
		LOG_DEVDIAG5(" I %i B %i P %i SEQ %i GOP %i\n", pes_holder->i_frame_present, pes_holder->b_frame_present,
			pes_holder->p_frame_present, pes_holder->seq_present, pes_holder->gop_present);
		return(0);
	}


	LOG_DEVDIAG5(" I %i B %i P %i SEQ %i GOP %i\n", pes_holder->i_frame_present, pes_holder->b_frame_present,
		pes_holder->p_frame_present, pes_holder->seq_present, pes_holder->gop_present);

	gotit = 0;
	while(tmp_pes_holder) {

		if(tmp_pes_holder == to_check_pes) {
			gotit = 1;
			break;
		}

		if(tmp_pes_holder->seq_present) {
			counter++;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	if(!gotit) {
		return(0);
	}

	return(counter);
}

int is_okay_frames(const pes_holder_t * pes_holder, pes_holder_t * to_check_pes) {


	pes_holder_t * tmp_pes_holder;
	int counter;
	int gotit;

	counter = 0;
	
	tmp_pes_holder = (pes_holder_t *)pes_holder;

	if(!to_check_pes || !pes_holder) {
		return(0);
	}


	gotit = 0;
	while(tmp_pes_holder) {

		if(tmp_pes_holder == to_check_pes) {
			gotit = 1;
			break;
		}

		counter++;
		tmp_pes_holder = tmp_pes_holder->next;
	
	}

	if(!gotit) {
		return(0);
	}

	return(counter);
}

