#include "../include/simulator.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


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

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
#define RTT 20.0



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

struct pkt packet_A;
struct pkt packet_B;

struct pkt *appln5_buffer;
bool expectedseqnum_A;
bool waiting_for_ack;
int last;
int first;
bool transfer;

bool seq_B;
bool ack_B;


void A_output(message)
    struct msg message;
    {
    printf("Running A_output()");
	printf("Waiting flag value %d\n",waiting_for_ack);
	printf("Data to deliver %s\n",message.data);
	
	packet_A.seqnum = expectedseqnum_A;
    packet_A.acknum = packet_A.seqnum;
    strncpy(packet_A.payload, message.data, 20);
	printf("Checksum at A_output %d",checksum(&packet_A));
    packet_A.checksum = checksum(&packet_A);
    
	//gl_ack = packet_A.acknum;
	
	//Store the packet in a buffer
	appln5_buffer[last] = packet_A;
	printf("Packet Buffered\n");
	last += 1;
	
		if(waiting_for_ack == appln5_buffer[first].seqnum && !transfer){
			//Send from buffer
			printf("waiting_for_ack %d\n",waiting_for_ack);
			printf("appln5_buffer[first].seqnum %d\n",appln5_buffer[first].seqnum);
			printf("transfer %d\n",transfer);
			tolayer3(0, appln5_buffer[first]);
			starttimer(0,RTT);
			transfer = 1;
		}
	expectedseqnum_A = !expectedseqnum_A;
}

/* called from layer 3, when a packet arrives for layer 5 */
void A_input(packet)
    struct pkt packet;
    {
		printf("Running A_input(packet)");
		int check_sum;
		check_sum = checksum(&packet);
		if(packet.acknum == waiting_for_ack && packet.checksum == check_sum)
		{
			printf("Correct ACK\n");
			stoptimer(0);
			transfer = 0;
			printf("first %d\n",first);
			printf("last %d\n",last);
			first = first + 1;
			printf("first after update %d\n",first);
		
			waiting_for_ack = !waiting_for_ack;
			if(first != last && packet.seqnum == waiting_for_ack && !transfer)
			{
				tolayer3(0,appln5_buffer[first]);
				printf("Packet %d sent to B again",first);
				transfer = 1;
				starttimer(0,RTT);
			}
	
		}
		else{
			printf("Incorrect ACK");
		}
}


/* called when A's timer goes off */
void A_timerinterrupt()
{	
	printf("Running A_timerinterrupt()");
    printf ("Sender A: Timeout event occured! Resending packet..\n");
	if(appln5_buffer[first].seqnum == waiting_for_ack){
		tolayer3(0, appln5_buffer[first]);
		transfer = 1;
		starttimer(0,RTT);
	}
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	printf("Running A_init()");
    //gl_ack = 0;
    expectedseqnum_A = 0;
	waiting_for_ack = 0;
	last = 0;
	first = 0;
	transfer = 0;
	appln5_buffer = (struct pkt *)malloc(sizeof (struct pkt) * 1500);
    return;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 5 at B*/
void B_input(packet)
struct pkt packet;
{
		printf("Running B_input(packet)");
        int check_sum;
        check_sum = checksum(&packet);
		
		printf("Packet.seqnum at B_input %d",packet.seqnum);
		printf("Expected_seqnum at B_input %d",seq_B);
		printf("Expected_ack at B_input %d",ack_B);
		printf("Checksum at B_input %d",checksum(&packet));
		
        /*Check if the packet is the one receiver is waiting for*/
        if (packet.seqnum == seq_B && packet.checksum == check_sum){
			printf ("Receiver B: Packet received.\n");
			tolayer5 (1, packet.payload);
			
			/*Make and send ack packet*/
			printf("Data received at B %s\n",packet.payload);
			packet_B.seqnum = packet.seqnum;
			packet_B.acknum = ack_B;
			strncpy (packet_B.payload, packet.payload, 20);
			packet_B.checksum = checksum(&packet);
			
			tolayer3 (1, packet_B);
			printf ("Receiver B: ACK sent.\n");
			
			seq_B = !seq_B;
			ack_B = !ack_B;
        }
		else{
			/*Make and send ack packet*/
			printf("Sending older ACK packet\n");
			packet_B.seqnum = packet.seqnum;
			packet_B.acknum = !ack_B;
			strncpy (packet_B.payload, packet.payload, 20);
			packet_B.checksum = checksum(&packet);
			tolayer3 (1, packet_B);
		}
        
        return;
        
    
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	printf("Running B_init()");
	seq_B = 0;
	ack_B = 0;
}
