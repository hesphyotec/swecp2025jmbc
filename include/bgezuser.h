#ifndef BGEZUSER
#define BGEZUSER

#include <iostream>
#include <string>

#include "crow.h"

#include "bgeztraits.h"

class User{                     //User class, used to store information about Users.
private:
    int m_uid{};
	std::string m_name{};
	std::string m_pass{};
	std::string m_email{};
	Trait m_preferences{};

public:
	User(int uid = -1, std::string name = "Guest", std::string pass = "", std::string email = ""):
	m_uid{uid}, m_name{name}, m_pass{pass}, m_email{email}
	{}

	int uid() const {return m_uid;}
	std::string name() const {return m_name;}
	std::string pass() const {return m_pass;}
	std::string email() const {return m_email;}
	Trait pref() const {return m_preferences;}

	User& setUid(const int id){
        m_uid = id;
        return *this;
	}
	User& setPass(const std::string_view pass){
        m_pass = pass;
        return *this;
    }
    User& setEmail(const std::string_view email){
        m_email = email;
        return *this;
    }
	User& setName(const std::string_view n){
		m_name = n;
		return *this;
	}
	User& setPref(const Trait& tr){
        m_preferences = tr;
        return *this;
	}
/*
	friend bool checkTrait(const User& user, Traits trt){
        return user.m_preferences & trt;
    }
    */
};

inline void userConnect(const User& user){             //Mostly just a test function to show that user was found in database.
    CROW_LOG_INFO << "User connected. Name: " << user.name() << " Email: " << user.email() << " User ID: " << user.uid() << "\n";
}

#endif
