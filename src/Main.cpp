#include "AF.h"				// Modelling of argumentation frameworks
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

task string_to_task(string problem) {
	string tmp = problem.substr(0, problem.find("-"));
	if (tmp == "DC") return DC;
	if (tmp == "DS") return DS;
	if (tmp == "SE") return SE;
	if (tmp == "EE") return EE;
	return UNKNOWN_TASK;
}

semantics string_to_sem(string problem) {
	problem.erase(0, problem.find("-") + 1);
	string tmp = problem.substr(0, problem.find("-"));
	if (tmp == "PR") return PR;
	return UNKNOWN_SEM;
}

void print_usage(string solver_name) {
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

void print_version(string solver_name) {
	cout << solver_name << " (version 1.0)\n" << "Lars Bengel, University of Hagen <lars.bengel@fernuni-hagen.de>\n";
}

void print_formats() {
	cout << "[i23]\n";
}

void print_problems() {
	vector<string> tasks = {"DC", "DS", "SE", "EE"};
	vector<string> sems = {"PR"};
	cout << "[";
	for (uint32_t i = 0; i < tasks.size(); i++) {
		for (uint32_t j = 0; j < sems.size(); j++) {
			string problem = tasks[i] + "-" + sems[j];
			cout << problem << ",";
		}
	}
	cout << "]\n";
}

int main(int argc, char ** argv) {
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
	string task, file, fileformat, sat_path;
	int query = 0;

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
				query = stoi(optarg);
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
	vector<pair<int,int>> atts;
	string op, type, source, target;
	int num_args;
	if (fileformat == "i23") {
		while (input >> op >> type >> num_args) {
			if (op[0] == '#') continue;
			if (op != "p" || type != "af") {
				cerr << "Warning: Unexpected header: " << op << " " << type << "\n";
			}
			af.set_arguments(num_args);
			af.initialize_attackers();
			break;
		}
		while (input >> source >> target) {
			if (source[0] == '#') continue;
			af.add_attack(make_pair(stoi(source), stoi(target)));
		}
	} else {
		cerr << argv[0] << ": Unsupported file format\n";
		return 1;
	}

	input.close();

	af.sem = string_to_sem(task);
	af.set_solver_path(sat_path);

#if defined(CONE_OF_INFLUENCE)
	if (string_to_task(task) == DS) {
		//vector<vector<uint32_t>> attackers;
		//attackers.resize(af.args);
		//for (uint32_t i = 0; i < atts.size(); i++) {
		//	attackers[af.arg_to_int[atts[i].second]].push_back(af.arg_to_int[atts[i].first]);
		//}

		vector<bool> visited(af.args);
		stack<uint32_t> stack;
		stack.push(query);
		uint32_t arg;

		while (!stack.empty()) {
			arg = stack.top();
			stack.pop();
			if(!visited[arg]) {
				visited[arg] = true;
			}
			for (uint32_t i = 0; i < attackers[arg].size(); i++) {
				if (!visited[attackers[arg][i]]) {
					stack.push(attackers[arg][i]);
				}
			}
		}
		AF new_af = getReduct(af, visited);
		//AF new_af = AF();
		//for (uint32_t i = 0; i < af.args; i++) {
		//	if (visited[i]) {
		//		new_af.add_argument(af.int_to_arg[i]);
		//	}
		//}
		//af = move(new_af);
	}
#endif

	switch (string_to_task(task)) {
		case DS:
		{
			if (query == 0) {
				cerr << argv[0] << ": Query argument must be specified via -a flag\n";
				return 1;
			} else if (query > af.args) {
				cerr << argv[0] << ": Query argument larger than number of arguments\n";
				return 1;
			}
			bool skept_accepted = false;
			switch (string_to_sem(task)) {
				case PR:
					skept_accepted = Problems::mt_ds_preferred(af, query);
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
				default:
					cerr << argv[0] << ": Problem not supported!\n";
					return 1;
			}
			break;
		}
		case EE:
		{
			switch (string_to_sem(task)) {
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