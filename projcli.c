#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Macro variables Declaration */
#define PORTNOA 16011
#define PORTNOB 16111
#define MAXLINE 100
#define NAME_LEN 20
#define CHAT_MAX 6

/* Global variables Declaration */
char *SERVER_ADDR = "*.*.*.*";
char *BLINK_MSG = "                                                        ";
// sockA=채팅서버, sockB=게임서버
int sockA, sockB;

/* Struct */
struct SuddaUser{
    char name[NAME_LEN];
    int money;
    int card_first;
    int card_second;
    int card_combine;
    int alive;
};

/* Functions Declaration */
void client_chat(char *nickname);
void client_input(char *nickname);
void client_game(char *nickname);
int getch(void);
void errquit(char *mesg);
int tcp_connect(int af, char *servip, unsigned short port);
void move_cur(int x, int y);
void change_fb(void);
void change_fg(int color);
void change_bg(int color);
void initial_setting(struct SuddaUser user[]);
void print_ui(char *msg, int px, int py, int fg_color, int bg_color);
void print_rule(int input);
void print_table(int input, struct SuddaUser *user, int turn);
void print_amount(int amount, int call);
void print_chatting(void);
void print_button(int turn);
void print_timer(int input, int own);

/* main Function */
int main(int argc, char *argv[]){
    pid_t pid[2];

    if(argc != 2){
        printf("사용법 : [%s] [username]\n", argv[0]);
        exit(0);
    }

    sockA = tcp_connect(AF_INET, SERVER_ADDR, PORTNOA);
    if(sockA==-1)
        errquit("ERROR :: tcp_connect 1st fail");
    sockB = tcp_connect(AF_INET, SERVER_ADDR, PORTNOB);
    if(sockB==-1)
        errquit("ERROR :: tcp_connect 2nd fail");

    if((pid[0] = fork()) < 0){
        errquit("ERROR :: fork error\n");
    }else if(pid[0] == 0){
        client_chat(argv[1]);
    }else{
        if((pid[1] = fork()) < 0){
            errquit("ERROR :: fork error\n");
        }else if(pid[1] == 0){
            client_game(argv[1]);
        }else{
            client_input(argv[1]);
        }
    }
    return 0;
}

/* Function Part Start */
void client_chat(char *nickname){
    char bufall[MAXLINE+NAME_LEN], *bufmsg;
    char chatlist[6][MAXLINE+NAME_LEN];
    fd_set read_fds;
    int maxfdp, chat_num = 0;
    int namelen, nbyte;
    int i,j;

    usleep(300000);
    sprintf(bufall, "[%s] : ", nickname);
    namelen = strlen(bufall);
    bufmsg = bufall+namelen;
    maxfdp = sockB + 1;

    FD_ZERO(&read_fds);
    while(1){
        FD_SET(sockA, &read_fds);
        if(select(maxfdp, &read_fds, NULL, NULL, NULL)<0)
            errquit("ERROR :: select fail");
        if(FD_ISSET(sockA, &read_fds)){
            if((nbyte = recv(sockA, bufmsg, MAXLINE, 0))>0){
                bufmsg[nbyte] = 0;
                if(chat_num == CHAT_MAX){
                    for(i=0; i<CHAT_MAX; i++){
                        print_ui(BLINK_MSG,6,37+i,30,47);
                        print_ui(BLINK_MSG,-1,-1,30,47);
                        if((i+1)!=CHAT_MAX)
                            strcpy(chatlist[i],chatlist[i+1]);
                        else
                            strcpy(chatlist[CHAT_MAX-1],bufmsg);
                    }
                }else{
                    strcpy(chatlist[chat_num++],bufmsg);
                }
                for(i=0; i<chat_num; i++){
                    print_ui(chatlist[i],6,37+i,30,47);
                    print_ui("",7,44,30,47);
                }
            }
        }
    }
}

