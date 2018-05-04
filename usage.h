#include <stdio.h>

static void usage( char * program ) {
	fprintf( stderr, "Usage: %s -c config-file -p socket-address [-t timeout] [-r reject-address] [-a add-address]\n\n", program );
}
