#ifndef __TSC_H__
#define __TSC_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define TSC_OVERHEAD_N 1000

enum TimeUnit {kNanosecond, kMicrosecond, kMillisecond};

static inline uint64_t bench_start(void)
{
	unsigned cycles_low, cycles_high;
	asm volatile("CPUID\n\t"
		"RDTSCP\n\t"
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t"
		: "=r" (cycles_high), "=r" (cycles_low)
		::"%rax", "%rbx", "%rcx", "%rdx");

	return (uint64_t) cycles_high << 32 | cycles_low;
}

static inline uint64_t bench_end(void)
{
	unsigned cycles_low, cycles_high;
	asm volatile("RDTSCP\n\t"
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t"
		"CPUID\n\t"
		: "=r" (cycles_high), "=r" (cycles_low)
		::"%rax", "%rbx", "%rcx", "%rdx");
	return (uint64_t) cycles_high << 32 | cycles_low;
}

static uint64_t measure_tsc_overhead(void)
{
	uint64_t t0, t1, overhead = 0xFFFFFFFFFFFF;
	int i;

	for (i = 0; i < TSC_OVERHEAD_N; ++i)
	{
		t0 = bench_start();
			t1 = bench_end();
			if (t1 -t0 < overhead) overhead = t1 - t0;
	}

	return overhead;
}

static uint64_t cycles_to_timeunit(enum TimeUnit time_unit)
{
	switch (time_unit){
		case kNanosecond:
			return 1e9;
		case kMicrosecond:
			return 1e6;
		case kMillisecond:
			return 1e3;
		default:
			return 1e9;
	}
}

static bool read_from_file(const char* file, long* value)
{
	bool ret = false;
	FILE *fp;
	if ((fp = fopen(file, "r")) != NULL){
		char buffer[1024];
		char *err;
		memset(buffer, '\0', sizeof(buffer));

		if (fread(buffer, sizeof(buffer)-1, 1, fp) < 1){
			const long temp_value = atol(buffer);
			if (buffer[0]!='\0' && (*err == '\n' || *err == '\0')){
				*value = temp_value;
				ret = true;
			}
		}
		fclose(fp);
	}

	return ret;
}

static uint64_t get_tsc_freq_from_cpuinfo(void)
{
	uint64_t freq = 0;
	if (read_from_file("/sys/devices/system/cpu/cpu0/tsc_freq_khz", &freq))
	{
		return freq;
	}

	if (read_from_file("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", &freq))
	{
		return freq;
	}

	const char* pname = "/proc/cpuinfo";
	FILE* fp = fopen(pname, "r");
	if (fp == NULL){
		printf("Warning : Can not get tsc_freq from cpuinfo");
		return 0;
	}

	char line[2048];
	memset(line, '\0', sizeof(line));
	while (!feof(fp)){
		fgets(line, sizeof(line) - 1, fp);
		char substr[8] = {0};
		memcpy(substr, line, 4);
		if (strcmp(substr, "cpu MHz")){
			int j = 0;
			while (line[j] != ':' && line[j] != '\n'){
				++j;
			}
			if (line[j] == ':'){
				freq = atol(line+j);
			}
		}
	}
	fclose(fp);
}

static inline uint64_t read_tsc_now(void)
{
	uint64_t cycles_high, cycles_low;
	asm volatile ("RDTSC\n\t"
				  : "=d" (cycles_high), "=a" (cycles_low)
				 );
	return (uint64_t) cycles_high << 32 | cycles_low;
}

static uint64_t get_tsc_freq_from_sleep(void)
{
	uint64_t start = read_tsc_now();
	sleep(1);
	return read_tsc_now() - start;
}

#endif
