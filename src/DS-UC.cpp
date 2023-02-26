#include "Problems.h"					// Header for all Problem methods

#include <algorithm>					// std::find
#include <atomic>						// for tracking status of counterexample search
#include <mutex>						// for checking duplicate thread creating
#include <thread>						// std::thread::hardware_concurrency()

#include <boost/asio/thread_pool.hpp>	// handles threads
#include <boost/asio/post.hpp>			// for submitting tasks to thread pool

using namespace std;

// Variables for IPC
std::atomic<bool> unchallenged_ce_found{false};

// For preventing duplicate thread creation
set<set<string>> checked_branches_dsuc;
mutex mtx_dsuc;

// Initializing thread pool
int num_max_threads_dsuc = std::thread::hardware_concurrency()-1;
boost::asio::thread_pool pool_dsuc(num_max_threads_dsuc);


namespace Problems {

/*
helper function (threaded) for the DS-UC problem that recursively searches for a counterexample

@param af		the argumentation framework
@param arg		the argument to be decided
@param atts		list of all attacks of the AF (only for faster reduct construction)
@param base_ext	the current status of the extension that is constructed by this thread	

@returns 'false' if the current extension is a counterexample for the skeptical acceptance of arg, 'true' if arg is accepted by the constructed extension
*/
bool ds_unchallenged_r(const AF & af, std::string const & arg, std::vector<std::pair<std::string,std::string>> & atts, std::vector<std::string> base_ext) {
	if (unchallenged_ce_found) {
		return true;
	}
	
	// Find all unattacked and unchallenged initial sets first
    set<vector<string>> ua_uc_initial_sets = get_ua_or_uc_initial(af);

	if (unchallenged_ce_found) {
		return true;
	}

	// If no unattacked/unchallenged initial set exists, base_ext is a counterexample for the skeptical acceptance of 'arg'
    if (ua_uc_initial_sets.empty()) {
		unchallenged_ce_found = true;
        return false;
    }
    
	// For each unattacked or unchallenged initial set, check their relation to 'arg' and (if necessary) create a new thread for the respective reduct and initial set
    for (const auto& ext : ua_uc_initial_sets) {
		if (unchallenged_ce_found) {
		return true;
	}
		// If the initial set accepts 'arg' is will not be a counterexample and can be skipped
		if (std::find(ext.begin(), ext.end(), arg) != ext.end()) {
			continue;
		}
		// If there exists an attack from the initial set to 'arg', the model rejects arg, thus we found a counterexample
		for(auto const& a: ext) {
			if (af.att_exists.find(make_pair(af.arg_to_int.find(a)->second, af.arg_to_int.find(arg)->second)) != af.att_exists.end()) {
				unchallenged_ce_found = true;
				return false;
			}
		}

		// Check if a thread with the same extension has already been created
		vector<string> new_ext;
		set<string> branch;
		for(auto const& a: base_ext) {
			new_ext.push_back(a);
			branch.insert(a);
		}
		for(auto const& a: ext) {
			new_ext.push_back(a);
			branch.insert(a);
		}
		mtx_dsuc.lock();
		bool already_checked = checked_branches_dsuc.find(branch) != checked_branches_dsuc.end();
		mtx_dsuc.unlock();
		// The extension has not been checked before, that means a new thread can be created
		if (!already_checked) {
			mtx_dsuc.lock();
			checked_branches_dsuc.insert(branch);
			mtx_dsuc.unlock();
			const AF reduct = getReduct(af, ext, atts);
			boost::asio::post(pool_dsuc, [reduct, arg, &atts, new_ext] {ds_unchallenged_r(reduct, arg, atts, new_ext);});
		}
    }
    return true;
}

/*
Main method for solving the DS-UC problem

@param af	the argumentation framework
@param arg	the argument to be decided
@param atts	list of all attacks of the AF (only for faster reduct construction)

@returns 'true' if arg is skeptically accepted wrt unchallenged semantics, 'false' otherwise
*/
bool ds_unchallenged(const AF & af, string const & arg, std::vector<std::pair<std::string,std::string>> & atts) {
	unchallenged_ce_found = false;
    vector<string> ext;

	boost::asio::post(pool_dsuc, [af, arg, &atts, ext] {ds_unchallenged_r(af, arg, atts, ext);});

	pool_dsuc.join();
    return !unchallenged_ce_found;
}

}