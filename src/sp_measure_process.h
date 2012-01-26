/*
 * This file is a part of sp-measure library.
 *
 * Copyright (C) 2010 by Nokia Corporation
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
#ifndef SP_MEASURE_PROCESS_H
#define SP_MEASURE_PROCESS_H

/**
 * @file sp_measure_process.h
 * API for process resource usage snapshots.
 *
 * This file contains API providing access to usage statistics of such
 * process resources as CPU, memory etc.
 *
 * Short example (without any error checking):
 * @code
 *    // declare snapshot data structures
 *    sp_measure_proc_data_t data1, data2;
 *    // initialize the first snapshot structure for process with pid 1234
 *    sp_measure_init_proc_data(&data1, 1234, SNAPSHOT_PROC, NULL);
 *    // initialize the second snapshot structure
 *    sp_measure_init_proc_data(&data2, 0, 0, &data2);
 *    // take first snapshot
 *    sp_measure_get_proc_data(&data1, NULL);
 *    // do some actions
 *    ...
 *    // take the second snapshot
 *    sp_measure_get_proc_data(&data2, NULL);
 *    // print statistics:
 *    int value;
 *    printf("Resource usage of the process %d(%s)\n", FIELD_PROC_PID(&data1), FIELD_PROC_NAME(&data2));
 *    sp_measure_diff_proc_mem_private_dirty(&data1, &data2, &value);
 *    printf("\tprivate dirty memory: %+d\n", value);
 *    // release snapshot data structures
 *    sp_measure_free_proc_data(&data1);
 *    sp_measure_free_proc_data(&data2);
 *  @endcode
 *
 *  res-monitor.c.gz file in the package documentation dir
 *  is a working source example of the API usage.
 */
