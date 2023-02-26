#ifndef ENUM_EXTENSIONS_H
#define ENUM_EXTENSIONS_H

#include "AF.h"

#include <set>

namespace EnumExtensions {

std::vector<std::vector<uint32_t>> initial_naive(const AF & af);
std::set<std::vector<std::string>> ua_or_uc_initial_naive(const AF & af);
bool unchallenged_naive(const AF & af, std::vector<std::pair<std::string,std::string>> & atts);
std::set<std::set<std::string>> unchallenged_naive_r(const AF & original_af, const AF & af, std::vector<std::pair<std::string,std::string>> & atts, std::set<std::string> base_ext);

}

#endif