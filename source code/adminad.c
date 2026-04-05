/*******************************************************************************************
*
* University Admin Portal - C and Raylib
*
* MODIFIED:
* - Screen size set to 1920x1080.
* - All UI elements rescaled for the new resolution.
* - LOGIN SCREEN REMOVED. Starts directly at the dashboard.
* - "Manage Users" tab completely rewritten:
* - Loads all users from student_info.txt and faculty_info.txt.
* - Uses the full StudentInfo and FacultyInfo structs from afterreg.c.
* - Allows REMOVING users (rewrites the .txt file).
* - Allows ADDING users (launches main.exe).
* - Added a REFRESH button to reload user lists.
* - File I/O for users now correctly parses/writes .txt files.
* -
* - FIX: All DrawButton() calls moved into the drawing loop to make
* - buttons visible.
*
********************************************************************************************/

#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define MAX_FACULTIES 50
#define MAX_STUDENTS 100
#define MAX_FEES 100
#define MAX_COMPLAINTS 100
#define MAX_INPUT_CHARS 50
#define MAX_ADDRESS_CHARS 100
#define MAX_COMPLAINT_CHARS 256
#define MAX_LINE_LENGTH 256

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// REMOVED SCREEN_LOGIN
typedef enum {
    SCREEN_MAIN_DASHBOARD
} GameScreen;

// Copied from afterreg.c for full user info
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


// Struct for fee data (unchanged)
typedef struct {
    char id[MAX_INPUT_CHARS];
    char password[MAX_INPUT_CHARS];
} UserLogin;

// Struct for fee data
typedef struct {
    char studentId[MAX_INPUT_CHARS];
    int tuitionTotal;
    int tuitionPaid;
    int hostelTotal;
    int hostelPaid;
} FeeData;

// Struct for complaints (unchanged)
typedef struct {
    char studentId[MAX_INPUT_CHARS];
    char message[MAX_COMPLAINT_CHARS];
} Complaint;

// Enum to track active text box
typedef enum {
    TEXTBOX_NONE,
    // REMOVED LOGIN TEXTBOXES
    TEXTBOX_FEE_SEARCH_ID,
    TEXTBOX_FEE_TUITION_TOTAL,
    TEXTBOX_FEE_TUITION_PAID,
    TEXTBOX_FEE_HOSTEL_TOTAL,
    TEXTBOX_FEE_HOSTEL_PAID,
    // REMOVED SIMPLE ADD USER TEXTBOXES
} ActiveTextBox;

//----------------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------------
const int screenWidth = 1920;
const int screenHeight = 1080;

GameScreen currentScreen = SCREEN_MAIN_DASHBOARD; // Start at dashboard
int activeTab = 0; // 0=Fees, 1=Users, 2=Complaints
Vector2 scroll = { 0 };
int framesCounter = 0; // For blinking cursor

// --- User Management Data (NEW) ---
StudentInfo allStudents[MAX_STUDENTS];
int allStudentCount = 0;
FacultyInfo allFaculties[MAX_FACULTIES];
int allFacultyCount = 0;
Vector2 studentScroll = { 0 };
Vector2 facultyScroll = { 0 };


// --- Fee Management Data (Unchanged) ---
FeeData fees[MAX_FEES];
int feeCount = 0;
FeeData currentFeeEntry = { 0 }; // The entry being edited
int currentFeeIndex = -1; // Index in the fees array, -1 if new
char feeSearchId[MAX_INPUT_CHARS] = { 0 };
char feeTuitionTotalStr[MAX_INPUT_CHARS] = { 0 };
char feeTuitionPaidStr[MAX_INPUT_CHARS] = { 0 };
char feeHostelTotalStr[MAX_INPUT_CHARS] = { 0 };
char feeHostelPaidStr[MAX_INPUT_CHARS] = { 0 };
bool feeEditorActive = false;
char statusMessage[100] = { 0 };

