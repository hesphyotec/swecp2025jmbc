#ifndef RECS

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
#include "bgezdb.h"

typedef std::vector<std::tuple<std::string, std::string, double>> recommendVec;
typedef std::vector<std::pair<std::string,std::string>> pairVec;
typedef std::vector<std::vector<double>> vecVector;
char* errmsg = nullptr;


class UserRecSys {
private:
	sqlite3 * db;

	static int callbackIID(void *ingredientList, int columns, char **columnValue, char **colName) {//callback for userIngredientParser
		auto* results = static_cast<std::vector<int>*>(ingredientList);
		results->push_back(std::stoi(columnValue[0]));
		return 0; // Return 0 to continue processing rows, non-zero to stop
	}

public:
	UserRecSys() {
		std::cout << "Hello!\n";
		if (sqlite3_open("core.db", &db)!=SQLITE_OK) {
			std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
			db = nullptr;
		}
	}

	std::vector<int> userIngredientParser (int userID) {//gets all ingredients stored by user
			std::vector<int> ingredientID{};
			std::string sqlPre = "SELECT iid FROM UserItems WHERE uid=";
			std::string sqlFix = sqlPre + std::to_string(userID);
			const char* sql = sqlFix.c_str();
			sqlite3_exec(db, sql, callbackIID, &ingredientID,  &errmsg);
			return ingredientID;
		}

	std::vector<int> userMealParser (int userID) {//gets all meals stored by user
			std::vector<int> mealID{};
			std::string sqlPre = "SELECT mid FROM UserMeals WHERE uid=";
			std::string sqlFix = sqlPre + std::to_string(userID);
			const char* sql = sqlFix.c_str();
			sqlite3_exec(db, sql, callbackIID, &mealID,  &errmsg);
			return mealID;
		}

	vecVector ingredientToVector(const std::vector<int>& ingredientID) {
			//this takes the user ingredients and vector-ifys them
			vecVector ingredient;
			std::string temp = " ";
			std::vector<double> tempVec = {};

			if (ingredientID.empty()) { return {}; }
			sqlite3_stmt *stmt;

			for (const int &ingID: ingredientID) {
				const char *sql = "SELECT vector, tfidf FROM Ingredients WHERE id = ?;";
				sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr); //This preps the statement to have values added to it
				sqlite3_bind_int(stmt, 1, ingID);
				sqlite3_step(stmt);
				std::istringstream iss(reinterpret_cast<const char*>(sqlite3_column_text(stmt,0)));
				double tfidf = sqlite3_column_double(stmt, 1);

				while (std::getline(iss, temp, ' ' ) ) {
					tempVec.push_back((std::stod(temp))*(1.0-tfidf));
				}

		if (tempVec.size() != 1){ingredient.push_back({tempVec});}

				tempVec = {};
				sqlite3_reset(stmt); // Reset statement for next row //call db for vector value
			}

			return ingredient;
		}

	vecVector mealToVector(const std::vector<int>& mealID) {//this takes the user past meals and vector-ifys them
		vecVector meals;
		std::string temp = " ";
		std::vector<double> tempVec = {};

		if (mealID.empty()) { return {}; }
			sqlite3_stmt *stmt;

			for (const int &ingID: mealID) {
				const char *sql = "SELECT vector FROM Recipes WHERE id = ?;";
				sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr); //This preps the statement to have values added to it
				sqlite3_bind_int(stmt, 1, ingID);

				sqlite3_step(stmt);
				std::istringstream iss(reinterpret_cast<const char*>(sqlite3_column_text(stmt,0)));

				while (std::getline(iss, temp, ' ' ) ) {
					tempVec.push_back(std::stod(temp));
				}

				meals.push_back({tempVec});

				sqlite3_reset(stmt); // Reset statement for next row //call db for vector value
			}
			return meals;
		}

	vecVector outputVector (const vecVector ingredientVector, const vecVector mealVector) {
			std::vector<double> tempIngVec = {};
			std::vector<double> tempMealVec = {};
			vecVector returnVec;

			if (!ingredientVector.empty()) {
				tempIngVec.resize(100);
				for (auto& ingredients : ingredientVector) {
					for (int i = 0; i < ingredients.size(); i++) {
						tempIngVec[i] += ingredients[i];
					}
				}
				for (auto& value : tempIngVec) {
					value /= ingredientVector.size();
				}
			}

		returnVec.emplace_back(tempIngVec);
		if (!mealVector.empty()) {
			tempMealVec.resize(100);
			for (auto& meals : mealVector) {
				for (int i = 0; i < meals.size(); i++) {
					tempMealVec[i] += meals[i];
				}
			}
			for (auto& value : tempMealVec) {
				value /= mealVector.size();
			}
		}

		return returnVec;
		}

	vecVector userGather (int uID) {
			vecVector totVec;
			const vecVector ingVec = ingredientToVector(userIngredientParser(uID));
			const vecVector mealVec = mealToVector(userMealParser(uID));
			 for (auto& vector : ingVec) {
			 	totVec.push_back(vector);
			 }
			for (auto& vector : mealVec) {
				totVec.push_back(vector);
			}
			return totVec;
		}
};

