/*
 * This file is a part of sp-measure library.
 *
 * Copyright (C) 2010 by Nokia Corporation
 *
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02r10-1301 USA
 */
#ifndef MEASURE_UTILS_H
#define MEASURE_UTILS_H


/**
 * Key/value pairs for file parsing.
 */
typedef struct parse_query_t {
	/* the key to look for */
	char* key;
	/* address where value should be written */
	int* value;
	/* true if the value was retrieved */
	bool ok;
} parse_query_t;


/* root of the /proc file system. */
extern char sp_measure_fs_root[];
extern char* sp_measure_virtual_fs_root;


#endif