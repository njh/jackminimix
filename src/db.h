/*
 *  db.h
 *
 *  Copyright (C) 2003,2005 Steve Harris, Nicholas Humfrey
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  Originally Stolen from JAMIN <http://jamin.sf.net/>
 */

#ifndef DB_H
#define DB_H

static inline float
db2lin( float db )
{
	if (db <= -90.0f) return 0.0f;
	else {
		return powf(10.0f, db * 0.05f);
	}
}

static inline float
lin2db( float lin )
{
	if (lin == 0.0f) return -90.0f;
	else return (20.0f * log10f(lin));
}

#endif
