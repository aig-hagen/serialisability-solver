#include "Problems.h"						// Header for all Problem methods

#include <atomic>							// for tracking number of threads und status of counterexample search
#include <mutex>							// for checking duplicate thread creating
#include <stack>							// for computing the grounded extension
#include <algorithm>						// std::find
#include <thread>							// std::thread::hardware_concurrency()

#include <boost/asio/thread_pool.hpp>		// handles threads
#include <boost/asio/post.hpp>				// for submitting tasks to thread pool

using namespace std;

#if defined(DEBUG_MODE_EE)
atomic<int> thread_counter(0);
#endif

// Structure for preventing duplicate thread creation
set<set<string>> checked_branches_eepr;
mutex mtx_eepr;

set<set<string>> printed_branches_eepr;
mutex mtx_printed_eepr;

// Initialize Thread Pool with (#sys_threads - 1) number of threads
int num_max_threads_eepr = std::thread::hardware_concurrency()-1;
boost::asio::thread_pool pool_eepr(num_max_threads_eepr);

namespace Problems {

/*
helper function (threaded) for the EE-PR problem

@param af		the argumentation framework
@param atts		list of all attacks of the AF (only for faster reduct construction)
@param base_ext	the current status of the extension that is constructed by this thread	

@returns 'true' once all preferred extensions have been computed
*/
bool ee_preferred_r(const AF & af, vector<pair<string,string>> & atts, vector<string> base_ext) {
	#if defined(DEBUG_MODE_EE)
	int thread_id = thread_counter++;
	log(thread_id, "STARTING THREAD FOR IS");
	log(thread_id, "CURRENT", base_ext);
	#endif

	vector<string> grounded = se_grounded(af);
	AF new_af = af;
	if (!grounded.empty()) {
		new_af = getReduct(af, grounded, atts);
		#if defined(DEBUG_MODE_EE)
		log(thread_id, "REMOVED GROUNDED EXT", grounded);
		#endif
	}

    // Initializing the SAT solver and creating the encodings for initial sets
	vector<string> extension;
    vector<int> complement_clause;
    complement_clause.reserve(new_af.args);
	SAT_Solver solver = SAT_Solver(af.count, new_af.solver_path);
	Encodings::add_admissible(new_af, solver);
    Encodings::add_nonempty(new_af, solver);	

	// Iterate over the initial sets of the SCC of the current AF
	bool no_initial_set_exists = true;
	while (true) {		
		// Search for a sub-model to determine whether it is minimal (i.e. initial)
        bool foundExt = false;
        while (true) {
			#if defined(DEBUG_MODE_EE)
			log(thread_id, "LOOKING FOR NEW MODEL");
			#endif
            int sat = solver.solve();
			#if defined(DEBUG_MODE_EE)
			log(thread_id, "SOLVER RESPONDED");
			#endif

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

			#if defined(DEBUG_MODE_EE)
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
			#if defined(DEBUG_MODE_EE)
			log(thread_id, "MINIMAL MODEL", extension);
			#endif

            // Check whether the current extension (base_ext + initial set) has already been checked by a different thread
            vector<string> new_ext;
            new_ext.reserve(new_af.args);
            set<string> branch;
            for(auto const& a: base_ext) {
                new_ext.push_back(a);
                branch.insert(a);
            }
            for(auto const& a: grounded) {
                new_ext.push_back(a);
                branch.insert(a);
            }
            for(auto const& a: extension) {
                new_ext.push_back(a);
                branch.insert(a);
            }
            mtx_eepr.lock();
            bool already_checked = checked_branches_eepr.find(branch) != checked_branches_eepr.end();
            mtx_eepr.unlock();
            if (!already_checked) {
                // The extension has not been checked before, that means a new thread can be created
                mtx_eepr.lock();
                checked_branches_eepr.insert(branch);
                mtx_eepr.unlock();

                #if defined(DEBUG_MODE_EE)
                log(thread_id, "DETACHING NEW TASK");
                #endif
                const AF reduct = getReduct(new_af, extension, atts);
                boost::asio::post(pool_eepr, [reduct, &atts, new_ext] {ee_preferred_r(reduct, atts, new_ext);});
            }
        } else {
			// No further initial set has been found for the AF
            if (no_initial_set_exists) {
                // since there is no initial set, base_ext + grounded is a preferred extension
                vector<string> preferred_ext;
                preferred_ext.reserve(new_af.args);
                set<string> branch;
                for(auto const& a: base_ext) {
                    preferred_ext.push_back(a);
                    branch.insert(a);
                }
                for(auto const& a: grounded) {
                    preferred_ext.push_back(a);
                    branch.insert(a);
                }

                #if defined(DEBUG_MODE_EE)
                log(thread_id, "FOUND PREFERRED EXTENSION", preferred_ext);
                #endif

                mtx_printed_eepr.lock();
                bool already_printed = printed_branches_eepr.find(branch) != printed_branches_eepr.end();
                mtx_printed_eepr.unlock();
                if (!already_printed) {
                    mtx_printed_eepr.lock();
                    printed_branches_eepr.insert(branch);
                    mtx_printed_eepr.unlock();
                    print_extension_ee_parallel(preferred_ext);
                }
            }
            return true; 
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
	#if defined(DEBUG_MODE_EE)
	log(thread_id, "DONE --> TERM");
	#endif
	return true;
}

/*
Main method for solving the EE-PR problem

@param af	the argumentation framework
@param arg	the argument to be decided
@param atts	list of all attacks of the AF (only for faster reduct construction)

@returns 'true' once all preferred extensions have been computed
*/
bool ee_preferred(const AF & af, vector<pair<string,string>> & atts) {
    std::cout << "[";
	// Initialize search, starting with the empty set
    vector<string> ext;
	boost::asio::post(pool_eepr, [af, &atts, ext] {ee_preferred_r(af, atts, ext);});
		
	// Wait for all threads to finish and return result
	pool_eepr.join();
    std::cout << "]\n";
    return true;
}

}