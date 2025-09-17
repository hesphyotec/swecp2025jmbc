#include <iostream>
#include <string>
#include <bitset>
#include "crow.h"
class Item{
	private:
		int m_id{};
		int m_owner_id{};
		std::string m_name{};
		std::bitset<8> m_flags{};
	public:
		Item(int id = 0, int oid = 0, std::string name = "Unknown", std::bitset<8> flag = 0b0000'0000)
		:m_id{id}, m_owner_id{oid}, m_name{name}, m_flags{flag} {}
		
		void print(){
			std::cout << "Item id: " << m_id << "\nOwner's id: " << m_owner_id << "\nItem Name: " << m_name << "\n";
		}
};

int main(){
	Item Apple{1, 1, "Apple", 0x5};
	Apple.print();
	std::cout << __cplusplus << "\n";
	return 0;
}
