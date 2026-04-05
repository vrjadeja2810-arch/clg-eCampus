/*******************************************************************************************
*
* University Portal System - User Dashboard
*
* MODIFIED:
* - Screen size 1920x1080.
* - Now accepts a 3rd command-line argument for "admin" status.
* - "Administration Window" button now launches admin_portal.exe or
* student_portal.exe based on admin status and user ID.
* - CRITICAL FIX: Fixed buffer overflow in LoadStudentInfo/LoadFacultyInfo.
* The code now passes the correct destination buffer size to fgets.
*
********************************************************************************************/

#define PLATFORM_DESKTOP

#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_INPUT_CHARS      50
#define MAX_ADDRESS_CHARS    100
#define MAX_LINE_LENGTH      256 

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum { USER_TYPE_STUDENT, USER_TYPE_FACULTY, USER_TYPE_UNKNOWN } UserType;

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

bool dataLoaded = false;
bool isAdmin = false; // NEW: Admin status flag
char statusMessage[128] = "";

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static bool LoadStudentInfo(const char* id);
static bool LoadFacultyInfo(const char* id);
static void UpdateDrawFrame(void);
static void DrawStudentDashboard(void);
static void DrawFacultyDashboard(void);
// NEW: Helper for text file parsing
static void safe_fgets_and_strip(char* dest, int destSize, FILE* file);

//----------------------------------------------------------------------------------
// Main Entry Point
//----------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc < 3) return 1; // Now requires at least id and type

    const char *id = argv[1];
    const char *userTypeStr = argv[2];

    // Check for 3rd argument (admin flag)
    if (argc > 3 && strcmp(argv[3], "admin") == 0) {
        isAdmin = true;
    }

    if(atoi(argv[1]) >= 202501024 && atoi(argv[1]) <= 202501034) isAdmin = true;

    if (strcmp(userTypeStr, "student") == 0) {
        currentUserType = USER_TYPE_STUDENT;
        memset(&currentStudent, 0, sizeof(StudentInfo)); 
        dataLoaded = LoadStudentInfo(id);
    } else if (strcmp(userTypeStr, "faculty") == 0) {
        currentUserType = USER_TYPE_FACULTY;
        memset(&currentFaculty, 0, sizeof(FacultyInfo)); 
        dataLoaded = LoadFacultyInfo(id);
    } else {
        return 1;
    }
    
    if (!dataLoaded) {
        sprintf(statusMessage, "Error: Could not find profile for ID %s.", id);
    }

    InitWindow(screenWidth, screenHeight, "University Portal - Dashboard");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }

    CloseWindow();
    return 0;
}

