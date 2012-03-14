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

#include "sp_measure.h"
#include "measure_utils.h"

/*
 * Private API
 */


/**
 * Calculates process clean and dirty memory.
 *
 * The clean and dirty memory is calculated by simply parsing
 * /proc/<pdi>/smaps file.
 * @param stats[in, out]   in  - the smaps file pointer.
 *                         out - memory statistics.
 * @return                 0 for success.
 */
static int file_parse_proc_smaps(
		sp_measure_proc_data_t* data
		)
{
	char buffer[128], key[128];
	int value, i;
	parse_query_t query[] = {
			{"Private_Clean", &data->mem_private_clean, false},
			{"Private_Dirty", &data->mem_private_dirty, false},
			{"Swap", &data->mem_swap, false},
			{"Shared_Clean", &data->mem_shared_clean, false},
			{"Shared_Dirty", &data->mem_shared_dirty, false},
			{"Size", &data->mem_size, false},
			{"Pss", &data->mem_pss, false},
			{"Rss", &data->mem_rss, false},
			{"Referenced", &data->mem_referenced, false},
		};
	for (i = 0; i < ARRAY_ITEMS(query); i++) {
		*(query[i].value) = 0;
	}
	FILE* fp = fopen(data->common->proc_smaps_path, "r");
	if (fp) {
		while (fgets(buffer, sizeof(buffer), fp)) {
			if (sscanf(buffer, "%[^:]: %d", key, &value) == 2) {
				for (i = 0; i < ARRAY_ITEMS(query); i++) {
					if (!strcmp(query[i].key, key)) {
						*(query[i].value) += value;
						break;
					}
				}
			}
		}
		fclose(fp);
		return 0;
	}
	for (i = 0; i < ARRAY_ITEMS(query); i++) {
		*(query[i].value) = ESPMEASURE_UNDEFINED;
	}
	return -1;
}

/**
 * Get cpu statistics from /proc/<pid>/stat file.
 *
 * @return stats[in, out]   in  - the /proc/<pid>/stat file pointer.
 *                          out - the cpu statistics.
 * @return                  0 for success.
 */
static int file_parse_proc_stat(
		sp_measure_proc_data_t* data
		)
{
	int rc = -1;
	int fd = open(data->common->proc_stat_path, O_RDONLY);
	if (fd != -1) {
		char* ptr = NULL;
		unsigned idx = 2;
		char buffer[1024];
		int n = read(fd, buffer, sizeof(buffer) - 1);
		if (n < 0) {
			rc = -errno;
		}
		else {
			if (n >= sizeof(buffer)) n = sizeof(buffer) - 1;
			buffer[n] = '\0';

			if ((ptr = strrchr(buffer, ')')) != NULL) {
				ptr++;
				while ((ptr = strchr(ptr + 1, ' ')) != NULL) {
					++idx;
					if (idx == 13) {
						int a = 0;
						if ( (a = sscanf(ptr, "%u", &data->cpu_utime)) != 1) {
							break;
						}
					}
					else if (idx == 14) {
						if (sscanf(ptr, "%u", &data->cpu_stime) != 1) {
							break;
						}
						rc = 0;
						break;
					}
				}
			}
		}
		close(fd);
	}
	if (rc) {
		data->cpu_utime = ESPMEASURE_UNDEFINED;
		data->cpu_stime = ESPMEASURE_UNDEFINED;
	}
	return rc;
}


/**
 * Retrieves process name.
 *
 * @param[in] pid   the process identifier.
 * @return          allocated string containing process name or NULL if
 *                  process name could not be retrieved.
 */
char* get_process_name(int pid)
{
	int fd, n;
	char buffer[512];
	char* proc_name = NULL;

	sprintf(buffer, "%s/proc/%d/cmdline", sp_measure_virtual_fs_root, pid);
	fd = open(buffer, O_RDONLY);
	if (fd == -1) {
		sprintf(buffer, "%s/proc/%d/status", sp_measure_virtual_fs_root, pid);
		FILE* fp = fopen(buffer, "r");
		if (fp) {
			char name[256];
			if (fgets(buffer, sizeof(buffer), fp)) {
				if (sscanf(buffer, "Name: %s\n", name) == 1) {
					sprintf(buffer, "[%s]", name);
					proc_name = strdup(buffer);
				}
			}
			fclose(fp);
		}
	}
	else {
		n = read(fd, buffer, sizeof(buffer) - 1);
		if (n > 0) {
			buffer[n] = '\0';
			char *pstart;
			char* ptr = strrchr(buffer, '/');
			if (ptr == NULL) {
				ptr = buffer;
				pstart = ptr;
			}
			else {
				pstart = ptr + 1;
			}
			while (ptr - buffer < n) {
				if (*ptr == '\0') {
					if (*(ptr + 1) == '\0') break;
					*ptr = ' ';
				}
				ptr++;
			}
			if (*pstart) proc_name = strdup(pstart);
		}
		close(fd);
	}
	return proc_name;
}

