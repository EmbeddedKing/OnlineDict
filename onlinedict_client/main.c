#include "dict_types.h"
#include "dict_menu.h"

/* 发送和接收协议的变量 */
protocol_t send_msg;
protocol_t recv_msg;
/* 正在登录的账号 */
char loginname[50] = {0};
time_t history_time;

int main(int argc, char **argv)
{
    /* 检查传入参数 */
    if (argc != 3)
    {
        printf("Usage: %s <serip> <serport>", argv[0]);
        return -1;
    }

    /* 创建客户端套接字 */
    int cli_sockfd = 0;
    cli_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (cli_sockfd < 0)
    {
        perror("socket");
        return -1;
    }

    /* 连接服务器 */
    struct sockaddr_in ser_addr;
    memset(&ser_addr, 0, sizeof(struct sockaddr_in));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(atoi(argv[2]));
    ser_addr.sin_addr.s_addr = inet_addr(argv[1]);
    socklen_t ser_addrlen = sizeof(struct sockaddr_in);
    int ret = 0;
    ret = connect(cli_sockfd, (struct sockaddr *)&ser_addr, ser_addrlen);
    if (ret < 0)
    {
        perror("connect");
        return -1;
    }

    int num = 0;
    int onechioce = 0;
    int twochioce = 0;

    while (1)
    {
        /* 打印一级菜单 */
        onemenu_show();
        /* 用户输入选择 */
        printf("input chioce:");
        scanf("%d", &onechioce);
        getchar();
        /* 处理选择 */
        switch (onechioce)
        {
            /* REGISTER */
            case 1:
                /* 清空协议变量，并设置协议变量 */
                memset(&send_msg, 0, sizeof(protocol_t));
                memset(&recv_msg, 0, sizeof(protocol_t));
                send_msg.com_opt = CLI_REGISTER;
                printf("\n-----------------------------\n");
                printf("input account:");
                fgets(send_msg.user.account, sizeof(send_msg.user.account), stdin);
                send_msg.user.account[strlen(send_msg.user.account) - 1] = '\0';
                printf("input passwd:");
                fgets(send_msg.user.passwd, sizeof(send_msg.user.passwd), stdin);
                send_msg.user.passwd[strlen(send_msg.user.passwd) - 1] = '\0';
                printf("-----------------------------\n");
                /* 发送协议 */
                num = send(cli_sockfd, &send_msg, sizeof(protocol_t), 0);
                if (num < 0)
                {
                    perror("send");
                    return -1;
                }

                /* 等待服务器响应 */
                num = recv(cli_sockfd, &recv_msg, sizeof(protocol_t), 0);
                if (num < 0)
                {
                    perror("recv");
                    return -1;
                }
                else if (num == 0)
                {
                    printf("connect break!\n");
                    close(cli_sockfd);
                    return -1;
                }

                /* 解析协议 */
                if (recv_msg.com_opt == SER_SUCCESS)
                {
                    printf("register success!\n");
                }
                else if (recv_msg.com_opt == SER_EXIST)
                {
                    printf("register fail,user is exist\n");
                }
                break;

            /* LOGIN */
            case 2:
                /* 清空协议变量，并设置协议变量 */
                memset(&send_msg, 0, sizeof(protocol_t));
                memset(&recv_msg, 0, sizeof(protocol_t));
                send_msg.com_opt = CLI_LOGIN;
                printf("-----------------------------\n");
                printf("input account:");
                fgets(send_msg.user.account, sizeof(send_msg.user.account), stdin);
                send_msg.user.account[strlen(send_msg.user.account) - 1] = '\0';
                printf("input passwd:");
                fgets(send_msg.user.passwd, sizeof(send_msg.user.passwd), stdin);
                send_msg.user.passwd[strlen(send_msg.user.passwd) - 1] = '\0';
                printf("-----------------------------\n");
                /* 发送登录服务协议 */
                num = send(cli_sockfd, &send_msg, sizeof(protocol_t), 0);
                if (num < 0)
                {
                    perror("send");
                    return -1;
                }

                /* 接收结果 */
                num = recv(cli_sockfd, &recv_msg, sizeof(protocol_t), 0);
                if (num < 0)
                {
                    perror("recv");
                    return -1;
                }
                if (num == 0)
                {
                    printf("connect break!\n");
                    return -1;
                }

                if (recv_msg.com_opt == SER_SUCCESS)
                {
                    printf("login success\n");
                    bzero(loginname, sizeof(loginname));
                    strcpy(loginname, send_msg.user.account);
                    twochioce = 0;
                    while (1)
                    {
                        twomenu_show();
                        printf("input chioce:");
                        scanf("%d", &twochioce);
                        getchar();
                        /* 查找单词 */
                        if (twochioce == 1)
                        {
                            memset(&send_msg, 0, sizeof(protocol_t));
                            memset(&recv_msg, 0, sizeof(protocol_t));
                            send_msg.com_opt = CLI_FIND;
                            printf("input word:");
                            fgets(send_msg.findword, sizeof(send_msg.findword), stdin);
                            send_msg.findword[strlen(send_msg.findword) - 1] = '\0';
                            strcpy(send_msg.username, loginname);
                            history_time = time(NULL);
                            strcpy(send_msg.asctime, asctime(localtime(&history_time)));
                            send_msg.asctime[strlen(send_msg.asctime) - 1] = '\0';
                            num = send(cli_sockfd, &send_msg, sizeof(protocol_t), 0);
                            if (num < 0)
                            {
                                perror("send");
                                return -1;
                            }
                            num = recv(cli_sockfd, &recv_msg, sizeof(protocol_t), 0);
                            if (num < 0)
                            {
                                perror("recv");
                                return -1;
                            }
                            if (num == 0)
                            {
                                printf("connect break\n");
                                return -1;
                            }
                            printf("word:%s\n", recv_msg.findword);
                        }

                        /* 搜索查找历史 */
                        else if (twochioce == 2)
                        {
                            printf("history\n");
                            memset(&send_msg, 0, sizeof(protocol_t));
                            memset(&recv_msg, 0, sizeof(protocol_t));
                            send_msg.com_opt = CLI_HISTORY;
                            strcpy(send_msg.username, loginname);
                            num = send(cli_sockfd, &send_msg, sizeof(protocol_t), 0);
                            if (num < 0)
                            {
                                perror("send");
                                return -1;
                            }
                            /* 接收到SER_HISTORY号结束 */
                            while (1)
                            {
                                num = recv(cli_sockfd, &recv_msg, sizeof(protocol_t), 0);
                                if (num < 0)
                                {
                                    perror("recv");
                                    return -1;
                                }
                                if (num == 0)
                                {
                                    printf("connect break!\n");
                                    return -1;
                                }

                                if (recv_msg.com_opt == SER_HISTORYEND)
                                {
                                    break;
                                }
                                printf("%s %s %s\n", recv_msg.findhistoryname, recv_msg.findhistoryword, recv_msg.findhistorytime);
                            }
                        }
                        else if (twochioce == 3)
                        {
                            printf("return\n");
                            break;
                        }
                    }
                }
                else if (recv_msg.com_opt == SER_PASSWDERR)
                {
                    printf("login fail: passwd error\n");
                }
                else if (recv_msg.com_opt == SER_NOEXIST)
                {
                    printf("login fail: user noexist\n");
                }
                break;

            /* EXIT */
            case 3:
                memset(&send_msg, 0, sizeof(protocol_t));
                memset(&recv_msg, 0, sizeof(protocol_t));
                send_msg.com_opt = CLI_EXIT;
                num = send(cli_sockfd, &send_msg, sizeof(protocol_t), 0);
                if (num < 0)
                {
                    perror("send");
                    return -1;
                }
                /* 等待服务器解析包 */
                num = recv(cli_sockfd, &recv_msg, sizeof(protocol_t), 0);
                if (num < 0)
                {
                    perror("recv");
                    return -1;
                }
                if (num == 0)
                {
                    printf("connect break!\n");
                    close(cli_sockfd);
                    return -1;
                }
                /* 解析协议 */
                if (recv_msg.com_opt == SER_SUCCESS)
                {
                    printf("exit\n");
                    close(cli_sockfd);
                    return 0;
                }

                break;
            default:
                break;
        }

    }
}
