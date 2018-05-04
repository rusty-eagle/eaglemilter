// The program will be expecting a function called logger(), where a string can be given.
// This is here to implement this however we need at the time

#define logger(stream, string, extra) (void) fprintf(stream, string, extra)
