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

int check_temporal_reference(const pes_holder_t * seq_pes_holder) {


	/* Markers */
	int got_i_frame;
	int got_p_frame;
	int got_b_frame;
	int got_gop;
	int closed_gop;
	int seq_header;

	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;

	payload_type_t  previous_frame;


	/* Reference holders */
	int stored_ref;
	int tmp_ref;
	int nr_row;
	int counter;


	/* Ref error counter */
	int error;

	/* Init */
	got_i_frame = 0;
	got_p_frame = 0;
	got_b_frame = 0;
	got_gop = 0;
	closed_gop = 0;
	seq_header = 0;
	nr_row = 0;
	stored_ref = 0;

	error = 0;
	counter = 0;

	previous_frame = UNKNOWN_PAYLOAD;

	LOG_DEVDIAG("Checking tmp ref\n");

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;


	while(tmp_pes_holder) {
		if(tmp_pes_holder->gap) {
			break;
		}
		tmp_payload = tmp_pes_holder->payload;
		while(tmp_payload) {

			/* Run into the next SEQ header */
			if(tmp_payload->payload_type == SEQ_HEADER && tmp_pes_holder != seq_pes_holder) {
				seq_header = 1;
				break;
			}

			/* We got the start GOP */
			if (tmp_payload->payload_type == GOP_HEADER && got_gop != 1) {
				got_gop = 1;
				closed_gop = is_closed_gop(tmp_pes_holder);
				if(closed_gop == -1) {
					/* error */
					LOG_DEVDIAG("Error GOP \n");
					return(-1);
				}

			} else if (tmp_payload->payload_type == GOP_HEADER && got_gop) {
				/* A GOP in the middle of a SEQ reset the counters */
				got_i_frame = 0;
				got_b_frame = 0;
				got_p_frame = 0;
				counter = 0;
				closed_gop = is_closed_gop(tmp_pes_holder);
				if(closed_gop == -1) {
					/* error */
					LOG_DEVDIAG("Error GOP \n");
					return(-1);
				}
			}

			if(got_gop) {
				if(closed_gop) {
					if(tmp_payload->payload_type == I_FRAME && !got_i_frame) {
						counter = get_temporal_reference(tmp_payload->payload_data);
						if(counter != 0) {
							error++;
							counter = 0;
							LOG_DEVDIAG1("Error -1- %i\n", error);
						}
						got_i_frame = 1;
						previous_frame = I_FRAME;

					}
					if(tmp_payload->payload_type == P_FRAME && got_i_frame && !got_p_frame) {
						stored_ref = get_temporal_reference(tmp_payload->payload_data);
						got_p_frame = 1;
						previous_frame = P_FRAME;

					} else if ((tmp_payload->payload_type == P_FRAME
							|| tmp_payload->payload_type == I_FRAME)
							&& got_i_frame && got_p_frame) {

						counter++;
						tmp_ref = get_temporal_reference(tmp_payload->payload_data);

						if(previous_frame == B_FRAME) {
							nr_row = 0;
						} else {
							nr_row++;
						}

						if(nr_row) {
							if(stored_ref + 1 != tmp_ref) {
								error++;
								LOG_DEVDIAG1("Error -2- %i\n", error);
							
							}
						} else {
							if(stored_ref != counter) {
								error++;
								LOG_DEVDIAG1("Error -3- %i\n", error);
							}
						}
						stored_ref = tmp_ref;
						previous_frame = P_FRAME;
					}
				} else {
					if(tmp_payload->payload_type == I_FRAME && !got_i_frame) {

						stored_ref = get_temporal_reference(tmp_payload->payload_data);
						got_i_frame = 1;
						previous_frame = I_FRAME;
					}

					if(tmp_payload->payload_type == P_FRAME && got_i_frame && !got_p_frame) {

						counter++;
						tmp_ref = get_temporal_reference(tmp_payload->payload_data);
						got_p_frame = 1;
						previous_frame = P_FRAME;
						LOG_DEVDIAG1("Stored %i\n",stored_ref);
						LOG_DEVDIAG1("Counter %i\n",counter);
						
						if(stored_ref != counter) {
							error++;
							//LOG_DEVDIAG1("Error -4- %i\n", error);
						
						}
						stored_ref = tmp_ref;
						previous_frame = P_FRAME;


					} else if ((tmp_payload->payload_type == P_FRAME
							|| tmp_payload->payload_type == I_FRAME)
							&& got_i_frame && got_p_frame) {

						counter++;
						tmp_ref = get_temporal_reference(tmp_payload->payload_data);

						if(previous_frame == B_FRAME) {
							nr_row = 0;
						} else {
							nr_row++;
						}

						if(nr_row) {
							if(stored_ref + 1 != tmp_ref) {
								error++;
								//LOG_DEVDIAG1("Error -5- %i\n", error);
							}
						} else {
							if(stored_ref != counter) {
								error++;
								//LOG_DEVDIAG1("Error -6- %i\n", error);
							
							}
						}
						stored_ref = tmp_ref;
						previous_frame = P_FRAME;
					}
				}

				if(tmp_payload->payload_type == B_FRAME) {
					if(!got_b_frame && closed_gop) {
						counter++;
						closed_gop = 0;
						got_b_frame = 1;

					} else if (got_b_frame && !closed_gop) {
						counter++;

					} else if (!got_b_frame && !closed_gop) {
						tmp_ref = get_temporal_reference(tmp_payload->payload_data);
						LOG_DEVDIAG1("Tmp ref is: %i\n",tmp_ref);
					
						if(tmp_ref != 0) {
							LOG_DEVDIAG("Error tmp ref in first B not zero \n");
							counter = 0;
						} else {
							counter = tmp_ref;
						}
						got_b_frame = 1;
					}

					tmp_ref = get_temporal_reference(tmp_payload->payload_data);
					LOG_DEVDIAG1("Tmp ref is: %i\n",tmp_ref);
					if(tmp_ref != counter - nr_row) {
						error++;
						LOG_DEVDIAG1("Error -7- %i\n", error);
					}
					previous_frame = B_FRAME;
				}

			}
			tmp_payload = tmp_payload->next;
		}
		if(seq_header) {
			break;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}
	/* We need to check the last stored ref */
	counter++;
	if(previous_frame == B_FRAME) {
		if(stored_ref != counter) {
			LOG_DEVDIAG("Error in last P-Frame\n");
			error++;
		}

	/* Highly experimental FIXME */
	} else if (previous_frame == P_FRAME ||  previous_frame == I_FRAME) {
		if(stored_ref != counter ) {
			error++;
			LOG_DEVDIAG1("ERROR IN NEW DETECT func %i\n", error);
		}
	}


	if(error) {
		LOG_DEVDIAG1("Temp ref Error %i\n", error);
	}

	return(error);
}


int check_nr_frames_tmp_ref(const pes_holder_t * seq_pes_holder) {
	/* Markers */
	int got_seq;
	int seq_header;

	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;



	/* Reference holders */
	int counter;
	uint16_t highest_tmp_ref;
	uint16_t second_highest_tmp_ref;
	/* Ref error counter */
	int error;

	/* Init */
	got_seq = 0;
	seq_header = 0;

	error = 0;
	counter = 0;


	LOG_DEVDIAG("Checking nr frames tmp ref\n");

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	highest_tmp_ref = get_highest_tmp_ref(tmp_pes_holder);
	second_highest_tmp_ref = get_second_highest_tmp_ref(tmp_pes_holder, highest_tmp_ref);

	if(highest_tmp_ref - second_highest_tmp_ref > 50 ) {
		/* We may better remove that temp ref! */
		remove_frame(tmp_pes_holder,highest_tmp_ref);
		highest_tmp_ref = second_highest_tmp_ref;

	}



	while(tmp_pes_holder) {
		if(tmp_pes_holder->gap) {
			break;
		}

		tmp_payload = tmp_pes_holder->payload;
		while(tmp_payload) {

			/* Run into the next SEQ header */
			if(tmp_payload->payload_type == SEQ_HEADER && tmp_pes_holder != seq_pes_holder) {
				seq_header = 1;
				break;
			}

			/* We got the start GOP */
			if (tmp_payload->payload_type == SEQ_HEADER && got_seq != 1) {
				got_seq = 1;
			}

			if(got_seq) {
				if(tmp_payload->payload_type == B_FRAME ||
					tmp_payload->payload_type == P_FRAME ||
					tmp_payload->payload_type == I_FRAME) {
					counter++;
				}
			}

			tmp_payload = tmp_payload->next;
		}
		if(seq_header) {
			break;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}





#if 0
	while(tmp_pes_holder) {
		if(tmp_pes_holder->gap) {
			break;
		}

		tmp_payload = tmp_pes_holder->payload;
		while(tmp_payload) {

			/* Run into the next SEQ header */
			if(tmp_payload->payload_type == SEQ_HEADER && tmp_pes_holder != seq_pes_holder) {
				seq_header = 1;
				break;
			}

			/* We got the start GOP */
			if (tmp_payload->payload_type == GOP_HEADER && got_gop != 1) {
				got_gop = 1;
				counter = 0;
				highest_tmp_ref = 0;
			} else if (tmp_payload->payload_type == GOP_HEADER && got_gop) {
				/* A GOP in the middle of a SEQ reset the counters */
				if(counter > highest_tmp_ref + 1) {
					error = error + counter - (highest_tmp_ref + 1);
				}
				counter = 0;
				highest_tmp_ref = 0;
			}

			if(got_gop) {

				if(tmp_payload->payload_type == B_FRAME ||
					tmp_payload->payload_type == P_FRAME ||
					tmp_payload->payload_type == I_FRAME) {
					counter++;
					if(tmp_payload->tmp_ref > highest_tmp_ref) {
						highest_tmp_ref = tmp_payload->tmp_ref;
					}
				}
			}

			tmp_payload = tmp_payload->next;
		}
		if(seq_header) {
			break;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}
#endif

	if(counter != highest_tmp_ref + 1) {
		error = counter - (highest_tmp_ref + 1);
	}

	if(error) {
		LOG_DEVDIAG1("Error %i\n", error);
	}

	return(error);

}
