

Tinyweb
===============
基于TinyWebServer
对timer的储存进行了修改由链表变为小根堆
对log的异步处理的写入进行了修改使用双缓存consume和produce，写入时写入pro，并创建一个新线程进行consume如果，consumer满了就swap两者。
使用 sendfile 零拷贝发送静态文件
//数据库登录名,密码,库名
string user = "root";
string passwd = "root";
string databasename = "yourdb";
