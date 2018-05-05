#define ACCEPT 0
#define REJECT 1
#define JUNK 2

struct Policy {
        char *title;
        char *word;
        bool case_sensitive;
        int action;
        char *message;
};
static struct Policy ** policies;
