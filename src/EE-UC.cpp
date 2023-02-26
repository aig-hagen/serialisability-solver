#include "Problems.h"
#include <iostream>
#include <fstream>

using namespace std;

namespace Problems {

bool ee_unchallenged(const AF & af, vector<pair<string,string>> & atts) {
    set<string> ext;
    set<set<string>> exts = ee_unchallenged_r(af, af, atts, ext);
    std::cout << "[";
    int ct = 0;
    for(auto const& ext: exts) {
        print_extension_ee(ext);
        if (ct != exts.size()-1) cout << ",";
        ct++;
    }
    std::cout << "]\n";
    return true;
}

set<set<string>> ee_unchallenged_r(const AF & original_af, const AF & af, std::vector<std::pair<std::string,std::string>> & atts, set<string> base_ext) {
    set<vector<string>> ua_uc_initial_sets = get_ua_or_uc_initial(af);
    set<set<string>> exts;

    if (ua_uc_initial_sets.empty()) {
        exts.insert(base_ext);
        return exts;
    }
    
    for (const auto& ext : ua_uc_initial_sets) {
        const AF reduct = getReduct(af, ext, atts);
        //TODO trim atts after each reduct
        set<string> new_ext;
        for(auto const& arg: base_ext) {
            new_ext.insert(arg);
        }
        for(auto const& arg: ext) {
            new_ext.insert(arg);
        }
        set<set<string>> new_exts = ee_unchallenged_r(original_af, reduct, atts, new_ext);
        for(auto const& ext: new_exts) {
            exts.insert(ext);
        }
    }
    return exts;
}

}