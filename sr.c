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


struct ack{
	int seqnum;
	int flag;
	float ts;
	float rel_to;
};

#define RTT 100.0

int last;

struct pkt *appln5_buffer;
struct pkt *recv_buffer;
struct ack *ack_buffer;
struct pkt packet_A;
struct pkt packet_B;
struct node *queue;

int a_base;
int b_base;
int win_size;
int nextSeq;

int succ_seq;
int expected_seq_no;
int expected_seq_no_atA;




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
	
	printf("Checksum %d Seqnum %d, data %s \n",checksum(&packet_A),packet_A.seqnum,packet_A.payload);
	
	//Store the packet in a buffer
	appln5_buffer[last] = packet_A;
	last += 1;
	
	//sending packets of windowsize
	while((nextSeq != last) && (nextSeq < a_base + win_size)){
		tolayer3(0,appln5_buffer[nextSeq]);
		
		printf("Packet sent\n");
		ack_buffer[nextSeq].seqnum = appln5_buffer[nextSeq].seqnum;
		ack_buffer[nextSeq].flag = 0;
		ack_buffer[nextSeq].ts = get_sim_time();
		ack_buffer[nextSeq].rel_to = ack_buffer[a_base].ts - ack_buffer[nextSeq].ts;
		//enqueue(&queue,ack_buffer[nextSeq].seqnum);
		
		if(nextSeq == a_base){
			starttimer (0, RTT);
		}
		nextSeq += 1;
	}
	return;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
		printf("Running A_input(packet)\n");
		printf("Checksum %d Seqnum %d, data %s nextSeq %d\n",checksum(&packet),packet_A.seqnum,packet_A.payload,nextSeq);
		
		if(packet.checksum != checksum(&packet))
		{
			printf("Incorrect ACK\n");
			return;	
		}
		
		if(expected_seq_no_atA == packet.seqnum)
		{
			printf("Correct ACK received\n");
			if (packet.seqnum > a_base && packet.seqnum < a_base + win_size){
				printf ("Marking ACK.\n");
				ack_buffer[packet.seqnum].seqnum = packet.seqnum;
				ack_buffer[packet.seqnum].flag = 1;
			}
			else if(packet.seqnum == a_base){
				
				printf("All packets received successfully. Moving front window\n");
				a_base = a_base + 1;
				stoptimer(0);
	
				int i;
				printf("a_base %d and nextSeq %d\n",a_base,nextSeq);
			
				for(i = a_base; i < nextSeq; i++)
				{
					if(ack_buffer[i].flag == 1){
						a_base = a_base + 1;
					}
					
					else{
						float sim_time = get_sim_time();
						float rem_time = sim_time - ack_buffer[i].ts;
						float newRTT = RTT - rem_time;
						//stoptimer(0);
						//tolayer3(0,appln5_buffer[a_base]);
						starttimer(0,newRTT);
						break;
					}
					
				while((nextSeq != last) && (nextSeq < a_base + win_size)){
					tolayer3(0,appln5_buffer[nextSeq]);
					nextSeq += 1;
					starttimer(0,RTT);
					}
				}
			}
		}
		
	return;

}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	printf("Running A_timerinterrupt()\n");
	
	int i;
	int j;
	float min;
	int loc;
	
	printf("Resending packet which timedout with base %d\n",a_base);
	tolayer3(0,appln5_buffer[a_base]);
	
	printf("Packet sent\n");

	ack_buffer[a_base].seqnum = appln5_buffer[a_base].seqnum;
	ack_buffer[a_base].flag = 0;
	ack_buffer[a_base].ts = get_sim_time();
	
	for(i = a_base; i < a_base + win_size; i++){
		if(ack_buffer[i].flag == 0){
			float sim_time = get_sim_time();
			float rem_time = sim_time - ack_buffer[i].ts;
			float newRTT = RTT - rem_time;
			starttimer(0,newRTT);
			break;
		}
	}
	/*for(i = 0, j = a_base; i< win_size, j< nextSeq; i++, j++)
	{
		if(ack_buffer[j].rel_to > 0)
		{
			min = ack_buffer[j].rel_to;
			loc = j;
		}
	}
	printf("Starting timer for: seqnum %d\n",loc);
	float sim_time = get_sim_time();
	float remain_time = get_sim_time() - ack_buffer[loc].ts;
	float time_val = RTT - remain_time;
	printf("Sim time %d, remain time %d , time val %d\n",sim_time,remain_time,time_val);
	
	if(time_val> 0)
		starttimer(0,time_val);
	else
		starttimer(0,RTT);
	*/
	
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	printf("Running A_init()\n");
	last = 0;
	a_base = 0;
	nextSeq = 0;
	expected_seq_no_atA = 0;
	appln5_buffer = (struct pkt *)malloc(sizeof (struct pkt) * 1500);
	ack_buffer = (struct ack *)malloc(sizeof (struct ack) * 1500);
	
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{	
		printf("Running B_input(packet)\n");
		
		int check_sum;
        check_sum = checksum(&packet);
		
		if (check_sum != packet.checksum){
                printf ("Receiver B: Packet received is corrupted!\n");
                return;
        }
		
		//Store the packet in a buffer
		printf("Storing in buffer at B side\n");
		printf("Received packet details seqnum %d, checksum %d, data %s\n",packet.seqnum,check_sum,packet.payload);
		
		recv_buffer[packet.seqnum] = packet;
        
		
		printf("b_base %d\n",b_base);
		
		/*if (packet.seqnum > b_base){
			printf ("Buffering out of order packets at B side.\n");
			recv_buffer[packet.seqnum] = packet;
		}*/
		
		if(packet.seqnum == expected_seq_no){
			printf("Packets received in order. Sending to upper layer\n");
			tolayer5 (1, packet.payload);
			b_base++;
			expected_seq_no ++;
			//succ_seq = packet.seqnum;
			//recv_buffer[packet.seqnum] = packet;

			int i;
			
			for(i = b_base; i < b_base + win_size; i++)
			{			
				if(recv_buffer[i].seqnum == expected_seq_no){
					
					printf ("Receiver B: Sending in-order packet to the the application layer\n");
					tolayer5 (1, recv_buffer[i].payload);
					b_base++;
					expected_seq_no ++;
					//recv_buffer[i].seqnum = -1;
					//i++;
				}
				i++;
			}
			
			
		}
		else if(packet.seqnum < b_base){
			printf("Ignoring packets\n");
		}
		
		//Send ACK packets for received packets
		printf("Make and send ack packet\n");
        packet_B.seqnum = packet.seqnum;
        packet_B.acknum = packet.acknum;
        strncpy (packet_B.payload, packet.payload, 20);
        packet_B.checksum = checksum(&packet);
        
        tolayer3 (1, packet_B);
        printf ("Receiver B: ACK sent.\n");
	return;

		
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	b_base = 0;
	recv_buffer = (struct pkt *)malloc(sizeof (struct pkt) * 1500);
	expected_seq_no = 0;
}
