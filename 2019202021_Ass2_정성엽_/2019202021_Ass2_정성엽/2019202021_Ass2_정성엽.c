#pragma warning(disable:4996)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 1    /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
    char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];
};

int init();

int generate_next_arrival();

int printevlist();

void stoptimer(int, int);

void starttimer(int, float, int);

void tolayer3(int, struct pkt);

void tolayer5(int, char datasent[20]);

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

#define WINDOW_SIZE 8

typedef struct Ack_List { //for output's Ack queue list
    int ack; //not use
    int seq; //output's sequence == receiver's Ack send
    struct Ack_List *next; //next queue list
} Ack_List; //typedef as AckList
Ack_List *A_ackList = NULL; // A queue list
Ack_List *B_ackList = NULL; // B queue list

Ack_List *createNode(int ack, int seq) { //create node function
    struct Ack_List *newNode = (Ack_List *) malloc(sizeof(Ack_List)); //make new window
    newNode->ack = ack; //set ack value
    newNode->seq = seq; //set seq value
    newNode->next = NULL; //next is NULL
    return newNode; //return new Node
}

int checksum(struct pkt *packet) {
    int cs = 0;     //checksum
    cs += packet->seqnum; // add
    cs += packet->acknum; //add
    for (int i = 0; i < 20; i++) {
        cs += packet->payload[i];//add each message value
    }
    return cs;//return checksum
}

struct Sender_Receiver {
    //for sender
    int send_base; //send_base
    int n_seq;  //next sequence = 보낼 예정
    float estimated_rtt; //각 init에서 설정
    int n_buffer; //next_buffer
    struct pkt packet_buffer[WINDOW_SIZE]; //8개를 순차적으로 보낼 예정

    //for receiver
    int rcv_base; //receive base
    int expect_seq; //
    struct pkt packet_to_send; //maybe packet that with Ack
};
struct Sender_Receiver A, B; //make struct sender and receiver information

