/*!
 * The following is largely taken from the mu-toksia argumentation-solver
 * and is subject to the following licence.
 *
 * 
 * Copyright (c) <2020> <Andreas Niskanen, University of Helsinki>
 * 
 * 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * 
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "AF.h"				// Modelling of argumentation frameworks
#include "EnumExtensions.h"	// Naive methods for EE-IT,EE-UC
#include "Problems.h"		// Methods for all supported Problems

#include <iostream>			//std::cout
#include <fstream>			//ifstream
#include <algorithm>
#include <stack>

#include <getopt.h>			// parsing commandline options

using namespace std;

static int version_flag = 0;
static int usage_flag = 0;
static int formats_flag = 0;
static int problems_flag = 0;

task string_to_task(string problem)
{
	string tmp = problem.substr(0, problem.find("-"));
	if (tmp == "DC") return DC;
	if (tmp == "DS") return DS;
	if (tmp == "SE") return SE;
	if (tmp == "EE") return EE;
	if (tmp == "CE") return CE;
	return UNKNOWN_TASK;
}

semantics string_to_sem(string problem)
{
	problem.erase(0, problem.find("-") + 1);
	string tmp = problem.substr(0, problem.find("-"));
	if (tmp == "IT") return IT;
	if (tmp == "UC") return UC;
	if (tmp == "PR") return PR;
	if (tmp == "GR") return GR;
	return UNKNOWN_SEM;
}

void print_usage(string solver_name)
{
	cout << "Usage: " << solver_name << " -p <task> -f <file> -fo <format> [-a <query>]\n\n";
	cout << "  <task>      computational problem; for a list of available problems use option --problems\n";
	cout << "  <file>      input argumentation framework\n";
	cout << "  <format>    file format for input AF; for a list of available formats use option --formats\n";
	cout << "  <query>     query argument\n";
	cout << "Options:\n";
	cout << "  --help      Displays this help message.\n";
	cout << "  --version   Prints version and author information.\n";
	cout << "  --formats   Prints available file formats.\n";
	cout << "  --problems  Prints available computational tasks.\n";
}

void print_version(string solver_name)
{
	cout << solver_name << " (version 1.0)\n" << "Lars Bengel, University of Hagen <lars.bengel@fernuni-hagen.de>\n";
}

void print_formats()
{
	cout << "[apx,tgf]\n";
}

void print_problems()
{
	vector<string> tasks = {"DC", "DS", "SE", "EE", "CE"};
	vector<string> sems = {"IT", "PR", "UC"};
	cout << "[";
	for (uint32_t i = 0; i < tasks.size(); i++) {
		for (uint32_t j = 0; j < sems.size(); j++) {
			string problem = tasks[i] + "-" + sems[j];
			cout << problem << ",";
		}
	}
	cout << "]\n";
}

int main(int argc, char ** argv)
{
	ios_base::sync_with_stdio(false);
	cin.tie(NULL);

	if (argc == 1) {
		print_version(argv[0]);
		return 0;
	}

	const struct option longopts[] =
	{
		{"help", no_argument, &usage_flag, 1},
		{"version", no_argument, &version_flag, 1},
		{"formats", no_argument, &formats_flag, 1},
		{"problems", no_argument, &problems_flag, 1},
		{"p", required_argument, 0, 'p'},
		{"f", required_argument, 0, 'f'},
		{"fo", required_argument, 0, 'o'},
		{"a", required_argument, 0, 'a'},
		{"s", required_argument, 0, 's'},
		{0, 0, 0, 0}
	};

	int option_index = 0;
	int opt = 0;
	string task, file, fileformat, query, sat_path;

	while ((opt = getopt_long_only(argc, argv, "", longopts, &option_index)) != -1) {
		switch (opt) {
			case 0:
				break;
			case 'p':
				task = optarg;
				break;
			case 'f':
				file = optarg;
				break;
			case 'o':
				fileformat = optarg;
				break;
			case 'a':
				query = optarg;
				break;
			case 's':
				sat_path = optarg;
				break;
			default:
				return 1;
		}
	}

	if (version_flag) {
		print_version(argv[0]);
		return 0;
	}

	if (usage_flag) {
		print_usage(argv[0]);
		return 0;
	}

	if (formats_flag) {
		print_formats();
		return 0;
	}

	if (problems_flag) {
		print_problems();
		return 0;
	}

	if (task.empty()) {
		cerr << argv[0] << ": Task must be specified via -p flag\n";
		return 1;
	}

	if (file.empty()) {
		cerr << argv[0] << ": Input file must be specified via -f flag\n";
		return 1;
	}

	if (fileformat.empty()) {
		cerr << argv[0] << ": File format must be specified via -fo flag\n";
		return 1;
	}

	ifstream input;
	input.open(file);

	if (!input.good()) {
		cerr << argv[0] << ": Cannot open input file\n";
		return 1;
	}

	AF af = AF();
	vector<pair<string,string>> atts;
	string line, arg, source, target;
	
	if (fileformat == "apx") {
		while (!input.eof()) {
			getline(input, line);
			line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
			if (line.length() == 0 || line[0] == '/' || line[0] == '%') continue;
			if (line.length() < 6) cerr << "Warning: Cannot parse line: " << line << "\n";
			string op = line.substr(0,3);
			if (op == "arg") {
				if (line[3] == '(' && line.find(')') != string::npos) {
					arg = line.substr(4,line.find(')')-4);
					af.add_argument(arg);
				} else {
					cerr << "Warning: Cannot parse line: " << line << "\n";
				}
			} else if (op == "att") {
				if (line[3] == '(' && line.find(',') != string::npos && line.find(')') != string::npos) {
					source = line.substr(4,line.find(',')-4);
					target = line.substr(line.find(',')+1,line.find(')')-line.find(',')-1);
					atts.push_back(make_pair(source,target));
				} else {
					cerr << "Warning: Cannot parse line: " << line << "\n";
				}
			} else {
				cerr << "Warning: Cannot parse line: " << line << "\n";
			}
		}
	} else if (fileformat == "tgf") {
		while (input >> arg) {
			if (arg == "#") break;
			af.add_argument(arg);
		}
		while (input >> source >> target) {
			atts.push_back(make_pair(source, target));
		}
	} else {
		cerr << argv[0] << ": Unsupported file format\n";
		return 1;
	}

	input.close();

	af.sem = string_to_sem(task);

#if defined(CONE_OF_INFLUENCE)
	if (string_to_task(task) == DS) {
		vector<vector<uint32_t>> attackers;
		attackers.resize(af.args);
		for (uint32_t i = 0; i < atts.size(); i++) {
			attackers[af.arg_to_int[atts[i].second]].push_back(af.arg_to_int[atts[i].first]);
		}

		vector<uint8_t> visited(af.args, 0);
		stack<uint32_t> stack;
		stack.push(af.arg_to_int.at(query));
		uint32_t arg;

		while (!stack.empty()) {
			arg = stack.top();
			stack.pop();
			if(!visited[arg]) {
				visited[arg] = 1;
			}
			for (uint32_t i = 0; i < attackers[arg].size(); i++) {
				if (!visited[attackers[arg][i]]) {
					stack.push(attackers[arg][i]);
				}
			}
		}

		AF new_af = AF();
		for (uint32_t i = 0; i < af.args; i++) {
			if (visited[i]) {
				new_af.add_argument(af.int_to_arg[i]);
			}
		}
		af = move(new_af);
	}
#endif

	af.initialize_attackers();

	for (uint32_t i = 0; i < atts.size(); i++) {
		af.add_attack(atts[i]);
	}

	af.initialize_vars();

	af.set_solver_path(sat_path);

	switch (string_to_task(task)) {
		case DS:
		{
			if (query.empty()) {
				cerr << argv[0] << ": Query argument must be specified via -a flag\n";
				return 1;
			}
			bool skept_accepted = false;
			switch (string_to_sem(task)) {
				case PR:
					//skept_accepted = Problems::mt_ds_preferred(af, query);
					skept_accepted = Problems::ds_preferred(af, query, atts);
					break;
				case UC:
					skept_accepted = Problems::ds_unchallenged(af, query, atts);
					break;
				default:
					cerr << argv[0] << ": Unsupported semantics\n";
					return 1;
			}
			cout << (skept_accepted ? "YES" : "NO") << "\n";
			break;
		}
		case SE:
		{
			vector<string> extension;
			switch (string_to_sem(task)) {
				case GR:
					extension = Problems::se_grounded(af);
					print_extension_ee(extension);
					cout << "\n";
					break;
				case IT:
					Problems::se_initial(af);
					break;
				default:
					cerr << argv[0] << ": Problem not supported!\n";
					return 1;
			}
			break;
		}
		case EE:
		{
			switch (string_to_sem(task)) {
				case IT:
					Problems::ee_initial(af);
					break;
				case UC:
					//EnumExtensions::unchallenged_naive(af, atts);
					Problems::ee_unchallenged(af, atts);
					break;
				default:
					cerr << argv[0] << ": Problem not supported!\n";
					return 1;
			}
			break;
		}
		case CE:
		{
			switch (string_to_sem(task)) {
				case IT:
					Problems::ce_initial(af);
					break;
				default:
					cerr << argv[0] << ": Problem not supported!\n";
					return 1;
			}
			break;
		}
		default:
			cerr << argv[0] << ": Problem not supported!\n";
			return 1;
	}

	return 0;
}