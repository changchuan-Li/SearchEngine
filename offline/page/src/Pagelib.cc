#include "../include/Pagelib.h"
#include "../include/tinyxml2.h"
#include "../include/DirScanner.h"
#include "../include/Configuration.h"
#include "../include/simhash/Simhasher.hpp"
#include "../include/MysqlHelper.h"

#include <math.h>
#include <fstream>
#include <regex>
#include <iostream>
#include <fstream>
#include <map>

using std::cout;
using std::endl;
using std::ofstream;
using std::map;
using namespace tinyxml2;

PageLib::PageLib(): _config(Configuration::getInstance("conf/page.conf")), _jieba(_dict.getJieba()){
    create();
    cout << "create page lib success !" << endl;
    PageDeDuplication();
    cout << "page deduplication success !" << endl;
    handleInvertIndex();
    cout << "handle invert index success !" << endl;

    store();
    cout << "store page lib success !" << endl;
    storeToMysql();
    cout << "store to mysql success !" << endl;
}


void PageLib::create(){
    DirScanner dir(_config->getConfigs().find("XMLDIR")->second);
    for(string& filename : dir.files()){
        XMLDocument doc;
        doc.LoadFile(filename.c_str());
        if(doc.ErrorID()){
            perror("loadFile fail");
            return;
        }

        // 解析XML文件，获取title、link、description
        // FirstChildElement()函数用于获取指定标签的第一个子元素，如果没有找到该标签，则返回nullptr
        XMLElement* itemNode = doc.FirstChildElement("rss")->FirstChildElement("channel")->FirstChildElement("item");
        while(itemNode){
            RSSIteam rssItem;
            std::regex reg("<[^>]+>"); // 匹配HTML标签的正则表达式
            
            if(itemNode->FirstChildElement("title")){
                // regex_replace函数将title中的HTML标签替换为空字符串
                rssItem._title = regex_replace(itemNode->FirstChildElement("title")->GetText(), reg, "");
            }else{
                rssItem._title = "this document has no title";
            }

            string link = itemNode->FirstChildElement("link")->GetText();
            rssItem._link = link;

            if(itemNode->FirstChildElement("description")){
                rssItem._description = regex_replace(itemNode->FirstChildElement("description")->GetText(), reg, "");
            }else{
                rssItem._description = "this document has no description";
            }

            _rss.emplace_back(rssItem);
            // NextSiblingElement()函数用于获取指定标签的下一个兄弟元素，如果没有找到该标签，则返回nullptr
            itemNode = itemNode->NextSiblingElement("item"); 
        }
    }
}

void PageLib::store(){
    ofstream ofs("data/ripepage.dat");
    if(!ofs){
        perror("ofstream open fail");
        return;
    }
    ofstream ofs1("data/offset.dat");
    if(!ofs1){
        perror("ofstream1 open fail");
        return;
    }

    for(size_t i = 0; i < _rss.size(); ++i){
        size_t idx = i + 1;
        ofs1 << idx << " "; // 将文档ID写入offset.dat文件中，文档ID从1开始递增
        size_t beginpos = ofs.tellp(); // ofs.tellp()函数用于获取当前输出流的写入位置，即文件末尾的位置
        ofs1 << beginpos << " "; // 将文档在ripepage.dat文件中的起始位置写入offset.dat文件中
        ofs << "<doc>" << endl;
        ofs << "\t<docid>" << i + 1 << "</docid>" <<endl;
        ofs << "\t<title>" << _rss[i]._title << "</title>" <<endl;
        ofs << "\t<url>" << _rss[i]._link << "</url>" <<endl;
        ofs << "\t<description>" << _rss[i]._description<< "</description>" <<endl;
        ofs << "</doc>" <<endl;
        size_t endpos = ofs.tellp(); 
        size_t len = endpos - beginpos;
        ofs1 << len << "\n";
    }
    cout << "网页库和网页偏移库持久化完成" << endl;

    ofstream ofs2("data/invertIndex.dat");
    if (!ofs2){
        perror("ofstream2 open fail");
        return;
    }

    for(auto& it : _invertIndex){
        ofs2 << it.first << " ";
        for(auto it2 = it.second.begin(); it2 != it.second.end(); ++it2){
            ofs2 << it2->first << " " << it2->second << " ";
        }
        ofs2 << endl;
    }
    cout << "倒排索引持久化完成" << endl;
    ofs.close();
    ofs1.close();
    ofs2.close();
}

