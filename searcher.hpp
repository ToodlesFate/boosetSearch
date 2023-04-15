#pragma once 

#include "index.hpp"
#include "log.hpp"

#include <algorithm>
#include <jsoncpp/json/json.h>

namespace ns_searcher {
    struct InvertedElemPrint {
        uint64_t doc_id;
        int weight;
        std::vector<std::string> words;
        InvertedElemPrint() : doc_id(0), weight(0) {}
    };

    class Searcher {
    private:
        ns_index::Index *index; // 索引 

    public:
        Searcher() {}
        ~Searcher() {}

    public:
        void InitSearcher(const std::string& input) 
        {
            std::cout << "for debug ..." << std::endl;
            // 1. 获取或者创建index 对象 
            index = ns_index::Index::GetInstance();
            LOG(NORMAL, " 获取index 单例成功...");
            // 2. 根据index 对象建立索引 
            index->BuildIndex(input);
            LOG(NORMAL, " 建立正排和倒排成功...");
        }

        void Search(const std::string &query, std::string *json_string) 
        {
            // 1. [分词] 对query 进行分词
            std::vector<std::string> words;
            ns_util::JiebaUtil::CutString(query, &words);

            // 2. [触发] 根据分词的各个 词 进行搜索 
            // 建立索引的时候是忽略大小写的 所以, 搜索的时候关键字也需要 
            // ns_index::InvertedList inverted_list_all;
            std::vector<InvertedElemPrint> inverted_list_all;
            std::unordered_map<uint64_t, InvertedElemPrint> tokens_map;
            for (std::string word : words) {
                boost::to_lower(word);

                ns_index::InvertedList *inverted_list = index->GetInvertedList(word);
                if (nullptr == inverted_list) {
                    continue;
                }

                // inverted_list_all.insert(inverted_list_all.end(), inverted_list->begin(), inverted_list->end()); // 获得所有的倒排拉链 
                for (const auto& elem : *inverted_list) {
                    auto& item = tokens_map[elem.doc_id];
                    item.doc_id = elem.doc_id;
                    item.weight += elem.weight;
                    item.words.push_back(elem.word);
                }

                for (const auto& item : tokens_map) {
                    inverted_list_all.push_back(std::move(item.second));
                }

            }

            // 3. [合并排序] 根据汇总查找结果 按照相关性 进行降序排序 
            std::sort(inverted_list_all.begin(), inverted_list_all.end(), [](const InvertedElemPrint & e1, const InvertedElemPrint& e2){
                return e1.weight > e2.weight;
            });

            // 4. [构建] 根据对应查找出来的结果 构建json 串 --- jsoncpp  -- 通过jsoncpp 完成序列化过程 
            Json::Value root;
            for (auto & item : inverted_list_all) {
                ns_index::DocInfo* doc = index->GetForwardIndex(item.doc_id);
                if (nullptr == doc) {
                    continue;
                }
                Json::Value elem;
                elem["title"] = doc->title;
                elem["desc"] = GetDesc(doc->content, item.words[0]); // 只要部分内容 不是全部内容 
                elem["url"] = doc->url; 
                // // for dubug
                // elem["weight"] = item.weight;
                // elem["id"] = (int)item.doc_id;
                root.append(elem);
            }

            Json::StyledWriter writer;
            *json_string = writer.write(root); // 将序列化的内容 输出 
        }

        // 获取摘要 
        std::string GetDesc(const std::string& html_content, const std::string& word) 
        {
            // 找到 word 在 html_content 中首次出现, 然后往前找 50 字节, 往后找 100 字节, 截取出来这部分  
            // 如果50字节, 则从开始处     如果没有100 字节, 到end 

            // 1. 找到首次出现 
            auto iter = std::search(html_content.begin(), html_content.end(), word.begin(), word.end(), [](int x, int y){
                return (std::tolower(x) == std::tolower(y));
            });

            if (iter == html_content.end()) {
                return "None1";
            }
            std::size_t pos = std::distance(html_content.begin(), iter);

            // 2. 获取start 和 end 位置 
            // size_t 是一个无符号的整形 
            const std::size_t prev_step = 50;
            const std::size_t next_step = 100;
            std::size_t start = 0;
            std::size_t end = html_content.size() - 1;
            if (pos > start + prev_step) start = pos - prev_step;
            if (pos + next_step < end) end = pos + next_step;

            // 3. 截取子串  返回 
            if (start >= end) return "None2"; 
            return html_content.substr(start, end - start + 1);
        }
    };
  
}
