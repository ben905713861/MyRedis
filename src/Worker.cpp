#include "Worker.h"
#include "Auth.h"
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <list>
using namespace std;


unordered_map<string, string> dataMap(16384);
unordered_map<int, list<void**>*> multiMap;

string Worker::work(int connection, void** commandParam) {
	try {
		string res;
		string commandHead = *(string*)commandParam[0];
		//转大写
		transform(commandHead.begin(), commandHead.end(), commandHead.begin(), ::toupper);
		
		//分析命令
		//登录操作
		if(commandHead == "COMMAND") {
			res = command();
			freeCommandParam(commandParam);
			return res;
		}
		//验证密码
		if(commandHead == "AUTH") {
			if(commandParam[1] == NULL) {
				throw invalid_argument("密码不得为空");
			}
			string* password = (string*)commandParam[1];
			res = auth(connection, password);
			freeCommandParam(commandParam);
			return res;
		}
		
		//检验密码登录权限
		if(Auth::checkLogin(connection) == false) {
			res = "-NOAUTH Authentication required.\r\n";
			freeCommandParam(commandParam);
			return res;
		}
		
		if(commandHead == "MULTI") {
			res = multi(connection);
			freeCommandParam(commandParam);
			return res;
		}
		if(commandHead == "EXEC") {
			res = exec(connection);
			freeCommandParam(commandParam);
			return res;
		}
		
		//判断是否有开启multi
		if(multiMap.count(connection) > 0) {
			//拷贝内存
			int len = 0;
			for(int i = 0; ; i++) {
				if(commandParam[i] == NULL) {
					break;
				}
				len++;
			}
			void** commandParamNew = new void*[len+1]{NULL};
			for(int i = 0; i < len; i++) {
				string* temp = new string;
				*temp = *(string*)commandParam[i];
				commandParamNew[i] = temp;
			}
			multiMap[connection]->push_back(commandParamNew);
			freeCommandParam(commandParam);
			return "+QUEUED\r\n";
		}
		
		//常规操作
		if(commandHead == "GET") {
			if(commandParam[1] == NULL) {
				throw invalid_argument("字段名不得为空");
			}
			string* key = (string*)commandParam[1];
			res = get(key);
		}
		else if(commandHead == "SET") {
			if(commandParam[1] == NULL) {
				throw invalid_argument("字段名不得为空");
			}
			string* key = (string*)commandParam[1];
			if(commandParam[2] == NULL) {
				throw invalid_argument("记录值不得为空");
			}
			string* value = (string*)commandParam[2];
			res = set(key, value);
		}
		else if(commandHead == "DEL") {
			if(commandParam[1] == NULL) {
				throw invalid_argument("字段名不得为空");
			}
			string* key = (string*)commandParam[1];
			res = del(key);
		}
		else if(commandHead == "KEYS") {
			if(commandParam[1] == NULL) {
				throw invalid_argument("参数1不得为空");
			}
			string* key = (string*)commandParam[1];
			res = keys(key);
		}
		else {
			res = "-ERR unknown command'"+ commandHead +"'\r\n";
		}
		freeCommandParam(commandParam);
		return res;
	} catch(invalid_argument& e) {
		//执行异常,清除登录连接
		clear(connection);
		freeCommandParam(commandParam);
		throw invalid_argument(e.what());
	}
}


