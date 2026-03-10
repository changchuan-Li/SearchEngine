#ifndef __SE_MYSQLHELPER_H__
#define __SE_MYSQLHELPER_H__

#include <mysql/mysql.h>
#include <string>
#include <stdexcept>
#include <iostream>

using std::string;
using std::cout;
using std::endl;

class MysqlHelper{
public:
    // 构造函数，连接数据库，如果连接失败会抛出异常
    MysqlHelper(const string& host, unsigned int port,
                const string& user, const string& pwd,
                const string& dbname): _conn(nullptr)
    {
        _conn = mysql_init(nullptr); 
        if(!_conn){
            throw std::runtime_error("mysql_init failed");
        }

        mysql_options(_conn, MYSQL_SET_CHARSET_NAME, "utf8mb4"); 

        
        if(!mysql_real_connect(_conn, host.c_str(), user.c_str(),
                            pwd.c_str(), dbname.c_str(), 
                            port, nullptr, 0))
        {
            string err = mysql_error(_conn);
            mysql_close(_conn);
            _conn = nullptr;
            throw std::runtime_error("mysql_real_connect failed: " + err);
        }

        mysql_set_character_set(_conn, "utf8mb4");
    }

    ~MysqlHelper(){ // 关闭连接
        if(_conn){
            mysql_close(_conn);
            _conn = nullptr;
        }
    }

    // 执行SQL语句，如果执行失败会抛出异常
    void execute(const string& sql){ 
        if(mysql_query(_conn, sql.c_str())){
            throw std::runtime_error("mysql_query failed: "
                                     + string(mysql_error(_conn))
                                     + "\nSQL: " + sql.substr(0, 200));
        }
    }

    // 转义字符串，防止SQL注入
    string escapeString(const string& src){
        string dst(src.size() * 2 + 1, '\0'); // 预分配足够的空间
        // mysql_real_escape_string返回转义后的字符串长度 (不包括末尾的'\0')
        unsigned long len = mysql_real_escape_string(_conn, &dst[0], src.c_str(), src.size());
        dst.resize(len);
        // dst已经是转义后的字符串，直接返回
        return dst;
    }

    MYSQL* getConn(){
        return _conn;
    }

private:
    MysqlHelper(const MysqlHelper&) = delete;
    MysqlHelper& operator=(const MysqlHelper&) = delete;
    MYSQL* _conn;
};

#endif // __SE_MYSQLHELPER_H__