class Recommend {
	private:
		sqlite3 * db;

	static int callbackKeyword(void *mealList, int columns, char **columnValue, char **colName) {//callback for keyword function
		auto* results = static_cast<std::vector<std::pair<std::string, std::string>>*>(mealList);
		results->push_back({columnValue[0],columnValue[1]});
		return 0; // Return 0 to continue processing rows, non-zero to stop
	}

	static int callbackEuclidean(void *output, int columns, char **columnValue, char **colName) {
		if (columnValue[0])
			*static_cast<double*>(output) = std::stod(columnValue[0]);
		return 0;
	}

	int partition(recommendVec &vec, int low, int high) {
		double pivot = std::get<2>(vec[high]);// Selecting last element as the pivot
		int i = (low - 1); // Index of elemment just before the last element it is used for swapping

		for (int j = low; j <= high - 1; j++) {// If current element is less than or equal to pivot
			if (std::get<2>(vec[j]) >= pivot) {
				i++;
				swap(vec[i], vec[j]);
			}
		}
		std::swap(vec[i + 1], vec[high]);// Put pivot to its position
		return (i + 1);// Return the point of partition
	}

	void quickSort(recommendVec &vec, int low, int high) {
		if (low < high) {// Base case: This part will be executed till the starting index low is higher than the ending index high

			int pi = partition(vec, low, high); // pi is Partitioning Index, arr[p] is now at right place

			quickSort(vec, low, pi - 1); // Separately sort elements before and after the Partition Index pi
			quickSort(vec, pi + 1, high);
		}
	}

	double cosPriv (std::vector<double> vec1, std::vector<double> vec2) {
		double dotProduct = 0.0;
		double magnitudeA = 0.0;
		double magnitudeB = 0.0;

		for (size_t i = 0; i < vec1.size(); ++i) {
			dotProduct += vec1[i] * vec2[i];
			magnitudeA += vec1[i] * vec1[i];
			magnitudeB += vec2[i] * vec2[i];
		}

		magnitudeA = std::sqrt(magnitudeA);
		magnitudeB = std::sqrt(magnitudeB);

		if (magnitudeA == 0.0 || magnitudeB == 0.0) {
			return 0.0;
		}

		return dotProduct / (magnitudeA * magnitudeB);
	}

	public:
		Recommend() {
			std::cout << "Recommending!\n";
			}

