#include "Encodings.h"
#include <algorithm>

using namespace std;

namespace Encodings {

void add_nonempty(const AF & af, SAT_Solver & solver) {
	vector<int> clause(af.args);
	for (uint32_t i = 1; i <= af.args; i++) {
		clause[i] = i;
	}
	solver.addClause(clause);
}

void add_nonempty_subset_of(const AF & af, vector<uint32_t> args, SAT_Solver & solver) {
	vector<int> non_empty_clause;
	for(auto const& arg: args) {
		non_empty_clause.push_back(arg);
	}
	solver.addClause(non_empty_clause);
	for (int i = 1; i <= af.args; i++) {
		if (std::find(args.begin(), args.end(), i) == args.end()) {
			vector<int> unit_clause = { -i };
        	solver.addClause(unit_clause);
		}
	}
}

void add_rejected_clauses(const AF & af, SAT_Solver & solver) {
	for (int i = 1; i <= af.args; i++) {
		vector<int> additional_clause = { -(af.args+i), -i };
		solver.addClause(additional_clause);
		for (uint32_t j = 0; j < af.attackers[i].size(); j++) {
			vector<int> clause = { (af.args+i), -(*af.attackers)[i][j] };
			solver.addClause(clause);
		}
		vector<int> clause(af.attackers[i].size() + 1);
		for (uint32_t j = 0; j < af.attackers[i].size(); j++) {
			clause[j] = (*af.attackers)[i][j];
		}
		clause[af.attackers[i].size()] = -(af.args+i);
		solver.addClause(clause);
	}
}

void add_conflict_free(const AF & af, SAT_Solver & solver) {
	for (int i = 1; i <= af.args; i++) {
		for (uint32_t j = 0; j < af.attackers[i].size(); j++) {
			vector<int> clause;
			if (i != (*af.attackers)[i][j]) {
				clause = { -i, -(*af.attackers)[i][j] };
			} else {
				clause = { -i };
			}
			solver.addClause(clause);
		}
	}
}

void add_admissible(const AF & af, SAT_Solver & solver) {
	add_conflict_free(af, solver);
	add_rejected_clauses(af, solver);
	for (int i = 1; i <= af.args; i++) {
		if ((*af.self_attack)[i]) continue;
		for (uint32_t j = 0; j < af.attackers[i].size(); j++) {
			if ((*af.symmetric_attack).at(make_pair((*af.attackers)[i][j], i))) continue;
			vector<int> clause = { -i, (af.args + (*af.attackers)[i][j]) };
			solver.addClause(clause);
		}
	}
}

void add_complete(const AF & af, SAT_Solver & solver) {
	add_admissible(af, solver);
	for (int i = 1; i <= af.args; i++) {
		vector<int> clause(af.attackers[i].size()+1);
		for (uint32_t j = 0; j < af.attackers[i].size(); j++) {
			clause[j] = -(af.args + (*af.attackers)[i][j]);
		}
		clause[af.attackers[i].size()] = i;
		solver.addClause(clause);
	}
}

}