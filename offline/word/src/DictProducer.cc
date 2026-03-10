#include "../include/DictProducer.h"

#include <fstream>
#include <map>
#include <iostream>

using std::ifstream;
using std::ofstream;
using std::cout;
using std::endl;
using std::map;


DictProducer::DictProducer()
: _jieba()
, _dir()
{   
    //打开目录
    _dir("data/CN/art");
    for(auto & fileName : _dir.files())
    {
        //将所有文件路径存储在DictProducer
        //的vector容器中
        // _file.push_back(fileName);
        _file.emplace_back(fileName);
    }

    //创建中文词典库
    createCnDict();
    //创建英文词典库
    createEnDict();
    //将词典库、索引库、id库存起来
    store();
    storeToMysql();
}

DictProducer::~DictProducer()
{

}

//创建中文词典库
void DictProducer::createCnDict()
{
    for(string & fileName : _file)
    {
        ifstream ifs;
        openFile(ifs, fileName);
        string line;
        //这里做的很简单,直接以空格为分隔符，
        //读到line中，然后进行分词处理
        while(ifs >> line)
        {
            //使用cppjieba对象进行分词
            vector<string> results = _jieba(line);  // 重写过wordSegmentation的 operator()，所以可以直接调用对象
            for(string & elem : results)
            {
                //汉字组(这里就是在判断是不是中文)
                if(elem.size() % CHSIZE == 0) // 如果是中文，那么一个词就是3个字节
                { 
                    int & isExit = _dict[elem]; // 通过elem这个词来访问_dict这个unordered_map，如果这个词之前没有出现过，那么就会在_dict中创建一个新的元素，键为elem，值为0，并返回这个值的引用；如果这个词之前已经出现过，那么就直接返回这个词对应的值的引用。
                    if(isExit)  // 如果这个词之前已经出现过，那么isExit的值就不为0，说明这个词已经存在于_dict中，那么就将这个词的频率加1；如果这个词之前没有出现过，那么isExit的值就是0，说明这个词还没有存在于_dict中，那么就将isExit的值设置为1，表示这个词第一次出现，并且将这个词的id和这个词本身存储在_idMap中，然后调用insertIndex函数将这个词的id插入到_index中。
                    {
                        ++isExit;
                    }
                    else
                    {
                        isExit = 1;
                        size_t id = _dict.size();
						//单词(或者短语)第一次出现的时候，会将对应的序号
						//与单词(或者短语)存起来，单词的序号最大值其实也
						//就是单词的总数
                        _idMap[id] = elem;
                        insertIndex(elem, id);
                    }
                }
            }
        }
        ifs.close();
    }
}

void DictProducer::insertIndex(const string & elem, size_t id)
{
    vector<string> results;
    _jieba.CutSmall(elem, results, 1); // CutSmall用于将一个字符串切分成更小的部分。第一个参数是要切分的字符串elem，第二个参数用于存储切分后的结果，第三个参数表示切分的粒度，也就是每个切分结果的最大长度。在这个例子中，sz被设置为1，表示每个切分结果最多只能包含一个字符。
    for(string & it : results)
    {
        _index[it].insert(id); // _index是一个unordered_map，键是切分结果（也就是elem的子串），值是一个set，存储了所有包含这个子串的单词的id。
                               // 通过_index[it]访问这个set，然后调用insert(id)将当前单词的id插入到这个set中。
    }
}

void DictProducer::store()
{
    ofstream ofs_dict;
    inputFile(ofs_dict, "data/dict.dat");
    for(auto it = _dict.begin(); it != _dict.end(); ++it )
    {
        ofs_dict <<  it->first << " " << it->second << endl;
    }
    
    ofstream ofs_index;
    inputFile(ofs_index, "data/index.dat");
    for(auto it = _index.begin(); it != _index.end(); ++it )
    {
        ofs_index << it->first << " " ;
        for(auto& st : it->second )
        {
            ofs_index << st << " ";
        }
        ofs_index << endl;
    }
#if 1
    ofstream ofs_idMap;
    inputFile(ofs_idMap, "data/idMap.dat");
    for(auto it = _idMap.begin(); it != _idMap.end(); ++it )
    {
        ofs_idMap <<  it->first << " " << it->second << endl;
    }
    ofs_idMap.close();
#endif
    ofs_dict.close();
    ofs_index.close();
}

