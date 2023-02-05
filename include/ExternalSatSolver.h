#ifndef EXTERNAL_SAT_SOLVER_H
#define EXTERNAL_SAT_SOLVER_H

#include <vector>
#include "pstream.h"

class ExternalSatSolver {
public:
    std::vector<std::vector<int>> clauses;
    std::vector<std::vector<int>> minimization_clauses;
    std::vector<int> assumptions;
    int num_vars;
    int num_clauses;
    int num_minimization_clauses;
    bool last_clause_closed;
    std::vector<bool> model;

    ExternalSatSolver(int num_of_vars);
    void assume(int lit);
    void addClause(std::vector<int> & clause);
    void addMinimizationClause(std::vector<int> & clause);
    int solve();
    int solve(std::vector<int> assumptions);
    void free();

};

#endif