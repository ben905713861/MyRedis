#ifndef WORKER_H_
#define WORKER_H_
#include <string>
using namespace std;

class Worker {
	
public:
	static string work(int connection, void** command);
	static void clear(int connection);
private:
	static string command();
	static string auth(int connection, string* password);
	static string get(string* key);
	static string set(string* key, string* value);
	static string del(string* key);
	static string multi(int connection);
	static string exec(int connection);
	static string keys(string* param);
};

#endif /* WORKER_H_ */
