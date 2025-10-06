#include <iostream>
#include <sqlite3.h>
#include <fstream>
#include <string>
#include <list>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cctype>
std::string file = "C:/Users/blake/swecp2025jmbc/word2vec.txt";

std::vector<double> search(const std::string& files, const std::string& wordToSearch) { //function to load word2vec
    std::vector<double> vectors; //creating a list of vectors
    std::ifstream ifs(files); //opening the file
    std::string line;
    int len = wordToSearch.length();
    std::string tempString;

    if (!ifs.is_open()) {
        return vectors;
    }

    std::cout << "Opened!\n";

    while (getline(ifs, line)) {
        if (line.find(wordToSearch)!=std::string::npos) {
            if (line.substr(0, (len)) == wordToSearch && line[len] == ' ') {
                std::cout << "Found " << line.substr(0, (len)) << "!\n";
                int start = len+1;
                int end = start+1;
                while (end < line.size()) {
                    while (line[end] != ' ' && end < line.size()) {
                        end++;
                    }
                    vectors.push_back(std::stod(line.substr(start, end)));
                    start = end+1;
                    end = start +1;
                }
                break;
            }
        }
    }
    return vectors;
}

double euclidean (const std::vector<double>& lst1, const std::vector<double>& lst2) {
    double sum1 = 0.0;
    double sum2 = 0.0;
    double sumReturn = 0.0;
    for (int i = 0; i < lst1.size(); i++) {
        sum1 += lst1[i];
        sum2 += lst2[i];
    }
    sumReturn = sqrt(std::pow((sum1-sum2),2.0));
    return sumReturn;
}

double vectorSimplify (const std::vector<double>& vector) {
    double sum = 0.0;
    for (const double& element: vector) {
        sum += element;
    }
    return sum;
}

std::string toKeyword(std::string& sqlIngredient) {
    std::ranges::transform(sqlIngredient,sqlIngredient.begin(), ::tolower);
    sqlIngredient[0] = std::toupper(sqlIngredient[0]);
    for (char &character : sqlIngredient) {
        if (std::isspace(static_cast<unsigned char>(character))) {
            character = '_'; // Replace with underscore
        }
    }
    return sqlIngredient;
}

static int callback(void *ingredientList, int columns, char **columnValue, char **colName) {
    auto* results = static_cast<std::vector<std::string>*>(ingredientList);
    std::string row;
    for (int i = 0; i < columns; i++) {
        results->push_back(columnValue[i]);
    }
    return 0; // Return 0 to continue processing rows, non-zero to stop
}

// void getResults (std::vector<std::string>& results) {
//     for (std::string &result : results) {
//         try {
//             double got = vectorSimplify(search(file, toKeyword(result)));
//             if (result.size() == 1) {
//                 throw "Not Found!";
//             }
//         }
//         catch (...) {
//             std::cerr << "Uh Oh. Unexpected error occured.";
//         }
//         sqlite3_exec(db, "SELECT name FROM ingredients", callback, &results, nullptr);
//     }
//}
int main() {
    sqlite3 * db;
    sqlite3_open("C:/Users/blake/swecp2025jmbc/RecipeDatabases.sqlite", &db);
    std::vector<std::string> results;
    sqlite3_exec(db, "SELECT name FROM ingredients", callback, &results, nullptr);


    return 0;
}
