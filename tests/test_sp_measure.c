/*
 * This file is a spart of sp-measure library.
 *
 * Copyright (C) 2010,2012 by Nokia Corporation
 *
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

/**
 * Simple measure API tests.
 *
 * As most of measure API data is retrieved by parsing /proc/ files,
 * it's possible to test it by setting fake rootfs system and checking
 * if the retrived values are correct.
 * Because process snapshots stores the path to required /proc/ files
 * (currently smaps and stats), changing rootfs after snapshot initialization
 * will have no effect. This leads to a hackish approach to build the
 * second snapshot for comparison tests.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sp_measure.h>

#define TEST(expression, ...) if (!(expression)) {fprintf(stderr, "[failure] " #expression "\n" __VA_ARGS__ ); exit(-1);}

#define TEST_VALUE_INT(expression, value)	TEST(expression == value, "\t" #expression "=%d\n", expression)
#define TEST_VALUE_STR(expression, value)	TEST(strcmp(expression, value) == 0, "\t" #expression "=%s\n", expression)

#define SNAPSHOT_TEST_SYS  (SNAPSHOT_SYS | SNAPSHOT_SYS_MEM_WATERMARK)
#define SNAPSHOT_TEST_PROC (SNAPSHOT_PROC)


void check_system_api()
{
	sp_measure_sys_data_t data1, data2, data3;

	/* set the fake rootfs */
	sp_measure_set_fs_root("./rootfs1");

	/* initialize data1, data2 snapshot structures */
	TEST(sp_measure_init_sys_data(&data1, SNAPSHOT_TEST_SYS, NULL) == 0);
	TEST(sp_measure_init_sys_data(&data2, 0, &data1) == 0);
	TEST(data1.common != NULL);
	TEST(data1.common == data2.common);

	/* check for global system parameters retrieved during initialization */
	TEST_VALUE_INT(data1.common->mem_total, 3096748);
	TEST_VALUE_INT(data1.common->mem_swap, 5111800);
	TEST_VALUE_INT(data1.common->cpu_max_freq, 2201000);

	/* take system resource usage snapshot */
	TEST(sp_measure_get_sys_data(&data1, SNAPSHOT_TEST_SYS, "snapshot1") == 0);
	TEST_VALUE_STR(data1.name, "snapshot1");

	/* check memory usage data */
	TEST_VALUE_INT(data1.mem_free, 460588);
	TEST_VALUE_INT(data1.mem_buffers, 304976);
	TEST_VALUE_INT(data1.mem_cached, 1593264);
	TEST_VALUE_INT(data1.mem_watermark, 3);

	/* check cpu usage data */
	TEST_VALUE_INT(data1.cpu_ticks_total, 85277555);
	TEST_VALUE_INT(data1.cpu_ticks_idle, 82387691);
	TEST_VALUE_INT(data1.cpu_freq_ticks_count, 5);

	/* set the fake rootfs */
	sp_measure_set_fs_root("./rootfs2");
	/* take the second snapshot */
	TEST(sp_measure_get_sys_data(&data2, SNAPSHOT_TEST_SYS, NULL) == 0);
	TEST(data2.name == NULL);
	/* take third snapshot */
	TEST(sp_measure_init_sys_data(&data3, SNAPSHOT_TEST_SYS, NULL) == 0);
	TEST(sp_measure_get_sys_data(&data3, SNAPSHOT_TEST_SYS, NULL) == 0);

	/* check memory usage data */
	TEST_VALUE_INT(data2.mem_free, 426176);
	TEST_VALUE_INT(data2.mem_buffers, 305772);
	TEST_VALUE_INT(data2.mem_cached, 1626056);
	TEST_VALUE_INT(data2.mem_watermark, 3);

	/* check cpu usage data */
	TEST_VALUE_INT(data2.cpu_ticks_total, 85580441);
	TEST_VALUE_INT(data2.cpu_ticks_idle, 82665362);
	TEST_VALUE_INT(data2.cpu_freq_ticks_count, 5);

	int diff;

	/* this isn't actually a diff, but average frequency between snapshots */
	TEST(sp_measure_diff_sys_cpu_avg_freq(&data1, &data2, &diff) == 0);
	TEST(diff == 839559, "\tsp_measure_diff_sys_cpu_avg_freq: diff=%d\n", diff);

	/* data1 and data3 does not share the same common data (they were
	 * initialized separately. Comparison operations for such snapshots
	 * should fail
	 */
	TEST(sp_measure_diff_sys_cpu_ticks(&data1, &data3, &diff) < 0);
	TEST(sp_measure_diff_sys_cpu_usage(&data1, &data3, &diff) < 0);
	TEST(sp_measure_diff_sys_cpu_avg_freq(&data1, &data3, &diff) < 0);
	TEST(sp_measure_diff_sys_mem_used(&data1, &data3, &diff) < 0);

	/* check snapshot comparison results */
	TEST(sp_measure_diff_sys_cpu_ticks(&data1, &data2, &diff) == 0);
	TEST(diff == 302886, "\tsp_measure_diff_sys_cpu_ticks: diff=%d\n", diff);

	TEST(sp_measure_diff_sys_cpu_usage(&data1, &data2, &diff) == 0);
	TEST(diff == 832, "\tsp_measure_diff_sys_cpu_usage: diff=%d\n", diff);

	TEST(sp_measure_diff_sys_mem_used(&data1, &data2, &diff) == 0);
	TEST(diff == 824, "\tsp_measure_diff_sys_mem_used: diff=%d\n", diff);

	/* reset the fake rootfs */
	sp_measure_set_fs_root(NULL);

	/* release resources allocated by snapshots */
	TEST(sp_measure_free_sys_data(&data1) == 0);
	TEST(sp_measure_free_sys_data(&data2) == 0);
	TEST(sp_measure_free_sys_data(&data3) == 0);
}