/*
 * Public API
 */
int sp_measure_init_proc_data(
		sp_measure_proc_data_t* new_data,
		int pid,
		int resources,
		const sp_measure_proc_data_t* sample_data		)
{
	memset(new_data, 0, sizeof(sp_measure_proc_data_t));
	if (sample_data) {
		new_data->common = sample_data->common;
		new_data->common->ref_count++;
	}
	else
	{
		char buffer[256];
		new_data->common = (sp_measure_proc_common_t*)malloc(sizeof(sp_measure_proc_common_t));
		if (new_data->common == NULL) return -ENOMEM;

		new_data->common->pid = pid;
		new_data->common->ref_count = 1;

		/* open data files */
		sprintf(buffer, "%s/proc/%d/smaps", sp_measure_virtual_fs_root, pid);
		new_data->common->proc_smaps_path = strdup(buffer);
		if (new_data->common->proc_smaps_path == NULL) return -ENOMEM;

		sprintf(buffer, "%s/proc/%d/stat", sp_measure_virtual_fs_root, pid);
		new_data->common->proc_stat_path = strdup(buffer);
		if (new_data->common->proc_stat_path == NULL) return -ENOMEM;

		/* read process name */
		new_data->common->name = get_process_name(pid);
	}
	return 0;
}


int sp_measure_reinit_proc_data(
		sp_measure_proc_data_t* data
		)
{
	if (data->common->name) free(data->common->name);
	data->common->name = get_process_name(data->common->pid);

	return 0;
}

int sp_measure_free_proc_data(
		sp_measure_proc_data_t* data
		)
{
	if (data->name) free(data->name);
	if (--data->common->ref_count == 0) {
		if (data->common->name) free(data->common->name);
		if (data->common->proc_smaps_path) free(data->common->proc_smaps_path);
		if (data->common->proc_stat_path) free(data->common->proc_stat_path);
		free(data->common);
	}
	return 0;
}


int sp_measure_get_proc_data(
		sp_measure_proc_data_t* data,
		int resources,
		const char* name
		)
{
	int rc = 0;
	/* first check if the process still exists */
	if (access(data->common->proc_stat_path, F_OK) != 0) {
		return -1;
	}
	if (name) {
		if (data->name) free(data->name);
		data->name = strdup(name);
		if (data->name == NULL) return -ENOMEM;
	}
	if ( (resources & SNAPSHOT_PROC_MEM_USAGE) && file_parse_proc_smaps(data) != 0) {
		rc |= SNAPSHOT_PROC_MEM_USAGE;
	}
	if ( (resources & SNAPSHOT_PROC_CPU_USAGE) && file_parse_proc_stat(data) != 0) {
		rc |= SNAPSHOT_PROC_CPU_USAGE;
	}
	return rc;
}


/*
 * Field comparison functions.
 */

int sp_measure_diff_proc_mem_private_dirty(
		const sp_measure_proc_data_t* data1,
		const sp_measure_proc_data_t* data2,
		int* diff
		)
{
	if (data1->common != data2->common) {
		return -EINVAL;
	}
	if (data1->mem_private_dirty == ESPMEASURE_UNDEFINED || data2->mem_private_dirty == ESPMEASURE_UNDEFINED) {
		/* either both memory data (private_dirty and mem_swap) are retrieved
		 * or none at all. So its enough to check only one for invalid value  */
		return -EINVAL;
	}
	*diff = (data2->mem_private_dirty + data2->mem_swap) - (data1->mem_private_dirty + data1->mem_swap);
	return 0;
}


int sp_measure_diff_proc_cpu_ticks(
		const sp_measure_proc_data_t* data1,
		const sp_measure_proc_data_t* data2,
		int* diff
		)
{
	if (data1->common != data2->common) {
		return -EINVAL;
	}
	if (data1->cpu_stime == ESPMEASURE_UNDEFINED || data2->cpu_stime == ESPMEASURE_UNDEFINED) {
		/* either cpu statistics (stime and utime) are retrieved or none
		 * or none at all. So its enough to check only one for invalid value  */
		return -EINVAL;
	}
	*diff = (data2->cpu_stime + data2->cpu_utime) - (data1->cpu_stime + data1->cpu_utime);
	return 0;
}

