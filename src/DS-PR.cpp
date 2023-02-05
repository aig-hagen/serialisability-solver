#include "Encodings.h"
#include "Problems.h"
#include <atomic>
#include <thread>
#include <iostream>

using namespace std;

std::atomic<bool> preferred_ce_found{false};
std::atomic<int> thread_counter(0);
std::atomic<int> num_active_threads(0);

namespace Problems {

bool preferred_p(const AF & af, string const & arg, vector<pair<string,string>> & atts) {
	preferred_ce_found = false;
    vector<string> ext;
	//TODO Thread Logic and some kind of flag to kill all threads once counterexample is found
	params p = {af, arg, atts, ext};
    preferred_p_r(p);

	// wait until all threads are done
	while (num_active_threads > 0) {
	}
	
	std::cout << thread_counter << "\n";
    return !preferred_ce_found;
}

bool preferred_p_r(params p) {
	thread_counter++;
	num_active_threads++;
	// check termination flag (some other thread found a counterexample)
	if (preferred_ce_found) {
		num_active_threads--;
		return true;
	}

	// TODO check if arg is unattacked
	// TODO check self attack
	// TODO check if arg is in gr(AF) or attacked by it
	
    vector<string> extension;
    vector<int> complement_clause;
    complement_clause.reserve(p.af.args);
	ExternalSatSolver solver = ExternalSatSolver(p.af.count, p.af.solver_path);
    Encodings::add_admissible(p.af, solver);
    Encodings::add_nonempty(p.af, solver);

	// iterate over the initial sets of the current AF
	while (true) {
		// check termination flag (some other thread found a counterexample)
		if (preferred_ce_found) {
			num_active_threads--;
			return true;
		}
		
		// Minimization of the first found model
        bool foundExt = false;
        while (true) {
            int sat = solver.solve();

			// check termination flag (some other thread found a counterexample)
			if (preferred_ce_found) {
				num_active_threads--;
				return true;
			}

            if (sat == 20) break;
            // If we reach this once, an extension has been found
            foundExt = true;
            extension.clear();
            for (uint32_t i = 0; i < p.af.args; i++) {
                if (solver.model[p.af.accepted_var[i]]) {
                    extension.push_back(p.af.int_to_arg[i]);
                }
            }
			// add clauses to force the solver to look for a subset of the extension
            vector<int> min_complement_clause;
            min_complement_clause.reserve(p.af.args);
            for (uint32_t i = 0; i < p.af.args; i++) {
                if (solver.model[p.af.accepted_var[i]]) {
                    min_complement_clause.push_back(-p.af.accepted_var[i]);
                } else {
                    vector<int> unit_clause = { -p.af.accepted_var[i] };
                    solver.addMinimizationClause(unit_clause);
                }
            }
            solver.addMinimizationClause(min_complement_clause);
        }
		// found an initial set. Start a new thread with the initial set and the respective reduct
        if (foundExt) {
			// Break Conditions

			// If arg is in the initial set, the preferred extension accepts it and will never be a counterexample
			if (std::find(extension.begin(), extension.end(), p.arg) != extension.end()) {
				// Extension would contain arg, no thread needs to be created and continue with the next initial set
			} else {
				// If arg gets rejected by the initial set, we found a counterexample 
				const AF reduct = getReduct(p.af, extension, p.atts);

				// If the argument is not present in the reduct a counterexample has been found
				if (reduct.arg_to_int.find(p.arg) == reduct.arg_to_int.end()) {
					//cout << "NO\n";
					num_active_threads--;
					preferred_ce_found = true;
					return false;
				}
				//TODO trim atts after each reduct
				vector<string> new_ext;
				new_ext.insert(new_ext.end(), p.base_ext.begin(), p.base_ext.end());
				new_ext.insert(new_ext.end(), extension.begin(), extension.end());

				params new_p =  {reduct, p.arg, p.atts, new_ext};
				std::thread t(preferred_p_r, new_p);
				t.detach();
			}
        } else {
			// no initial set found, thread done
			num_active_threads--;
            return true;
        }

		// add complement clause to prevent the just found initial set
        complement_clause.clear();
        for (uint32_t i = 0; i < p.af.args; i++) {
            if (solver.model[p.af.accepted_var[i]]) {
                complement_clause.push_back(-p.af.accepted_var[i]);
            } else {
                //complement_clause.push_back(-af.rejected_var[i]);
            }
        }
        solver.addClause(complement_clause);
	}
	num_active_threads--;
	return true;
}
}