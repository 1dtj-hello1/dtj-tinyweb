#!/bin/bash
# fix_db.sh

echo "=== 修复数据库配置 ==="

# 1. 启动 MySQL
sudo service mysql start

# 2. 设置 root 密码
sudo mysql << EOF
ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY 'root';
FLUSH PRIVILEGES;
EOF

# 3. 创建数据库和表
mysql -u root -proot << EOF
CREATE DATABASE IF NOT EXISTS yourdb;
USE yourdb;
CREATE TABLE IF NOT EXISTS user(
    username CHAR(50) NULL,
    passwd CHAR(50) NULL
);
INSERT INTO user(username, passwd) VALUES('test', '123456');
SELECT * FROM user;
EOF

# 4. 测试连接
if mysql -u root -proot -e "SELECT 1" 2>/dev/null; then
    echo "✓ 数据库连接成功"
else
    echo "✗ 数据库连接失败"
    exit 1
fi

echo "=== 修复完成 ==="
echo "现在运行: ./server -p 9006"
