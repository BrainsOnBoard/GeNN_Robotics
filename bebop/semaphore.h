#pragma once
#include <condition_variable>
#include <mutex>

namespace Parrot {
using namespace std;

/*
 * A C++11 semaphore implementation (after:
 * https://stackoverflow.com/questions/4792449/c0x-has-no-semaphores-how-to-synchronize-threads)
 *
 * The ARSDK has its own C implemetation but this is cleaner.
 */
class Semaphore
{
public:
    /*
     * Stop wait() blocking on another thread if it's running.
     */
    void notify()
    {
        unique_lock<mutex> lock(mtx);
        fired = true;
        cv.notify_one();
    }

    /*
     * Wait for notify() to be invoked by another thread.
     */
    void wait()
    {
        unique_lock<mutex> lock(mtx);
        while (!fired) {
            cv.wait(lock);
        }
        fired = false;
    }

private:
    mutex mtx;
    condition_variable cv;
    bool fired = false;
};
}