void send_packet(char AorB, int seqnum) { //after output send to layer 3 and start timer
    if (AorB == 'A') {
        struct pkt *packet = &A.packet_buffer[A.n_seq % WINDOW_SIZE]; //make packet_buffer array under window size
        printf("\t\tsend_packet_A: send packet (seq=%d): %.*s\n", packet->seqnum, sizeof(packet->payload) - 1,
               packet->payload);//끝에 NULL 문자가 없으니까 20개만 출력하도록 설정
        printf("\t\tsend_packet_A: send ACK (ack = %d)\n", packet->acknum);//print acknum
        tolayer3(0, *packet);//send packet to layer3
        starttimer(0, A.estimated_rtt, seqnum);//start timer about seqnum;
        printf("\t\tsend_packet_A: start timer\n");
        A.n_seq++;//n_seq+=1
    } else if (AorB == 'B') {
        struct pkt *packet = &B.packet_buffer[B.n_seq % WINDOW_SIZE];
        printf("\t\tsend_packet_B: send packet(seq=%d): %.*s\n", packet->seqnum, sizeof(packet->payload),
               packet->payload);//끝에 NULL 문자가 없으니까 20개만 출력하도록 설정
        printf("\t\tsend_packet_B: send ACK (ack = %d)\n", packet->acknum); //print acknum
        tolayer3(1, *packet);//send packet to layer3
        starttimer(1, B.estimated_rtt, seqnum);//start timer about seqnum
        printf("\t\tsend_packet_B: start timer\n");
        B.n_seq++;//n_seq+=1
    }
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
        struct msg message;
{
    struct Ack_List *q; //struct Ack_list pointer q
    q = A_ackList; //임시로 A_ackList를 q로 가져오기
    if (A.n_buffer - A.send_base >= WINDOW_SIZE) {
        printf("\t\tA_output: buffer full. drop the message: %.*s\n", sizeof(message.data) - 1,
               message.data); //만약 window_size 8을 넘는 message 전송이 있다면 drop
        return;
    }
    printf("\t\tA_output: buffered packet(seq=%d): %.*s\n", A.n_buffer, sizeof(message.data) - 1,
           message.data); //보낼 패킷의 seq와 message 출력
    struct pkt *packet = &A.packet_buffer[A.n_buffer % WINDOW_SIZE]; //0~7

    packet->seqnum = A.n_buffer; // 보낼 packet의 seqnum 은 n_buffer의 위치이다.
    packet->acknum = A.packet_to_send.acknum;// 보낼 acknum은 A.packet_to_send.acknum이다. 코드 수정 이후에 초기 값 이외에는 사용하지 않아 따로 없애고 요약할 수 있지만 혹시 모를 오류 방지를 위해 유지

    memmove(packet->payload, message.data, 20); //payload값 설정
    if (packet->acknum != 999) { //acknum이 999가 아닐 때
        if (q == NULL) {
            packet->acknum = 999; // q에 들어있는 대기 queue가 없다면 acknum=999
        } else {
            packet->acknum = q->seq; //있다면 acknum = q->seq
            A_ackList = q->next;//초기 q 삭제 위함
            free(q);//free q
        }
    }

    packet->checksum = checksum(packet); //packet의 checksum은 미리 설정한 함수 사용
    A.n_buffer++;// n_buffer +=1
    send_packet('A', packet->seqnum); //send_packet 함수 사용
}

void B_output(message)  /* need be completed only for extra credit */
        struct msg message;
{
    struct Ack_List *q;//struct Ack_list pointer q
    q = B_ackList;//임시로 B_ackList를 q로 가져오기


    if (B.n_buffer - B.send_base >= WINDOW_SIZE) {
        printf("\t\tB_output: buffer full. drop the message: %.*s\n", sizeof(message.data) - 1,
               message.data);//만약 window_size 8을 넘는 message 전송이 있다면 drop
        return;
    }
    printf("\t\tB_output: buffered packet(seq=%d): %.*s\n", B.n_buffer, sizeof(message.data) - 1,
           message.data);//보낼 패킷의 seq와 message 출력
    struct pkt *packet = &B.packet_buffer[B.n_buffer % WINDOW_SIZE];//0~7
    packet->acknum = B.packet_to_send.acknum;// 보낼 acknum은 A.packet_to_send.acknum이다. 코드 수정 이후에 초기 값 이외에는 사용하지 않아 따로 없애고 요약할 수 있지만 혹시 모를 오류 방지를 위해 유지
    packet->seqnum = B.n_buffer;// 보낼 packet의 seqnum 은 n_buffer의 위치이다.

    memmove(packet->payload, message.data, 20);//payload값 설정
    if (packet->acknum != 999) {//acknum이 999가 아닐 때
        if (q == NULL)
            packet->acknum = 999;// q에 들어있는 대기 queue가 없다면 acknum=999
        else {
            packet->acknum = q->seq;//있다면 acknum = q->seq
            B_ackList = q->next;//초기 q 삭제 위함
            free(q);//free q
        }
    }
    packet->checksum = checksum(packet);//packet의 checksum은 미리 설정한 함수 사용
    B.n_buffer++;// n_buffer +=1
    send_packet('B', packet->seqnum);//send_packet 함수 사용
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
        struct pkt packet;
{
    if (packet.checksum != checksum(&packet)) { //checksum이 다르다면 packet이 corrupted 된 것이다.
        printf("\t\tA_input: packet corrupted. Drop\n"); //corrupted message 출력
        return; //함수 종료
    }
    Ack_List *newNode = createNode(packet.acknum, packet.seqnum); //문제가 없다면 이후 ack를 보낼 queue에 대해 새롭게 추가하는 함수를 가져온다.
    if (A_ackList == NULL) { //만약 없다면
        A_ackList = newNode; // head가 새로운 queue이다.
    } else {//있다면
        Ack_List *current = A_ackList; //head부터 시작하여
        while (current->next != NULL) { //다음이 NULL일 때까지 이동
            if (current->seq == packet.seqnum) { //단 이미 존재하는 queue인 경우 무시
                goto A_out;//추가 X
            }
            current = current->next;//존재하지 않는다면 다음 queue로 이동
        }
        current->next = newNode; //마지막에 새로운 함수 추가
    }
    A_out:
    printf("\t\tA_input: recv packet(seq=%d):%.*s\n", packet.seqnum, sizeof(packet.payload), packet.payload);//정상적으로 받았을 때 출력
    tolayer5(0, packet.payload); //layer5로 payload값을 보내준다.
    A.packet_to_send.acknum = packet.seqnum; //초기값이 아님을 설정한다
    //이 조건문은 queue를 생성한 이후 사용하지 않아도 될 조건문이다.
    if (A.expect_seq < packet.seqnum) {
        A.rcv_base = packet.seqnum;
        A.expect_seq = packet.seqnum;
    } else if (A.expect_seq > packet.seqnum) {
        if (A.rcv_base > packet.seqnum)
            A.rcv_base = packet.seqnum;
        A.expect_seq--;
    }
    A.expect_seq++;

    printf("\t\tA_input:got ACK (ack=%d)\n", packet.acknum);//받은 ACK를 출력한다.
    if (packet.acknum != 999) { //만약 999가 아니라면 해당 acknum에 대해 timer를 stop하고 sendbase를 추가한다.
        stoptimer(0, packet.acknum);
        printf("\t\tA_input: stop timer(ack=%d)\n", packet.acknum);
        A.send_base++; //위치가 중요하다기 보다 n_buffer와의 차이 계산을 위해 필요하다.
    }
}

/* called when A's timer goes off */
/* This is called when the timer goes off
 * I loop from the base till the nextseq number and decrement 1 each time . If the value is 0
 * I will retransmit the packet.
 * If the value is 0 previously I just skip beacuse the ack for the pkt is received
 */
void A_timerinterrupt(int seqnum) {
    struct pkt *packet = &A.packet_buffer[seqnum % WINDOW_SIZE];//time out이 되었다면 해당 seqnum의 패킷 배열에 접근하여 정보를 가져온다.
    printf("\t\tA_timerinterrupt: resend packet (seq=%d): %.*s\n", packet->seqnum,
           sizeof(packet->payload) - 1,
           packet->payload);
    tolayer3(0, *packet);//tolayer3로 바로 보낸다.

    starttimer(0, A.estimated_rtt, seqnum);//타이머 시작
    printf("\t\tA_timerinterrupt: start timer.\n");
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
    A.send_base = 1;
    A.n_seq = 1;
    A.estimated_rtt = 30;
    A.n_buffer = 1;


    A.rcv_base = 1;
    A.expect_seq = 1;
    A.packet_to_send.seqnum = -1;
    A.packet_to_send.acknum = 999;
    memset(A.packet_to_send.payload, 0, 20);
    A.packet_to_send.checksum = checksum(&A.packet_to_send);
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */


/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
        struct pkt packet;
{
    if (packet.checksum != checksum(&packet)) {//checksum이 다르다면 packet이 corrupted 된 것이다.
        printf("\t\tB_input:packet corrupted. Drop\n");//corrupted message 출력
        return;//함수 종료
    }
    Ack_List *newNode = createNode(packet.acknum, packet.seqnum); //문제가 없다면 이후 ack를 보낼 queue에 대해 새롭게 추가하는 함수를 가져온다.
    if (B_ackList == NULL) {//만약 없다면
        B_ackList = newNode;//head는 새로운 node
    } else {//있다면
        Ack_List *current = B_ackList;//head부터
        while (current->next != NULL) {//다음이 null이 아닐 때까지
            if (current->seq == packet.seqnum) {//만약 중복된다면
                goto B_out;//탈출
            }
            current = current->next;//다음 current로 이동
        }
        current->next = newNode;//마지막에 새로운 queue 생성
    }
    B_out:
    printf("\t\tB_input:recv packet (seq=%d):%.*s\n", packet.seqnum, sizeof(packet.payload) - 1, packet.payload);//정상적으로 받았을 때 출력
    tolayer5(1, packet.payload);//layer5로 payload값을 보내준다.
    B.packet_to_send.acknum = packet.seqnum;//초기값이 아님을 설정한다
//이 조건문은 queue를 생성한 이후 사용하지 않아도 될 조건문이다.
    if (B.expect_seq < packet.seqnum) {
        B.rcv_base = packet.seqnum;
        B.expect_seq = packet.seqnum;
    } else if (B.expect_seq > packet.seqnum) {
        if (B.rcv_base > packet.seqnum)
            B.rcv_base = packet.seqnum;
        B.expect_seq--;
    }
    B.expect_seq++;
    printf("\t\tB_input: Got ACK(ack=%d)\n", packet.acknum);//받은 ACK를 출력한다.
    if (packet.acknum != 999) {//만약 999가 아니라면 해당 acknum에 대해 timer를 stop하고 sendbase를 추가한다.
        stoptimer(1, packet.acknum);
        printf("\t\tB_input: stop timer.(ack=%d)\n", packet.acknum);
        B.send_base++;//위치가 중요하다기 보다 n_buffer와의 차이 계산을 위해 필요하다.
    }
}

/* called when B's timer goes off */
void B_timerinterrupt(int seqnum) {
    struct pkt *packet = &B.packet_buffer[seqnum % WINDOW_SIZE]; //time out이 되었다면 해당 seqnum의 패킷 배열에 접근하여 정보를 가져온다.
    printf("\t\tB_timerinterrupt: resend packet (seq=%d): %.*s\n", packet->seqnum,
           sizeof(packet->payload) - 1,
           packet->payload);//메세지 출력
    tolayer3(1, *packet);//바로 tolayer3로 보낸다.

    starttimer(1, B.estimated_rtt, seqnum);//새롭게 timer를 시작한다.
    printf("\t\tB_timerinterrupt: start timer.\n");
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
    B.send_base = 1;
    B.n_seq = 1;
    B.estimated_rtt = 30;
    B.n_buffer = 1;


    B.rcv_base = 1;
    B.expect_seq = 1;
    B.packet_to_send.seqnum = -1;
    B.packet_to_send.acknum = 999;
    memset(B.packet_to_send.payload, 0, 20);
    B.packet_to_send.checksum = checksum(&B.packet_to_send);
}


/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
    float evtime;           /* event time */
    int evtype;             /* event type code */
    int eventity;           /* entity where event occurs */
    struct pkt packet;      /* packet (if any) assoc w/ this event */
    struct event *prev;
    struct event *next;
};
struct event *evlist = NULL;   /* the event list */

/* possible events: */
//evtype
#define  TIMER_INTERRUPT 0
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define  A               0
#define  B               1


void insertevent(struct event *);


int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int ntolayer3;           /* number sent into layer 3 */
int nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

int main() {
    struct event *eventptr;
    struct msg msg2give;
    struct pkt pkt2give;

    int i, j;
    char c;
    init(); //initialize each element
    A_init(); //initialize about A
    B_init(); //initialize about B

    while (1) {
        //printevlist();
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        if (TRACE >= 2) {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer5 ");
            else
                printf(", fromlayer3 ");
            printf(" entity: %d\n", eventptr->eventity);
        }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim == nsimmax)
            break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */
            j = nsim % 26;
            for (i = 0; i < 20; i++)
                msg2give.data[i] = 97 + j;
            if (TRACE > 2) {
                printf("          MAINLOOP: data given to student: ");
                for (i = 0; i < 20; i++)
                    printf("%c", msg2give.data[i]);
                printf("\n");
            }
            nsim++;
            if (eventptr->eventity == A)
                A_output(msg2give);
            else
                B_output(msg2give);
        } else if (eventptr->evtype == FROM_LAYER3) {
            pkt2give.seqnum = eventptr->packet.seqnum;
            pkt2give.acknum = eventptr->packet.acknum;
            pkt2give.checksum = eventptr->packet.checksum;
            for (i = 0; i < 20; i++)
                pkt2give.payload[i] = eventptr->packet.payload[i];
            if (eventptr->eventity == A)      /* deliver packet by calling */
                A_input(pkt2give);            /* appropriate entity */
            else
                B_input(pkt2give);
        } else if (eventptr->evtype == TIMER_INTERRUPT) {
            if (eventptr->eventity == A)
                A_timerinterrupt(eventptr->packet.seqnum);
            else
                B_timerinterrupt(eventptr->packet.seqnum);
        } else {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

    terminate:
    printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n", time, nsim);
}


int init()                         /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();


    printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
    printf("Enter the number of messages to simulate: ");
    scanf("%d", &nsimmax); //actually we used to input 20
    printf("Enter packet loss probability [enter 0.0 for no loss]:");
    scanf("%f", &lossprob); //loss probabilty
    printf("Enter packet corruption probability [0.0 for no corruption]:");
    scanf("%f", &corruptprob);
    printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
    scanf("%f", &lambda);
    printf("Enter TRACE:");
    scanf("%d", &TRACE);

    srand(9999);              /* init random number generator */
    sum = 0.0;                /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand();    /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    //what mean about average 'avg'
    /*
     * I have to know about what about jimsrand()
     * it just mean random number
     * jimsrand가 0과 1 랜덤일텐데 250번 이상 750번 이하로 1이 되어야 함.
     * it just machine by machine
     */
    if (avg < 0.25 || avg > 0.75) { //if there isn't function completion
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(0);
    }

    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;

    time = 0.0;                    /* initialize time to 0.0 */
    generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() {
    double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
    //double mmm = 32767;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
    float x;                   /* individual students may need to change mmm */
    x = rand() / mmm;            /* x should be uniform in [0,1] */
    return (x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

int generate_next_arrival() {
    double x, log(), ceil();
    struct event *evptr;
    //char* malloc();
    float ttime;
    int tempint;

    if (TRACE > 2)
        printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2;  /* x is uniform on [0,2*lambda] */
    /* having mean of lambda        */
    evptr = (struct event *) malloc(sizeof(struct event));
    evptr->evtime = time + x;
    evptr->evtype = FROM_LAYER5;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}


void insertevent(p)
        struct event *p;
{
    struct event *q, *qold;

    if (TRACE > 2) {
        printf("            INSERTEVENT: time is %lf\n", time);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist;     /* q points to header of list in which p struct inserted */
    if (q == NULL) {   /* list is empty */ //eventlist NULL, maybe initial of program
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    } else { //if not NULL, maybe not initial of program
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q; //maybe find last linked list
        if (q == NULL) {   /* end of list */ //if loop executed all of q
            qold->next = p; //new next
            p->prev = qold; //new prev
            p->next = NULL; //p->next NULL
        } else if (q == evlist) { /* front of list */ // maybe p->evtime<= q->evtime
            p->next = evlist; // evlist = first of event linked list
            p->prev = NULL; //since it's first
            p->next->prev = p;// old evlist's prev  = p
            evlist = p; // new first event linked list
        } else {     /* middle of list */
            p->next = q; //maybe p->evtime <= q->evtime => p is executed earlier than q
            p->prev = q->prev;
            q->prev->next = p; // make prev link
            q->prev = p;
        }
    }
}

//no use?
int printevlist() //is this necessary? Where to use?
{
    struct event *q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next) {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype, q->eventity);
    }
    printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(AorB, seqnum) //literally stop timer with AorB and seqnum;
        int AorB;  /* A or B is trying to stop timer */
        int seqnum; /* sequence number of the packet whose timer needs to be stopped */
/*
 * Why it needed?
 * When Ack receive
 * or Nak receive, this time ACK not receive
 * or Time out
 * if Nak or Time out restart
 */
{
    struct event *q;

    if (TRACE > 2)
        printf("          STOP TIMER: stopping timer at %f for sequence number %d\n", time, seqnum);

    /* Search for the TIMER_INTERRUPT event with the given sequence number and AorB */
    for (q = evlist; q != NULL; q = q->next) {
        if ((q->evtype == TIMER_INTERRUPT) && (q->eventity == AorB) && (q->packet.seqnum == seqnum)) {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;         /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist) {   /* front of list - there must be event after */
                q->next->prev = NULL;
                evlist = q->next;
            } else {      /* middle of list */
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(int AorB, float increment, int seqnum)
//int AorB;  /* A or B is trying to start timer */
//float increment;
//int seqnum; /* sequence number of the packet whose timer needs to be started */
{
    struct event *q;
    struct event *evptr; //tmp event pointer
    //char* malloc();

    if (TRACE > 2)
        printf("          START TIMER: starting timer at %f for sequence number %d\n", time, seqnum);

    //이미 시작한 경우
    /* Check if timer is already started for this sequence number */
    for (q = evlist; q != NULL; q = q->next) {
        if ((q->evtype == TIMER_INTERRUPT) && (q->eventity == AorB) && (q->packet.seqnum == seqnum)) {
            printf("Warning: attempt to start a timer that is already started\n");
            return;// 종료
        }
    }

    /* create future event for when timer goes off */
    evptr = (struct event *) malloc(sizeof(struct event));
    evptr->evtime = time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    evptr->packet.seqnum = seqnum;
    insertevent(evptr);
}


/************************** TOLAYER3 ***************/
void tolayer3(AorB, packet)
        int AorB;  /* A or B is trying to stop timer */
        struct pkt packet;
{
    struct pkt mypktptr; // packet struct
    struct event *evptr, *q; // tmp event pointer and q pointer
    //char* malloc();
    float lastime, x, jimsrand();
    int i;


    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob) {
        nlost++;
        if (TRACE > 0)
            printf("          TOLAYER3: packet%d being lost\n", packet.seqnum);
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr.seqnum = packet.seqnum;
    mypktptr.acknum = packet.acknum;
    mypktptr.checksum = packet.checksum;
    for (i = 0; i < 20; i++)
        mypktptr.payload[i] = packet.payload[i];
    if (TRACE > 2) {
        printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr.seqnum,
               mypktptr.acknum, mypktptr.checksum);
        for (i = 0; i < 20; i++)
            printf("%c", mypktptr.payload[i]);
        printf("\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event *) malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;   /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */ //to know which is operated
    /* finally, compute the arrival time of packet at the other end.
       medium can not reorder, so make sure packet arrives between 1 and 10
       time units after the latest arrival time of packets
       currently in the medium on their way to the destination */
    lastime = time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next) //아마 이미 실행중이라면?
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity)) //find evtype FROM_LAYER3 and same eventity
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();

    /* simulate corruption: */
    if (jimsrand() < corruptprob) {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr.payload[0] = 'Z';   /* corrupt payload */
        else if (x < .875)
            mypktptr.seqnum = 999999;
        else
            mypktptr.acknum = 999999;
        if (TRACE > 0)
            printf("          TOLAYER3: packet%d being corrupted\n", packet.seqnum);
    }
    evptr->packet = mypktptr;       /* save ptr to my copy of packet */
    if (TRACE > 2)
        printf("          TOLAYER3: scheduling arrival on other side\n");
    insertevent(evptr);
}

void tolayer5(AorB, datasent)
        int AorB;
        char datasent[20];
{
    int i;
    if (TRACE > 2) {
        printf("          TOLAYER5: data received: ");
        for (i = 0; i < 20; i++)
            printf("%c", datasent[i]);
        printf("\n");
    }

}

