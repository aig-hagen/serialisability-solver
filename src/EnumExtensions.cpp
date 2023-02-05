#include "Encodings.h"
#include "Util.h"
#include "EnumExtensions.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>

using namespace std;

namespace EnumExtensions {

vector<vector<uint32_t>> initial_naive(const AF & af) {
    vector<vector<uint32_t>> extensions;

    int n = af.args;
    int subset_count = pow(2, n);

    for (int i = 0; i < subset_count; i++) {
        std::vector<uint32_t> subset;
        for (int j = 0; j < n; j++) {
            if (i & (1 << j)) {
                subset.push_back(j);
            }
        }

        // non emptiness
        if (subset.empty()) {
            continue;
        }

        // minimality
        std::sort(subset.begin(), subset.end());

        bool minimal = true;
        for (const auto &other_subset : extensions) {
            if (std::includes(subset.begin(), subset.end(), other_subset.begin(), other_subset.end())) {
                minimal = false;
                break;
            }
        }

        if (!minimal) {
            continue;
        }

        //conflict-freeness
        bool cf = true;
        for(auto const& arg1: subset) {
            if (af.self_attack[arg1]) {
                cf = false; 
                break;
            }
            for(auto const& arg2: subset) {
                if (af.att_exists.count(make_pair(arg1,arg2)) > 0) {
                    cf = false;
                    break;
                }
            }
            if (!cf) {
                break;
            }
        }
        if (!cf) {
            continue;
        }

        //admissibility
        bool adm = true;
        for (auto const& arg1: subset) {
            for (auto const& attacker: af.attackers[arg1]) {
                bool defended = false;
                for(auto const& arg2: subset) {
                    if (af.att_exists.count(make_pair(arg2,attacker)) > 0) {
                        defended = true;
                        break;
                    }
                }
                if (!defended) {
                    adm = false;
                    break;
                }
            }
            if (!adm) {
                break;
            }
        }
        if (!adm) {
            continue;
        }

        //print_extension_ee(af, subset);
        extensions.push_back(subset);
    }

    return extensions;
}

std::set<vector<string>> ua_or_uc_initial_naive(const AF & af) {
    vector<vector<uint32_t>> initialSets = initial_naive(af);

    set<vector<string>> result;
    for(auto const& set1: initialSets) {
        bool challenged = false;
        for(auto const& set2: initialSets) {
            if (set1 == set2) {
                continue;
            }
            for (auto arg1: set1) {
                for (auto arg2: set2) {
                    try {
                        if (af.att_exists.count(make_pair(arg1, arg2)) > 0 || af.att_exists.count(make_pair(arg2, arg1)) > 0) {
                            challenged = true;
                            break;
                        }
                    }
                    catch(const std::exception& e) {
                        ;
                    } 
                }
                if (challenged) {
                    break;
                }
            }
            if (challenged) {
                break;
            }
        }
        if (!challenged) {
            vector<string> ext;
            for(auto const& arg: set1) {
                ext.push_back(af.int_to_arg[arg]);
            }
            result.insert(ext);
        } 
    }

    return result;

}

bool unchallenged_naive(const AF & af, vector<pair<string,string>> & atts) {
    cout << "[";
    vector<string> ext;
    unchallenged_naive_r(af, af, atts, ext);
    cout << "]\n";
    return true;
}

bool unchallenged_naive_r(const AF & original_af, const AF & af, vector<pair<string,string>> & atts, vector<string> base_ext) {
    set<vector<string>> candidateSets = ua_or_uc_initial_naive(af);

    if (candidateSets.empty()) {
        print_extension_ee(base_ext);
        return true;
    }

    for (auto const& initialSet: candidateSets) {
        const AF reduct = getReduct(af, initialSet, atts);
        //TODO trim atts after each reduct
        vector<string> new_ext;
        new_ext.insert(new_ext.end(), base_ext.begin(), base_ext.end());
        new_ext.insert(new_ext.end(), initialSet.begin(), initialSet.end());

        unchallenged_naive_r(original_af, reduct, atts, new_ext);
    }
    return true;
}

}