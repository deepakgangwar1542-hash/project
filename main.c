#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NAME_LEN 50

typedef struct {
    int rollNo;
    char name[NAME_LEN];
    int row;
    int col;
} Student;

typedef struct {
    int occupied;
    int rollNo;
} Seat;

Seat **hall = NULL;
int rows = 0, cols = 0;

Student **students = NULL;
int studentCount = 0;

const char *STUDENT_FILE = "students.dat";
const char *LOG_FILE = "allocation_log.txt";

void getTimestamp(char *buffer, int size) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

void logAction(const char *action, int rollNo, int row, int col) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) {
        printf("Error opening log file.\n");
        return;
    }
    char ts[32];
    getTimestamp(ts, sizeof(ts));
    fprintf(fp, "[%s] %s - Roll: %d, Seat: (%d,%d)\n",
            ts, action, rollNo, row, col);
    fclose(fp);
}

void createHall() {
    printf("Enter number of rows: ");
    scanf("%d", &rows);
    printf("Enter number of columns: ");
    scanf("%d", &cols);

    hall = (Seat **)malloc(rows * sizeof(Seat *));
    for (int i = 0; i < rows; i++) {
        hall[i] = (Seat *)malloc(cols * sizeof(Seat));
        for (int j = 0; j < cols; j++) {
            hall[i][j].occupied = 0;
            hall[i][j].rollNo = -1;
        }
    }
}

void loadStudentsFromFile() {
    FILE *fp = fopen(STUDENT_FILE, "rb");
    if (!fp) {
        return;
    }

    Student temp;
    while (fread(&temp, sizeof(Student), 1, fp) == 1) {
        students = (Student **)realloc(students, (studentCount + 1) * sizeof(Student *));
        students[studentCount] = (Student *)malloc(sizeof(Student));
        *(students[studentCount]) = temp;

        if (temp.row >= 0 && temp.row < rows && temp.col >= 0 && temp.col < cols) {
            hall[temp.row][temp.col].occupied = 1;
            hall[temp.row][temp.col].rollNo = temp.rollNo;
        } else {
            printf("Warning: Student %d (Seat %d,%d) fits outside current hall dimensions.\n", 
                   temp.rollNo, temp.row, temp.col);
        }

        studentCount++;
    }
    fclose(fp);
}

void saveStudentsToFile() {
    FILE *fp = fopen(STUDENT_FILE, "wb");
    if (!fp) {
        printf("Error opening binary file for writing.\n");
        return;
    }
    for (int i = 0; i < studentCount; i++) {
        fwrite(students[i], sizeof(Student), 1, fp);
    }
    fclose(fp);
}

int findNextFreeSeat(int *outRow, int *outCol) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (!hall[i][j].occupied) {
                *outRow = i;
                *outCol = j;
                return 1;
            }
        }
    }
    return 0;
}

int findStudentIndexByRoll(int rollNo) {
    for (int i = 0; i < studentCount; i++) {
        if (students[i]->rollNo == rollNo) {
            return i;
        }
    }
    return -1;
}

void allocateSeat() {
    if (rows == 0 || cols == 0) {
        printf("Hall not initialized.\n");
        return;
    }

    int roll;
    char name[NAME_LEN];

    printf("Enter Roll No: ");
    scanf("%d", &roll);
    
    if (findStudentIndexByRoll(roll) != -1) {
        printf("Roll number already exists.\n");
        return;
    }

    printf("Enter Name: ");
    getchar();
    fgets(name, NAME_LEN, stdin);
    name[strcspn(name, "\n")] = '\0';

    int r, c;
    if (!findNextFreeSeat(&r, &c)) {
        printf("No free seat available.\n");
        return;
    }

    students = (Student **)realloc(students, (studentCount + 1) * sizeof(Student *));
    students[studentCount] = (Student *)malloc(sizeof(Student));
    
    students[studentCount]->rollNo = roll;
    strcpy(students[studentCount]->name, name);
    students[studentCount]->row = r;
    students[studentCount]->col = c;

    hall[r][c].occupied = 1;
    hall[r][c].rollNo = roll;

    studentCount++;

    saveStudentsToFile();
    logAction("ALLOCATE", roll, r, c);

    printf("Seat allocated at Row %d, Col %d.\n", r, c);
}

void deallocateSeat() {
    if (studentCount == 0) {
        printf("No students to remove.\n");
        return;
    }

    int roll;
    printf("Enter Roll No to deallocate: ");
    scanf("%d", &roll);

    int idx = findStudentIndexByRoll(roll);
    if (idx == -1) {
        printf("Student not found.\n");
        return;
    }

    int r = students[idx]->row;
    int c = students[idx]->col;

    logAction("DEALLOCATE", roll, r, c);

    if (r >= 0 && r < rows && c >= 0 && c < cols) {
        hall[r][c].occupied = 0;
        hall[r][c].rollNo = -1;
    }

    free(students[idx]);

    for (int i = idx; i < studentCount - 1; i++) {
        students[i] = students[i + 1];
    }

    studentCount--;
    
    if (studentCount == 0) {
        free(students);
        students = NULL;
    } else {
        Student **temp = (Student **)realloc(students, studentCount * sizeof(Student *));
        if (temp != NULL) {
            students = temp;
        }
    }

    saveStudentsToFile();
    printf("Seat deallocated for Roll %d.\n", roll);
}

void displayHall() {
    if (rows == 0 || cols == 0) {
        printf("Hall not initialized.\n");
        return;
    }

    printf("\nHall Status:\n");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (hall[i][j].occupied) {
                printf("R%02dC%02d(X) ", i + 1, j + 1);
            } else {
                printf("R%02dC%02d(O) ", i + 1, j + 1);
            }
        }
        printf("\n");
    }
}

void searchStudent() {
    int roll;
    printf("Enter Roll No to search: ");
    scanf("%d", &roll);

    int idx = findStudentIndexByRoll(roll);
    if (idx == -1) {
        printf("Student not found.\n");
        return;
    }

    Student *s = students[idx];
    printf("Roll: %d\nName: %s\nSeat: Row %d, Col %d\n",
           s->rollNo, s->name, s->row, s->col);
}

void viewLog() {
    FILE *fp = fopen(LOG_FILE, "r");
    if (!fp) {
        printf("No log file found.\n");
        return;
    }

    char line[256];
    printf("\n--- Allocation Log ---\n");
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    fclose(fp);
}

void freeAll() {
    if (hall) {
        for (int i = 0; i < rows; i++) {
            free(hall[i]);
        }
        free(hall);
    }

    if (students) {
        for (int i = 0; i < studentCount; i++) {
            free(students[i]);
        }
        free(students);
    }
}

int main() {
    createHall();
    loadStudentsFromFile();

    int choice;
    do {
        printf("\n--- Dynamic Student Seating Allocator ---\n");
        printf("1. Allocate Seat\n");
        printf("2. Deallocate Seat\n");
        printf("3. Display Hall Status\n");
        printf("4. Search Student\n");
        printf("5. View Audit Log\n");
        printf("0. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: allocateSeat(); break;
            case 2: deallocateSeat(); break;
            case 3: displayHall(); break;
            case 4: searchStudent(); break;
            case 5: viewLog(); break;
            case 0: printf("Exiting...\n"); break;
            default: printf("Invalid choice.\n");
        }

    } while (choice != 0);

    freeAll();
    return 0;
}