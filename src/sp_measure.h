/*
 * This file is a part of sp-measure library.
 *
 * Copyright (C) 2010-2012 by Nokia Corporation
 *
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#ifndef SP_MEASURE_H
#define SP_MEASURE_H

#include <stdio.h>

/**
 * System resource identifiers and their generic groupings.
 */
typedef enum {
	SNAPSHOT_SYS_TIMESTAMP       = 1 << 0,
	SNAPSHOT_SYS_MEM_TOTALS      = 1 << 1,
	SNAPSHOT_SYS_MEM_USAGE       = 1 << 2,
	SNAPSHOT_SYS_MEM_WATERMARK   = 1 << 3,
	SNAPSHOT_SYS_CPU_MAX_FREQ    = 1 << 4,
	SNAPSHOT_SYS_CPU_USAGE       = 1 << 5,
	SNAPSHOT_SYS_CPU_FREQ        = 1 << 6,
	SNAPSHOT_SYS_MEM_CGROUPS     = 1 << 7,
	SNAPSHOT_SYS_MEM             = SNAPSHOT_SYS_MEM_TOTALS | SNAPSHOT_SYS_MEM_USAGE | SNAPSHOT_SYS_MEM_WATERMARK,
	SNAPSHOT_SYS_CPU             = SNAPSHOT_SYS_CPU_MAX_FREQ | SNAPSHOT_SYS_CPU_USAGE | SNAPSHOT_SYS_CPU_FREQ,
	SNAPSHOT_SYS                 = SNAPSHOT_SYS_TIMESTAMP | SNAPSHOT_SYS_CPU | SNAPSHOT_SYS_MEM
} sp_measure_sys_resource_t;


/**
 * Process resource identifiers.
 */
typedef enum {
	SNAPSHOT_PROC_MEM_USAGE      = 1 << 0,
	SNAPSHOT_PROC_CPU_USAGE      = 1 << 1,
	SNAPSHOT_PROC_MEM            = SNAPSHOT_PROC_MEM_USAGE,
	SNAPSHOT_PROC_CPU            = SNAPSHOT_PROC_CPU_USAGE,
	SNAPSHOT_PROC                = SNAPSHOT_PROC_MEM | SNAPSHOT_PROC_CPU
} sp_measure_proc_resource_t;


/**
 * Maemo5 specific - MEM watermark flags.
 */
typedef enum {
	MEM_WATERMARK_LOW    = 1 << 0,
	MEM_WATERMARK_HIGH   = 1 << 1
} sp_measure_mem_watermark_t;

#define ESPMEASURE_UNDEFINED		(-1)

#include <sp_measure_system.h>
#include <sp_measure_process.h>

#endif
