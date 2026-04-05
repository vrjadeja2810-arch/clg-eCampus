/*******************************************************************************************
*
* University Portal System - After Registration Details
*
* This program is launched after a user registers for the first time.
* It receives user details via command-line arguments.
*
* How to Compile (Windows with MinGW-w64):
* This file MUST be compiled WITHOUT -mwindows, as it needs a standard main()
* to receive command-line arguments.
* gcc afterreg.c -o afterreg.exe -lraylib -lgdi32 -lwinmm
*
********************************************************************************************/

// This define is the KEY to fixing the compile error.
// It tells raylib that we are using a standard main() function.
#define PLATFORM_DESKTOP

#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_INPUT_CHARS      50
#define MAX_ADDRESS_CHARS    100
#define NUM_STUDENT_TEXTBOXES 7
#define NUM_FACULTY_TEXTBOXES 5

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum { USER_TYPE_STUDENT, USER_TYPE_FACULTY, USER_TYPE_UNKNOWN } UserType;

typedef struct {
    Rectangle bounds;
    char *text;
    int textMaxSize;
    int charCount;
    bool isActive;
    const char* label;
} TextBox;

typedef struct {
    char id[MAX_INPUT_CHARS + 1];
    char password[MAX_INPUT_CHARS + 1];
    char name[MAX_INPUT_CHARS + 1];
    char address[MAX_ADDRESS_CHARS + 1];
    char phone[MAX_INPUT_CHARS + 1];
    char email[MAX_INPUT_CHARS + 15];
    char course[MAX_INPUT_CHARS + 1];
    char semester[MAX_INPUT_CHARS + 1];
    char height[MAX_INPUT_CHARS + 1];
    char bloodGroup[5];
} StudentInfo;

typedef struct {
    char id[MAX_INPUT_CHARS + 1];
    char password[MAX_INPUT_CHARS + 1];
    char name[MAX_INPUT_CHARS + 1];
    char address[MAX_ADDRESS_CHARS + 1];
    char phone[MAX_INPUT_CHARS + 1];
    char email[MAX_INPUT_CHARS + 15];
    char department[MAX_INPUT_CHARS + 1];
    char designation[MAX_INPUT_CHARS + 1];
} FacultyInfo;

//----------------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------------
const int screenWidth = 1920;
const int screenHeight = 1080;

UserType currentUserType = USER_TYPE_UNKNOWN;
StudentInfo currentStudent;
FacultyInfo currentFaculty;

TextBox studentTextBoxes[NUM_STUDENT_TEXTBOXES];
TextBox facultyTextBoxes[NUM_FACULTY_TEXTBOXES];

// Buffers for student text boxes
char s_infoNameText[MAX_INPUT_CHARS + 1] = {0};
char s_infoAddressText[MAX_ADDRESS_CHARS + 1] = {0};
char s_infoPhoneText[MAX_INPUT_CHARS + 1] = {0};
char s_infoCourseText[MAX_INPUT_CHARS + 1] = {0};
char s_infoSemesterText[MAX_INPUT_CHARS + 1] = {0};
char s_infoHeightText[MAX_INPUT_CHARS + 1] = {0};
char s_infoBloodText[5] = {0};

// Buffers for faculty text boxes
char f_infoNameText[MAX_INPUT_CHARS + 1] = {0};
char f_infoAddressText[MAX_ADDRESS_CHARS + 1] = {0};
char f_infoPhoneText[MAX_INPUT_CHARS + 1] = {0};
char f_infoDeptText[MAX_INPUT_CHARS + 1] = {0};
char f_infoDesignationText[MAX_INPUT_CHARS + 1] = {0};

char statusMessage[128] = "Please fill all details. All fields are mandatory.";
Color statusMessageColor = DARKGRAY;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void InitStudentTextBoxes(void);
static void InitFacultyTextBoxes(void);
static void HandleTextBoxInput(TextBox *textBox);
static void SaveStudentInfo(void);
static void SaveFacultyInfo(void);
static void UpdateDrawFrame(void);

