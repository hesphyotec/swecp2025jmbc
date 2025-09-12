#include "crow.h"
#include <iostream>
#include <string>
#include <iostream>
#include "sqlite3.h"

class User{
private:
	std::string m_name{};
	int m_ssn{};
public:
	User(std::string name = "", int ssn = 0):
	m_name{name}, m_ssn{ssn}
	{}

	std::string name() const { return m_name;}
	int ssn() const { return m_ssn;}

	void setName(std::string n){
		m_name = n;
	}
	void setSsn(int s){
		m_ssn = s;
	} 
};

User getUserSQL(std::string_view name);


int main(){
	User user{};
	sqlite3* db{};
    if(sqlite3_open("testa.db", &db) == SQLITE_OK){
        std::cout << "DB Opened!\n";
        sqlite3_stmt* getAll{};
        const char* tail{};
        const char* getall{"SELECT * FROM Person;"};
        int prepstmt{sqlite3_prepare_v2(db, getall, -1, &getAll, &tail)};
        //std::cout << prepstmt << "\n";
        if (prepstmt != SQLITE_OK){
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
        }
        int res{};
        while(res != SQLITE_DONE){
            res = sqlite3_step(getAll);
            //std::cout << "SQLITE DONE: " << SQLITE_DONE << "\n"; 101
            //std::cout << "SQLITE ROW: " << SQLITE_ROW << "\n"; 100
            if (res == SQLITE_ROW){
            	std::string uname{reinterpret_cast<const char*>(sqlite3_column_text(getAll, 1))};
                user.setName(uname);
                user.setSsn(sqlite3_column_int(getAll, 0));
                //std::cout << "Name: " << sqlite3_column_text(getAll, 1) << " | " << "SSN: " << sqlite3_column_int(getAll, 0) << "\n";
            } else if (res = SQLITE_DONE){
                std::cout << "Query Complete!\n";
            } else {
                std::cerr << "Bad step. Code: " << res << "\n";
            }
        }
        sqlite3_close(db);
    }


	crow::SimpleApp app;

	CROW_ROUTE(app, "/")([](){
		auto page = crow::mustache::load_text("testpage.html");
		return page;
	});

	CROW_ROUTE(app, "/<string>")([user](std::string name){
		auto page = crow::mustache::load("testpage.html");
		std::string username {user.name()};
		crow::mustache::context ctx ({{"person", username}});
		return page.render(ctx);
	});

	app.port(18080).multithreaded().run();
	return 0;
}
