#ifndef BGEZDB
#define BGEZDB

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>

#include "sqlite3.h"
#include "crow.h"

#include "bgeztraits.h"
#include "bgezuser.h"
#include "bgezitem.h"


namespace DBCore{                               //List of functions for interacting with the DB.
    template <typename T>
    inline bool accessDB(const std::string& query, T get){
        sqlite3* db{};
        if(sqlite3_open("core.db", &db) == SQLITE_OK){
            CROW_LOG_DEBUG << "DB Opened.";
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
                sqlite3_close(db);
                return false;
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
                    sqlite3_close(db);
                    return false;
                }
            }
            if (res == SQLITE_DONE){
                CROW_LOG_INFO << "Query Complete!\n";
            }
            sqlite3_finalize(statement);
            sqlite3_close(db);
            return true;
        }
        return false;
    }
    inline User getUser(const std::string& name){      //getUser: retrieves user information from database.
        using namespace std::literals::string_literals;
        CROW_LOG_DEBUG << name;
        User user{};
        std::string s{
            ("SELECT * "s)+
            ("FROM Users "s)+
            ("WHERE username = '"s)+name+("';"s)
        };
        accessDB(s, [&user](sqlite3_stmt* statement){
            const int id{sqlite3_column_int(statement, 0)};
            const std::string uname{reinterpret_cast<const char*>(sqlite3_column_text(statement, 1))};
            const std::string passw{reinterpret_cast<const char*>(sqlite3_column_text(statement, 2))};
            const std::string email{reinterpret_cast<const char*>(sqlite3_column_text(statement, 3))};
            const Trait traits{sqlite3_column_int(statement, 4)};
            user.setUid(id).setName(uname).setPass(passw).setEmail(email);
        });
        return user;
    }

    inline User getUser(const int id){      //getUser: retrieves user information from database.
        using namespace std::literals::string_literals;
        CROW_LOG_DEBUG << id;
        User user{};
        std::string s{
            ("SELECT * "s)+
            ("FROM Users "s)+
            ("WHERE uid = "s)+std::to_string(id)+(";"s)
        };
        accessDB(s, [&user](sqlite3_stmt* statement){
            const int id{sqlite3_column_int(statement, 0)};
            const std::string uname{reinterpret_cast<const char*>(sqlite3_column_text(statement, 1))};
            const std::string passw{reinterpret_cast<const char*>(sqlite3_column_text(statement, 2))};
            const std::string email{reinterpret_cast<const char*>(sqlite3_column_text(statement, 3))};
            const Trait traits{sqlite3_column_int(statement, 4)};
            user.setUid(id).setName(uname).setPass(passw).setEmail(email);
        });
        return user;
    }

    inline bool addUser(const User& user){
    using namespace std::literals::string_literals;
        std::string s{
            ("INSERT INTO Users VALUES ("s) +
            std::to_string(user.uid()) + ", '"s +
            user.name() + "', '"s +
            user.pass() + "', '"s +
            user.email() + "',"s +
            std::to_string(user.pref()) + ");"s
        };
        return accessDB(s, [](sqlite3_stmt*){});
    }

    inline bool deleteUser(const User& user){
    using namespace std::literals::string_literals;
        std::string s{("DELETE FROM Users WHERE uid = '"s) + std::to_string(user.uid()) + "';"s};
        return accessDB(s, [](sqlite3_stmt*){});
    }

    inline bool addItem(const User& user, const Item& item){
        CROW_LOG_DEBUG << "Adding item to db.";
    using namespace std::literals::string_literals;
        std::string s{
            ("INSERT INTO UserItems VALUES ("s) +
            std::to_string(user.uid()) + ", "s +
            std::to_string(item.id()) + ");"s
        };
        return accessDB(s, [](sqlite3_stmt*){});
    }

    inline std::vector<int> getItemList(const User& user){
    using namespace std::literals::string_literals;
        std::vector<int> items{};
        std::string s{
            ("SELECT iid "s)+
            ("FROM UserItems "s)+
            ("WHERE uid = "s)+std::to_string(user.uid())+(";"s)
        };
        accessDB(s, [&items](sqlite3_stmt* statement){
            items.push_back(sqlite3_column_int(statement, 0));
        });
        return items;
    }

    inline Item getItem(const std::string& name){
    using namespace std::literals::string_literals;
        Item item{};
        std::string s{
            ("SELECT * "s)+
            ("FROM Ingredients "s)+
            ("WHERE name = '"s)+name+("';"s)
        };
        accessDB(s, [&item](sqlite3_stmt* statement){
            const int id{sqlite3_column_int(statement, 0)};
            const std::string name{reinterpret_cast<const char*>(sqlite3_column_text(statement, 1))};
            const std::string desc{reinterpret_cast<const char*>(sqlite3_column_text(statement, 2))};
            const std::string type{reinterpret_cast<const char*>(sqlite3_column_text(statement, 3))};
            //const Trait iTraits{sqlite3_column_int(statement, 4)};
            item.setId(id).setName(name).setDesc(desc).setType(type);
        });
        return item;
    }

    inline Item getItem(int id){
    using namespace std::literals::string_literals;
        Item item{};
        std::string s{
            ("SELECT * "s)+
            ("FROM Ingredients "s)+
            ("WHERE id = '"s)+std::to_string(id)+("';"s)
        };
        accessDB(s, [&item](sqlite3_stmt* statement){
            const int id{sqlite3_column_int(statement, 0)};
            const std::string name{reinterpret_cast<const char*>(sqlite3_column_text(statement, 1))};
            const std::string desc{reinterpret_cast<const char*>(sqlite3_column_text(statement, 2))};
            const std::string type{reinterpret_cast<const char*>(sqlite3_column_text(statement, 3))};
            //const Trait iTraits{sqlite3_column_int(statement, 4)};
            item.setId(id).setName(name).setDesc(desc).setType(type);
        });
        return item;
    }
}

#endif
