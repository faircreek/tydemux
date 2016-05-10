	/*if(tystream->tivo_version == V_2X && tystream->tivo_type == SA && tystream->tivo_series == S1) {
		fix_video_pes_time(tystream);
	}*/


static void fix_video_pes_time(tystream_holder_t * tystream) {

	pes_holder_t * tmp_pes_holder;
	pes_holder_t * tmp_pes_holder_2;
	pes_holder_t * time_before_pes_holder;
	pes_holder_t * time_after_pes_holder;

	ticks_t time;

	tmp_pes_holder = tystream->pes_holder_video;

	while(tmp_pes_holder) {
		if(!tmp_pes_holder->time) {
			if(tmp_pes_holder->gap) {
				tmp_pes_holder_2  = NULL;
			} else {
				tmp_pes_holder_2 = tmp_pes_holder->next;
			}
			time_before_pes_holder = NULL;
			time_after_pes_holder = NULL;
			while(tmp_pes_holder_2) {
				if(tmp_pes_holder_2->gap) {
					break;
				}
				if(!tmp_pes_holder_2->time || tmp_pes_holder_2->i_frame_present) {
					if(tmp_pes_holder_2->previous->gap != 1) {
						time_before_pes_holder = tmp_pes_holder_2->previous;
					}
					if(tmp_pes_holder_2->next && tmp_pes_holder_2->next->gap != 1) {
						time_after_pes_holder = tmp_pes_holder_2->next;
					}
					break;
				}
				tmp_pes_holder_2 = tmp_pes_holder_2->next;
			}

			if(time_before_pes_holder && time_after_pes_holder) {
				time = (time_after_pes_holder->time - time_before_pes_holder->time)/2;
				tmp_pes_holder->time = time_before_pes_holder->time + time;

			}
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}
	return;
}



int check_sync_drift(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {


	/* Number of frames */
	uint16_t highest_tmp_ref;

	/* Reminder */
	uint16_t reminder;
	uint16_t last_reminder;
	uint16_t nr_frame_pairs;

	/* Times */
	ticks_t last_time;
	ticks_t new_last_time;
	ticks_t time_to_be;
	int64_t  drift;



	if(seq_pes_holder->repaired) {
		last_time = 0;
		last_reminder = 0;
		tystream->reminder = 0;
	} else {
		last_time = tystream->last_time;
		last_reminder = tystream->reminder;
	}


	highest_tmp_ref = get_highest_tmp_ref(seq_pes_holder);



	if(!last_time) {
		if(ty_debug >= 7) {
			LOG_DEVDIAG("Init last time for sync drift\n");
		}
		last_time = get_time_of_tmp_ref(seq_pes_holder, 0);
		if(highest_tmp_ref%2) {
			new_last_time = get_time_of_tmp_ref(seq_pes_holder, highest_tmp_ref - 1);
			reminder = 1;
		} else {
			new_last_time = get_time_of_tmp_ref(seq_pes_holder, highest_tmp_ref);
			reminder = 0;
		}
		nr_frame_pairs = highest_tmp_ref / 2;
		time_to_be  = last_time + (nr_frame_pairs * tystream->tick_diff);

		if(time_to_be >= new_last_time) {
			drift = tystream->drift + (time_to_be - new_last_time);
		} else {
			drift = tystream->drift - (new_last_time - time_to_be) ;
		}
		if(ty_debug >= 7) {
			LOG_DEVDIAG4("pre last time " I64FORMAT ", new last time " I64FORMAT ", time_to_be " I64FORMAT ", pairs %i, ", \
				last_time, new_last_time, time_to_be,nr_frame_pairs );
			LOG_DEVDIAG1("added time " I64FORMAT "\n", nr_frame_pairs * tystream->tick_diff);
		}
	} else {

		if(last_reminder) {
			if(highest_tmp_ref%2) {
				reminder = 1;
				new_last_time = get_time_of_tmp_ref(seq_pes_holder, highest_tmp_ref - 1);
			} else {
				reminder = 0;
				new_last_time = get_time_of_tmp_ref(seq_pes_holder, highest_tmp_ref);
			}

			nr_frame_pairs = highest_tmp_ref/2 + last_reminder;
			time_to_be  = last_time + (nr_frame_pairs * tystream->tick_diff);

			if(time_to_be >= new_last_time) {

				drift = tystream->drift + (time_to_be - new_last_time);
			} else {
				drift = tystream->drift - (new_last_time - time_to_be);
			}
			if(ty_debug >= 7) {
				LOG_DEVDIAG("Reminder\n");
			}
		} else {

			if(highest_tmp_ref%2) {
				reminder = 0;
				new_last_time = get_time_of_tmp_ref(seq_pes_holder, highest_tmp_ref);
			} else {
				reminder = 1;
				new_last_time = get_time_of_tmp_ref(seq_pes_holder, highest_tmp_ref - 1);
			}

			nr_frame_pairs = (highest_tmp_ref + 1)/2;

			time_to_be  = last_time + (nr_frame_pairs * tystream->tick_diff);

			if(time_to_be >= new_last_time) {

				drift = tystream->drift + (time_to_be - new_last_time);
			} else {
				drift = tystream->drift - (new_last_time - time_to_be);
			}
			if(ty_debug >= 7) {
				LOG_DEVDIAG("No reminder\n");
			}
		}
		if(ty_debug >= 7) {
			LOG_DEVDIAG4("pre last time " I64FORMAT ", new last time " I64FORMAT ", time_to_be " I64FORMAT ", pairs %i, ", \
				last_time, new_last_time, time_to_be,nr_frame_pairs );
			LOG_DEVDIAG1("added time " I64FORMAT "\n", nr_frame_pairs * tystream->tick_diff);
		}
	}

	tystream->reminder = reminder;
	tystream->last_time = new_last_time;
	tystream->drift = drift;

	if(ty_debug >= 7) {
		LOG_DEVDIAG2("SEQ: " I64FORMAT " Drift is: " I64FORMAT " ticks\n", seq_pes_holder->seq_number, tystream->drift);
	}

	if(llabs(tystream->drift) >= (int64_t)tystream->drift_threshold) {
		if(ty_debug >= 7) {
			print_tmp_ref(seq_pes_holder);
		}

		fix_drift(tystream, seq_pes_holder);

		if(ty_debug >= 7) {
			LOG_DEVDIAG2("SEQ: " I64FORMAT " Drift fixed is: " I64FORMAT " ticks\n", seq_pes_holder->seq_number, tystream->drift);
		}

		/*result = check_temporal_reference(seq_pes_holder);

		if(result) {
			if(ty_debug >= 7) {
				print_tmp_ref(seq_pes_holder);
				printf("error in tmp_ref sync check\n");
			}
		} else {
			if(ty_debug >= 7) {
				print_tmp_ref(seq_pes_holder);
			}
		}
		*/
	}

	return(1);
}
 
