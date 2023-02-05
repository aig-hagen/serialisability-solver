#include "Encodings.h"
#include "Problems.h"
#include <iostream>

using namespace std;

namespace Problems {

bool se_initial(const AF & af) {
	ExternalSatSolver solver = ExternalSatSolver(af.count, af.solver_path);
	Encodings::add_nonempty(af, solver);
	Encodings::add_admissible(af, solver);
	bool sat = solver.solve();
	if (sat) {
		vector<uint32_t> extension;
		for (uint32_t i = 0; i < af.args; i++) {
			if (solver.model[af.accepted_var[i]-1]) {
				extension.push_back(i);
			}
		}
		print_extension(af, extension);
		return true;
	}else {
		std::cout << "NO\n";
		return false;
	}
}

}