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

bool ds_preferred(const AF & af, string const & arg) {
	std::ofstream outfile;
	outfile.open("out.out", std::ios_base::app);
	outfile << "STARTING COMPUTATION FOR: " << arg << ": " << af.arg_to_int.find(arg)->second << "\n";
	outfile.close();
	preferred_ce_found = false;
	vector<vector<int>> encoding;
	Encodings::add_admissible(af, encoding);
    Encodings::add_nonempty(af, encoding);

	outfile.open("out.out", std::ios_base::app);
	outfile << "ENCODING: \n";
	for (size_t i = 0; i < encoding.size(); i++) {
		for (size_t j = 0; j < encoding[i].size(); j++) {
			outfile << encoding[i][j] << " ";
		}
		outfile << "\n";
	}
	outfile.close();

    vector<int> assumptions;
	int arg_id = af.arg_to_int.find(arg)->second;
	params p = {af, arg_id, encoding, assumptions};
    ds_preferred_r(p);

	// wait until all threads are done
	while (num_active_threads > 0) {
	}
	
	std::cout << thread_counter << "\n";
    return !preferred_ce_found;
}

bool ds_preferred_r(params p) {
	std::ofstream outfile;
	outfile.open("out.out", std::ios_base::app);
	outfile << "STARTING THREAD\n";
	outfile << "CURRENT ASSUMPTIONS: \n";
	for (size_t j = 0; j < p.assumptions.size(); j++) {
		outfile << p.assumptions[j] << "\n";
	}
	outfile.close();
	thread_counter++;
	num_active_threads++;
	
	// if arg is self-attacking it cannot be accepted
	if (p.af.self_attack[p.arg]) {
		num_active_threads--;
		preferred_ce_found = true;
		return false;
	}

	// if arg is unattacked in the af it has to be included in some preferred extension
	if (p.af.unattacked[p.arg]) {
		num_active_threads--;
		return true;
	}

	// check termination flag (some other thread found a counterexample)
	if (preferred_ce_found) {
		num_active_threads--;
		return true;
	}

	// TODO check if arg is in gr(AF) or attacked by it

	vector<int> assumptions;
    vector<int> extension;
    vector<int> complement_clause;
	bool acceptsArg = false;
    complement_clause.reserve(p.af.args);
	ExternalSatSolver solver = ExternalSatSolver(p.af.count, p.af.solver_path);
	solver.addClauses(p.encoding);

	outfile.open("out.out", std::ios_base::app);
	outfile << "STARTING INITIAL SET ITERATION\n";
	outfile.close();
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
            int sat = solver.solve(p.assumptions);

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
                    extension.push_back(i);
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
			outfile.open("out.out", std::ios_base::app);
			outfile << "FOUND INITIAL SET\n";
			outfile.close();
			acceptsArg = false;
			// compute the assumptions for the S-reduct
			assumptions.clear();
			
			outfile.open("out.out", std::ios_base::app);
			outfile << "[";
			for (int i = 0; i < extension.size(); i++) {
				outfile << extension[i] << ":" << p.af.int_to_arg[extension[i]] << ", ";
			}
			outfile << "]\n";
			outfile.close();

			for (int i = 0; i < extension.size(); i++) {
				outfile.open("out.out", std::ios_base::app);
				outfile << extension[i] << ":\n";
				outfile.close();
				if (p.arg == extension[i]) {
					outfile.open("out.out", std::ios_base::app);
					outfile << "ARG ACCEPTED" << "\n";
					outfile.close();
					// If arg is in the initial set, the preferred extension accepts it and will never be a counterexample
					acceptsArg = true;
					break;
				}
				assumptions.push_back(p.af.accepted_var[extension[i]]);
				for(auto const& attacked: p.af.attacked[extension[i]]) {
					outfile.open("out.out", std::ios_base::app);
					outfile << p.arg << " vs " << attacked << "\n";
					outfile.close();
					if (p.arg == attacked) {
						// If the argument is not present in the reduct a counterexample has been found
						//cout << "NO\n";
						num_active_threads--;
						preferred_ce_found = true;
						return false;
					}
					assumptions.push_back(p.af.rejected_var[attacked]);
				}					
			}

			// If the initial set does not accept 'arg', 
			if (!acceptsArg) {
				outfile.open("out.out", std::ios_base::app);
				outfile << "STARTING THREAD WITH REDUCT\n";
				outfile.close();
				// add assumptions from previous steps
				for (size_t i = 0; i < p.assumptions.size(); i++) {
					assumptions.push_back(p.assumptions[i]);
				}
				// create a new thread
				params new_p =  {p.af, p.arg, p.encoding, assumptions};
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
        for (uint32_t i = 0; i < p.af.args; i++) {
            if (solver.model[p.af.accepted_var[i]]) {
                complement_clause.push_back(-p.af.accepted_var[i]);
            } else {
                //complement_clause.push_back(-af.rejected_var[i]);
            }
        }
		// TODO what happens if we add this to the encoding as well?
        solver.addClause(complement_clause);
	}
	num_active_threads--;
	return true;
}
}