string Worker::command() {
	return "*172\r\n*6\r\n$4\r\nzadd\r\n:-4\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\ntouch\r\n:-2\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\ngeoadd\r\n:-5\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nexec\r\n:1\r\n*2\r\n+noscript\r\n+skip_monitor\r\n:0\r\n:0\r\n:0\r\n*6\r\n$9\r\nreadwrite\r\n:1\r\n*1\r\n+fast\r\n:0\r\n:0\r\n:0\r\n*6\r\n$4\r\neval\r\n:-3\r\n*2\r\n+noscript\r\n+movablekeys\r\n:0\r\n:0\r\n:0\r\n*6\r\n$11\r\nzinterstore\r\n:-4\r\n*3\r\n+write\r\n+denyoom\r\n+movablekeys\r\n:0\r\n:0\r\n:0\r\n*6\r\n$9\r\nrandomkey\r\n:1\r\n*2\r\n+readonly\r\n+random\r\n:0\r\n:0\r\n:0\r\n*6\r\n$7\r\npersist\r\n:2\r\n*2\r\n+write\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$7\r\ngeodist\r\n:-4\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\nbitpos\r\n:-3\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$10\r\npsubscribe\r\n:-2\r\n*4\r\n+pubsub\r\n+noscript\r\n+loading\r\n+stale\r\n:0\r\n:0\r\n:0\r\n*6\r\n$6\r\nconfig\r\n:-2\r\n*3\r\n+admin\r\n+loading\r\n+stale\r\n:0\r\n:0\r\n:0\r\n*6\r\n$10\r\nsdiffstore\r\n:-3\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:-1\r\n:1\r\n*6\r\n$6\r\nlrange\r\n:4\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\nhsetnx\r\n:4\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nkeys\r\n:2\r\n*2\r\n+readonly\r\n+sort_for_script\r\n:0\r\n:0\r\n:0\r\n*6\r\n$6\r\nasking\r\n:1\r\n*1\r\n+fast\r\n:0\r\n:0\r\n:0\r\n*6\r\n$5\r\nblpop\r\n:-3\r\n*2\r\n+write\r\n+noscript\r\n:1\r\n:-2\r\n:1\r\n*6\r\n$4\r\nhdel\r\n:-3\r\n*2\r\n+write\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\nclient\r\n:-2\r\n*2\r\n+admin\r\n+noscript\r\n:0\r\n:0\r\n:0\r\n*6\r\n$4\r\necho\r\n:2\r\n*1\r\n+fast\r\n:0\r\n:0\r\n:0\r\n*6\r\n$7\r\nzincrby\r\n:4\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$7\r\nhgetall\r\n:2\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$7\r\ncluster\r\n:-2\r\n*1\r\n+admin\r\n:0\r\n:0\r\n:0\r\n*6\r\n$9\r\ngeoradius\r\n:-6\r\n*1\r\n+write\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\nlpushx\r\n:3\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\npttl\r\n:2\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$12\r\nhincrbyfloat\r\n:4\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nhlen\r\n:2\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$9\r\nsismember\r\n:3\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$7\r\nflushdb\r\n:1\r\n*1\r\n+write\r\n:0\r\n:0\r\n:0\r\n*6\r\n$11\r\nsunionstore\r\n:-3\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:-1\r\n:1\r\n*6\r\n$11\r\nzrangebylex\r\n:-4\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\ninfo\r\n:-1\r\n*2\r\n+loading\r\n+stale\r\n:0\r\n:0\r\n:0\r\n*6\r\n$14\r\nrestore-asking\r\n:-4\r\n*3\r\n+write\r\n+denyoom\r\n+asking\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nlrem\r\n:4\r\n*1\r\n+write\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\nsinter\r\n:-2\r\n*2\r\n+readonly\r\n+sort_for_script\r\n:1\r\n:-1\r\n:1\r\n*6\r\n$5\r\nsscan\r\n:-3\r\n*2\r\n+readonly\r\n+random\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\nstrlen\r\n:2\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$8\r\nshutdown\r\n:-1\r\n*3\r\n+admin\r\n+loading\r\n+stale\r\n:0\r\n:0\r\n:0\r\n*6\r\n$6\r\nmsetnx\r\n:-3\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:-1\r\n:2\r\n*6\r\n$4\r\nrpop\r\n:2\r\n*2\r\n+write\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$11\r\nsinterstore\r\n:-3\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:-1\r\n:1\r\n*6\r\n$8\r\nexpireat\r\n:3\r\n*2\r\n+write\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$8\r\nbitfield\r\n:-2\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\nhkeys\r\n:2\r\n*2\r\n+readonly\r\n+sort_for_script\r\n:1\r\n:1\r\n:1\r\n*6\r\n$7\r\nevalsha\r\n:-3\r\n*2\r\n+noscript\r\n+movablekeys\r\n:0\r\n:0\r\n:0\r\n*6\r\n$11\r\nunsubscribe\r\n:-1\r\n*4\r\n+pubsub\r\n+noscript\r\n+loading\r\n+stale\r\n:0\r\n:0\r\n:0\r\n*6\r\n$8\r\ngetrange\r\n:4\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\ngeopos\r\n:-2\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\nzcard\r\n:2\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\nscript\r\n:-2\r\n*1\r\n+noscript\r\n:0\r\n:0\r\n:0\r\n*6\r\n$7\r\npublish\r\n:3\r\n*4\r\n+pubsub\r\n+loading\r\n+stale\r\n+fast\r\n:0\r\n:0\r\n:0\r\n*6\r\n$8\r\nreplconf\r\n:-1\r\n*4\r\n+admin\r\n+noscript\r\n+loading\r\n+stale\r\n:0\r\n:0\r\n:0\r\n*6\r\n$4\r\nsadd\r\n:-3\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$7\r\nhstrlen\r\n:3\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\nselect\r\n:2\r\n*2\r\n+loading\r\n+fast\r\n:0\r\n:0\r\n:0\r\n*6\r\n$7\r\nlinsert\r\n:5\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:1\r\n:1\r\n*6\r\n$16\r\nzremrangebyscore\r\n:4\r\n*1\r\n+write\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\ntype\r\n:2\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\nzcount\r\n:4\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\nsubstr\r\n:4\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\nbrpop\r\n:-3\r\n*2\r\n+write\r\n+noscript\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\nincrby\r\n:3\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$8\r\nbitcount\r\n:-2\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$7\r\nmigrate\r\n:-6\r\n*2\r\n+write\r\n+movablekeys\r\n:0\r\n:0\r\n:0\r\n*6\r\n$6\r\nsetbit\r\n:4\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\nlpush\r\n:-3\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$9\r\nrpoplpush\r\n:3\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:2\r\n:1\r\n*6\r\n$7\r\nlatency\r\n:-2\r\n*4\r\n+admin\r\n+noscript\r\n+loading\r\n+stale\r\n:0\r\n:0\r\n:0\r\n*6\r\n$5\r\nrpush\r\n:-3\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$9\r\npexpireat\r\n:3\r\n*2\r\n+write\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$8\r\nzrevrank\r\n:3\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$9\r\nzrevrange\r\n:-4\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$12\r\npunsubscribe\r\n:-1\r\n*4\r\n+pubsub\r\n+noscript\r\n+loading\r\n+stale\r\n:0\r\n:0\r\n:0\r\n*6\r\n$6\r\nappend\r\n:3\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nlset\r\n:4\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nzrem\r\n:-3\r\n*2\r\n+write\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$8\r\nsmembers\r\n:2\r\n*2\r\n+readonly\r\n+sort_for_script\r\n:1\r\n:1\r\n:1\r\n*6\r\n$7\r\ngeohash\r\n:-2\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$8\r\nlastsave\r\n:1\r\n*2\r\n+random\r\n+fast\r\n:0\r\n:0\r\n:0\r\n*6\r\n$4\r\nscan\r\n:-2\r\n*2\r\n+readonly\r\n+random\r\n:0\r\n:0\r\n:0\r\n*6\r\n$7\r\npfmerge\r\n:-2\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:-1\r\n:1\r\n*6\r\n$5\r\nzrank\r\n:3\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$7\r\nmonitor\r\n:1\r\n*2\r\n+admin\r\n+noscript\r\n:0\r\n:0\r\n:0\r\n*6\r\n$6\r\nexpire\r\n:3\r\n*2\r\n+write\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nping\r\n:-1\r\n*2\r\n+stale\r\n+fast\r\n:0\r\n:0\r\n:0\r\n*6\r\n$14\r\nzremrangebylex\r\n:4\r\n*1\r\n+write\r\n:1\r\n:1\r\n:1\r\n*6\r\n$7\r\nhincrby\r\n:4\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$11\r\nsrandmember\r\n:-2\r\n*2\r\n+readonly\r\n+random\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\npubsub\r\n:-2\r\n*4\r\n+pubsub\r\n+random\r\n+loading\r\n+stale\r\n:0\r\n:0\r\n:0\r\n*6\r\n$15\r\nzremrangebyrank\r\n:4\r\n*1\r\n+write\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nrole\r\n:1\r\n*3\r\n+noscript\r\n+loading\r\n+stale\r\n:0\r\n:0\r\n:0\r\n*6\r\n$4\r\nwait\r\n:3\r\n*1\r\n+noscript\r\n:0\r\n:0\r\n:0\r\n*6\r\n$17\r\ngeoradiusbymember\r\n:-5\r\n*1\r\n+write\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\nobject\r\n:3\r\n*1\r\n+readonly\r\n:2\r\n:2\r\n:2\r\n*6\r\n$4\r\ndecr\r\n:2\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\npfadd\r\n:-2\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$10\r\npfselftest\r\n:1\r\n*1\r\n+admin\r\n:0\r\n:0\r\n:0\r\n*6\r\n$4\r\nspop\r\n:-2\r\n*3\r\n+write\r\n+random\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\ndebug\r\n:-1\r\n*2\r\n+admin\r\n+noscript\r\n:0\r\n:0\r\n:0\r\n*6\r\n$5\r\nsmove\r\n:4\r\n*2\r\n+write\r\n+fast\r\n:1\r\n:2\r\n:1\r\n*6\r\n$4\r\nllen\r\n:2\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\nmulti\r\n:1\r\n*2\r\n+noscript\r\n+fast\r\n:0\r\n:0\r\n:0\r\n*6\r\n$6\r\ngetset\r\n:3\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\nsdiff\r\n:-2\r\n*2\r\n+readonly\r\n+sort_for_script\r\n:1\r\n:-1\r\n:1\r\n*6\r\n$5\r\nhscan\r\n:-3\r\n*2\r\n+readonly\r\n+random\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nsave\r\n:1\r\n*2\r\n+admin\r\n+noscript\r\n:0\r\n:0\r\n:0\r\n*6\r\n$7\r\nslaveof\r\n:3\r\n*3\r\n+admin\r\n+noscript\r\n+stale\r\n:0\r\n:0\r\n:0\r\n*6\r\n$4\r\nauth\r\n:2\r\n*4\r\n+noscript\r\n+loading\r\n+stale\r\n+fast\r\n:0\r\n:0\r\n:0\r\n*6\r\n$6\r\nrename\r\n:3\r\n*1\r\n+write\r\n:1\r\n:2\r\n:1\r\n*6\r\n$6\r\nbgsave\r\n:1\r\n*1\r\n+admin\r\n:0\r\n:0\r\n:0\r\n*6\r\n$6\r\ndecrby\r\n:3\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$7\r\ndiscard\r\n:1\r\n*2\r\n+noscript\r\n+fast\r\n:0\r\n:0\r\n:0\r\n*6\r\n$6\r\nsunion\r\n:-2\r\n*2\r\n+readonly\r\n+sort_for_script\r\n:1\r\n:-1\r\n:1\r\n*6\r\n$7\r\npfdebug\r\n:-3\r\n*1\r\n+write\r\n:0\r\n:0\r\n:0\r\n*6\r\n$7\r\npexpire\r\n:3\r\n*2\r\n+write\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nsync\r\n:1\r\n*3\r\n+readonly\r\n+admin\r\n+noscript\r\n:0\r\n:0\r\n:0\r\n*6\r\n$5\r\nhvals\r\n:2\r\n*2\r\n+readonly\r\n+sort_for_script\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\nzscan\r\n:-3\r\n*2\r\n+readonly\r\n+random\r\n:1\r\n:1\r\n:1\r\n*6\r\n$3\r\nget\r\n:2\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\nexists\r\n:-2\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:-1\r\n:1\r\n*6\r\n$6\r\nlindex\r\n:3\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$7\r\nrestore\r\n:-4\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nsort\r\n:-2\r\n*3\r\n+write\r\n+denyoom\r\n+movablekeys\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\nsetex\r\n:4\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nincr\r\n:2\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$3\r\nset\r\n:-3\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\nscard\r\n:2\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nmget\r\n:-2\r\n*1\r\n+readonly\r\n:1\r\n:-1\r\n:1\r\n*6\r\n$10\r\nbrpoplpush\r\n:4\r\n*3\r\n+write\r\n+denyoom\r\n+noscript\r\n:1\r\n:2\r\n:1\r\n*6\r\n$6\r\nzscore\r\n:3\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nsrem\r\n:-3\r\n*2\r\n+write\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$14\r\nzrevrangebylex\r\n:-4\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$8\r\nsetrange\r\n:4\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nmset\r\n:-3\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:-1\r\n:2\r\n*6\r\n$7\r\nunwatch\r\n:1\r\n*2\r\n+noscript\r\n+fast\r\n:0\r\n:0\r\n:0\r\n*6\r\n$8\r\nflushall\r\n:1\r\n*1\r\n+write\r\n:0\r\n:0\r\n:0\r\n*6\r\n$8\r\nrenamenx\r\n:3\r\n*2\r\n+write\r\n+fast\r\n:1\r\n:2\r\n:1\r\n*6\r\n$6\r\ngetbit\r\n:3\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$12\r\nbgrewriteaof\r\n:1\r\n*1\r\n+admin\r\n:0\r\n:0\r\n:0\r\n*6\r\n$9\r\nsubscribe\r\n:-2\r\n*4\r\n+pubsub\r\n+noscript\r\n+loading\r\n+stale\r\n:0\r\n:0\r\n:0\r\n*6\r\n$6\r\nzrange\r\n:-4\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$7\r\nslowlog\r\n:-2\r\n*1\r\n+admin\r\n:0\r\n:0\r\n:0\r\n*6\r\n$4\r\nhget\r\n:3\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$7\r\nhexists\r\n:3\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\nltrim\r\n:4\r\n*1\r\n+write\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\nrpushx\r\n:3\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$16\r\nzrevrangebyscore\r\n:-4\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$9\r\nzlexcount\r\n:4\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\npsync\r\n:3\r\n*3\r\n+readonly\r\n+admin\r\n+noscript\r\n:0\r\n:0\r\n:0\r\n*6\r\n$4\r\ntime\r\n:1\r\n*2\r\n+random\r\n+fast\r\n:0\r\n:0\r\n:0\r\n*6\r\n$11\r\nzunionstore\r\n:-4\r\n*3\r\n+write\r\n+denyoom\r\n+movablekeys\r\n:0\r\n:0\r\n:0\r\n*6\r\n$5\r\nsetnx\r\n:3\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nhset\r\n:4\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$3\r\nttl\r\n:2\r\n*2\r\n+readonly\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\nhmget\r\n:-3\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$3\r\ndel\r\n:-2\r\n*1\r\n+write\r\n:1\r\n:-1\r\n:1\r\n*6\r\n$4\r\ndump\r\n:2\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nmove\r\n:3\r\n*2\r\n+write\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\nwatch\r\n:-2\r\n*2\r\n+noscript\r\n+fast\r\n:1\r\n:-1\r\n:1\r\n*6\r\n$6\r\npsetex\r\n:4\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:1\r\n:1\r\n*6\r\n$4\r\nlpop\r\n:2\r\n*2\r\n+write\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$11\r\nincrbyfloat\r\n:3\r\n*3\r\n+write\r\n+denyoom\r\n+fast\r\n:1\r\n:1\r\n:1\r\n*6\r\n$13\r\nzrangebyscore\r\n:-4\r\n*1\r\n+readonly\r\n:1\r\n:1\r\n:1\r\n*6\r\n$5\r\nbitop\r\n:-4\r\n*2\r\n+write\r\n+denyoom\r\n:2\r\n:-1\r\n:1\r\n*6\r\n$8\r\nreadonly\r\n:1\r\n*1\r\n+fast\r\n:0\r\n:0\r\n:0\r\n*6\r\n$7\r\npfcount\r\n:-2\r\n*1\r\n+readonly\r\n:1\r\n:-1\r\n:1\r\n*6\r\n$7\r\ncommand\r\n:0\r\n*2\r\n+loading\r\n+stale\r\n:0\r\n:0\r\n:0\r\n*6\r\n$5\r\nhmset\r\n:-4\r\n*2\r\n+write\r\n+denyoom\r\n:1\r\n:1\r\n:1\r\n*6\r\n$6\r\ndbsize\r\n:1\r\n*2\r\n+readonly\r\n+fast\r\n:0\r\n:0\r\n:0\r\n";
}

