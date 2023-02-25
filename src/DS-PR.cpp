#include "Problems.h"
#include <atomic>
#include <mutex>
#include <iostream>
#include <fstream>
#include <stack>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <thread>

using namespace std;

// Variables for IPC
atomic<bool> preferred_ce_found{false};
atomic<int> thread_counter(0);
atomic<int> num_active_threads(0);
set<set<string>> checked_branches;
mutex mtx;
auto num_max_threads = std::thread::hardware_concurrency()-1;
boost::asio::thread_pool pool(num_max_threads);


namespace Problems {

/*mutoksia version of ds-pr*/
bool mt_ds_preferred(const AF & af, string const & arg)
{
	ExternalSatSolver solver = ExternalSatSolver(af.count, af.solver_path);
	Encodings::add_complete(af, solver);

	vector<int> assumptions = { -af.accepted_var[af.arg_to_int.at(arg)] };

	while (true) {
		int sat = solver.solve(assumptions);
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

bool ds_preferred(const AF & af, string const & arg, vector<pair<string,string>> & atts) {
	log(-1, arg);
	preferred_ce_found = false;
    vector<string> ext;
	params p = {af, arg, atts, ext};
    //ds_preferred_r(p);
	boost::asio::post(pool, [p] {ds_preferred_r(p);});
		
	pool.join();
    return !preferred_ce_found;
}

bool ds_preferred_r(params p) {
	int thread_id = thread_counter++;
	num_active_threads++;
	log(thread_id, "STARTING THREAD FOR IS");
	log(thread_id, "CURRENT", p.base_ext);

	// check termination flag (some other thread found a counterexample)
	if (preferred_ce_found) {
		log(thread_id, "SIGNAL --> TERM");
		num_active_threads--;
		return true;
	}
	
	/*
	================================================================================================================================
	Checking if 'arg' is self-attacking and thus never acceptable
	*/
	if (p.af.self_attack[p.af.arg_to_int.find(p.arg)->second]) {
		log(thread_id, "ARG SELF_ATTACKING --> TERM NO");
		num_active_threads--;
		preferred_ce_found = true;
		return false;
	}//==============================================================================================================================

	/*
	=================================================================================================================================
	Checking if 'arg' is an unattacked argument, as a shortcut for selecting all the unatttacked initial sets
	*/
	if (p.af.unattacked[p.af.arg_to_int.find(p.arg)->second]) {
		log(thread_id, "ARG UNATTACKED --> TERM");
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
				log(thread_id, "GROUNDED REJECTS ARG --> TERM NO");
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
							log(thread_id, "ARG GROUNDED --> TERM");
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
		log(thread_id, "REMOVED GROUNDED EXT");
		log(thread_id, "GROUNDED EXT", grounded);
	}
	//====================================================================================================================================

	if (preferred_ce_found) {
		log(thread_id, "SIGNAL --> TERM");
		num_active_threads--;
		return true;
	}

	log(thread_id, "COMPUTING SCCS");
	vector<vector<uint32_t>> sccs = computeStronglyConnectedComponents(af);
	log(thread_id, "COMPUTED SCCS");
	/*
	int arg_scc = 0;
	for (size_t i = 0; i < sccs.size(); i++) {
		log(thread_id, "SCC", sccs[i], af);
		if (sccs[i].size() == 1) {
			continue;
		}
		if (std::find(sccs[i].begin(), sccs[i].end(), af.arg_to_int.find(p.arg)->second) != sccs[i].end()) {
			log(thread_id, "STARTING PRIORITY SEARCH");
			log(thread_id, "SCC", sccs[i], af);
			params2 new_p =  {af, p.arg, p.atts, p.base_ext, sccs[i]};
			//ds_preferred_r_scc(new_p);
			boost::asio::post(pool, [new_p] {ds_preferred_r_scc(new_p);});
			arg_scc = i;
		}
		if (preferred_ce_found) {
			log(thread_id, "SIGNAL --> TERM");
			num_active_threads--;
			return true;
		}
	}
	*/
	if (preferred_ce_found) {
		log(thread_id, "SIGNAL --> TERM");
		num_active_threads--;
		return true;
	}

	log(thread_id, "STARTING SCC LOOP");
	for (size_t i = 0; i < sccs.size(); i++) {
		if (preferred_ce_found) {
			log(thread_id, "SIGNAL --> TERM");
			num_active_threads--;
			return true;
		}
		// TODO sound?
		// If SCC consists of only one argument, it won't have any initial set.
		// If a SCC of size one has an initial set, it would be an unattacked initial set, 
		// which cannot be since we accepted all unattacked initial sets via the grounded extension already.
		if (sccs[i].size() == 1) {
			continue;
		}
		//if (i==arg_scc) continue;
		// check termination flag (some other thread found a counterexample)
		if (preferred_ce_found) {
			log(thread_id, "SIGNAL --> TERM");
			num_active_threads--;
			return true;
		}

		params2 new_p =  {af, p.arg, p.atts, p.base_ext, sccs[i]};
		//ds_preferred_r_scc(new_p);
		log(thread_id, "DETACHING TASK FOR SCC");
		boost::asio::post(pool, [new_p] {ds_preferred_r_scc(new_p);});
	}

	log(thread_id, "LOOPED ALL SCCS --> TERM");
	num_active_threads--;
	return true;
}

bool ds_preferred_r_scc(params2 p) {
	int thread_id = thread_counter++;
	num_active_threads++;
	log(thread_id, "STARTING THREAD FOR SCC");
	log(thread_id, "SCC", p.scc, p.af);

	// check termination flag (some other thread found a counterexample)
	if (preferred_ce_found) {
		log(thread_id, "SIGNAL --> TERM");
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
			log(thread_id, "SIGNAL --> TERM");
			num_active_threads--;
			return true;
		}
		
		// Minimization of the first found model
        bool foundExt = false;
        while (true) {
			log(thread_id, "LOOKING FOR NEW MODEL");
            int sat = solver.solve(thread_id);
			log(thread_id, "SOLVER RESPONDED");

			// check termination flag (some other thread found a counterexample)
			if (preferred_ce_found) {
				log(thread_id, "SIGNAL --> TERM");
				num_active_threads--;
				return true;
			}

            if (sat == 20) break;
            foundExt = true;
            extension.clear();
            for (uint32_t i = 0; i < p.af.args; i++) {
                if (solver.model[p.af.accepted_var[i]]) {
                    extension.push_back(p.af.int_to_arg[i]);
                }
            }
			log(thread_id, "MODEL", extension);

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
			log(thread_id, "MINIMAL MODEL", extension);
			// Break Conditions

			// If arg is in the initial set, the preferred extension accepts it and will never be a counterexample
			if (std::find(extension.begin(), extension.end(), p.arg) != extension.end()) {
				log(thread_id, "MODEL ACCEPTS ARG --> SKIP");
				// Extension would contain arg, no thread needs to be created and continue with the next initial set
			} else {
				//TODO trim atts after each reduct?
				vector<string> new_ext;
				set<string> branch;
				
				for(auto const& arg: p.base_ext) {
					new_ext.push_back(arg);
					branch.insert(arg);
				}
				for(auto const& arg: extension) {
					new_ext.push_back(arg);
					branch.insert(arg);
				}
				mtx.lock();
				bool already_checked = checked_branches.find(branch) != checked_branches.end();
				mtx.unlock();
				if (already_checked) {
					log(thread_id, "BRANCH ALREADY CHECKED --> SKIP");
				} else {
					mtx.lock();
					checked_branches.insert(branch);
					mtx.unlock();
				// If there exists an attack from extension to arg, the model rejects arg thus we found a counterexample
				for(auto const& arg: extension) {
					if (p.af.att_exists.find(make_pair(p.af.arg_to_int.find(arg)->second, p.af.arg_to_int.find(p.arg)->second)) != p.af.att_exists.end()) {
						log(thread_id, "MODEL REJECTS ARG --> TERM NO");
						num_active_threads--;
						preferred_ce_found = true;
						return false;
					}
				}

				const AF reduct = getReduct(p.af, extension, p.atts);

				log(thread_id, "DETACHING NEW TASK");
				params new_p =  {reduct, p.arg, p.atts, new_ext};
				boost::asio::post(pool, [new_p] {ds_preferred_r(new_p);});
				}
			}
        } else {
			log(thread_id, "NO MORE INITIAL SETS --> TERM");
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
		log(thread_id, "ADDED COMPLEMENT CLAUSE");
	}
	log(thread_id, "DONE --> TERM");
	num_active_threads--;
	return true;
}

}