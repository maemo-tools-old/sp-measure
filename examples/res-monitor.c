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

/**
 * A simple example demonstrating the usage of sp-measure library.
 * It monitors system (and process if pid is specified) resource usage.
 * The data is printed into console with 1 second interval until the
 * program is aborted with ctrl+c.
 *
 * Compile:
 * 1) example in source package
 *    gcc -I../src -L../src/.libs res-monitor.c -lspmeasure -o res-monitor
 * 2) example installed with libsp-measure-dev
 *    gcc -O -Wall res-monitor.c -lspmeasure -o res-monitor
 *
 * run:
 *    LD_LIBRARY_PATH=../src/.libs ./res-monitor [<pid>]
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sp_measure.h>


int main(int argc, char* argv[])
{
	int monitored_process = 0;
	if (argc > 1) {
		monitored_process = atoi(argv[1]);
	}

	sp_measure_sys_data_t sys_data[2];
	sp_measure_sys_data_t* sys_data1 = &sys_data[0];
	sp_measure_sys_data_t* sys_data2 = &sys_data[1];

	sp_measure_proc_data_t proc_data[2];
	sp_measure_proc_data_t* proc_data1 = &proc_data[0];
	sp_measure_proc_data_t* proc_data2 = &proc_data[1];

	/* initialize system snapshots */
	if (sp_measure_init_sys_data(sys_data1, SNAPSHOT_SYS, NULL) < 0) {
		fprintf(stderr, "Failed to initialize first system snapshot\n");
	}
	if (sp_measure_init_sys_data(sys_data2, 0,  sys_data1) < 0) {
		fprintf(stderr, "Failed to initialize second system snapshot\n");
	}

	/* Initialize process snapshots if necessary */
	if (monitored_process) {
		if (sp_measure_init_proc_data(proc_data1, monitored_process, SNAPSHOT_PROC, NULL) < 0) {
			fprintf(stderr, "Failed to initialize first process snapshot\n");
		}
		if (sp_measure_init_proc_data(proc_data2, 0, 0, proc_data1) < 0) {
			fprintf(stderr, "Failed to initialize second process snapshot\n");
		}
	}

	/* get the initial system snapshot */
	if (sp_measure_get_sys_data(sys_data1, SNAPSHOT_SYS, NULL) < 0) {
		fprintf(stderr, "Failed to get system snapshot\n");
	}
	/* get the initial process snapshot if necessary */
	if (monitored_process && sp_measure_get_proc_data(proc_data1, SNAPSHOT_PROC, NULL) < 0) {
		fprintf(stderr, "Failed to get process snapshot\n");
	}

	/* print report header */
	printf("System:");
	if (monitored_process) {
		printf("                        %d %s", FIELD_PROC_PID(proc_data1), FIELD_PROC_NAME(proc_data1));
	}
	printf("\n");
	printf("used mem: change:  cpu%%: freq:  ");
	if (monitored_process) {
		printf("clean:   dirty:  change:  cpu%%:");
	}
	printf("\n");

	/* loop until aborted */
	while (1) {
		int sys_mem_change, sys_cpu_usage, sys_cpu_avg_freq;
		/* get the next system snapshot */
		if (sp_measure_get_sys_data(sys_data2, SNAPSHOT_SYS, NULL) < 0) {
			fprintf(stderr, "Failed to get system snapshot\n");
		}
		/* calculate and print resource usage differences from the previous system snapshot */
		sp_measure_diff_sys_mem_used(sys_data1, sys_data2, &sys_mem_change);
		sp_measure_diff_sys_cpu_usage(sys_data1, sys_data2, &sys_cpu_usage);
		sp_measure_diff_sys_cpu_avg_freq(sys_data1, sys_data2, &sys_cpu_avg_freq);
		printf("%8d %+8d %5.1f%% %5d",  FIELD_SYS_MEM_USED(sys_data2), sys_mem_change, (float)sys_cpu_usage / 100, sys_cpu_avg_freq / 1000);

		if (monitored_process) {
			int sys_cpu_ticks, proc_cpu_ticks, proc_mem_change;
			/* get the next process snapshot */
			if (sp_measure_get_proc_data(proc_data2, SNAPSHOT_PROC, NULL) < 0) {
				fprintf(stderr, "Failed to get process snapshot\n");
				exit(-1);
			}
			/* calculate and print resource usage differences from the previous process snapshot */
			sp_measure_diff_sys_cpu_ticks(sys_data1, sys_data2, &sys_cpu_ticks);
			sp_measure_diff_proc_cpu_ticks(proc_data1, proc_data2, &proc_cpu_ticks);
			sp_measure_diff_proc_mem_private_dirty(proc_data1, proc_data2, &proc_mem_change);
			printf("%8d %8d %+8d %5.1f%%", FIELD_PROC_MEM_PRIVATE_CLEAN(proc_data2), FIELD_PROC_MEM_PRIV_DIRTY_SUM(proc_data2), proc_mem_change,
					 sys_cpu_ticks ? (float)proc_cpu_ticks * 100 / sys_cpu_ticks : 0);

		}
		printf("\n");

		/* swap system and process snapshot references - so that the last snapshot becomes previous
		 * snapshot and the next snapshot is written over the previous snapshot becoming
		 * the last snapshot */
		sp_measure_sys_data_t* sys_data_swap = sys_data1;
		sys_data1 = sys_data2;
		sys_data2 = sys_data_swap;

		sp_measure_proc_data_t* proc_data_swap = proc_data1;
		proc_data1 = proc_data2;
		proc_data2 = proc_data_swap;

		sleep(1);
	}

	/* releases resources allocated by snapshots.
	 * As the program is aborted, this is unnecessary.
	 * Still normally every initialized snapshot should be freed afterwards.  */
	sp_measure_free_sys_data(&sys_data[0]);
	sp_measure_free_sys_data(&sys_data[1]);

	sp_measure_free_proc_data(&proc_data[0]);
	sp_measure_free_proc_data(&proc_data[1]);

	return 0;
}