// --- Complaint Data (Unchanged) ---
Complaint complaints[MAX_COMPLAINTS];
int complaintCount = 0; // <<< FIX: ADD THIS LINE BACK
// --- Text Input State (Removed login vars) ---
ActiveTextBox activeTextBox = TEXTBOX_NONE;
// char loginId[MAX_INPUT_CHARS] = { 0 }; // REMOVED
// char loginPass[MAX_INPUT_CHARS] = { 0 }; // REMOVED
// char loginError[100] = { 0 }; // REMOVED


//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);
// static void UpdateDrawLoginScreen(void); // REMOVED
static void UpdateDrawMainScreen(void);

// --- File I/O Functions ---
// static void LoadFaculties(void); // REMOVED (simple login)
// static void LoadStudents(void); // REMOVED (simple login)
static void LoadFees(void);
static void SaveFees(void);
static void LoadComplaints(void);

// NEW Full User I/O
static void LoadAllStudentInfo(void);
static void SaveAllStudentInfo(void);
static void LoadAllFacultyInfo(void);
static void SaveAllFacultyInfo(void);
static void safe_fgets_and_strip_admin(char* dest, int n, FILE* file, int maxLen);


// --- GUI Helper Functions ---
static void DrawMyTextBox(Rectangle bounds, const char *text, bool active);
static void HandleTextInput(char *buffer, int maxChars);
static bool DrawButton(Rectangle bounds, const char* text, Color color); // NEW Button helper

//----------------------------------------------------------------------------------
// Main Entry Point
//----------------------------------------------------------------------------------
int main(int argc, char *argv[])
{   if(argc<3) return 0;
    InitWindow(screenWidth, screenHeight, "University Admin Portal");

    // Load initial data
    // LoadFaculties(); // REMOVED
    LoadAllStudentInfo(); // NEW
    LoadAllFacultyInfo(); // NEW
    LoadFees();
    LoadComplaints();

    SetTargetFPS(60);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }
#endif

    CloseWindow();
    return 0;
}

//----------------------------------------------------------------------------------
// Main Update/Draw Loop
//----------------------------------------------------------------------------------
void UpdateDrawFrame(void)
{
    framesCounter++;

    switch (currentScreen)
    {
        // case SCREEN_LOGIN: // REMOVED
        //     UpdateDrawLoginScreen();
        //     break;
        case SCREEN_MAIN_DASHBOARD:
            UpdateDrawMainScreen();
            break;
        default:
            UpdateDrawMainScreen(); // Default to main screen
            break;
    }
}

//----------------------------------------------------------------------------------
// File I/O Functions
//----------------------------------------------------------------------------------

// Helper function to read a line from the .txt file
static void safe_fgets_and_strip_admin(char* dest, int n, FILE* file, int maxLen) {
    if (fgets(dest, n, file) != NULL) {
        dest[strcspn(dest, "\r\n")] = 0; // Strip newline
        dest[maxLen] = '\0'; 
    } else {
        dest[0] = '\0'; // Set to empty string on error
    }
}

void LoadAllStudentInfo(void) {
    FILE *file = fopen("student_info.txt", "r");
    if (!file) {
        TraceLog(LOG_WARNING, "student_info.txt not found.");
        return;
    }

    allStudentCount = 0;
    char line[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), file) != NULL && allStudentCount < MAX_STUDENTS) {
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) < 2) continue; // Skip empty lines

        StudentInfo* s = &allStudents[allStudentCount];
        
        strcpy(s->id, line);
        safe_fgets_and_strip_admin(s->password,   sizeof(line), file, MAX_INPUT_CHARS);
        safe_fgets_and_strip_admin(s->name,       sizeof(line), file, MAX_INPUT_CHARS);
        safe_fgets_and_strip_admin(s->address,    sizeof(line), file, MAX_ADDRESS_CHARS);
        safe_fgets_and_strip_admin(s->phone,      sizeof(line), file, MAX_INPUT_CHARS);
        safe_fgets_and_strip_admin(s->email,      sizeof(line), file, MAX_INPUT_CHARS + 15);
        safe_fgets_and_strip_admin(s->course,     sizeof(line), file, MAX_INPUT_CHARS);
        safe_fgets_and_strip_admin(s->semester,   sizeof(line), file, MAX_INPUT_CHARS);
        safe_fgets_and_strip_admin(s->height,     sizeof(line), file, MAX_INPUT_CHARS);
        safe_fgets_and_strip_admin(s->bloodGroup, sizeof(line), file, 4);
        fgets(line, sizeof(line), file); // Read the "--END--" separator
        
        allStudentCount++;
    }
    fclose(file);
    TraceLog(LOG_INFO, TextFormat("Loaded %d students.", allStudentCount));
}

