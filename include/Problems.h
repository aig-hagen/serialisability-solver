#ifndef PROBLEMS_H
#define PROBLEMS_H

#include "AF.h"
#include "Util.h"

namespace Problems {

bool unchallenged(const AF & af, std::string const & arg, std::vector<std::pair<std::string,std::string>> & atts);
bool unchallenged_r(params p);
bool preferred_p(const AF & af, std::string const & arg, std::vector<std::pair<std::string,std::string>> & atts);
bool preferred_p_r(params p);

}

#endif