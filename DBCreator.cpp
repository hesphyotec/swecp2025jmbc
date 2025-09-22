#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include <string>
#include <sqlite3.h>
#include <sstream>

int main() {
    sqlite3 * db;
    sqlite3_open("C:/Users/blake/swecp2025jmbc/RecipeDatabases.sqlite", &db); //Calls DB
    FILE* fp =fopen("C:/Users/blake/Downloads/meals.json", "rb");
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO meals(id, name, category, area, instructions, image, ingredients, tags) VALUES(?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);//This preps the statement to have values added to it

    char readBuffer[65536];

    rapidjson::FileReadStream is (fp, readBuffer, sizeof(readBuffer)); //Opens JSON File

    // Parse the JSON data using a Document object
    rapidjson::Document doc;
    doc.ParseStream(is);

    // Close the file
    fclose(fp);



    // for (const auto& ingredient : doc["ingredients"].GetArray()) { //INGREDIENTS DB
    //     // rapidjson uses SizeType instead of size_t
    //     const char* id = ingredient["id"].GetString(); //ID is stored as a string
    //     const char* name = ingredient["name"].GetString();
    //     const char* description = ingredient.HasMember("description") && ingredient["description"].IsString()
    //         ? ingredient["description"].GetString(): "null"; //Check if description is null or a string
    //     const char* type = ingredient.HasMember("type") && ingredient["type"].IsString()
    //         ? ingredient["type"].GetString(): "null";
    //
    //     sqlite3_bind_text(stmt, 1, id, -1, SQLITE_TRANSIENT);
    //     sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
    //     sqlite3_bind_text(stmt, 3, description, -1, SQLITE_TRANSIENT);
    //     sqlite3_bind_text(stmt, 4, type, -1, SQLITE_TRANSIENT); //This replaces its respective question mark with a value
    //
    //     if (sqlite3_step(stmt) != SQLITE_DONE) {std::cerr << "Insert failed: " << sqlite3_errmsg(db) << std::endl;} //This preforms the insert
    //     sqlite3_reset(stmt); // Reset statement for next row
    // }
    //

    // for (const auto& category : doc["categories"].GetArray()) { //categories DB
    //     // rapidjson uses SizeType instead of size_t
    //     const char* cat = category["category"].GetString(); //ID is stored as a string
    //
    //     sqlite3_bind_text(stmt, 1, cat, -1, SQLITE_TRANSIENT); //This replaces its respective question mark with a value
    //
    //     if (sqlite3_step(stmt) != SQLITE_DONE) {std::cerr << "Insert failed: " << sqlite3_errmsg(db) << std::endl;} //This preforms the insert
    //     sqlite3_reset(stmt); // Reset statement for next row
    // }

    // for (const auto& area : doc["areas"].GetArray()) { //areas DB
    //     // rapidjson uses SizeType instead of size_t
    //     const char* are = area["area"].GetString(); //ID is stored as a string
    //
    //     sqlite3_bind_text(stmt, 1, are, -1, SQLITE_TRANSIENT); //This replaces its respective question mark with a value
    //
    //     if (sqlite3_step(stmt) != SQLITE_DONE) {std::cerr << "Insert failed: " << sqlite3_errmsg(db) << std::endl;} //This preforms the insert
    //     sqlite3_reset(stmt); // Reset statement for next row
    // }

    for (const auto& meals : doc["meals"].GetArray()) { //meals DB
        std::string temp;
        // rapidjson uses SizeType instead of size_t
        const char* id = meals["id"].GetString(); //ID is stored as a string
        const char* name = meals["name"].GetString();
        const char* category = meals["category"].GetString();
        const char* area = meals["area"].GetString();
        const char* instructions = meals["instructions"].GetString();
        const char* image = meals.HasMember("image") && meals["image"].IsString()
             ? meals["image"].GetString(): "null"; //Check if images is null or a string
        for (const auto& ingredients : meals["ingredients"].GetArray()) {//since ingredients itself is an array
            std::string ing = ingredients["ingredient"].GetString();
            std::string measurements = ingredients["measure"].GetString();
            temp.append(ing + " , " + measurements + "; "); //making it all one string
        }
        temp.erase(temp.length()-2);// removing the last semicolon and space
        const char* ingredients = temp.c_str();
        const char* tags = meals.HasMember("tags") && meals["tags"].IsString()
            ? meals["tags"].GetString(): "null";


        sqlite3_bind_text(stmt, 1, id, -1, SQLITE_TRANSIENT); //This replaces its respective question mark with a value
        sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, category, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, area, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, instructions, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, image, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 7, ingredients, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 8, tags, -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {std::cerr << "Insert failed: " << sqlite3_errmsg(db) << std::endl;} //This preforms the insert
        sqlite3_reset(stmt); // Reset statement for next row
    }

    sqlite3_close(db);
    return 0;
}