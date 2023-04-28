#define SIZEofBUFF 20
#define NumOfCourses 8

typedef struct{
	long  	custid; 
	char 	FirstName[SIZEofBUFF];
	char 	LastName[SIZEofBUFF];
	float   Marks[NumOfCourses];
	float   GPA;
} MyRecord;