//----------------------------------------------------------------------------------
// Main Entry Point
//----------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc < 4) {
        // This program is not meant to be run directly.
        // It requires ID, password, and user type as arguments.
        return 1;
    }

    const char *id = argv[1];
    const char *password = argv[2];
    const char *userTypeStr = argv[3];

    if (strcmp(userTypeStr, "student") == 0) {
        currentUserType = USER_TYPE_STUDENT;
        memset(&currentStudent, 0, sizeof(StudentInfo));
        strcpy(currentStudent.id, id);
        strcpy(currentStudent.password, password);
        sprintf(currentStudent.email, "%s@uni.ac.in", id);
    } else if (strcmp(userTypeStr, "faculty") == 0) {
        currentUserType = USER_TYPE_FACULTY;
        memset(&currentFaculty, 0, sizeof(FacultyInfo));
        strcpy(currentFaculty.id, id);
        strcpy(currentFaculty.password, password);
        sprintf(currentFaculty.email, "%s@uni.ac.in", id);
    } else {
        return 1; // Invalid user type
    }
    
    InitWindow(screenWidth, screenHeight, "University Portal - Complete Your Profile");
    InitStudentTextBoxes();
    InitFacultyTextBoxes();
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }

    CloseWindow();
    return 0;
}

//----------------------------------------------------------------------------------
// Main Update and Draw Loop (Unchanged)
//----------------------------------------------------------------------------------
void UpdateDrawFrame(void) {
    Vector2 mousePos = GetMousePosition();
    TextBox *currentTextBoxes = (currentUserType == USER_TYPE_STUDENT) ? studentTextBoxes : facultyTextBoxes;
    int numTextBoxes = (currentUserType == USER_TYPE_STUDENT) ? NUM_STUDENT_TEXTBOXES : NUM_FACULTY_TEXTBOXES;

    // Activate Text Boxes
    for (int i = 0; i < numTextBoxes; i++) {
        if (CheckCollisionPointRec(mousePos, currentTextBoxes[i].bounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            for (int j = 0; j < numTextBoxes; j++) currentTextBoxes[j].isActive = false;
            currentTextBoxes[i].isActive = true;
        }
    }

    // Handle Text Input
    for (int i = 0; i < numTextBoxes; i++) {
        if (currentTextBoxes[i].isActive) HandleTextBoxInput(&currentTextBoxes[i]);
    }
    
    // Handle Save Button
    Rectangle saveButtonRec = { screenWidth / 2 - 150, screenHeight - 120, 300, 50 };
    if (CheckCollisionPointRec(mousePos, saveButtonRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        bool allFieldsFilled = true;
        for (int i = 0; i < numTextBoxes; i++) {
            if (strlen(currentTextBoxes[i].text) == 0) {
                allFieldsFilled = false;
                break;
            }
        }
        
        if (!allFieldsFilled) {
            strcpy(statusMessage, "Error: All fields must be filled.");
            statusMessageColor = MAROON;
        } else {
            if (currentUserType == USER_TYPE_STUDENT) {
                SaveStudentInfo();
            } else {
                SaveFacultyInfo();
            }
            // After saving, the program should close.
            // A small delay could show a success message.
            strcpy(statusMessage, "Information submitted successfully! You can now log in.");
            statusMessageColor = LIME;
            
            // Draw one last frame with the success message then exit.
            BeginDrawing();
            ClearBackground(RAYWHITE);
             DrawText(statusMessage, screenWidth / 2 - MeasureText(statusMessage, 40) / 2, screenHeight / 2 - 20, 40, LIME);
            EndDrawing();
            WaitTime(2.5);
            system("start main.exe"); // Wait 2.5 seconds
            exit(0);
        }
    }

    // --- Drawing ---
    BeginDrawing();
    ClearBackground(RAYWHITE);
    
    if (currentUserType == USER_TYPE_STUDENT) {
         DrawText("Student Information Registration", screenWidth / 2 - MeasureText("Student Information Registration", 40) / 2, 40, 40, DARKGRAY);
         // Display read-only info
        DrawText(TextFormat("Student ID: %s", currentStudent.id), 50, 120, 20, GRAY);
        DrawText(TextFormat("Email: %s", currentStudent.email), 50, 150, 20, GRAY);
    } else {
         DrawText("Faculty Information Registration", screenWidth / 2 - MeasureText("Faculty Information Registration", 40) / 2, 40, 40, DARKGRAY);
        // Display read-only info
        DrawText(TextFormat("Faculty ID: %s", currentFaculty.id), 50, 120, 20, GRAY);
        DrawText(TextFormat("Email: %s", currentFaculty.email), 50, 150, 20, GRAY);
    }
    
    // Draw Textboxes
    int currentY = 220;
    for (int i = 0; i < numTextBoxes; i++) {
        currentTextBoxes[i].bounds.x = screenWidth / 2 - 200;
        currentTextBoxes[i].bounds.y = currentY;
        DrawText(currentTextBoxes[i].label, currentTextBoxes[i].bounds.x, currentTextBoxes[i].bounds.y - 25, 20, GRAY);
        DrawRectangleRec(currentTextBoxes[i].bounds, WHITE);
        DrawRectangleLinesEx(currentTextBoxes[i].bounds, 1, currentTextBoxes[i].isActive ? SKYBLUE : GRAY);
        DrawText(currentTextBoxes[i].text, currentTextBoxes[i].bounds.x + 10, currentTextBoxes[i].bounds.y + 10, 20, BLACK);
        currentY += 80;
    }
    
    // Draw Blinking Cursor
    for(int i = 0; i < numTextBoxes; i++) {
        if (currentTextBoxes[i].isActive && ((int)(GetTime() * 2.0f)) % 2 == 0) {
            int textWidth = MeasureText(currentTextBoxes[i].text, 20);
            DrawLine(currentTextBoxes[i].bounds.x + 10 + textWidth, currentTextBoxes[i].bounds.y + 10,
                     currentTextBoxes[i].bounds.x + 10 + textWidth, currentTextBoxes[i].bounds.y + 30, BLACK);
        }
    }

    // Draw Save Button
    DrawRectangleRec(saveButtonRec, LIME);
    DrawText("SAVE & SUBMIT", screenWidth / 2 - MeasureText("SAVE & SUBMIT", 20) / 2, saveButtonRec.y + 15, 20, DARKGREEN);

    // Draw status message
    DrawText(statusMessage, screenWidth / 2 - MeasureText(statusMessage, 20) / 2, saveButtonRec.y + 70, 20, statusMessageColor);

    EndDrawing();
}

//----------------------------------------------------------------------------------
// Helper and File Functions (Unchanged)
//----------------------------------------------------------------------------------
void HandleTextBoxInput(TextBox *textBox) {
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (textBox->charCount < textBox->textMaxSize)) {
            textBox->text[textBox->charCount] = (char)key;
            textBox->charCount++;
        }
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (textBox->charCount > 0) {
            textBox->charCount--;
            textBox->text[textBox->charCount] = '\0';
        }
    }
}