void PageLib::storeToMysql(){
    auto& configs = _config->getConfigs();
    string host   = configs["MYSQL_HOST"];
    int    port   = std::stoi(configs["MYSQL_PORT"]);
    string user   = configs["MYSQL_USER"];
    string passwd = configs["MYSQL_PASSWD"];
    string dbname = configs["MYSQL_DBNAME"];

    MysqlHelper mysql(host, port, user, passwd, dbname);
    cout << "连接MySQL数据库成功" << endl;

    mysql.execute("TRUNCATE TABLE web_pages");
    mysql.execute("TRUNCATE TABLE inverted_index");
    cout << "清空MySQL数据库中的表成功" << endl;

    const size_t BATCH_SIZE = 500;
    size_t count = 0;
    string sql;

    for(size_t i = 0; i < _rss.size(); ++i){
        size_t docid = i + 1;
        if(count == 0){ // 如果当前批次的SQL语句为空，则构建INSERT语句的开头
            sql = "INSERT INTO web_pages (docid, title, url, description) VALUES ";
        }else sql += ", "; // 如果当前批次的SQL语句不为空，则在前面添加逗号分隔符

        sql += "(" + std::to_string(docid)
             + ",'" + mysql.escapeString(_rss[i]._title) + "'"
             + ",'" + mysql.escapeString(_rss[i]._link) + "'"
             + ",'" + mysql.escapeString(_rss[i]._description) + "')";

        ++count;
        if(count >= BATCH_SIZE || i == _rss.size() - 1){ // 如果当前批次的SQL语句数量达到批次大小，则执行SQL语句，并重置批次计数器和SQL语句
            mysql.execute(sql);
            count = 0;
        }
    }
    cout << "网页数据批量插入MySQL数据库成功" << endl;

    count = 0;
    // 批量插入倒排索引数据到MySQL数据库中，构建INSERT语句的开头
    for(auto it = _invertIndex.begin(); it != _invertIndex.end(); ++it){
        const string& word = it->first;
        for(auto it2 = it->second.begin(); it2 != it->second.end(); ++it2){
            if(count == 0){
                sql = "INSERT INTO inverted_index (word, docid, weight) VALUES ";
            }else sql += ", ";

            sql += "('" + mysql.escapeString(word) + "'"
                 + "," + std::to_string(it2->first)
                 + "," + std::to_string(it2->second) + ")";
            
             ++count;

            if(count >= BATCH_SIZE){ // 如果当前批次的SQL语句数量达到批次大小，则执行SQL语句，并重置批次计数器和SQL语句
                sql += " ON DUPLICATE KEY UPDATE weight = weight + VALUES(weight)";
                mysql.execute(sql);
                count = 0;
            }
        }
    }

    if(count > 0){
        // 如果当前批次的SQL语句不为空，则在末尾添加ON DUPLICATE KEY UPDATE子句，处理主键冲突的情况，并执行SQL语句
        // ON DUPLICATE KEY UPDATE : 当插入的数据在数据库中已经存在时，更新该数据的权重值，而不是插入新的数据
                                  // 可以避免主键冲突导致的插入失败，同时也可以累加权重值，反映词在文档中的重要程度
        sql += "ON DUPLICATE KEY UPDATE weight = weight + VALUES(weight)"; // 如果当前批次的SQL语句不为空，则在末尾添加ON DUPLICATE KEY UPDATE子句，处理主键冲突的情况
        mysql.execute(sql);
    }

    cout << "倒排索引数据批量插入MySQL数据库成功" << endl;
}

