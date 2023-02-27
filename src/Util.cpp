#include "Encodings.h"
#include "Util.h"

#include <unordered_set>
#include <stack>
#include <algorithm>
#include <fstream>
#include <mutex>

using namespace std;

void print_extension(const AF & af, const std::vector<uint32_t> & extension) {
	cout << "[";
	for (uint32_t i = 0; i < extension.size(); i++) {
		cout << af.int_to_arg[extension[i]];
		if (i != extension.size()-1) cout << ",";
	}
	cout << "]\n";
}

void print_extension_ee(const AF & af, const std::vector<uint32_t> & extension) {
	std::cout << "[";
	for (uint32_t i = 0; i < extension.size(); i++) {
		std::cout << af.int_to_arg[extension[i]];
		if (i != extension.size()-1) cout << ",";
	}
	std::cout << "]";
}

void print_extension_ee(const std::vector<string> & extension) {
	std::cout << "[";
	for (uint32_t i = 0; i < extension.size(); i++) {
		std::cout << extension[i];
		if (i != extension.size()-1) cout << ",";
	}
	std::cout << "]";
}

mutex mtx_stdout;

void print_extension_ee_parallel(const std::vector<string> & extension) {
	mtx_stdout.lock();
	std::cout << "[";
	for (uint32_t i = 0; i < extension.size(); i++) {
		std::cout << extension[i];
		if (i != extension.size()-1) cout << ",";
	}
	std::cout << "],";
	mtx_stdout.unlock();
}

void print_extension_ee(const std::set<string> & extension) {
	std::cout << "[";
	int ct = 0;
	for (auto const& arg: extension) {
		std::cout << arg;
		if (ct != extension.size()-1) cout << ",";
		ct++;
	}
	std::cout << "]";
}

AF getReduct(const AF & af, vector<string> ext, vector<pair<string,string>> & atts) {
	if (ext.empty()) {
		return af;
	}
	AF reduct = AF();

	std::unordered_set<uint32_t> removed_args;
	for (const auto& arg: ext) {
		uint32_t arg_id = af.arg_to_int.find(arg)->second;
		removed_args.insert(arg_id);
		removed_args.insert(af.attacked[arg_id].begin(), af.attacked[arg_id].end());
	}
	for (uint32_t i = 0; i < af.args; i++) {
		string arg_str = af.int_to_arg[i];
		if (!removed_args.count(i)) {
			reduct.add_argument(arg_str);
		}
	}

	reduct.set_solver_path(af.solver_path);

	if (!reduct.args) {
		return reduct;
	}
	
	reduct.initialize_attackers();
	reduct.initialize_vars();

	for (const auto& att : atts) {
		uint32_t source;
		uint32_t target;
		auto it1 = af.arg_to_int.find(att.first);
		if (it1 != af.arg_to_int.end()) {
			source = it1->second;
		} else {
			continue;
		}
		auto it2 = af.arg_to_int.find(att.second);
		if (it2 != af.arg_to_int.end()) {
			target = it2->second;
		} else {
			continue;
		}
		if (removed_args.count(source) == 0 && removed_args.count(target) == 0) {
			reduct.add_attack(att);
		}
	}
	return reduct;
}

/*
 * The following functions for working with SCCs have been adapted from the fudge argumentation-solver
 * which is subject to the GPL3 licence. 
*/
int __scc__compute_strongly_connected_components(int idx, uint32_t v, stack<uint32_t>* arg_stack, vector<vector<uint32_t>>* sccs, const AF & af, int index[], int lowlink[], bool stack_member[]) {
    index[v] = idx;
	lowlink[v] = idx;
	idx++;
    arg_stack->push(v);
	stack_member[v] = true;
    for(auto const& w: af.attacked[v]){
        if (index[w] == -1) {
	        idx = __scc__compute_strongly_connected_components(idx, w, arg_stack, sccs, af, index, lowlink, stack_member);
    	    lowlink[v] = lowlink[v] > lowlink[w] ? lowlink[w] : lowlink[v];
	    } else if (stack_member[w]) {
            lowlink[v] = lowlink[v] > index[w] ? index[w] : lowlink[v];
        }
	}
    if(lowlink[v] == index[v]){
        vector<uint32_t> scc;
        
        uint32_t w;
        do {
            w = arg_stack->top();
            arg_stack->pop();
			stack_member[w] = false;
            scc.push_back(w);
        } while ( v != w);
        sccs->push_back(scc);
	}
	return idx;
}

// computes the set of strongly connected components using Tarjan's algorithm
vector<vector<uint32_t>> computeStronglyConnectedComponents(const AF & af) {
    vector<vector<uint32_t>>* sccs = new vector<vector<uint32_t>>();
    int idx = 0;
    stack<uint32_t>* arg_stack = new stack<uint32_t>();

    int* index = new int[af.args];
    int* lowlink = new int[af.args];
	bool* stack_member = new bool[af.args];
    for (size_t i = 0; i < af.args; i++) {
		index[i] = -1;
		lowlink[i] = -1;
		stack_member[i] = false;
	}
	
    for(uint32_t i = 0; i < af.args; i++) {
        if(index[i] == -1) {
            idx = __scc__compute_strongly_connected_components(idx,i,arg_stack,sccs,af,index,lowlink, stack_member);
        }
    }
    return *sccs;
}

// print the set of strongly connected components
void print_sccs(const AF & af, vector<vector<uint32_t>> sccs) {
    for(auto const& scc: sccs) {
        cout << "<";
        char isFirst = true;
        for(auto const& arg: scc) {
            if (isFirst) {
                isFirst = false;
                cout << af.int_to_arg[arg];
            } else {
				cout << "," << af.int_to_arg[arg];
			}
        }
        cout << ">\n";
    }
}

mutex mtx_log;

void log(int thread_id, string output) {
	mtx_log.lock();
	std::ofstream outfile;
	outfile.open("out.log", std::ios_base::app);
	outfile << thread_id << ": " << output <<"\n";
	outfile.close();
	mtx_log.unlock();
}

void log(int thread_id, int output) {
	mtx_log.lock();
	std::ofstream outfile;
	outfile.open("out.log", std::ios_base::app);
	outfile << thread_id << ": " << output <<"\n";
	outfile.close();
	mtx_log.unlock();
}

void log(int thread_id, std::string output, vector<int> clause) {
	mtx_log.lock();
	std::ofstream outfile;
	outfile.open("out.log", std::ios_base::app);
	outfile << thread_id << ": " << output << ": ";
	for(auto const& arg: clause) {
		outfile << arg << ",";
	}
	outfile << "\n";
	outfile.close();
	mtx_log.unlock();
}

void log(int thread_id, std::string output, vector<string> ext) {
	mtx_log.lock();
	std::ofstream outfile;
	outfile.open("out.log", std::ios_base::app);
	outfile << thread_id << ": " << output << ": ";
	for(auto const& arg: ext) {
		outfile << arg << ",";
	}
	outfile << "\n";
	outfile.close();
	mtx_log.unlock();
}

void log(int thread_id, std::string output, vector<uint32_t> ext, const AF & af) {
	mtx_log.lock();
	std::ofstream outfile;
	outfile.open("out.log", std::ios_base::app);
	outfile << thread_id << ": " << output << ": ";
	for(auto const& arg: ext) {
		outfile << af.int_to_arg[arg] << ",";
	}
	outfile << "\n";
	outfile.close();
	mtx_log.unlock();
}