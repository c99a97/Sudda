#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/* Macro variables Declaration */
#define PORTNOA 16011
#define PORTNOB 16111
#define MAX_SOCK 128 
#define MAX_LINE 511

/* Global variables */
char *START_STRING = "Connected to chat server";
int num_chatA = 0;
int num_chatB = 0;
int clisock_listA[MAX_SOCK];
int clisock_listB[MAX_SOCK];

/* Struct variables */
struct SuddaUser{
    char name[20];
    int money;
    int card_first;
    int card_second;
    int card_combine;
    int alive;
};

/* Function Declaration */
void server_chat(void);
void server_game(void);
void errquit(char *msg);
void addClient(int s, struct sockaddr_in *newcliaddr, int type);
void removeClient(int s,int type);
int is_nonblock(int sockfd);
int set_nonblock(int sockfd);
int tcp_listen(int host, int port, int backlog);
int decide_combination(int card_first, int card_second);
int decide_winner(struct SuddaUser user[]);
int check_combine(struct SuddaUser user[], int checkList[], int check_n);
int cal_pay(int wallet, int pay);
int gamemaster(struct SuddaUser user[], int pan_money, int *amount_money, int type);
void send_userinfo(struct SuddaUser user[], int user_no);
void send_callinfo(int amount, int call, int user_no);
void send_turninfo(int user_no, int turn, int turnno);

/* main Fuction */
int main(int argc, char *argv[]){
    pid_t pid;
    
    if((pid = fork()) < 0){
        errquit("ERROR :: fork error\n");
    }else if(pid == 0){
        server_game();
    }else{
        server_chat();
    }
    return 0;
}

