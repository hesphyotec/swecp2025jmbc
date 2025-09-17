#include "crow.h"
#include <iostream>
#include <string>
#include <iostream>
#include "sqlite3.h"
#include <cstring>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <iomanip>
#include <bitset>
#include "bcrypt.h"

using Trait = int;              //Traits of food. Each trait is identified by a particular bit in an integer.

enum class Traits : int{
    vegetarian    = 0x0001,
    vegan         = 0x0002,
    lowCarb       = 0x0004,
    keto          = 0x0008,
    trait5        = 0x0010,
    trait6        = 0x0020,
    trait7        = 0x0040,
    trait8        = 0x0080,
    trait9        = 0x0100,
    trait10       = 0x0200,
    trait11       = 0x0400,
    trait12       = 0x0800,
    trait13       = 0x1000,
    trait14       = 0x2000,
    trait15       = 0x4000,
    trait16       = 0x8000
};

class User{                     //User class, used to store information about Users.
private:
    int m_uid{};
	std::string m_name{};
	std::string m_pass{};
	std::string m_email{};
	Trait m_preferences{};

public:
	User(int uid = -1, std::string name = "Guest", std::string pass = "", std::string email = ""):
	m_uid{uid}, m_name{name}, m_pass{pass}, m_email{email}
	{}

	int uid() const {return m_uid;}
	std::string name() const {return m_name;}
	std::string pass() const {return m_pass;}
	std::string email() const {return m_email;}
	Trait pref() const {return m_preferences;}

	User& setUid(const int id){
        m_uid = id;
        return *this;
	}
	User& setPass(const std::string_view pass){
        m_pass = pass;
        return *this;
    }
    User& setEmail(const std::string_view email){
        m_email = email;
        return *this;
    }
	User& setName(const std::string_view n){
		m_name = n;
		return *this;
	}
	User& setPref(const Trait& tr){
        m_preferences = tr;
        return *this;
	}
	bool checkPref(const Trait& tr){
        if(m_preferences & tr == tr){
            return true;
        }
        return false;
	}
};

void userConnect(const User& user){             //Mostly just a test function to show that user was found in database.
    std::cout << "User connected.\n Name: " << user.name() << "\n Email: " << user.email() << "\n User ID: " << user.uid() << "\n";
}

namespace DBCore{                               //List of functions for interacting with the DB.
    User getUser(const std::string& name){      //getUser: retrieves user information from database.
        std::cout << name << "\n";
        User user{};
        using namespace std::literals::string_literals;
        sqlite3* db{};
        if(sqlite3_open("core.db", &db) == SQLITE_OK){
            sqlite3_stmt* statement{};
            const char* tail{};
            std::string s{
            ("SELECT * "s)+
            ("FROM Users "s)+
            ("WHERE username = '"s)+name+("';"s)
            };
            std::cout << s << "\n";
            const char* st{s.c_str()};
            int prepstmt{sqlite3_prepare_v2(db, st, -1, &statement, &tail)};
            if (prepstmt != SQLITE_OK){
                std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
            }
            int res{};
            while(res != SQLITE_DONE){
                res = sqlite3_step(statement);
                if (res == SQLITE_ROW){
                    const int id{sqlite3_column_int(statement, 0)};
                    const std::string uname{reinterpret_cast<const char*>(sqlite3_column_text(statement, 1))};
                    const std::string passw{reinterpret_cast<const char*>(sqlite3_column_text(statement, 2))};
                    const std::string email{reinterpret_cast<const char*>(sqlite3_column_text(statement, 3))};
                    const Trait traits{sqlite3_column_int(statement, 0)};
                    user.setUid(id).setName(uname).setPass(passw).setEmail(email);
                } else if (res = SQLITE_DONE){
                    std::cout << "Query Complete!\n";
                } else {
                    std::cerr << "Bad step. Code: " << res << "\n";
                }
            }
            sqlite3_close(db);
        }
        return user;
    }

    bool addUser(const User& user){
    using namespace std::literals::string_literals;
        sqlite3* db{};
        if(sqlite3_open("core.db", &db) == SQLITE_OK){
            sqlite3_stmt* statement{};
            const char* tail{};
            std::string s{
            ("INSERT INTO Users VALUES ("s) +
            std::to_string(user.uid()) + ", '"s +
            user.name() + "', '"s +
            user.pass() + "', '"s +
            user.email() + "',"s +
            std::to_string(user.pref()) + ");"s
            };
            std::cout << s << "\n";
            const char* st{s.c_str()};
            int prepstmt{sqlite3_prepare_v2(db, st, -1, &statement, &tail)};
            if (prepstmt != SQLITE_OK){
                std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
                return false;
            }
            int res{};
            while(res != SQLITE_DONE){
                res = sqlite3_step(statement);
                if (res = SQLITE_DONE){
                    std::cout << "Insert Complete!\n";
                } else {
                    std::cerr << "Bad step. Code: " << res << "\n";
                    return false;
                }
            }
            sqlite3_close(db);
            return true;
        }
        return false;
    }