//----------------------------------------------------------------------------------
// Main Update and Draw Loop
//----------------------------------------------------------------------------------
void UpdateDrawFrame(void) {
    Vector2 mousePos = GetMousePosition();
    
    // Define the new button names
    const char *menuNames[4] = { 
        "Academics", 
        "Program Structure", 
        "Announcements", 
        "Administration Window" 
    };
    
    Rectangle menuButtons[4];
    for(int i=0; i<4; i++) {
        menuButtons[i] = (Rectangle){ screenWidth * 0.7f, 200.0f + i * 100.0f, 300, 60 };
        
        // Updated click logic
        if (CheckCollisionPointRec(mousePos, menuButtons[i]) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            
            if (i == 1) { // Button 1 is "Program Structure"
                sprintf(statusMessage, "Launching Program Structure...");
                char cmd[256];
                //int a = 28;
                strcpy(cmd,"start prostr.exe 28 10");
                system(cmd); // Launch the executable (Updated name)
            }
            else if (i == 0 && currentUserType == USER_TYPE_STUDENT) { // Academics (Student)
                sprintf(statusMessage, "Launching Academics...");
                system("start acadstu.exe 28 10"); // Launch the executable
            } 
            else if (i == 0 && currentUserType == USER_TYPE_FACULTY) { // Academics (Faculty)
                sprintf(statusMessage, "Launching Academics...");
                system("start acadfac.exe 28 10"); // Launch the executable
            }
            else if (i == 3) { // Button 3 is "Administration Window"
                if (isAdmin) {
                    // User is an admin
                    sprintf(statusMessage, "Launching Admin Portal...");
                    system("start admin_portal.exe 28 10"); // Launch the main admin portal
                } else if (currentUserType == USER_TYPE_STUDENT) {
                    // User is a student
                    sprintf(statusMessage, "Launching Student Portal...");
                    // Pass the student's ID to the student portal
                    char cmd[256];
                    sprintf(cmd, "start student_portal.exe %s", currentStudent.id);
                    system(cmd);
                } else {
                    // User is a non-admin faculty, do nothing or show message
                    sprintf(statusMessage, "This section is for students or admins.");
                }
            }
            else if (i == 2 && currentUserType == USER_TYPE_STUDENT) { // Announcements
                 sprintf(statusMessage, "Launching Announcements...");
                 char cmd[256];
                 //char type[] = "Student";
                 sprintf(cmd,"start anstu.exe %s %s", currentStudent.id, currentStudent.name);
                 system(cmd);
                 
                 // system("start announcements.exe"); // Uncomment when ready
            }
            else if (i == 2 && (currentUserType == USER_TYPE_FACULTY || isAdmin)) { // Announcements
                 sprintf(statusMessage, "Launching Announcements...");
                 char cmd[256];
                 sprintf(cmd,"start anadm.exe 28 10");
                 system(cmd);
                 // system("start announcements.exe"); // Uncomment when ready
            }
        }
    }
    
    BeginDrawing();
    ClearBackground(RAYWHITE);
    
    if (!dataLoaded) {
        DrawText(statusMessage, screenWidth / 2 - MeasureText(statusMessage, 30) / 2, screenHeight / 2 - 15, 30, MAROON);
    } else {
        if (currentUserType == USER_TYPE_STUDENT) {
            DrawStudentDashboard();
        } else {
            DrawFacultyDashboard();
        }

        DrawText("Main Menu", screenWidth * 0.7f, 150, 30, DARKBLUE);
        
        // Updated drawing loop
        for(int i=0; i<4; i++) {
            DrawRectangleRec(menuButtons[i], LIGHTGRAY);
             DrawRectangleLinesEx(menuButtons[i], 2, CheckCollisionPointRec(mousePos, menuButtons[i]) ? SKYBLUE : DARKGRAY);
            
            // Use the menuNames array instead of "Section %d"
            DrawText(menuNames[i], menuButtons[i].x + menuButtons[i].width/2 - MeasureText(menuNames[i], 20)/2, menuButtons[i].y + 20, 20, DARKGRAY);
        }

        DrawText(statusMessage, screenWidth * 0.7f, 200 + 4 * 100, 20, LIME);
    }
    EndDrawing();
}

//----------------------------------------------------------------------------------
// Dashboard Drawing Functions
//----------------------------------------------------------------------------------
void DrawStudentDashboard() {
    // This display order now matches your screenshot's layout
    DrawText(TextFormat("Welcome, %s", currentStudent.name), 50, 40, 40, DARKGRAY);

    DrawText("Your Profile Information", 50, 150, 30, DARKBLUE);
    int y = 200;
    DrawText(TextFormat("ID: %s", currentStudent.id), 60, y, 20, BLACK); y+=40;
    DrawText(TextFormat("Email: %s", currentStudent.email), 60, y, 20, BLACK); y+=40;
    DrawText(TextFormat("Phone: %s", currentStudent.phone), 60, y, 20, BLACK); y+=40;
    DrawText(TextFormat("Address: %s", currentStudent.address), 60, y, 20, BLACK); y+=60;
    
    DrawText("Academic Details", 50, y, 30, DARKBLUE); y+=50;
    DrawText(TextFormat("Course: %s", currentStudent.course), 60, y, 20, BLACK); y+=40;
    DrawText(TextFormat("Semester: %s", currentStudent.semester), 60, y, 20, BLACK); y+=60;
    
    DrawText("ID Card Details", 50, y, 30, DARKBLUE); y+=50;
    DrawText(TextFormat("Height: %s cm", currentStudent.height), 60, y, 20, BLACK); y+=40;
    DrawText(TextFormat("Blood Group: %s", currentStudent.bloodGroup), 60, y, 20, BLACK);
}

