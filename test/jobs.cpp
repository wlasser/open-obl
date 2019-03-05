#include "job/job.hpp"
#include <iostream>

int main() {
  oo::JobManager::start();

  oo::JobCounter jc0{1};
  oo::JobManager::runJob([]() {
    std::cerr << "Job 0 says goodnight\n";
    boost::this_fiber::sleep_for(std::chrono::seconds(3));
    std::cerr << "Job 0 woke up!\n";
  }, &jc0);

  oo::JobCounter jc1{1};
  oo::JobManager::runJob([]() { std::cerr << "Job 1 says hello\n"; }, &jc1);

  oo::JobManager::runJob([]() { std::cerr << "Job 2 says hi\n"; });

  // Wait on jc1
  std::cerr << "Waiting on job 1...\n";
  oo::JobManager::waitOn(&jc1);
  std::cerr << "Job 1 is done!\n";

  // Do some 'work' involving the result of jc0
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Wait on jc0
  std::cerr << "Waiting on job 0...\n";
  oo::JobManager::waitOn(&jc0);
  std::cerr << "Job 0 is done!\n";

  oo::JobManager::stop();

  return 0;
}