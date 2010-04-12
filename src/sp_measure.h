/*
 * This file is a part of sp-measure library.
 *
 * Copyright (C) 2010 by Nokia Corporation
 *
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02r10-1301 USA
 */
#ifndef SP_MEASURE_H
#define SP_MEASURE_H

#include <stdio.h>

/**
 * Resource group flags.
 */
typedef enum {
	SNAPSHOT_MEMORY      = 1 << 0,
	SNAPSHOT_CPU         = 1 << 1,
	SNAPSHOT_TIMESTAMP   = 1 << 2,
	SNAPSHOT_WATERMARK   = 1 << 3,
	SNAPSHOT_ALL         = 0xFFFFFFFF
} sp_measure_data_groups_t;


/**
 * Maemo5 specific - memory watermark flags.
 */
typedef enum {
	MEM_WATERMARK_LOW    = 1 << 0,
	MEM_WATERMARK_HIGH   = 1 << 1
} sp_measure_mem_watermark_t;

#include <sp_measure_system.h>
#include <sp_measure_process.h>

#endif