void PageLib::handleInvertIndex(){ // 构建倒排索引, 例如：_invertIndex["中国"] = {{1, 0.5}, {2, 0.3}}，表示词“中国”在文档1中的权重为0.5，在文档2中的权重为0.3
    unordered_map<string, unordered_map<int, double>> tf; // tf["中国"] = {{1, 3}, {2, 2}}，表示词“中国”在文档1中的词频为3，在文档2中的词频为2
    int i = 0;
    for(auto it = _rss.begin(); it != _rss.end(); ++it){
        int docid = ++i;
        vector<string> results = _jieba(it->_description); // 对文档描述信息进行分词，得到一个词的列表
        for(string& word : results){ // 遍历分词结果，统计每个词在每个文档中的词频
            for(char& c : word) {
                if(c >= 'A' && c <= 'Z') {
                    c += 32; // ASCII 码转换：大写转小写
                }
            }
            
            auto& isExit = tf[word];
            if(isExit[docid]){ // 如果该词在该文档中已经出现过，则将词频加1
                ++isExit[docid];
            }else isExit[docid] = 1;
        }
    }

    unordered_map<int, double> docW;
    for(auto it = tf.begin(); it != tf.end(); ++it){
        string word = it->first;
        size_t DF = it->second.size(); // DF表示包含该词的文档数量

        for(auto num = it->second.begin(); num != it->second.end(); ++num){
            int docid = num->first;
            int TF = num->second; // TF表示词在文档中的词频，计算公式为词在文档中出现的次数 / 文档中总词数，这里为了简化计算，直接使用词在文档中出现的次数作为TF值
            double IDF = log(1.0 * _DOCICNUM / (DF + 1)) / log(2); // IDF表示逆文档频率，计算公式为log(总文档数量 / (包含该词的文档数量 + 1))，加1是为了避免分母为0的情况
            double w = TF * IDF; // 计算词的权重，计算公式为TF * IDF
            num->second = w;
            docW[docid] += w * w; // 计算每个文档的权重平方和，后续计算余弦相似度时需要用到
        }
    }

    for(auto it = tf.begin(); it != tf.end(); ++it){
        string word = it->first;
        for(auto elem = it->second.begin(); elem != it->second.end(); ++elem){
            int docid = elem->first;
            double sumW2 = docW[docid];
            double w = elem->second;
            double FinW = w / sqrt(sumW2); // 计算词的最终权重，计算公式为词的权重 / 文档权重平方和的平方根
            // _invertIndex : unordered_map<string, set<pair<int , double>>>
            _invertIndex[word].insert({docid, FinW}); // 将词和文档ID以及词的最终权重插入倒排索引中
        }
    }
}

void PageLib::handleWordMap(){

}

void PageLib::handleTopK(){

}

void PageLib::PageDeDuplication(){
    size_t topN = 20;

    // 创建SimHasher对象
    simhash::Simhasher simhasher(DICT_PATH, HMM_PATH, IDF_PATH, STOP_WORD_PATH);
    vector<pair<int, uint64_t>> Sim; 

    // 计算每个文档的SimHash值，并存储在Sim向量中
    for(size_t i = 0; i < _rss.size(); ++i){
        uint64_t u = 0;
        simhasher.make(_rss[i]._description, topN, u);
        Sim.emplace_back(i, u);
    }

    set<int> tmp;
    // 比较每个文档的SimHash值，如果相同，则说明这两个文档是重复的
    for(size_t it = 0; it < Sim.size(); ++it){
        int id1 = Sim[it].first;
        int sz1 = _rss[id1]._description.size();
        for(size_t it2 = (it + 1); it2 < Sim.size(); ++it2){ // 只需要比较SimHash值在Sim向量中位置在it之后的文档，避免重复比较
            if(simhash::Simhasher::isEqual(Sim[it].second, Sim[it2].second)){ // 如果两个文档的SimHash值相同，则说明它们是重复的
                int sz2 = _rss[Sim[it2].first]._description.size(); 
                if(sz1 > sz2) tmp.insert(Sim[it2].first); // 如果文档1的描述信息长度大于文档2的描述信息长度，则说明文档2是重复的，应该删除文档2
                else{
                    tmp.insert(id1);
                    break;
                }
            }
        }
    }

    int s = 0;
    for(auto num : tmp){ 
        num -= 0;
        int i = 0;

        // 遍历_rss，找到索引为num的文档，并删除它
        for(auto it = _rss.begin(); it != _rss.end(); ++it, ++i){
            if(num == i){ // 如果num等于当前文档的索引，则说明该文档是重复的，需要删除
                _rss.erase(it); // 从_rss中删除重复的文档
                ++s;
                break;
            }
        }
    }
    _DOCICNUM = _rss.size();
    cout << "去重了 " << s << " 个文档" << endl;
}