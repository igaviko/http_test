#ifndef _SOCK_THREADS_INCLUDED
#define _SOCK_THREADS_INCLUDED

#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <condition_variable>
#include <functional>
#include <atomic>
	

typedef std::function<int(int)> TFunc;
	
class SockThreads {
	public:
		SockThreads(TFunc fnc,
			std::vector<int> &cl_vect,
			int threads_cnt = std::thread::hardware_concurrency());

		~SockThreads();
		
		void AddSock(int sock);
		int WaitWrk();

	private:
		SockThreads() = delete;
		SockThreads(SockThreads&) = delete;
		SockThreads& operator=(SockThreads&) = delete;
  
		bool run;
		std::atomic_int wrc_cntr;
		std::queue<int> sock_queue;
		
		std::mutex m_queue;
		std::mutex m_cond;
		std::mutex m_cond2;
		
		std::condition_variable cv;
		std::condition_variable cv2;

		std::vector<int> &closed;
		std::vector<std::thread> thr_v;
		
		
		TFunc wrk_fnc;
};


#endif