void SaveAllStudentInfo(void) {
    FILE *file = fopen("student_info.txt", "w"); // "w" = overwrite
    if (!file) {
        TraceLog(LOG_ERROR, "Could not open student_info.txt for writing.");
        return;
    }
    
    for (int i = 0; i < allStudentCount; i++) {
        StudentInfo* s = &allStudents[i];
        fprintf(file, "%s\n", s->id);
        fprintf(file, "%s\n", s->password);
        fprintf(file, "%s\n", s->name);
        fprintf(file, "%s\n", s->address);
        fprintf(file, "%s\n", s->phone);
        fprintf(file, "%s\n", s->email);
        fprintf(file, "%s\n", s->course);
        fprintf(file, "%s\n", s->semester);
        fprintf(file, "%s\n", s->height);
        fprintf(file, "%s\n", s->bloodGroup);
        fprintf(file, "--END--\n");
    }
    fclose(file);
    TraceLog(LOG_INFO, TextFormat("Saved %d students.", allStudentCount));
}

void LoadAllFacultyInfo(void) {
    FILE *file = fopen("faculty_info.txt", "r");
    if (!file) {
        TraceLog(LOG_WARNING, "faculty_info.txt not found.");
        return;
    }

    allFacultyCount = 0;
    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file) != NULL && allFacultyCount < MAX_FACULTIES) {
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) < 2) continue;

        FacultyInfo* f = &allFaculties[allFacultyCount];

        strcpy(f->id, line);
        safe_fgets_and_strip_admin(f->password,   sizeof(line), file, MAX_INPUT_CHARS);
        safe_fgets_and_strip_admin(f->name,       sizeof(line), file, MAX_INPUT_CHARS);
        safe_fgets_and_strip_admin(f->address,    sizeof(line), file, MAX_ADDRESS_CHARS);
        safe_fgets_and_strip_admin(f->phone,      sizeof(line), file, MAX_INPUT_CHARS);
        safe_fgets_and_strip_admin(f->email,      sizeof(line), file, MAX_INPUT_CHARS + 15);
        safe_fgets_and_strip_admin(f->department, sizeof(line), file, MAX_INPUT_CHARS);
        safe_fgets_and_strip_admin(f->designation,sizeof(line), file, MAX_INPUT_CHARS);
        fgets(line, sizeof(line), file); // Read the "--END--" separator
        
        allFacultyCount++;
    }
    fclose(file);
    TraceLog(LOG_INFO, TextFormat("Loaded %d faculties.", allFacultyCount));
}

void SaveAllFacultyInfo(void) {
    FILE *file = fopen("faculty_info.txt", "w"); // "w" = overwrite
    if (!file) {
        TraceLog(LOG_ERROR, "Could not open faculty_info.txt for writing.");
        return;
    }
    
    for (int i = 0; i < allFacultyCount; i++) {
        FacultyInfo* f = &allFaculties[i];
        fprintf(file, "%s\n", f->id);
        fprintf(file, "%s\n", f->password);
        fprintf(file, "%s\n", f->name);
        fprintf(file, "%s\n", f->address);
        fprintf(file, "%s\n", f->phone);
        fprintf(file, "%s\n", f->email);
        fprintf(file, "%s\n", f->department);
        fprintf(file, "%s\n", f->designation);
        fprintf(file, "--END--\n");
    }
    fclose(file);
    TraceLog(LOG_INFO, TextFormat("Saved %d faculties.", allFacultyCount));
}


// --- Fee and Complaint I/O (Unchanged) ---

