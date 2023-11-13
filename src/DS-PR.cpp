#include "Problems.h"						// Header for all Problem methods

#include <stack>							// for computing the grounded extension
#include <algorithm>						// std::find

using namespace std;


namespace Problems {

/*mutoksia version of ds-pr, for internal testing purposes*/
bool mt_ds_preferred(const AF & af, int arg) {
	SAT_Solver solver = SAT_Solver(2*af.args, af.solver_path);
	Encodings::add_complete(af, solver);

	vector<int> assumptions = { -arg };

	while (true) {
		int sat = solver.solve(assumptions);
		for (size_t i = 0; i < solver.model.size(); i++) {
			cout << i << ": " << solver.model[i] << "\n";
		}
		
		if (sat == 20) break;

		vector<int> complement_clause;
		complement_clause.reserve(af.args);
		vector<uint8_t> visited(af.args);
		vector<int> new_assumptions = assumptions;
		new_assumptions.reserve(af.args);

		while (true) {
			complement_clause.clear();
			for (int i = 1; i <= af.args; i++) {
				if (solver.model[i]) {
					if (!visited[i]) {
						new_assumptions.push_back(i);
						visited[i] = 1;
					}
				} else {
					complement_clause.push_back(i);
				}
			}
			solver.addClause(complement_clause);
			int superset_exists = solver.solve(new_assumptions);
			if (superset_exists == 20) break;
		}

		new_assumptions[0] = -new_assumptions[0];

		if (solver.solve(new_assumptions) == 20) {
			return false;
		}
	}
	return true;
}

}