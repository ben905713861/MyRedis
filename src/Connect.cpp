#include "Connect.h"
#include "MessageHandler.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <list>
#include <iostream>
using namespace std;


int listenfd;
//连接池
list<int> connectionList;
//连接描述符集合
fd_set fds;

void Connect::startServer(int port) {
	//创建 IPv4协议的字节流套接字,若成功则返回一个套接字描述符
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1) {
		perror("create socket error");
		exit(-1);
	}
	
	int flags = fcntl(listenfd, F_GETFL);
	fcntl(listenfd, F_SETFL, flags | O_NONBLOCK);

	//一般是储存地址和端口，用于信息的显示及存储作用
	struct sockaddr_in serv_addr;
	//IPv4协议
	serv_addr.sin_family = AF_INET;
	//将端口号转换为网络字节序，即大端模式
	serv_addr.sin_port = htons(port);
	//将主机无符号长整型数转换成网络字节顺序。INADDR_ANY就是指定地址为0.0.0.0的地址，这个地址事实上表示不确定地址，或"所有地址"、“任意地址”
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1) {
		perror("bind socket error");
		exit(-1);
	}

	//listen第二个参数连接请求队列数等等
	if(listen(listenfd, 10) == -1) {
		perror("listen socket error\n");
		exit(-1);
	}
	
	listening();
}


void Connect::listening() {
	int maxsock = listenfd;
	//select超时时间
	timeval timeout;
	
	printf("======waiting for client's request======\n");
	
	while (1) {
		//每次循环都要清空集合，否则不能检测描述符变化
		FD_ZERO(&fds);
		//添加描述符用于监听socket端口的accept是否有新connection进来
		FD_SET(listenfd, &fds);
		//添加描述符用于监听每个connection的recv是否有数据流进来
		for(auto connection : connectionList) {
			FD_SET(connection, &fds);
		}
		
		//每次轮询都要重新设置超时时间
		timeout.tv_sec = 30;
		timeout.tv_usec = 0;
		//同时监听所有的连接是否来信息和是否有新连接产生
		int ret = select(maxsock+1, &fds, NULL, NULL, &timeout);
		if(ret < 0) {
			perror("select错误");
			exit(-1);
		}
		//轮询等待超时,跳入下一次循环
		else if(ret == 0) {
			continue;
		}
		
		//对所有连接遍历,找出其中有数据流的连接,然后接收数据
		char buff[200];
		for(list<int>::iterator item = connectionList.begin(); item != connectionList.end(); ) {
			int connection = *item;
			if(!FD_ISSET(connection, &fds)) {
				item ++;
				continue;
			}
			//接收数据流
			ret = recv(connection, buff, sizeof(buff), 0);
			//返回长度为0，意味着客户端断开连接，这时服务端也可以断开连接并从连接池中去除该连接
			if(ret <= 0) {
				printf("客户端断开连接\n");
				clear(connection, true);
				item = connectionList.erase(item);
				continue;
			}
			buff[ret] = '\0';
			
			char* msg = buff;
			//得到处理结果
			char* returnChars;
			try {
				returnChars = MessageHandler::action(connection, msg);
			} catch(invalid_argument& e) {
				clear(connection, false);
				item = connectionList.erase(item);
				cout << e.what() << "-服务器断开连接\n";
				continue;
			}
			//响应数据流
			send(connection, returnChars, strlen(returnChars), 0);
			
			delete returnChars;
			item ++;
		}
		
		//当有新连接到来时
		struct sockaddr_in client_addr;
		socklen_t sizeof_client_addr = sizeof(client_addr);
		if(FD_ISSET(listenfd, &fds)) {
			int connection = accept(listenfd, (struct sockaddr*) &client_addr, &sizeof_client_addr);
			if(connection <= 0) {
				perror("accept错误");
				continue;
			}
			//加入连接池
			connectionList.push_back(connection);
			//这里很重要，一定要判断最大值，因为重新分配的connection值有可能是已经释放过的历史值
			maxsock = connection > maxsock ? connection : maxsock;
			printf("发现新连接\n");
		}
		fflush(stdout);
	}
	close(listenfd);
}

void Connect::clear(int connection, bool clearChild) {
	close(connection);
	FD_CLR(connection, &fds);
	if(clearChild) {
		MessageHandler::clear(connection, true);
	}
}
