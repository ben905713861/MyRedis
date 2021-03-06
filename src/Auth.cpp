#include "Auth.h"
#include <string>
#include <iostream>
#include <set>
using namespace std;


set<int> authConnectionSet;
string loginPassword = "";


bool Auth::login(int connection, string password) {
	if(loginPassword == "") {
		return true;
	}
	if(password == loginPassword) {
		authConnectionSet.insert(connection);
		return true;
	}
	return false;
}

bool Auth::checkLogin(int connection) {
	if(loginPassword == "") {
		return true;
	}
	return authConnectionSet.count(connection);
}

void Auth::clear(int connection) {
	if(loginPassword == "") {
		return;
	}
	if(authConnectionSet.count(connection) > 0) {
		set<int>::iterator item = authConnectionSet.find(connection);
		authConnectionSet.erase(item);
	}
}
