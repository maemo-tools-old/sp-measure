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
#ifndef SP_MEASURE_SYSTEM_H
#define SP_MEASURE_SYSTEM_H

/** @file sp_measure_system.h
 * API for system resource usage snapshots.
 *
 * This file contains API providing access to usage statistics of such
 * system resources as CPU, memory, time etc.
 *
 * Short example (without any error checking):
 * @code
 *    // declare snapshot data structures
 *    sp_measure_sys_data_t data1, data2;
 *    // initialize the first snapshot structure
 *    sp_measure_init_sys_data(&data1, SNAPSHOT_SYS, NULL);
 *    // initialize the second snapshot structure
 *    sp_measure_init_sys_data(&data2, 0, &data2);
 *    // take first snapshot
 *    sp_measure_get_sys_data(&data1, NULL);
 *    // do some actions
 *    ...
 *    // take the second snapshot
 *    sp_measure_get_sys_data(&data2, NULL);
 *    // print statistics:
 *    int value;
 *    sp_measure_diff_sys_timestamp(&data1, &data2, &value);
 *    printf("Resource usage difference during %d seconds\n", (float)value / 1000);
 *    sp_measure_diff_sys_mem_used(&data1, &data2, &value);
 *    printf("\tmemory: %+d\n", value);
 *    sp_measure_diff_sys_cpu_usage(&data1, &data2, &value);
 *    printf("\tcpu usage: %f5.1%%\n", (float)value / 100);
 *    sp_measure_diff_sys_cpu_avg_freq(&data1, &data2, &value);
 *    printf("\tcpu average frequency: %f5.1MHz\n", (float)value / 1000);
 *    // release snapshot data structures
 *    sp_measure_free_sys_data(&data1);
 *    sp_measure_free_sys_data(&data2);
 * @endcode
 *
 *  res-monitor.c.gz file in the package documentation dir
 *  is a working source example of the API usage.
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Common system information.
 */
typedef struct sp_measure_sys_common_t {
	/* the total system memory */
	int mem_total;
	/* the total swap memory */
	int mem_swap;

	/* maximum cpu frequency in KHz */
	int cpu_max_freq;

	/* system common data reference counter */
	int ref_count;

	/* root of the cgroups file system. */
	char* cgroup_root;
} sp_measure_sys_common_t;

/**
 * Number of cpu ticks (ticks) spent at frequency (freq).
 */
typedef struct sp_measure_cpu_freq_ticks_t {
	/* the cpu frequency in KHz */
	int freq;
	/* number of cpu ticks spent at frequency freq */
	int ticks;
} sp_measure_cpu_freq_ticks_t;

/**
 * System resource usage snapshot
 */
typedef struct sp_measure_sys_data_t {
	/* common system data, initialized at the beginning and not a subject to change */
	sp_measure_sys_common_t* common;

	/* the snapshot name */
	char* name;

	/* The snapshot timestamp in milliseconds since midnight.
	 * The milliseconds resolution was deemed satisfactory as the
	 * wall clock times can differ wildly due to system activity
	 * and time spent within the process is provided by kernel
	 * only as CPU ticks i.e. at 10ms accuracy */
	int timestamp;

	/* the unused system memory */
	int mem_free;
	/* memory used for file buffers */
	int mem_buffers;
	/* memory used for caching */
	int mem_cached;

	/* unused swap memory */
	int mem_swap_free;
	/* swap memory used for caching */
	int mem_swap_cached;

	/* contents of memory.memsw.usage_in_bytes for specified cgroup or root group */
	int mem_cgroup;

	/* Maemo5 specific memory watermarks,
	 * see sp_measure_mem_watermark_t enumeration */
	int mem_watermark;

	/* total cpu ticks */
	int cpu_ticks_total;
	/* idle cpu ticks */
	int cpu_ticks_idle;
	/* ticks per frequencies */
	sp_measure_cpu_freq_ticks_t* cpu_freq_ticks;
	/* number of used items in freq_ticks array */
	int cpu_freq_ticks_count;

} sp_measure_sys_data_t;


/**
 * Initializes system snapshot data structure.
 *
 * If sample_data parameter is NULL this function reads 'global' system
 * parameters like total memory, max cpu frequency etc. from system.
 * Otherwise those parameters are copied from the sample_data snapshot.
 * The rest of parameters are reseted to zero values.
 * Afterwards the internal data structure data must be freed with
 * sp_measure_free_sys_data() function.
 * @param[out] new_data    the system snapshot data structure to initialize.
 * @param[in] resources    a flag specifying which system parameters should be
 *                         retrieved (ignored if sample_data is given).
 * @param[in] sample_data  An already initialized system snapshot.
 * @return                 0  - success
 *                         >0 - only a part of the 'global' system parameters
 *                              was retrieved. The returned value consists of the failed
 *                              resource identifiers (see sp_measure_sys_resource_t
 *                              enumeration).
 *                         <0 - unrecoverable error during system parameter retrieval.
 *
 */
int sp_measure_init_sys_data(
		sp_measure_sys_data_t* new_data,
		int resources,
		const sp_measure_sys_data_t* sample_data
		);


/**
 * Releases system snapshot data structure.
 *
 * This function releases the resources allocated by sp_measure_init_sys_data()
 * and sp_measure_init_cloned_sys_data() functions.
 * @param[in] data   the system snapshot data structure to free.
 * @return           0 for success.
 */
int sp_measure_free_sys_data(
		sp_measure_sys_data_t* data
		);

