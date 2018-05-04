#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <sysexits.h>
#include <string.h>

#include "libmilter/mfapi.h"

struct mlfiPrivate {
	char * mlfi_fname;
	char * mlfi_connfrom;
	char * mlfi_ehlofrom;
	FILE * mlfi_file;
};

#define MLFIPRIVATE	((struct mlfiPrivate *) smfi_getpriv(ctx))

extern sfsistat		mlfi_cleanup(SMFICTX *, bool);

sfsistat mlfi_connect( SMFICTX * ctx, char * host, _SOCK_ADDR * addr) {
	struct mlfiPrivate * private;
	char * identity;

	logger( stdout, "Connection: %s\n", host );

	// Acquire memory
	private = malloc(sizeof * private);
	if( private == NULL ) {
		// Cannot allocate memory!
		// Set return message
		// Log this
		return SMFIS_TEMPFAIL;
	}

	memset( private, '\0', sizeof * private );

	// Save session data
	smfi_setpriv(ctx, private);

	identity = smfi_getsymval(ctx, "_");
	if( identity == NULL ) {
		identity = "?ERR?";
	}
	if ( (private->mlfi_connfrom = strdup(identity)) == NULL ) {
		(void) mlfi_cleanup(ctx, FALSE);
		return SMFIS_TEMPFAIL;
	}

	// Go on to next part of message
	return SMFIS_CONTINUE;
}

sfsistat mlfi_ehlo( SMFICTX * ctx, char * helohost ) {
	size_t len;
	char * tls;
	char * buf;
	struct mlfiPrivate * private = MLFIPRIVATE;

	tls = smfi_getsymval(ctx, "{tls_version}");
	if( tls == NULL ) {
		tls = "No TLS";
	}
	if( helohost == NULL ) {
		helohost = "?err?";
	}
	len = strlen(tls) + strlen(helohost) + 3;
	if( (buf = (char*) malloc(len)) == NULL ) {
		(void) mlfi_cleanup(ctx, FALSE);
		return SMFIS_TEMPFAIL;
	}
	snprintf(buf, len, "%s, %s", helohost, tls);
	if( private->mlfi_ehlofrom != NULL ) {
		free( private->mlfi_ehlofrom );
	}
	private->mlfi_ehlofrom = buf;

	// Test static data

	return SMFIS_CONTINUE;
}

sfsistat mlfi_envfrom( SMFICTX * ctx, char ** argv ) {
	int fd = -1;
	int argc = 0;
	struct mlfiPrivate * private = MLFIPRIVATE;
	char * mail_address = smfi_getsymval( ctx, "{mail_addr}" );

	if( (private->mlfi_fname = strdup("/tmp/msg.XXXXXX") ) == NULL ) {
		(void) mlfi_cleanup(ctx,FALSE);
		return SMFIS_TEMPFAIL;
	}

	if( (fd = mkstemp(private->mlfi_fname)) == -1 ) {
		(void) mlfi_cleanup(ctx,FALSE);
		return SMFIS_TEMPFAIL;
	}

	if( (private->mlfi_file = fdopen(fd, "w+") ) == NULL ) {
		(void) close(fd);
		(void) mlfi_cleanup(ctx,FALSE);
		return SMFIS_TEMPFAIL;
	}

	// Count recipients
	while( *argv++ != NULL ) {
		++argc;
	}

	// Log
	if( fprintf( private->mlfi_file, "Connect from %s (%s)\n\n", private->mlfi_ehlofrom, private->mlfi_connfrom ) == EOF ) {
		(void) mlfi_cleanup(ctx,FALSE);
		return SMFIS_TEMPFAIL;
	}

	// Log Sender
	if( fprintf( private->mlfi_file, "FROM %s (%d argument%s)\n", mail_address ? mail_address : "??", argc, (argc == 1) ? "" : "s") == EOF) {
		(void) mlfi_cleanup(ctx,FALSE);
		return SMFIS_TEMPFAIL;
	}

	return SMFIS_CONTINUE;
}

sfsistat mlfi_envrcpt( SMFICTX * ctx , char ** argv ) {
	struct mlfiPrivate * private = MLFIPRIVATE;
	char * recipient_address = smfi_getsymval(ctx, "{rcpt_addr}" );
	int argc = 0;

	// Count recipients
	while( *argv++ != NULL ) {
		++argc;
	}

	if( fprintf( private->mlfi_file, "Recipient: %s (%d argument%s)\n", recipient_address ? recipient_address : "??", argc, (argc == 1) ? "" : "s") == EOF ) {
		(void) mlfi_cleanup(ctx,FALSE);
		return SMFIS_TEMPFAIL;
	}

	return SMFIS_CONTINUE;
}

