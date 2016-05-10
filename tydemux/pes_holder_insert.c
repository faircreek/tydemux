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

int insert_pes_holder(const pes_holder_t * seq_pes_holder, pes_holder_t * pes_holder_insert, uint16_t tmp_ref) {

	/* Markers */
	int seq_header;
	int next_seq_header;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	/* Okay instead of making this one over complicated we simply insert the pes_holder
	then when all of them are done we will simply fix the tmp ref */


	while(tmp_pes_holder) {

		tmp_payload = tmp_pes_holder->payload;
		while(tmp_payload) {

			/* Run into the next SEQ header */
			if(tmp_payload->payload_type == SEQ_HEADER
				&& tmp_pes_holder != seq_pes_holder) {

				next_seq_header = 1;
				break;
			}

			if(tmp_payload->payload_type == SEQ_HEADER) {
				seq_header = 1;
			}

			/* We don't want to insert it before the I-frame or
			close to it */

			if(tmp_payload->tmp_ref > 2) {
				next_seq_header = 1;
				break;
			}

			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	pes_holder_insert->previous = tmp_pes_holder->previous;
	pes_holder_insert->next = tmp_pes_holder;

	tmp_pes_holder->previous->next = pes_holder_insert;
	tmp_pes_holder->previous = pes_holder_insert;

	return(1);
}
 
