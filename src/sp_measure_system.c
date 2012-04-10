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
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <ftw.h>
#include <stdint.h>
#include <limits.h>

#include "sp_measure.h"
#include "measure_utils.h"

/*
 * Private API
 */


/* the chunk size must be a power of 2 */
#define CPU_FREQ_TICKS_CHUNK_SIZE	(1 << 5)

/**
 * Set ticks spent at the specified frequency.
 *
 * @param[out] data  the system snapshot.
 * @param[in] freq        the cpu frequency in Hz.
 * @param[in] ticks       the number of ticks spent at the frequency freq.
 * @return                0 for success.
 */
static int cpu_stats_set_freq_ticks(
		sp_measure_sys_data_t* data,
		int freq,
		int ticks
		)
{
	sp_measure_cpu_freq_ticks_t* state = data->cpu_freq_ticks;
	if (state) {
		/* find existing data or free record for the specified frequency */
		while (true) {
			if (state - data->cpu_freq_ticks >= data->cpu_freq_ticks_count) {
				state = NULL;
				break;
			}
			if (state->freq == freq) break;
			state++;
		}
	}
	if (state == NULL) {
		/* A new ticks per freq record must be added.
		   Check if the array has any free space and increase if necessary */
		if (! (data->cpu_freq_ticks_count & (CPU_FREQ_TICKS_CHUNK_SIZE - 1)) ) {
			data->cpu_freq_ticks = (sp_measure_cpu_freq_ticks_t*)
					     realloc(data->cpu_freq_ticks, (data->cpu_freq_ticks_count + CPU_FREQ_TICKS_CHUNK_SIZE) *
					     sizeof(sp_measure_cpu_freq_ticks_t));
			if (data->cpu_freq_ticks == NULL) {
				fprintf(stderr, "Not enough memory to allocate cpu ticks per frequency array of size %d\n",
						data->cpu_freq_ticks_count);
				exit(-1);
			}
		}
		state = &data->cpu_freq_ticks[data->cpu_freq_ticks_count++];
	}
	state->freq = freq;
	state->ticks = ticks;
	return 0;
}

/**
 * Calculates average frequency during the time slice between two snapshots.
 *
 * @param[in] first   the first snapshot.
 * @param[in] second  the second snapshot.
 * @return            average frequency in Hz
 */
static int cpu_stats_diff_avg_freq(
		const sp_measure_sys_data_t* first,
		const sp_measure_sys_data_t* second
		)
{
	int i, j;
	uint64_t total_time = 0;
	uint64_t total_freq = 0;
	for (i = 0; i < second->cpu_freq_ticks_count; i++) {
		uint64_t ticks = 0;
		uint64_t freq = second->cpu_freq_ticks[i].freq;
		for (j = 0; j < first->cpu_freq_ticks_count; j++) {
			if (first->cpu_freq_ticks[i].freq == freq) {
				ticks = first->cpu_freq_ticks[i].ticks;
				break;
			}
		}
		uint64_t diff = second->cpu_freq_ticks[i].ticks - ticks;
		total_time += diff;
		total_freq += freq * diff;
	}
	return total_time ? total_freq / total_time : 0;
}

/**
 * Retrieves values specified in data structure (key field) from
 * /proc/meminfo file.
 *
 * @param[in,out] data    in  - list of keys to read.
 *                        out - the scanned values.
 * @param[in] length      the number of items in data list.
 * @return                the number of values retrieved.
 */
static int file_parse_proc_meminfo(
		parse_query_t* data,
		int length
		)
{
	int nscanned = 0, value = 0, i;
	char key[128], buffer[PATH_MAX];
	snprintf(buffer, sizeof(buffer), "%s/proc/meminfo", sp_measure_virtual_fs_root);
	FILE* fp = fopen(buffer, "r");
	if (fp) {
		while (fgets(buffer, sizeof(buffer), fp) && nscanned < length) {
			if (sscanf(buffer, "%[^:]: %d", key, &value) == 2) {
				for (i = 0; i < length; i++) {
					if (!data[i].ok) {
						if (!strcmp(data[i].key, key)) {
							data[i].ok = true;
							*(data[i].value) = value;
							nscanned++;
							break;
						}
					}
				}
			}
		}
		fclose(fp);
	}
	return nscanned;
}

/**
 * Reads single integer value from a file.
 *
 * @param[in] filename   the file to read.
 * @param[out] value     the read value.
 * @return               0 for success.
 */
