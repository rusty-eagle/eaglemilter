static bool DKIM = FALSE;
static bool SPF = FALSE;
static bool DMARC = FALSE;

#define ACCEPT 0
#define REJECT 1
#define JUNK 2
struct CustomFilter {
        char title[50];
        char word[50];
        bool case_sensitive;
        int action;
        char message[50];
};
static struct CustomFilter filters[10];
