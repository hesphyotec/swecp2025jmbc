#include<iostream>
#include"sqlite3.h"

int main(){
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
                std::cout << "Name: " << sqlite3_column_text(getAll, 1) << " | " << "SSN: " << sqlite3_column_int(getAll, 0) << "\n";
            } else if (res = SQLITE_DONE){
                std::cout << "Query Complete!\n";
            } else {
                std::cerr << "Bad step. Code: " << res << "\n";
            }
        }
        sqlite3_close(db);
    }
    //std::cout << "Hello World!\n";
    return 0;
}
