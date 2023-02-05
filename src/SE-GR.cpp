#include "Encodings.h"
#include "Problems.h"
#include <iostream>

using namespace std;

namespace Problems {

vector<string> se_grounded(const AF & af) {
    /*TODO implement properly
	ExternalSatSolver solver = ExternalSatSolver(af.count, af.solver_path);
	Encodings::add_complete(af, solver);
	bool sat = solver.propagate();
	vector<string> extension;
	if (sat) {
		for (uint32_t i = 0; i < af.args; i++) {
			if (solver.model[af.accepted_var[i]]) {
				extension.push_back(af.int_to_arg[i]);
			}
		}
	}
	return extension;
    */
   vector<string> extension;
   return extension;
}

}