#ifdef __cplusplus
extern "C" {
#endif


/**
 * Common process information.
 */
typedef struct sp_measure_proc_common_t {
	/* pid */
	int pid;
	/* the process name */
	char* name;

	/* path of the /proc/<pid>/smaps file */
	char* proc_smaps_path;
	/* path of the /proc/<pid>/data file */
	char* proc_stat_path;

	/* process common data reference counter */
	int ref_count;
} sp_measure_proc_common_t;

/**
 * Process resource usage snapshot
 */
typedef struct sp_measure_proc_data_t {
	/* common process data, initialized at the beginning and not a subject to change */
	sp_measure_proc_common_t* common;

	/* the snapshot name */
	char* name;

	/* process memory statistics (from /proc/<pid>/smaps) */
	int mem_private_clean;
	int mem_private_dirty;
	int mem_swap;
	int mem_size;
	int mem_shared_clean;
	int mem_shared_dirty;
	int mem_pss;
	int mem_rss;
	int mem_referenced;

	/* system time ticks spent by process */
	int cpu_stime;
	/* user time ticks spent by process */
	int cpu_utime;

} sp_measure_proc_data_t;


/**
 * Initializes process snapshot data structure.
 *
 * If sample_data parameter is NULL the 'global' process parameters like
 * name are retrieved from system. Otherwise they are copied from the
 * sample_data structure.
 * The rest of parameters are reseted to zero values.
 * Afterwards the internal data structure data must be freed with
 * sp_measure_free_proc_data() function.
 * @param[out] new_data    the process snapshot data structure to initialize.
 * @param[in] pid          the process id (ignored if sample_data is given).
 * @param[in] resources    a flag specifying which initial process resource
 *                         statistics should be retrieved (ignored if
 *                         sample_data is given).
 * @param[in] sample_data  An already initialized process snapshot.
 * @return                 0  - success
 *                         >0 - only part of the initial process resource statistics
 *                              was retrieved. The returned value consists of the failed
 *                              resource identifiers (see sp_measure_proc_resource_t
 *                              enumeration).
 *                         <0 - unrecoverable error during process resource statistics
 *                              retrieval. For example process does not exist.
 */
int sp_measure_init_proc_data(
		sp_measure_proc_data_t* new_data,
		int pid,
		int resources,
		const sp_measure_proc_data_t* sample_data
		);


/**
 * Reinitializes process snapshot data structure.
 *
 * This function reinitializes 'global' parameters (like process name) of the process
 * snapshot.
 * @param[in] data   the process snapshot data structure.
 * @return           0 for success.
 */
int sp_measure_reinit_proc_data(
		sp_measure_proc_data_t* data
		);

/**
 * Releases process snapshot data structure.
 *
 * This function releases the resources allocated by sp_measure_init_proc_data() function.
 * @param[in] data   the process statistics data structure to free.
 * @return            0 for success.
 */
int sp_measure_free_proc_data(
		sp_measure_proc_data_t* data
		);

/**
 * Retrieves process resource usage snapshot.
 *
 * The data data structure must be initialized by sp_measure_init_proc_data() or
 * sp_measure_init_cloned_proc_data() functions.
 * @param[out] data      the process statistics data structure.
 * @param[in] resources  a flag specifying which process resource statistics
 *                       should be retrieved
 * @param[in] name       the snapshot name (optional). Can be NULL if snapshot naming
 *                       is not required.
 * @return               0  - success
 *                       >0 - only part of the process resource statistics was retrieved.
 *                            The returned value consists of the failed resource
 *                            identifiers (see sp_measure_proc_resource_t enumeration).
 *                       <0 - unrecoverable error during process resource statistics
 *                            retrieval.
 */
int sp_measure_get_proc_data(
		sp_measure_proc_data_t* data,
		int resources,
		const char* name
		);


/**
 * The process snapshot field comparison functions.
 *
 * All process snapshot field comparison functions have the following signature:
 * int (*)(const sp_measure_proc_data_t* data1, const sp_measure_proc_data_t* data2, int* diff);
 *
 * @param[in] data1    the first snapshot.
 * @param[in] data2    the second snapshot.
 * @param[out] diff    the difference of the field value between snapshots.
 * @return             0 if the difference was calculated successfully.
 */


/**
 * Retrieves process private dirty memory difference (in KB) between two snapshots.
 *
 * @param[in] data1  the first snapshot.
 * @param[in] data2  the second snapshot.
 * @param[out] diff  the memory difference.
 * @return           0 for success.
 */
int sp_measure_diff_proc_mem_private_dirty(
		const sp_measure_proc_data_t* data1,
		const sp_measure_proc_data_t* data2,
		int* diff
		);

/**
 * Retrieves time spent in process between two snapshots.
 *
 * @param[in] data1  the first snapshot.
 * @param[in] data2  the second snapshot.
 * @param[out] diff  the number of cpu ticks spent in process.
 * @return           0 for success.
 */
int sp_measure_diff_proc_cpu_ticks(
		const sp_measure_proc_data_t* data1,
		const sp_measure_proc_data_t* data2,
		int* diff
		);

/*
 * Field access definitions
 */

#define FIELD_PROC_PID(data)                 (data)->common->pid
#define FIELD_PROC_NAME(data)                (data)->common->name
#define FIELD_PROC_MEM_PRIVATE_CLEAN(data)   (data)->mem_private_clean
#define FIELD_PROC_MEM_PRIVATE_DIRTY(data)   (data)->mem_private_dirty
#define FIELD_PROC_MEM_SWAP(data)            (data)->mem_swap
#define FIELD_PROC_MEM_SIZE(data)            (data)->mem_size
#define FIELD_PROC_MEM_PSS(data)             (data)->mem_pss
#define FIELD_PROC_MEM_RSS(data)             (data)->mem_rss
#define FIELD_PROC_MEM_REFERENCED(data)      (data)->mem_referenced
#define FIELD_PROC_MEM_SHARED_CLEAN(data)    (data)->mem_shared_clean
#define FIELD_PROC_MEM_SHARED_DIRTY(data)    (data)->mem_shared_dirty
#define FIELD_PROC_MEM_PRIV_DIRTY_SUM(data)  (FIELD_PROC_MEM_SWAP(data) + FIELD_PROC_MEM_PRIVATE_DIRTY(data))

#define FIELD_PROC_CPU_STIME(data)           (data)->cpu_stime
#define FIELD_PROC_CPU_UTIME(data)           (data)->cpu_utime

#ifdef __cplusplus
}
#endif

#endif
