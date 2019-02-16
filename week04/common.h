#define MAX_STUDENT_NAME_LENGTH 256
#define MAX_STUDENT_GROUP_LENGTH 10
#define MAX_UNSIGNED_INT_LENGTH 10

typedef struct _test_struct{
    
    unsigned int a;
    unsigned int b;
} test_struct_t;


typedef struct result_struct_{

    unsigned int c;

} result_struct_t;

typedef struct _student_struct {

    char first_name[MAX_STUDENT_NAME_LENGTH];
    char second_name[MAX_STUDENT_NAME_LENGTH];
    unsigned int age;
    char group[MAX_STUDENT_GROUP_LENGTH];
} student_struct_t;

typedef struct student_result_struct_ {
    char information[MAX_STUDENT_NAME_LENGTH+MAX_STUDENT_NAME_LENGTH+MAX_STUDENT_GROUP_LENGTH+5];
} student_result_struct_t;
