#include "Problems.h"
#include "Encodings.h"
#include <iostream>

using namespace std;

namespace Problems {

bool ee_unchallenged(const AF & af, vector<pair<string,string>> & atts) {
    vector<string> ext;
    std::cout << "[";
    // TODO don't print directly. check for duplicate first
    ee_unchallenged_r(af, af, atts, ext);
    std::cout << "]\n";
    return true;
}

bool ee_unchallenged_r(const AF & original_af, const AF & af, vector<pair<string,string>> & atts, vector<string> base_ext) {
    set<vector<string>> ua_uc_initial_sets = get_ua_or_uc_initial(af);

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

        ee_unchallenged_r(original_af, reduct, atts, new_ext);
    }
    return true;
}

}