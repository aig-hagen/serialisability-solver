#include <iostream>
#include "Encodings.h"
#include "Util.h"
#include <unordered_set>

using namespace std;

/*
vector<int> grounded_assumptions(const AF & af)
{
	//ExternalSatSolver solver = ExternalSatSolver(af.count, 2*af.args); TODO ??
	ExternalSatSolver solver = ExternalSatSolver(af.count);
	Encodings::add_complete(af, solver);
	bool sat = solver.propagate();
	vector<int> new_assumptions;
	if (sat) {
		for (uint32_t i = 0; i < af.args; i++) {
			if (solver.model[af.accepted_var[i]-1]) {
				new_assumptions.push_back(af.accepted_var[i]);
			} else if (solver.model[af.rejected_var[i]-1]) {
				new_assumptions.push_back(-af.accepted_var[i]);
			}
		}
	}
	return new_assumptions;
}*/

void print_extension(const AF & af, const std::vector<uint32_t> & extension)
{
	cout << "[";
	for (uint32_t i = 0; i < extension.size(); i++) {
		cout << af.int_to_arg[extension[i]];
		if (i != extension.size()-1) cout << ",";
	}
	cout << "]\n";
}

void print_extension_ee(const AF & af, const std::vector<uint32_t> & extension)
{
	std::cout << "[";
	for (uint32_t i = 0; i < extension.size(); i++) {
		std::cout << af.int_to_arg[extension[i]];
		if (i != extension.size()-1) cout << ",";
	}
	std::cout << "]";
}

void print_extension_ee(const std::vector<string> & extension)
{
	std::cout << "[";
	for (uint32_t i = 0; i < extension.size(); i++) {
		std::cout << extension[i];
		if (i != extension.size()-1) cout << ",";
	}
	std::cout << "]";
}

AF getReduct(const AF & af, vector<string> ext, vector<pair<string,string>> & atts) {
	AF reduct = AF();

	if (ext.empty()) {
		return af;
	}

	std::unordered_set<uint32_t> removed_args;
	for (const auto& arg: ext) {
		uint32_t arg_id = af.arg_to_int.find(arg)->second;
		removed_args.insert(arg_id);
		removed_args.insert(af.attacked[arg_id].begin(), af.attacked[arg_id].end());
	}
	for (uint32_t i = 0; i < af.args; i++) {
		string arg_str = af.int_to_arg[i];
		if (!removed_args.count(i)) {
			reduct.add_argument(arg_str);
		}
	}

	reduct.set_solver_path(af.solver_path);

	if (!reduct.args) {
		return reduct;
	}
	
	reduct.initialize_attackers();
	reduct.initialize_vars();

	for (const auto& att : atts) {
		uint32_t source;
		uint32_t target;
		auto it1 = af.arg_to_int.find(att.first);
		if (it1 != af.arg_to_int.end()) {
			source = it1->second;
		} else {
			continue;
		}
		auto it2 = af.arg_to_int.find(att.second);
		if (it2 != af.arg_to_int.end()) {
			target = it2->second;
		} else {
			continue;
		}
		if (removed_args.count(source) == 0 && removed_args.count(target) == 0) {
			reduct.add_attack(att);
		}
	}
	return reduct;
}