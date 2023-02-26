#ifndef EXTERNAL_SAT_SOLVER_H
#define EXTERNAL_SAT_SOLVER_H

#include "pstream.h"
#include "SatSolver.h"

/*
Class for all kinds of pre-compiled SAT solvers, e.g. cadical, cryptominisat5
SAT calls are answered by opening a pipe to an instance of the external solver with pstream
TODO some bug in pstream
*/
class ExternalSatSolver : public SatSolver {
public:
    std::vector<std::vector<int>> clauses;
    std::vector<std::vector<int>> minimization_clauses;
    std::vector<int> assumptions;
    int num_vars;
    int num_clauses;
    int num_minimization_clauses;
    bool last_clause_closed;
    std::string solver_path;

    ExternalSatSolver(int num_of_vars, std::string path);
    void assume(int lit);
    void addClause(std::vector<int> & clause);
    void addMinimizationClause(std::vector<int> & clause);
    int solve();
    int solve(const std::vector<int> assumptions);
    void free();
    
};

#endif