#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <climits>
#include <cstring>
#include <vector>
#include <cctype>

#include "Vvex_top.h"
#include "Vvex_top_vex_top.h"

#include "verilated.h"
#include <verilated_fst_c.h>

#define TRACE_ON

using namespace std;

long long max_sim_time = 0LL;

void usage()
{
	printf("Usage: sim [options]\n");
	printf("  -t     start tracing once md is on (to waveform.fst)\n");
	printf("  -s T   stop simulation at time T\n");
}

VerilatedFstC *m_trace;
Vvex_top *top = new Vvex_top;
Vvex_top_vex_top *vex = top->vex_top;
uint64_t sim_time;
uint8_t clkcnt;

int main(int argc, char **argv, char **env)
{
	Verilated::commandArgs(argc, argv);

	if (argc == 1)
	{
		usage();
		exit(1);
	}

	// parse options
	bool loaded = false;
	for (int i = 1; i < argc; i++)
	{
		char *eptr;
		if (strcmp(argv[i], "-t") == 0)
		{
			printf("Tracing ON\n");
			m_trace = new VerilatedFstC;
			top->trace(m_trace, 5);
			Verilated::traceEverOn(true);
			m_trace->open("waveform.fst");

		}
		else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc)
		{
			max_sim_time = strtoll(argv[++i], &eptr, 10);
			if (max_sim_time == 0)
				printf("Simulating forever.\n");
			else
				printf("Simulating %lld steps\n", max_sim_time);
		}
		else if (argv[i][0] == '-') {
			printf("Unrecognized option: %s\n", argv[i]);
			usage();
			exit(1);
		}
	}

	bool sim_on = true; // max_sim_time > 0;
	// bool done = false;

	while (sim_on)
	{

		if (sim_on && max_sim_time > 0 && sim_time >= max_sim_time) {
			printf("Simulation time is up: sim_time=%" PRIu64 "\n", sim_time);
			sim_on = false;
		}

		if (sim_on) {

			sim_time++;

			top->clk_sys = !top->clk_sys;
			top->eval();

			if (m_trace)
				m_trace->dump(sim_time);
		}
	}

	if (m_trace)
		m_trace->close();
	delete top;

	return 0;
}

