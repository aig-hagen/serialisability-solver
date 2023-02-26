/*!
 * Copyright (c) <2020> <Andreas Niskanen, University of Helsinki>
 * 
 * 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * 
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "CryptoMiniSatSolver.h"
#include "Util.h"

using namespace std;
using namespace CMSat;

CryptoMiniSatSolver::CryptoMiniSatSolver(uint32_t number_of_vars, uint32_t number_of_arg_vars) {
	n_args = number_of_arg_vars;
	n_vars = number_of_vars+1;
	model = vector<bool>(n_vars+1);
	clauses = std::vector<std::vector<Lit>>();
    minimization_clauses = std::vector<std::vector<Lit>>();
}

void CryptoMiniSatSolver::addClause(const vector<int> & clause) {
	vector<Lit> lits(clause.size());
	for (int i = 0; i < clause.size(); i++) {
		int var = abs(clause[i]);
		lits[i] = Lit(var, (clause[i] > 0) ? false : true);
	}
	clauses.push_back(lits);
}

void CryptoMiniSatSolver::addMinimizationClause(const vector<int> & clause) {
	vector<Lit> lits(clause.size());
	for (int i = 0; i < clause.size(); i++) {
		int var = abs(clause[i]);
		lits[i] = Lit(var, (clause[i] > 0) ? false : true);
	}
	minimization_clauses.push_back(lits);
}

int CryptoMiniSatSolver::solve(int thread_id) {
	//log(thread_id, "INITIALIZING SOLVER");
	//log(thread_id, ((int)n_vars));
	CMSat::SATSolver solver;
	solver.set_num_threads(1);
	solver.new_vars(n_vars);
	//log(thread_id, "ADDING CLAUSES");
	for(auto const& clause: clauses) {
		solver.add_clause(clause);
	}
	//log(thread_id, "ADDING MIN CLAUSES");
	for(auto const& clause: minimization_clauses) {
		solver.add_clause(clause);
	}
	minimization_clauses.clear();
	//log(thread_id, "SOLVING");
	bool sat = (solver.solve() == l_True);
	//log(thread_id, "SOLVED");
	model.clear();
	if (sat) {
		//log(thread_id, "PARSING MODEL");
		for (int i = 0; i < n_vars; i++) {
			model[i] = (solver.get_model()[i] == l_True) ? true : false;
		}
		//log(thread_id, "MODEL PARSED");
	}
	//log(thread_id, sat);
	return sat ? 10 : 20;
}