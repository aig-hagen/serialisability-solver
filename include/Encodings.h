#ifndef ENCODINGS_H
#define ENCODINGS_H

#include "AF.h"

#if defined(SAT_CMSAT)
#include "CryptoMiniSatSolver.h"
typedef CryptoMiniSatSolver SAT_Solver;
#else
#include "ExternalSatSolver.h"
typedef ExternalSatSolver SAT_Solver;
#endif

namespace Encodings {

void add_rejected_clauses(const AF & af, SAT_Solver & solver);
void add_nonempty(const AF & af, SAT_Solver & solver);
void add_nonempty_subset_of(const AF & af, std::vector<uint32_t> args, SAT_Solver & solver);
void add_conflict_free(const AF & af, SAT_Solver & solver);
void add_admissible(const AF & af, SAT_Solver & solver);
void add_complete(const AF & af, SAT_Solver & solver);

}

#endif