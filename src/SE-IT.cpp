#include "Problems.h"

using namespace std;

namespace Problems {

bool se_initial(const AF & af) {
	SAT_Solver solver = SAT_Solver(af.count, af.solver_path);
	Encodings::add_nonempty(af, solver);
	Encodings::add_admissible(af, solver);

	vector<uint32_t> extension;
	bool foundExt = false;
	while (true) {
		int sat = solver.solve();
		if (sat==20) break;
		foundExt = true;
		extension.clear();
		for (uint32_t i = 0; i < af.args; i++) {
			if (solver.model[af.accepted_var[i]]) {
				extension.push_back(i);
			}
		}
		
		vector<int> min_complement_clause;
		min_complement_clause.reserve(af.args);
		for (uint32_t i = 0; i < af.args; i++) {
			if (solver.model[af.accepted_var[i]]) {
				// create a clause with all negated variables for the accepted arguments, makes sure the next found model (if it exists) is a subset of the current model
				min_complement_clause.push_back(-af.accepted_var[i]);
			} else {
				// for all non-accepted arguments, at their negated acceptance variable to the solver, makes sure no non-accepted argument is accepted in the next found model
				vector<int> unit_clause = { -af.accepted_var[i] };
				solver.addMinimizationClause(unit_clause);
			}
		}
		solver.addMinimizationClause(min_complement_clause);
	}
	if (foundExt) {
		print_extension_ee(af, extension);
		return true;
	} else {
		std::cout << "NO\n";
		return false;
	}
}

}