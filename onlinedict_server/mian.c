#include "dict_types.h"

/* 连接套接字变量 */
int con_sockfd = 0;
/* 发送和接收协议的变量 */
protocol_t recv_msg;
protocol_t send_msg;
/* 词典文件指针 */
FILE *dictfp = NULL;

int his_callback(void *data, int fnum, char **f_value, char **f_name)
{
    memset(&send_msg, 0, sizeof(protocol_t));
    send_msg.com_opt = SER_SUCCESS;
    strcpy(send_msg.findhistoryname, f_value[0]);
    strcpy(send_msg.findhistoryword, f_value[1]);
    strcpy(send_msg.findhistorytime, f_value[2]);
    int num = send(con_sockfd, &send_msg, sizeof(protocol_t), 0);
    if (num < 0)
    {
        perror("send");
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    /* 检查参数传入错误 */
    if (argc != 3)
    {
        printf("Usage: %s <serip> <serport>", argv[0]);
        return -1;
    }

    /* 创建套接字 */
    int lis_sockfd = 0;
    lis_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (lis_sockfd < 0)
    {
        perror("socket");
        return -1;
    }

    /* 绑定IP地址和端口 */
    struct sockaddr_in ser_addr;
    memset(&ser_addr, 0, sizeof(struct sockaddr_in));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(atoi(argv[2]));
    ser_addr.sin_addr.s_addr = inet_addr(argv[1]);
    socklen_t ser_addrlen = sizeof(struct sockaddr_in);
    int ret = 0;
    ret = bind(lis_sockfd, (struct sockaddr *)&ser_addr, ser_addrlen);
    if (ret < 0)
    {
        perror("bind");
        return -1;
    }

    /* 监听该端口 */
    ret = listen(lis_sockfd, 20);
    if (ret < 0)
    {
        perror("listen");
        return -1;
    }


    struct sockaddr_in cli_addr;
    memset(&cli_addr, 0, sizeof(struct sockaddr_in));
    socklen_t cli_addrlen = sizeof(struct sockaddr_in);
    printf("onlinedict find service is on\n");
    while (1)
    {
        /* 处理连接请求 */
        con_sockfd = accept(lis_sockfd, (struct sockaddr *)&cli_addr, &cli_addrlen);
        if (con_sockfd < 0)
        {
            perror("accept");
            return -1;
        }
        printf("add client ip:%s port:%hu\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        /* 创建多进程并发服务器 */
        pid_t branch = 0;
        branch = fork();
        if (branch < 0)
        {
            perror("fork");
            return -1;
        }
        /* 子进程处理客户端服务请求 */
        else if (branch == 0)
        {
            close(lis_sockfd);
            int num = 0;


            /* 用于数据库操作的变量 */
            sqlite3 *dict_db = NULL;
            char sqlbuf[100] = {0};
            char *sqlerrmsg = NULL;
            int sqlflag = 0;
            char **sqllist = NULL;
            int sqlrow = 0;
            int sqlcolumn = 0;

            /* 打开数据库 */
            ret = sqlite3_open("onlinedict.db", &dict_db);
            if (ret != SQLITE_OK)
            {
                printf("sqlite3_open:%s\n", sqlite3_errmsg(dict_db));
                return -1;
            }

            while (1)
            {
                /* 接收客户端发送过来的协议 */
                memset(&recv_msg, 0, sizeof(protocol_t));
                num = recv(con_sockfd, &recv_msg, sizeof(protocol_t), 0);
                if (num < 0)
                {
                    perror("recv");
                    return -1;
                }

                else if (num == 0)
                {
                    printf("connect break!\n");
                    close(con_sockfd);
                    return -1;
                }

                switch (recv_msg.com_opt) {
                    case CLI_REGISTER:
                        bzero(sqlbuf, sizeof(sqlbuf));
                        sqlrow = 0;
                        sqlflag = 0;
                        sqlcolumn = 0;
                        sqllist = NULL;
                        memset(&send_msg, 0, sizeof(protocol_t));
                        /* 查询要注册的账号名在数据库中是否存在 */
                        sprintf(sqlbuf, "select * from user_info where account = \"%s\"", recv_msg.user.account);
                        ret = sqlite3_get_table(dict_db, sqlbuf, &sqllist, &sqlrow, &sqlcolumn, &sqlerrmsg);
                        if (ret != SQLITE_OK)
                        {
                            printf("sqlite3_exec:%s", sqlerrmsg);
                            return -1;
                        }
                        /* 如果sqlflag被置为其他数，则证明已经找到 */
                        if (sqlrow != 0)
                        {
                            send_msg.com_opt = SER_EXIST;
                            num = send(con_sockfd, &send_msg, sizeof(send_msg), 0);
                            if (num < 0)
                            {
                                perror("send");
                                return -1;
                            }
                        }
                        /* 否则没有找到，进行插入操作并通知客户端已经成功 */
                        else
                        {
                            /* 数据库插入操作 */
                            sprintf(sqlbuf, "insert into user_info values(\"%s\",\"%s\");", recv_msg.user.account, recv_msg.user.passwd);
                            ret = sqlite3_exec(dict_db, sqlbuf, NULL, NULL, &sqlerrmsg);
                            if (ret != SQLITE_OK)
                            {
                                printf("sqlite3_exec:%s", sqlerrmsg);
                                return -1;
                            }
                            /* 通知客户端创建成功 */
                            send_msg.com_opt = SER_SUCCESS;
                            num = send(con_sockfd, &send_msg, sizeof(send_msg), 0);
                            if (num < 0)
                            {
                                perror("send");
                                return -1;
                            }
                        }
                        sqlite3_free_table(sqllist);
                        break;

                    /* 用户发送登录请求 */
                    case CLI_LOGIN:
                        bzero(sqlbuf, sizeof(sqlbuf));
                        sqlflag = 0;
                        memset(&send_msg, 0, sizeof(protocol_t));
                        /* 数据库查询操作 */
                        sprintf(sqlbuf, "select * from user_info where account = \"%s\";", recv_msg.user.account);
                        ret = sqlite3_get_table(dict_db, sqlbuf, &sqllist, &sqlrow, &sqlcolumn, &sqlerrmsg);
                        //ret = sqlite3_exec(dict_db, sqlbuf, reg_callback, &sqlflag, &sqlerrmsg);
                        if (ret != SQLITE_OK)
                        {
                            printf("sqlite3_exec:%s", sqlerrmsg);
                            return -1;
                        }
                        /* 已找到 */
                        if (sqlrow != 0)
                        {
                            if (strcmp(*(sqllist+sqlcolumn+1), recv_msg.user.passwd) == 0)
                            {
                                printf("%s login success!\n", *(sqllist+sqlcolumn));
                                send_msg.com_opt = SER_SUCCESS;
                                num = send(con_sockfd, &send_msg, sizeof(send_msg), 0);
                                if (num < 0)
                                {
                                    perror("send");
                                    return -1;
                                }
                            }
                            else
                            {
                                send_msg.com_opt = SER_PASSWDERR;
                                num = send(con_sockfd, &send_msg, sizeof(send_msg), 0);
                                if (num < 0)
                                {
                                    perror("send");
                                    return -1;
                                }
                            }

                        }
                        /* 未找到 */
                        else
                        {
                            send_msg.com_opt = SER_NOEXIST;
                            num = send(con_sockfd, &send_msg, sizeof(send_msg), 0);
                            if (num < 0)
                            {
                                perror("send");
                                return -1;
                            }
                        }
                        break;
                    /* exit */
                    case CLI_EXIT:
                        memset(&send_msg, 0, sizeof(protocol_t));
                        send_msg.com_opt = SER_SUCCESS;
                        num = send(con_sockfd, &send_msg, sizeof(protocol_t), 0);
                        if (num < 0)
                        {
                            perror("send");
                            return -1;
                        }
                        close(con_sockfd);
                        return 0;
                        break;

                    case CLI_FIND:
                        //printf("username: %s,findword: %s, time:%s\n", recv_msg.username, recv_msg.findword, recv_msg.asctime);
                        bzero(sqlbuf, sizeof(sqlbuf));
                        sprintf(sqlbuf, "insert into history_info values(\"%s\", \"%s\", \"%s\");", recv_msg.username, recv_msg.findword, recv_msg.asctime);
                        ret = sqlite3_exec(dict_db, sqlbuf, NULL, NULL, &sqlerrmsg);
                        if (ret != SQLITE_OK)
                        {
                            printf("sqlite3_exec:%s\n", sqlerrmsg);
                            return -1;
                        }

                        memset(&send_msg, 0, sizeof(protocol_t));
                        dictfp = fopen("dict.data", "r");
                        while (!feof(dictfp))
                        {
                            bzero(send_msg.findword, sizeof(send_msg.findword));
                            fgets(send_msg.findword, sizeof(send_msg.findword), dictfp);
                            if (strncmp(recv_msg.findword, send_msg.findword, strlen(recv_msg.findword)) == 0)
                            {
                                break;
                            }
                        }
                        if (feof(dictfp))
                        {
                            bzero(send_msg.findword, sizeof(send_msg.findword));
                            strcpy(send_msg.findword, "word not fount");
                        }
                        num = send(con_sockfd, &send_msg, sizeof(protocol_t), 0);
                        if (num < 0)
                        {
                            perror("send");
                            return -1;
                        }
                        fclose(dictfp);
                        break;

                    /* 发送历史 */
                    case CLI_HISTORY:
                        sprintf(sqlbuf, "select * from history_info where name = \"%s\";", recv_msg.username);
                        ret = sqlite3_exec(dict_db, sqlbuf, his_callback, NULL, &sqlerrmsg);
                        if (ret != SQLITE_OK)
                        {
                            printf("sqlite3_exec:%s", sqlerrmsg);
                            return -1;
                        }
                        memset(&send_msg, 0, sizeof(protocol_t));
                        send_msg.com_opt = SER_HISTORYEND;
                        int num = send(con_sockfd, &send_msg, sizeof(protocol_t), 0);
                        if (num < 0)
                        {
                            perror("send");
                            return -1;
                        }
                        break;

                }
            }
        }

        /* 父进程继续处理客户端请求 */
        else
        {
            close(con_sockfd);
        }
    }
}
