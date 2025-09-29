#ifndef BGEZITEM
#define BGEZITEM

#include <iostream>
#include <string>

#include "crow.h"

#include "bgeztraits.h"

class Item{
private:
    int m_id{};
    std::string m_name{};
    std::string m_desc{};
    std::string m_type{};
    Trait m_traits{};
public:
    Item(int id = -1, const std::string& name = "Unknown", const std::string& desc = "", const std::string& type = "", Trait trt = 0):
	m_id{id}, m_name{name}, m_desc{desc}, m_type{type}, m_traits{trt}
	{}

    int id() const {return m_id;}
    const std::string& name() const {return m_name;}
    const std::string& desc() const {return m_desc;}
    const std::string& type() const {return m_type;}

    Item& setId(int id){
        m_id = id;
        return *this;
    }
    Item& setName(const std::string& name){
        m_name = name;
        return *this;
    }
    Item& setDesc(const std::string& desc){
        m_desc = desc;
        return *this;
    }
    Item& setType(const std::string& type){
        m_type = type;
        return *this;
    }
    Item& setTraits(Trait trt){
        m_traits = trt;
        return *this;
    }
/*
    friend bool checkTrait(const Item& item, Traits trt){
        return item.m_traits & trt;
    }
*/
    const crow::json::wvalue toJson() const {
        crow::json::wvalue jItem;
        jItem["id"] = m_id;
        jItem["name"] = m_name;
        jItem["desc"] = (m_desc.empty()) ? "": m_desc;
        jItem["type"] = (m_type.empty()) ? "": m_type;

        return jItem;
    }
};

#endif // BGEZITEM
