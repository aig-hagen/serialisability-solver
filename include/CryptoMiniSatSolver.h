/*!
 * The following is largely taken from the mu-toksia solver
 * and is subject to the following licence.
 * 
 * 
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

#ifndef CMSAT_SOLVER_H
#define CMSAT_SOLVER_H

#include <vector>
#include <string>

#include <cryptominisat5/cryptominisat.h>

/*
Class that models the Crpytominisat5 SAT solver
SAT calls are answered by directly asking the API of cryptominisat
*/
class CryptoMiniSatSolver {

public:
	std::vector<bool> model;
	uint32_t n_vars; // number of vars
	std::vector<std::vector<CMSat::Lit>> clauses;
    std::vector<std::vector<CMSat::Lit>> minimization_clauses;

	CryptoMiniSatSolver(uint32_t number_of_vars, std::string path_to_solver);
	CryptoMiniSatSolver(const CryptoMiniSatSolver& obj);
	~CryptoMiniSatSolver() {};
	void addClause(const std::vector<int> & clause);
	void addMinimizationClause(const std::vector<int> & clause);
	int solve();
	int solve(const std::vector<int> & assumptions);
	void free();
};

#endif