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

uint16_t get_temporal_reference(const uint8_t * picture) {


	uint16_t return_value;

	return_value = (picture[4] << 2) + (picture[5] >> 6);

	LOG_DEVDIAG1("Temporal ref is: %i\n", return_value);

	return(return_value);

}


void set_temporal_reference(uint8_t * picture, uint16_t temp_ref) {

	//printf("Set %i\n", temp_ref);

	//printf("Picture 4 before %02x\n", picture[4]);
	picture[4] = (temp_ref >> 2);
	//printf("Picture 4 after %02x\n", picture[4]);

	//printf("Picture 5 before %02x\n", picture[5]);
	picture[5] = (temp_ref << 6) | (picture[5] & 0x3f);
	//printf("Picture 5 after %02x\n", picture[5]);

	//printf("Result %i\n", get_temporal_reference(picture));
}

