#include "sock_threads.hpp"
//#include <iostream>

SockThreads::SockThreads(TFunc fnc, std::vector<int> &cl_vect, int threads_cnt) :
	run(true),
	thr_v(threads_cnt),
	wrk_fnc(fnc),
	wrc_cntr(0),
	closed(cl_vect)
{
	auto ThreadLoop = [&](int i)
	{
		while (run) 
		{
			std::unique_lock<std::mutex> ul(m_cond);
			cv.wait(ul, [this](){return !this->sock_queue.empty();});
			++wrc_cntr;
			m_queue.lock();
			int sock = sock_queue.front();
			sock_queue.pop();
			m_queue.unlock();
			
			if (wrk_fnc(sock) == -1)
				closed.push_back(sock);

			--wrc_cntr;
			cv2.notify_one();
		}
	};	
		
	for (int i = 0; i < threads_cnt; i++) 
		thr_v.push_back(std::thread(ThreadLoop, i));
}

SockThreads::~SockThreads()
{
	run = false;
	for (std::thread& it : thr_v)
		it.join();
}


void SockThreads::AddSock(int sock)
{
	m_queue.lock();	
	sock_queue.push(sock);
	m_queue.unlock();
	cv.notify_one();
}


int SockThreads::WaitWrk()
{
	std::unique_lock<std::mutex> ul(m_cond2);
	cv2.wait(ul, [this](){return this->sock_queue.empty()&&(wrc_cntr == 0);});
	return 0;
}