static int file_read_int(
		const char* filename,
		int* value
		)
{
	char buffer[PATH_MAX];
	snprintf(buffer, sizeof(buffer), "%s%s", sp_measure_virtual_fs_root, filename);
	int fd = open(buffer, O_RDONLY);
	if (fd != -1) {
		int n = read(fd, buffer, sizeof(buffer) - 1);
		close(fd);
		if (n > 0) {
			buffer[n] = '\0';
			*value = atoi(buffer);
			return 0;
		}
	}
	return -1;
}

/**
 * Reads single int value from a cgroup file with conversion to kbytes.
 *
 * @param[in] filename   the file to read.
 * @param[out] value     the read value.
 * @return               0 for success.
 */
static int cgroup_read_int(
		sp_measure_sys_data_t* data,
		const char* filename
		)
{
	if (data->common->cgroup_root) {
		char buffer[PATH_MAX];
		snprintf(buffer, sizeof(buffer), "%s/%s", data->common->cgroup_root, filename);
		int fd = open(buffer, O_RDONLY);
		if (fd != -1) {
			int n = read(fd, buffer, sizeof(buffer) - 1);
			close(fd);
			if (n > 0) {
				buffer[n] = '\0';
				data->mem_cgroup = (int)(strtoull(buffer, NULL, 10) >> 10);
				return 0;
			}
		}
	}
	data->mem_cgroup = ESPMEASURE_UNDEFINED;
	return -1;
}


/**
 * Initializes common memory parameters.
 *
 * The common parameters are assigned during initialization and not
 * changed during snapshots.
 * @param[out] stats  the system snapshot.
 * @return            0 for success.
 */
static int sys_init_memory_data(
		sp_measure_sys_data_t* data
		)
{
	parse_query_t query[] = {
		{ "MemTotal",  &data->common->mem_total, false },
		{ "SwapTotal", &data->common->mem_swap, false },
	};

	if (file_parse_proc_meminfo(query, ARRAY_ITEMS(query))
			!= ARRAY_ITEMS(query)) {
		int i;
		for (i = 0; i < ARRAY_ITEMS(query); i++) {
			*query[i].value = ESPMEASURE_UNDEFINED;
		}
		return -1;
	}
	return 0;
}

/**
 * Initializes common cpu parameters.
 *
 * The common parameters are assigned during initialization and not
 * changed during snapshots.
 * @param[out] stats  the cpu snapshot.
 * @return            0 for success.
 */
static int sys_init_cpu_data(
		sp_measure_sys_data_t* data
		)
{
	int rc = file_read_int("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", &data->common->cpu_max_freq);
	if (rc) {
		data->common->cpu_max_freq = ESPMEASURE_UNDEFINED;
	}
	return rc;
}


/**
 * Retrieve snapshot cpu ticks.
 *
 * @param[out] stats    the system snapshot.
 * @return              0 for success.
 */
static int sys_get_cpu_ticks_total(
		sp_measure_sys_data_t* stats
		)
{
	char buffer[PATH_MAX];
	snprintf(buffer, sizeof(buffer), "%s/proc/stat", sp_measure_virtual_fs_root);
	FILE* fp = fopen(buffer, "r");
	if (fp) {
		stats->cpu_ticks_total = 0;
		while (fgets(buffer, sizeof(buffer), fp)) {
			if (strstr(buffer, "cpu ")) {
				int index = 0;
				char *ptr = buffer + 3;
				char *token, *saveptr = NULL;
				while (*ptr == ' ') ptr++;
				while ( (token = strtok_r(ptr, " ", &saveptr)) ) {
					int ticks = atoi(token);
					stats->cpu_ticks_total += ticks;
					if (index++ == 3) {
						stats->cpu_ticks_idle = ticks;
					}
					ptr = NULL;
				}

			}
		}
		fclose(fp);
		return 0;
	}
	stats->cpu_ticks_total = ESPMEASURE_UNDEFINED;
	stats->cpu_ticks_idle = ESPMEASURE_UNDEFINED;
	return -1;
}

/**
 * Retrieves ticks per frequency statistics for the first cpu.
 *
 * @param[out] stats   the cpu snapshot.
 * @return             0 for success.
 */
static int sys_get_cpu_ticks_per_freq(
		sp_measure_sys_data_t* stats
		)
{
	char buffer[PATH_MAX];
	snprintf(buffer, sizeof(buffer), "%s/sys/devices/system/cpu/cpu0/cpufreq/stats/time_in_state", sp_measure_virtual_fs_root);
	FILE* fp = fopen(buffer, "r");
	if (fp) {
		int freq, ticks;
		while (fgets(buffer, sizeof(buffer), fp)) {
			if (sscanf(buffer, "%d %d", &freq, &ticks) == 2) {
				cpu_stats_set_freq_ticks(stats, freq, ticks);
			}
		}
		fclose(fp);
		return 0;
	}
	return -1;
}


