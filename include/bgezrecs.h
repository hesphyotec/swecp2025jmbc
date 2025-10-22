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

class UserRecSys : public DBConnection{
private:
	sqlite3 * db;

	static int callbackIID(void *ingredientList, int columns, char **columnValue, char **colName) {//callback for userIngredientParser
		auto* results = static_cast<std::vector<int>*>(ingredientList);
		results->push_back(std::stoi(columnValue[0]));
		return 0; // Return 0 to continue processing rows, non-zero to stop
	}

public:
	std::vector<int> userIngredientParser (int userID) {//gets all ingredients stored by user
			std::vector<int> ingredientID;
			std::string sqlPre = "SELECT iid FROM UserItems WHERE uid=";
			std::string sqlFix = sqlPre + std::to_string(userID);
			const char* sql = sqlFix.c_str();
			sqlite3_exec(db, sql, callbackIID, &ingredientID, nullptr);
			return ingredientID;
		}

	std::vector<int> userMealParser (int userID) {//gets all ingredients stored by user
			std::vector<int> mealID;
			std::string sqlPre = "SELECT mid FROM UserMeals WHERE uid=";
			std::string sqlFix = sqlPre + std::to_string(userID);
			const char* sql = sqlFix.c_str();
			sqlite3_exec(db, sql, callbackIID, &mealID, nullptr);
			return mealID;
		}

	double ingredientToVector(const std::vector<int>& ingredientID) {
			//this takes the user ingredients and vector-ifys them
			if (ingredientID.size() == 0) { return 0.0; }
			sqlite3_stmt *stmt;
			double count = 0.0;
			double ingredientVectors = 0.0;

			for (const int &ingID: ingredientID) {
				const char *sql = "SELECT vectors FROM Ingredients WHERE id = ?;";
				sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr); //This preps the statement to have values added to it
				sqlite3_bind_int(stmt, 1, ingID);

				sqlite3_step(stmt);
				ingredientVectors += sqlite3_column_double(stmt,0); //add that vector value to ingredientVectors
				count += 1.0; //update count so you can get avg

				sqlite3_reset(stmt); // Reset statement for next row //call db for vector value
			}
			return ingredientVectors / count;
		}

	double mealToVector(const std::vector<int>& mealID) {
			//this takes the user past meals and vector-ifys them
			if (mealID.size() == 0) { return 0.0; }
			sqlite3_stmt *stmt;
			double count = 0.0;
			double mealVectors = 0.0;

			for (const int &ingID: mealID) {
				const char *sql = "SELECT vectors FROM Recipes WHERE id = ?;";
				sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr); //This preps the statement to have values added to it
				sqlite3_bind_int(stmt, 1, ingID);

				sqlite3_step(stmt);
				mealVectors += sqlite3_column_double(stmt, 0); //add that vector value to melaVectors
				count += 1.0; //update count so you can get avg

				sqlite3_reset(stmt); // Reset statement for next row //call db for vector value
			}
			return mealVectors / count;
		}

	double outputVector (const double ingredientVector, const double mealVector) {
			double avg = 0;
			if (mealVector!=0.0){avg+=1.0;}
			if (ingredientVector!=0.0){avg+=1.0;}
			return (ingredientVector + mealVector) / avg;
		}

	double userGather (int uID) {
			double ingVec = ingredientToVector(userIngredientParser(uID));
			double mealVec = mealToVector(userMealParser(uID));
			double totVec = outputVector(ingVec, mealVec);
			return totVec;
		}
};

class Recommend : public DBConnection{
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

		for (int j = low; j <= high - 1; j++) {// If current element is smaller than or equal to pivot
			if (std::get<2>(vec[j]) <= pivot) {
				i++;
				swap(vec[i], vec[j]);
			}
		}
		std::swap(vec[i + 1], vec[high]);// Put pivot to its position
		return (i + 1);// Return the point of partition
	}

	void quickSort(recommendVec &vec, int low, int high) {
		if (low < high) {// Base case: This part will be executed till the starting index low is lesser than the ending index high

			int pi = partition(vec, low, high); // pi is Partitioning Index, arr[p] is now at right place

			quickSort(vec, low, pi - 1); // Separately sort elements before and after the Partition Index pi
			quickSort(vec, pi + 1, high);
		}
	}

public:
	pairVec fromKeyword(int uID) { //this returns all the meals with a certain keyword
        pairVec meals;
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
        sqlite3_exec(db, keyword.c_str(), callbackKeyword, &meals, nullptr);

        return meals;
    }

	recommendVec euclidean (double searchedVector, pairVec toSearch) {//this returns a list of
        recommendVec results;//ids and euclidean distances from a provided vector and provided search list
        sqlite3_stmt* stmt;
        const char* sql = "SELECT vector FROM Recipes WHERE name = ?";

        for (const auto &[id,img] : toSearch) {
            sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

            sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

            sqlite3_step(stmt);
            double recipeVec = sqlite3_column_double(stmt, 0);

            double dist = std::sqrt(pow((searchedVector - recipeVec),2));
            results.push_back(std::tuple(id, img, dist));
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
        }
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
				json_array[i] = (std::move(item));
				i++;
			}

		return json_array;
		}

	crow::json::wvalue doIt (int uID, double searchedVector) {
			if (sqlite3_open("C:/Users/blake/swecp2025jmbc/core.db", &db)!=SQLITE_OK) {
				std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
				db = nullptr;
			}

			recommendVec finalRec;
			pairVec filteredRec; //= fromKeyword(uID); have to fix fromKeyword

			if (!filteredRec.empty()) {
				finalRec = euclidean(searchedVector, filteredRec);
				quickSort(finalRec, 0, finalRec.size()-1);
			}
			else {
				std::cout << "TO DB!\n";

				sqlite3_exec(db, "SELECT name, image FROM Recipes", callbackKeyword, &filteredRec, nullptr); //get all meals from DB

				finalRec = euclidean(searchedVector, filteredRec);
				quickSort(finalRec, 0, finalRec.size()-1);
			}
			sqlite3_close(db);
			crow::json::wvalue json_array = toJson(finalRec);
			return json_array;
		}

};

#endif
