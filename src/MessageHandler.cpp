#include "MessageHandler.h"
#include "Worker.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include <map>
using namespace std;


map<int, string> connection2tempcommand;


char* MessageHandler::action(int connection, char* msg) {
	string command = msg;
	connection2tempcommand[connection] += command;
	string tempCommand = connection2tempcommand[connection];
	
	//解码
	int startIndex = 0;
	string* returnMsg;
	void** res;
	try {
		res = (void**)decode(tempCommand, startIndex);
	} catch(invalid_argument& e) {
		//解码异常,清除
		clear(connection, true);
		
		throw invalid_argument(e.what());
	}
	
	if(res == NULL) {
		cout << "本次发送数据不全,暂停解码" << endl;
		returnMsg = new string("");
	} else {
		connection2tempcommand[connection] = "";
		//执行命令
		try {
			returnMsg = Worker::work(connection, res);
		} catch(invalid_argument& e) {
			//执行异常,清除
			clear(connection, false);
			throw invalid_argument(e.what());
		} 
	}
	delete [] res;
	
	int returnLength = (*returnMsg).length();
	char* returnChars = new char[returnLength + 1];
	returnMsg->copy(returnChars, returnLength, 0);
	returnChars[returnLength] = '\0';
	
	delete returnMsg;
	return returnChars;
}


void* MessageHandler::decode(string command, int& startIndex) {
	if(startIndex != 0) {
		startIndex += 1;
	}
	
	int isLine = false;
	int commandLen = command.length();
	for(int i = startIndex; i < commandLen - 1; i++) {
		if(command[i] == '\r' && command[i+1] == '\n') {
			isLine = true;
			break;
		}
	}
	if(isLine == false) {
		cout << "没有接收够一行数据,退出" << endl;
		return NULL;
	}
	
	switch(command[startIndex]) {
		//数组
		case '*': {
			if(command.length() <= 3) {
				throw invalid_argument("*后没有数字");
			}
			int length = 0;
			while(1) {
				startIndex ++;
				if(command[startIndex] == '\r') {
					continue;
				}
				if(command[startIndex-1] == '\r' && command[startIndex] == '\n') {
					break;
				}
				//char数字字符转int数字
				int lentemp = command[startIndex] - '0';
				if(lentemp < 0 || lentemp > 9) {
					//不是合法数字,异常，需要断开连接
					throw invalid_argument("*后不是合法数字");
				}
				length = length * 10 + lentemp;
			}
			if(length == 0) {
				throw invalid_argument("*后数字不得为0");
			}
			//递归数组
			//这里多放一个NULL元素在末尾,用以提示数组结束
			void** array = new void*[length+1]{NULL};
			for(int i = 0; i < length; i++) {
				//发生异常时候释放array内存,然后再次向上层抛出
				try {
					void* result = decode(command, startIndex);
					if(result == NULL) {
						return NULL;
					}
					array[i] = result;
				} catch(invalid_argument& e) {
					delete [] array;
					throw invalid_argument(e.what());
				}
			}
			return array;
		}
		
		//简单字符串
		case '+': {
			string* tempStr = new string;
			while(1) {
				startIndex ++;
				if(command[startIndex] == '\r') {
					continue;
				}
				if(command[startIndex-1] == '\r' && command[startIndex] == '\n') {
					break;
				}
				*tempStr += command[startIndex];
			}
			return tempStr;
		}
		
		//变长字符串
		case '$': {
			if(command.length() <= 3) {
				throw invalid_argument("$后没有数字");
			}
			int length = 0;
			while(1) {
				startIndex ++;
				if(command[startIndex] == '\r') {
					continue;
				}
				if(command[startIndex-1] == '\r' && command[startIndex] == '\n') {
					break;
				}
				//char数字字符转int数字
				int lentemp = command[startIndex] - '0';
				if(lentemp < 0 || lentemp > 9) {
					//不是合法数字,异常，需要断开连接
					throw invalid_argument("*后不是合法数字");
				}
				length = length * 10 + lentemp;
			}
			//接下来要接的字符串少于定义的长度,退出不处理
			int commandLen = command.length();
			if(commandLen - (startIndex+1) < length + 2) {
				cout << "没有接收够一行数据,退出" << endl;
				return NULL;
			}
			string* tempStr = new string; 
			for(int i = 0; i < length; i++) {
				startIndex ++;
				*tempStr += command[startIndex];
			}
			startIndex += 2;
			return tempStr;
		}
//		case ':':
//			;
//			break;
//		case '-':
//			;
//			break;
		default:
			return NULL;
	}
	
}


void MessageHandler::clear(int connection, bool clearChild) {
	if(connection2tempcommand.count(connection) > 0) {
		map<int, string>::iterator item = connection2tempcommand.find(connection);
		connection2tempcommand.erase(item);
	}
	if(clearChild) {
		Worker::clear(connection);
	}
}