string Worker::auth(int connection, string* password) {
	bool res = Auth::login(connection, password);
	if(res == false) {
		return "-NOAUTH Authentication required.\r\n";
	}
	return "+OK\r\n";
}

string Worker::get(string* key) {
	string res;
	if(dataMap.count(*key) == 0) {
		res = "$-1\r\n";
	} else {
		string value = dataMap[*key];
		stringstream ss;
		int len = value.length();
		ss << "$" << len << "\r\n" << value << "\r\n";
		res = ss.str();
	}
	return res;
}

string Worker::set(string* key, string* value) {
	dataMap[*key] = *value;
	return "+OK\r\n";
}

string Worker::del(string* key) {
	string res;
	int count = dataMap.count(*key);
	if(count > 0) {
		unordered_map<string, string>::iterator item = dataMap.find(*key);
		dataMap.erase(item);
		res = ":1\r\n";
	} else {
		res = ":0\r\n";
	}
	return res;
}

string Worker::multi(int connection) {
	if(multiMap.count(connection) > 0) {
		return "-ERR MULTI calls can not be nested\r\n";
	}
	multiMap[connection] = new list<void**>();
	return "+OK\r\n";
}

string Worker::exec(int connection) {
	if(multiMap.count(connection) == 0) {
		return "-ERR EXEC without MULTI\r\n";
	}
	list<void**>* commandParamList = multiMap[connection];
	//先从map中移除
	multiMap.erase(multiMap.find(connection));
	
	stringstream ss;
	ss << "*" << commandParamList->size() << "\r\n";
	//重新调用work()获取结果
	for(list<void**>::iterator item = commandParamList->begin(); item != commandParamList->end(); item++) {
		string res = work(connection, *item);
		ss << res;
	}
	delete commandParamList;
	
	return ss.str();
}