// may need to check on param 3 not being unsigned
sfsistat mlfi_header( SMFICTX * ctx, char * headerf, char * headerv ) {
	// Write header line to log file
	if( fprintf( MLFIPRIVATE->mlfi_file, "%s: %s\n", headerf, headerv ) == EOF ) {
		(void) mlfi_cleanup(ctx,FALSE);
		return SMFIS_TEMPFAIL;
	}

	return SMFIS_CONTINUE;
}

sfsistat mlfi_eoh( SMFICTX * ctx ) {
	// Put space in log after header
	if( fprintf(MLFIPRIVATE->mlfi_file, "\n") == EOF ) {
		(void) mlfi_cleanup(ctx,FALSE);
		return SMFIS_TEMPFAIL;
	}

	return SMFIS_CONTINUE;
}

sfsistat mlfi_body( SMFICTX * ctx, unsigned char * bodyp, size_t bodylen ) {
	struct mlfiPrivate * private = MLFIPRIVATE;

	// Write body of message to log file
	if( fwrite(bodyp, bodylen, 1, private->mlfi_file) != 1 ) {
		// problem writing
		fprintf( stderr, "Couldn't write to file %s: %s\n", private->mlfi_fname, strerror(errno) );
		(void) mlfi_cleanup(ctx,FALSE);
		return SMFIS_TEMPFAIL;
	}

	return SMFIS_CONTINUE;
}

sfsistat mlfi_eom( SMFICTX * ctx ) {
	bool ok = TRUE;
	char * add = "SSSSSSSS";
	if( add != NULL ) {
		ok = (smfi_addrcpt(ctx,add) == MI_SUCCESS);
	}
	return mlfi_cleanup(ctx, ok);
}

sfsistat mlfi_abort( SMFICTX * ctx ) {
	return mlfi_cleanup( ctx, FALSE );
}

sfsistat mlfi_cleanup( SMFICTX * ctx, bool ok ) {
	sfsistat status = SMFIS_CONTINUE;
	struct mlfiPrivate * private = MLFIPRIVATE;
	char * p;
	char host[512];
	char hbuf[1024];

	if( private == NULL ) {
		return status;
	}

	// Close log file
	if( private->mlfi_file != NULL && fclose( private->mlfi_file ) == EOF ) {
		// Failed to close file
		fprintf( stderr, "Could not close archive file %s: %s\n", private->mlfi_fname, strerror(errno) );
		status = SMFIS_TEMPFAIL;
		(void) unlink( private->mlfi_fname );
	} else if ( ok ) {
		// Add something to header
		if( gethostname( host, sizeof host ) < 0 ) {
			snprintf( host, sizeof host, "localhost" );
		}

		p = strrchr( private-> mlfi_fname, '/' );
		if( p == NULL ) {
			p = private->mlfi_fname;
		} else {
			p++;
		}

		snprintf( hbuf, sizeof hbuf, "%s@%s", p, host );
		if( smfi_addheader( ctx, "Eagle-Milter", hbuf ) != MI_SUCCESS ) {
			// Failed to add header
			fprintf( stderr, "Problem adding header: %s\n", hbuf );
			ok = FALSE;
			status = SMFIS_TEMPFAIL;
			(void) unlink( private->mlfi_fname );
		}
	} else {
		// Message aborted, remove log file
		fprintf( stderr, "Message was aborted, removing file %s\n", private->mlfi_fname );
		status = SMFIS_TEMPFAIL;
		(void) unlink( private->mlfi_fname );
	}

	// Free memory
	if( private->mlfi_fname != NULL ) {
		free( private->mlfi_fname );
	}

	return status;
}

sfsistat mlfi_close( SMFICTX * ctx ) {
	struct mlfiPrivate * private = MLFIPRIVATE;

	if( private == NULL ) {
		return SMFIS_CONTINUE;
	}
	if( private->mlfi_connfrom != NULL ) {
		free( private->mlfi_connfrom );
	}
	if( private->mlfi_ehlofrom != NULL ) {
		free( private->mlfi_ehlofrom );
	}
	free( private );
	smfi_setpriv( ctx, NULL );
	return SMFIS_CONTINUE;
}

sfsistat mlfi_unknown( SMFICTX * ctx, const char * cmd ) {
	logger(stderr, "Uknown command: %s\n", cmd);
	return SMFIS_CONTINUE;
}

sfsistat mlfi_data( SMFICTX * ctx ) {
	return SMFIS_CONTINUE;
}

sfsistat mlfi_negotiate( SMFICTX * ctx, unsigned long f0, unsigned long f1, unsigned long f2,
		unsigned long f3, unsigned long *pf0, unsigned long *pf1, unsigned long *pf2,
		unsigned long *pf3) {
	return SMFIS_ALL_OPTS;
}
