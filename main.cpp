#include <iostream>
#include <memory>
#include <vector>

#include "gorilla_compressor.hpp"

int main() {
    std::cout << "hello world" << std::endl;


    auto timeseries = std::make_shared<std::vector<std::pair<uint64_t, float>>>();
    timeseries->push_back({1647838800, 100.2}); // 0
    timeseries->push_back({1647838860, 600.2}); // 60
    timeseries->push_back({1647838920, 50.2});  // 0
    timeseries->push_back({1647838981, 25.1});
    timeseries->push_back({1647839043, 10});
    timeseries->push_back({1647839100, 5.9});



    auto gorilla = new GorillaCompressor(timeseries);
    gorilla->Compress();
    gorilla->Decompress();


    
}