    void deleteUser(const User& user){
    using namespace std::literals::string_literals;
        sqlite3* db{};
        if(sqlite3_open("core.db", &db) == SQLITE_OK){
            sqlite3_stmt* statement{};
            const char* tail{};
            std::string s{("DELETE FROM Users WHERE uid = '"s) + std::to_string(user.uid()) + "';"s};
            std::cout << s << "\n";
            const char* st{s.c_str()};
            int prepstmt{sqlite3_prepare_v2(db, st, -1, &statement, &tail)};
            if (prepstmt != SQLITE_OK){
                std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
            }
            int res{};
            while(res != SQLITE_DONE){
                res = sqlite3_step(statement);
                if (res = SQLITE_DONE){
                    std::cout << "Delete Complete!\n";
                } else {
                    std::cerr << "Bad step. Code: " << res << "\n";
                }
            }
            sqlite3_close(db);
        }
    }
}
std::string urlDecode(const std::string& str){
    std::ostringstream decoded;
    for(int i = 0; i < str.length(); i++){
        if(str[i] == '%' && i + 2 < str.length()){
            std::istringstream iss{str.substr(i+1,2)};
            int hexChar{};
            if(iss >> std::hex >> hexChar){
                decoded << static_cast<char>(hexChar);
                i += 2;
            }
        } else if(str[i] == '+') {
            decoded << ' ';
        } else {
            decoded << str[i];
        }
    }
    return decoded.str();
}

std::unordered_map<std::string, std::string> urlParse(const std::string& str){
    std::unordered_map<std::string, std::string> result{};
    std::istringstream iss(str);
    std::string token{};

    while(std::getline(iss, token, '&')){
        auto delimiter_pos = token.find('=');
        if(delimiter_pos != std::string::npos){
            std::string key = urlDecode(token.substr(0, delimiter_pos));
            std::string value = urlDecode(token.substr(delimiter_pos+1));
            result[key] = value;
        }
    }
    return result;
}

int genId(){
    int id{};
    using namespace std::literals::string_literals;
    sqlite3* db{};
    if(sqlite3_open("core.db", &db) == SQLITE_OK){
        sqlite3_stmt* statement{};
        const char* tail{};
        std::string s{"SELECT MAX(uid) FROM Users "};
        std::cout << s << "\n";
        const char* st{s.c_str()};
        int prepstmt{sqlite3_prepare_v2(db, st, -1, &statement, &tail)};
        if (prepstmt != SQLITE_OK){
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
        }
        int res{};
        while(res != SQLITE_DONE){
            res = sqlite3_step(statement);
            if (res == SQLITE_ROW){
                id = sqlite3_column_int(statement, 0);
            } else if (res = SQLITE_DONE){
                std::cout << "Query Complete!\n";
            } else {
                std::cerr << "Bad step. Code: " << res << "\n";
            }
        }
        sqlite3_close(db);
    }
    return ++id;
}

int main(){
	crow::SimpleApp app;

	CROW_ROUTE(app, "/")([](){
		auto page = crow::mustache::load_text("testpage.html");
		return page;
	});

	CROW_ROUTE(app, "/signup").methods("GET"_method)([](){
		auto page = crow::mustache::load("signup.html").render();
		return crow::response(page);
	});

	CROW_ROUTE(app, "/signup").methods("POST"_method)([](const crow::request& req){
        std::unordered_map<std::string, std::string> body = urlParse(req.body);

        std::string username = body["username"];
        std::string email = body["email"];

        //char hash[61];
        //char salt[61];
        //bcrypt_gensalt(12, salt);
        //bcrypt_hashpw(body["psw"].c_str(), salt, hash);
        std::string h_pass{bcrypt::generateHash(body["psw"].c_str())};

        std::cout << username << " | " << email << " | " << h_pass << "\n";

        if (username.empty() || email.empty() || h_pass.empty()){
            return crow::response(400, "Username or Password cannot be empty.");
        }
        if (DBCore::getUser(username).uid() != -1){
            return crow::response(400, "User already exists");
        }

        User newUser{genId(), username, h_pass, email};
        if (!DBCore::addUser(newUser)){
            return crow::response(500, "Internal Server Error. Account creation failed.");
        }
        std::string result = "User sign up completed for: " + newUser.name();

        return crow::response(result);
    });

	CROW_ROUTE(app, "/<string>")([](std::string name){
		auto page = crow::mustache::load("testpage.html");
		User activeUser{DBCore::getUser(name)};
		userConnect(activeUser);
		std::string username {activeUser.name()};
		crow::mustache::context ctx ({{"person", username}});
		return page.render(ctx);
	});

	app.port(18080).multithreaded().run();
	return 0;
}