void InitStudentTextBoxes(void) {
    studentTextBoxes[0] = (TextBox){ {0, 0, 400, 40}, s_infoNameText, MAX_INPUT_CHARS, 0, false, "Full Name:" };
    studentTextBoxes[1] = (TextBox){ {0, 0, 400, 40}, s_infoAddressText, MAX_ADDRESS_CHARS, 0, false, "Address:" };
    studentTextBoxes[2] = (TextBox){ {0, 0, 400, 40}, s_infoPhoneText, MAX_INPUT_CHARS, 0, false, "Phone Number:" };
    studentTextBoxes[3] = (TextBox){ {0, 0, 400, 40}, s_infoCourseText, MAX_INPUT_CHARS, 0, false, "Course (e.g., B.Tech CSE):" };
    studentTextBoxes[4] = (TextBox){ {0, 0, 400, 40}, s_infoSemesterText, MAX_INPUT_CHARS, 0, false, "Semester:" };
    studentTextBoxes[5] = (TextBox){ {0, 0, 400, 40}, s_infoHeightText, MAX_INPUT_CHARS, 0, false, "Height (cm):" };
    studentTextBoxes[6] = (TextBox){ {0, 0, 400, 40}, s_infoBloodText, 4, 0, false, "Blood Group:" };
}

