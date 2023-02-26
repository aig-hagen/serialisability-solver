#include "Problems.h"					// Header for all Problem methods

#include <algorithm>                    // std::find
#include <mutex>                        // for checking duplicate thread creating
#include <thread>						// std::thread::hardware_concurrency()

#include <boost/asio/thread_pool.hpp>	// handles threads
#include <boost/asio/post.hpp>			// for submitting tasks to thread pool

using namespace std;

// Variables for preventing duplicate threads
set<set<string>> checked_branches_eeuc;
mutex mtx_eeuc;

// Lock for synchronizing output of extensions
mutex stdout_lock_eeuc;

// Initialize thread pool
auto num_max_threads_eeuc = std::thread::hardware_concurrency()-1;
boost::asio::thread_pool pool_eeuc(num_max_threads_eeuc);


namespace Problems {

/*
Helper function for recursive (threaded) construction of unchallenged extensions

@param af		the argumentation framework
@param atts		list of all attacks of the AF (only for faster reduct construction)
@param base_ext	the current status of the extension that is constructed by this thread

@returns 'true'; also prints the extension, if the termination criterion is met
*/
bool ee_unchallenged_r(const AF & af, std::vector<std::pair<std::string,std::string>> & atts, vector<string> base_ext) {
    set<vector<string>> ua_uc_initial_sets = get_ua_or_uc_initial(af);

    if (ua_uc_initial_sets.empty()) {
        stdout_lock_eeuc.lock();
        print_extension_ee(base_ext);
        stdout_lock_eeuc.unlock();
        return true;
    }
    
    for (const auto& ext : ua_uc_initial_sets) {
        vector<string> new_ext;
        set<string> branch;
        for(auto const& arg: base_ext) {
            new_ext.push_back(arg);
            branch.insert(arg);
        }
        for(auto const& arg: ext) {
            new_ext.push_back(arg);
            branch.insert(arg);
        }
        
        mtx_eeuc.lock();
        bool already_checked = checked_branches_eeuc.find(branch) != checked_branches_eeuc.end();
        mtx_eeuc.unlock();
        if (!already_checked) {
            mtx_eeuc.lock();
            checked_branches_eeuc.insert(branch);
            mtx_eeuc.unlock();
            const AF reduct = getReduct(af, ext, atts);
            boost::asio::post(pool_eeuc, [reduct, &atts, new_ext] { ee_unchallenged_r(reduct, atts, new_ext); });
        }
        
    }
    return true;
}

/*
Main function for the EE-UC problem, starts the iterative construction with the empty set

@param af		the argumentation framework
@param atts		list of all attacks of the AF (only for faster reduct construction)

@returns 'true' once all extensions have been constructed
*/
bool ee_unchallenged(const AF & af, vector<pair<string,string>> & atts) {
    std::cout << "[";
    
    vector<string> ext;
    boost::asio::post(pool_eeuc, [af, &atts, ext] { ee_unchallenged_r(af, atts, ext); });
    
    pool_eeuc.join();
    std::cout << "]\n";
    return true;
}

}