/**
 * Frees the common system data.
 * @param common
 */
static void sys_data_free_common(sp_measure_sys_common_t* common)
{
	if (common->cgroup_root) free(common->cgroup_root);
	free(common);
}

/*
 * Public API
 */
int sp_measure_init_sys_data(
		sp_measure_sys_data_t* new_data,
		int resources,
		const sp_measure_sys_data_t* sample_data
		)
{
	int rc = 0;
	memset(new_data, 0, sizeof(sp_measure_sys_data_t));
	if (sample_data) {
		new_data->common = sample_data->common;
		new_data->common->ref_count++;
	}
	else {
		new_data->common = (sp_measure_sys_common_t*)malloc(sizeof(sp_measure_sys_common_t));
		if (new_data->common == NULL) return -ENOMEM;
		memset(new_data->common, 0, sizeof(sp_measure_sys_common_t));
		new_data->common->ref_count = 1;
		if ( (resources & SNAPSHOT_SYS_MEM_TOTALS) && sys_init_memory_data(new_data) != 0) rc |= SNAPSHOT_SYS_MEM_TOTALS;
		if ( (resources & SNAPSHOT_SYS_CPU_MAX_FREQ) && sys_init_cpu_data(new_data) != 0) rc |= SNAPSHOT_SYS_CPU_MAX_FREQ;
		if ( (resources & SNAPSHOT_SYS_MEM_CGROUPS) ) sp_measure_cgroup_select(new_data, NULL);
	}
	return rc;
}

int sp_measure_free_sys_data(
        sp_measure_sys_data_t* data
    )
{
	if (data->name) free(data->name);
	if (data->cpu_freq_ticks) free(data->cpu_freq_ticks);
	if (--data->common->ref_count == 0) {
		sys_data_free_common(data->common);
	}
	return 0;
}


static const char* cgroup_pattern = NULL;
static char* cgroup_root = NULL;

static int cgroup_match(const char *fpath, const struct stat *sb, int tflag)
{
    if (FTW_D == tflag && strstr(fpath, cgroup_pattern)) {
		cgroup_root = strdup(fpath);
		return 1;
	}
	return 0; /* To tell ftw() to continue */
}


const char* sp_measure_cgroup_select(sp_measure_sys_data_t* data, const char* name)
{
	static const char* root = "/syspart";
	cgroup_root = NULL;

	if (NULL != name && 0 != *name) {
		cgroup_pattern = name;
		ftw(root, cgroup_match, 32);
	}
	if (data->common->cgroup_root) free(data->common->cgroup_root);
	data->common->cgroup_root = cgroup_root ? cgroup_root : strdup(root);
	return data->common->cgroup_root;
}

int sp_measure_get_sys_data(
		sp_measure_sys_data_t* data,
		int resources,
		const char* name
		)
{
	int rc = 0;
	if (name) {
		if (data->name) free(data->name);
		data->name = strdup(name);
		if (data->name == NULL) return -ENOMEM;
	}
	if (resources & SNAPSHOT_SYS_TIMESTAMP) {
		struct timeval tv;
		if ( (rc = gettimeofday(&tv, NULL)) != 0) return rc;
		data->timestamp = tv.tv_sec % (60 * 60 * 24) * 1000 + tv.tv_usec / 1000;
	}
	if (resources & SNAPSHOT_SYS_MEM_USAGE) {
		parse_query_t query[] = {
			{ "MemFree",  &data->mem_free, false },
			{ "Buffers", &data->mem_buffers, false },
			{ "Cached", &data->mem_cached, false },
			{ "SwapCached", &data->mem_swap_cached, false },
			{ "SwapFree", &data->mem_swap_free, false },
		};
		if (file_parse_proc_meminfo(query, ARRAY_ITEMS(query))
				!= ARRAY_ITEMS(query)) {
			int i;
			for (i = 0; i < ARRAY_ITEMS(query); i++) {
				*query[i].value = ESPMEASURE_UNDEFINED;
			}
			rc |= SNAPSHOT_SYS_MEM_USAGE;
		}
	}
	if (resources & SNAPSHOT_SYS_MEM_CGROUPS) {
		if (cgroup_read_int(data, "memory.memsw.usage_in_bytes") != 0) {
			rc |= SNAPSHOT_SYS_MEM_CGROUPS;
		}
	}
	if (resources & SNAPSHOT_SYS_MEM_WATERMARK) {
		int low = 0, high = 0;
		if (file_read_int("/sys/kernel/low_watermark", &low) != 0) {
			rc |= SNAPSHOT_SYS_MEM_WATERMARK;
		}
		if (file_read_int("/sys/kernel/high_watermark", &high) != 0) {
			rc |= SNAPSHOT_SYS_MEM_WATERMARK;
		}
		data->mem_watermark = low | (high << 1);
	}
	if ( (resources & SNAPSHOT_SYS_CPU_USAGE) && sys_get_cpu_ticks_total(data) != 0) {
		rc |= SNAPSHOT_SYS_CPU_USAGE;
	}
	if ( (resources & SNAPSHOT_SYS_CPU_FREQ) && sys_get_cpu_ticks_per_freq(data) != 0) {
		rc |= SNAPSHOT_SYS_CPU_FREQ;
	}
	return rc;
}


