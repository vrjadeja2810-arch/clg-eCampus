/*******************************************************************************************
*
* University Portal System - Login & Registration
*
* This program provides the main authentication interface.
* - Handles student and faculty login and registration.
* - Launches 'afterreg.exe' for new users or 'afterlog.exe' for existing users.
*
* How to Compile (Windows with MinGW-w64):
* This file does NOT take command-line arguments, so we use -mwindows to hide the console.
* gcc login_system.c -o login_system -lraylib -lgdi32 -lwinmm -mwindows
*
********************************************************************************************/

#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_INPUT_CHARS 50

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum { AUTH_MODE_LOGIN, AUTH_MODE_REGISTER } AuthMode;
typedef enum { USER_TYPE_STUDENT, USER_TYPE_FACULTY } UserType;

typedef struct {
    Rectangle bounds;
    char *text;
    int textMaxSize;
    int charCount;
    bool isActive;
    bool isPassword;
    const char* label;
} TextBox;

//----------------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------------
const int screenWidth = 1920;
const int screenHeight = 1080;

AuthMode currentAuthMode = AUTH_MODE_LOGIN;
UserType currentUserType = USER_TYPE_STUDENT;

TextBox authTextBoxes[3];
char authIdText[MAX_INPUT_CHARS + 1] = {0};
char authPassText[MAX_INPUT_CHARS + 1] = {0};
char authConfirmPassText[MAX_INPUT_CHARS + 1] = {0};

char statusMessage[128] = "Please enter your credentials.";
Color statusMessageColor = LIGHTGRAY;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static bool UserExists(const char *filename, const char *id);
static bool AuthenticateUser(const char *filename, const char *id, const char *password);
static void SaveUser(const char *filename, const char *id, const char *password);
static void InitAuthTextBoxes(void);
static void ClearTextBoxes(TextBox *textBoxes, int count);
static void HandleTextBoxInput(TextBox *textBox);
static void UpdateDrawFrame(void);
static void UpdateAuthScreen(void);
static void DrawAuthScreen(void);

//----------------------------------------------------------------------------------
// Main Entry Point
//----------------------------------------------------------------------------------
int main(void) {
    InitWindow(screenWidth, screenHeight, "University Portal - Authentication");
    InitAuthTextBoxes();
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
    UpdateAuthScreen();

    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawAuthScreen();
    EndDrawing();
}