void check_process_api()
{
	sp_measure_proc_data_t data1, data2, data3;

	/* set the fake rootfs */
	sp_measure_set_fs_root("./rootfs1");
	/* initialize data1, data2 snapshot structures */
	TEST(sp_measure_init_proc_data(&data1, 25268, SNAPSHOT_TEST_PROC, NULL) == 0);
	TEST(sp_measure_init_proc_data(&data2, 25268, 0, &data1) == 0);
	TEST(data1.common != NULL);
	TEST(data1.common == data2.common);

	/* check common process parameters */
	TEST_VALUE_STR(FIELD_PROC_NAME(&data1), "eclipse");
	TEST_VALUE_INT(FIELD_PROC_PID(&data1), 25268);

	/* take process snapshot */
	TEST(sp_measure_get_proc_data(&data1, SNAPSHOT_TEST_PROC, "snapshot1") == 0);
	TEST_VALUE_STR(data1.name, "snapshot1");

	/* check memory usage data */
	TEST_VALUE_INT(FIELD_PROC_MEM_PRIVATE_CLEAN(&data1), 14104);
	TEST_VALUE_INT(FIELD_PROC_MEM_PRIVATE_DIRTY(&data1), 95992);
	TEST_VALUE_INT(FIELD_PROC_MEM_SWAP(&data1), 16192);
	TEST_VALUE_INT(FIELD_PROC_MEM_SIZE(&data1), 686500);
	TEST_VALUE_INT(FIELD_PROC_MEM_PSS(&data1), 110781);
	TEST_VALUE_INT(FIELD_PROC_MEM_RSS(&data1), 114404);
	TEST_VALUE_INT(FIELD_PROC_MEM_REFERENCED(&data1), 68956);
	TEST_VALUE_INT(FIELD_PROC_MEM_SHARED_CLEAN(&data1), 3540);
	TEST_VALUE_INT(FIELD_PROC_MEM_SHARED_DIRTY(&data1), 768);
	TEST_VALUE_INT(FIELD_PROC_MEM_PRIV_DIRTY_SUM(&data1), 112184);

	/* check cpu usage data */
	TEST_VALUE_INT(FIELD_PROC_CPU_STIME(&data1), 47282);
	TEST_VALUE_INT(FIELD_PROC_CPU_UTIME(&data1), 262287);

	/* set the fake roorfs */
	sp_measure_set_fs_root("./rootfs2");
	/* initialize and take third snapshot */
	TEST(sp_measure_init_proc_data(&data3, 25268, SNAPSHOT_TEST_PROC, NULL) == 0);
	TEST(data3.common != NULL);
	TEST(sp_measure_get_proc_data(&data3, SNAPSHOT_TEST_PROC, NULL) == 0);
	TEST(data3.name == NULL);

	/* check memory usage data */
	TEST_VALUE_INT(FIELD_PROC_MEM_PRIVATE_CLEAN(&data3), 14300);
	TEST_VALUE_INT(FIELD_PROC_MEM_PRIVATE_DIRTY(&data3), 97096);
	TEST_VALUE_INT(FIELD_PROC_MEM_SWAP(&data3), 15084);
	TEST_VALUE_INT(FIELD_PROC_MEM_SIZE(&data3), 686500);
	TEST_VALUE_INT(FIELD_PROC_MEM_PSS(&data3), 112140);
	TEST_VALUE_INT(FIELD_PROC_MEM_RSS(&data3), 116108);
	TEST_VALUE_INT(FIELD_PROC_MEM_REFERENCED(&data3), 75672);
	TEST_VALUE_INT(FIELD_PROC_MEM_SHARED_CLEAN(&data3), 3944);
	TEST_VALUE_INT(FIELD_PROC_MEM_SHARED_DIRTY(&data3), 768);
	TEST_VALUE_INT(FIELD_PROC_MEM_PRIV_DIRTY_SUM(&data3), 112180);

	/* check cpu usage data */
	TEST_VALUE_INT(FIELD_PROC_CPU_STIME(&data3), 47299);
	TEST_VALUE_INT(FIELD_PROC_CPU_UTIME(&data3), 262479);


	int diff;

	/* data1 and data3 does not share the same common data (they  wer
	 * initalized separately. Comparison operations for such snapshots
	 * should fail */
	TEST(sp_measure_diff_proc_mem_private_dirty(&data1, &data3, &diff) < 0);
	TEST(sp_measure_diff_proc_cpu_ticks(&data1, &data3, &diff) < 0);

	/* copy over data2 data to data3, restore data3.common reference.
	 * We can't compare data1 and data2 directly, as they have different common
	 * data sets. So we create data3 from the data1 and set the fields
	 * to data2 values. */
	data2 = data3;
	data2.common = data1.common;

	/* check snapshot comparison values */
	TEST(sp_measure_diff_proc_mem_private_dirty(&data1, &data2, &diff) == 0);
	TEST(diff == -4, "\tsp_measure_diff_proc_mem_private_dirty: diff=%d\n", diff);

	TEST(sp_measure_diff_proc_cpu_ticks(&data1, &data2, &diff) == 0);
	TEST(diff == 209, "\tsp_measure_diff_proc_cpu_ticks: diff=%d\n", diff);

	/* reset the fake rootfs */
	sp_measure_set_fs_root(NULL);

	/* release resources allocated by snapshots */
	TEST(sp_measure_free_proc_data(&data1) == 0);
	TEST(sp_measure_free_proc_data(&data2) == 0);
	TEST(sp_measure_free_proc_data(&data3) == 0);
}

int main() 
{
	check_system_api();
	
	check_process_api();

	return 0;
}
