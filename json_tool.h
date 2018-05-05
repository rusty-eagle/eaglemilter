/* vim: set et ts=4
 *
 * Copyright (C) 2015 Mirko Pasqualetti  All rights reserved.
 * https://github.com/udp/json-parser
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "json-parser/json.h"

static void load_config( json_value* value, int depth );

static void set_policy( char * title, char * word, bool case_sensitive, int action, char * message ) {
	struct Policy * new_policy;
	new_policy = malloc( sizeof * new_policy );
	if( new_policy == NULL ) {
		logger( stderr, "Problem getting new policy memory.\n", NULL );
	}
	memset( new_policy, '\0', sizeof * new_policy );

	new_policy->title = title;
	new_policy->word = word;
	new_policy->case_sensitive = case_sensitive;
	new_policy->action = action;
	new_policy->message = message;

	free(new_policy);
}

static void process_object(json_value* value, int depth) {
        int length, x;
        if (value == NULL) {
                return;
        }
        length = value->u.object.length;
        for (x = 0; x < length; x++) {
                printf("object[%d].name = %s\n", x, value->u.object.values[x].name);

		json_value * word = value->u.object.values[x].value->u.object.values[0].value;
		json_value * action = value->u.object.values[x].value->u.object.values[1].value;
		json_value * message = value->u.object.values[x].value->u.object.values[2].value;

		if(
				strncmp( value->u.object.values[x].name, "word", 4 ) == 0 ||
				strncmp( value->u.object.values[x].name, "action", 6 ) == 0 ||
				strncmp( value->u.object.values[x].name, "message", 7 ) == 0
		  )
			continue;

		int ACTION = 0;
		if( action != NULL ) {
			if( strncmp(action->u.string.ptr,"accept", 6) == 0 ) {
				ACTION = ACCEPT;
			} else if( strncmp(action->u.string.ptr,"reject", 6) == 0 ) {
				ACTION = REJECT;
			} else if( strncmp(action->u.string.ptr,"junk", 4) == 0 ) {
				ACTION = JUNK;
			}
		}

		set_policy( value->u.object.values[x].name, word->u.string.ptr, FALSE, ACTION, message->u.string.ptr );

                load_config(value->u.object.values[x].value, depth+1);
        }
}

static void process_array(json_value* value, int depth) {
        int length, x;
        if (value == NULL) {
                return;
        }
        length = value->u.array.length;
        printf("array\n");
        for (x = 0; x < length; x++) {
                load_config(value->u.array.values[x], depth);
        }
}

static void load_config( json_value* value, int depth ) {
	if( value == NULL ) {
		fprintf( stderr, "Problem loading Config values!\n" );
		exit( EX_SOFTWARE );
	}
	switch( value-> type ) {
		case json_null:
			printf("none\n");
			break;
                case json_none:
                        printf("none\n");
                        break;
                case json_object:
                        process_object(value, depth+1);
                        break;
                case json_array:
                        process_array(value, depth+1);
                        break;
                case json_integer:
                        printf("int: %10" PRId64 "\n", value->u.integer);
                        break;
                case json_double:
                        printf("double: %f\n", value->u.dbl);
                        break;
                case json_string:
			printf("string: %s\n", value->u.string.ptr);
                        break;
                case json_boolean:
                        printf("bool: %d\n", value->u.boolean);
                        break;
        }
}


static int validate_config (char * filename ) {
        FILE *fp;
        struct stat filestatus;
        int file_size;
        char* file_contents;
        json_char* json;
        json_value* value;

        if ( stat(filename, &filestatus) != 0) {
		logger(stderr, "File %s not found\n", filename);
                return 1;
        }
        file_size = filestatus.st_size;
        file_contents = (char*)malloc(filestatus.st_size);
        if ( file_contents == NULL) {
                logger(stderr, "Memory error: unable to allocate %d bytes\n", file_size);
                return 1;
        }

        fp = fopen(filename, "rt");
        if (fp == NULL) {
                logger(stderr, "Unable to open %s\n", filename);
                fclose(fp);
                free(file_contents);
                return 1;
        }
        if ( fread(file_contents, file_size, 1, fp) != 1 ) {
                logger(stderr, "Unable to read content of %s\n", filename);
                fclose(fp);
                free(file_contents);
                return 1;
        }
        fclose(fp);

        //printf("%s\n", file_contents);

        //printf("--------------------------------\n\n");

        json = (json_char*)file_contents;
        value = json_parse(json,file_size);

        if (value == NULL) {
                logger(stderr, "Unable to parse data\n", NULL);
                free(file_contents);
                exit(1);
        } else {
		logger(stdout, "Configuration file is parsable\n", NULL);
	}

	load_config(value,0);

        json_value_free(value);
        free(file_contents);
        return 0;
}
