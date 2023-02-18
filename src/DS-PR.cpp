#include "Encodings.h"
#include "Problems.h"
#include <atomic>
#include <thread>
#include <iostream>
#include <fstream>

using namespace std;

// Variables for IPC
std::atomic<bool> preferred_ce_found{false};
std::atomic<int> thread_counter(0);
std::atomic<int> num_active_threads(0);

namespace Problems {

bool ds_preferred(const AF & af, string const & arg, vector<pair<string,string>> & atts) {
	preferred_ce_found = false;
    vector<string> ext;
	//TODO Thread Logic and some kind of flag to kill all threads once counterexample is found
	params p = {af, arg, atts, ext};
    ds_preferred_r(p);

	// wait until all threads are done
	while (num_active_threads > 0) {
	}
	
	std::cout << thread_counter << "\n";
    return !preferred_ce_found;
}

bool ds_preferred_r(params p) {
	std::ofstream outfile;
	//outfile.open("out.out", std::ios_base::app);
	//outfile << "STARTING THREAD\n";
	//outfile.close();
	
	thread_counter++;
	num_active_threads++;
	
	// if arg is self-attacking it cannot be accepted
	if (p.af.self_attack[p.af.arg_to_int.find(p.arg)->second]) {
		//outfile.open("out.out", std::ios_base::app);
		//outfile << "TERM NO --> ARG SELF-ATTACKING\n";
		//outfile.close();
		num_active_threads--;
		preferred_ce_found = true;
		return false;
	}

	// if arg is unattacked in the af it has to be included in some preferred extension
	if (p.af.unattacked[p.af.arg_to_int.find(p.arg)->second]) {
		//outfile.open("out.out", std::ios_base::app);
		//outfile << "TERM --> ARG UNATTACKED\n";
		//outfile.close();
		num_active_threads--;
		return true;
	}

	// check termination flag (some other thread found a counterexample)
	if (preferred_ce_found) {
		//outfile.open("out.out", std::ios_base::app);
		//outfile << "TERM --> SIGNAL\n";
		//outfile.close();
		num_active_threads--;
		return true;
	}

	// TODO can be optimized slightly
	vector<string> grounded_ext = se_grounded(p.af);
	for(auto const& arg: grounded_ext) {
		if (arg == p.arg) {
			//outfile.open("out.out", std::ios_base::app);
			//outfile << "TERM --> ARG GROUNDED\n";
			//outfile.close();
			num_active_threads--;
			return true;
		}
	}
	AF af = getReduct(p.af, grounded_ext, p.atts);
	if (af.arg_to_int.find(p.arg) == af.arg_to_int.end()) {
		//outfile.open("out.out", std::ios_base::app);
		//outfile << "GROUNDED REJECTS ARG --> TERM NO \n";
		//outfile.close();
		//cout << "NO\n";
		num_active_threads--;
		preferred_ce_found = true;
		return false;
	}
	//outfile.open("out.out", std::ios_base::app);
	//outfile << "REMOVED GROUNDED EXT: ";
	//for(auto const& arg: grounded_ext) {
		//outfile << arg << ",";
	//}
	//outfile << "\n";
	//outfile.close();

	
    vector<string> extension;
    vector<int> complement_clause;
    complement_clause.reserve(af.args);
	ExternalSatSolver solver = ExternalSatSolver(af.count, af.solver_path);
    Encodings::add_admissible(af, solver);
    Encodings::add_nonempty(af, solver);
	//outfile.open("out.out", std::ios_base::app);
	//outfile << "ENCODING CREATED\n";
	//outfile.close();

	//outfile.open("out.out", std::ios_base::app);
	//outfile << "STARTING INITIAL SET ITERATION\n";
	//outfile.close();
	// iterate over the initial sets of the current AF
	while (true) {
		// check termination flag (some other thread found a counterexample)
		if (preferred_ce_found) {
			//outfile.open("out.out", std::ios_base::app);
			//outfile << "TERM --> SIGNAL\n";
			//outfile.close();
			num_active_threads--;
			return true;
		}
		
		// Minimization of the first found model
        bool foundExt = false;
        while (true) {
            int sat = solver.solve();

			// check termination flag (some other thread found a counterexample)
			if (preferred_ce_found) {
				//outfile.open("out.out", std::ios_base::app);
				//outfile << "TERM --> SIGNAL\n";
				//outfile.close();
				num_active_threads--;
				return true;
			}

            if (sat == 20) break;
            // If we reach this once, an extension has been found
			//outfile.open("out.out", std::ios_base::app);
			//outfile << "FOUND MODEL: ";
            foundExt = true;
            extension.clear();
            for (uint32_t i = 0; i < af.args; i++) {
                if (solver.model[af.accepted_var[i]]) {
                    extension.push_back(af.int_to_arg[i]);
                }
            }
			//for(auto const& arg: extension) {
				//outfile << arg << ",";
			//}
			//outfile << "\n";
			//outfile.close();
			// add clauses to force the solver to look for a subset of the extension
            vector<int> min_complement_clause;
            min_complement_clause.reserve(af.args);
            for (uint32_t i = 0; i < af.args; i++) {
                if (solver.model[af.accepted_var[i]]) {
                    min_complement_clause.push_back(-af.accepted_var[i]);
                } else {
                    vector<int> unit_clause = { -af.accepted_var[i] };
                    solver.addMinimizationClause(unit_clause);
                }
            }
            solver.addMinimizationClause(min_complement_clause);
        }
		// found an initial set. Start a new thread with the initial set and the respective reduct
        if (foundExt) {
			//outfile.open("out.out", std::ios_base::app);
			//outfile << "FOUND MINIMAL MODEL: ";
			//for(auto const& arg: extension) {
				//outfile << arg << ",";
			//}
			//outfile << "\n";
			//outfile.close();
			// Break Conditions

			// If arg is in the initial set, the preferred extension accepts it and will never be a counterexample
			if (std::find(extension.begin(), extension.end(), p.arg) != extension.end()) {
				//outfile.open("out.out", std::ios_base::app);
				//outfile << "MODEL ACCEPTS ARG --> SKIP\n";
				//outfile.close();
				// Extension would contain arg, no thread needs to be created and continue with the next initial set
			} else {
				// If arg gets rejected by the initial set, we found a counterexample 
				const AF reduct = getReduct(af, extension, p.atts);

				// If the argument is not present in the reduct a counterexample has been found
				if (reduct.arg_to_int.find(p.arg) == reduct.arg_to_int.end()) {
					//outfile.open("out.out", std::ios_base::app);
					//outfile << "MODEL REJECTS ARG --> TERM NO \n";
					//outfile.close();
					//cout << "NO\n";
					num_active_threads--;
					preferred_ce_found = true;
					return false;
				}
				//TODO trim atts after each reduct?
				vector<string> new_ext;
				new_ext.insert(new_ext.end(), p.base_ext.begin(), p.base_ext.end());
				new_ext.insert(new_ext.end(), extension.begin(), extension.end());

				//outfile.open("out.out", std::ios_base::app);
				//outfile << "DETACHING NEW THREAD\n";
				//outfile.close();
				params new_p =  {reduct, p.arg, p.atts, new_ext};
				std::thread t(ds_preferred_r, new_p);
				t.detach();
			}
        } else {
			// no further initial set found, thread done
			num_active_threads--;
            return true;
        }

		// add complement clause to prevent the just found initial set
        complement_clause.clear();
        for (uint32_t i = 0; i < af.args; i++) {
            if (solver.model[af.accepted_var[i]]) {
                complement_clause.push_back(-af.accepted_var[i]);
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