void client_input(char *nickname){
    char bufall[MAXLINE+NAME_LEN], *bufmsg;
    char buf[20], name[100];
    char one_char;
    fd_set read_fds;
    int maxfdp, namelen;
    int input_type = 0;
    int i,j;

    sprintf(bufall, "[%s] :", nickname);
    namelen = strlen(bufall);
    bufmsg = bufall+namelen;
    maxfdp = sockB + 1;

    FD_ZERO(&read_fds);
    print_ui(BLINK_MSG,6,44,37,40);
    print_ui(BLINK_MSG,-1,-1,37,40);
    print_ui("",7,44,37,40);
    sprintf(name,"a@%s",nickname);
    if(send(sockB, name, strlen(name), 0)<0){
        puts("ERROR :: Write error on socket.");
    }

    while(1){
        FD_SET(0, &read_fds);
        if(input_type==1){
            print_ui("  << 명령어 입력모드",9,44,43,44);
            print_ui("",7,44,37,40);
            if((one_char=getch())=='\n'){
                input_type=0;
                print_ui(BLINK_MSG,6,44,37,40);
                print_ui(BLINK_MSG,-1,-1,37,40);
                print_ui("",7,44,37,40);
            }else if(one_char=='1' || one_char=='2' || one_char=='3' || one_char=='q' || one_char=='w' || one_char=='e'){
                sprintf(buf,"b@%c",one_char);
                if(send(sockB, buf, strlen(buf), 0)<0)
                    puts("ERROR :: Write error on socket.");
                printf("%c",one_char);
                print_ui("",7,44,37,40);
            }
        }else{
            if(select(maxfdp, &read_fds, NULL, NULL, NULL)<0){
                errquit("ERROR :: select fail");
            }else if(FD_ISSET(0, &read_fds)){
                if(fgets(bufmsg, MAXLINE, stdin)){
                    if(strcmp(bufmsg, "\n") == 0){
                        input_type = 1;
                        print_ui(BLINK_MSG,6,44,30,44);
                        print_ui(BLINK_MSG,-1,-1,30,44);
                        continue;
                    }
                    bufall[namelen+strlen(bufmsg)-1]='\n';
                    if(send(sockA, bufall, namelen+strlen(bufmsg), 0)<0){
                        puts("ERROR :: Write error on socket.");
                    }else{
                        print_ui(BLINK_MSG,6,44,37,40);
                        print_ui(BLINK_MSG,-1,-1,37,40);
                        print_ui("",7,44,37,40);
                    }
                }
            }
        }
    }
}

