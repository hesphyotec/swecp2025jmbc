#ifndef BGEZDB
#define BGEZDB

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <mutex>
#include <variant>

#include "sqlite3.h"
#include "crow.h"

#include "bgeztraits.h"
#include "bgezuser.h"
#include "bgezitem.h"

using DBArg = std::variant<int, std::string, double>;
using DBArgList = std::vector<DBArg>;

class DBConnection{                               //List of functions for interacting with the DB.
private:
    inline static std::mutex mtx{};
    sqlite3* db;

    bool bindValue(sqlite3_stmt* stmt, int index, int arg){
        return sqlite3_bind_int(stmt, index, arg) == SQLITE_OK;
    }

    bool bindValue(sqlite3_stmt* stmt, int index, double arg){
        return sqlite3_bind_double(stmt, index, arg) == SQLITE_OK;
    }

    bool bindValue(sqlite3_stmt* stmt, int index, const std::string& arg){
        return sqlite3_bind_text(stmt, index, arg.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
    }

    bool bindValue(sqlite3_stmt* stmt, int index, const DBArg& arg){
        return std::visit([&](auto&& value){
            return bindValue(stmt, index, value);
            }, arg);
    }
public:
    DBConnection(){
        if(sqlite3_open("core.db", &db) == SQLITE_OK){
            CROW_LOG_DEBUG << "DB Opened.";
        } else {
            CROW_LOG_DEBUG << "DB failed to open.";
            sqlite3_close(db);
        }
    }

    ~DBConnection(){
        if (db != nullptr){
            sqlite3_close(db);
            db = nullptr;
        }
    }

    template <typename T>
    inline bool accessDB(const std::string& query, const std::vector<DBArgList>& qArgs, T get){
        std::lock_guard<std::mutex> _(mtx);
        sqlite3_stmt* statement{};
        const char* tail{};
        CROW_LOG_DEBUG << query << "\n";
        const char* st{query.c_str()};
        int prepstmt{sqlite3_prepare_v2(db, st, -1, &statement, &tail)};
        if (prepstmt != SQLITE_OK){
            CROW_LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
            if (statement != nullptr){
                sqlite3_finalize(statement);
            };
            return false;
        }
        for(int j = 0; j < static_cast<int>(qArgs.size()); ++j){
            for(int i = 0; i < static_cast<int>(qArgs[j].size()); ++i){
                if (!bindValue(statement,i+1, qArgs[j][i])){
                    CROW_LOG_ERROR << "Failed to bind Statement. Aborting Query.";
                    sqlite3_finalize(statement);
                    return false;
                }
            }
            CROW_LOG_DEBUG << "DB statement prepared.";
            int res{};
            while((res = sqlite3_step(statement)) != SQLITE_DONE){
                CROW_LOG_DEBUG << res;
                if (res == SQLITE_ROW){
                    get(statement);
                    CROW_LOG_DEBUG << "Statement Step.";
                } else {
                    CROW_LOG_ERROR << "Bad step. Code: " << res << "\n";
                    sqlite3_finalize(statement);
                    return false;
                }
            }
            if (res == SQLITE_DONE){
                CROW_LOG_INFO << "Query Complete!\n";
            }
            sqlite3_reset(statement);
        }
        sqlite3_finalize(statement);
        return true;
    }
    template<typename T>
    bool accessDB(const std::string& query, const std::vector<DBArg>& qArgs, T get){
        std::lock_guard<std::mutex> _(mtx);
        sqlite3_stmt* statement{};
        const char* tail{};
        CROW_LOG_DEBUG << query << "\n";
        const char* st{query.c_str()};
        int prepstmt{sqlite3_prepare_v2(db, st, -1, &statement, &tail)};
        if (prepstmt != SQLITE_OK){
            CROW_LOG_ERROR << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
            if (statement != nullptr){
                sqlite3_finalize(statement);
            };
            return false;
        }
        for(int i = 0; i < static_cast<int>(qArgs.size()); ++i){
            if (!bindValue(statement,i+1, qArgs[i])){
                CROW_LOG_ERROR << "Failed to bind Statement. Aborting Query.";
                sqlite3_finalize(statement);
                return false;
            }
        }

        CROW_LOG_DEBUG << "DB statement prepared.";
        int res{};
        while((res = sqlite3_step(statement)) != SQLITE_DONE){
            CROW_LOG_DEBUG << res;
            if (res == SQLITE_ROW){
                get(statement);
                CROW_LOG_DEBUG << "Statement Step.";
            } else {
                CROW_LOG_ERROR << "Bad step. Code: " << res << "\n";
                sqlite3_finalize(statement);
                return false;
            }
        }
        if (res == SQLITE_DONE){
            CROW_LOG_INFO << "Query Complete!\n";
        }
        sqlite3_finalize(statement);
        return true;
    }
};

namespace DBCore{
    inline User getUser(const std::string& name, DBConnection& con){      //getUser: retrieves user information from database.
        CROW_LOG_DEBUG << name;
        User user{};
        std::string s{"SELECT * FROM Users WHERE username = ?;"};
        DBArgList arg{name};
        con.accessDB(s, arg, [&user](sqlite3_stmt* statement){
            const int id{sqlite3_column_int(statement, 0)};
            const std::string uname{reinterpret_cast<const char*>(sqlite3_column_text(statement, 1))};
            const std::string passw{reinterpret_cast<const char*>(sqlite3_column_text(statement, 2))};
            const std::string email{reinterpret_cast<const char*>(sqlite3_column_text(statement, 3))};
            const Trait traits{sqlite3_column_int(statement, 4)};
            user.setUid(id).setName(uname).setPass(passw).setEmail(email);
        });
        return user;
    }

