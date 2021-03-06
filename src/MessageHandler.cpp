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
	int deep = 0;
	string returnMsg;
	void** commandParam;
	try {
		commandParam = (void**)decode(tempCommand, startIndex, deep);
	} catch(invalid_argument& e) {
		//解码异常,清除
		clear(connection, true);
		throw invalid_argument(e.what());
	}
	
	if(commandParam == NULL) {
//		cout << "本次发送数据不全,暂停解码" << endl;
		returnMsg = "";
	} else {
		connection2tempcommand[connection] = "";
		//执行命令
		try {
			returnMsg = Worker::work(connection, commandParam);
		} catch(invalid_argument& e) {
			//执行异常,清除
			clear(connection, false);
			throw invalid_argument(e.what());
		}
	}
	
	int returnLength = returnMsg.length();
	char* returnChars = new char[returnLength + 1];
	returnMsg.copy(returnChars, returnLength, 0);
	returnChars[returnLength] = '\0';
	
	return returnChars;
}


void* MessageHandler::decode(string command, int& startIndex, int& deep) {
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
//		cout << "没有接收够一行数据,退出" << endl;
		return NULL;
	}
	
	switch(command[startIndex]) {
		//数组
		case '*': {
			if(command.length() <= 3) {
				throw invalid_argument("*后没有数字");
			}
			//数组层数计数判断
			if(++deep > 1) {
				throw invalid_argument("*数组深度最大为2");
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
					void* result = decode(command, startIndex, deep);
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
		
		//变长字符串
		case '$': {
			if(command.length() <= 3) {
				throw invalid_argument("$后没有数字");
			}
			//数组层数计数判断
			if(deep != 1) {
				throw invalid_argument("仅能在1层数组中传递$字符串");
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
//				cout << "没有接收够一行数据,退出" << endl;
				return NULL;
			}
			char buff[length + 1];
			for(int i = 0; i < length; i++) {
				startIndex ++;
				buff[i] = command[startIndex];
			}
			buff[length] = '\0';
			
			string* tempStr = new string(buff);
			startIndex += 2;
			return tempStr;
		}
		default:
			throw invalid_argument("不能识别的命令头");
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
