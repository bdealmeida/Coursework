#ifndef BoundedBuffer_h
#define BoundedBuffer_h

#include <stdio.h>
#include <queue>
#include <string>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include <assert.h>
#include <memory.h>

using namespace std;

class BoundedBuffer {
private:
	int cap; // max number of items in the buffer
	queue<vector<char>> q;	/* the queue of items in the buffer. Note
	that each item a sequence of characters that is best represented by a vector<char> for 2 reasons:
	1. An STL std::string cannot keep binary/non-printables
	2. The other alternative is keeping a char* for the sequence and an integer length (i.e., the items can be of variable length).
	While this would work, it is clearly more tedious */

	// add necessary synchronization variables and data structures 
    mutex mtx;
    condition_variable data_ready;
    condition_variable slot_open;

public:
	BoundedBuffer(int _cap): cap(_cap) {}
	~BoundedBuffer() {}

	void push(char* data, int len) {
        //1. Convert the incoming byte sequence given by data and len into a vector<char>
        vector<char> vec_data(data, data + len);

		//2. Wait until there is room in the queue (i.e., queue lengh is less than cap)
        unique_lock<mutex> ul(mtx);
		slot_open.wait(ul, [this]{return q.size() < cap;});

		//3. Then push the vector at the end of the queue
		q.push(vec_data);
		ul.unlock();

		//4. Unlock mutex and notify that data is ready
		data_ready.notify_one();
	}

	int pop(char* buf, int bufcap) {
	    //1. Wait until the queue has at least 1 item
        unique_lock<mutex> ul(mtx);
		data_ready.wait(ul, [this]{return q.size() > 0;});

		//2. pop the front item of the queue. The popped item is a vector<char>
		vector<char> data = q.front();
		q.pop();
		ul.unlock();

		//3. Convert the popped vector<char> into a char*, copy that into buf, make sure that vector<char>'s length is <= bufcap
		assert(data.size() <= bufcap);
		memcpy(buf, data.data(), data.size());

		//4. Notify that slot is available and unlock mutex
		slot_open.notify_one();

		//5. Return the vector's length to the caller so that he knows many bytes were popped
		return data.size();
	}
};

#endif /* BoundedBuffer_ */
