#ifndef MESSAGEHANDLER_H_
#define MESSAGEHANDLER_H_

#include <string>
using namespace std;

class MessageHandler {
	
public:
	static char* action(int connection, char* msg);
	static void clear(int connection);
private:
	static void* decode(string commandStr, int& startIndex);
	
};

#endif /* MESSAGEHANDLER_H_ */