/*
 * Field comparison functions.
 */

int sp_measure_diff_sys_timestamp(
		const sp_measure_sys_data_t* data1,
		const sp_measure_sys_data_t* data2,
		int* diff
		)
{
	if (data1->common != data2->common) {
		return -EINVAL;
	}
	*diff = data2->timestamp - data1->timestamp;
	if (*diff < 0) {
		*diff += 24 * 60 * 60 * 1000;
	}
	return 0;
}

int sp_measure_diff_sys_cpu_ticks(
		const sp_measure_sys_data_t* data1,
		const sp_measure_sys_data_t* data2,
		int* diff
		)
{
	if (data1->common != data2->common) {
		return -EINVAL;
	}
	if (data2->cpu_ticks_total == ESPMEASURE_UNDEFINED || data1->cpu_ticks_total == ESPMEASURE_UNDEFINED) {
		return -EINVAL;
	}
	*diff = data2->cpu_ticks_total - data1->cpu_ticks_total;
	return 0;
}

int sp_measure_diff_sys_cpu_usage(
		const sp_measure_sys_data_t* data1,
		const sp_measure_sys_data_t* data2,
		int* diff
		)
{
	if (data1->common != data2->common) {
		return -EINVAL;
	}
	if (data2->cpu_ticks_total == ESPMEASURE_UNDEFINED || data1->cpu_ticks_total == ESPMEASURE_UNDEFINED ||
			data2->cpu_ticks_idle == ESPMEASURE_UNDEFINED || data1->cpu_ticks_idle == ESPMEASURE_UNDEFINED) {
		return -EINVAL;
	}
	int total_ticks_diff = data2->cpu_ticks_total - data1->cpu_ticks_total;
	*diff = total_ticks_diff ? (total_ticks_diff - (data2->cpu_ticks_idle - data1->cpu_ticks_idle)) * 10000 / total_ticks_diff : 0;
	return 0;
}

int sp_measure_diff_sys_cpu_avg_freq(
		const sp_measure_sys_data_t* data1,
		const sp_measure_sys_data_t* data2,
		int* diff
		)
{
	if (data1->common != data2->common) {
		return -EINVAL;
	}
	*diff = cpu_stats_diff_avg_freq(data1, data2);
	return 0;
}

int sp_measure_diff_sys_mem_used(
		const sp_measure_sys_data_t* data1,
		const sp_measure_sys_data_t* data2,
		int* diff
		)
{
	if (data1->common != data2->common) {
		return -EINVAL;
	}
	if (data1->common->mem_total == ESPMEASURE_UNDEFINED || data1->mem_free == ESPMEASURE_UNDEFINED ||
			data2->mem_free == ESPMEASURE_UNDEFINED) {
		return -EINVAL;
	}
	*diff = FIELD_SYS_MEM_USED(data2) - FIELD_SYS_MEM_USED(data1);
	return 0;
}

int sp_measure_diff_sys_mem_cgroup(
		const sp_measure_sys_data_t* data1,
		const sp_measure_sys_data_t* data2,
		int* diff
		)
{
	if (data1->common != data2->common) {
		return -EINVAL;
	}
	*diff = FIELD_SYS_MEM_CGROUP(data2) - FIELD_SYS_MEM_CGROUP(data1);
	return 0;
}


int sp_measure_set_fs_root(const char* path)
{
	if (sp_measure_virtual_fs_root != sp_measure_fs_root) {
		free(sp_measure_virtual_fs_root);
	}
	sp_measure_virtual_fs_root = path ? strdup(path) : sp_measure_fs_root;
	if (sp_measure_virtual_fs_root) {
		return 0;
	}
	return -1;
}
