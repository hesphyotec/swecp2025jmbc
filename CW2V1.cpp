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



typedef std::vector<std::tuple<std::string, std::string, double>> recommendVec;
typedef std::vector<std::pair<std::string,std::string>> pairVec;
char* errmsg = nullptr;

class CreateWord2Vec2 {// Creates vectors
	private:
		std::string file = "word2vec.txt";
		sqlite3 * db;

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
		int totCount = 0;
		for (const auto &single : input) {
			std::stringstream ss(single);
			std::string item;
			int pos = single.find_last_of(',');

			item = single.substr(0,pos);
			item.erase(0, item.find_first_not_of(" \t\n\r"));
			item.erase(item.find_last_not_of(" \t\n\r") + 1);// Trim leading/trailing spaces
			if (!item.empty())
				output.push_back(item);
		}
		return output;
	}

	static int callback(void *ingredientList, int columns, char **columnValue, char **colName) {
		auto* results = static_cast<pairVec*>(ingredientList);//takes the list of pairs
		results->push_back({columnValue[0], columnValue[1]}); //and pushes the results found in them to the back of the vector
		return 0; // Return 0 to continue processing rows, non-zero to stop
	}

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

	std::vector<double> search(const std::string& wordToSearch) { //function to search word2vec.txt and return doubles
		std::vector<double> vectors; //creating a list of vectors
		std::ifstream ifs(file); //opening the file
		std::string line;
		int len = wordToSearch.length();//this is used as end point for later
		std::string tempString;

		if (!ifs.is_open()) {
			return vectors;
		}

		std::cout << "Opened!\n";

		while (getline(ifs, line)) {//in the file, get all lines
			if (line.find(wordToSearch)!=std::string::npos) {//if line contains the word to search
				if (line.substr(0, (len)) == wordToSearch && line[len] == ' ') {//if the next character after the length of the word to find from index 0 is a space, you have an exact match
					std::cout << "Found " << line.substr(0, (len)) << "!\n";
					int start = len+1;
					int end = start+1;
					while (end < line.size()) {//this searches all vectors in that line
						while (line[end] != ' ' && end < line.size()) {//this finds each individual vector
							end++;
						}
						vectors.push_back(std::stod(line.substr(start, end)));//this puts each individual vector into the list
						start = end+1;
						end = start +1;
					}
					break;
				}
			}
			if (vectors.empty()) {
				return {0.0};
			}
		}
		return vectors;
	}

	double vectorSimplify (const std::vector<double>& vector) {//this takes the vectors and makes them into a mean value
		double sum = 0.0;
		for (const double& element: vector) {// THIS DOES NOT WORK
			sum += element;
		}
		return sum/300;//there are 300 vectors per word
	}

	std::string searchString(const std::string& wordToSearch) { //function to search word2vec.txt and return doubles
			std::string vectors = ""; //creating a list of vectors
			std::ifstream ifs(file); //opening the file
			std::string line;
			int len = wordToSearch.length();//this is used as end point for later
			std::string tempString;

			if (!ifs.is_open()) {
				std::cout << "word2vec not open";
				return vectors;
			}

			std::cout << "Opened!\n";

			while (getline(ifs, line)) {//in the file, get all lines
				if (line.find(wordToSearch)!=std::string::npos) {//if line contains the word to search
					if (line.substr(0, (len)) == wordToSearch && line[len] == ' ') {//if the next character after the length of the word to find from index 0 is a space, you have an exact match
						std::cout << "Found " << line.substr(0, (len)) << "!\n";
						int start = len+1;
						int end = start+1;
						while (end < line.size()) {//this searches all vectors in that line
							while (line[end] != ' ' && end < line.size()) {//this finds each individual vector
								end++;
							}
							std::string temp = line.substr(start, end) + " ";
							vectors.append(temp);//this puts each individual vector into the list
							start = end+1;
							end = start +1;
						}
						break;
					}
				}
			}
			if (vectors.empty()) {
				return "0.0 ";
			}
			vectors.pop_back();
			return vectors;
		}

	std::string toKeyword(std::string& sqlIngredient) {//this turns the values in the db to the form they're in, in the word2vec file
		std::ranges::transform(sqlIngredient,sqlIngredient.begin(), ::tolower);
		sqlIngredient[0] = std::toupper(sqlIngredient[0]); //Only needed sometimes
		sqlIngredient.erase(std::remove(sqlIngredient.begin(), sqlIngredient.end(), '-'), sqlIngredient.end());
		sqlIngredient.erase(std::remove(sqlIngredient.begin(), sqlIngredient.end(), ','), sqlIngredient.end());
		for (char &character : sqlIngredient) {
			if (std::isspace(static_cast<unsigned char>(character))) {
				character = '_'; // Replace spaces with underscore
			}
		}
		return sqlIngredient;
	}

	void getResults (pairVec& results) {//this puts vectors into the db
		sqlite3_stmt* stmt;
		const char* sql = "UPDATE Ingredients SET vectors = ? WHERE id = ?;";
		sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);//This preps the statement to have values added to it
			for (auto& [id, name] : results) {
			try {
				std::string got = searchString(toKeyword(name));//for all names do this

				if (got == "0.0") {
					throw name;
				}
				const char* idC = id.c_str();

				sqlite3_bind_text(stmt, 1, got.c_str(), -1, SQLITE_TRANSIENT);
				sqlite3_bind_text(stmt, 2, idC, -1, SQLITE_TRANSIENT);

				if (sqlite3_step(stmt) != SQLITE_DONE) {std::cerr << "Insert failed: " << sqlite3_errmsg(db) << std::endl;} //This preforms the insert
				else{std::cout << "Inserted vector for " << id <<"\n";}
			}
			catch (const std::exception& e) {
				std::cerr << "Uh Oh. Error " << e.what() << " occured on key " << id << "\n";
			}
			catch (...) {
					std::cerr << "Uh Oh. Unexpected error occured on key " << id << "\n";
			}
				sqlite3_reset(stmt); // Reset statement for next row
		}
			sqlite3_finalize(stmt);
	}

	void start() {//super easy way to use this class
			if (!db) {
				std::cerr << "Database not opened. Exiting start().\n";
				return;
			}
			pairVec results;
			sqlite3_exec(db, "SELECT id, name FROM Ingredients WHERE vectors IS NULL OR LENGTH(vectors) = 4", callback, &results, &errmsg);
			getResults(results);
			std::cout << "Done! \n";
			sqlite3_close(db);
		}

	void check() {//super easy way to use this class
			if (!db) {
				std::cerr << "Database not opened. Exiting start().\n";
				return;
			}
			pairVec results;
			sqlite3_exec(db, "SELECT name FROM Ingredients WHERE LENGTH(vectors) > 270000", callback, &results, &errmsg);
			for (const auto& item : results) {
				std::cout << item.first << "\n";
			}
			std::cout << "Done! \n";
			sqlite3_close(db);
		}

	std::vector<std::string> getMeals() {
			std::vector<std::string> output;
			sqlite3_exec(db, "SELECT id FROM Recipes WHERE vector IS NULL", callbackMeals, &output,  &errmsg);
			return output;
		}

	std::vector<std::pair<std::string, std::vector<std::string>>> getIngredient(std::vector<std::string> meals){
		sqlite3_stmt *stmt;
		std::string result;
		std::vector<std::pair<std::string, std::vector<std::string>>> output;

		for (const auto& recipe : meals) {
			const char *sql = "SELECT ingredients FROM Recipes WHERE id = ?;";
			sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr); //This preps the statement to have values added to it
			sqlite3_bind_text(stmt, 1, recipe.c_str(), -1, nullptr);

			sqlite3_step(stmt);
			result = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)); //add that vector value to result

			sqlite3_reset(stmt); // Reset statement for next row //call db for vector value

			std::vector<std::string> temp = individualIngredients(seperateIngredients(result));
			output.push_back({recipe, temp});
		}
		return output;
	}

	void vectorify (std::vector<std::pair<std::string, std::vector<std::string>>> input) {
			sqlite3_stmt* stmt;
			for (const auto &items : input) {
				double sum = 0.0;
				double avg = 0.0;
				for (const auto &list : items.second) {
					const char *sql = "SELECT vectors FROM Ingredients WHERE name = ? COLLATE NOCASE;";
					sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr); //This preps the statement to have values added to it
					sqlite3_bind_text(stmt, 1, list.c_str(), -1, nullptr);

					sqlite3_step(stmt);
					sum += sqlite3_column_double(stmt, 0); //add that vector value to result
					avg +=1;

					sqlite3_reset(stmt); // Reset statement for next row //call db for vector value
				}
				double finalVec = sum/avg;
				if (finalVec != 0) {
					const char* sql = "UPDATE Recipes SET vector = ? WHERE id = ?;";
					sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

					sqlite3_bind_double(stmt, 1, finalVec);
					sqlite3_bind_text(stmt, 2, (items.first).c_str(), -1, nullptr);

					sqlite3_step(stmt);
					sqlite3_reset(stmt);
				}
			}
		}

	void vectorInMeals() {
			vectorify(getIngredient(getMeals()));
		}
};



int main() {
	CreateWord2Vec2 create;
	create.check();

	return 0;
}