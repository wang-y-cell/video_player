#pragma once

#include<thread>
#include<mutex>
#include<condition_variable>
#include<vector>
#include<queue>
#include<functional>


class thread_pool {
public:
	thread_pool(int numThreads) : stop(false) {
		for (int i = 0; i < numThreads; ++i) {
			//emplace_back的值是一个lambda,作为构造一个线程的参数
			threads.emplace_back([this] {
				while (1) {
					std::unique_lock<std::mutex> lock(mtx);
					//等待任务,如果队列为空
					condition.wait(lock, [this] {
						return !tasks.empty() || stop;
						//如果任务队列不为空了,或者任务结束就不等待
						});
					if (stop && tasks.empty())return; //如果是任务结束,就return
					//如果任务结束但是还有任务,需要将剩余任务完成再结束

					//否则取出任务
					std::function<void()> task(std::move(tasks.front()));
					tasks.pop();
					lock.unlock(); //手动解除提高效率
					task();
				}
			});
		}
	}

	~thread_pool() {
		{
			std::unique_lock<std::mutex> lock(mtx);		//stop是共享变量,需要加锁
			stop = true;   
		}
		//调用所有线程将剩余任务完成
		condition.notify_all();
		for (auto& x : threads) {
			x.join();
		}
	}

	template<typename F,typename... Args> //F是函数类型,Args是函数的参数类型
	void add_task(F&& f, Args&&... args) {
		std::function<void()> task =
			std::bind(std::forward<F>(f), std::forward<Args>(args)...);	
		{
			std::unique_lock<std::mutex> lock(mtx);
			tasks.emplace(std::move(task));
		}
		condition.notify_one();
	}

private:
	std::vector<std::thread> threads;
	std::queue<std::function<void()>> tasks;
	
	std::mutex mtx;
	std::condition_variable condition;

	bool stop;
};
