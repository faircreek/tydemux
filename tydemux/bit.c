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



int getbit(uint8_t *set, int number) {

	int bit;

	bit = 7 - (number % 8);

	set += number / 8;
	/* 0 or 1       */
        return (*set & (1 << bit)) != 0;

}

void setbit(uint8_t *set, int number, int value) {

	int bit;

	bit = 7 - (number % 8);

	set += number / 8;

	if (value) {
		/* set bit      */
                *set |= 1 << bit;
	} else {
		 /* clear bit    */
		*set &= ~(1 << bit);
	}
}


