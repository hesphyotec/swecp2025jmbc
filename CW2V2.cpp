//
// Created by blake on 11/1/2025.
//
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#endif
#include "bgezdb.h"
#include "my_bcrypt.h"
#include <iostream>
#include <sqlite3.h>
#include <fstream>
#include <string>
#include <cmath>
#include <utility>
#include <vector>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <tuple>
#include <sstream>
#include <mutex>
#include <unordered_set>
#include "crow.h"
#include "asio.hpp"
#include "Word2vec-master/main.cpp"

class CreateWord2Vec2 {
// Creates vectors
private:
	std::string file = "word2vec.txt";
	sqlite3 * db;

	static int callbackMeals(void *ingredientList, int columns, char **columnValue, char **colName) {
		auto* results = static_cast<std::vector<std::string>*>(ingredientList);//takes the list of pairs
		results->push_back(columnValue[0]); //and pushes the results found in them to the back of the vector
		return 0; // Return 0 to continue processing rows, non-zero to stop
	}

public:
	CreateWord2Vec2() {
		std::cout << "started!\n";
		if (sqlite3_open("core.db", &db)!=SQLITE_OK) {
			std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
			db = nullptr;
		}
	}

	std::vector<std::string> seperateIngredients(std::string input){
		std::vector<std::string> output;
		std::stringstream ss(input);
		std::string item;

		while (std::getline(ss, item, ';')) {
			// Trim leading/trailing spaces
			item.erase(0, item.find_first_not_of(" \t\n\r"));
			item.erase(item.find_last_not_of(" \t\n\r") + 1);

			if (!item.empty())
				output.push_back(item);
		}
		return output;
	}

	std::vector<std::string> individualIngredients(std::vector<std::string> input){
		std::vector<std::string> output;
		for (const auto &single : input) {
			std::stringstream ss(single);
			std::string item;
			int pos = single.find_last_of(',');

			item = single.substr(0,pos);
			item.erase(0, item.find_first_not_of(" \t\n\r"));
			item.erase(item.find_last_not_of(" \t\n\r") + 1);// Trim leading/trailing spaces
			for (char &c : item) {
				c = std::tolower(static_cast<unsigned char>(c));
				if (std::isspace(static_cast<unsigned char>(c)) || c == '-') {
					c = '_';
				}
			}
			if (!item.empty())
				output.push_back(item);
		}
		std::ranges::sort(output);
		return output;
	}

	std::vector<std::string> getMeals() {
		char* errmsg = nullptr;
		std::vector<std::string> output;
		sqlite3_exec(db, "SELECT name FROM Recipes WHERE vector IS NULL", callbackMeals, &output,  &errmsg);
		return output;
	}

	void textFile(std::vector<std::string> meals) {
		sqlite3_stmt *stmt;
		std::string result;
		std::vector<std::pair<std::string, std::vector<std::string>>> output;

		for (auto &recipe : meals) {
			const char *sql = "SELECT ingredients FROM Recipes WHERE name = ?;";
			sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr); //This preps the statement to have values added to it
			sqlite3_bind_text(stmt, 1, recipe.c_str(), -1, nullptr);

			sqlite3_step(stmt);
			result = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)); //add that vector value to result

			sqlite3_reset(stmt); // Reset statement for next row //call db for vector value

			std::vector<std::string> temp = individualIngredients(seperateIngredients(result));

			for (char &c : recipe) {
				if (std::isspace(static_cast<unsigned char>(c))) {
					c = '_';
				}
			}

			output.push_back({recipe, temp});
		}
		std::ofstream outFile("output.txt");

		if (outFile.is_open()) {
			for (const auto& items : output) {
				outFile << items.first;
				for (const auto& ingre : items.second) {
					outFile << " " << ingre;
				}
				outFile << "\n";
			}
			outFile.close();
			std::cout << "Vector written to output.txt" << std::endl;
		}
		else {
			std::cerr << "Unable to open file" << std::endl;
		}
	}

	void textFix() {
		std::ifstream ifs("result"); //opening the file
		std::string line;

		if (!ifs.is_open()) {
			std::cout << "Not open.";
			return;
		}

		std::ofstream outFile("outputFix.txt");
		std::cout << "Opened!\n";
		if (outFile.is_open()) {
			while (getline(ifs, line)) {//in the file, get all lines
				for (char &c : line) {
					if (c == '_') {
						c = ' ';
					}
				}
				outFile << line << "\n";
			}
			outFile.close();
			std::cout << "Vector written to outputFix.txt" << std::endl;
		}
		else {
			std::cerr << "Unable to open file" << std::endl;
		}
	}

	void use() {
		if (!db) {
			std::cerr << "Database not opened. Exiting start().\n";
			return;
		}

		// textFile(getMeals());
		// Word2Vec word2Vec("output.txt");
		// word2Vec.init();
		// word2Vec.debugMode = 2;
		// word2Vec.numThreads = 4;
		// word2Vec.iterTimes = 10;
		// trainModel(word2Vec);
		// word2Vec.saveResult();
		// textFix();
		toVector(); //run this line and only this line twice, once for recipes and once for ingredients
	}

	void enterVector(const std::string& search, const std::string& vector) {
		sqlite3_stmt* stmt;
		const char* name = search.c_str();
		const char* check = "SELECT EXISTS(SELECT 1 FROM Ingredients WHERE name = ? COLLATE NOCASE);";
		sqlite3_prepare_v2(db, check, -1, &stmt, nullptr);
		sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
		sqlite3_step(stmt);
		if (sqlite3_column_int(stmt, 0)) {
			sqlite3_finalize(stmt);
			const char* sql = "UPDATE Ingredients SET vector = ? WHERE name = ? COLLATE NOCASE;";
			sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);//This preps the statement to have values added to it

			sqlite3_bind_text(stmt, 1, vector.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);

			if (sqlite3_step(stmt) != SQLITE_DONE) {std::cerr << "Insert failed: " << sqlite3_errmsg(db) << std::endl;} //This preforms the insert
			else{std::cout << "Inserted vector for " << search <<"\n";}
		}
		else {
			std::cerr << "Name not found: " << search << "\n";
		}
		sqlite3_reset(stmt); // Reset statement for next row
	}

	void toVector() {
		if (!db) {
			std::cerr << "Database not opened. Exiting start().\n";
			return;
		}
		std::ifstream ifs("outputFix.txt"); //opening the file
		std::string line;
		std::string name;
		int end = 0;

		if (!ifs.is_open()) {
			std::cout << "Not open.";
			return;
		}

		std::cout << "Opened!\n";
			while (getline(ifs, line)) {//in the file, get all lines
				for (char c : line){
					if (c == '-' || c =='0' || c =='1') {
						enterVector(line.substr(0,end-1),line.substr(end));
						end=0;
						break;
					}
					end++;
				}
			}
	}
};

int main() {
	CreateWord2Vec2 create;
	create.use();
	return 0;
}