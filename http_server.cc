#include <iostream>
#include "searcher.hpp"
#include "cpp-httplib/httplib.h"

const std::string root_path = "./wwwroot";
const std::string input = "data/raw_html/raw.txt";

int main()
{
    httplib::Server svr;

    ns_searcher::Searcher search;
    search.InitSearcher(input);
    svr.set_base_dir(root_path.c_str());
    
    svr.Get("/s", [&search](const httplib::Request &req, httplib::Response &resp){
        if (!req.has_param("word")) {
            resp.set_content("必须要有搜索关键字", "text/plain; charset=utf-8");
            return;
        }
        std::string word = req.get_param_value("word");
        std::cout << "用户在搜索: " << word << std::endl;
        std::string json_string;
        search.Search(word, &json_string);
        resp.set_content(json_string, "application/json");
    });
    
 

    svr.listen("0.0.0.0", 8081);
    return 0;
}