/**
 * select cgroup based on its name.
 *
 * This functions scans /syspart folder contents to find proper cgroup by name.
 * @param[in] data   the system snapshot data structure.
 * @param[in] name   the system cgroup to be selected.
 * @return           the full path of selected cgroup.
 */
const char* sp_measure_cgroup_select(sp_measure_sys_data_t* data, const char* name);

/**
 * Takes system resource usage snapshot.
 *
 * The data structure must be initialized by sp_measure_init_sys_data() function.
 * @param[out] data      the system resource usage snapshot.
 * @param[in] resources  a flag specifying resources which usage statistics should be retrieved.
 * @param[in] name       the snapshot name (optional). Can be NULL.
 * @return               0  - success
 *                       >0 - only a part of the system resource usage statistics was retrieved.
 *                            The returned value consists of the failed resource identifiers
 *                            (see sp_measure_sys_resource_t enumeration).
 *                       <0 - unrecoverable error during resource usage statistics retrieval.
 */
int sp_measure_get_sys_data(
		sp_measure_sys_data_t* data,
		int resources,
		const char* name
		);



/*
 * The system snapshot field comparison functions.
 *
 * All system snapshot field comparison functions have the following signature:
 * int (*)(const sp_measure_sys_data_t* data1, const sp_measure_sys_data_t* data2, int* diff);
 *
 * @param[in] data1    the first snapshot.
 * @param[in] data2    the second snapshot.
 * @param[out] diff    the difference of the field value between snapshots.
 * @return             0 if the difference was calculated successfully.
 */

/**
 * Retrieves time difference between two snapshots (in milliseconds).
 *
 * @param[in] data1  the first snapshot.
 * @param[in] data2  the second snapshot.
 * @param[out] diff  the time difference in milliseconds.
 * @return           0 for success.
 */
int sp_measure_diff_sys_timestamp(
		const sp_measure_sys_data_t* data1,
		const sp_measure_sys_data_t* data2,
		int* diff
		);

/**
 * Retrieves number of total cpu ticks happened between two snapshots.
 *
 * @param[in] data1  the first snapshot.
 * @param[in] data2  the second snapshot.
 * @param[out] diff  the difference of total cpu ticks.
 * @return           0 for success.
 */
int sp_measure_diff_sys_cpu_ticks(
		const sp_measure_sys_data_t* data1,
		const sp_measure_sys_data_t* data2,
		int* diff
		);

/**
 * Retrieves cpu usage during the time slice between two snapshots.
 *
 * The cpu usage is measured as (% of cpu used) * 100
 * @param[in] data1  the first snapshot.
 * @param[in] data2  the second snapshot.
 * @param[out] diff  the cpu usage data.
 * @return           0 for success.
 */
int sp_measure_diff_sys_cpu_usage(
		const sp_measure_sys_data_t* data1,
		const sp_measure_sys_data_t* data2,
		int* diff
		);

/**
 * Retrieves average cpu frequency during the time slice between two snapshots.
 *
 * The cpu frequency is measured in KHz.
 * @param[in] data1  the first snapshot.
 * @param[in] data2  the second snapshot.
 * @param[out] diff  the average cpu frequency.
 * @return           0 for success.
 *
 */
int sp_measure_diff_sys_cpu_avg_freq(
		const sp_measure_sys_data_t* data1,
		const sp_measure_sys_data_t* data2,
		int* diff
		);

/**
 * Retrieves system memory usage difference (in kB) between two snapshots.
 *
 * @param[in] data1  the first snapshot.
 * @param[in] data2  the second snapshot.
 * @param[out] diff  the memory difference.
 * @return           0 for success.
 */
int sp_measure_diff_sys_mem_used(
		const sp_measure_sys_data_t* data1,
		const sp_measure_sys_data_t* data2,
		int* diff
		);


/**
 * Retrieves system cgroup memory usage difference between two snapshots.
 *
 * @param[in] data1  the first snapshot.
 * @param[in] data2  the second snapshot.
 * @param[out] diff  the memory difference.
 * @return           0 for success.
 */
int sp_measure_diff_sys_mem_cgroup(
		const sp_measure_sys_data_t* data1,
		const sp_measure_sys_data_t* data2,
		int* diff
		);

/**
 * Sets root of the /proc file system.
 *
 * This function allows to override the default proc file system root
 * /proc with a custom value.  Use NULL path to reset it to the default
 * value.
 * This function can be used for getting measurements out of saved
 * /proc/ & /sys/ files, and for testing purposes.
 * @param[in] path   the new root of proc file system. Use NULL to
 *                   reset to the default value /proc.
 * @return           0 for success.
 */
extern int sp_measure_set_fs_root(const char* path);


/*
 * Field access definitions
 */
#define FIELD_SYS_MEM_USED(data)             ((data)->common->mem_total + (data)->common->mem_swap -\
                                             (data)->mem_free - (data)->mem_cached - (data)->mem_buffers -\
                                             (data)->mem_swap_free - (data)->mem_swap_cached)
#define FIELD_SYS_MEM_TOTAL(data)            (data)->common->mem_total
#define FIELD_SYS_MEM_SWAP(data)             (data)->common->mem_swap
#define FIELD_SYS_MEM_WATERMARK(data)        (data)->mem_watermark
#define FIELD_SYS_CPU_MAX_FREQ(data)         (data)->common->cpu_max_freq
#define FIELD_SYS_CPU_TICKS(data)            (data)->cpu_total_ticks
#define FIELD_SYS_TIMESTAMP(data)            (data)->timestamp
#define FIELD_SYS_MEM_CGROUP(data)           (data)->mem_cgroup

#ifdef __cplusplus
}
#endif

#endif
