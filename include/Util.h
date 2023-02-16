#ifndef UTIL_H
#define UTIL_H

#include "AF.h"
#include <vector>
#include <string>

struct params {
	const AF af;
	const int & arg;
	std::vector<std::vector<int>> encoding;
	std::vector<int> assumptions;
};

//std::vector<int> grounded_assumptions(const AF & af);
void print_extension(const AF & af, const std::vector<uint32_t> & extension);
void print_extension_ee(const AF & af, const std::vector<uint32_t> & extension);
void print_extension_ee(const std::vector<std::string> & extension);
AF getReduct(const AF & af, std::vector<std::string> ext, std::vector<std::pair<std::string,std::string>> & atts);

#endif