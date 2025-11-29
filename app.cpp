#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#endif
#include <iostream>
#include <string>
#include <iostream>
#include <string>
#include <iostream>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <iomanip>
#include <bitset>
#include <mutex>
#include <unordered_set>

#include "my_bcrypt.h"
#include "sqlite3.h"
#include "crow.h"
#include "bgezuser.h"
#include "bgeztraits.h"
#include "bgezdb.h"
#include "bgezrecs.h"


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

int main(){
	crow::SimpleApp app;
	std::mutex mtx;
	std::unordered_set<crow::websocket::connection*> users;

	app.loglevel(crow::LogLevel::Debug);

	CROW_ROUTE(app, "/")([](){ //test, no longer needed
		auto page = crow::mustache::load_text("testpage.html");
		char name[256];
		gethostname(name, 256);
		CROW_LOG_INFO << name;
		return page;
	});

    CROW_WEBSOCKET_ROUTE(app, "/signup")
    .onopen([&](crow::websocket::connection& conn){ //connection to the signup page
        CROW_LOG_INFO << "new websocket connection from " << conn.get_remote_ip();
        std::lock_guard<std::mutex> _(mtx);
        users.insert(&conn);
    })
    .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary){
        std::lock_guard<std::mutex> _(mtx);
        DBConnection db;
        CROW_LOG_DEBUG << "Received Data: " << data;//JSON data from html

        crow::json::rvalue parsed;

        parsed = crow::json::load(data);

        if (!parsed.has("username") || !parsed.has("password")){
            conn.send_text("Missing username or password");
            return;
        }

        std::string username{parsed["username"].s()};
        std::string password{parsed["password"].s()};
        std::string h_pass{bcrypt::generateHash(password.c_str())};//this hashes the password for storage

        if (username.empty() || h_pass.empty()){ //if no password or username
            conn.send_text("{\"status\":\"error\",\"message\":\"Username or password empty\"}");
            return;
        }
        if (DBCore::getUser(username, db).uid() != -1){ //if username already exists
            conn.send_text("{\"status\":\"error\",\"message\":\"User already exists.\"}");
            return;
        }

        User newUser{0, username, h_pass};
        if (!DBCore::addUser(newUser, db)){  //random failure
            conn.send_text("{\"status\":\"error\",\"message\":\"Server failed to create user.\"}");
            return;
        }

        crow::json::wvalue response;
        User activeUser = DBCore::getUser(parsed["username"].s(), db);

        response["status"] = "success";
        response["id"] = activeUser.uid();//set uid from active userID
        response["name"] = newUser.name();//set name from active user name

        conn.send_text(response.dump());
        db.~DBConnection();
    })
    .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t){
        CROW_LOG_INFO << "WS Connection closed: " << reason;
        std::lock_guard<std::mutex> _(mtx);
        users.erase(&conn);//close websocket and kill connection
    });

    CROW_WEBSOCKET_ROUTE(app, "/login") //connection to login page
    .onopen([&](crow::websocket::connection& conn){
        CROW_LOG_INFO << "new websocket connection from " << conn.get_remote_ip();
        std::lock_guard<std::mutex> _(mtx);
        users.insert(&conn);
    })
    .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        std::lock_guard<std::mutex> _(mtx);
        DBConnection db;
        crow::json::rvalue parsed;

        parsed = crow::json::load(data);//loads sent data from html

        User activeUser = DBCore::getUser(parsed["username"].s(), db);
        if (activeUser.uid() == -1){//username does not exist
            conn.send_text("{\"status\":\"error\",\"message\":\"Invalid Username or Password.\"}");
            return;
        }
        if (!bcrypt::validatePassword(parsed["password"].s(), activeUser.pass())){ //password is incorrect
            conn.send_text("{\"status\":\"error\",\"message\":\"Invalid Username or Password.\"}");
            return;
        }

        crow::json::wvalue response;

        response["status"] = "success";
        response["id"] = activeUser.uid();
        response["name"] = activeUser.name();

        conn.send_text(response.dump());
        db.~DBConnection();//manually call destructor, this forces db connection to stop
    })
    .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t){
        CROW_LOG_INFO << "WS Connection closed: " << reason;
        std::lock_guard<std::mutex> _(mtx);
        users.erase(&conn);
    });

    CROW_WEBSOCKET_ROUTE(app, "/home")//connection to home page
    .onopen([&](crow::websocket::connection& conn){
        CROW_LOG_INFO << "new websocket connection from " << conn.get_remote_ip();
        std::lock_guard<std::mutex> _(mtx);
        users.insert(&conn);
    })
    .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        std::lock_guard<std::mutex> _(mtx);
        DBConnection db;
        User activeUser = DBCore::getUser(std::stoi(data), db); //checks if user is active
        if (is_binary){
            conn.send_binary(activeUser.name());
        } else {
            conn.send_text(activeUser.name());
        }
        db.~DBConnection();
    })
    .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t){
        CROW_LOG_INFO << "WS Connection closed: " << reason;
        std::lock_guard<std::mutex> _(mtx);
        users.erase(&conn);
    });

    CROW_WEBSOCKET_ROUTE(app, "/inventory")
    .onopen([&](crow::websocket::connection& conn){
        CROW_LOG_INFO << "new websocket connection from " << conn.get_remote_ip();
        std::lock_guard<std::mutex> _(mtx);
        users.insert(&conn);
    })
    .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        std::lock_guard<std::mutex> _(mtx);
        DBConnection db;
        CROW_LOG_DEBUG << "Received Data: " << data;

        crow::json::rvalue parsed;

        parsed = crow::json::load(data);
        if (parsed.has("op")){ //checks if sent data has "op" code
            if (parsed["op"] == "additem"){
                if (!parsed.has("uid")){ //not signed in
                    conn.send_text("{\"status\":\"error\",\"message\":\"Please sign in\"}");
                    return;
                }

                if (!parsed.has("name")){ //no ingredient to enter
                    conn.send_text("{\"status\":\"error\",\"message\":\"Name cannot be empty\"}");
                    return;
                }

                std::string name{parsed["name"].s()};
                int uid{static_cast<int>(parsed["uid"].i())};

                Item item = DBCore::getItem(name, db);
                if (item.id() == -1){ //ingredient does not exist
                    conn.send_text("{\"status\":\"error\",\"message\":\"Ingredient does not exist.\"}");
                    return;
                }

                try {
                    CROW_LOG_DEBUG << DBCore::addItem(uid, item, db);
                } catch (const std::exception& e) {
                    CROW_LOG_ERROR << "Error adding item to inventory: " << e.what();
                    conn.send_text("{\"status\":\"error\",\"message\":\"Server error adding item.\"}");
                    return;
                }
                crow::json::wvalue response = item.toJson();
            } else if (parsed["op"] == "getlist"){//this returns list of saved items by user
                int uid{static_cast<int>(parsed["uid"].i())};
                crow::json::wvalue response = DBCore::getItemList(uid, db);
                response["status"] = "success";
                conn.send_text(response.dump());
            } else if (parsed["op"] == "delitem"){//this deletes an item from user in db
                int uid{static_cast<int>(parsed["uid"].i())};
                int iid{static_cast<int>(parsed["iid"].i())};
                Item toDelete = DBCore::getItem(iid, db);
                if (DBCore::deleteItem(toDelete, db)){
                    crow::json::wvalue response = DBCore::getItemList(uid, db);
                    response["status"] = "success";
                    conn.send_text(response.dump());
                } else {
                    CROW_LOG_ERROR << "Error deleting item from inventory: ";
                    conn.send_text("{\"status\":\"error\",\"message\":\"Server error removing item.\"}");
                }
            }else if (parsed["op"] == "addReceipt"){//this is how we add receipt data
                    int uid{static_cast<int>(parsed["uid"].i())};
                    std::vector<std::string> ingredients{};
                    for(crow::json::rvalue& ing : parsed["ingredients"].lo()){//for loop to allow for multiple scanned items to be added
                        Item item = DBCore::getItem(ing.s(), db);
                        try {
                            CROW_LOG_DEBUG << DBCore::addItem(uid, item, db);
                        } catch (const std::exception& e) {
                            CROW_LOG_ERROR << "Error adding item to inventory: " << e.what();
                            conn.send_text("{\"status\":\"error\",\"message\":\"Server error adding item.\"}");
                            return;
                        }
                    }
            }
        }
        db.~DBConnection();
    })
    .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t){
        CROW_LOG_INFO << "WS Connection closed: " << reason;
        std::lock_guard<std::mutex> _(mtx);
        users.erase(&conn);
    });

    CROW_WEBSOCKET_ROUTE(app, "/recipes")
    .onopen([&](crow::websocket::connection& conn){
        CROW_LOG_INFO << "new websocket connection from " << conn.get_remote_ip();
        std::lock_guard<std::mutex> _(mtx);
        users.insert(&conn);
    })
    .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        std::lock_guard<std::mutex> _(mtx);
        DBConnection db;
        Recommend rec;

        crow::json::rvalue parsed;

        parsed = crow::json::load(data);

        if (parsed["op"] == "getrecipes"){//if op is to get recipes
            UserRecSys urs;
            int uid = parsed["uid"].i();

            CROW_LOG_DEBUG << "Retrieving Recipes";
            auto recipes = rec.doIt(uid, urs.userGather(uid));
            crow::json::wvalue result;
            result["status"] = "success";
            result["recipes"] = std::move(recipes);
            CROW_LOG_DEBUG << "SENDING";
            conn.send_text(result.dump());
            CROW_LOG_DEBUG << "SENT";
        }
        else if (parsed["op"] == "getInstructions") {
           std::string name = parsed["name"].s();
           CROW_LOG_DEBUG << "Getting Instructions";
           crow::json::wvalue instruction = rec.getInstructions(name);
           CROW_LOG_DEBUG << "Sending";
           conn.send_text(instruction.dump());
           CROW_LOG_DEBUG << "Sent";
        }
        else {
            CROW_LOG_DEBUG << "Malformed Operation";
        }
        db.~DBConnection();
    })
    .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t){
        CROW_LOG_INFO << "WS Connection closed: " << reason;
        std::lock_guard<std::mutex> _(mtx);
        users.erase(&conn);
    });

        CROW_WEBSOCKET_ROUTE(app, "/Account")
    .onopen([&](crow::websocket::connection& conn){
        CROW_LOG_INFO << "new websocket connection from " << conn.get_remote_ip();
        std::lock_guard<std::mutex> _(mtx);
        users.insert(&conn);
    })
    .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        std::lock_guard<std::mutex> _(mtx);
        DBConnection db;
        UserRecSys usr;
        crow::json::rvalue parsed;

        parsed = crow::json::load(data);
        if (parsed["op"] == "save" && parsed["uid"]) {//this saves prefs if user is logged in
            std::vector<std::string> saved {};
            int uid = parsed["uid"].i();
            for (auto& items: parsed["saved"]) {
                saved.push_back(items.s());
                CROW_LOG_DEBUG << "ITEM " << items.s();
            }
            usr.save(saved, uid);
        }
        if (parsed["op"] == "load" && parsed["uid"]) {//this loads prefs if user is logged in to keep prefs even after logout
            CROW_LOG_DEBUG << "Loading User Prefs";
            std::string loadPref;
            int uid = parsed["uid"].i();
            loadPref = usr.userPrefParser(uid);
            if (!loadPref.empty()) {
                CROW_LOG_DEBUG <<  "User has Prefs";
                std::vector<std::string> sendData;
                std::vector<int> tempVec;
                std::string s;
                std::istringstream iss(loadPref);
                while (std::getline(iss, s, ' ' ) ) {
                    tempVec.push_back(std::stoi(s));
                  }
                for (auto& pref : tempVec) {
                    sendData.push_back(traitToString(pref));
                }
                crow::json::wvalue send;
                send = sendData;
                std::string jsonOutput = send.dump();
                CROW_LOG_DEBUG << "Sending User Prefs " << jsonOutput;
                conn.send_text(jsonOutput);
            }
        }
        db.~DBConnection();
    })
    .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t){
        CROW_LOG_INFO << "Account WS Connection closed: " << reason;
        std::lock_guard<std::mutex> _(mtx);
        users.erase(&conn);
    });
	app.port(18080).multithreaded().run();
	return 0;
}