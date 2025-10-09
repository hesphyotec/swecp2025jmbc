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

#include "bcrypt.h"
#include "sqlite3.h"
#include "crow.h"
#include "bgezuser.h"
#include "bgeztraits.h"
#include "bgezdb.h"


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
    std::string s{"SELECT MAX(uid) FROM Users "};
    DBArgList arg{};
    DBCore::accessDB(s, arg, [&](sqlite3_stmt* stmt){
        id = sqlite3_column_int(stmt, 0);
    });
    return ++id;
}

int main(){
	crow::SimpleApp app;
	std::mutex mtx;
	std::unordered_set<crow::websocket::connection*> users;

	app.loglevel(crow::LogLevel::Debug);

	CROW_ROUTE(app, "/")([](){
		auto page = crow::mustache::load_text("testpage.html");
		char name[256];
		gethostname(name, 256);
		CROW_LOG_INFO << name;
		return page;
	});

    CROW_WEBSOCKET_ROUTE(app, "/signup")
    .onopen([&](crow::websocket::connection& conn){
        CROW_LOG_INFO << "new websocket connection from " << conn.get_remote_ip();
        std::lock_guard<std::mutex> _(mtx);
        users.insert(&conn);
    })
    .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        std::lock_guard<std::mutex> _(mtx);
        //int id = std::stoi(data);
        CROW_LOG_DEBUG << "Received Data: " << data;

        crow::json::rvalue parsed;

        parsed = crow::json::load(data);

        if (!parsed.has("username") || !parsed.has("password")){
            conn.send_text("Missing username or password");
            return;
        }

        std::string username{parsed["username"].s()};
        std::string password{parsed["password"].s()};
        std::string h_pass{bcrypt::generateHash(password.c_str())};

        if (username.empty() || h_pass.empty()){
            conn.send_text("{\"status\":\"error\",\"message\":\"Username or password empty\"}");
            return;
        }
        if (DBCore::getUser(username).uid() != -1){
            conn.send_text("{\"status\":\"error\",\"message\":\"User already exists.\"}");
            return;
        }

        User newUser{genId(), username, h_pass};
        if (!DBCore::addUser(newUser)){
            conn.send_text("{\"status\":\"error\",\"message\":\"Server failed to create user.\"}");
            return;
        }

        crow::json::wvalue response;

        response["status"] = "success";
        response["id"] = newUser.uid();
        response["name"] = newUser.name();

        conn.send_text(response.dump());
    })
    .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t){
        CROW_LOG_INFO << "WS Connection closed: " << reason;
        std::lock_guard<std::mutex> _(mtx);
        users.erase(&conn);
    });

    CROW_WEBSOCKET_ROUTE(app, "/login")
    .onopen([&](crow::websocket::connection& conn){
        CROW_LOG_INFO << "new websocket connection from " << conn.get_remote_ip();
        std::lock_guard<std::mutex> _(mtx);
        users.insert(&conn);
    })
    .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        std::lock_guard<std::mutex> _(mtx);
        //int id = std::stoi(data);
        crow::json::rvalue parsed;

        parsed = crow::json::load(data);

        User activeUser = DBCore::getUser(parsed["username"].s());
        if (activeUser.uid() == -1){
            conn.send_text("{\"status\":\"error\",\"message\":\"Invalid Username or Password.\"}");
            return;
        }
        if (!bcrypt::validatePassword(parsed["password"].s(), activeUser.pass())){
            conn.send_text("{\"status\":\"error\",\"message\":\"Invalid Username or Password.\"}");
            return;
        }

        crow::json::wvalue response;

        response["status"] = "success";
        response["id"] = activeUser.uid();
        response["name"] = activeUser.name();

        conn.send_text(response.dump());
    })
    .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t){
        CROW_LOG_INFO << "WS Connection closed: " << reason;
        std::lock_guard<std::mutex> _(mtx);
        users.erase(&conn);
    });

    CROW_WEBSOCKET_ROUTE(app, "/home")
    .onopen([&](crow::websocket::connection& conn){
        CROW_LOG_INFO << "new websocket connection from " << conn.get_remote_ip();
        std::lock_guard<std::mutex> _(mtx);
        users.insert(&conn);
    })
    .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        std::lock_guard<std::mutex> _(mtx);
        //int id = std::stoi(data);
        User activeUser = DBCore::getUser(std::stoi(data));
        if (is_binary){
            conn.send_binary(activeUser.name());
        } else {
            conn.send_text(activeUser.name());
        }
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
        //int id = std::stoi(data);
        CROW_LOG_DEBUG << "Received Data: " << data;

        crow::json::rvalue parsed;

        parsed = crow::json::load(data);
        if (parsed.has("op")){
            if (parsed["op"] == "additem"){
                if (!parsed.has("uid")){
                    conn.send_text("{\"status\":\"error\",\"message\":\"Please sign in\"}");
                    return;
                }

                if (!parsed.has("name")){
                    conn.send_text("{\"status\":\"error\",\"message\":\"Name cannot be empty\"}");
                    return;
                }

                std::string name{parsed["name"].s()};
                int uid{static_cast<int>(parsed["uid"].i())};

                Item item = DBCore::getItem(name);
                if (item.id() == -1){
                    conn.send_text("{\"status\":\"error\",\"message\":\"Ingredient does not exist.\"}");
                    return;
                }

                User activeUser = DBCore::getUser(uid);
                try {
                    CROW_LOG_DEBUG << DBCore::addItem(activeUser, item);
                } catch (const std::exception& e) {
                    CROW_LOG_ERROR << "Error adding item to inventory: " << e.what();
                    conn.send_text("{\"status\":\"error\",\"message\":\"Server error adding item.\"}");
                    return;
                }
                crow::json::wvalue response = item.toJson();
            } else if (parsed["op"] == "getlist"){
                int uid{static_cast<int>(parsed["uid"].i())};
                User activeUser = DBCore::getUser(uid);
                crow::json::wvalue response = DBCore::getItemList(activeUser);
                response["status"] = "success";
                conn.send_text(response.dump());
            } else if (parsed["op"] == "delitem"){
                int uid{static_cast<int>(parsed["uid"].i())};
                int iid{static_cast<int>(parsed["iid"].i())};
                User activeUser = DBCore::getUser(uid);
                Item toDelete = DBCore::getItem(iid);
                if (DBCore::deleteItem(toDelete)){
                    crow::json::wvalue response = DBCore::getItemList(activeUser);
                    response["status"] = "success";
                    conn.send_text(response.dump());
                } else {
                    CROW_LOG_ERROR << "Error deleting item from inventory: ";
                    conn.send_text("{\"status\":\"error\",\"message\":\"Server error removing item.\"}");
                }
            }
        }
    })
    .onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t){
        CROW_LOG_INFO << "WS Connection closed: " << reason;
        std::lock_guard<std::mutex> _(mtx);
        users.erase(&conn);
    });

	app.port(18080).multithreaded().run();
	return 0;
}
