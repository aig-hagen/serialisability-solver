#ifndef PROBLEMS_H
#define PROBLEMS_H

#include "Util.h"
#include "Encodings.h"

#include <atomic>

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

// EE-PR

bool ee_preferred(const AF & af, std::vector<std::pair<std::string,std::string>> & atts);

//GROUNDED

// SE-GR
std::vector<std::string> se_grounded(const AF & af);

}
#endif