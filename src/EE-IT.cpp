#include "Problems.h"

using namespace std;

namespace Problems {

set<vector<string>> get_ua_or_uc_initial(const AF & af) {
    set<vector<string> > extensions;

    if (!af.args) {
        return extensions;
    }

    vector<string> extension;
    vector<int> complement_clause;
    complement_clause.reserve(af.args);
    vector<vector<uint32_t>> sccs = computeStronglyConnectedComponents(af);
    for (auto const& scc: sccs) {
        #if defined(SAT_EXTERNAL)
        SAT_Solver solver = SAT_Solver(af.count, af.solver_path);
        #elif defined(SAT_CMSAT)
        SAT_Solver solver = SAT_Solver(af.count, af.args);
        #else
        #error "No SAT solver defined"
        #endif
        Encodings::add_admissible(af, solver);
        Encodings::add_nonempty_subset_of(af, scc, solver);

        while (true) {
            bool foundExt = false;
            while (true) {
                int sat = solver.solve();          
                if (sat==20) break;
                
                foundExt = true;
                extension.clear();
                for (uint32_t i = 0; i < af.args; i++) {
                    if (solver.model[af.accepted_var[i]]) {
                        extension.push_back(af.int_to_arg[i]);
                    }
                }

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
            if (foundExt) {
                extensions.insert(extension);
            } else {
                break;
            }

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
        solver.free();
	}


    // filter out the challenged initial sets
    // TODO can maybe be optimized
    std::set<vector<string> > result;
    for (auto ext1: extensions) {
        bool challenged = false;
        for (auto ext2: extensions) {
            if (ext1 == ext2) {
                continue;
            }
            for (auto arg1: ext1) {
                for (auto arg2: ext2) {
                    try {
                        u_int32_t arg1_id = af.arg_to_int.find(arg1)->second;
                        u_int32_t arg2_id = af.arg_to_int.find(arg2)->second;
                        if (af.att_exists.at(make_pair(arg1_id, arg2_id)) || af.att_exists.at(make_pair(arg2_id, arg1_id))) {
                            challenged = true;
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
        if (!challenged) {
            result.insert(ext1);
        } 
    }

	return result;
}

bool ee_initial(const AF & af) {
    std::cout << "[";

    int count = 0;
    vector<uint32_t> extension;
    vector<int> complement_clause;
    complement_clause.reserve(af.args);
    
    if (!af.args) {
        std::cout << "]\n";
        return true;
    }

    vector<vector<uint32_t>> sccs = computeStronglyConnectedComponents(af);
    for (auto const& scc: sccs) {
        #if defined(SAT_EXTERNAL)
        SAT_Solver solver = SAT_Solver(af.count, af.solver_path);
        #elif defined(SAT_CMSAT)
        SAT_Solver solver = SAT_Solver(af.count, af.args);
        #else
        #error "No SAT solver defined"
        #endif
        Encodings::add_admissible(af, solver);
        Encodings::add_nonempty_subset_of(af, scc, solver);

        while (true) {
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
                        min_complement_clause.push_back(-af.accepted_var[i]);
                    } else {
                        vector<int> unit_clause = { -af.accepted_var[i] };
                        solver.addMinimizationClause(unit_clause);
                    }
                }
                solver.addMinimizationClause(min_complement_clause);
            }
            if (foundExt) {
                if (!count++ == 0) {
                    std::cout << ", ";
                }
                print_extension_ee(af, extension);
            } else {
                break;
            }

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
	}
    std::cout << "]\n";
	return true;
}

}