void InitFacultyTextBoxes(void) {
    facultyTextBoxes[0] = (TextBox){ {0, 0, 400, 40}, f_infoNameText, MAX_INPUT_CHARS, 0, false, "Full Name:" };
    facultyTextBoxes[1] = (TextBox){ {0, 0, 400, 40}, f_infoAddressText, MAX_ADDRESS_CHARS, 0, false, "Address:" };
    facultyTextBoxes[2] = (TextBox){ {0, 0, 400, 40}, f_infoPhoneText, MAX_INPUT_CHARS, 0, false, "Phone Number:" };
    facultyTextBoxes[3] = (TextBox){ {0, 0, 400, 40}, f_infoDeptText, MAX_INPUT_CHARS, 0, false, "Department:" };
    facultyTextBoxes[4] = (TextBox){ {0, 0, 400, 40}, f_infoDesignationText, MAX_INPUT_CHARS, 0, false, "Designation:" };
}

// *** FIX: CHANGED TO TEXT I/O ***
void SaveStudentInfo(void) {
    // Copy data from textboxes into the struct
    strcpy(currentStudent.name, studentTextBoxes[0].text);
    strcpy(currentStudent.address, studentTextBoxes[1].text);
    strcpy(currentStudent.phone, studentTextBoxes[2].text);
    strcpy(currentStudent.course, studentTextBoxes[3].text);
    strcpy(currentStudent.semester, studentTextBoxes[4].text);
    strcpy(currentStudent.height, studentTextBoxes[5].text);
    strcpy(currentStudent.bloodGroup, studentTextBoxes[6].text);

    // Open the correct file (student_info.txt) in text append mode ("a")
    FILE *file = fopen("student_info.txt", "a"); 
    if (!file) {
        TraceLog(LOG_ERROR, "Could not open student_info.txt for writing.");
        return;
    }

    // Write the data as text, matching the format LoadStudentInfo expects
    fprintf(file, "%s\n", currentStudent.id);
    fprintf(file, "%s\n", currentStudent.password);
    fprintf(file, "%s\n", currentStudent.name);
    fprintf(file, "%s\n", currentStudent.address);
    fprintf(file, "%s\n", currentStudent.phone);
    fprintf(file, "%s\n", currentStudent.email);
    fprintf(file, "%s\n", currentStudent.course);
    fprintf(file, "%s\n", currentStudent.semester);
    fprintf(file, "%s\n", currentStudent.height);
    fprintf(file, "%s\n", currentStudent.bloodGroup);
    fprintf(file, "--END--\n"); // Add the separator line

    fclose(file);
}

// *** FIX: CHANGED TO TEXT I/O ***
void SaveFacultyInfo(void) {
    // Copy data from textboxes into the struct
    strcpy(currentFaculty.name, facultyTextBoxes[0].text);
    strcpy(currentFaculty.address, facultyTextBoxes[1].text);
    strcpy(currentFaculty.phone, facultyTextBoxes[2].text);
    strcpy(currentFaculty.department, facultyTextBoxes[3].text);
    strcpy(currentFaculty.designation, facultyTextBoxes[4].text);

    // Open the correct file (faculty_info.txt) in text append mode ("a")
    FILE *file = fopen("faculty_info.txt", "a");
    if (!file) {
        TraceLog(LOG_ERROR, "Could not open faculty_info.txt for writing.");
        return;
    }

    // Write the data as text, matching the format LoadFacultyInfo expects
    fprintf(file, "%s\n", currentFaculty.id);
    fprintf(file, "%s\n", currentFaculty.password);
    fprintf(file, "%s\n", currentFaculty.name);
    fprintf(file, "%s\n", currentFaculty.address);
    fprintf(file, "%s\n", currentFaculty.phone);
    fprintf(file, "%s\n", currentFaculty.email);
    fprintf(file, "%s\n", currentFaculty.department);
    fprintf(file, "%s\n", currentFaculty.designation);
    fprintf(file, "--END--\n"); // Add the separator line

    fclose(file);
}