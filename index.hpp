/* 
    简历索引
*/ 

#pragma once 
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <mutex>

#include "util.hpp"
#include "log.hpp"

namespace ns_index {

    // 文档的内容
    struct DocInfo {
        std::string title;             // 文档标题
        std::string content;           // 文档内容
        std::string url;               // 文档的 URL 
        uint64_t doc_id;               // 文档的id
    };


    struct InvertedElem {
        uint64_t doc_id;
        std::string word;
        int weight;
    };

    typedef std::vector<InvertedElem>  InvertedList; // 倒排拉链 

    class Index {

    private:
        std::vector<DocInfo> forward_index;                           // 正排索引的数据结构 类似于数组, 数组的下标天然是文档的id
        std::unordered_map<std::string, InvertedList> inverted_index; // 倒排索引 一定是一个关键字和一组InvertedElem对应 关键字和倒排拉链的映射
    
    private:
        Index() { }
        Index(const Index&) = delete;
        Index& operator=(const Index&) = delete;
        static Index* instance;
        static std::mutex mtx;

    public:
        ~Index() { }

    public:
        static Index* GetInstance() {
            if (nullptr == instance) {
                mtx.lock();
                if (nullptr == instance) {
                    instance = new Index();
                }
                mtx.unlock();
            }
            return instance;
        }

        // 根据doc_id 找到文档内容
        DocInfo* GetForwardIndex(const uint64_t doc_id) {
            if (doc_id >= forward_index.size()) {
                std::cerr << "doc_id out range, error " << std::endl;
                return nullptr;
            }
            return &forward_index[doc_id];
        }

        // 根据关键字string, 获得倒排拉链 
        InvertedList* GetInvertedList(const std::string& word){ 
            auto iter = inverted_index.find(word);
            if (iter == inverted_index.end()) {
                std::cerr << word << " have no InvertedList ---- " << std::endl;
                return nullptr;
            }
            return &(iter->second);
        }

        // 根据去标签格式化后的文档, 构建正排和倒排索引 
        bool BuildIndex(const std::string& input) { 
            std::ifstream in(input, std::ios::in | std::ios::binary);
            if (!in.is_open()) {
                std::cerr << "sorry, " << input << " open error " << std::endl;
                return false;
            }
            std::string line;
            int cnt = 0;
            while (std::getline(in, line)) {
                DocInfo* doc =  BuildForwardIndex(line);
                if (doc == nullptr) {
                    std::cerr << "build " << line << " error " << std::endl;
                    continue;
                }
                BuildInvertedIndex(*doc);
                ++ cnt;
                if (cnt % 50 == 0) {
                    std::string s = " 当前已经建立的索引文档  ";
                    s += std::to_string(cnt);
                    LOG(NORMAL, s);
                }
            }
            return true;
        }

    private:
        DocInfo* BuildForwardIndex(const std::string &line)
        {
            // 1. 解析line, 字符串切分
            std::vector<std::string> result;
            const std::string sep = "\3";
            ns_util::StringUtil::Split(line, &result, sep);
            if (result.size() != 3) {
                return nullptr;
            }

            // 2. 切分好的字符串填充到docInfo
            DocInfo doc;
            doc.title = result[0];
            doc.content = result[1];
            doc.url = result[2];
            doc.doc_id = forward_index.size();

            // 3. 插入到正排索引的vector 中
            forward_index.push_back(doc);
            return &forward_index.back();
        }

        bool BuildInvertedIndex(const DocInfo &doc)
        {
            // DocInfo[title, content, url, doc_id]
            // word --> InvertedList
            // 先要对 title 和 content 进行分词  
            struct word_cnt {
                int title_cnt;
                int content_cnt;
                word_cnt():title_cnt(0), content_cnt(0) {}
            };
            
            std::unordered_map<std::string, word_cnt> word_map;       // 用来暂存词频的映射表 

            std::vector<std::string> title_words;

            ns_util::JiebaUtil::CutString(doc.title, &title_words);    // 分词
 
            for (std::string &s : title_words) {
                boost::to_lower(s);                   // 将分词统一转换为小写 
                word_map[s].title_cnt ++;
            }

            
            std::vector<std::string> content_words;
            ns_util::JiebaUtil::CutString(doc.content, &content_words);

            for (std::string &s : content_words) {
                boost::to_lower(s); 
                word_map[s].content_cnt ++;
            }
#define X 10
#define Y 1

            // 搜索是不区分大小写  需要在代码中对大小写左做统一转换 
            for (auto & word_pair : word_map) {
                InvertedElem item;
                item.doc_id = doc.doc_id;
                item.word = word_pair.first;
                item.weight = X * word_pair.second.title_cnt + Y * word_pair.second.content_cnt;
                InvertedList &inverted_list = inverted_index[word_pair.first];
                inverted_list.push_back(std::move(item));
            }

            return true;
        }
    };

    Index* Index::instance = nullptr;
    std::mutex Index::mtx;
}
