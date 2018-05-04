#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <sysexits.h>
#include <string.h>
#include "types.h"

#include "Debugger.h"
#include "strings.h"
#include "usage.h"
#include "json_tool.h"
// Default initiators for policies
#include "policies.h"
#include "api.h"

struct smfiDesc milter = {
	MILTER_NAME, // Milter Name
	SMFI_VERSION, // Don't change
	SMFIF_ADDHDRS | SMFIF_ADDRCPT, // Extra Abilities
	mlfi_connect, // Connection callback
	mlfi_ehlo, // HELO callback
	mlfi_envfrom, // Envelope Sender callback
	mlfi_envrcpt, // Envelope Recipient callback
	mlfi_header, // Header callback
	mlfi_eoh, // End Of Header callback
	mlfi_body, // Body callback
	mlfi_eom, // End Of Message callback
	mlfi_abort, // Abort callback
	mlfi_close, // Close callback
	mlfi_unknown, // Uknown commands callback
	mlfi_data, // DATA callback
	mlfi_negotiate, // callback
};


int main( int argc, char ** argv ) {
	const char * args = "c:p:t:r:a:h";
	bool set_connection = FALSE;
	int c;
	extern char *optarg;

	printf("%s", GREETING);

	// Get Command Line Options
	while( (c = getopt(argc, argv, args)) != -1 ) {
		switch(c) {
			case 'c':
				printf("Attempting to load configuration file: %s\n", optarg);
				if( optarg == NULL || *optarg == '\0' ) {
					(void) fprintf( stderr, "%s\n", MISSING_CONFIG );
					exit( EX_USAGE );
				}

				validate_config( optarg );

				break;
			case 'p':
				if( optarg == NULL || *optarg == '\0' ) {
					(void) fprintf( stderr, "%s%s\n", ILLEGAL_CONNECTION, optarg );
					exit( EX_USAGE );
				}

				if( smfi_setconn(optarg) == MI_FAILURE ) {
					(void) fprintf( stderr, "%s\n", SMFI_SETCONN_FAIL );
					exit( EX_SOFTWARE );
				}

				// If we use a local socket, make sure it doesn't already exist
				// And be careful if you run this as root

				if( strncasecmp( optarg, "unix:", 5 ) == 0 ) {
					unlink( optarg + 5 );
				} else if ( strncasecmp( optarg, "local:", 6 ) == 0 ) {
					unlink( optarg + 6 );
				}

				set_connection = TRUE;
				break;
			case 't':
				if( optarg == NULL || *optarg == '\0' ) {
					(void) fprintf( stderr, "%s %s\n", ILLEGAL_TIMEOUT, optarg );
					exit( EX_USAGE );
				}
				if( smfi_settimeout( atoi(optarg) ) == MI_FAILURE ) {
					(void) fprintf( stderr, "%s\n", SMFI_SETTIMEOUT_FAIL );
					exit( EX_USAGE );
				}
				break;
			//case 'r':
				//if( optarg == NULL ) {
					//(void) fprintf( stderr, "%s\n", ILLEGAL_REJ );
					//exit( EX_USAGE );
				//}
				//break;
			//case 'a':
				//if( optarg == NULL ) {
					//(void) fprintf( stderr, "%s\n", ILLEGAL_ADD );
					//exit( EX_USAGE );
				//}
				//add = optarg
				//milter.xxfi_flags |= SMFIF_ADDRCPT
				//break;
			case 'h':
			default:
				usage( argv[0] );
				exit( EX_USAGE );
		}
	}

	if( ! set_connection ) {
		fprintf( stderr, "%s\n", MISSING_OPTION);
		usage( argv[0] );
		exit( EX_USAGE );
	}
	if( smfi_register(milter) == MI_FAILURE ) {
		fprintf( stderr, "%s\n", SMFI_REGISTER_FAIL );
		exit( EX_UNAVAILABLE );
	}

	return smfi_main();
}
