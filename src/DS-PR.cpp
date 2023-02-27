#include "Problems.h"						// Header for all Problem methods

#include <atomic>							// for tracking number of threads und status of counterexample search
#include <mutex>							// for checking duplicate thread creating
#include <stack>							// for computing the grounded extension
#include <algorithm>						// std::find
#include <thread>							// std::thread::hardware_concurrency()

#include <boost/asio/thread_pool.hpp>		// handles threads
#include <boost/asio/post.hpp>				// for submitting tasks to thread pool

using namespace std;

#if defined(DEBUG_MODE)
atomic<int> thread_counter(0);
#endif

// Variables for Inter-Process Communication
atomic<bool> preferred_ce_found{false};

// Structure for preventing duplicate thread creation
set<set<string>> checked_branches;
mutex mtx;

// Initialize Thread Pool with (#sys_threads - 1) number of threads
int num_max_threads_dspr = std::thread::hardware_concurrency()-1;
boost::asio::thread_pool pool(num_max_threads_dspr);

namespace Problems {

/*
helper function (threaded) for the DS-PR problem that catches simple cases and then starts new threads for each SCC

@param af		the argumentation framework
@param arg		the argument to be decided
@param atts		list of all attacks of the AF (only for faster reduct construction)
@param base_ext	the current status of the extension that is constructed by this thread	

@returns 'false' if the current extension is a counterexample for the skeptical acceptance of arg, 'true' if arg is accepted by the constructed extension
*/
bool ds_preferred_r(const AF & af, string const & arg, vector<pair<string,string>> & atts, vector<string> base_ext) {
	#if defined(DEBUG_MODE)
	int thread_id = thread_counter++;
	log(thread_id, "STARTING THREAD FOR IS");
	log(thread_id, "CURRENT", base_ext);
	#endif

	// check termination flag (some other thread found a counterexample)
	if (preferred_ce_found) {
		#if defined(DEBUG_MODE)
		log(thread_id, "SIGNAL --> TERM");
		#endif
		return true;
	}
	
	// Checking if 'arg' is self-attacking and thus never acceptable
	if (af.self_attack[af.arg_to_int.find(arg)->second]) {
		#if defined(DEBUG_MODE)
		log(thread_id, "ARG SELF_ATTACKING --> TERM NO");
		#endif
		preferred_ce_found = true;
		return false;
	}

	//Checking if 'arg' is an unattacked argument, as a shortcut for selecting all the unatttacked initial sets
	if (af.unattacked[af.arg_to_int.find(arg)->second]) {
		#if defined(DEBUG_MODE)
		log(thread_id, "ARG UNATTACKED --> TERM");
		#endif
		return true;
	}

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
	num_attackers.resize(af.args, 0);
	vector<string> grounded;
	vector<bool> grounded_out;
	grounded_out.resize(af.args, false);
	stack<uint32_t> arg_stack;
	for (size_t i = 0; i < af.args; i++) {
		if (af.unattacked[i]) {
			grounded.push_back(af.int_to_arg[i]);
			arg_stack.push(i);
		}
		num_attackers[i] = af.attackers[i].size();
	}
	while (arg_stack.size() > 0) {
		uint32_t a = arg_stack.top();
		arg_stack.pop();
		for (auto const& arg1: af.attacked[a]) {
			if (grounded_out[arg1]) {
				continue;
			}
			if (af.int_to_arg[arg1] == arg) {
				#if defined(DEBUG_MODE)
				log(thread_id, "GROUNDED REJECTS ARG --> TERM NO");
				#endif
				preferred_ce_found = true;
				return false;
			}
			
			grounded_out[arg1] = true;
			for (auto const& arg2: af.attacked[arg1]) {
				if (num_attackers[arg2] > 0) {
					num_attackers[arg2]--;
					if (num_attackers[arg2] == 0) {
						if (af.int_to_arg[arg2] == arg) {
							#if defined(DEBUG_MODE)
							log(thread_id, "ARG GROUNDED --> TERM");
							#endif
							return true;
						}
						grounded.push_back(af.int_to_arg[arg2]);
						arg_stack.push(arg2);
					}
				}
			}
		}
	}
	AF new_af = af;
	if (!grounded.empty()) {
		new_af = getReduct(af, grounded, atts);
		#if defined(DEBUG_MODE)
		log(thread_id, "REMOVED GROUNDED EXT", grounded);
		#endif
	}
	//====================================================================================================================================

	if (preferred_ce_found) {
		#if defined(DEBUG_MODE)
		log(thread_id, "SIGNAL --> TERM");
		#endif
		return true;
	}

	// Initializing the SAT solver and creating the encodings for initial sets
	vector<string> extension;
    vector<int> complement_clause;
    complement_clause.reserve(new_af.args);
	SAT_Solver solver = SAT_Solver(new_af.count, new_af.solver_path);
	Encodings::add_admissible(new_af, solver);
    Encodings::add_nonempty(new_af, solver);	

	// Iterate over the initial sets of the current AF
	bool no_initial_set_exists = true;
	while (true) {
		// check termination flag (some other thread found a counterexample)
		if (preferred_ce_found) {
			#if defined(DEBUG_MODE)
			log(thread_id, "SIGNAL --> TERM");
			#endif
			return true;
		}
		
		// Search for a sub-model to determine whether it is minimal (i.e. initial)
        bool foundExt = false;
        while (true) {
			// check termination flag (some other thread found a counterexample)
			if (preferred_ce_found) {
				#if defined(DEBUG_MODE)
				log(thread_id, "SIGNAL --> TERM");
				#endif
				return true;
			}

			#if defined(DEBUG_MODE)
			log(thread_id, "LOOKING FOR NEW MODEL");
			#endif
            int sat = solver.solve();
			#if defined(DEBUG_MODE)
			log(thread_id, "SOLVER RESPONDED");
			#endif

			// check termination flag (some other thread found a counterexample)
			if (preferred_ce_found) {
				#if defined(DEBUG_MODE)
				log(thread_id, "SIGNAL --> TERM");
				#endif
				return true;
			}

			// If no more model exists, stop searching
            if (sat == 20) break;

			// Parse the found model
            foundExt = true;
			no_initial_set_exists = false;
            extension.clear();
            for (uint32_t i = 0; i < new_af.args; i++) {
                if (solver.model[new_af.accepted_var[i]]) {
                    extension.push_back(new_af.int_to_arg[i]);
                }
            }

			#if defined(DEBUG_MODE)
			log(thread_id, "MODEL", extension);
			#endif

			// No minimization needed, if extension has already size of 1
			if (extension.size() == 1) {
				break;
			}
			
			// Add temporary clauses to force the solver to look for a subset of the extension
            vector<int> min_complement_clause;
            min_complement_clause.reserve(new_af.args);
            for (uint32_t i = 0; i < new_af.args; i++) {
                if (solver.model[new_af.accepted_var[i]]) {
                    min_complement_clause.push_back(-new_af.accepted_var[i]);
                } else {
                    vector<int> unit_clause = { -new_af.accepted_var[i] };
                    solver.addMinimizationClause(unit_clause);
                }
            }
            solver.addMinimizationClause(min_complement_clause);
        }
		// If an initial set has been found , start a new thread with the initial set and the respective reduct
        if (foundExt) {
			#if defined(DEBUG_MODE)
			log(thread_id, "MINIMAL MODEL", extension);
			#endif

			// If 'arg' is in the initial set, the preferred extension accepts it and will never be a counterexample, 
			// i.e., we continue without creating a thread for it
			if (std::find(extension.begin(), extension.end(), arg) != extension.end()) {
				#if defined(DEBUG_MODE)
				log(thread_id, "MODEL ACCEPTS ARG --> SKIP");
				#endif
			} else {
				// If there exists an attack from the initial set to 'arg', the model rejects arg, thus we found a counterexample
				for(auto const& a: extension) {
					if (new_af.att_exists.find(make_pair(new_af.arg_to_int.find(a)->second, new_af.arg_to_int.find(arg)->second)) != new_af.att_exists.end()) {
						#if defined(DEBUG_MODE)
						log(thread_id, "MODEL REJECTS ARG --> TERM NO");
						#endif
						preferred_ce_found = true;
						return false;
					}
				}

				// Check whether the current extension (base_ext + initial set) has already been checked by a different thread
				vector<string> new_ext;
				set<string> branch;
				for(auto const& a: base_ext) {
					new_ext.push_back(a);
					branch.insert(a);
				}
				for(auto const& a: extension) {
					new_ext.push_back(a);
					branch.insert(a);
				}
				mtx.lock();
				bool already_checked = checked_branches.find(branch) != checked_branches.end();
				mtx.unlock();
				if (already_checked) {
					#if defined(DEBUG_MODE)
					log(thread_id, "BRANCH ALREADY CHECKED --> SKIP");
					#endif
				} else {
					// The extension has not been checked before, that means a new thread can be created
					mtx.lock();
					checked_branches.insert(branch);
					mtx.unlock();

					#if defined(DEBUG_MODE)
					log(thread_id, "DETACHING NEW TASK");
					#endif
					const AF reduct = getReduct(new_af, extension, atts);
					boost::asio::post(pool, [reduct, arg, &atts, new_ext] {ds_preferred_r(reduct, arg, atts, new_ext);});
				}
			}
        } else {
			// No further initial set has been found for the AF
			if (no_initial_set_exists) {
				// The AF has no initial sets at all: base_ext+gr is a preferred extension and since it does not contain 'arg' it is a counterexample
				#if defined(DEBUG_MODE)
				log(thread_id, "CURRENT EXT IS PREFERRED AND ARG NOT INCLUDED --> TERM NO");
				#endif
				preferred_ce_found = true;
            	return false;
			} else {
				// The AF has had at least one initial set: no definite decision possible yet, search continues in the created threads
				#if defined(DEBUG_MODE)
				log(thread_id, "NO MORE INITIAL SETS --> TERM");
				#endif
            	return true;
			}
        }

		// Add complement clause to prevent the just found initial set
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
	#if defined(DEBUG_MODE)
	log(thread_id, "DONE --> TERM");
	#endif	
	return true;
}

/*
Main method for solving the DS-PR problem

@param af	the argumentation framework
@param arg	the argument to be decided
@param atts	list of all attacks of the AF (only for faster reduct construction)

@returns 'true' if arg is skeptically accepted wrt preferred semantics, 'false' otherwise
*/
bool ds_preferred(const AF & af, string const & arg, vector<pair<string,string>> & atts) {
	preferred_ce_found = false;

	// Initialize search, starting with the empty set
    vector<string> ext;
	boost::asio::post(pool, [af, arg, &atts, ext] {ds_preferred_r(af, arg, atts, ext);});
		
	// Wait for all threads to finish and return result
	// TODO Optimize pool destruction if counterexample has been found
	pool.join();
    return !preferred_ce_found;
}

//================================================================================================================================================
//================================================================================================================================================

/*!
 * The following is largely taken from the mu-toksia solver
 * and is subject to the following licence.
 *
 * 
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

/*mutoksia version of ds-pr, for internal testing purposes*/
bool mt_ds_preferred(const AF & af, string const & arg) {
	SAT_Solver solver = SAT_Solver(af.count, af.solver_path);
	Encodings::add_complete(af, solver);

	vector<int> assumptions = { -af.accepted_var[af.arg_to_int.at(arg)] };

	while (true) {
		int sat = solver.solve(assumptions);
		for (size_t i = 0; i < solver.model.size(); i++) {
			cout << i << ": " << solver.model[i] << "\n";
		}
		
		if (sat == 20) break;

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
			int superset_exists = solver.solve(new_assumptions);
			if (superset_exists == 20) break;
		}

		new_assumptions[0] = -new_assumptions[0];

		if (solver.solve(new_assumptions) == 20) {
			return false;
		}
	}
	return true;
}

}