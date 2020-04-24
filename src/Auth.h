#ifndef AUTH_H_
#define AUTH_H_

#include <string>
using namespace std;

class Auth {
	
public:
	static bool login(int connection, string password);
	static bool checkLogin(int connection);
	static void clear(int connection);
};

#endif /* AUTH_H_ */
