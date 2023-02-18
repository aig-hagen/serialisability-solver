#include "Encodings.h"
#include "Problems.h"
#include <iostream>
#include <stack>

using namespace std;

namespace Problems {

vector<string> se_grounded(const AF & af) {
	vector<uint32_t> num_attackers;
	num_attackers.resize(af.args, 0);
	vector<string> grounded;
	vector<bool> grounded_out;
	grounded_out.resize(af.args, false);
	stack<uint32_t> arg_stack;
	for (size_t i = 0; i < af.args; i++) {
		if (af.unattacked[i]) {
			grounded.push_back(af.int_to_arg[i]);
			arg_stack.push(i);
		}
		num_attackers[i] = af.attackers[i].size();
	}
	while (arg_stack.size() > 0) {
		uint32_t arg = arg_stack.top();
		arg_stack.pop();
		for (auto const& arg1: af.attacked[arg]) {
			if (grounded_out[arg1]) {
				continue;
			}
			grounded_out[arg1] = true;
			for (auto const& arg2: af.attacked[arg1]) {
				if (num_attackers[arg2] > 0) {
					num_attackers[arg2]--;
					if (num_attackers[arg2] == 0) {
						grounded.push_back(af.int_to_arg[arg2]);
						arg_stack.push(arg2);
					}
				}
			}
		}
	}
	return grounded;
}

}