#ifndef UTIL_H
#define UTIL_H

#include "AF.h"

#include <iostream>
#include <set>

void print_extension(const AF & af, const std::vector<uint32_t> & extension);
void print_extension_ee(const AF & af, const std::vector<uint32_t> & extension);
void print_extension_ee(const std::vector<std::string> & extension);
void print_extension_ee(const std::set<std::string> & extension);

AF getReduct(const AF & af, std::vector<std::string> ext, std::vector<std::pair<std::string,std::string>> & atts);

std::vector<std::vector<uint32_t>> computeStronglyConnectedComponents(const AF & af);
void print_sccs(const AF & af, std::vector<std::vector<uint32_t>> sccs);

void log(int thread_id, int output);
void log(int thread_id, std::string output);
void log(int thread_id, std::string output, std::vector<int> clause);
void log(int thread_id, std::string output, std::vector<std::string> ext);
void log(int thread_id, std::string output, std::vector<uint32_t> ext, const AF & af);

#endif