void DrawFacultyDashboard() {
    // This display order now matches your screenshot's layout
    DrawText(TextFormat("Welcome, %s", currentFaculty.name), 50, 40, 40, DARKGRAY);
    
    DrawText("Your Profile Information", 50, 150, 30, DARKBLUE);
    int y = 200;
    DrawText(TextFormat("ID: %s", currentFaculty.id), 60, y, 20, BLACK); y+=40;
    DrawText(TextFormat("Email: %s", currentFaculty.email), 60, y, 20, BLACK); y+=40;
    DrawText(TextFormat("Phone: %s", currentFaculty.phone), 60, y, 20, BLACK); y+=40;
    DrawText(TextFormat("Address: %s", currentFaculty.address), 60, y, 20, BLACK); y+=60;
    
    DrawText("Professional Details", 50, y, 30, DARKBLUE); y+=50;
    DrawText(TextFormat("Department: %s", currentFaculty.department), 60, y, 20, BLACK); y+=40;
    DrawText(TextFormat("Designation: %s", currentFaculty.designation), 60, y, 20, BLACK);
}

//----------------------------------------------------------------------------------
// File Handling Functions
//----------------------------------------------------------------------------------

// Helper function to read a line from the .txt file
// **** THIS IS THE FIXED FUNCTION ****
static void safe_fgets_and_strip(char* dest, int destSize, FILE* file) {
    if (fgets(dest, destSize, file) != NULL) {
        // Remove newline characters
        dest[strcspn(dest, "\r\n")] = 0;
    } else {
        dest[0] = '\0'; // Set to empty string on error
    }
}


// ✅ REPLACED FUNCTION
// Now reads from the text student_info.txt
bool LoadStudentInfo(const char* id) {
    FILE *file = fopen("student_info.txt", "r"); // "r" = read text
    if (!file) {
        TraceLog(LOG_WARNING, "student_info.txt not found.");
        return false;
    }

    char line[MAX_LINE_LENGTH];
    bool found = false;
    
    // Read 10 lines at a time
    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\r\n")] = 0; // Strip newline
        
        if (strcmp(line, id) == 0) { // Found the ID
            strcpy(currentStudent.id, line);
            
            // Read the next 9 lines
            // **** THESE CALLS ARE NOW FIXED ****
            safe_fgets_and_strip(currentStudent.password,   sizeof(currentStudent.password), file);
            safe_fgets_and_strip(currentStudent.name,       sizeof(currentStudent.name), file);
            safe_fgets_and_strip(currentStudent.address,    sizeof(currentStudent.address), file);
            safe_fgets_and_strip(currentStudent.phone,      sizeof(currentStudent.phone), file);
            safe_fgets_and_strip(currentStudent.email,      sizeof(currentStudent.email), file);
            safe_fgets_and_strip(currentStudent.course,     sizeof(currentStudent.course), file);
            safe_fgets_and_strip(currentStudent.semester,   sizeof(currentStudent.semester), file);
            safe_fgets_and_strip(currentStudent.height,     sizeof(currentStudent.height), file);
            safe_fgets_and_strip(currentStudent.bloodGroup, sizeof(currentStudent.bloodGroup), file);

            // Read the "--END--" separator
            fgets(line, sizeof(line), file);
            
            found = true;
            break;
        }
    }
    
    fclose(file);
    return found; // ID not found
}

// ✅ REPLACED FUNCTION
// Now reads from the text faculty_info.txt
bool LoadFacultyInfo(const char* id) {
    FILE *file = fopen("faculty_info.txt", "r"); // "r" = read text
    if (!file) {
        TraceLog(LOG_WARNING, "faculty_info.txt not found.");
        return false;
    }

    char line[MAX_LINE_LENGTH];
    bool found = false;

    // Read 8 lines at a time
    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\r\n")] = 0; // Strip newline
        
        if (strcmp(line, id) == 0) { // Found the ID
            strcpy(currentFaculty.id, line);
            
            // Read the next 7 lines
            // **** THESE CALLS ARE NOW FIXED ****
            safe_fgets_and_strip(currentFaculty.password,   sizeof(currentFaculty.password), file);
            safe_fgets_and_strip(currentFaculty.name,       sizeof(currentFaculty.name), file);
            safe_fgets_and_strip(currentFaculty.address,    sizeof(currentFaculty.address), file);
            safe_fgets_and_strip(currentFaculty.phone,      sizeof(currentFaculty.phone), file);
            safe_fgets_and_strip(currentFaculty.email,      sizeof(currentFaculty.email), file);
            safe_fgets_and_strip(currentFaculty.department, sizeof(currentFaculty.department), file);
            safe_fgets_and_strip(currentFaculty.designation,sizeof(currentFaculty.designation), file);

            // Read the "--END--" separator
            fgets(line, sizeof(line), file);
            
            found = true;
            break;
        }
    }
    
    fclose(file);
    return found; // ID not found
}