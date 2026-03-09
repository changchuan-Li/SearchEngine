#ifndef __SE_PAGELIB_H__
#define __SE_PAGELIB_H__

#include "DirProducer.h"
#include "WordSegmentation.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <set>
#include <map>

using std::string;
using std::vector;
using std::unordered_map;
using std::set;
using std::map;
using std::pair;

class Configuration;

struct RSSIteam{
    string _title;
    string _link;
    string _description;
};

class Compare{
    bool operator()(const pair<int, double>& lhs, const pair<int, double>& rhs){
        return lhs.first < rhs.first;
    }
};

class PageLib{
public:
    PageLib();
    ~PageLib(){}
    unordered_map<int, map<string, int>>& getWordsMap(){
        return _wordMap;
    }

    unordered_map<int, vector<string>>& getTopK(){
        return _topK;
    }

private:
    void create();
    void store();
    void handleInvertIndex();
    void handleWordMap();
    void handleTopK();
    void PageDeDuplication();
    

private:
    int _DOCICNUM;
    vector<RSSIteam> _rss;
    Configuration* _config;
    DictProducer _dict;
    WordSegmentation& _jieba;
    unordered_map<string, set<pair<int , double>>> _invertIndex;//倒排索引
    unordered_map<int, map<string, int>> _wordMap;//没有用到
    unordered_map<int, vector<string>>   _topK;//没有用到
};

#endif // __SE_PAGELIB_H__