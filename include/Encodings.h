#ifndef ENCODINGS_H
#define ENCODINGS_H

#include "AF.h"
#include "ExternalSatSolver.h"

namespace Encodings {

void add_nonempty(const AF & af, ExternalSatSolver & solver);
void add_nonempty_subset_of(const AF & af, std::vector<uint32_t> args, ExternalSatSolver & solver);
void add_conflict_free(const AF & af, ExternalSatSolver & solver);
void add_admissible(const AF & af, ExternalSatSolver & solver);
void add_complete(const AF & af, ExternalSatSolver & solver);

}

#endif