	pairVec fromKeyword(int uID) { //this returns all the meals with a certain keyword
			pairVec meals{};
			sqlite3_stmt* stmt;
			std::string keyword;

			const char* sql1 = "SELECT pref FROM Users WHERE uid = ?";
			sqlite3_prepare_v2(db, sql1, -1, &stmt, nullptr);
			sqlite3_bind_text(stmt, 1, (std::to_string(uID)).c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_step(stmt);
			int keywordInt = sqlite3_column_double(stmt, 0);
			sqlite3_reset(stmt);

			const char* sql2 = "SELECT name, image FROM Recipes WHERE category = ? COLLATE NOCASE;";;
			sqlite3_prepare_v2(db, sql2, -1, &stmt, nullptr);
			sqlite3_bind_text(stmt, 1, (std::to_string(uID)).c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_exec(db, keyword.c_str(), callbackKeyword, &meals,  &errmsg);

			return meals;
		}

	recommendVec cosine (vecVector searchedVector, pairVec toSearch) {//this returns a list of
			recommendVec results{};//ids and euclidean distances from a provided vector and provided search list
			sqlite3_stmt* stmt;
			std::string temp = "";
			std::vector<double> tempVec = {};
			const char* sql = "SELECT vector FROM Recipes WHERE name = ?";
            CROW_LOG_DEBUG << "Starting calculations";
			for (const auto &[id,img] : toSearch) {
				double dist = 0.0;
				sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

				sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

				sqlite3_step(stmt);
				std::istringstream iss(reinterpret_cast<const char*>(sqlite3_column_text(stmt,0)));
				while (std::getline(iss, temp, ' ' ) ) {tempVec.push_back(std::stod(temp));}

                CROW_LOG_DEBUG << "Vector received";

				for (int i =0; i<searchedVector.size(); i++) {
					dist += cosPriv(searchedVector[i], tempVec);
				}

				CROW_LOG_DEBUG << "Distance calculated";
				results.push_back(std::tuple(id, img, dist));
				CROW_LOG_DEBUG << "Distance pushed to results";
				sqlite3_reset(stmt);
				CROW_LOG_DEBUG << "Statement reset";
				tempVec = {};
				CROW_LOG_DEBUG << "tempVec cleared";
			}
			CROW_LOG_DEBUG << "Calculation complete, returning";
			sqlite3_finalize(stmt);
			return results;
		}

	crow::json::wvalue toJson(recommendVec finalRec) {
			crow::json::wvalue json_array = crow::json::wvalue::list();
			int i = 0;

			for (const auto& [name, image, dist] : finalRec) {
				crow::json::wvalue item;
				item["name"] = name;
				item["image"] = image;
				std::cout << dist << " ";
				json_array[i] = (std::move(item));
				i++;
			}

		return json_array;
		}

	crow::json::wvalue doIt (int uID, vecVector searchedVector) {
			if (searchedVector[0].empty() && searchedVector[1].empty()) {
				CROW_LOG_DEBUG << "SearchedVector cannot be empty";
				exit(67);
			}
			if (sqlite3_open("core.db", &db)!=SQLITE_OK) {
				std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
				db = nullptr;
			}

			recommendVec finalRec{};
			pairVec filteredRec{}; //= fromKeyword(uID); have to fix fromKeyword

			if (!filteredRec.empty()) {
				finalRec = cosine(searchedVector, filteredRec);
				quickSort(finalRec, 0, finalRec.size()-1);
			}
			else {
				std::cout << "TO DB!\n";
                CROW_LOG_DEBUG << "Getting names and images from recipes";
				sqlite3_exec(db, "SELECT name, image FROM Recipes", callbackKeyword, &filteredRec,  &errmsg); //get all meals from DB
                CROW_LOG_DEBUG << "Success! Getting cosine distance";
				finalRec = cosine(searchedVector, filteredRec);
				CROW_LOG_DEBUG << "Success! Sorting";
				quickSort(finalRec, 0, finalRec.size()-1);
				CROW_LOG_DEBUG << "Success! Converting to json";
			}
			sqlite3_close(db);
			finalRec.resize(10);
			crow::json::wvalue json_array = toJson(finalRec);
			CROW_LOG_DEBUG << "Success!!!";

			return json_array;
		}

	crow::json::wvalue getInstructions(std::string name) {
			if (sqlite3_open("core.db", &db)!=SQLITE_OK) {
				std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
				db = nullptr;
			}
			crow::json::wvalue json_array = crow::json::wvalue::list();
			crow::json::wvalue item;
			sqlite3_stmt* stmt = nullptr;
			name.erase(0, name.find_first_not_of(" \t\n\r"));
			name.erase(name.find_last_not_of(" \t\n\r") + 1);
			std::cout << name;

			const char* sql = "SELECT ingredients, instructions FROM Recipes WHERE name = ?;";
			if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
				std::cout << "Prep error " << sqlite3_errmsg(db)<< "\n";
				return json_array;
			}
			if (sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
				std::cout << "Bind error " << sqlite3_errmsg(db) << "\n";
				return json_array;
			}

			sqlite3_step(stmt);
			item["ingredients"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
			item["instructions"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
			json_array[0] = (std::move(item));

			sqlite3_close(db);
			sqlite3_finalize(stmt);
			return json_array;
		}
};
#endif