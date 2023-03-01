#include "Problems.h"					// Header for all Problem methods

#include <algorithm>                    // std::find

#if defined(PARALLEL)
#include <mutex>                        // for checking duplicate thread creating
#include <thread>						// std::thread::hardware_concurrency()
#include <boost/asio/thread_pool.hpp>	// handles threads
#include <boost/asio/post.hpp>			// for submitting tasks to thread pool
#endif

using namespace std;

// Variables for preventing duplicate threads
set<set<string>> checked_branches_eeuc;

#if defined(PARALLEL)
mutex mtx_eeuc;
// Lock for synchronizing output of extensions
mutex stdout_lock_eeuc;

// Initialize thread pool
auto num_max_threads_eeuc = std::thread::hardware_concurrency()-1;
boost::asio::thread_pool pool_eeuc(num_max_threads_eeuc);
#endif

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
        #if defined(PARALLEL)
        print_extension_ee_parallel(base_ext);
        #else
        print_extension_ee(base_ext);
        #endif
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
        
        #if defined(PARALLEL)
        mtx_eeuc.lock();
        bool already_checked = checked_branches_eeuc.find(branch) != checked_branches_eeuc.end();
        mtx_eeuc.unlock();
        #else
        bool already_checked = checked_branches_eeuc.find(branch) != checked_branches_eeuc.end();
        #endif
        if (!already_checked) {
            #if defined(PARALLEL)
            mtx_eeuc.lock();
            checked_branches_eeuc.insert(branch);
            mtx_eeuc.unlock();
            #else
            checked_branches_eeuc.insert(branch);
            #endif

            const AF reduct = getReduct(af, ext, atts);
            #if defined(PARALLEL)
            boost::asio::post(pool_eeuc, [reduct, &atts, new_ext] { ee_unchallenged_r(reduct, atts, new_ext); });
            #else
            ee_unchallenged_r(reduct, atts, new_ext);
            #endif
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
    #if defined(PARALLEL)
    boost::asio::post(pool_eeuc, [af, &atts, ext] { ee_unchallenged_r(af, atts, ext); });
    pool_eeuc.join();
    #else
    ee_unchallenged_r(af, atts, ext);
    #endif
    
    std::cout << "]\n";
    return true;
}

}