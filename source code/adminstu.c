/*******************************************************************************************
*
* University Student Portal - C and Raylib
*
* MODIFIED:
* - Screen size set to 1920x1080.
* - All UI elements rescaled for new resolution.
* - LOGIN SCREEN REMOVED.
* - Now accepts Student ID as a command-line argument (argv[1]).
* - Loads fee data for that specific student on startup.
*
* - FIX: Moved DrawButton() call for submit button into the
* - drawing loop to make it visible.
*
* - ADDED: "My Complaints" tab (Tab 1) to view and delete submitted complaints.
* - ADDED: `LoadStudentComplaints()` to read complaints from file.
* - ADDED: `DeleteComplaintByIndex()` to remove complaints.
* - ADDED: Scrolling list for complaints.
* - MOVED: "Submit Complaint" to Tab 2.
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
#define MAX_FEES 100
#define MAX_INPUT_CHARS 50
#define MAX_COMPLAINT_CHARS 256
#define MAX_STUDENT_COMPLAINTS 50 // Max complaints to load into memory

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum {
    SCREEN_MAIN_DASHBOARD
} GameScreen;

typedef struct {
    char studentId[MAX_INPUT_CHARS];
    int tuitionTotal;
    int tuitionPaid;
    int hostelTotal;
    int hostelPaid;
} FeeData;

// NEW: Structure to hold loaded complaints
typedef struct {
    char studentId[MAX_INPUT_CHARS];
    char message[MAX_COMPLAINT_CHARS];
    int originalFileIndex; // The line number in the original file, for deletion
} Complaint;

typedef enum {
    TEXTBOX_NONE,
    TEXTBOX_COMPLAINT
} ActiveTextBox;

//----------------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------------
const int screenWidth = 1920;
const int screenHeight = 1080;

GameScreen currentScreen = SCREEN_MAIN_DASHBOARD; // Start at dashboard
int activeTab = 0; // 0=Fees, 1=My Complaints, 2=Submit Complaint
int framesCounter = 0; // For blinking cursor

// --- Student Data ---
char loggedInStudentId[MAX_INPUT_CHARS] = { 0 }; // Will be set from argv

// --- Fee Data ---
FeeData studentFee = { 0 };
bool feeDataFound = false;

// --- Complaint Data ---
char complaintMessage[MAX_COMPLAINT_CHARS] = { 0 }; // For NEW complaints
char statusMessage[100] = { 0 };

// NEW: Array to hold student's loaded complaints
Complaint studentComplaints[MAX_STUDENT_COMPLAINTS];
int studentComplaintCount = 0;
int complaintScrollY = 0; // For scrolling the complaints list

// --- Text Input State ---
ActiveTextBox activeTextBox = TEXTBOX_NONE;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);
static void UpdateDrawMainScreen(void);

// --- File I/O Functions ---
static void LoadStudentFeeData(const char* studentId);
static void SaveComplaint(void);
static void LoadStudentComplaints(void); // NEW
static void DeleteComplaintByIndex(int originalIndex); // NEW

// --- GUI Helper Functions ---
static void DrawMyTextBox(Rectangle bounds, const char *text, bool active);
static void DrawMyTextArea(Rectangle bounds, char *text, int maxChars, bool active);
static void HandleTextInput(char *buffer, int maxChars);
static bool DrawButton(Rectangle bounds, const char* text, Color color);


//----------------------------------------------------------------------------------
// Main Entry Point
//----------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // Need Student ID to run
    if (argc < 2) {
        printf("ERROR: No Student ID provided.\n");
        return 1;
    }
    strcpy(loggedInStudentId, argv[1]); // Set student ID from arg

    InitWindow(screenWidth, screenHeight, "University Portal - Student Portal"); // Changed title

    LoadStudentFeeData(loggedInStudentId); // Load data for this student
    LoadStudentComplaints(); // NEW: Load initial complaints for this student

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

void LoadStudentFeeData(const char* studentId) {
    FILE* file = fopen("fees.dat", "r");
    if (file == NULL) {
        feeDataFound = false;
        return;
    }
    
    feeDataFound = false;
    FeeData tempFee;
    while (fscanf(file, "%[^|]|%d|%d|%d|%d\n",
                  tempFee.studentId,
                  &tempFee.tuitionTotal,
                  &tempFee.tuitionPaid,
                  &tempFee.hostelTotal,
                  &tempFee.hostelPaid) == 5) {
        if (strcmp(tempFee.studentId, studentId) == 0) {
            studentFee = tempFee;
            feeDataFound = true;
            break;
        }
    }
    fclose(file);
}

// NEW: Loads *only* the logged-in student's complaints into the global array
void LoadStudentComplaints(void) {
    studentComplaintCount = 0; // Reset count
    FILE* file = fopen("complaints.dat", "r");
    if (file == NULL) {
        return; // No complaints file, that's fine.
    }
    
    // Buffer includes space for ID, message, pipe, and null terminator
    char line[MAX_INPUT_CHARS + MAX_COMPLAINT_CHARS + 5]; 
    int totalLinesRead = 0;
    
    while (fgets(line, sizeof(line), file) != NULL && studentComplaintCount < MAX_STUDENT_COMPLAINTS) {
        // Remove newline character if present
        line[strcspn(line, "\n")] = 0;
        line[strcspn(line, "\r")] = 0;
        
        char tempId[MAX_INPUT_CHARS];
        char tempMsg[MAX_COMPLAINT_CHARS];
        
        // Find the first '|'
        char *pipe = strchr(line, '|');
        if (pipe == NULL) {
            totalLinesRead++;
            continue; // Malformed line, skip it
        }
        
        // Copy ID
        int idLen = pipe - line;
        if (idLen >= MAX_INPUT_CHARS) idLen = MAX_INPUT_CHARS - 1;
        strncpy(tempId, line, idLen);
        tempId[idLen] = '\0';
        
        // Copy message
        strncpy(tempMsg, pipe + 1, MAX_COMPLAINT_CHARS - 1);
        tempMsg[MAX_COMPLAINT_CHARS - 1] = '\0'; // Ensure null-terminated
        
        // If this complaint is from the logged-in student, add it to our list
        if (strcmp(tempId, loggedInStudentId) == 0) {
            strcpy(studentComplaints[studentComplaintCount].studentId, tempId);
            strcpy(studentComplaints[studentComplaintCount].message, tempMsg);
            studentComplaints[studentComplaintCount].originalFileIndex = totalLinesRead;
            studentComplaintCount++;
        }
        totalLinesRead++;
    }
    fclose(file);
}

// NEW: Deletes a complaint by its *original line number* from the file
void DeleteComplaintByIndex(int originalIndex) {
    FILE* fin = fopen("complaints.dat", "r");
    if (fin == NULL) {
        strcpy(statusMessage, "Error: Cannot read complaints file.");
        return;
    }
    
    FILE* fout = fopen("complaints.tmp", "w");
    if (fout == NULL) {
        strcpy(statusMessage, "Error: Cannot create temp file.");
        fclose(fin);
        return;
    }
    
    char line[MAX_INPUT_CHARS + MAX_COMPLAINT_CHARS + 5];
    int currentLineIndex = 0;
    bool deleted = false;
    
    while (fgets(line, sizeof(line), fin) != NULL) {
        if (currentLineIndex == originalIndex) {
            // Skip this line (don't write it to fout)
            deleted = true;
        } else {
            fputs(line, fout); // Write the line to the temp file
        }
        currentLineIndex++;
    }
    
    fclose(fin);
    fclose(fout);
    
    if (deleted) {
        remove("complaints.dat");
        rename("complaints.tmp", "complaints.dat");
        strcpy(statusMessage, "Complaint marked as solved.");
    } else {
        remove("complaints.tmp"); // Just remove the temp file
        strcpy(statusMessage, "Error: Complaint not found to delete.");
    }
    
    // ALWAYS reload the list to reflect the change
    LoadStudentComplaints();
}


void SaveComplaint(void) {
    FILE* file = fopen("complaints.dat", "a"); // Append mode
    if (file == NULL) {
        TraceLog(LOG_ERROR, "Could not open complaints.dat for appending.");
        strcpy(statusMessage, "Error: Could not submit complaint.");
        return;
    }
    
    // Sanitize message: remove newlines for file format
    for (int i = 0; i < strlen(complaintMessage); i++) {
        if (complaintMessage[i] == '\n' || complaintMessage[i] == '\r') {
            complaintMessage[i] = ' ';
        }
    }
    
    fprintf(file, "%s|%s\n", loggedInStudentId, complaintMessage);
    fclose(file);
    
    strcpy(statusMessage, "Complaint submitted successfully.");
    strcpy(complaintMessage, ""); // Clear message box

    LoadStudentComplaints(); // NEW: Reload complaints list after submitting a new one
}


//----------------------------------------------------------------------------------
// GUI Helper Functions
//----------------------------------------------------------------------------------

void DrawMyTextBox(Rectangle bounds, const char *text, bool active) {
    DrawRectangleRec(bounds, LIGHTGRAY);
    if (active) {
        DrawRectangleLinesEx(bounds, 2, BLUE);
        if (((framesCounter / 30) % 2) == 0) {
            int textWidth = MeasureText(text, 20);
            DrawText("|", bounds.x + 5 + textWidth, bounds.y + 10, 20, BLACK);
        }
    } else {
        DrawRectangleLinesEx(bounds, 1, DARKGRAY);
    }
    DrawText(text, bounds.x + 5, bounds.y + 10, 20, BLACK);
}

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

void DrawMyTextArea(Rectangle bounds, char *text, int maxChars, bool active) {
    DrawRectangleRec(bounds, LIGHTGRAY);
     if (active) {
        DrawRectangleLinesEx(bounds, 2, BLUE);
    } else {
        DrawRectangleLinesEx(bounds, 1, DARKGRAY);
    }

    DrawText(text, bounds.x + 5, bounds.y + 5, 20, BLACK);
    
    if (active && ((framesCounter / 30) % 2) == 0) {
        int textWidth = MeasureText(text, 20);
        DrawText("|", bounds.x + 5 + textWidth, bounds.y + 5, 20, BLACK);
    }
}


void HandleTextInput(char *buffer, int maxChars) {
    int key = GetCharPressed();
    
    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (strlen(buffer) < maxChars)) {
            int len = strlen(buffer);
            buffer[len] = (char)key;
            buffer[len + 1] = '\0';
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = strlen(buffer);
        if (len > 0) {
            buffer[len - 1] = '\0';
        }
    }
}


//----------------------------------------------------------------------------------
// Screen: Main Dashboard
//----------------------------------------------------------------------------------
void UpdateDrawMainScreen(void)
{
    // --- Update Logic ---
    Vector2 mousePos = GetMousePosition();
    
    // CHANGED: Added "My Complaints" tab
    const char *tabNames[] = { "My Fees", "My Complaints", "Submit New Complaint" };
    int tabCount = 3; 
    Rectangle tabs[3]; // CHANGED: Size 3
    int tabWidth = (screenWidth - 100) / tabCount - 10;
    int tabHeight = 50;
    int startX = 50;
    int startY = 100;

    for (int i = 0; i < tabCount; i++) {
        tabs[i] = (Rectangle){ startX + i * (tabWidth + 10), startY, tabWidth, tabHeight };
        if (CheckCollisionPointRec(mousePos, tabs[i]) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            activeTab = i;
            activeTextBox = TEXTBOX_NONE;
            strcpy(statusMessage, "");
            if (activeTab == 1) complaintScrollY = 0; // Reset scroll on tab click
        }
    }
    
    Rectangle contentBox = { startX, startY + tabHeight + 10, screenWidth - (2 * startX), screenHeight - startY - tabHeight - 90 };

    // --- Tab 2: Submit Complaint (Update) ---
    // MOVED from tab 1 to tab 2
    if (activeTab == 2) { 
        Rectangle complaintBox = { contentBox.x + 20, contentBox.y + 110, contentBox.width - 40, 300 };
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mousePos, complaintBox)) {
                activeTextBox = TEXTBOX_COMPLAINT;
            } else {
                activeTextBox = TEXTBOX_NONE;
            }
        }
        
        if (activeTextBox == TEXTBOX_COMPLAINT) {
            HandleTextInput(complaintMessage, MAX_COMPLAINT_CHARS - 1);
        }
    }
    
    // --- NEW: Tab 1: My Complaints (Update) ---
    if (activeTab == 1) {
        // Handle scrolling
        if (CheckCollisionPointRec(mousePos, contentBox)) {
            complaintScrollY += GetMouseWheelMove() * 20; // Scroll faster
            if (complaintScrollY > 0) complaintScrollY = 0;
            
            // Calculate max scroll
            int maxScroll = (studentComplaintCount * 40) - (contentBox.height - 100); // 40px per item, 100px padding
            if (maxScroll < 0) maxScroll = 0;
            if (complaintScrollY < -maxScroll) complaintScrollY = -maxScroll;
        }
    }


    // --- Drawing ---
    BeginDrawing();
    ClearBackground(RAYWHITE);

    DrawText(TextFormat("Welcome, %s", loggedInStudentId), 50, 40, 30, DARKGRAY);

    // Draw Tabs
    for (int i = 0; i < tabCount; i++) {
        Color tabColor = (i == activeTab) ? SKYBLUE : LIGHTGRAY;
        DrawRectangleRec(tabs[i], tabColor);
        DrawRectangleLinesEx(tabs[i], 1, GRAY);
        DrawText(tabNames[i], tabs[i].x + (tabWidth - MeasureText(tabNames[i], 20))/2, tabs[i].y + 15, 20, DARKGRAY);
    }

    DrawRectangleRec(contentBox, WHITE);
    DrawRectangleLinesEx(contentBox, 2, LIGHTGRAY);
    
    DrawText(statusMessage, contentBox.x + 20, contentBox.y + contentBox.height + 10, 20, GREEN);

    // --- Draw Tab Content ---
    
    // --- Tab 0: My Fees ---
    if (activeTab == 0)
    {
        int yPos = contentBox.y + 40;
        DrawText("Fee Status Receipt", contentBox.x + 40, yPos, 30, BLACK);
        yPos += 60;
        
        if (!feeDataFound) {
            DrawText("Your fee data has not been generated by the admin yet.", contentBox.x + 40, yPos, 20, GRAY);
        } else {
            int tuitionDue = studentFee.tuitionTotal - studentFee.tuitionPaid;
            int hostelDue = studentFee.hostelTotal - studentFee.hostelPaid;
            int totalDue = tuitionDue + hostelDue;
            
            // Tuition
            DrawText("--- TUITION FEES ---", contentBox.x + 60, yPos, 20, DARKGRAY); yPos += 40;
            DrawText(TextFormat("Total Tuition:   %d", studentFee.tuitionTotal), contentBox.x + 60, yPos, 20, GRAY); yPos += 40;
            DrawText(TextFormat("Tuition Paid:    %d", studentFee.tuitionPaid), contentBox.x + 60, yPos, 20, GRAY); yPos += 40;
            DrawText(TextFormat("Tuition Due:     %d", tuitionDue), contentBox.x + 60, yPos, 20, (tuitionDue > 0) ? RED : GREEN); yPos += 50;
            
            // Hostel
            DrawText("--- HOSTEL FEES ---", contentBox.x + 60, yPos, 20, DARKGRAY); yPos += 40;
            DrawText(TextFormat("Total Hostel:    %d", studentFee.hostelTotal), contentBox.x + 60, yPos, 20, GRAY); yPos += 40;
            DrawText(TextFormat("Hostel Paid:     %d", studentFee.hostelPaid), contentBox.x + 60, yPos, 20, GRAY); yPos += 40;
            DrawText(TextFormat("Hostel Due:      %d", hostelDue), contentBox.x + 60, yPos, 20, (hostelDue > 0) ? RED : GREEN); yPos += 50;
            
            // Summary
            DrawLine(contentBox.x, yPos, contentBox.x + contentBox.width, yPos, LIGHTGRAY); yPos += 30;
            DrawText(TextFormat("TOTAL AMOUNT DUE: %d", totalDue), contentBox.x + 60, yPos, 30, (totalDue > 0) ? RED : GREEN); yPos += 50;
            
            if (totalDue <= 0) {
                DrawText("STATUS: ALL FEES PAID", contentBox.x + 60, yPos, 30, GREEN);
            } else {
                DrawText("STATUS: PENDING PAYMENT", contentBox.x + 60, yPos, 30, RED);
            }
        }
    }
    // --- NEW: Tab 1: My Complaints ---
    else if (activeTab == 1)
    {
        DrawText("My Submitted Complaints", contentBox.x + 20, contentBox.y + 20, 30, BLACK);
        
        // Use Scissor to clip the scrolling content
        BeginScissorMode(contentBox.x, contentBox.y + 60, contentBox.width, contentBox.height - 60);
        
        int yPos = contentBox.y + 70 + complaintScrollY; // Start list below title, with scroll
        
        if (studentComplaintCount == 0) {
            DrawText("You have no submitted complaints.", contentBox.x + 20, yPos, 20, GRAY);
        } else {
            for (int i = 0; i < studentComplaintCount; i++) {
                // Simple culling: only process/draw if it's roughly in the visible area
                if (yPos > contentBox.y && yPos < (contentBox.y + contentBox.height - 40)) {
                    
                    // Draw message (truncate if too long)
                    char truncatedMsg[120];
                    strncpy(truncatedMsg, studentComplaints[i].message, 119);
                    truncatedMsg[119] = '\0';
                    if (strlen(studentComplaints[i].message) > 119) strcat(truncatedMsg, "...");
                    
                    DrawText(truncatedMsg, contentBox.x + 20, yPos + 5, 20, BLACK);
                    
                    Rectangle solvedButton = { contentBox.x + contentBox.width - 140, yPos, 120, 30 };
                    if (DrawButton(solvedButton, "Solved", RED)) {
                        DeleteComplaintByIndex(studentComplaints[i].originalFileIndex);
                        complaintScrollY = 0; // Reset scroll
                        break; // List is now stale, exit loop to avoid crash
                    }
                }
                yPos += 40; // Next item
                
                // Stop processing if we're way off-screen
                if (yPos > contentBox.y + contentBox.height + 40) break;
            }
        }
        
        EndScissorMode();
    }
    // --- Tab 2: Submit Complaint ---
    // MOVED from tab 1
    else if (activeTab == 2)
    {
        DrawText("Submit New Complaint", contentBox.x + 20, contentBox.y + 20, 30, BLACK); // Changed title
        DrawText("Your Student ID will be attached automatically.", contentBox.x + 20, contentBox.y + 60, 20, GRAY);
        
        Rectangle complaintBox = { contentBox.x + 20, contentBox.y + 110, contentBox.width - 40, 300 };
        
        DrawMyTextArea(complaintBox, complaintMessage, MAX_COMPLAINT_CHARS - 1, (activeTextBox == TEXTBOX_COMPLAINT));
        
        Rectangle submitButton = { contentBox.x + 20, contentBox.y + 430, 250, 40 };
        if (DrawButton(submitButton, "Submit Complaint", BLUE)) {
            if (strlen(complaintMessage) > 10) {
                SaveComplaint();
                activeTextBox = TEXTBOX_NONE;
            } else {
                strcpy(statusMessage, "Complaint message is too short.");
            }
        }
    }
    
    EndDrawing();
}