#include "Problems.h"

#include <algorithm>
#include <atomic>
#include <thread>
#include <mutex>

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>

using namespace std;

namespace Problems {

std::atomic<bool> unchallenged_ce_found{false};
atomic<int> thread_counter(0);
set<set<string>> checked_branches_dsuc;
mutex mtx_dsuc;
auto num_max_threads = std::thread::hardware_concurrency()-1;
boost::asio::thread_pool pool_dsuc(num_max_threads);

bool ds_unchallenged(const AF & af, string const & arg, std::vector<std::pair<std::string,std::string>> & atts) {
	unchallenged_ce_found = false;
    vector<string> ext;

	boost::asio::post(pool_dsuc, [af, arg, &atts, ext] {ds_unchallenged_r(af, arg, atts, ext);});

	pool_dsuc.join();
    return !unchallenged_ce_found;
}

bool ds_unchallenged_r(const AF & af, std::string const & arg, std::vector<std::pair<std::string,std::string>> & atts, std::vector<std::string> base_ext) {
	if (unchallenged_ce_found) {
		return true;
	}
	
    set<vector<string>> ua_uc_initial_sets = get_ua_or_uc_initial(af);

	if (unchallenged_ce_found) {
		return true;
	}

    if (ua_uc_initial_sets.empty()) {
        return true;
    }
    
    for (const auto& ext : ua_uc_initial_sets) {
		if (unchallenged_ce_found) {
		return true;
	}
		// Break Conditions
		if (std::find(ext.begin(), ext.end(), arg) != ext.end()) {
			continue;
		}
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
		if (already_checked) {
			//log(thread_id, "BRANCH ALREADY CHECKED --> SKIP");
		} else {
			mtx_dsuc.lock();
			checked_branches_dsuc.insert(branch);
			mtx_dsuc.unlock();
		}
        const AF reduct = getReduct(af, ext, atts);
		if (reduct.arg_to_int.find(arg) == reduct.arg_to_int.end()) {
			unchallenged_ce_found = true;
			return false;
		}

        boost::asio::post(pool_dsuc, [reduct, arg, &atts, new_ext] {ds_unchallenged_r(reduct, arg, atts, new_ext);});
    }
    return true;
}

}