void client_game(char *nickname){
    char bufall[MAXLINE+NAME_LEN], *bufmsg, *ptr;
    int namelen, nbyte;
    int maxfdp, user_num;
    int i, j, set = 1;
    int turn;
    fd_set read_fds;
    struct SuddaUser user[5];

    // 화면 및 초기설정
    for(i=0; i<5; i++){
        if(i==0)
            strcpy(user[i].name,nickname);
        else
            strcpy(user[i].name,"+");
        user[i].money = 0;
        user[i].card_first = -1;
        user[i].card_second = -1;
        user[i].card_combine = -1;
        user[i].alive = 1;
    }
    move_cur(0,0);
    change_fb();
    system("clear");
    initial_setting(user);
    print_chatting();

    maxfdp = sockB + 1;
    FD_ZERO(&read_fds);
    while(1){
        FD_SET(sockB, &read_fds);
        if(select(maxfdp, &read_fds, NULL, NULL, NULL)<0)
            errquit("ERROR :: select fail");
        if(FD_ISSET(sockB, &read_fds)){
            if((nbyte = recv(sockB, bufmsg, MAXLINE, 0))>0){
                bufmsg[nbyte] = 0;
                print_ui(bufmsg,0,0,37,45);
                print_ui("",7,44,37,40);
            }

            if(bufmsg[0] == '9'){
                user_num = 1;
                turn = 0; // 지우면 안 됨
                set = 1;
                for(i=0; i<5; i++){
                    if(i!=0){
                        strcpy(user[i].name,"+");
                        user[i].money = 0;
                    }
                    user[i].card_first = -1;
                    user[i].card_first = -1;
                    user[i].card_combine = -1;
                    user[i].alive = 1;
                }
                initial_setting(user);
            }else if(bufmsg[0] == '1'){
                ptr = strtok(bufmsg,"@");
                ptr = strtok(NULL, "@");
                if(strcmp(nickname,ptr)==0){
                    ptr = strtok(NULL, "@");
                    user[0].money = atoi(ptr);
                    ptr = strtok(NULL, "@");
                    user[0].card_first = atoi(ptr);
                    ptr = strtok(NULL, "@");
                    user[0].card_second = atoi(ptr);
                    ptr = strtok(NULL, "@");
                    user[0].card_combine = atoi(ptr);
                    ptr = strtok(NULL, "@");
                    user[0].alive = atoi(ptr);
                    if(user[0].card_combine!=-1)
                        print_rule(user[0].card_combine);
                    print_table(0,&user[0],0);
                }else if(set == 1){
                    strcpy(user[user_num].name,ptr);
                    ptr = strtok(NULL, "@");
                    user[user_num].money = atoi(ptr);
                    ptr = strtok(NULL, "@");
                    user[user_num].card_first = atoi(ptr);
                    ptr = strtok(NULL, "@");
                    user[user_num].card_second = atoi(ptr);
                    ptr = strtok(NULL, "@");
                    user[user_num].card_combine = atoi(ptr);
                    ptr = strtok(NULL, "@");
                    user[user_num].alive = atoi(ptr);
                    print_table(user_num,&user[user_num],0);
                    user_num++;
                }else if(set ==0){
                    for(j=1; j<user_num; j++){
                        if(strcmp(user[j].name,ptr)==0){
                            ptr = strtok(NULL, "@");
                            user[j].money = atoi(ptr);
                            ptr = strtok(NULL, "@");
                            user[j].card_first = atoi(ptr);
                            ptr = strtok(NULL, "@");
                            user[j].card_second = atoi(ptr);
                            ptr = strtok(NULL, "@");
                            user[j].card_combine = atoi(ptr);
                            ptr = strtok(NULL, "@");
                            user[j].alive = atoi(ptr);
                            print_table(j,&user[j],0);
                        }
                    }
                }
            }else if(bufmsg[0] == '2'){
                set = 0;
                for(i=0; i<user_num; i++)
                    print_table(i,&user[i],0);
            }else if(bufmsg[0] == '3'){
                int amount, call;
                ptr = strtok(bufmsg,"@");
                ptr = strtok(NULL, "@");
                amount = atoi(ptr);
                ptr = strtok(NULL, "@");
                call = atoi(ptr);
                print_amount(amount,call);
            }else if(bufmsg[0] == '4'){
                print_table(0,&user[0],1);
                ptr = strtok(bufmsg,"@");
                ptr = strtok(NULL, "@");
                print_button(atoi(ptr));
            }else if(bufmsg[0] == '5'){
                print_table(0,&user[0],0);
                print_button(2);
            }
        }
    }
}

int getch(void){
    int ch;
    struct termios old, new;

    tcgetattr(0, &old);
    new = old;
    new.c_lflag &= ~(ICANON | ECHO);
    new.c_cc[VMIN] = 1;
    new.c_cc[VTIME] = 0;
    tcsetattr(0, TCSAFLUSH, &new);
    ch = getchar();
    tcsetattr(0, TCSAFLUSH, &old);
    return ch;
}

void errquit(char *mesg){
    perror(mesg);
    exit(1);
}