string Worker::keys(string* param) {
	int resKeySize = 0;
	string resKeys[dataMap.size()];
	
	//遍历所有的key
	for(unordered_map<string, string>::iterator item = dataMap.begin(); item != dataMap.end(); item ++) {
		string key = item->first;
		int keyLen = item->first.length();
		
		int paramLen = (*param).length();
		bool isMultiPass = false;
		int keyStartIndex = 0;
		
		for(int i = 0; i < paramLen; i++) {
			char paramItem = (*param)[i];
			if(paramItem == '*') {
				isMultiPass = true;
				continue;
			}
			
			for(int j = keyStartIndex; j < keyLen; j++) {
				char keyItem = key[j];
				if(paramItem == '*') {
					isMultiPass = true;
					break;
				} else {
					if(keyItem == paramItem) {
						keyStartIndex = j + 1;
						isMultiPass = false;
						break;
					} else {
						if(isMultiPass) {
							keyStartIndex = j + 1;
							//已经扫完所有的keyItem,但仍然未循环完param,直接结束,本次过滤不合格
							if(keyStartIndex >= keyLen) {
								goto end;
							}
							continue;
						} else {
							goto end;
						}
					}
				}
			}
			//param已经遍历完,但keyItem没有扫完,直接结束,本次过滤不合格
			if(keyStartIndex < keyLen && i == paramLen-1) {
				goto end;
			}
		}
		//过滤得到合格的
		resKeys[resKeySize] = key;
		resKeySize ++;
		
		end:;
	}
	
	//输出
	stringstream ss;
	ss << "*" << resKeySize << "\r\n";
	for(int k = 0; k < resKeySize; k++) {
		string key = resKeys[k];
		ss << "$" << key.length() << "\r\n" << key << "\r\n";
	}
	
	delete param;
	return ss.str();
}

void Worker::clear(int connection) {
	if(multiMap.count(connection) > 0) {
		multiMap.erase(multiMap.find(connection));
	}
	Auth::clear(connection);
}

void Worker::freeCommandParam(void** commandParam) {
	for(int i = 0; ; i++) {
		if(commandParam[i] == NULL) {
			break;
		}
		delete (string*)commandParam[i];
	}
	delete [] commandParam;
}
