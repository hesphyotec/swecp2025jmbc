#include <iostream>
#include <sqlite3.h>
#include <fstream>
#include <string>
#include <list>
#include <cmath>
#include <utility>
#include <vector>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <tuple>
#include <sstream>
#include <ranges>

typedef std::vector<std::tuple<std::string, std::string, double>> recommendVec;
typedef std::vector<std::pair<std::string,std::string>> pairVec;

class CreateWord2Vec {// Creates vectors
	private:
		std::string file = "C:/Users/blake/swecp2025jmbc/word2vec.txt";
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
		CreateWord2Vec() {
			std::cout << "started!\n";
			if (sqlite3_open("C:/Users/blake/swecp2025jmbc/core.db", &db)!=SQLITE_OK) {
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
		}
		return vectors;
	}

	double vectorSimplify (const std::vector<double>& vector) {//this takes the vectors and makes them into a mean value
		double sum = 0.0;
		for (const double& element: vector) {
			sum += element;
		}
		return sum/300;//there are 300 vectors per word
	}

	std::string toKeyword(std::string& sqlIngredient) {//this turns the values in the db to the form they're in, in the word2vec file
		std::ranges::transform(sqlIngredient,sqlIngredient.begin(), ::tolower);
		//sqlIngredient[0] = std::toupper(sqlIngredient[0]); //Only needed sometimes
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
				double got = vectorSimplify(search(toKeyword(name)));//for all names do this

				if (got == 0.000000) {
					throw name;
				}
				const char* idC = id.c_str();

				sqlite3_bind_double(stmt, 1, got);
				sqlite3_bind_text(stmt, 2, idC, -1, SQLITE_TRANSIENT);

				if (sqlite3_step(stmt) != SQLITE_DONE) {std::cerr << "Insert failed: " << sqlite3_errmsg(db) << std::endl;} //This preforms the insert
				else{std::cout << "Inserted " << std::to_string(got) << " for " << name <<"\n";}
			}
			catch (...) {
				std::cerr << "Uh Oh. Unexpected error occured on key" << id << "\n";
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
			std::vector<std::pair<std::string, std::string>> results;
			sqlite3_exec(db, "SELECT id, name FROM Ingredients WHERE vectors IS NULL", callback, &results, nullptr);
			getResults(results);
			std::cout << "Done! \n";
			sqlite3_close(db);
		}

	std::vector<std::string> getMeals() {
			std::vector<std::string> output;
			sqlite3_exec(db, "SELECT id FROM Recipes WHERE vector IS NULL", callbackMeals, &output, nullptr);
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

class User {
	private:
		sqlite3 * db;

	static int callbackIID(void *ingredientList, int columns, char **columnValue, char **colName) {//callback for userIngredientParser
		auto* results = static_cast<std::vector<int>*>(ingredientList);
		results->push_back(std::stoi(columnValue[0]));
		return 0; // Return 0 to continue processing rows, non-zero to stop
	}

	public:
		User() {
			std::cout << "Hello!\n";
			if (sqlite3_open("C:/Users/blake/swecp2025jmbc/core.db", &db)!=SQLITE_OK) {
				std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
				db = nullptr;
			}
		}

	int getUserID(int userID) {
			return userID;
		}


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

	double userGather (int userID) {
			int uID = getUserID(userID);
			double ingVec = ingredientToVector(userIngredientParser(uID));
			double mealVec = mealToVector(userMealParser(uID));
			double totVec = outputVector(ingVec, mealVec);
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
		Recommend() {
			std::cout << "Recommending!\n";
			}

	pairVec fromKeyword(std::string keyword) { //this returns all the meals with a certain keyword
			pairVec meals;

			std::string sql = "SELECT name, image FROM Recipes WHERE category = '" + keyword + "' COLLATE NOCASE;";
			sqlite3_exec(db, sql.c_str(), callbackKeyword, &meals, nullptr);

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

	recommendVec doIt (std::string keyword, double searchedVector) {
			if (sqlite3_open("C:/Users/blake/swecp2025jmbc/core.db", &db)!=SQLITE_OK) {
				std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
				db = nullptr;
			}

			recommendVec finalRec;
			pairVec filteredRec = fromKeyword(std::move(keyword));

			if (!filteredRec.empty()) {
				finalRec = euclidean(searchedVector, filteredRec);
				quickSort(finalRec, 0, finalRec.size()-1);
			}
			else {
				std::cout << "TO DB!" << "\n";

				sqlite3_exec(db, "SELECT name, image FROM Recipes", callbackKeyword, &filteredRec, nullptr); //get all meals from DB

				finalRec = euclidean(searchedVector, filteredRec);
				quickSort(finalRec, 0, finalRec.size()-1);
			}
			sqlite3_close(db);
			return finalRec;
		}
};

void test() {
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	User user;
	Recommend rec;
	double foundVec = user.userGather(4);
	std::cout << foundVec << "\n";
	recommendVec output = rec.doIt("Chicken",foundVec);
	for (const auto &[name, img, vec]: output) {
		std::cout << name << ", " << img << ", " << vec << "\n";
	}
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::cout << "\n" << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << "[ns]" << std::endl;
}

int main() {
	test();

	return 0;
	//get uid from browser USER CLASS
}