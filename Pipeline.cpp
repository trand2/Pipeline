// Project:  Pipeline
// Name: Tyler Randolph
// Submitted: 12/03/2019
// I declare that the following source code was written by me, or provided
// by the instructor for this project. I understand that copying source
// code from any other source, providing source code to another student,
// or leaving my code on a public web site constitutes cheating.
// I acknowledge that  If I am found in violation of this policy this may result
// in a zero grade, a permanent record on file and possibly immediate failure of the class.

/*
 Reflection (1-2 paragraphs):  I really enjoyed this program mainly because most of the code was given. But it really was a good program and I feel I understand threads and locking better now. I did struggle on understanding what the assignment was trying to have us do with the numbers and the output, but once I figured that out, it wasn't too bad. 
 */

// Note: Uses an atomic to decrement nprod.
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
using namespace std;

class Pipeline {
    static queue<int> q, r;
    static condition_variable q_cond, r_cond;
    static mutex q_sync, print, r_sync;
    static atomic_size_t nprod, nfilts;
public:
    static const size_t nprods = 4, nfilt = 3, nout = 10;

    static void produce(int i) {
        // Generate 10000 random ints
        srand(time(nullptr)+i*(i+1));
        for (int i = 0; i < 1000; ++i) {
            int n = rand();     // Get random int

            // Get lock for queue; push int
            unique_lock<mutex> slck(q_sync);
            q.push(n);
            slck.unlock();
            q_cond.notify_one();

        }

        // Notify consumers that a producer has shut down
        --nprod;
        q_cond.notify_all();
    }
    
    static void filter() {
        for (;;) {
            // Get lock for sync mutex
            unique_lock<mutex> qlck(q_sync);

            // Wait for queue to have something to process
            q_cond.wait(qlck,[](){return !q.empty() || !nprod;});

            // if queue is empty or we are out of producers,
            // we then break out of the loop, terminating this thread
            if (q.empty()) {
                assert(!nprod);
                break;
            }
            
            //otherwise,  process next item in queue
            auto x = q.front();
            q.pop();
            qlck.unlock();
            q_cond.notify_one();
            
            unique_lock<mutex> rlck(r_sync);
            r.push(x);
            rlck.unlock();
            r_cond.notify_one();

        }
        
        nfilts--;
        r_cond.notify_all();
    }
    
    static void output(size_t id, ofstream* output) {
        int count = 0;
       
        for (;;) {
            unique_lock<mutex> rlck(r_sync);
            r_cond.wait(rlck, []() { return !r.empty() || !nfilts; });
            
            if (r.empty() && !nfilts) { break; }
            
            if (r.front() % 10 == id) {
                *output << r.front() << '\n';
                r.pop(); count++;
            }
            
            rlck.unlock();
            
            r_cond.notify_one();
        }
        
        lock_guard<mutex> plck(print);
        cout << "Group " << id << " has " << count << " numbers" << endl;
        output->close();
    }
};

queue<int> Pipeline::q, Pipeline::r;
condition_variable Pipeline::q_cond, Pipeline::r_cond;
mutex Pipeline::q_sync, Pipeline::print, Pipeline::r_sync;
atomic_size_t Pipeline::nprod(nprods);
atomic_size_t Pipeline::nfilts(Pipeline::nfilt);

int main() {
    vector<thread> prods, filts, outs;
    for (size_t i = 0; i < Pipeline::nfilt; ++i)
        filts.push_back(thread(&Pipeline::filter));
    for (size_t i = 0; i < Pipeline::nprods; ++i)
        prods.push_back(thread(&Pipeline::produce,i));
    for (size_t i = 0; i < Pipeline::nout; ++i) {
        ofstream* file = new ofstream("/Users/tylerrandolph/Desktop/CS3370/Pipeline/Pipeline/Pipeline/group-" + to_string(i) + ".txt", ios::trunc);
        outs.push_back(thread(Pipeline::output, i, file));
    }

    // Join all threads
    for (auto &p: prods)
        p.join();
    for (auto &f: filts)
        f.join();
    for (auto &o: outs)
        o.join();
}