int tcp_connect(int af, char *servip, unsigned short port){
    struct sockaddr_in servaddr;
    int s;

    if((s = socket(af, SOCK_STREAM, 0))<0)
        return -1;
    bzero((char *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = af;
    inet_pton(AF_INET, servip, &servaddr.sin_addr);
    servaddr.sin_port = htons(port);

    if(connect(s, (struct sockaddr *)&servaddr, sizeof(servaddr))<0)
        return -1;
    return s;
}

void move_cur(int x, int y){
    printf("\033[%dd\033[%dG", y, x);
}
void change_fb(void){
    printf("\x1b[0m");
}
void change_fg(int color){
    printf("\x1b[%dm",color);
}
void change_bg(int color){
    printf("\x1b[%dm",color);
}

void initial_setting(struct SuddaUser user[]){
    int i;
    for(i=0; i<5; i++)
        print_table(i, &user[i], 0);
    print_rule(user[0].card_combine);
    print_button(2);
    print_amount(0,1000);
    print_ui("",7,44,30,47);
}

void print_ui(char *msg, int px, int py, int fg_color, int bg_color){
    if(px!=-1 && py!=-1)
        move_cur(px,py);
    if(fg_color!=-1)
        change_fg(fg_color);
    if(bg_color!=-1)
        change_bg(bg_color);
    printf("%s",msg);
    change_fb();
    fflush(stdout);
}

void print_rule(int input){
    int i;
    char *rule_msg[]={
        "         [  족보  ]          ",
        " 01. 3∙8광땡                 ",
        " 02. 1∙8광땡                 ",
        " 03. 1∙3광땡                 ",
        " 04. 장땡(10땡)              ",
        " 05. 9땡                     ",
        " 06. 8땡                     ",
        " 07. 7땡                     ",
        " 08. 6땡                     ",
        " 09. 5땡                     ",
        " 10. 4땡                     ",
        " 11. 3땡                     ",
        " 12. 2땡                     ",
        " 13. 1땡                     ",
        " 14. 알리(1∙2)               ",
        " 15. 독사(1∙4)               ",
        " 16. 구삥(9∙1)               ",
        " 17. 장삥(10∙1)              ",
        " 18. 장사(10∙4)              ",
        " 19. 세륙(4∙6)               ",
        " 20. 갑오(9끗)               ",
        " 21. 8끗                     ",
        " 22. 7끗                     ",
        " 23. 6끗                     ",
        " 24. 5끗                     ",
        " 25. 4끗                     ",
        " 26. 3끗                     ",
        " 27. 2끗                     ",
        " 28. 1끗                     ",
        " 29. 망통(0끗)               ",
        " 30. 4∙7암행어사             ",
        " 31. 3∙7땡잡이               ",
        " 32. 멍텅구리구사            ",
        " 33. 구사(9∙4)               "
    };

    if(input != -1){
        print_ui(rule_msg[input],123,input+5,37,41);
    }else{
        for(i=0; i<(sizeof(rule_msg)/8); i++){
            print_ui("=",122,i+5,30,42);
            if(i==0)
                print_ui(rule_msg[i],-1,-1,30,46);
            else if(i!=0)
                print_ui(rule_msg[i],-1,-1,30,47);
            print_ui("=",-1,-1,30,42);
        }
    }
    print_ui("",7,44,37,40);
}

void print_table(int input, struct SuddaUser *user, int turn){
    int i,j;
    int px, py;
    char buf[100], cardName[10];
    char *card_msg[]={
        "┌─────┐",
        "│     │",
        "│     │",
        "│     │",
        "│     │",
        "└─────┘"
    };
    char *usertag_msg[]={
        "┌───────────────┐",
        "│               │",
        "│               │",
        "│               │",
        "└───────────────┘"
    };
    char *table_msg[]={
        "┌───────────────────────────────────┐",
        "│                                   │",
        "│                                   │",
        "│                                   │",
        "│                                   │",
        "│                                   │",
        "│                                   │",
        "│                                   │",
        "│                                   │",
        "└───────────────────────────────────┘"
    };

    switch(input){
        case 0:
            px = 43;
            py = 25;
            break;
        case 1:
            px = 5;
            py = 3;
            break;
        case 2:
            px = 81;
            py = 3;
            break;
        case 3:
            px = 5;
            py = 14;
            break;
        case 4:
            px = 81;
            py = 14;
            break;
        default:
            break;
    }

    // 테이블 출력
    for(i=0; i<(sizeof(table_msg)/8); i++){
        if(input==0 && turn==1){
            print_ui(table_msg[i],px,py+i,30,46);
        }else{
            print_ui(table_msg[i],px,py+i,37,40);
        }
    }
    // 유저 이름 및 보유금액 출력
    for(i=0; i<(sizeof(usertag_msg)/8); i++){
        if(user->alive==1)
            print_ui(usertag_msg[i],px+2,py+i+3,30,47);
        else
            print_ui(usertag_msg[i],px+2,py+i+3,37,40);
    }
    if(strcmp(user->name,"+") != 0){
        print_ui(user->name,px+3,py+4,30,42);
        sprintf(buf,"%d원",user->money);
        print_ui(buf,px+4,py+5,37,40);
    }
    // 카드 출력
    for(i=0; i<2; i++){
        for(j=0; j<(sizeof(card_msg)/8); j++){
            print_ui(card_msg[j],px+i*8+20,py+j+2,37,41);
        }
    }
    if(user->card_first != -1){
        if(user->card_first==1 || user->card_first==21 || user->card_first==71)
            sprintf(cardName,"%s","광");
        else if(user->card_first==31 || user->card_first==61 || user->card_first==81)
            sprintf(cardName,"%s","끗");
        else
            sprintf(cardName,"%s","");
        sprintf(buf,"%d%s",(user->card_first)/10+1,cardName);
        print_ui(buf,px+22,py+3,30,43);
    }
    if(user->card_second != -1){
        if(user->card_second==1 || user->card_second==21 || user->card_second==71)
            sprintf(cardName,"%s","광");
        else if(user->card_second==31 || user->card_second==61 || user->card_second==81)
            sprintf(cardName,"%s","끗");
        else
            sprintf(cardName,"%s","");
        sprintf(buf,"%d%s",(user->card_second)/10+1,cardName);
        print_ui(buf,px+30,py+3,30,43);
    }
    print_ui("",7,44,30,47);
}

void print_amount(int amount, int call){
    int i;
    char msg[100];
    char *table_msg[]={
        "┌──────────────────────────────────┐",
        "│                                  │",
        "│                                  │",
        "├──────────────────────────────────┤",
        "│                                  │",
        "│                                  │",
        "└──────────────────────────────────┘"
    };
    for(i=0; i<(sizeof(table_msg)/8); i++){
        print_ui(table_msg[i],44,10+i,37,40);
    }

    sprintf(msg," %d 원",amount);
    print_ui(" 총 액 ",46,11,30,46);
    print_ui(msg,48,12,30,47);
    sprintf(msg," %d 원",call);
    print_ui(" 콜비용 ",46,14,30,46);
    print_ui(msg,48,15,30,47);
    print_ui("",7,44,30,47);
}

void print_chatting(void){
    int i;
    char *chatting_msg[]={
        "┌─ < CHATTING > ─────────────────────────────────────────────────────────────────────────────────────────────────┐",
        "│                                                                                                                │",
        "│                                                                                                                │",
        "│                                                                                                                │",
        "│                                                                                                                │",
        "│                                                                                                                │",
        "│                                                                                                                │",
        "├────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤",
        "│                                                                                                                │",
        "└────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘"
    };

    for(i=0; i<(sizeof(chatting_msg)/8); i++){
        print_ui(chatting_msg[i],5,36+i,30,47);
    }
    print_ui("",7,44,30,47);
}

void print_button(int turn){
    int i,j,n;
    char *button_msg[]={
        "┌────────┐",
        "│        │",
        "└────────┘"
    };
    char *option_msg[]={
        " 1 다이 ",
        " 2  삥  ",
        " 3 따당 ",
        " q  콜  ",
        " w 쿼터 ",
        " e 하프 "
    };

    // 버튼박스 출력
    for(i=0;i<2;i++){
        for(j=0;j<3;j++){
            for(n=0;n<(sizeof(button_msg)/8);n++){
                if((turn==1 && j!=0) || turn==2)
                    print_ui(button_msg[n],84+j*12,26+i*5+n,37,40);
                else
                    print_ui(button_msg[n],84+j*12,26+i*5+n,30,46);
            }
        }
    }
    // 버튼이름 출력
    for(i=0;i<2;i++){
        for(j=0;j<3;j++){
            print_ui(option_msg[i*3+j],85+j*12,27+i*5,37,40);
        }
    }
    print_ui("",7,44,30,47);
}

void print_timer(int input, int own){
    int i;
    int fg, bg;
    char ment[30];
    char *bombline[]={
        "~!",
        "~☜",
        "~☜☜",
        "~☜☜☜",
        "~☜☜☜☜",
        "~☜☜☜☜☜"
    };
    char *bombmotion[]={
        "☆☆☆☆☆☆☆☆☆☆",
        "■■--------",
        "■■■■------",
        "■■■■■■----",
        "■■■■■■■■--",
        "■■■■■■■■■■"
    };
    char *timer_msg[]={
        "┌─TIMER─────────────┐",
        "│                   │",
        "│                   │",
        "│                   │",
        "└───────────────────┘"
    };

    if(own == 1){
        fg = 34;
        bg = 47;
    }else{
        fg = 30;
        bg = 46;
    }
    for(i=0;i<(sizeof(timer_msg)/8);i++){
        print_ui(timer_msg[i],15,28+i,fg,bg);
    }
    if(input != -1){
        sprintf(ment," [ %d 초]%s ",input,bombline[input]);
        print_ui(ment,17,29,31,bg);
        sprintf(ment," [ %s ] ",bombmotion[input]);
        print_ui(ment,17,31,31,bg);

    }
    print_ui("",7,44,30,47);
}
