/*!
 * Copyright (c) <2020> <Andreas Niskanen, University of Helsinki>
 * 
 * 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * 
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "Encodings.h"
#include "Util.h"
#include "SkeptAcceptance.h"
#include "SingleExtension.h"
#include "EnumExtensions.h"
#include <algorithm>
#include <atomic>
#include <thread>
#include <iostream>
#include <fstream>
#include <chrono>

using namespace std;

namespace SkeptAcceptance {

struct params {
	const AF af;
	const std::string & arg;
	std::vector<std::pair<std::string,std::string>> & atts;
	std::vector<string> base_ext;
};

bool complete(const AF & af, string const & arg)
{
	ExternalSatSolver solver = ExternalSatSolver(af.count);
	vector<int> arg_rejected_clause = { -af.accepted_var[af.arg_to_int.at(arg)] };
	solver.addClause(arg_rejected_clause);
	Encodings::add_complete(af, solver);
	bool sat = solver.solve();
	return !sat;
}

bool preferred(const AF & af, string const & arg)
{
	ExternalSatSolver solver = ExternalSatSolver(af.count);
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
				if (solver.model[af.accepted_var[i]-1]) {
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

std::atomic<bool> unchallenged_ce_found{false};

bool unchallenged(const AF & af, string const & arg, vector<pair<string,string>> & atts) {
    vector<string> ext;
	//TODO Thread Logic and some kind of flag to kill all threads once counterexample is found
	params p = {af, arg, atts, ext};
    unchallenged_r(p);

    return true;
}

bool unchallenged_r(params p) {
	if (unchallenged_ce_found) {
		return true;
	}
	
    set<vector<string>> ua_uc_initial_sets = EnumExtensions::ua_or_uc_initial(p.af);

	if (unchallenged_ce_found) {
		return true;
	}

    if (ua_uc_initial_sets.empty()) {
		// should never be reached
        return false;
    }
    
    for (const auto& ext : ua_uc_initial_sets) {
		if (unchallenged_ce_found) {
		return true;
	}
		// Break Conditions
		if (std::find(ext.begin(), ext.end(), p.arg) != ext.end()) {
			return true;
		}
        const AF reduct = getReduct(p.af, ext, p.atts);
		if (reduct.arg_to_int.find(p.arg) == reduct.arg_to_int.end()) {
			cout << "NO\n";
			unchallenged_ce_found = true;
			return false;
		}
        //TODO trim atts after each reduct
        vector<string> new_ext;
        new_ext.insert(new_ext.end(), p.base_ext.begin(), p.base_ext.end());
        new_ext.insert(new_ext.end(), ext.begin(), ext.end());

		params new_p =  {reduct, p.arg, p.atts, new_ext};
		std::thread t(unchallenged_r, new_p);
		t.detach();
        //unchallenged(original_af, reduct, arg, atts, new_ext);
    }
    return true;
}

std::atomic<bool> preferred_ce_found{false};

bool preferred_p(const AF & af, string const & arg, vector<pair<string,string>> & atts) {
	preferred_ce_found = false;
    vector<string> ext;
	//TODO Thread Logic and some kind of flag to kill all threads once counterexample is found
	params p = {af, arg, atts, ext};
    preferred_p_r(p);
	
    return !preferred_ce_found;
}

bool preferred_p_r(params p) {
	// check termination flag (some other thread found a counterexample)
	if (preferred_ce_found) {
		return true;
	}

	// TODO check if arg is unattacked
	// TODO check self attack
	// TODO check if arg is in gr(AF) or attacked by it
	
    vector<string> extension;
    vector<int> complement_clause;
    complement_clause.reserve(p.af.args);
	ExternalSatSolver solver = ExternalSatSolver(p.af.count);
	
    Encodings::add_admissible(p.af, solver);
    Encodings::add_nonempty(p.af, solver);

	// iterate over the initial sets of the current AF
	while (true) {
		// check termination flag (some other thread found a counterexample)
		if (preferred_ce_found) {
			return true;
		}
		
		// Minimization of the first found model
        bool foundExt = false;
        while (true) {
            int sat = solver.solve();

			// check termination flag (some other thread found a counterexample)
			if (preferred_ce_found) {
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
	return true;
}
}