void LoadFees(void) {
    FILE* file = fopen("fees.dat", "r");
    if (file == NULL) {
        TraceLog(LOG_INFO, "fees.dat not found. A new one will be created.");
        return;
    }
    feeCount = 0;
    while (feeCount < MAX_FEES &&
           fscanf(file, "%[^|]|%d|%d|%d|%d\n",
                  fees[feeCount].studentId,
                  &fees[feeCount].tuitionTotal,
                  &fees[feeCount].tuitionPaid,
                  &fees[feeCount].hostelTotal,
                  &fees[feeCount].hostelPaid) == 5) {
        feeCount++;
    }
    fclose(file);
}

void SaveFees(void) {
    // This function rewrites the *entire* fees file.
    // First, update the fees array with the data from the editor.
    if (currentFeeIndex != -1) { // Update existing
        fees[currentFeeIndex] = currentFeeEntry;
    } else { // Add new
        if (feeCount < MAX_FEES) {
            fees[feeCount] = currentFeeEntry;
            feeCount++;
        }
    }

    FILE* file = fopen("fees.dat", "w");
    if (file == NULL) {
        TraceLog(LOG_ERROR, "Could not open fees.dat for writing.");
        return;
    }
    for (int i = 0; i < feeCount; i++) {
        fprintf(file, "%s|%d|%d|%d|%d\n",
                fees[i].studentId,
                fees[i].tuitionTotal,
                fees[i].tuitionPaid,
                fees[i].hostelTotal,
                fees[i].hostelPaid);
    }
    fclose(file);
    
    // Reload fees to ensure consistency
    LoadFees();
    sprintf(statusMessage, "Fees for %s saved.", currentFeeEntry.studentId);
}


void LoadComplaints(void) {
    FILE* file = fopen("complaints.dat", "r");
    if (file == NULL) {
        TraceLog(LOG_INFO, "complaints.dat not found.");
        return;
    }
    complaintCount = 0;
    while (complaintCount < MAX_COMPLAINTS &&
           fscanf(file, "%[^|]|%[^\n]\n",
                  complaints[complaintCount].studentId,
                  complaints[complaintCount].message) == 2) {
        complaintCount++;
    }
    fclose(file);
}

// REMOVED AddStudentToFile and AddFacultyToFile (simple versions)

//----------------------------------------------------------------------------------
// GUI Helper Functions
//----------------------------------------------------------------------------------

// A simple text box drawing function
void DrawMyTextBox(Rectangle bounds, const char *text, bool active) {
    DrawRectangleRec(bounds, LIGHTGRAY);
    if (active) {
        DrawRectangleLinesEx(bounds, 2, BLUE);
        // Draw blinking cursor
        if (((framesCounter / 30) % 2) == 0) {
            int textWidth = MeasureText(text, 20);
            DrawText("|", bounds.x + 5 + textWidth, bounds.y + 10, 20, BLACK);
        }
    } else {
        DrawRectangleLinesEx(bounds, 1, DARKGRAY);
    }
    DrawText(text, bounds.x + 5, bounds.y + 10, 20, BLACK);
}

// NEW Button Helper
static bool DrawButton(Rectangle bounds, const char* text, Color color)
{
    Vector2 mousePos = GetMousePosition();
    bool clicked = false;
    Color buttonColor = color;
    
    if (CheckCollisionPointRec(mousePos, bounds))
    {
        buttonColor = ColorBrightness(color, 0.2f); // Darken on hover
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            clicked = true;
        }
    }
    
    DrawRectangleRec(bounds, buttonColor);
    DrawText(text, bounds.x + (bounds.width - MeasureText(text, 20)) / 2, bounds.y + (bounds.height - 20) / 2, 20, WHITE);
    
    return clicked;
}


// A simple text input handler
void HandleTextInput(char *buffer, int maxChars) {
    int key = GetCharPressed();
    
    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (strlen(buffer) < maxChars)) {
            int len = strlen(buffer);
            buffer[len] = (char)key;
            buffer[len + 1] = '\0';
        }
        key = GetCharPressed(); // Check for next char
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = strlen(buffer);
        if (len > 0) {
            buffer[len - 1] = '\0';
        }
    }
}


//----------------------------------------------------------------------------------
// Screen: Login (REMOVED)
//----------------------------------------------------------------------------------
// void UpdateDrawLoginScreen(void) { ... } // ENTIRE FUNCTION REMOVED


