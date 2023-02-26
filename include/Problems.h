#ifndef PROBLEMS_H
#define PROBLEMS_H

#include "Util.h"
#include "Encodings.h"

namespace Problems {

// INITIAL

 //DC-IT
//bool dc_initial(const AF & af, std::string const & arg);

// SE-IT
bool se_initial(const AF & af);

// EE-IT
bool ee_initial(const AF & af);
std::set<std::vector<std::string>> get_ua_or_uc_initial(const AF & af); // helper method to get only the unattacked and unchallenged initial sets

// CE-IT
bool ce_initial(const AF & af); // counts the types of initial sets and their sizes


// UNCHALLENGED

// DS-UC
bool ds_unchallenged(const AF & af, std::string const & arg, std::vector<std::pair<std::string,std::string>> & atts);

// EE-UC
bool ee_unchallenged(const AF & af, std::vector<std::pair<std::string,std::string>> & atts);


// PREFERRED

// DS-PR
bool mt_ds_preferred(const AF & af, std::string const & arg);
bool ds_preferred(const AF & af, std::string const & arg, std::vector<std::pair<std::string,std::string>> & atts);
bool ds_preferred_r(const AF & af, std::string const & arg, std::vector<std::pair<std::string,std::string>> & atts, std::vector<std::string> base_ext);
bool ds_preferred_r_scc(const AF & af, std::string const & arg, std::vector<std::pair<std::string,std::string>> & atts, std::vector<std::string> base_ext, std::vector<uint32_t> scc);

//GROUNDED

// SE-GR
std::vector<std::string> se_grounded(const AF & af);

}
#endif