    inline User getUser(const int id, DBConnection& con){      //getUser: retrieves user information from database.
        CROW_LOG_DEBUG << id;
        User user{};
        std::string s{"SELECT * FROM Users WHERE uid = ?;"};
        DBArgList arg{id};
        con.accessDB(s, arg, [&user](sqlite3_stmt* statement){
            const int id{sqlite3_column_int(statement, 0)};
            const std::string uname{reinterpret_cast<const char*>(sqlite3_column_text(statement, 1))};
            const std::string passw{reinterpret_cast<const char*>(sqlite3_column_text(statement, 2))};
            const std::string email{reinterpret_cast<const char*>(sqlite3_column_text(statement, 3))};
            const Trait traits{sqlite3_column_int(statement, 4)};
            user.setUid(id).setName(uname).setPass(passw).setEmail(email);
        });
        return user;
    }

    inline bool addUser(const User& user, DBConnection& con){
        std::string s{"INSERT INTO Users (username, password, email, pref) VALUES (?, ?, ?, ?); "};
        DBArgList args{
            user.name(),
            user.pass(),
            user.email(),
            static_cast<int>(user.pref())
        };
        return con.accessDB(s, args, [](sqlite3_stmt*){});
    }

    inline bool deleteUser(const User& user, DBConnection& con){
        std::string s{"DELETE FROM Users WHERE uid = ?;"};
        DBArgList arg{user.uid()};
        return con.accessDB(s, arg, [](sqlite3_stmt*){});
    }

    inline bool addItem(const User& user, const Item& item, DBConnection& con){
        CROW_LOG_DEBUG << "Adding item to db.";
        std::string s{"INSERT INTO UserItems VALUES (?, ?);"};
        DBArgList args{user.uid(), item.id()};
        return con.accessDB(s, args, [](sqlite3_stmt*){});
    }

    inline Item getItem(const std::string& name, DBConnection& con){
        Item item{};
        std::string s{"SELECT * FROM Ingredients WHERE name = ?;"};
        DBArgList arg{name};
        con.accessDB(s, arg, [&item](sqlite3_stmt* statement){
            const int id{sqlite3_column_int(statement, 0)};
            const std::string name{reinterpret_cast<const char*>(sqlite3_column_text(statement, 1))};
            const std::string desc{reinterpret_cast<const char*>(sqlite3_column_text(statement, 2))};
            const std::string type{reinterpret_cast<const char*>(sqlite3_column_text(statement, 3))};
            //const Trait iTraits{sqlite3_column_int(statement, 4)};
            item.setId(id).setName(name).setDesc(desc).setType(type);
        });
        return item;
    }

    inline Item getItem(int id, DBConnection& con){
        Item item{};
        std::string s{"SELECT * FROM Ingredients WHERE id = ?;"};
        DBArgList arg{id};
        con.accessDB(s, arg, [&item](sqlite3_stmt* statement){
            const int id{sqlite3_column_int(statement, 0)};
            const std::string name{reinterpret_cast<const char*>(sqlite3_column_text(statement, 1))};
            const std::string desc{reinterpret_cast<const char*>(sqlite3_column_text(statement, 2))};
            const std::string type{reinterpret_cast<const char*>(sqlite3_column_text(statement, 3))};
            //const Trait iTraits{sqlite3_column_int(statement, 4)};
            item.setId(id).setName(name).setDesc(desc).setType(type);
        });
        return item;
    }

    inline crow::json::wvalue getItemList(const User& user, DBConnection& con){
        std::vector<int> ids{};
        std::string s{"SELECT iid FROM UserItems WHERE uid = ?;"};
        DBArgList arg{user.uid()};
        con.accessDB(s, arg, [&ids](sqlite3_stmt* statement){
            ids.push_back(sqlite3_column_int(statement, 0));
        });

        std::vector<Item> items{};
        for (int i : ids){
            items.push_back(getItem(i, con));
        }

        crow::json::wvalue res;
        std::vector<crow::json::wvalue> datalist = crow::json::wvalue::list();

        res["status"] = "success";

        for (const auto& item : items){
            datalist.push_back(item.toJson());
        }
        res["data"] = std::move(datalist);
        return res;
    }

    inline bool deleteItem(const Item& item, DBConnection& con){
        std::string s{"DELETE FROM UserItems WHERE iid = ?;"};
        DBArgList arg{item.id()};
        return con.accessDB(s, arg, [](sqlite3_stmt*){});
    }

}

#endif