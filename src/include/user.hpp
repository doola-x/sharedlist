#pragma once

using namespace std;

class User {
public:
	int id;
	string username;
	string email;

	User();
	User(int _id, string _username, string _email);
	~User();
}