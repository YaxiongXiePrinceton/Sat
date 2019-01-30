#include <assert.h>
#include <math.h>

#include "saturateservo.hh"
#include "socket.hh"
#include "payload.hh"
#include "acker.hh"

SaturateServo::SaturateServo( const char * s_name,
                              FILE* log_file,
			      const Socket & s_listen,
			      const Socket & s_send,
			      const Socket::Address & s_remote,
			      const bool s_server,
			      const int s_send_id )
  : _name( s_name ),
    _log_file(log_file),
    _listen( s_listen ),
    _send( s_send ),
    _remote( s_remote ),
    _server( s_server ),
    _send_id( s_send_id ),
    _acker( NULL ),
    _next_transmission_time( Socket::timestamp() ),
    _foreign_id( -1 ),
    _packets_sent( 0 ),
    _max_ack_id( -1 ),
    _window( LOWER_WINDOW )
{}

void SaturateServo::recv( void )
{
  /* get the ack packet */
  Socket::Packet incoming( _listen.recv() );
  SatPayload *contents = (SatPayload *) incoming.payload.data();
  contents->recv_timestamp = incoming.timestamp;

  if ( contents->sequence_number != -1 ) {
    /* not an ack */
    printf( "MARTIAN!\n" );
    return;
  }

  /* possibly roam */
  if ( _server ) {
      if ( (contents->sender_id > _foreign_id) && (contents->ack_number == -1) ) {
	_foreign_id = contents->sender_id;
	_remote = incoming.addr;
      }
  }

  /* process the ack */
  if ( contents->sender_id != _send_id ) {
    /* not from us */
    return;
  } else {
    if ( contents->ack_number > _max_ack_id ) {
      _max_ack_id = contents->ack_number;
    }


    int64_t rtt_ns = contents->recv_timestamp - contents->sent_timestamp;
    double rtt = rtt_ns / 1.e9;

// Yaxiong's code
    int64_t oneway_ns = contents->oneway_ns;
    double oneway_delay = oneway_ns / 1.e9;
    int thput = 40000000;
    //double bits_sent = thput * oneway_delay;
    double bits_sent = thput * LOWER_RTT;
    int window_size = (int) floor( bits_sent / (1500 * 8) );
     
   /* increase-decrease rules */

    if (_window < window_size){
	_window++;
    }
    if (_window > window_size){
	_window -= 20;
    }   
// End of Yaxiong's code
 
    //fprintf( _log_file, "%s ACK RECEIVED senderid=%d, seq=%d, send_time=%ld,  recv_time=%ld, rtt=%.4f, %d => ",
      // _name.c_str(),_server ? _foreign_id : contents->sender_id , contents->ack_number, contents->sent_timestamp, contents->recv_timestamp, (double)rtt,  _window );
 
 /*   if ( (rtt < LOWER_RTT) && (_window < UPPER_WINDOW) ) {
      _window++;
    }

    if ( (rtt > UPPER_RTT) && (_window > LOWER_WINDOW + 10) ) {
      _window -= 20;
    }
*/
    //printf( "ACK RECEIVED senderid=%d ack_num=%d pkt_sent=%d pkt_OTA=%d RTT:%f OneWay:%f WINDOW:%d targetW:%d\n",
//	  contents->sender_id, contents->ack_number,_packets_sent, _packets_sent - contents->ack_number, rtt, oneway_delay, _window, window_size); 
 //   fprintf( _log_file, "%d\n", _window );
  }
}

uint64_t SaturateServo::wait_time( void ) const
{
  int num_outstanding = _packets_sent - _max_ack_id - 1;

  if ( _remote == UNKNOWN ) {
    return 1000000000;
  }

  if ( num_outstanding < _window ) {
    return 0;
  } else {
    int diff = _next_transmission_time - Socket::timestamp();
    if ( diff < 0 ) {
      diff = 0;
    }
    return diff;
  }
}

void SaturateServo::tick( void )
{
  if ( _remote == UNKNOWN ) {
    return;
  }

  int num_outstanding = _packets_sent - _max_ack_id - 1;

  if ( num_outstanding < _window ) {
    /* send more packets */
    int amount_to_send = _window - num_outstanding;
    for ( int i = 0; i < amount_to_send; i++ ) {
      SatPayload outgoing;
      outgoing.sequence_number = _packets_sent;
      outgoing.ack_number = -1;
      outgoing.sent_timestamp = Socket::timestamp();
      outgoing.recv_timestamp = 0;
      outgoing.oneway_ns = 0;
      outgoing.sender_id = _send_id;

      _send.send( Socket::Packet( _remote, outgoing.str( 1400 ) ) );

      /*
      printf( "%s pid=%d DATA SENT %d senderid=%d seq=%d, send_time=%ld, recv_time=%ld\n",
      _name.c_str(), getpid(), amount_to_send, outgoing.sender_id, outgoing.sequence_number, outgoing.sent_timestamp, outgoing.recv_timestamp ); */

      _packets_sent++;
    }

    _next_transmission_time = Socket::timestamp() + _transmission_interval;
  }

  if ( _next_transmission_time < Socket::timestamp() ) {
    SatPayload outgoing;
    outgoing.sequence_number = _packets_sent;
    outgoing.ack_number = -1;
    outgoing.sent_timestamp = Socket::timestamp();
    outgoing.recv_timestamp = 0;
    outgoing.oneway_ns = 0;
    outgoing.sender_id = _send_id;

    _send.send( Socket::Packet( _remote, outgoing.str( 1400 ) ) );

    /*
    printf( "%s pid=%d DATA SENT senderid=%d seq=%d, send_time=%ld, recv_time=%ld\n",
    _name.c_str(), getpid(), outgoing.sender_id, outgoing.sequence_number, outgoing.sent_timestamp, outgoing.recv_timestamp ); */

    _packets_sent++;

    _next_transmission_time = Socket::timestamp() + _transmission_interval;
  }
}
