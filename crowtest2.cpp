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

class User{
private:
    int m_uid{};
	std::string m_name{};
	std::string m_pass{};
	std::string m_email{};

public:
	User(int uid = -1, std::string name = "Guest", std::string pass = "", std::string email = ""):
	m_uid{uid}, m_name{name}, m_pass{pass}, m_email{email}
	{}

	int uid() const {return m_uid;}
	std::string name() const {return m_name;}
	std::string pass() const {return m_pass;}
	std::string email() const {return m_email;}

	User& setUid(int id){
        m_uid = id;
        return *this;
	}
	User& setPass(std::string_view pass){
        m_pass = pass;
        return *this;
    }
    User& setEmail(std::string_view email){
        m_email = email;
        return *this;
    }
	User& setName(std::string_view n){
		m_name = n;
		return *this;
	}
};

void userConnect(const User& user){
    std::cout << "User connected.\n Name: " << user.name() << "\n Email: " << user.email() << "\n User ID: " << user.uid() << "\n";
}

User getUserSQL(const std::string& name){
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
            //std::cout << "SQLITE DONE: " << SQLITE_DONE << "\n"; 101
            //std::cout << "SQLITE ROW: " << SQLITE_ROW << "\n"; 100
            if (res == SQLITE_ROW){
                const std::string uname{reinterpret_cast<const char*>(sqlite3_column_text(statement, 1))};
                const std::string passw{reinterpret_cast<const char*>(sqlite3_column_text(statement, 2))};
                const std::string email{reinterpret_cast<const char*>(sqlite3_column_text(statement, 3))};
                const int id{sqlite3_column_int(statement, 0)};
                user.setUid(id).setName(uname).setPass(passw).setEmail(email);
                //std::cout << "Name: " << sqlite3_column_text(getAll, 1) << " | " << "SSN: " << sqlite3_column_int(getAll, 0) << "\n";
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

void addUserSql(const User& user){
using namespace std::literals::string_literals;
    sqlite3* db{};
    if(sqlite3_open("core.db", &db) == SQLITE_OK){
        sqlite3_stmt* statement{};
        const char* tail{};
        std::string s{
        ("INSERT INTO Users VALUES ('"s) +
        std::to_string(user.uid()) + "', '"s +
        user.name() + "', '"s +
        user.pass() + "', '"s +
        user.email() + "');"s
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
            if (res = SQLITE_DONE){
                std::cout << "Insert Complete!\n";
            } else {
                std::cerr << "Bad step. Code: " << res << "\n";
            }
        }
        sqlite3_close(db);
    }
}

void deleteUserSql(const User& user){
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

std::string url_decode(const std::string& str){
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

std::unordered_map<std::string, std::string> url_parse(const std::string& str){
    std::unordered_map<std::string, std::string> result{};
    std::istringstream iss(str);
    std::string token{};

    while(std::getline(iss, token, '&')){
        auto delimiter_pos = token.find('=');
        if(delimiter_pos != std::string::npos){
            std::string key = url_decode(token.substr(0, delimiter_pos));
            std::string value = url_decode(token.substr(delimiter_pos+1));
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
            //std::cout << "SQLITE DONE: " << SQLITE_DONE << "\n"; 101
            //std::cout << "SQLITE ROW: " << SQLITE_ROW << "\n"; 100
            if (res == SQLITE_ROW){
                id = sqlite3_column_int(statement, 0);
                //std::cout << "Name: " << sqlite3_column_text(getAll, 1) << " | " << "SSN: " << sqlite3_column_int(getAll, 0) << "\n";
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
        std::unordered_map<std::string, std::string> body = url_parse(req.body);

        //std::cout << body << "\n";

        std::string username = body["username"];
        std::string email = body["email"];
        std::string pass = body["psw"];

        std::cout << username << " | " << email << " | " << pass << "\n";
        //std::cout << req.url_params.get("username") << " : " << req.url_params.get("email") << " : " << req.url_params.get("psw") << "\n";
        std::cout << req.body << "\n";
        std::cout << req.get_header_value("Content-Type") << "\n";

        if (username.empty() || email.empty() || pass.empty()){
            return crow::response(400, "Username or Password cannot be empty.");
        }

        User newUser{genId(), username, pass, email};
        addUserSql(newUser);
        std::string result = "User sign up completed for: " + newUser.name();

        return crow::response(result);
    });

	CROW_ROUTE(app, "/<string>")([](std::string name){
		auto page = crow::mustache::load("testpage.html");
		User activeUser{getUserSQL(name)};
		userConnect(activeUser);
		std::string username {activeUser.name()};
		crow::mustache::context ctx ({{"person", username}});
		return page.render(ctx);
	});


	app.port(18080).multithreaded().run();
	return 0;
}
