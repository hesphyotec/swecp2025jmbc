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
#include <unordered_map>
#include "crow.h"
#include "asio.hpp"
#include "bgezdb.h"

typedef std::vector<std::tuple<std::string, std::string, double>> recommendVec;
typedef std::vector<std::pair<std::string,std::string>> pairVec;
typedef std::vector<std::vector<double>> vecVector;
char* errmsg = nullptr;
static const std::unordered_map<std::string, Traits> traitMap = {
	{"Breakfast", Traits::Breakfast},
	{"Dessert", Traits::Dessert},
	{"Pasta", Traits::Pasta},
	{"Seafood", Traits::Seafood},
	{"Vegan", Traits::Vegan},
	{"Vegetarian", Traits::Vegetarian},
	{"American", Traits::American},
	{"British", Traits::British},
	{"Canadian", Traits::Canadian},
	{"Chinese", Traits::Chinese},
	{"Croatian", Traits::Croatian},
	{"Dutch", Traits::Dutch},
	{"Egyptian", Traits::Egyptian},
	{"Filipino", Traits::Filipino},
	{"French", Traits::French},
	{"Greek", Traits::Greek},
	{"Indian", Traits::Indian},
	{"Irish", Traits::Irish},
	{"Italian", Traits::Italian},
	{"Jamaican", Traits::Jamaican},
	{"Japanese", Traits::Japanese},
	{"Kenyan", Traits::Kenyan},
	{"Malaysian", Traits::Malaysian},
	{"Mexican", Traits::Mexican},
	{"Moroccan", Traits::Moroccan},
	{"Polish", Traits::Polish},
	{"Portuguese", Traits::Portuguese},
	{"Russian", Traits::Russian},
	{"Spanish", Traits::Spanish},
	{"Thai", Traits::Thai},
	{"Tunisian", Traits::Tunisian},
	{"Turkish", Traits::Turkish},
	{"Ukrainian", Traits::Ukrainian},
	{"Uruguayan", Traits::Uruguayan},
	{"Vietnamese", Traits::Vietnamese}
};

int stringToTrait(const std::string& name) {
	auto it = traitMap.find(name);
	if (it != traitMap.end()) {
		return static_cast<int>(it->second);
	}
	return -1; // or handle error case
}

std::unordered_map<int, std::string> reverseTraitMap = [] {
	std::unordered_map<int, std::string> m;
	for (const auto& [key, val] : traitMap) {
		m[static_cast<int>(val)] = key;
	}
	return m;
}();

std::string traitToString(int val) {
	auto it = reverseTraitMap.find(val);
	if (it != reverseTraitMap.end()) {
		return it->second;
	}
	return "Unknown";
}

class UserRecSys {
private:
	sqlite3 * db;

	static int callbackIID(void *ingredientList, int columns, char **columnValue, char **colName) {//callback for userIngredientParser
		auto* results = static_cast<std::vector<int>*>(ingredientList);
		results->push_back(std::stoi(columnValue[0]));
		return 0; // Return 0 to continue processing rows, non-zero to stop
	}

	int stringToTrait(const std::string& name) {
		auto it = traitMap.find(name);
		if (it != traitMap.end()) {
			return static_cast<int>(it->second);
		}
		return -1; // or handle error case
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

	std::string userPrefParser (int userID) {//gets all meals stored by user
		sqlite3_stmt* stmt;
		std::string prefs = "";
		const char* sql = "SELECT pref FROM Users WHERE uid = ?;";
		sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr); //This preps the statement to have values added to it
		sqlite3_bind_int(stmt, 1, userID);
		sqlite3_step(stmt);
		if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
			prefs = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		}
		return prefs;
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

