#include "searcher.hpp"
#include <iostream>
#include <string>

const std::string input = "data/raw_html/raw.txt";

int main()
{
    ns_searcher::Searcher * search =new ns_searcher::Searcher();
    search->InitSearcher(input); // 处理好的结果 

    std::string query;
    std::string json_string;
    char buffer[1024];
    while (true) {
        std::cout << "Please Enter You Searcher Query# ";
        fgets(buffer, sizeof(buffer) - 1, stdin);
        query = buffer;
        search->Search(query, &json_string);
        std::cout << json_string << std::endl;
    }
    return 0;
}
