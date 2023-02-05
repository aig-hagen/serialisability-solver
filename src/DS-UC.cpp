#include "EnumExtensions.h"
#include "Problems.h"
#include <algorithm>
#include <iostream>
#include <atomic>
#include <thread>

using namespace std;

namespace Problems {

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
}