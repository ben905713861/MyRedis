#ifndef CONNECT_H_
#define CONNECT_H_

class Connect {
	
public:
	void static startServer(int port);
	virtual ~Connect();
private:
	void static listening();
	
};

#endif /* CONNECT_H_ */
