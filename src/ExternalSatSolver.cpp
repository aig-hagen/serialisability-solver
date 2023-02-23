#include "ExternalSatSolver.h"
#include <iostream>

using namespace std;

/*
 * The following is adapted from the fudge argumentation-solver
 * and is subject to the GPL3 licence.
*/
ExternalSatSolver::ExternalSatSolver(int num_of_vars, string path) {
    num_vars = num_of_vars;
    num_clauses = 0;
    num_minimization_clauses = 0;
    solver_path = path;
    
    last_clause_closed = true;
    clauses = std::vector<std::vector<int>>();
    minimization_clauses = std::vector<std::vector<int>>();
    model = std::vector<bool>(num_vars+1);
}

void ExternalSatSolver::assume(int lit) {
    assumptions.push_back(lit);
}

void ExternalSatSolver::addClause(std::vector<int> & clause) {
    if(!last_clause_closed){
        // this should not happen
        printf("Previous clause not closed.");
        exit(1);
    }
    clauses.push_back(clause);
    num_clauses++;
    clauses[num_clauses-1].push_back(0);
}

void ExternalSatSolver::addMinimizationClause(std::vector<int> & clause) {
    if(!last_clause_closed){
        // this should not happen
        printf("Previous clause not closed.");
        exit(1);
    }
    minimization_clauses.push_back(clause);
    num_minimization_clauses++;
    minimization_clauses[num_minimization_clauses-1].push_back(0);
}

int ExternalSatSolver::solve() {
    //redi::pstream proc(solver_path, redi::pstreams::pstdout | redi::pstreams::pstdin | redi::pstreams::pstderr);
    redi::pstream process(solver_path + " --polar false");
    process << "p cnf " << num_vars << " " << (num_clauses+assumptions.size()+num_minimization_clauses) << "\n";
    //std::cout << "p cnf " << num_vars << " " << (num_clauses+assumptions.size()+num_minimization_clauses) << "\n";
    for(auto const& clause: clauses) {
        for(const int lit: clause){
            process << lit << " ";
            //std::cout << lit << " ";
        }
        process << "\n";
        //std::cout << "\n";
    }
    for(auto const& clause: minimization_clauses) {
        for(const int lit: clause){
            process << lit << " ";
            //std::cout << lit << " ";
        }
        process << "\n";
        //std::cout << "\n";
    }
    if (!assumptions.empty()) {
        for(const int assumption: assumptions){
            process << assumption << " 0\n";
            //std::cout << assumption << " 0\n";
        }
    }
    
    //std::cout << "-----------------------------------------------" << std::endl;
    assumptions.clear();
    minimization_clauses.clear();
    num_minimization_clauses = 0;
    process << redi::peof;
    std::string line;
    model.clear();
    while (std::getline(process.out(),line)) {
        //std::cout << line << std::endl;
        if(line.rfind("c ", 0) == 0)
        continue;
        if(line.rfind("s ", 0) == 0){
            if(line.rfind("UNSATISFIABLE") != std::string::npos){
                return 20;
            }
        }
        if(line.rfind("v ", 0) == 0){
            line.erase(0, 2);
            size_t pos = 0;
            while(line.length() > 0) {
            pos = line.find(" ");
            if(pos == std::string::npos)
                pos = line.length();
            int var = stoi(line.substr(0, pos));
            if(var > 0)
                model[var] = true;
            else if(var < 0)
                model[-var] = false;
            else break;
            line.erase(0, pos + 1);
            }
        }
    }
    return 10;
}

int ExternalSatSolver::solve(std::vector<int> assumptions) {
    for(auto const& assumption: assumptions) {
        assume(assumption);
    }
    return solve();
}

void ExternalSatSolver::free() {
    clauses.clear();
    minimization_clauses.clear();
    assumptions.clear();
    model.clear();
}