#include "../include/simulator.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
#define RTT 100.0

struct pkt *appln5_buffer;
struct pkt packet_A;
struct pkt packet_B;
int ack;
int nextSeq;
int gl_expectedseqnum;
int base;
int front;
int last;
int win_size;

int checksum(struct pkt * packet)
{
    int checksum = 0;
    for (int i = 0; i < 20; ++i){
        checksum += packet->payload[i];
    }
    checksum += packet->seqnum;
    checksum += packet->acknum;
    return checksum;
}


/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
	printf("Running A_output(message)\n");
	
	//Create a packet and store in the buffer
	memset (&packet_A, 0, sizeof (struct pkt));
    packet_A.seqnum = nextSeq;
    packet_A.acknum = packet_A.seqnum;
    strncpy(packet_A.payload, message.data, 20);
    packet_A.checksum = checksum(&packet_A);
	
	printf("Checksum at A_output %d\n",checksum(&packet_A));
	printf("Seqnum at A_output %d\n",packet_A.seqnum);
	
	//Store the packet in a buffer
	appln5_buffer[last] = packet_A;
	last += 1;
	
	//sending packets of windowsize
	printf("nextSeq %d,last %d, win_size %d",nextSeq,last,win_size);
	while((nextSeq != last) && (nextSeq < base + win_size)){
		
		tolayer3(0,appln5_buffer[nextSeq]);
		printf("Packet sent\n");
		/*if(nextSeq == base){
			starttimer (0, RTT);
		}*/
		nextSeq += 1;
		starttimer (0, RTT);
	}
	return;
}

/* called from layer 3, when a packet arrives for layer 5 */
void A_input(packet)
  struct pkt packet;
{
	printf("Running A_input(packet)\n");
	if(packet.checksum != checksum(&packet)){
		printf("Incorrect ACK\n");
		return;	
	}
	printf("Correct ACK received\n");
	//stoptimer(0);
	
	/*while(base <= packet.acknum){
		base = base + 1;	//change the base now that correct ack is received
		printf("Updated base in A_input %d\n",base);
	}*/
	base = packet.acknum + 1;
		
	if(base == nextSeq){
		stoptimer(0);
		while((nextSeq != last) && (nextSeq < base + win_size)){
			tolayer3(0,appln5_buffer[nextSeq]);
			printf("Packet sent in A_input\n");
			nextSeq ++;
		}
	}
		
	else{
		
		starttimer(0, RTT);
		printf("Start Timer\n");
	}
	return;
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	printf("Running A_timerinterrupt()\n");
	printf("Value of nextSeq %d\n",nextSeq);
	printf("Value of base %d\n",base);
	
	int temp_front = base;
	int buffpkts = nextSeq - base;
	
	//while((nextSeq != last) && (nextSeq < base + win_size)){
	//for (int i=0; i < buffpkts;i++){
	for (int i=temp_front; i < buffpkts;i++){
		tolayer3(0,appln5_buffer[i]);
		printf("Packet sent in timerIntrrupt\n");
		i += 1;
		//nextSeq++;
	}
	starttimer(0,RTT);
	return;
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	printf("Running A_init()\n");
	//packet_A.seqnum = 0;
	appln5_buffer = (struct pkt *)malloc(sizeof (struct pkt) * 1500);
	nextSeq = 0;
	base = 0;
	front = 0;
	last = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 5 at B*/
void B_input(packet)
  struct pkt packet;
{
	printf("Running B_input(packet)\n");
        int check_sum;
        check_sum = checksum(&packet);
		
		printf("B_input packet.seqnum %d\n",packet.seqnum);
		printf("B_input gl_expectedseqnum %d\n",gl_expectedseqnum);
		
		printf("Checksum at B_input %d\n",checksum(&packet));
		
        /*Check if the packet is the one receiver is waiting for*/
        if (packet.seqnum != gl_expectedseqnum){
            printf ("Receiver B: Waiting for a different sequence number.\n");
            printf ("Receiver B: Resending previous correct ACK %d\n",gl_expectedseqnum);
			if(gl_expectedseqnum == 0){
				printf("No packet received. No ACK to send");
				return;
			}
			else{
				tolayer3 (1, packet_B);
				stoptimer(0);
			}
            return;
        }
		
		if (check_sum != packet.checksum){
            printf ("Receiver B: Incorrect cheksum.\n");
            printf ("Receiver B: Resending previous correct ACK\n");
            tolayer3 (1, packet_B);
			stoptimer(0);
            return;
        }
        
        printf ("Receiver B: Packet received.\n");
        tolayer5 (1, packet.payload);
     
        /*Make and send ack packet*/
		printf("Make and send ack packet\n");
        packet_B.seqnum = packet.seqnum;
		//packet_B.seqnum = gl_expectedseqnum;
        packet_B.acknum = packet.acknum;
        strncpy (packet_B.payload, packet.payload, 20);
        packet_B.checksum = checksum(&packet);
        
        tolayer3 (1, packet_B);
        printf ("Receiver B: ACK sent.\n");
		gl_expectedseqnum += 1;
        return;

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	printf("Running B_init()\n");
	gl_expectedseqnum = 0;
	return;
}
