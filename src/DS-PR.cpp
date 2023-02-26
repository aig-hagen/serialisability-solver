#include "Problems.h"						// Header for all Problem methods

#include <atomic>							// for tracking number of threads und status of counterexample search
#include <mutex>							// for checking duplicate thread creating
#include <stack>							// for computing the grounded extension
#include <thread>							// thread::hardware_concurrency(); to get number of max threads
#include <algorithm>						// std::find

#include <boost/asio/thread_pool.hpp>		// handles threads
#include <boost/asio/post.hpp>				// for submitting tasks to thread pool
#include <boost/asio/io_context.hpp>		// for interrupting the thread pool once the search is done

using namespace std;

// Variables for Inter-Process Communication
atomic<bool> preferred_ce_found{false};
atomic<int> thread_counter(0);

// Structure for preventing duplicate thread creation
set<set<string>> checked_branches;
mutex mtx;

// Initialize Thread Pool with (#sys_threads - 1) number of threads
auto num_max_threads = std::thread::hardware_concurrency()-1;
boost::asio::thread_pool pool(num_max_threads);
boost::asio::io_context io_context;

namespace Problems {

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

/*
helper function for the DS-PR problem that catches simple cases and then starts new threads for each SCC

@param af		the argumentation framework
@param arg		the argument to be decided
@param atts		list of all attacks of the AF (only for faster reduct construction)
@param base_ext	the current status of the extension that is constructed by this thread	

@returns 'false' if the current extension is a counterexample for the skeptical acceptance of arg, 'true' if arg is accepted by the constructed extension
*/
bool ds_preferred_r(const AF & af, string const & arg, vector<pair<string,string>> & atts, vector<string> base_ext) {
	int thread_id = thread_counter++;
	//log(thread_id, "STARTING THREAD FOR IS");
	//log(thread_id, "CURRENT", base_ext);

	// check termination flag (some other thread found a counterexample)
	if (preferred_ce_found) {
		//log(thread_id, "SIGNAL --> TERM");
		return true;
	}
	
	// Checking if 'arg' is self-attacking and thus never acceptable
	if (af.self_attack[af.arg_to_int.find(arg)->second]) {
		//log(thread_id, "ARG SELF_ATTACKING --> TERM NO");
		preferred_ce_found = true;
		io_context.stop();
		return false;
	}

	//Checking if 'arg' is an unattacked argument, as a shortcut for selecting all the unatttacked initial sets
	if (af.unattacked[af.arg_to_int.find(arg)->second]) {
		//log(thread_id, "ARG UNATTACKED --> TERM");
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
				//log(thread_id, "GROUNDED REJECTS ARG --> TERM NO");
				preferred_ce_found = true;
				io_context.stop();
				return false;
			}
			
			grounded_out[arg1] = true;
			for (auto const& arg2: af.attacked[arg1]) {
				if (num_attackers[arg2] > 0) {
					num_attackers[arg2]--;
					if (num_attackers[arg2] == 0) {
						if (af.int_to_arg[arg2] == arg) {
							//log(thread_id, "ARG GROUNDED --> TERM");
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
		//log(thread_id, "REMOVED GROUNDED EXT");
		//log(thread_id, "GROUNDED EXT", grounded);
	}
	//====================================================================================================================================

	if (preferred_ce_found) {
		//log(thread_id, "SIGNAL --> TERM");
		return true;
	}

	// Compute the SCCs of the current argumentation framework, we can then spawn one thread for each SCC to search for initial sets more effectively
	//log(thread_id, "COMPUTING SCCS");
	vector<vector<uint32_t>> sccs = computeStronglyConnectedComponents(new_af);
	//log(thread_id, "COMPUTED SCCS");
	
	if (preferred_ce_found) {
		//log(thread_id, "SIGNAL --> TERM");
		return true;
	}

	// For each SCC, create a new thread to search for initial sets
	//log(thread_id, "STARTING SCC LOOP");
	for (size_t i = 0; i < sccs.size(); i++) {
		if (preferred_ce_found) {
			//log(thread_id, "SIGNAL --> TERM");
			return true;
		}
		// If SCC consists of only one argument, it won't have any initial set.
		// If a SCC of size one has an initial set, it would be an unattacked initial set, 
		// which cannot be since we accepted all unattacked initial sets via the grounded extension already.
		if (sccs[i].size() == 1) {
			continue;
		}

		//log(thread_id, "DETACHING TASK FOR SCC");
		vector<uint32_t> scc = sccs[i];
		boost::asio::post(pool, [new_af, arg, &atts, base_ext, scc] {ds_preferred_r_scc(new_af, arg, atts, base_ext, scc);});
	}

	//log(thread_id, "LOOPED ALL SCCS --> TERM");
	return true;
}

/*
helper function for the DS-PR problem that searches for the initial sets of a SCC and creates new threads for each initial set found

@param af		the argumentation framework
@param arg		the argument to be decided
@param atts		list of all attacks of the AF (only for faster reduct construction)
@param base_ext	the current status of the extension that is constructed by this thread	
@param scc		the SCC of AF that the search for initial sets should be restricted to

@returns 'false' if the current extension is a counterexample for the skeptical acceptance of arg, 'true' if arg is accepted by the constructed extension
*/
bool ds_preferred_r_scc(const AF & af, std::string const & arg, std::vector<std::pair<std::string,std::string>> & atts, std::vector<std::string> base_ext, std::vector<uint32_t> scc) {
	int thread_id = thread_counter++;
	//log(thread_id, "STARTING THREAD FOR SCC");
	//log(thread_id, "SCC", scc, af);

	// check termination flag (some other thread found a counterexample)
	if (preferred_ce_found) {
		//log(thread_id, "SIGNAL --> TERM");
		return true;
	}

	// Initializing the SAT solver and creating the encodings for initial sets
	vector<string> extension;
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
    Encodings::add_nonempty_subset_of(af, scc, solver);	

	// Iterate over the initial sets of the SCC of the current AF
	bool no_initial_set_exists = true;
	while (true) {
		// check termination flag (some other thread found a counterexample)
		if (preferred_ce_found) {
			//log(thread_id, "SIGNAL --> TERM");
			return true;
		}
		
		// Search for a sub-model to determine whether it is minimal (i.e. initial)
        bool foundExt = false;
        while (true) {
			// check termination flag (some other thread found a counterexample)
			if (preferred_ce_found) {
				//log(thread_id, "SIGNAL --> TERM");
				return true;
			}

			//log(thread_id, "LOOKING FOR NEW MODEL");
            int sat = solver.solve();
			//log(thread_id, "SOLVER RESPONDED");

			// check termination flag (some other thread found a counterexample)
			if (preferred_ce_found) {
				//log(thread_id, "SIGNAL --> TERM");
				return true;
			}

			// If no more model exists, stop searching
            if (sat == 20) break;

			// Parse the found model
            foundExt = true;
			no_initial_set_exists = false;
            extension.clear();
            for (uint32_t i = 0; i < af.args; i++) {
                if (solver.model[af.accepted_var[i]]) {
                    extension.push_back(af.int_to_arg[i]);
                }
            }
			//log(thread_id, "MODEL", extension);

			// No minimization needed, if extension has already size of 1
			if (extension.size() == 1) {
				break;
			}
			
			// Add temporary clauses to force the solver to look for a subset of the extension
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
		// If an initial set has been found , start a new thread with the initial set and the respective reduct
        if (foundExt) {
			//log(thread_id, "MINIMAL MODEL", extension);

			// If 'arg' is in the initial set, the preferred extension accepts it and will never be a counterexample, 
			// i.e., we continue without creating a thread for it
			if (std::find(extension.begin(), extension.end(), arg) != extension.end()) {
				//log(thread_id, "MODEL ACCEPTS ARG --> SKIP");
			} else {
				// If there exists an attack from the initial set to 'arg', the model rejects arg, thus we found a counterexample
				for(auto const& a: extension) {
					if (af.att_exists.find(make_pair(af.arg_to_int.find(a)->second, af.arg_to_int.find(arg)->second)) != af.att_exists.end()) {
						//log(thread_id, "MODEL REJECTS ARG --> TERM NO");
						preferred_ce_found = true;
						io_context.stop();
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
					//log(thread_id, "BRANCH ALREADY CHECKED --> SKIP");
				} else {
					// The extension has not been checked before, that means a new thread can be created
					mtx.lock();
					checked_branches.insert(branch);
					mtx.unlock();

					//log(thread_id, "DETACHING NEW TASK");
					const AF reduct = getReduct(af, extension, atts);
					boost::asio::post(pool, [reduct, arg, &atts, new_ext] {ds_preferred_r(reduct, arg, atts, new_ext);});
				}
			}
        } else {
			// No further initial set has been found for the SCC
			if (no_initial_set_exists) {
				// The SCC has no initial sets at all: base_ext is a preferred extension and since it does not contain 'arg' it is a counterexample
				//log(thread_id, "CURRENT EXT IS PREFERRED AND ARG NOT INCLUDED --> TERM NO");
				preferred_ce_found = true;
				io_context.stop();
            	return false;
			} else {
				// The SCC has had at least one initial set: no definite decision possible yet, search continues in the created threads
				//log(thread_id, "NO MORE INITIAL SETS --> TERM");
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
	//log(thread_id, "DONE --> TERM");
	return true;
}

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
	#if defined(SAT_EXTERNAL)
	SAT_Solver solver = SAT_Solver(af.count, af.solver_path);
	#elif defined(SAT_CMSAT)
	SAT_Solver solver = SAT_Solver(af.count, af.args);
	#else
	#error "No SAT solver defined"
	#endif
	Encodings::add_complete(af, solver);

	vector<int> assumptions = { -af.accepted_var[af.arg_to_int.at(arg)] };

	while (true) {
		//log(0, "LOOKING FOR MODEL");
		int sat = solver.solve(assumptions);
		//log(0, "SOLVER RESPONDED");
		//log(0, sat);
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