#ifndef CONNECT_H_
#define CONNECT_H_

class Connect {
	
public:
	static void startServer(int port);
private:
	static void listening();
	static void clear(int connection, bool clearChild);
	static void exitListening(int sig);
};

#endif /* CONNECT_H_ */
