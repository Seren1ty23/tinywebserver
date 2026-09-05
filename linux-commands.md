# Linux 常用命令速查

> 阶段 0 前置知识。覆盖 WSL2 + Ubuntu 开发中每天都会用到的命令，按使用场景分类。
> 配套：`PLAN.md`（总体计划）、`MEMORY.md`（进度与笔记）。

## 0. 三个核心概念

1. **一切皆文件**：普通文件、目录、socket、设备，都是「文件」，都有路径。
2. **路径**：
   - 绝对路径：`/home/lu_yunhao/xxx`（从根 `/` 开始）
   - 相对路径：`.` 当前目录、`..` 上一级、`~` 家目录（`/home/lu_yunhao`）
3. **命令格式**：`命令 [选项] [参数]`；短选项 `-l`、长选项 `--list`。

---

## 1. 导航

| 命令 | 作用 | 例子 |
|------|------|------|
| `pwd` | 显示当前目录 | `pwd` → `/home/lu_yunhao` |
| `ls` | 列出文件 | `ls -l` 详情；`ls -a` 含隐藏；`ls -lh` 人类可读大小 |
| `cd` | 切换目录 | `cd ~` 回家；`cd ..` 上一级；`cd -` 回上一个目录；`cd /mnt/d/mimo_work/tinywebserver` 去项目 |

---

## 2. 文件 / 目录操作

```bash
mkdir tinywebserver       # 建目录；-p 一次建多层：mkdir -p a/b/c
touch main.cpp            # 新建空文件（或刷新时间戳）
cp PLAN.md backup.md      # 复制文件；复制目录加 -r：cp -r src dst
mv old.cpp new.cpp        # 重命名；mv file dir/ 是移动
rm old.cpp                # 删除文件；删目录 rm -r dir；强制 rm -f
```

> ⚠️ Linux 没有回收站，`rm -rf` 删了就没了，动手前先 `ls` 确认路径。

查看文件内容：

```bash
cat main.cpp              # 一次性打印全文（小文件）
less PLAN.md              # 翻页；q 退出；/关键字 搜索
head -n 20 file           # 看前 20 行
tail -f server.log        # 实时追踪日志尾部（日志系统天天用；Ctrl+C 退出）
```

---

## 3. 权限与 sudo

`ls -l` 第一列：`drwxrwxrwx`

```
d        rwx       rwx       rwx
类型     所有者    同组      其他人
d=目录   r读 w写 x执行
```

```bash
chmod u+x build.sh        # 给所有者加执行权限（u=user，+x=加执行）
chmod 755 build.sh        # 数字法：7=rwx 5=r-x 5=r-x
sudo apt install xxx      # 管理员身份执行；要密码且不回显（正常现象）
chown                     # 改属主；入门先了解即可
```

---

## 4. 软件包 apt

```bash
sudo apt update                      # 刷新软件源索引（装新包前先跑）
sudo apt install -y build-essential  # 安装；-y 自动确认
sudo apt remove xxx                  # 卸载
```

---

## 5. 进程管理

```bash
ps aux | grep server       # 查进程；| 管道：左边输出喂给 grep 过滤
top                        # 实时看 CPU/内存；q 退出
kill 12345                 # 正常结束进程；kill -9 12345 强杀（最后手段）
Ctrl+C                     # 终止前台程序
```

---

## 6. 网络（项目高频）

```bash
curl -i http://localhost:9006/                       # 看完整响应头（测服务器必用）
curl -X POST -d "a=1&b=2" http://localhost:9006/     # 发 POST
ping 127.0.0.1                                       # 测连通性
ss -tlnp                                             # 看监听端口（-t tcp -l 监听 -n 数字 -p 进程）
ip addr                                              # 看本机 IP（hostname -I 更简短）
```

---

## 7. 搜索

```bash
grep -rn "main" .          # 当前目录递归搜"main"；-r 递归 -n 行号 -i 忽略大小写
find . -name "*.cpp"       # 按文件名找所有 .cpp 文件
```

---

## 8. 查帮助

```bash
g++ --help                 # 多数命令支持 --help
man 2 socket               # 手册：2=系统调用 3=库函数 7=杂项
man 7 epoll                # 本项目会用到
```

---

## 9. 提速小技巧

- **Tab 补全**：输入前几个字母按 Tab 自动补全
- **通配符**：`*.cpp` 匹配所有 .cpp 文件
- `Ctrl+A` 行首、`Ctrl+E` 行尾、`Ctrl+R` 搜索历史命令、`history` 看历史
- `clear` 清屏