//----------------------------------------------------------------------------------
// Screen: Main Dashboard
//----------------------------------------------------------------------------------
void UpdateDrawMainScreen(void)
{
    // --- Update Logic ---
    Vector2 mousePos = GetMousePosition();
    
    // Tab button logic
    const char *tabNames[] = { "Manage Fees", "Manage Users", "View Complaints" };
    int tabCount = 3;
    Rectangle tabs[3];
    int tabWidth = (screenWidth - 100) / tabCount - 10; // Adjusted for 1920
    int tabHeight = 50; // Taller tabs
    int startX = 50;
    int startY = 100;

    for (int i = 0; i < tabCount; i++) {
        tabs[i] = (Rectangle){ startX + i * (tabWidth + 10), startY, tabWidth, tabHeight };
        if (CheckCollisionPointRec(mousePos, tabs[i]) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            activeTab = i;
            scroll = (Vector2){ 0, 0 };
            activeTextBox = TEXTBOX_NONE; // Deactivate text boxes on tab switch
            strcpy(statusMessage, ""); // Clear status
        }
    }
    
    Rectangle contentBox = { startX, startY + tabHeight + 10, screenWidth - (2 * startX), screenHeight - startY - tabHeight - 90 };

    // --- Tab 0: Manage Fees (Update) ---
    if (activeTab == 0)
    {
        Rectangle searchIdBox = { contentBox.x + 40, contentBox.y + 60, 300, 40 };
        // Rectangle searchButton = { contentBox.x + 350, contentBox.y + 60, 120, 40 }; // Button moved to draw loop
        
        // --- Input Handling ---
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mousePos, searchIdBox)) {
                activeTextBox = TEXTBOX_FEE_SEARCH_ID;
            } else {
                activeTextBox = TEXTBOX_NONE; // Default
            }
        }
        if (activeTextBox == TEXTBOX_FEE_SEARCH_ID) {
            HandleTextInput(feeSearchId, MAX_INPUT_CHARS - 1);
        }
        
        // --- Fee Editor Logic (Input) ---
        if (feeEditorActive) {
            float editorX = contentBox.x + 550; // Position editor
            Rectangle tTotalBox = { editorX, contentBox.y + 100, 250, 40 };
            Rectangle tPaidBox  = { editorX, contentBox.y + 170, 250, 40 };
            Rectangle hTotalBox = { editorX, contentBox.y + 270, 250, 40 };
            Rectangle hPaidBox  = { editorX, contentBox.y + 340, 250, 40 };
            // Rectangle saveButton = { editorX, contentBox.y + 420, 250, 40 }; // Button moved to draw loop

            // Check for text box activation
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mousePos, tTotalBox)) activeTextBox = TEXTBOX_FEE_TUITION_TOTAL;
                else if (CheckCollisionPointRec(mousePos, tPaidBox)) activeTextBox = TEXTBOX_FEE_TUITION_PAID;
                else if (CheckCollisionPointRec(mousePos, hTotalBox)) activeTextBox = TEXTBOX_FEE_HOSTEL_TOTAL;
                else if (CheckCollisionPointRec(mousePos, hPaidBox)) activeTextBox = TEXTBOX_FEE_HOSTEL_PAID;
                else if (!CheckCollisionPointRec(mousePos, searchIdBox)) activeTextBox = TEXTBOX_NONE;
            }

            // Handle text input
            if (activeTextBox == TEXTBOX_FEE_TUITION_TOTAL) HandleTextInput(feeTuitionTotalStr, 10);
            if (activeTextBox == TEXTBOX_FEE_TUITION_PAID) HandleTextInput(feeTuitionPaidStr, 10);
            if (activeTextBox == TEXTBOX_FEE_HOSTEL_TOTAL) HandleTextInput(feeHostelTotalStr, 10);
            if (activeTextBox == TEXTBOX_FEE_HOSTEL_PAID) HandleTextInput(feeHostelPaidStr, 10);
        }
    }
    // --- Tab 1: Manage Users (Update) ---
    else if (activeTab == 1)
    {
        // Update scrolling for both lists
        if (CheckCollisionPointRec(mousePos, (Rectangle){ contentBox.x, contentBox.y, contentBox.width / 2, contentBox.height }))
        {
            studentScroll.y += GetMouseWheelMove() * 20;
            if (studentScroll.y > 0) studentScroll.y = 0;
            // Add max scroll
        }
        
        if (CheckCollisionPointRec(mousePos, (Rectangle){ contentBox.x + contentBox.width / 2, contentBox.y, contentBox.width / 2, contentBox.height }))
        {
            facultyScroll.y += GetMouseWheelMove() * 20;
            if (facultyScroll.y > 0) facultyScroll.y = 0;
            // Add max scroll
        }

        // All DrawButton() calls are now in the drawing loop.
    }
    // --- Tab 2: View Complaints (Update) ---
    else if (activeTab == 2)
    {
        // Scrolling logic
        if (CheckCollisionPointRec(mousePos, contentBox)) {
            scroll.y += GetMouseWheelMove() * 20;
            if (scroll.y > 0) scroll.y = 0;
            // Add max scroll limit if needed
        }
    }


    // --- Drawing ---
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw Header
    DrawText("Admin Dashboard", 50, 40, 30, DARKGRAY);

    // Draw Tabs
    for (int i = 0; i < tabCount; i++) {
        Color tabColor = (i == activeTab) ? SKYBLUE : LIGHTGRAY;
        DrawRectangleRec(tabs[i], tabColor);
        DrawRectangleLinesEx(tabs[i], 1, GRAY);
        DrawText(tabNames[i], tabs[i].x + (tabWidth - MeasureText(tabNames[i], 20))/2, tabs[i].y + 15, 20, DARKGRAY); // Centered text
    }

    // Draw Content Area Box
    DrawRectangleRec(contentBox, WHITE);
    DrawRectangleLinesEx(contentBox, 2, LIGHTGRAY);
    
    // Draw Status Message
    DrawText(statusMessage, contentBox.x + 20, contentBox.y + contentBox.height + 10, 20, (strncmp(statusMessage, "Removed", 7) == 0) ? RED : GREEN);

    // --- Draw Tab Content ---
    
    // --- Tab 0: Manage Fees (Draw) ---
    if (activeTab == 0)
    {
        DrawText("Find Student Fees", contentBox.x + 40, contentBox.y + 20, 24, BLACK);
        DrawMyTextBox((Rectangle){ contentBox.x + 40, contentBox.y + 60, 300, 40 }, feeSearchId, (activeTextBox == TEXTBOX_FEE_SEARCH_ID));
        
        // Search Button (Moved to Draw)
        Rectangle searchButton = { contentBox.x + 350, contentBox.y + 60, 120, 40 };
        if (DrawButton(searchButton, "Search", BLUE)) {
            currentFeeIndex = -1;
            feeEditorActive = true;
            strcpy(statusMessage, "");
            for (int i = 0; i < feeCount; i++) {
                if (strcmp(fees[i].studentId, feeSearchId) == 0) {
                    currentFeeEntry = fees[i];
                    currentFeeIndex = i;
                    break;
                }
            }
            if (currentFeeIndex == -1) {
                memset(&currentFeeEntry, 0, sizeof(FeeData));
                strcpy(currentFeeEntry.studentId, feeSearchId);
            }
            sprintf(feeTuitionTotalStr, "%d", currentFeeEntry.tuitionTotal);
            sprintf(feeTuitionPaidStr, "%d", currentFeeEntry.tuitionPaid);
            sprintf(feeHostelTotalStr, "%d", currentFeeEntry.hostelTotal);
            sprintf(feeHostelPaidStr, "%d", currentFeeEntry.hostelPaid);
        }
        
        DrawLine(contentBox.x + 500, contentBox.y, contentBox.x + 500, contentBox.y + contentBox.height, LIGHTGRAY);

        if (feeEditorActive) {
            float editorX = contentBox.x + 550;
            DrawText(TextFormat("Editing Fees for: %s", currentFeeEntry.studentId), editorX, contentBox.y + 20, 24, BLACK);
            
            DrawText("Tuition Total:", editorX, contentBox.y + 75, 20, GRAY);
            DrawMyTextBox((Rectangle){ editorX, contentBox.y + 100, 250, 40 }, feeTuitionTotalStr, (activeTextBox == TEXTBOX_FEE_TUITION_TOTAL));
            
            DrawText("Tuition Paid:", editorX, contentBox.y + 145, 20, GRAY);
            DrawMyTextBox((Rectangle){ editorX, contentBox.y + 170, 250, 40 }, feeTuitionPaidStr, (activeTextBox == TEXTBOX_FEE_TUITION_PAID));

            DrawText("Hostel Total:", editorX, contentBox.y + 245, 20, GRAY);
            DrawMyTextBox((Rectangle){ editorX, contentBox.y + 270, 250, 40 }, feeHostelTotalStr, (activeTextBox == TEXTBOX_FEE_HOSTEL_TOTAL));

            DrawText("Hostel Paid:", editorX, contentBox.y + 315, 20, GRAY);
            DrawMyTextBox((Rectangle){ editorX, contentBox.y + 340, 250, 40 }, feeHostelPaidStr, (activeTextBox == TEXTBOX_FEE_HOSTEL_PAID));

            // Save Button (Moved to Draw)
            Rectangle saveButton = { editorX, contentBox.y + 420, 250, 40 };
            if (DrawButton(saveButton, "Save Changes", GREEN)) {
                currentFeeEntry.tuitionTotal = atoi(feeTuitionTotalStr);
                currentFeeEntry.tuitionPaid = atoi(feeTuitionPaidStr);
                currentFeeEntry.hostelTotal = atoi(feeHostelTotalStr);
                currentFeeEntry.hostelPaid = atoi(feeHostelPaidStr);
                SaveFees();
                feeEditorActive = false; // Close editor
            }
        }
    }
    // --- Tab 1: Manage Users (Draw) ---
    else if (activeTab == 1)
    {
        Rectangle studentBox = { contentBox.x, contentBox.y, contentBox.width / 2 - 10, contentBox.height };
        Rectangle facultyBox = { contentBox.x + contentBox.width / 2 + 10, contentBox.y, contentBox.width / 2 - 10, contentBox.height };

        // Student Side
        DrawText("Manage Students", studentBox.x + 20, studentBox.y + 20, 24, BLACK);
        Rectangle addStudentButton = { studentBox.x + 20, studentBox.y + 60, 180, 40 };
        Rectangle refreshStudentButton = { studentBox.x + 210, studentBox.y + 60, 100, 40 };
        
        if (DrawButton(addStudentButton, "Add New Student", BLUE)) {
            system("start main.exe"); // Launch registration
            strcpy(statusMessage, "Launched registration. Click Refresh.");
        }
        if (DrawButton(refreshStudentButton, "Refresh", GREEN)) {
            LoadAllStudentInfo();
            strcpy(statusMessage, "Student list refreshed.");
        }

        // Student List
        Rectangle studentListBounds = { studentBox.x + 20, studentBox.y + 120, studentBox.width - 40, studentBox.height - 140 };
        DrawRectangleRec(studentListBounds, WHITE);
        DrawRectangleLinesEx(studentListBounds, 1, DARKGRAY);
        
        BeginScissorMode(studentListBounds.x, studentListBounds.y, studentListBounds.width, studentListBounds.height);
            int studentY = studentListBounds.y + 10 + (int)studentScroll.y;
            for(int i = 0; i < allStudentCount; i++) {
                if (studentY > studentListBounds.y - 40 && studentY < studentListBounds.y + studentListBounds.height) {
                    DrawText(TextFormat("ID: %s (%s)", allStudents[i].id, allStudents[i].name), studentListBounds.x + 10, studentY, 20, GRAY);
                    Rectangle removeBtnBounds = { studentListBounds.x + studentListBounds.width - 100, studentY - 5, 80, 30 };
                    if (DrawButton(removeBtnBounds, "Remove", RED)) {
                        char removedName[100];
                        strcpy(removedName, allStudents[i].name);
                        for (int j = i; j < allStudentCount - 1; j++) {
                            allStudents[j] = allStudents[j+1];
                        }
                        allStudentCount--;
                        SaveAllStudentInfo();
                        sprintf(statusMessage, "Removed student: %s", removedName);
                        break; 
                    }
                }
                studentY += 40;
            }
        EndScissorMode();
        
        DrawLine(contentBox.x + (contentBox.width/2), contentBox.y, contentBox.x + (contentBox.width/2), contentBox.y + contentBox.height, LIGHTGRAY);

        // Faculty Side
        DrawText("Manage Faculty/Admins", facultyBox.x + 20, facultyBox.y + 20, 24, BLACK);
        Rectangle addFacultyButton = { facultyBox.x + 20, facultyBox.y + 60, 180, 40 };
        Rectangle refreshFacultyButton = { facultyBox.x + 210, facultyBox.y + 60, 100, 40 };
        
        if (DrawButton(addFacultyButton, "Add New Faculty", BLUE)) {
            system("start main.exe"); // Launch registration
            strcpy(statusMessage, "Launched registration. Click Refresh.");
        }
        if (DrawButton(refreshFacultyButton, "Refresh", GREEN)) {
            LoadAllFacultyInfo();
            strcpy(statusMessage, "Faculty list refreshed.");
        }

        // Faculty List
        Rectangle facultyListBounds = { facultyBox.x + 20, facultyBox.y + 120, facultyBox.width - 40, facultyBox.height - 140 };
        DrawRectangleRec(facultyListBounds, WHITE);
        DrawRectangleLinesEx(facultyListBounds, 1, DARKGRAY);
        
        BeginScissorMode(facultyListBounds.x, facultyListBounds.y, facultyListBounds.width, facultyListBounds.height);
            int facultyY = facultyListBounds.y + 10 + (int)facultyScroll.y;
            for(int i = 0; i < allFacultyCount; i++) {
                if (facultyY > facultyListBounds.y - 40 && facultyY < facultyListBounds.y + facultyListBounds.height) {
                    DrawText(TextFormat("ID: %s (%s)", allFaculties[i].id, allFaculties[i].name), facultyListBounds.x + 10, facultyY, 20, GRAY);
                    Rectangle removeBtnBounds = { facultyListBounds.x + facultyListBounds.width - 100, facultyY - 5, 80, 30 };
                    if (DrawButton(removeBtnBounds, "Remove", RED)) {
                        char removedName[100];
                        strcpy(removedName, allFaculties[i].name);
                        for (int j = i; j < allFacultyCount - 1; j++) {
                            allFaculties[j] = allFaculties[j+1];
                        }
                        allFacultyCount--;
                        SaveAllFacultyInfo();
                        sprintf(statusMessage, "Removed faculty: %s", removedName);
                        break;
                    }
                }
                facultyY += 40;
            }
        EndScissorMode();
    }
    // --- Tab 2: View Complaints (Draw) ---
    else if (activeTab == 2)
    {
        DrawText("Student Complaints", contentBox.x + 20, contentBox.y + 20, 24, BLACK);
        
        Rectangle complaintListBounds = { contentBox.x, contentBox.y + 60, contentBox.width, contentBox.height - 60 };
        
        BeginScissorMode(complaintListBounds.x, complaintListBounds.y, complaintListBounds.width, complaintListBounds.height);
        
        int yPos = complaintListBounds.y + 10 + (int)scroll.y;
        if (complaintCount == 0) {
            DrawText("No complaints found.", contentBox.x + 20, yPos, 20, GRAY);
        }

        for (int i = 0; i < complaintCount; i++) {
            if (yPos > complaintListBounds.y - 80 && yPos < complaintListBounds.y + complaintListBounds.height) {
                DrawText(TextFormat("From Student: %s", complaints[i].studentId), contentBox.x + 20, yPos, 20, BLACK);
                yPos += 30;
                
                // Replaced DrawTextRec with DrawText for compatibility
                DrawText(complaints[i].message, contentBox.x + 40, yPos, 20, GRAY);
                
                yPos += 40; 
                DrawLine(contentBox.x + 20, yPos, contentBox.x + contentBox.width - 20, yPos, LIGHTGRAY);
                yPos += 20;
            } else {
                 yPos += 90; // Approx height of one entry
            }
        }
        
        EndScissorMode();
    }
    
    EndDrawing();
}