void DictProducer::createEnDict()
{
    ifstream ifs;  
	//因为英文就一个文件，不然也需要像中文一样
	//遍历文件夹
    openFile(ifs,"data/EN/english.txt");
    string line;   
    while(getline(ifs, line))
    {                  
        processLine(line);
        istringstream iss(line);
        string word;                                         
        while(iss >> word)
        {   
            //对于英文而言，一个字符就是一个字节
            int &isExit = _dict[word];
            if(isExit)
            {
                ++isExit;
            }
            else
            {
                isExit = 1;
                size_t id = _dict.size();
                _idMap[id] = word;
                insertEnIndex(word, id);
            }
        }    
    }    
    ifs.close();
}

void DictProducer::insertEnIndex(const string & word, size_t id)
{
    for(size_t  i = 0; i < word.size(); ++i)
    {   
		//ch的初始化采用的是count个char，也就是每次都是一个char
		//也就是a b c d这种，也就是26个字母对应的索引           
        string ch(1,word[i]);
        _index[ch].insert(id);
    }           
}

void DictProducer::processLine(string & line)
{
    for(auto & elem : line)
    {
        if(!isalpha(elem))
        {
            elem = ' ';
        }
        else if(isupper(elem))
        {
            elem = tolower(elem);
        }
    }
}

void DictProducer::openFile(ifstream &ifs, const string & fileName)
{
    ifs.open(fileName);
    cout << "open >>>" << fileName << endl;
    if(!ifs)
    {
        perror("open file failed in Dict ifs");
        return;
    }
}

void DictProducer::inputFile(ofstream & ofs, const string & fileName)
{
    ofs.open(fileName);
    if(!ofs)
    {
        perror("open file failed in Dict ifs");
        return;
    }
}

void DictProducer::storeToMysql(){
    ifstream ifs("conf/word.conf");
    if(!ifs){
        std::cerr << "Failed to open config file: " << "conf/word.conf" << std::endl;
        return;
    }

    map<string, string> configs;
    string key, val;
    while(ifs >> key >> val){
        configs[key] = val;
    }

    ifs.close();

    if(configs.find("MYSQL_HOST") == configs.end()){
        std::cerr << "MYSQL_HOST not found in config file" << std::endl;
        return;
    }

    string host = configs["MYSQL_HOST"];
    int port = stoi(configs["MYSQL_PORT"]);
    string user = configs["MYSQL_USER"];
    string passwd = configs["MYSQL_PASSWD"]; 
    string dbname = configs["MYSQL_DBNAME"];

    
    MysqlHelper mysql(host, port, user, passwd, dbname);
    cout << "Connected to MySQL database successfully!" << endl;

    mysql.execute("TRUNCATE TABLE dictionary"); // 清空dictionary表
    mysql.execute("TRUNCATE TABLE char_index");

    const size_t BATCH_SIZE = 500; 
    size_t count = 0;
    string sql;

    // 批量插入数据到dictionary表
    for(auto it = _idMap.begin(); it != _idMap.end(); ++it){
        int id = it->first;
        const string& word = it->second;
        int freq = _dict[word];
        if(count == 0){ // 如果是批次的第一条记录，初始化SQL语句
            sql = "INSERT INTO dictionary (id, word, freq) VALUES ";
        } else sql += ", "; 

        // 对word进行转义，防止SQL注入和语法错误
        sql += "(" + std::to_string(id) + ", '" + mysql.escapeString(word) + "', " + std::to_string(freq) + ")";
        ++count;
        if(count >= BATCH_SIZE){
            mysql.execute(sql);
            count = 0;
        }
    }

    // 插入剩余的记录
    if(count > 0){
        mysql.execute(sql);
        count = 0;
    }
    cout << "Inserted data into dictionary table successfully!" << endl;
    cout << _idMap.size() << " records inserted into dictionary table." << endl;


    for(auto it = _index.begin(); it != _index.end(); ++it){
        const string& charKey = it->first;
        for(auto& wordId : it->second){
            if(count == 0){
                sql = "INSERT INTO char_index (char_key, word_id) VALUES ";
            }else sql += ", ";

            sql += "('" + mysql.escapeString(charKey) + "', " + std::to_string(wordId) + ")";
            ++count;

            if(count >= BATCH_SIZE){
                mysql.execute(sql);
                count = 0;
            }
        }
    }

    if(count > 0){
        mysql.execute(sql);
    }
    cout << "Inserted data into char_index table successfully!" << endl;
    cout << "Total " << _index.size() << " unique char keys inserted into char_index table." << endl;
    cout << "Data insertion to MySQL completed!" << endl;
}