#ifndef PROBLEMS_H
#define PROBLEMS_H

#include "AF.h"
#include "Util.h"
#include <set>

namespace Problems {

// DS-UC
bool ds_unchallenged(const AF & af, std::string const & arg, std::vector<std::pair<std::string,std::string>> & atts);
bool ds_unchallenged_r(params p);

// DS-PR
bool ds_preferred(const AF & af, std::string const & arg, std::vector<std::pair<std::string,std::string>> & atts);
bool ds_preferred_r(params p);

// SE-IT
bool se_initial(const AF & af);

// SE-GR
std::vector<std::string> se_grounded(const AF & af);

// EE-IT
bool ee_initial(const AF & af);
std::set<std::vector<std::string>> get_ua_or_uc_initial(const AF & af); // helper method to get only the unattacked and unchallenged initial sets

// EE-UC
bool ee_unchallenged(const AF & af, std::vector<std::pair<std::string,std::string>> & atts);
bool ee_unchallenged_r(const AF & original_af, const AF & af, std::vector<std::pair<std::string,std::string>> & atts, std::vector<std::string> base_ext);

}

#endif