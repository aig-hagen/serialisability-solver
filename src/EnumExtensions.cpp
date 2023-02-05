#include "Encodings.h"
#include "Util.h"
#include "SingleExtension.h"
#include "EnumExtensions.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>

using namespace std;

namespace EnumExtensions {

vector<vector<uint32_t>> initial_naive(const AF & af) {
    vector<vector<uint32_t>> extensions;

    int n = af.args;
    int subset_count = pow(2, n);

    for (int i = 0; i < subset_count; i++) {
        std::vector<uint32_t> subset;
        for (int j = 0; j < n; j++) {
            if (i & (1 << j)) {
                subset.push_back(j);
            }
        }

        // non emptiness
        if (subset.empty()) {
            continue;
        }

        // minimality
        std::sort(subset.begin(), subset.end());

        bool minimal = true;
        for (const auto &other_subset : extensions) {
            if (std::includes(subset.begin(), subset.end(), other_subset.begin(), other_subset.end())) {
                minimal = false;
                break;
            }
        }

        if (!minimal) {
            continue;
        }

        //conflict-freeness
        bool cf = true;
        for(auto const& arg1: subset) {
            if (af.self_attack[arg1]) {
                cf = false; 
                break;
            }
            for(auto const& arg2: subset) {
                if (af.att_exists.count(make_pair(arg1,arg2)) > 0) {
                    cf = false;
                    break;
                }
            }
            if (!cf) {
                break;
            }
        }
        if (!cf) {
            continue;
        }

        //admissibility
        bool adm = true;
        for (auto const& arg1: subset) {
            for (auto const& attacker: af.attackers[arg1]) {
                bool defended = false;
                for(auto const& arg2: subset) {
                    if (af.att_exists.count(make_pair(arg2,attacker)) > 0) {
                        defended = true;
                        break;
                    }
                }
                if (!defended) {
                    adm = false;
                    break;
                }
            }
            if (!adm) {
                break;
            }
        }
        if (!adm) {
            continue;
        }

        //print_extension_ee(af, subset);
        extensions.push_back(subset);
    }

    return extensions;
}

std::set<vector<string>> ua_or_uc_initial_naive(const AF & af) {
    vector<vector<uint32_t>> initialSets = initial_naive(af);

    set<vector<string>> result;
    for(auto const& set1: initialSets) {
        bool challenged = false;
        for(auto const& set2: initialSets) {
            if (set1 == set2) {
                continue;
            }
            for (auto arg1: set1) {
                for (auto arg2: set2) {
                    try {
                        if (af.att_exists.count(make_pair(arg1, arg2)) > 0 || af.att_exists.count(make_pair(arg2, arg1)) > 0) {
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
            vector<string> ext;
            for(auto const& arg: set1) {
                ext.push_back(af.int_to_arg[arg]);
            }
            result.insert(ext);
        } 
    }

    return result;

}

bool unchallenged_naive(const AF & af, vector<pair<string,string>> & atts) {
    cout << "[";
    vector<string> ext;
    unchallenged_naive_r(af, af, atts, ext);
    cout << "]\n";
    return true;
}

bool unchallenged_naive_r(const AF & original_af, const AF & af, vector<pair<string,string>> & atts, vector<string> base_ext) {
    set<vector<string>> candidateSets = ua_or_uc_initial_naive(af);

    if (candidateSets.empty()) {
        print_extension_ee(base_ext);
        return true;
    }

    for (auto const& initialSet: candidateSets) {
        const AF reduct = getReduct(af, initialSet, atts);
        //TODO trim atts after each reduct
        vector<string> new_ext;
        new_ext.insert(new_ext.end(), base_ext.begin(), base_ext.end());
        new_ext.insert(new_ext.end(), initialSet.begin(), initialSet.end());

        unchallenged_naive_r(original_af, reduct, atts, new_ext);
    }
    return true;
}

std::set<vector<string>> ua_or_uc_initial(const AF & af) {
    std::set<vector<string> > extensions;

    if (!af.args) {
        return extensions;
    }
    
    vector<string> extension;
    vector<vector<int>> clauses;
    vector<int> complement_clause;
    complement_clause.reserve(af.args);
	while (true) {
        // Compute one extension by finding a minimal solution to the KB
        // TODO: This should be outside of the loop and a copy should be used each loop
        ExternalSatSolver solver = ExternalSatSolver(af.count);
        Encodings::add_admissible(af, solver);
        Encodings::add_nonempty(af, solver);
        if (!clauses.empty())
        {
            for (size_t i = 0; i < clauses.size(); i++)
            {
                solver.addClause(clauses[i]);
            }
        }
        
        bool foundExt = false;
        while (true) {
            bool sat = solver.solve();
            if (!sat) break;
            foundExt = true;
            extension.clear();
            for (uint32_t i = 0; i < af.args; i++) {
                if (solver.model[af.accepted_var[i]-1]) {
                    extension.push_back(af.int_to_arg[i]);
                }
            }
            vector<int> min_complement_clause;
            min_complement_clause.reserve(af.args);
            for (uint32_t i = 0; i < af.args; i++) {
                if (solver.model[af.accepted_var[i]-1]) {
                    min_complement_clause.push_back(-af.accepted_var[i]);
                } else {
                    vector<int> unit_clause = { -af.accepted_var[i] };
                    solver.addClause(unit_clause);
                }
            }
            solver.addClause(min_complement_clause);
        }
        if (foundExt) {
            extensions.insert(extension);
        } else {
            break;
        }

        complement_clause.clear();
        for (uint32_t i = 0; i < af.args; i++) {
            if (solver.model[af.accepted_var[i]-1]) {
                complement_clause.push_back(-af.accepted_var[i]);
            } else {
                //complement_clause.push_back(-af.rejected_var[i]);
            }
        }
        clauses.push_back(complement_clause);
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

bool unchallenged(const AF & af, vector<pair<string,string>> & atts) {
    vector<string> ext;
    cout << "[";
    // TODO don't print directly. check for duplicate first
    unchallenged(af, af, atts, ext);
    cout << "]\n";
    return true;
}

bool unchallenged(const AF & original_af, const AF & af, vector<pair<string,string>> & atts, vector<string> base_ext) {
    set<vector<string>> ua_uc_initial_sets = ua_or_uc_initial(af);

    if (ua_uc_initial_sets.empty()) {
        print_extension_ee(base_ext);
        return true;
    }
    
    for (const auto& ext : ua_uc_initial_sets) {
        const AF reduct = getReduct(af, ext, atts);
        //TODO trim atts after each reduct
        vector<string> new_ext;
        new_ext.insert(new_ext.end(), base_ext.begin(), base_ext.end());
        new_ext.insert(new_ext.end(), ext.begin(), ext.end());

        unchallenged(original_af, reduct, atts, new_ext);
    }
    return true;
}

/*
bool initial(const AF & af) {
    ExternalSatSolver solver = ExternalSatSolver(af.count);
    Encodings::add_admissible(af, solver);
    Encodings::add_nonempty(af, solver);

    int sat = solver.solve();
    vector<uint32_t> extension;
    for (uint32_t i = 0; i < af.args; i++) {
        if (solver.model[af.accepted_var[i]]) {
            extension.push_back(i);
        }
    }
    print_extension(af, extension);

    
    std::cout << sat << " DONE\n";
    return true;
}
*/

bool initial(const AF & af) {
    std::cout << "[";
    vector<uint32_t> extension;
    vector<int> complement_clause;
    int count = 0;
    complement_clause.reserve(af.args);
    ExternalSatSolver solver = ExternalSatSolver(af.count);
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
            if (!count++ == 0) {
                std::cout << ", ";
            }
            print_extension_ee(af, extension);
        } else {
            std::cout << "]\n";
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
	return true;
}

}