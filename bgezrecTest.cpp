#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#endif
#include <iostream>
#include <vector>
#include <string>
#include "bgezrecs.h"
#include "crow.h"
#include "asio.hpp"
#include "bgezdb.h"

int main () {
    UserRecSys user;
    Recommend rec;

    const bool test1 = stringToTrait("Chinese") == 10;
    std::cout << "1: " << test1 << "\n";
    const bool test2 = stringToTrait("Breakfast") == 1;
    std::cout << "2: " << test2 << "\n";
    const bool test3 = stringToTrait("FISH") == -1;
    std::cout << "3: " << test3 << "\n";

    const bool test4 = traitToString(10) == "Chinese";
    std::cout << "4: " << test4 << "\n";
    const bool test5 = traitToString(1) == "Breakfast";
    std::cout << "5: "  << test5 << "\n";
    const bool test6 = traitToString(157) == "Unknown";
    std::cout << "6: " << test6 << "\n";

    const bool test7 = user.userIngredientParser(5) == std::vector<int>{60}; //These are found in the DB, might be different
    std::cout << "7: " << test7 << "\n";
    const bool test8 = user.userIngredientParser(4)== std::vector<int>{60}; //if you were to test it locally
    std::cout << "8: " << test8 << "\n";
    const bool test9 = user.userIngredientParser(9).empty();
    std::cout << "9: " << test9 << "\n";

    const bool test10 = user.userMealParser(1).empty();
    std::cout << "10: " << test10 << "\n";
    const bool test11 = user.userPrefParser(1) == "1 2";
    std::cout << "11: " << test11 << "\n";
    const bool test12 = user.userPrefParser(2).empty();
    std::cout << "12: " << test12 << "\n";

    const bool test13 = user.ingredientToVector({2}) != std::vector<std::vector<double>>{{}};
    std::cout << "13: " << test13 << "\n";
    const bool test14 = user.ingredientToVector({1,2})!= std::vector<std::vector<double>>{{}};
    std::cout << "14: " << test14 << "\n";
    const bool test15 = user.ingredientToVector({9999}) == std::vector<std::vector<double>>{{}};
    std::cout << "15: " << test15 << "\n";
    const bool test16 = user.ingredientToVector({})== std::vector<std::vector<double>>{{}};
    std::cout << "16: " << test16 << "\n";

    const bool test17 = user.mealToVector({52764}) != std::vector<std::vector<double>>{{}};
    std::cout << "17: " << test17 << "\n";
    const bool test18 = user.mealToVector({52764,52765})!= std::vector<std::vector<double>>{{}};
    std::cout << "18: " << test18 << "\n";
    const bool test19 = user.mealToVector({9999}) == std::vector<std::vector<double>>{{}};
    std::cout << "19: " << test19 << "\n";
    const bool test20 = user.mealToVector({})== std::vector<std::vector<double>>{{}};
    std::cout << "20: " << test20 << "\n";

    const bool test21 = user.outputVector(user.ingredientToVector({2}), user.mealToVector({52764})) != std::vector<std::vector<double>>{{}};
    std::cout << "21: " << test21 << "\n";
    const bool test22 = user.outputVector(user.ingredientToVector({1,2}),user.mealToVector({52764,52765}))!= std::vector<std::vector<double>>{{}};
    std::cout << "22: " << test22 << "\n";
    const bool test23 = user.outputVector(user.ingredientToVector({9999}), user.mealToVector({9999}))== std::vector<std::vector<double>>{{},{}};
    std::cout << "23: " << test23 << "\n";
    const bool test24 = user.outputVector(user.ingredientToVector({}),user.mealToVector({})) == std::vector<std::vector<double>>{{},{}};
    std::cout << "24: " << test24 << "\n";

    const bool test25 = user.userGather(1) != std::vector<std::vector<double>>{{},{}};
    std::cout << "25: " << test25 << "\n";
    const bool test26 = user.userGather(2) == std::vector<std::vector<double>>{{},{}};
    std::cout << "26: " << test26 << "\n";
    const bool test27 = user.userGather(9999) == std::vector<std::vector<double>>{{},{}};
    std::cout << "27: " << test27 << "\n";

    user.save({"Breakfast"}, 3);
    user.save({"Breakfast", "Chinese"}, 3);
    user.save({"Yuck"}, 3);
    user.save({"Breakfast"}, 99999);
    user.save({}, 3);

    const bool test28 = rec.fromKeyword(1) != std::vector<std::pair<std::string, std::string>>{{"",""}};
    std::cout << "28: " << test28 << "\n";
    const bool test29 = rec.fromKeyword(3).empty();
    std::cout << "29: " << test29 << "\n";
    const bool test30 = rec.fromKeyword(99999).empty();
    std::cout << "30: " << test30 << "\n";

    rec.doIt(1,user.userGather(1) );
    std::cout << "31: 0 0 0 0 0 0 0 0 0 0" << "\n";
    rec.doIt(9999, user.userGather(1));
    std::cout << "32: 0 0 0 0 0 0 0 0 0 0" << "\n";
    rec.doIt(1,user.userGather(9999));
    std::cout << "33: 0 0 0 0 0 0 0 0 0 0" << "\n";
    rec.doIt(9999,user.userGather(9999));
    std::cout << "34: 0 0 0 0 0 0 0 0 0 0" << "\n";

    std::cout << rec.getInstructions("Bread omelette").dump()  << "\n";
    std::cout << rec.getInstructions("Yummy Food").dump() << "\n";
    std::cout << rec.getInstructions("").dump() << "\n";
    return 0;
}