	void save(const std::vector<std::string>& toSave, const int uID) {
		if (sqlite3_open("core.db", &db)!=SQLITE_OK) {
			std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
			db = nullptr;
		}

		std::vector<int> tempVec;
		std::string toEnter;
		sqlite3_stmt* stmt;
		std::string keyword;

		const char* sql2 = "UPDATE Users SET pref = ? WHERE uid = ?;";
		sqlite3_prepare_v2(db, sql2, -1, &stmt, nullptr);

		if (toSave.empty()) {
			sqlite3_bind_null(stmt, 1);
		}
		else{
			for (auto& item: toSave) {
				tempVec.push_back(stringToTrait(item));
			}
			for (const auto& item: tempVec) {
				toEnter.append(std::to_string(item) + " ");
			}
			toEnter.pop_back();
			sqlite3_bind_text(stmt, 1, toEnter.c_str(), -1, SQLITE_TRANSIENT);
		}

		sqlite3_bind_int(stmt, 2, uID);
		int rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE) {
			CROW_LOG_ERROR << "Error executing update: " << sqlite3_errmsg(db);
		}
		sqlite3_finalize(stmt);
		sqlite3_close(db);
		CROW_LOG_DEBUG << "Entered Saved Items";
	}
};

class Recommend {
	private:
		sqlite3 * db;

	static int callbackKeyword(void *mealList, int columns, char **columnValue, char **colName) {//callback for keyword function
		auto* results = static_cast<std::vector<std::pair<std::string, std::string>>*>(mealList);
		results->emplace_back(columnValue[0],columnValue[1]);
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
			std::vector<int> tempVec;
			std::string toEnter;
			std::string s;
			std::string sql2;

			CROW_LOG_DEBUG << "Filtering";
			const char* sql1 = "SELECT pref FROM Users WHERE uid = ?";
			sqlite3_prepare_v2(db, sql1, -1, &stmt, nullptr);
			sqlite3_bind_text(stmt, 1, (std::to_string(uID)).c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_step(stmt);

			if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
				keyword = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
				CROW_LOG_DEBUG << "FOUND " << keyword;
				sqlite3_finalize(stmt);
				std::stringstream ss(keyword);

				while (getline(ss, s, ' ')) {
					tempVec.push_back(std::stoi(s));
				}

				for (auto& ing : tempVec) {
					if (ing < 7) {
						sql2 = "SELECT name, image FROM Recipes WHERE category = '" + traitToString(ing) + "' COLLATE NOCASE;";;
					}
					else {
						sql2 = "SELECT name, image FROM Recipes WHERE area = '" + traitToString(ing) + "' COLLATE NOCASE;";;
					}
					CROW_LOG_DEBUG << "Keyword " << traitToString(ing);
					sqlite3_exec(db, sql2.c_str(), callbackKeyword, &meals,  &errmsg);
				}
			}
			CROW_LOG_DEBUG << "Returning Filtered List";
			return meals;
		}

	recommendVec cosine (vecVector searchedVector, pairVec toSearch) {//this returns a list of
			recommendVec results{};//ids and cosine distances from a provided vector and provided search list
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
			if (searchedVector.empty()) {
				searchedVector = {{}, {}};
			}
			if (sqlite3_open("core.db", &db)!=SQLITE_OK) {
				std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
				db = nullptr;
			}

			recommendVec finalRec{};
			CROW_LOG_DEBUG << "Creating Filtered List";
			pairVec filteredRec = fromKeyword(uID);

			if (!filteredRec.empty()) {
				CROW_LOG_DEBUG << "Using Filtered List";
				finalRec = cosine(searchedVector, filteredRec);
				quickSort(finalRec, 0, finalRec.size()-1);
			}
			else {
				CROW_LOG_DEBUG << "TO DB!";
                CROW_LOG_DEBUG << "Getting names and images from recipes";
				sqlite3_exec(db, "SELECT name, image FROM Recipes", callbackKeyword, &filteredRec,  &errmsg); //get all meals from DB
                CROW_LOG_DEBUG << "Success! Getting cosine distance";
				finalRec = cosine(searchedVector, filteredRec);
				CROW_LOG_DEBUG << "Success! Sorting";
				quickSort(finalRec, 0, finalRec.size()-1);
				CROW_LOG_DEBUG << "Success! Converting to json";
			}
			sqlite3_close(db);
			if (finalRec.size() > 10) {
				finalRec.resize(10);
			}
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

			sqlite3_finalize(stmt);
			return json_array;
		}
};
#endif