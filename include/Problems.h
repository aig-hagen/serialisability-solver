#ifndef PROBLEMS_H
#define PROBLEMS_H

#include "Util.h"
#include "Encodings.h"

namespace Problems {

std::set<std::vector<std::string>> get_ua_or_uc_initial(const AF & af); // helper method to get only the unattacked and unchallenged initial sets

// EE-UC
bool ee_unchallenged(const AF & af, std::vector<std::pair<std::string,std::string>> & atts);

// SE-GR
std::vector<std::string> se_grounded(const AF & af);

}
#endif