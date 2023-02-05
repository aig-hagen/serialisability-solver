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

#include "Encodings.h"
#include "Util.h"
#include "SingleExtension.h"

using namespace std;

namespace SingleExtension {

bool initial(const AF & af)
{
	ExternalSatSolver solver = ExternalSatSolver(af.count, af.solver_path);
	Encodings::add_nonempty(af, solver);
	Encodings::add_admissible(af, solver);
	bool sat = solver.solve();
	if (sat) {
		vector<uint32_t> extension;
		for (uint32_t i = 0; i < af.args; i++) {
			if (solver.model[af.accepted_var[i]-1]) {
				extension.push_back(i);
			}
		}
		print_extension(af, extension);
		return true;
	}else {
		cout << "NO\n";
		return false;
	}
}

/*
TODO implement properly
vector<string> grounded(const AF & af)
{
	ExternalSat::ExternalSolver solver;
	ExternalSat::sat__init(solver, af.count);
	Encodings::add_complete(af, solver);
	bool sat = solver.propagate();
	vector<string> extension;
	if (sat) {
		for (uint32_t i = 0; i < af.args; i++) {
			if (solver.assignment[af.accepted_var[i]-1]) {
				extension.push_back(af.int_to_arg[i]);
			}
		}
	}
	return extension;
}
*/

}