/* Function Start */
void server_chat(void){
    struct sockaddr_in cliaddr;
    int accp_sock, listen_sock;
    int clilen, addrlen=sizeof(struct sockaddr_in);
    int i, j, nbyte, count = 400000;
    char buf[MAX_LINE+1];

    listen_sock = tcp_listen(INADDR_ANY, PORTNOA, 10);
    if(listen_sock == -1){
        errquit("ERROR :: listen fail");
    }
    if(set_nonblock(listen_sock) == -1){
        errquit("ERROR :: set_nonblock fail");
    }

    while(1){
        if(count++ > 400000){
            printf("CHAT SERVER IS RUNNING\n");
            count = 0;
        }
        accp_sock = accept(listen_sock, (struct sockaddr *)&cliaddr, &clilen);
        if(accp_sock == -1 && errno!=EWOULDBLOCK){
            errquit("ERROR :: accept fail");
        }else if(accp_sock > 0){
            if(is_nonblock(accp_sock)!=0 && set_nonblock(accp_sock)<0){
                errquit("ERROR :: set_nonblock fail");
            }
            addClient(accp_sock, &cliaddr, 0);
            send(accp_sock, START_STRING, strlen(START_STRING), 0);
            sprintf(buf,"다른 사용자가 접속했습니다.");
            for(i=0; i<num_chatA; i++){
                if(accp_sock != clisock_listA[i])
                    send(clisock_listA[i], buf, strlen(buf), 0);
            }
            printf("CHAT SERVER : %d 번째 사용자 추가\n", num_chatA);
        }

        // 메시지 수신 및 전송
        for(i=0; i<num_chatA; i++){
            errno = 0;
            nbyte = recv(clisock_listA[i], buf, MAX_LINE, 0);
            if(nbyte==-1 && errno==EWOULDBLOCK){
                continue;
            }else if(nbyte==-1 || nbyte==0){
                removeClient(i, 0);
                continue;
            }
            buf[nbyte]=0;
            for(j=0; j<num_chatA; j++){
                send(clisock_listA[j], buf, nbyte, 0);
            }
            printf("CHAT SERVER : %s\n",buf);
        }
    }
}
void server_game(void){
    struct sockaddr_in cliaddr;
    struct SuddaUser user[5];
    time_t start_time,diff_time;
    int listen_sock, accp_sock;
    int clilen, addrlen=sizeof(struct sockaddr_in);
    int user_num, winner, amount_money;
    int i, nbyte, count = 0;
    char buf[MAX_LINE+1];
    char *ptr;

    srand(time(NULL));

    listen_sock = tcp_listen(INADDR_ANY, PORTNOB, 10);
    if(listen_sock == -1){
        errquit("ERROR :: listen fail");
    }
    if(set_nonblock(listen_sock) == -1){
        errquit("ERROR :: set_nonblock fail");
    }

    while(1){
        start_time = time(NULL);
        while(1){
            if(count++ > 200000){
                printf("GAME SERVER IS RUNNING\n");
                count = 0;
            }
            accp_sock = accept(listen_sock, (struct sockaddr *)&cliaddr, &clilen);
            if(accp_sock == -1 && errno!=EWOULDBLOCK){
                errquit("ERROR :: accept fail");
            }else if(accp_sock > 0){
                if(is_nonblock(accp_sock)!=0 && set_nonblock(accp_sock)<0){
                    errquit("ERROR :: set_nonblock fail");
                }
                addClient(accp_sock, &cliaddr, 1);
                printf("GAME SERVER : %d 번째 사용자 추가\n", num_chatB);
            }
            for(i=0; i<num_chatB; i++){
                errno = 0;
                nbyte = recv(clisock_listB[i], buf, MAX_LINE, 0);
                if(nbyte==-1 && errno==EWOULDBLOCK){
                    continue;
                }else if(nbyte==-1 || nbyte==0){
                    removeClient(i, 1);
                    continue;
                }
                buf[nbyte]=0;
                printf("%d PORT : %s nbyte = %d\n", PORTNOB, buf, nbyte);

                if(buf[0]=='a'){
                    ptr = strtok(buf, "@");
                    ptr = strtok(NULL, "@");
                    strcpy(user[i].name, ptr);
                    printf("User NAME : %s\n", user[i].name);
                    user[i].money = 500000;
                    start_time = time(NULL);
                }
            }
            diff_time = time(NULL) - start_time;
            if(diff_time > 5)
                break;
        }
        if(num_chatB >= 2){
            struct SuddaUser swap_user;
            int swap_int, chair_no;
            chair_no = rand() % num_chatB;
            for(i=0; i<num_chatB; i++){
                swap_int = clisock_listB[i];
                memcpy(&swap_user,&user[i],sizeof(struct SuddaUser));
                clisock_listB[i] = clisock_listB[(chair_no+i)%num_chatB];
                memcpy(&user[i],&user[(chair_no+i)%num_chatB],sizeof(struct SuddaUser));
                clisock_listB[(chair_no+i)%num_chatB] = swap_int;
                memcpy(&user[(chair_no+i)%num_chatB],&swap_user,sizeof(struct SuddaUser));
                send_turninfo(clisock_listB[i],9,0);
            }
            amount_money = 0;
            winner = gamemaster(user, 1000, &amount_money,0);
            for(i=0; i<num_chatB; i++){
                if(i == winner){
                    user[winner].money += amount_money;
                }
                send_callinfo(0, 1000, i);
            }
            send_userinfo(user,-1);
            printf("GAME SERVER : GAME MASTER IS END\n");
        }
    }
    // 카드 조합을 볼 수 있게하는 부분
    /*
    for(j=0;j<20;j++){
        if(j==0){
            printf("     ");
            for(n=0;n<20;n++){
                printf(" %02d ",card_deck[n]);
            }
            printf("\n");
        }
        printf("%02d : ",card_deck[j]);
        for(n=0;n<20;n++){
            if(j==n)
                printf(" -- ");
            else
                printf(" %02d ",decide_combination(card_deck[j],card_deck[n]));
        }
        printf("\n");
    }
    */
}
void errquit(char *msg){
    perror(msg);
    exit(1);
}
void addClient(int s, struct sockaddr_in *newcliaddr, int type){
    char buf[20];
    inet_ntop(AF_INET, &newcliaddr->sin_addr, buf, sizeof(buf));
    if(type == 0){
        clisock_listA[num_chatA] = s;
        num_chatA++;
    }else{
        clisock_listB[num_chatB] = s;
        num_chatB++;
    }
    printf("New Client come : %s\n",buf);
}
void removeClient(int s,int type){
    if(type == 0){
        printf("listA CLIENT REMOVE\n");
        close(clisock_listA[s]);
        if(s != num_chatA-1)
            clisock_listA[s] = clisock_listA[num_chatA-1];
        num_chatA--;
    }else{
        printf("listB CLIENT REMOVE\n");
        close(clisock_listB[s]);
        if(s != num_chatB-1)
            clisock_listB[s] = clisock_listB[num_chatB-1];
        num_chatB--;
    }
}
int is_nonblock(int sockfd){
    int val;
    val = fcntl(sockfd, F_GETFL, 0);
    if(val & O_NONBLOCK)
        return 0;
    else
        return -1;
}
int set_nonblock(int sockfd){
    int val;
    val = fcntl(sockfd, F_GETFL, 0);
    if(fcntl(sockfd, F_SETFL, val | O_NONBLOCK) == -1)
        return -1;
    else
        return 0;
}
int tcp_listen(int host, int port, int backlog){
    int sd;
    struct sockaddr_in servaddr;

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd == -1)
        errquit("socket fail");
    bzero((char *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(host);
    servaddr.sin_port = htons(port);
    if(bind(sd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        errquit("bind fail");
    listen(sd, backlog);
    return sd;
}
// 카드패의 조합을 판단
int decide_combination(int card_first, int card_second){
    // 각 카드의 십의자리 숫자- 1
    // 예시) 01 은 1광, 71은 8광
    int tena = card_first/10, tenb = card_second/10;

    // 04~13 :: 장땡~1땡
    if(tena==tenb){
        return 13-tena;
    }
    // 01,03 :: 38광땡, 13광땡
    if(card_first==21 || card_second==21){
        if(card_first==71 || card_second==71)
            return 1;
        else if(card_first==1 || card_second==1)
            return 3;
    }
    // 02 :: 18광땡
    if(card_first==1 || card_second==1){
        if(card_first==71 || card_second==71)
            return 2;
    }
    // 14~17 :: 알리,독사,구삥,장삥
    if(tena==0 || tenb==0){
        if(tena==1 || tenb==1)
            return 14;
        else if(tena==3 || tenb==3)
            return 15;
        else if(tena==8 || tenb==8)
            return 16;
        else if(tena==9 || tenb==9)
            return 17;
    }
    // 18~19 :: 장사, 세륙
    if(tena==3 || tenb==3){
        if(tena==9 || tenb==9)
            return 18;
        else if(tena==5 || tenb==5)
            return 19;
    }
    // 30,31 :: 47암행어사, 37땡잡이
    if(card_first==61 || card_second==61){
        if(card_first==31 || card_second==31)
            return 30;
        else if(card_first==21 || card_second==21)
            return 31;
    }
    // 32 :: 멍텅구리 구사(사구)
    if((card_first==31 || card_second==31)&&(card_first==81 || card_second==81)){
        return 32;
    }
    // 33 :: 구사(사구)
    if((tena==3 || tenb==3)&&(tena==8 || tenb==8)){
        return 33;
    }
    // 20~29 :: 갑오~망통
        return (29-((tena+tenb+2)%10));
}
// 유저들의 카드패 조합들로 승패를 결정
int decide_winner(struct SuddaUser user[]){
    int user_total;
    int checkList[5], checkList2[5];
    int serveral_winner = 0;
    int i,j;

    // 38 광땡
    user_total = check_combine(user,checkList,1);
    if(user_total == 1)
        return checkList[0];
    // 암행어사 + 18광땡 13광땡
    user_total = check_combine(user,checkList,30);
    if(user_total == 1){
        if(check_combine(user,checkList2,2)==1)
            return checkList[0];
        else if(check_combine(user,checkList2,3)==1)
            return checkList[0];
    }else{
        if(check_combine(user,checkList2,2)==1)
            return checkList2[0];
        else if(check_combine(user,checkList2,3)==1)
            return checkList2[0];
    }
    user_total = check_combine(user,checkList,32);
    if(user_total == 1){
        return 12345;
    }
    // 땡잡이 + 장땡~1땡
    user_total = check_combine(user,checkList,31);
    if(user_total == 1){
        for(i=4; i<14; i++){
            if(check_combine(user,checkList2,i)==1)
                return checkList[0];
        }
    }else{
        for(i=4; i<14; i++){
            if(check_combine(user,checkList2,i)==1)
                return checkList2[0];
        }
    }
    user_total = check_combine(user,checkList,33);
    if(user_total == 1){
        return 12345;
    }
    // 알리~망통
    for(i=14; i<30; i++){
        user_total = check_combine(user,checkList,i);
        if(user_total == 1)
            return checkList[0];
        else if(user_total >= 2)
            for(j=0; j<user_total; j++)
                serveral_winner = serveral_winner*10 + (checkList[j]+1);
            return serveral_winner;
    }
    return 12345;
}
// 주어진 조합에 해당하는 카드패가 있는지 검사
int check_combine(struct SuddaUser user[], int checkList[], int check_n){
    int i, count_n = 0;

    for(i=0; i<5; i++){
        if(user[i].card_combine == check_n)
            checkList[count_n++] = i;
    }
    return count_n;
}
// 잔액과 지불해야 할 돈을 판단
int cal_pay(int wallet, int pay){
    if(wallet < pay)
        return wallet;
    else
        return pay;
}
// 게임 진행을 담당
int gamemaster(struct SuddaUser user[], int pan_money, int *amount_money, int type){
    int now_num, live_num, winner;
    int user_num;
    int user_pay[5];
    int pay = 0;
    int card_max = 20;
    int card_deck[20]={
        0,1,
        10,11,
        20,21,
        30,31,
        40,41,
        50,51,
        60,61,
        70,71,
        80,81,    
        90,91
    };
    char user_input;
    char buf[MAX_LINE+1];
    time_t start_time, diff_time;
    int i,j,l,nbyte;
    int turn_count = 0;

    printf("GAME MASTER IS START !!!!\n");
    user_num = num_chatB;
    live_num = user_num;

    for(i=0; i<user_num; i++){
        user[i].card_first = -1;
        user[i].card_second = -1;
        user[i].card_combine = -1;
        if(type==0){
            user[i].alive = 1;
        }
        send_turninfo(i,9,0);
    }
    // 카드 1차 배분
    for(i=0; i<user_num; i++){
        user[i].money -= 1000;
        *amount_money += 1000;
        now_num = rand() % card_max;
        user[i].card_first = card_deck[now_num];
        if(now_num != --card_max)
            card_deck[now_num] = card_deck[card_max];
    }
    for(i=0; i<user_num; i++){
        user_pay[i] = 0;
        send_callinfo(*amount_money, 1000, i);
    }
    send_userinfo(user,8);
    // 재경기가 아니면, 첫 카드 나눔부터 배팅
    for(i=0; type==0 && i<2; i++){
        if(live_num == 1)
            break;
        for(j=0; j<user_num; j++){
            if(user[j].alive==0 || user[j].money == 0 || user_pay[j] >= pan_money)
                continue;
            else if(live_num == 1)
                continue;
            send_turninfo(j,4,i);
            start_time = time(NULL);
            user_input = '0';
            diff_time=time(NULL)-start_time;
            while(diff_time < 7){
                diff_time=time(NULL)-start_time;
                errno = 0;
                nbyte = recv(clisock_listB[j], buf, MAX_LINE, 0);
                if(nbyte == -1 || errno == EWOULDBLOCK){
                    continue;
                }else if(nbyte == 0){
                    removeClient(j,1);
                    continue;
                }
                buf[nbyte]=0;
                printf("PORT %d Betting : %s\n",PORTNOB,buf);
                if(buf[0]=='b'){
                    user_input = buf[2];
                    break;
                }
                if(i==1){
                    if(user_input == '1' || user_input == '4')
                        break;
                }else if(user_input != '0')
                    break;
            }
            if(user_input == '0')
                user_input = '1';
            else if(user_input == '2')
                user_input = 'q';
            if(i==1 && user_input!='q' && user_input!='1')
                user_input = 'q';

            if(user_input == '2'){
                pay = cal_pay(user[j].money,1000);
            }else if(user_input == '3'){
                pay = cal_pay(user[j].money, pan_money*2);
            }else if(user_input == 'q'){
                pay = cal_pay(user[j].money, pan_money-user_pay[j]);
            }else if(user_input == 'w'){
                pay = cal_pay(user[j].money, (int)(*amount_money*0.25));
                if(user[j].money > 1000 && pay < 1000)
                    pay = 1000;
            }else if(user_input == 'e'){
                pay = cal_pay(user[j].money, (int)(*amount_money*0.5));
            }else{
                user[j].alive = 0;
                live_num--;
                pay = 0;
            }
            *amount_money += pay;
            user[j].money -= pay;
            user_pay[j] += pay;
            if(pay > pan_money)
                pan_money = pay;

            send_userinfo(user,j);
            for(l=0; l<num_chatB; l++)
                send_callinfo(*amount_money, pan_money-user_pay[l],l);
            send_turninfo(j,5,i);
        }
    }
    if(live_num == 1){
        for(i=0; i<user_num; i++){
            if(user[i].alive==1){
                user[winner].money += *amount_money;
                return i;
            }
        }
    }
    // 카드 2차 배분
    for(j=0;j<user_num;j++){
        if(user[j].alive == 0)
            continue;
        now_num = rand() % card_max;
        user_pay[i] = 0;
        user[j].card_second = card_deck[now_num];
        user[j].card_combine = decide_combination(user[j].card_first, user[j].card_second);
        if(now_num != --card_max)
            card_deck[now_num] = card_deck[card_max];
    }
    for(i=0; i<user_num; i++){
        user_pay[i] = 0;
        send_callinfo(*amount_money, pan_money-user_pay[i],i);
    }

    send_userinfo(user,8);
    for(i=0; i<2 && live_num!=1; i++){
        for(j=0; j<user_num; j++){
            if(user[j].alive==0 || user[j].money == 0 || user_pay[j] >= pan_money)
                continue;
            else if(live_num == 1)
                continue;
            send_turninfo(j,4,i);

            start_time = time(NULL);
            user_input = '0';
            diff_time=time(NULL)-start_time;
            while(diff_time < 7){
                diff_time=time(NULL)-start_time;
                errno = 0;
                nbyte = recv(clisock_listB[j], buf, MAX_LINE, 0);
                if(nbyte == -1 || errno == EWOULDBLOCK){
                    continue;
                }else if(nbyte == 0){
                    removeClient(j,1);
                    continue;
                }
                printf("PORT 16111 in Betting : %s\n",buf);
                if(buf[0]=='b'){
                    user_input = buf[2];
                    break;
                }
                if(i==1){
                    if(user_input == '1' || user_input == '4')
                        break;
                }else if(user_input != '0')
                    break;
            }
            if(user_input == '0')
                user_input = '1';
            else if(user_input == '2')
                user_input = 'q';
            if(i==1 && user_input!='q' && user_input!='1')
                user_input = 'q';

            if(user_input == '2'){
                pay = cal_pay(user[j].money,1000);
            }else if(user_input == '3'){
                pay = cal_pay(user[j].money, pan_money*2);
            }else if(user_input == 'q'){
                pay = cal_pay(user[j].money, pan_money-user_pay[j]);
            }else if(user_input == 'w'){
                pay = cal_pay(user[j].money, (int)(*amount_money*0.25));
            }else if(user_input == 'e'){
                pay = cal_pay(user[j].money, (int)(*amount_money*0.5));
            }else{
                user[j].alive = 0;
                live_num--;
                pay = 0;
            }
            *amount_money += pay;
            user[j].money -= pay;
            user_pay[j] += pay;
            if(pay > pan_money)
                pan_money = pay;

            send_userinfo(user,j);
            for(l=0; l<num_chatB; l++)
                send_callinfo(*amount_money, pan_money-user_pay[l],l);
            send_turninfo(j,5,i);
        }
    }

    // 카드패 조합 검사
    for(j=0; j<user_num; j++){
        if(user[j].alive == 0)
            user[j].card_combine = -1;
        send_userinfo(user,-1);
    }

    winner = decide_winner(user);
    if(winner > 10){
        for(i=0; i<user_num; i++)
            user[i].alive += 2;
        while(winner != 0){
            user[(winner%10)-1].alive = 1;
            winner = winner / 10;
        }
        for(i=0; i<user_num; i++){
            if(user[i].alive > 1)
                user[i].alive = 0;
        }
        winner = gamemaster(user, pan_money, amount_money, 1);
    }
    return winner;
}
void send_userinfo(struct SuddaUser user[], int user_no){
    char buf[MAX_LINE+1];
    int i, j;

    if(user_no<5 && user_no!=-1){
        for(j=0; j<num_chatB; j++){
            if(j==user_no){
                sprintf(buf,"1@%s@%d@%d@%d@%d@%d",user[user_no].name,user[user_no].money,user[user_no].card_first,user[user_no].card_second,user[user_no].card_combine,user[user_no].alive);
            }else{
                sprintf(buf,"1@%s@%d@%d@%d@%d@%d",user[user_no].name,user[user_no].money,-1,-1,-1,user[user_no].alive);
            }
            send(clisock_listB[j], buf, strlen(buf), 0);
            fflush(stdin);
            printf("to %d ~ send_userinfo : %s\n",j,buf);
            usleep(300000);
        }
    }else{
        for(i=0; i<num_chatB; i++){
            for(j=0; j<num_chatB; j++){
                if(i==j || user_no==-1){
                    sprintf(buf,"1@%s@%d@%d@%d@%d@%d",user[i].name,user[i].money,user[i].card_first,user[i].card_second,user[i].card_combine,user[i].alive);
                }else{
                    sprintf(buf,"1@%s@%d@%d@%d@%d@%d",user[i].name,user[i].money,-1,-1,-1,user[i].alive);
                }
                send(clisock_listB[j], buf, strlen(buf), 0);
                fflush(stdin);
                printf("to %d ~ send_userinfo : %s\n",j,buf);
                usleep(300000);
            }
        }
    }
    for(i=0; i<num_chatB; i++){
        sprintf(buf,"2@%d\n",0);
        send(clisock_listB[i], buf, strlen(buf), 0);
        fflush(stdin);
        printf("to %d ~ send_userinfo 2 : %s\n",i,buf);
    }
    usleep(300000);
}
void send_callinfo(int amount, int call, int user_no){
    char buf[MAX_LINE+1];
    sprintf(buf,"3@%d@%d\n",amount,call);
    send(clisock_listB[user_no], buf, strlen(buf), 0);
    fflush(stdin);
    printf("to %d ~ send_callinfo : %s\n",user_no,buf);
    usleep(300000);
}

void send_turninfo(int user_no, int turn, int turn_no){
    char buf[MAX_LINE+1];
    sprintf(buf,"%d@%d\n",turn,turn_no);
    send(clisock_listB[user_no], buf, strlen(buf), 0);
    fflush(stdin);
    printf("to %d ~ send_turninfo : %s\n",user_no,buf);
    usleep(300000);
}
