#include <boost/thread/mutex.hpp>
#include <queue>
#include <list>

template<typename _T> class locked_queue
{
private:
    boost::mutex mutex;
    std::queue<_T> queue;
public:
    void push(_T value) 
    {
        boost::mutex::scoped_lock lock(mutex);
        queue.push(value);
    };

    _T pop()
    {
        boost::mutex::scoped_lock lock(mutex);
        _T value;
        std::swap(value,queue.front());
        queue.pop();
        return value;
    };

    bool empty() {
        boost::mutex::scoped_lock lock(mutex);
        return queue.empty();
    }
};