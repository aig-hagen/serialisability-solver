#include "Problems.h"
#include "Encodings.h"
#include <iostream>

using namespace std;

namespace Problems {

set<vector<uint32_t>> get_all_initial(const AF & af) {
    set<vector<uint32_t>> extensions;

    vector<uint32_t> extension;
    vector<int> complement_clause;

    complement_clause.reserve(af.args);
    #if defined(SAT_EXTERNAL)
	SAT_Solver solver = SAT_Solver(af.count, af.solver_path);
	#elif defined(SAT_CMSAT)
	SAT_Solver solver = SAT_Solver(af.count, af.args);
	#else
	#error "No SAT solver defined"
	#endif
    Encodings::add_admissible(af, solver);
    Encodings::add_nonempty(af, solver);
	while (true) {
        // Compute one extension by finding a minimal solution to the KB
        bool foundExt = false;
        while (true) {
            int sat = solver.solve();
            if (sat==20) break;
            // If we reach this once, an extension has been found
            //cout << "======" << sat << "=======";
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
            extensions.insert(extension);
        } else {
            break;
        }

        // create a complement clause, so that the solver doesnt find the same model again
        // TODO can maybe be even more specific
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
	return extensions;
}

bool ce_initial(const AF & af) {
    set<vector<uint32_t>> extensions = get_all_initial(af);
    uint32_t num_unattacked = 0;
    uint32_t num_unchallenged = 0;
    uint32_t num_challenged = 0;
    vector<uint32_t> sizes_unattacked;
    vector<uint32_t> sizes_unchallenged;
    vector<uint32_t> sizes_challenged;
    for (auto ext1: extensions) {
        bool challenged = false;
        bool unattacked = false;
        if (ext1.size() == 1 && af.attackers[ext1[0]].size() == 0) {
            unattacked = true;
            num_unattacked++;
            sizes_unattacked.push_back(1);
            continue;
        }        
        for (auto ext2: extensions) {
            if (ext1 == ext2) {
                continue;
            }
            for (auto arg1: ext1) {
                for (auto arg2: ext2) {
                    try {
                        if (af.att_exists.at(make_pair(arg1, arg2)) || af.att_exists.at(make_pair(arg2, arg1))) {
                            challenged = true;
                            num_challenged++;
                            sizes_challenged.push_back(ext1.size());
                            break;
                        }
                    }
                    catch(const std::exception& e) {
                        ;
                    } 
                }
                if (challenged) {
                    break;
                }
            }
            if (challenged) {
                break;
            }
        }
        if (!challenged && !unattacked) {
            num_unchallenged++;
            sizes_unchallenged.push_back(ext1.size());
        } 
    }
    cout << extensions.size() << "," << num_unattacked << "," << num_unchallenged << "," << num_challenged << "\n";
    for (uint32_t i = 0; i < sizes_unattacked.size(); i++) {
        cout << sizes_unattacked[i];
        if (i != sizes_unattacked.size()-1) cout << ",";
    }
    cout << "\n";
    for (uint32_t i = 0; i < sizes_unchallenged.size(); i++) {
        cout << sizes_unchallenged[i];
        if (i != sizes_unchallenged.size()-1) cout << ",";
    }
    cout << "\n";
    for (uint32_t i = 0; i < sizes_challenged.size(); i++) {
        cout << sizes_challenged[i];
        if (i != sizes_challenged.size()-1) cout << ",";
    }
    cout << "\n";
    

	return true;
}

}