//----------------------------------------------------------------------------------
// Auth Screen Functions
//----------------------------------------------------------------------------------
void UpdateAuthScreen(void) {
    const char *currentFilename = (currentUserType == USER_TYPE_STUDENT) ? "students.dat" : "faculties.dat";
    Vector2 mousePos = GetMousePosition();

    // Toggle User Type
    Rectangle studentToggleRec = { screenWidth / 2 - 150, 80, 140, 40 };
    if (CheckCollisionPointRec(mousePos, studentToggleRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentUserType = USER_TYPE_STUDENT;
        ClearTextBoxes(authTextBoxes, 3);
        strcpy(statusMessage, "Switched to Student mode.");
        statusMessageColor = LIGHTGRAY;
    }
    Rectangle facultyToggleRec = { screenWidth / 2 + 10, 80, 140, 40 };
    if (CheckCollisionPointRec(mousePos, facultyToggleRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentUserType = USER_TYPE_FACULTY;
        ClearTextBoxes(authTextBoxes, 3);
        strcpy(statusMessage, "Switched to Faculty mode.");
        statusMessageColor = LIGHTGRAY;
    }

    // Toggle Auth Mode
    Rectangle loginToggleRec = { screenWidth / 2 - 150, 140, 140, 40 };
    if (CheckCollisionPointRec(mousePos, loginToggleRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentAuthMode = AUTH_MODE_LOGIN;
        ClearTextBoxes(authTextBoxes, 3);
        strcpy(statusMessage, "Please enter your credentials.");
        statusMessageColor = LIGHTGRAY;
    }
    Rectangle registerToggleRec = { screenWidth / 2 + 10, 140, 140, 40 };
    if (CheckCollisionPointRec(mousePos, registerToggleRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentAuthMode = AUTH_MODE_REGISTER;
        ClearTextBoxes(authTextBoxes, 3);
        strcpy(statusMessage, "Please create a new account.");
        statusMessageColor = LIGHTGRAY;
    }

    // Activate Text Boxes
    for (int i = 0; i < 3; i++) {
        if (CheckCollisionPointRec(mousePos, authTextBoxes[i].bounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            for (int j = 0; j < 3; j++) authTextBoxes[j].isActive = false;
            authTextBoxes[i].isActive = true;
        }
    }

    // Handle Text Input
    for (int i = 0; i < 3; i++) {
        if (authTextBoxes[i].isActive) HandleTextBoxInput(&authTextBoxes[i]);
    }

    // Handle Action Button
    Rectangle actionButtonRec = { screenWidth / 2 - 150, 420, 300, 50 };
    if (CheckCollisionPointRec(mousePos, actionButtonRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        const char *id = authTextBoxes[0].text;
        const char *pass = authTextBoxes[1].text;
        const char *confirmPass = authTextBoxes[2].text;
        const char *userTypeStr = (currentUserType == USER_TYPE_STUDENT) ? "student" : "faculty";
        char command[256];

        if (currentAuthMode == AUTH_MODE_LOGIN) {
            if (strlen(id) == 0 || strlen(pass) == 0) {
                strcpy(statusMessage, "ID and Password cannot be empty.");
                statusMessageColor = MAROON;
            } else if (AuthenticateUser(currentFilename, id, pass)) {
                // Launch the after-login program and exit
                sprintf(command, "start afterlog.exe %s %s", id, userTypeStr);
                system(command);
                exit(0); // Close the login portal
            } else {
                strcpy(statusMessage, "Invalid ID or Password.");
                statusMessageColor = MAROON;
            }
        } else { // AUTH_MODE_REGISTER
            if (strlen(id) == 0 || strlen(pass) == 0 || strlen(confirmPass) == 0) {
                strcpy(statusMessage, "All fields are required.");
                statusMessageColor = MAROON;
            } else if (strcmp(pass, confirmPass) != 0) {
                strcpy(statusMessage, "Passwords do not match.");
                statusMessageColor = MAROON;
            } else if (UserExists(currentFilename, id)) {
                strcpy(statusMessage, "User ID already exists.");
                statusMessageColor = ORANGE;
            } else {
                SaveUser(currentFilename, id, pass);
                // Launch the after-registration program and exit
                sprintf(command, "start afterreg.exe %s %s %s", id, pass, userTypeStr);
                system(command);
                exit(0); // Close the login portal
            }
        }
    }
}

void DrawAuthScreen(void) {
    DrawText("University Portal", screenWidth / 2 - MeasureText("University Portal", 40) / 2, 20, 40, DARKGRAY);

    // --- Draw Toggles ---
    Rectangle studentToggleRec = { screenWidth / 2 - 150, 80, 140, 40 };
    DrawRectangleRec(studentToggleRec, currentUserType == USER_TYPE_STUDENT ? SKYBLUE : LIGHTGRAY);
    DrawText("Student", studentToggleRec.x + 35, studentToggleRec.y + 10, 20, DARKGRAY);

    Rectangle facultyToggleRec = { screenWidth / 2 + 10, 80, 140, 40 };
    DrawRectangleRec(facultyToggleRec, currentUserType == USER_TYPE_FACULTY ? SKYBLUE : LIGHTGRAY);
    DrawText("Faculty", facultyToggleRec.x + 35, facultyToggleRec.y + 10, 20, DARKGRAY);

    Rectangle loginToggleRec = { screenWidth / 2 - 150, 140, 140, 40 };
    DrawRectangleRec(loginToggleRec, currentAuthMode == AUTH_MODE_LOGIN ? SKYBLUE : LIGHTGRAY);
    DrawText("Login", loginToggleRec.x + 45, loginToggleRec.y + 10, 20, DARKGRAY);

    Rectangle registerToggleRec = { screenWidth / 2 + 10, 140, 140, 40 };
    DrawRectangleRec(registerToggleRec, currentAuthMode == AUTH_MODE_REGISTER ? SKYBLUE : LIGHTGRAY);
    DrawText("Register", registerToggleRec.x + 30, registerToggleRec.y + 10, 20, DARKGRAY);

    // --- Draw Text Boxes and Labels ---
    for (int i = 0; i < 3; i++) {
        if (currentAuthMode == AUTH_MODE_LOGIN && i == 2) continue; // Don't draw "Confirm Password" in login mode

        DrawText(authTextBoxes[i].label, authTextBoxes[i].bounds.x, authTextBoxes[i].bounds.y - 25, 20, GRAY);
        DrawRectangleRec(authTextBoxes[i].bounds, WHITE);
        DrawRectangleLinesEx(authTextBoxes[i].bounds, 1, authTextBoxes[i].isActive ? SKYBLUE : GRAY);

        if (authTextBoxes[i].isPassword) {
            char passwordDisplay[MAX_INPUT_CHARS + 1] = {0};
            for(int k = 0; k < authTextBoxes[i].charCount; k++) passwordDisplay[k] = '*';
            DrawText(passwordDisplay, authTextBoxes[i].bounds.x + 10, authTextBoxes[i].bounds.y + 10, 20, BLACK);
        } else {
            DrawText(authTextBoxes[i].text, authTextBoxes[i].bounds.x + 10, authTextBoxes[i].bounds.y + 10, 20, BLACK);
        }
    }

    // --- Draw Blinking Cursor ---
    for(int i = 0; i < 3; i++) {
        if (authTextBoxes[i].isActive && ((int)(GetTime() * 2.0f)) % 2 == 0) {
            int textWidth;
            if (authTextBoxes[i].isPassword) {
                char passwordDisplay[MAX_INPUT_CHARS + 1] = {0};
                for(int k=0; k < authTextBoxes[i].charCount; k++) passwordDisplay[k] = '*';
                textWidth = MeasureText(passwordDisplay, 20);
            } else {
                textWidth = MeasureText(authTextBoxes[i].text, 20);
            }
            DrawLine(authTextBoxes[i].bounds.x + 10 + textWidth, authTextBoxes[i].bounds.y + 10,
                     authTextBoxes[i].bounds.x + 10 + textWidth, authTextBoxes[i].bounds.y + 30, BLACK);
        }
    }

    // --- Draw Action Button ---
    const char *buttonText = (currentAuthMode == AUTH_MODE_LOGIN) ? "LOGIN" : "REGISTER";
    Rectangle actionButtonRec = { screenWidth / 2 - 150, 420, 300, 50 };
    DrawRectangleRec(actionButtonRec, SKYBLUE);
    DrawText(buttonText, screenWidth / 2 - MeasureText(buttonText, 20) / 2, actionButtonRec.y + 15, 20, DARKBLUE);

    // --- Draw Status Message ---
    DrawText(statusMessage, screenWidth / 2 - MeasureText(statusMessage, 20) / 2, 500, 20, statusMessageColor);
}

//----------------------------------------------------------------------------------
// Helper and File Functions
//----------------------------------------------------------------------------------
void InitAuthTextBoxes(void) {
    int posX = screenWidth / 2 - 150;
    authTextBoxes[0] = (TextBox){ (Rectangle){ posX, 220, 300, 40 }, authIdText, MAX_INPUT_CHARS, 0, false, false, "ID:" };
    authTextBoxes[1] = (TextBox){ (Rectangle){ posX, 290, 300, 40 }, authPassText, MAX_INPUT_CHARS, 0, false, true, "Password:" };
    authTextBoxes[2] = (TextBox){ (Rectangle){ posX, 360, 300, 40 }, authConfirmPassText, MAX_INPUT_CHARS, 0, false, true, "Confirm Password:" };
}

void ClearTextBoxes(TextBox *textBoxes, int count) {
    for (int i = 0; i < count; i++) {
        memset(textBoxes[i].text, 0, textBoxes[i].textMaxSize + 1);
        textBoxes[i].charCount = 0;
    }
}

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

bool UserExists(const char *filename, const char *id) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) return false;
    char fileId[MAX_INPUT_CHARS + 1], filePass[MAX_INPUT_CHARS + 1];
    bool found = false;
    while (fscanf(file, "%s %s", fileId, filePass) == 2) {
        if (strcmp(fileId, id) == 0) {
            found = true;
            break;
        }
    }
    fclose(file);
    return found;
}

bool AuthenticateUser(const char *filename, const char *id, const char *password) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) return false;
    char fileId[MAX_INPUT_CHARS + 1], filePass[MAX_INPUT_CHARS + 1];
    bool authenticated = false;
    while (fscanf(file, "%s %s", fileId, filePass) == 2) {
        if (strcmp(fileId, id) == 0 && strcmp(filePass, password) == 0) {
            authenticated = true;
            break;
        }
    }
    fclose(file);
    return authenticated;
}

void SaveUser(const char *filename, const char *id, const char *password) {
    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        strcpy(statusMessage, "Error: Could not open data file.");
        statusMessageColor = RED;
        return;
    }
    fprintf(file, "%s %s\n", id, password);
    fclose(file);
}

