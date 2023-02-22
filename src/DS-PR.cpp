#include "Problems.h"
#include <atomic>
#include <thread>
#include <iostream>
#include <fstream>
#include <stack>

using namespace std;

// Variables for IPC
std::atomic<bool> preferred_ce_found{false};
std::atomic<int> thread_counter(0);
std::atomic<int> num_active_threads(0);

namespace Problems {

/*mutoksia version of ds-pr*/
bool mt_ds_preferred(const AF & af, string const & arg)
{
	ExternalSatSolver solver = ExternalSatSolver(af.count, af.solver_path);
	Encodings::add_complete(af, solver);

	vector<int> assumptions = { -af.accepted_var[af.arg_to_int.at(arg)] };

	while (true) {
		bool sat = solver.solve(assumptions);
		if (!sat) break;

		vector<int> complement_clause;
		complement_clause.reserve(af.args);
		vector<uint8_t> visited(af.args);
		vector<int> new_assumptions = assumptions;
		new_assumptions.reserve(af.args);

		while (true) {
			complement_clause.clear();
			for (uint32_t i = 0; i < af.args; i++) {
				if (solver.model[af.accepted_var[i]]) {
					if (!visited[i]) {
						new_assumptions.push_back(af.accepted_var[i]);
						visited[i] = 1;
					}
				} else {
					complement_clause.push_back(af.accepted_var[i]);
				}
			}
			solver.addClause(complement_clause);
			bool superset_exists = solver.solve(new_assumptions);
			if (!superset_exists) break;
		}

		new_assumptions[0] = -new_assumptions[0];

		if (!solver.solve(new_assumptions)) {
			return false;
		}
	}
	return true;
}

bool ds_preferred(const AF & af, string const & arg, vector<pair<string,string>> & atts) {
	preferred_ce_found = false;
    vector<string> ext;
	params p = {af, arg, atts, ext};
    ds_preferred_r(p);

	// wait until all threads are done
	while (num_active_threads > 0) {
	}
	
	//std::cout << thread_counter << "\n";
    return !preferred_ce_found;
}

bool ds_preferred_r(params p) {
	int thread_id = thread_counter++;
	num_active_threads++;
	//std::ofstream outfile;
	//outfile.open("out.out", std::ios_base::app);
	//outfile << thread_id << ": " << "STARTING THREAD\n";
	//outfile.close();

	// check termination flag (some other thread found a counterexample)
	if (preferred_ce_found) {
		//outfile.open("out.out", std::ios_base::app);
		//outfile << thread_id << ": " << "SIGNAL --> TERM\n";
		//outfile.close();
		num_active_threads--;
		return true;
	}
	
	/*
	================================================================================================================================
	Checking if 'arg' is self-attacking and thus never acceptable
	*/
	if (p.af.self_attack[p.af.arg_to_int.find(p.arg)->second]) {
		//outfile.open("out.out", std::ios_base::app);
		//outfile << thread_id << ": " << "ARG SELF-ATTACKING --> NO\n";
		//outfile.close();
		num_active_threads--;
		preferred_ce_found = true;
		return false;
	}//==============================================================================================================================

	/*
	=================================================================================================================================
	Checking if 'arg' is an unattacked argument, as a shortcut for selecting all the unatttacked initial sets
	*/
	if (p.af.unattacked[p.af.arg_to_int.find(p.arg)->second]) {
		//outfile.open("out.out", std::ios_base::app);
		//outfile << thread_id << ": " << "ARG UNATTACKED --> TERM\n";
		//outfile.close();
		num_active_threads--;
		return true;
	}//==============================================================================================================================

	/*
	==================================================================================================================================
	Checking if 'arg' is accepted or rejected by the grounded extension.
	This is a shortcut for accepting all unattacked initial sets (and further unattacked initial sets revealed by that), since these
	have to be accepted eventually anyway by every preferred extension.
	If 'arg' is accepted by the grounded extension, this thread terminates since every preferred extension that would be constructed 
	will contain 'arg' anyway.
	If 'arg' is rejected by the grounded extension, we found an admissible extension, i.e., ext + grounded that does not contain 'arg'
	If neither is the case, we move to the reduct wrt to the grounded extension to simplify
	*/
	vector<uint32_t> num_attackers;
	num_attackers.resize(p.af.args, 0);
	vector<string> grounded;
	vector<bool> grounded_out;
	grounded_out.resize(p.af.args, false);
	stack<uint32_t> arg_stack;
	for (size_t i = 0; i < p.af.args; i++) {
		if (p.af.unattacked[i]) {
			grounded.push_back(p.af.int_to_arg[i]);
			arg_stack.push(i);
		}
		num_attackers[i] = p.af.attackers[i].size();
	}
	while (arg_stack.size() > 0) {
		uint32_t arg = arg_stack.top();
		arg_stack.pop();
		for (auto const& arg1: p.af.attacked[arg]) {
			if (grounded_out[arg1]) {
				continue;
			}
			if (p.af.int_to_arg[arg1] == p.arg) {
				//outfile.open("out.out", std::ios_base::app);
				//outfile << thread_id << ": " << "GROUNDED REJECTS ARG --> TERM NO \n";
				//outfile.close();
				num_active_threads--;
				preferred_ce_found = true;
				return false;
			}
			
			grounded_out[arg1] = true;
			for (auto const& arg2: p.af.attacked[arg1]) {
				if (num_attackers[arg2] > 0) {
					num_attackers[arg2]--;
					if (num_attackers[arg2] == 0) {
						if (p.af.int_to_arg[arg2] == p.arg) {
							//outfile.open("out.out", std::ios_base::app);
							//outfile << thread_id << ": " << "ARG GROUNDED --> TERM\n";
							//outfile.close();
							num_active_threads--;
							return true;
						}
						grounded.push_back(p.af.int_to_arg[arg2]);
						arg_stack.push(arg2);
					}
				}
			}
		}
	}
	AF af = p.af;
	if (!grounded.empty()) {
		af = getReduct(p.af, grounded, p.atts);
		//outfile.open("out.out", std::ios_base::app);
		//outfile << thread_id << ": " << "REMOVED GROUNDED EXT: ";
		//for(auto const& arg: grounded_ext) {
			//outfile << arg << ",";
		//}
		//outfile << "\n";
		//outfile.close();
	}
	//====================================================================================================================================

	//outfile.open("out.out", std::ios_base::app);
	//outfile << thread_id << ": " << "COMPUTING SCCS\n";
	//outfile.close();
	vector<vector<uint32_t>> sccs = computeStronglyConnectedComponents(af);
	//outfile.open("out.out", std::ios_base::app);
	//outfile << thread_id << ": " << "COMPUTING SCCS DONE\n";
	//outfile.close();
	//outfile.open("out.out", std::ios_base::app);
	//outfile << thread_id << ": " << "STARTING SCC LOOP FOR " << sccs.size() << " SCCS\n";
	//outfile.close();
	int arg_scc = 0;
	for (size_t i = 0; i < sccs.size(); i++) {
		if (std::find(sccs[i].begin(), sccs[i].end(), i) != sccs[i].end()) {
			//outfile.open("out.out", std::ios_base::app);
			//outfile << thread_id << ": " << "STARTING PRIORITY SEARCH FOR SCC " << i << "\n";
			//outfile.close();
			params2 new_p =  {af, p.arg, p.atts, p.base_ext, sccs[i]};
			ds_preferred_r_scc(new_p);
			arg_scc = i;
		}
	}
	if (preferred_ce_found) {
		//outfile.open("out.out", std::ios_base::app);
		//outfile << thread_id << ": " << "SIGNAL --> TERM\n";
		//outfile.close();
		num_active_threads--;
		return true;
	}

	for (size_t i = 0; i < sccs.size(); i++) {
		// TODO sound?
		// If SCC consists of only one argument, it won't have any initial set.
		// If a SCC of size one has an initial set, it would be an unattacked initial set, 
		// which cannot be since we accepted all unattacked initial sets via the grounded extension already.
		if (sccs[i].size() == 1) {
			continue;
		}
		if (i==arg_scc) continue;
		// check termination flag (some other thread found a counterexample)
		if (preferred_ce_found) {
			//outfile.open("out.out", std::ios_base::app);
			//outfile << thread_id << ": " << "SIGNAL --> TERM\n";
			//outfile.close();
			num_active_threads--;
			return true;
		}

		params2 new_p =  {af, p.arg, p.atts, p.base_ext, sccs[i]};
		ds_preferred_r_scc(new_p);
		//std::thread t(ds_preferred_r_scc, new_p);
		//t.detach();
	}

	//outfile.open("out.out", std::ios_base::app);
	//outfile << thread_id << ": " << "LOOPED ALL SCCS --> TERM\n";
	//outfile.close();
	num_active_threads--;
	return true;
}

bool ds_preferred_r_scc(params2 p) {
	int thread_id = thread_counter++;
	num_active_threads++;
	//std::ofstream outfile;
	//outfile.open("out.out", std::ios_base::app);
	//outfile << thread_id << ": " << "STARTING THREAD FOR SCC <";
	//for(auto const& a: p.scc) {
		//outfile << p.af.int_to_arg[a] << ",";
	//}
	//outfile << ">\n";
	//outfile.close();
	

	// check termination flag (some other thread found a counterexample)
	if (preferred_ce_found) {
		//outfile.open("out.out", std::ios_base::app);
		//outfile << thread_id << ": " << "SIGNAL --> TERM\n";
		//outfile.close();
		num_active_threads--;
		return true;
	}

	vector<string> extension;
    vector<int> complement_clause;
    complement_clause.reserve(p.af.args);
	ExternalSatSolver solver = ExternalSatSolver(p.af.count, p.af.solver_path);	
	Encodings::add_admissible(p.af, solver);
    Encodings::add_nonempty_subset_of(p.af, p.scc, solver);	

	// iterate over the initial sets of the current AF
	while (true) {
		// check termination flag (some other thread found a counterexample)
		if (preferred_ce_found) {
			//outfile.open("out.out", std::ios_base::app);
			//outfile << thread_id << ": " << "SIGNAL --> TERM\n";
			//outfile.close();
			num_active_threads--;
			return true;
		}
		
		// Minimization of the first found model
        bool foundExt = false;
        while (true) {
			//outfile.open("out.out", std::ios_base::app);
			//outfile << thread_id << ": " << "LOOKING FOR NEW MODEL\n";
			//outfile.close();
            int sat = solver.solve();
			//outfile.open("out.out", std::ios_base::app);
			//outfile << thread_id << ": " << "CADICAL RESPONDED\n";
			//outfile.close();

			// check termination flag (some other thread found a counterexample)
			if (preferred_ce_found) {
				//outfile.open("out.out", std::ios_base::app);
				//outfile << thread_id << ": " << "SIGNAL --> TERM\n";
				//outfile.close();
				num_active_threads--;
				return true;
			}

            if (sat == 20) break;
            // If we reach this once, an extension has been found
			//outfile.open("out.out", std::ios_base::app);
			//outfile << thread_id << ": " << "FOUND MODEL: ";
            foundExt = true;
            extension.clear();
            for (uint32_t i = 0; i < p.af.args; i++) {
                if (solver.model[p.af.accepted_var[i]]) {
                    extension.push_back(p.af.int_to_arg[i]);
                }
            }
			//for(auto const& arg: extension) {
				//outfile << arg << ",";
			//}
			//outfile << "\n";
			//outfile.close();

			// no minimization needed, if extension has already size of 1
			if (extension.size() == 1) {
				break;
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
			//outfile.open("out.out", std::ios_base::app);
			//outfile << thread_id << ": " << "FOUND MINIMAL MODEL: ";
			//for(auto const& arg: extension) {
				//outfile << arg << ",";
			//}
			//outfile << "\n";
			//outfile.close();
			// Break Conditions

			// If arg is in the initial set, the preferred extension accepts it and will never be a counterexample
			if (std::find(extension.begin(), extension.end(), p.arg) != extension.end()) {
				//outfile.open("out.out", std::ios_base::app);
				//outfile << thread_id << ": " << "MODEL ACCEPTS ARG --> SKIP\n";
				//outfile.close();
				// Extension would contain arg, no thread needs to be created and continue with the next initial set
			} else {
				// If arg gets rejected by the initial set, we found a counterexample 
				const AF reduct = getReduct(p.af, extension, p.atts);

				// If the argument is not present in the reduct a counterexample has been found
				if (reduct.arg_to_int.find(p.arg) == reduct.arg_to_int.end()) {
					//outfile.open("out.out", std::ios_base::app);
					//outfile << thread_id << ": " << "MODEL REJECTS ARG --> TERM NO \n";
					//outfile.close();
					num_active_threads--;
					preferred_ce_found = true;
					return false;
				}
				//TODO trim atts after each reduct?
				vector<string> new_ext;
				new_ext.insert(new_ext.end(), p.base_ext.begin(), p.base_ext.end());
				new_ext.insert(new_ext.end(), extension.begin(), extension.end());

				//outfile.open("out.out", std::ios_base::app);
				//outfile << thread_id << ": " << "DETACHING NEW THREAD\n";
				//outfile.close();
				params new_p =  {reduct, p.arg, p.atts, new_ext};
				std::thread t(ds_preferred_r, new_p);
				t.detach();
			}
        } else {
			// no further initial set found, thread done
			//outfile.open("out.out", std::ios_base::app);
			//outfile << thread_id << ": " << "NO MORE INITIAL SETS --> TERM\n";
			//outfile.close();
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
		//outfile.open("out.out", std::ios_base::app);
		//outfile << thread_id << ": " << "ADDED COMPLEMENT CLAUSE\n";
		//outfile.close();
	}
	//outfile.open("out.out", std::ios_base::app);
	//outfile << thread_id << ": " << "FUNCTION DONE --> TERM\n";
	//outfile.close();
	num_active_threads--;
	return true;
}

}