#ifndef __SE_MYSQLHELPER_H__
#define __SE_MYSQLHELPER_H__

#include <mysql/mysql.h>
#include <string>
#include <stdexcept>
#include <iostream>

using std::string;
using std::cout;
using std::endl;


class MysqlHelper
{
public:
    MysqlHelper(const string &host, unsigned int port,
                const string &user, const string &passwd,
                const string &dbname)
    : _conn(nullptr)
    {
        _conn = mysql_init(nullptr);
        if (!_conn)
        {
            throw std::runtime_error("mysql_init failed");
        }

        // 设置 UTF-8 字符集
        mysql_options(_conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");

        if (!mysql_real_connect(_conn, host.c_str(), user.c_str(),
                                passwd.c_str(), dbname.c_str(),
                                port, nullptr, 0))
        {
            string err = mysql_error(_conn);
            mysql_close(_conn);
            _conn = nullptr;
            throw std::runtime_error("mysql_real_connect failed: " + err);
        }

        // 连接后再确认字符集
        mysql_set_character_set(_conn, "utf8mb4");
    }

    ~MysqlHelper()
    {
        if (_conn)
        {
            mysql_close(_conn);
            _conn = nullptr;
        }
    }

    // 执行非查询语句（INSERT/UPDATE/DELETE/DDL）
    void execute(const string &sql)
    {
        if (mysql_query(_conn, sql.c_str()))
        {
            throw std::runtime_error("mysql_query failed: "
                                     + string(mysql_error(_conn))
                                     + "\nSQL: " + sql.substr(0, 200));
        }
    }

    // 对字符串进行转义，防止 SQL 注入
    string escapeString(const string &src)
    {
        // 转义后最大长度 = 2*len + 1
        string dst(src.size() * 2 + 1, '\0');
        unsigned long len = mysql_real_escape_string(
            _conn, &dst[0], src.c_str(), src.size());
        dst.resize(len);
        return dst;
    }

    // 获取原始连接（用于 mysql_stmt 等高级操作）
    MYSQL *getConn() { return _conn; }

private:
    MysqlHelper(const MysqlHelper &) = delete;
    MysqlHelper &operator=(const MysqlHelper &) = delete;

    MYSQL *_conn;
};

#endif // __SE_MYSQLHELPER_H__
