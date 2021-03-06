#include <string>
#include <vector>
#include <poll.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

#include "acker.hh"
#include "saturateservo.hh"

using namespace std;

int main( int argc, char *argv[] )
{
  if ( argc != 1 && argc != 6 ) {
    fprintf( stderr, "Usage: %s [RELIABLE_IP RELIABLE_DEV TEST_IP TEST_DEV SERVER_IP]\n",
	     argv[ 0 ]);
    exit( 1 );
  }

  Socket server_socket;
  bool server;

  int sender_id = getpid();

  Socket::Address remote_data_address( UNKNOWN ), remote_feedback_address( UNKNOWN );

  uint64_t ts=Socket::timestamp();
  server = true;
  server_socket.bind( Socket::Address( "0.0.0.0", 9001 ) );
    
  FILE* log_file;
  char log_file_name[50];
  sprintf(log_file_name,"%s-%d-%d",server ? "server" : "client",(int)(ts/1e9),sender_id);
  log_file=fopen(log_file_name,"w");

  SaturateServo saturatr( "OUTGOING", log_file, server_socket, server_socket, remote_data_address, server, sender_id );

  while ( 1 ) {
    fflush( NULL );

    /* possibly send packet */
    saturatr.tick();
    
    /* wait for incoming packet OR expiry of timer */
    struct pollfd poll_fds[ 1 ];
    poll_fds[ 0 ].fd = server_socket.get_sock();
    poll_fds[ 0 ].events = POLLIN;

    struct timespec timeout;
    uint64_t next_transmission_delay =  saturatr.wait_time();


    timeout.tv_sec = next_transmission_delay / 1000000000;
    timeout.tv_nsec = next_transmission_delay % 1000000000;
    ppoll( poll_fds, 1, &timeout, NULL );


    if ( poll_fds[ 0 ].revents & POLLIN ) {
      saturatr.recv();
    }
  }
}
