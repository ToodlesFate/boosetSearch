/* 
    去标签, 数据清洗
*/ 
#include <iostream>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

#include "util.hpp"

const std::string src_path = "data/input";      //  是一个目录, 下面放的是所有的html 网页 
const std::string output = "data/raw_html/raw.txt"; // 解析后的路径 

typedef struct DocInfo
{
    std::string title;
    std::string content;
    std::string url;
}DocInfo_t;


/* 
    const &: 表示输入
    *: 输出
    &: 输入输出
*/ 

// src_path 从这个路径中 得到文件列表放在 files_list 中
// 枚举文件 
bool EnumFile(const std::string &src_path, std::vector<std::string>* files_list) {
    // 枚举文件 
    namespace fs = boost::filesystem;
    fs::path root_path(src_path);  // 设置搜索路径 

    // 判断路径是否存在
    if (!fs::exists(root_path)) {
        std::cerr << src_path << " not exists " << std::endl;
        return false;
    } 

    fs::recursive_directory_iterator end;  // 定义一个空的迭代器, 用来进行判断递归结束
    for (fs::recursive_directory_iterator iter(root_path); iter != end; iter ++ ) {
        // 判断文件是否是普通文件 html 都是普通文件
        if (!fs::is_regular_file(*iter)){
            continue;
        }

        // path()       提起当前指向路径的字符串 
        // exitension() 提取当前文件的后缀
        if (iter->path().extension() != ".html") {
            continue;
        }

        // 当前一定一定是一个合法的以 html 结尾的普通网页文件 
        files_list->push_back(iter->path().string());

    }
    return true;
}

/* 
<title> info </title>
*/ 
static bool ParseTitle(const std::string& file, std::string* title) {
    std::size_t begin = file.find("<title>");
    if (begin == std::string::npos) {
        return false;
    }

    std::size_t end = file.find("</title>");
    if (end == std::string::npos) {
        return false;
    } 

    // begin 指向第一个字符
    begin += std::string("<title>").size();

    if (begin > end) {
        return false;
    } 

    *title = file.substr(begin, end - begin); // 截取之间的内容

    return true;
}

// 获取 html 的content 内容 
// 将单标签和双变迁去掉
static bool ParseContent(const std::string& file, std::string* content) {
    // 去标签 数据清洗的一种方案 基于一个简易的状态机 
    enum status {
        LABLE,  // 读的是 label 
        CONTENT // 读的是 content
    };
 
    enum status s = LABLE;
    for (char c : file) {
        switch(s) {
            case LABLE:
                if (c == '>') s = CONTENT; // 碰到右标签, 下一个开始是内容
                break;

            case CONTENT:
                if (c == '<') s = LABLE;  // 碰到左标签, 下一个开始是标签
                else {
                    // 我们不想保留原始文件中的 \n, 因为我们想用 \n 作为 html 解析之后文本的分隔符
                    if (c == '\n') c = ' ';
                    content->push_back(c);
                }
                break;

            default :
                break;
        }   
    }
    return true;   
}

// 解析 URL 
// 由文件的路劲构建出来
static bool ParseUrl(const std::string& file_path, std::string* url) {
    // boost 库的官方文档 和我们的下载下来的文档
    std::string url_head = "https://www.boost.org/doc/libs/1_81_0/doc/html";
    std::string url_tail = file_path.substr(src_path.size());
    // 路径拼接
    *url = url_head + url_tail;
    return true;
}

void showDoc(const DocInfo_t &doc) {
    std::cout << "title: " << doc.title << std::endl;
    // std::cout << "content: " << doc.content << std::endl;
    std::cout << "url: " << doc.url << std::endl;
}

// 解析 HTML 
bool ParseHtml(const std::vector<std::string> &files_list,  std::vector<DocInfo_t> * result) {
    for (const std::string& file : files_list) {
        // 1. 读取文件 
        std::string file_result;
        if (!ns_util::FileUtil::ReadFile(file, &file_result)) {
            continue;
        }
        
        // 2. 解析指定的文件, 提取title 
        DocInfo_t doc;
        if (!ParseTitle(file_result, &doc.title)) {
            continue;
        }
        
        // 3. 解析指定的文件, 提起content
        if (!ParseContent(file_result, &doc.content)) {
            continue;
        }

        // 4. 解析指定的文件路径, 构建url
        if (!ParseUrl(file, &doc.url)) {
            continue;
        }

        result->push_back(std::move(doc));
        // debug
        // showDoc(doc);
    }
    return true;
}

// 将网页保存至目标文件中
// title\3content\3url\n 
// 方便我们直接过去一个文档的全部内容
bool SavaHtml(const std::vector<DocInfo_t> &results, const std::string& output) {
#define SEP '\3'
    // 按照二进制方式写入 
    std::ofstream out(output, std::ios::out | std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "open " << output << " failed " << std::endl;
        return false;
    }

    for (auto& item: results) {
        std::string out_string;
        out_string = item.title;
        out_string += SEP;
        out_string += item.content;
        out_string += SEP;
        out_string += item.url;
        out_string += '\n';
        out.write(out_string.c_str(), out_string.size());
    }

    out.close();
    return true;
}

int main()
{
    std::vector<std::string> files_list; // 文件名列表
    // 1. 获取文件名列表 
    if (!EnumFile(src_path, &files_list) ) {
        // 递归式的把每个 html 文件名带路径保存到 files_list 中, 方便后期进行一个一个的文件处理
        std::cerr << "enum file error " << std::endl;
        return 1;
    }

    // 2. 按照files_list 读取每个文件的内容, 并进行解析 
    std::vector<DocInfo_t> result;         //  得到结果
    if (!ParseHtml(files_list, &result)) {
        // 解析失败
        std::cerr << "Parse html error" << std::endl;
        return 2;
    }

    // 3. 把解析完毕的各个文件内容, 写入到 output , 按照 \3 作为每个文档的分隔符 
    if (!SavaHtml(result, output)) {
        // 保存失败
        std::cerr << "save html error " << std::endl;
        return 3;
    }

    return 0;
}