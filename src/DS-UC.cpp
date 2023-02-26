#include "Problems.h"
#include <algorithm>
#include <atomic>

using namespace std;

namespace Problems {

std::atomic<bool> unchallenged_ce_found{false};

bool ds_unchallenged(const AF & af, string const & arg, std::vector<std::pair<std::string,std::string>> & atts) {
    vector<string> ext;
	//TODO Thread Logic and some kind of flag to kill all threads once counterexample is found
    ds_unchallenged_r(af, arg, atts, ext);

    return true;
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
		// should never be reached
        return false;
    }
    
    for (const auto& ext : ua_uc_initial_sets) {
		if (unchallenged_ce_found) {
		return true;
	}
		// Break Conditions
		if (std::find(ext.begin(), ext.end(), arg) != ext.end()) {
			return true;
		}
        const AF reduct = getReduct(af, ext, atts);
		if (reduct.arg_to_int.find(arg) == reduct.arg_to_int.end()) {
			cout << "NO\n";
			unchallenged_ce_found = true;
			return false;
		}
        //TODO trim atts after each reduct
        vector<string> new_ext;
        new_ext.insert(new_ext.end(), base_ext.begin(), base_ext.end());
        new_ext.insert(new_ext.end(), ext.begin(), ext.end());

        ds_unchallenged_r(reduct, arg, atts, new_ext);
    }
    return true;
}

}