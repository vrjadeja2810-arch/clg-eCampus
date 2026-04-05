/*******************************************************************************************
*
* University Portal System - Admin Panel
*
* This program is the admin-facing management tool.
* - It uses Raylib to create, list, and delete announcements and events.
* - It reads from and writes to 'announcements.dat' and 'events.dat'.
* - Resolution set to 1920x1080 as requested.
*
* How to Compile (Windows with MinGW-w64):
* gcc admin_portal.c -o admin_portal -lraylib -lgdi32 -lwinmm -mwindows
*
********************************************************************************************/

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ----------------------------------------------------------------------------------
// Data Definitions (from original admin.c)
// ----------------------------------------------------------------------------------
#define ANN_FILE "announcements.dat"
#define EVT_FILE "events.dat"
#define TITLE_LEN 80
#define BODY_LEN 2048 // Increased from 512
#define MAX_RECORDS 10000

typedef struct {
    int id;
    char title[TITLE_LEN];
    char body[BODY_LEN];
    char created[26];
} Announcement;

typedef struct {
    int id;
    char title[TITLE_LEN];
    char body[BODY_LEN];
    char date[11]; // YYYY-MM-DD
} Event;

// ----------------------------------------------------------------------------------
// TextBox Definition (from main.c)
// ----------------------------------------------------------------------------------
typedef struct {
    Rectangle bounds;
    char *text;
    int textMaxSize;
    int charCount;
    bool isActive;
    const char* label;
} TextBox;

// ----------------------------------------------------------------------------------
// Global Variables
// ----------------------------------------------------------------------------------
const int screenWidth = 1920;
const int screenHeight = 1080;

typedef enum {
    SCREEN_MAIN_MENU,
    SCREEN_LIST_ANNOUNCEMENTS,
    SCREEN_LIST_EVENTS,
    SCREEN_ADD_ANNOUNCEMENT,
    SCREEN_ADD_EVENT,
    SCREEN_DETAIL_ANNOUNCEMENT, // NEW: Added detail screen
    SCREEN_DETAIL_EVENT         // NEW: Added detail screen
} PortalScreen;

PortalScreen currentScreen = SCREEN_MAIN_MENU;

// Loaded data
Announcement *g_ann = NULL;
int g_ann_count = 0;
Event *g_evt = NULL;
int g_evt_count = 0;

// UI state
Vector2 g_mouse_pos = { 0 };
float g_scroll_y = 0;
char g_status_msg[128] = {0};
Color g_status_color = GRAY;
int g_selected_id = -1;     // NEW: ID of item to view in detail

// Text boxes for Add forms
TextBox addTextBoxes[3]; // [0] = Title, [1] = Body, [2] = Date
char addTitle[TITLE_LEN] = {0};
char addBody[BODY_LEN] = {0}; // Note: BODY_LEN is now 2048
char addDate[12] = {0}; // YYYY-MM-DD + NUL

// ----------------------------------------------------------------------------------
// Helper Functions (from original admin.c)
// ----------------------------------------------------------------------------------
void now_str(char *buf, size_t n) {
    time_t t = time(NULL);
    strncpy(buf, ctime(&t), n - 1);
    buf[n - 1] = '\0';
    char *p = strchr(buf, '\n');
    if (p) *p = '\0';
}

int is_leap(int y){ return (y%4==0 && y%100!=0) || (y%400==0); }

int valid_date(const char *s) {
    int y,m,d;
    if (!s || strlen(s) != 10) return 0;
    if (sscanf(s, "%4d-%2d-%2d", &y, &m, &d) != 3) return 0;
    if (s[4] != '-' || s[7] != '-') return 0; // Stricter format check
    if (y < 1900 || y > 2100 || m < 1 || m > 12 || d < 1) return 0;
    int days[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2 && is_leap(y)) days[2] = 29;
    if (d > days[m]) return 0;
    return 1;
}

// ----------------------------------------------------------------------------------
// File I/O Functions (from original admin.c)
// ----------------------------------------------------------------------------------
int load_announcements(Announcement **out, int *count) {
    *out = NULL; *count = 0;
    FILE *f = fopen(ANN_FILE, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    long sz = ftell(f);
    if (sz <= 0) { fclose(f); return 0; }
    rewind(f);
    int slots = (int)(sz / sizeof(Announcement));
    if (slots <= 0) { fclose(f); return 0; }
    if (slots > MAX_RECORDS) slots = MAX_RECORDS;
    Announcement *arr = calloc(slots, sizeof(Announcement));
    if (!arr) { fclose(f); return -1; }
    int read = fread(arr, sizeof(Announcement), slots, f);
    fclose(f);
    if (read <= 0) { free(arr); return 0; }
    *out = arr; *count = read; return 1;
}

int save_announcements(Announcement *arr, int count) {
    FILE *f = fopen(ANN_FILE, "wb");
    if (!f) return -1;
    fwrite(arr, sizeof(Announcement), count, f);
    fclose(f); return 0;
}

int load_events(Event **out, int *count) {
    *out = NULL; *count = 0;
    FILE *f = fopen(EVT_FILE, "rb");
    if (!f) return 0;
    if (fseek(f,0,SEEK_END)!=0){ fclose(f); return -1; }
    long sz = ftell(f); if (sz <= 0){ fclose(f); return 0; }
    rewind(f);
    int slots = (int)(sz / sizeof(Event));
    if (slots <= 0) { fclose(f); return 0; }
    if (slots > MAX_RECORDS) slots = MAX_RECORDS;
    Event *arr = calloc(slots, sizeof(Event));
    if (!arr) { fclose(f); return -1; }
    int read = fread(arr, sizeof(Event), slots, f);
    fclose(f);
    if (read <= 0) { free(arr); return 0; }
    *out = arr; *count = read; return 1;
}

int save_events(Event *arr, int count) {
    FILE *f = fopen(EVT_FILE, "wb");
    if (!f) return -1;
    fwrite(arr, sizeof(Event), count, f);
    fclose(f); return 0;
}

// ----------------------------------------------------------------------------------
// GUI Helper Functions
// ----------------------------------------------------------------------------------
void ReloadData(void) {
    free(g_ann); g_ann = NULL; g_ann_count = 0;
    free(g_evt); g_evt = NULL; g_evt_count = 0;
    load_announcements(&g_ann, &g_ann_count);
    load_events(&g_evt, &g_evt_count);
}

void InitAddTextBoxes(void) {
    // Centered layout for 1920x1080
    int posX = screenWidth / 2 - 400; // 960 - 400 = 560
    int width = 800;
    addTextBoxes[0] = (TextBox){ (Rectangle){ posX, 200, width, 40 }, addTitle, TITLE_LEN - 1, 0, false, "Title:" };
    addTextBoxes[1] = (TextBox){ (Rectangle){ posX, 300, width, 400 }, addBody, BODY_LEN - 1, 0, false, "Body:" }; // Increased height
    addTextBoxes[2] = (TextBox){ (Rectangle){ posX, 730, width, 40 }, addDate, 10, 0, false, "Date (YYYY-MM-DD):" }; // Moved down
}

void ClearTextBoxes(void) {
    for (int i = 0; i < 3; i++) {
        memset(addTextBoxes[i].text, 0, addTextBoxes[i].textMaxSize + 1);
        addTextBoxes[i].charCount = 0;
    }
    g_status_msg[0] = '\0';
}

// MODIFIED: Added 'allowNewline' and support for Enter/Backspace repeat
void HandleTextBoxInput(TextBox *textBox, bool allowNewline) {
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (textBox->charCount < textBox->textMaxSize)) {
            textBox->text[textBox->charCount] = (char)key;
            textBox->charCount++;
        }
        key = GetCharPressed();
    }
    
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        if (textBox->charCount > 0) {
            textBox->charCount--;
            textBox->text[textBox->charCount] = '\0';
        }
    }
    
    if (allowNewline && (IsKeyPressed(KEY_ENTER) || IsKeyPressedRepeat(KEY_ENTER))) {
        if (textBox->charCount < textBox->textMaxSize) {
            textBox->text[textBox->charCount] = '\n';
            textBox->charCount++;
        }
    }
}

// ----------------------------------------------------------------------------------
// Module Functions Declaration
// ----------------------------------------------------------------------------------
void UpdateDrawFrame(void);
void UpdateMainMenu(void);
void DrawMainMenu(void);
void UpdateListScreen(bool isAnnouncements);
void DrawListScreen(bool isAnnouncements);
void UpdateAddScreen(bool isAnnouncements);
void DrawAddScreen(bool isAnnouncements);
void UpdateDetailView(void);        // NEW
void DrawDetailView(bool isAnnouncement); // NEW

// --- NEW HELPER FUNCTION ---
// This function draws text, manually handling newlines ('\n')
// and allowing for vertical scrolling.
// This is a replacement for DrawTextRec() for older Raylib versions.
void DrawTextScrollable(const char *text, Rectangle bounds, float scrollY, int fontSize, Color color) {
    // Create a mutable copy of the text to use with strtok
    // BODY_LEN is 2048, which is a safe buffer size.
    char textCopy[BODY_LEN];
    strncpy(textCopy, text, BODY_LEN - 1);
    textCopy[BODY_LEN - 1] = '\0'; // Ensure null termination

    int lineSpacing = fontSize + 5; // e.g., 20 + 5 = 25 pixels per line
    int drawY = (int)bounds.y + (int)scrollY;

    BeginScissorMode((int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height);

    char *line = strtok(textCopy, "\n");
    while (line != NULL) {
        // Only draw the line if it's within the visible bounds
        if ((drawY > bounds.y - lineSpacing) && (drawY < bounds.y + bounds.height)) {
            DrawText(line, (int)bounds.x, drawY, fontSize, color);
        }
        
        drawY += lineSpacing;
        line = strtok(NULL, "\n");
    }
    
    EndScissorMode();
}
// --- END NEW HELPER FUNCTION ---

// ----------------------------------------------------------------------------------
// Main Entry Point
// ----------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    if(argc<3) return 0;
    InitWindow(screenWidth, screenHeight, "University Portal - Announcements");
    
    InitAddTextBoxes();
    ReloadData();

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }

    free(g_ann);
    free(g_evt);
    CloseWindow();
    return 0;
}

// ----------------------------------------------------------------------------------
// Main Update and Draw Loop
// ----------------------------------------------------------------------------------
void UpdateDrawFrame(void) {
    g_mouse_pos = GetMousePosition();

    // Update logic
    switch (currentScreen) {
        case SCREEN_MAIN_MENU:            UpdateMainMenu(); break;
        case SCREEN_LIST_ANNOUNCEMENTS:   UpdateListScreen(true); break;
        case SCREEN_LIST_EVENTS:          UpdateListScreen(false); break;
        case SCREEN_ADD_ANNOUNCEMENT:     UpdateAddScreen(true); break;
        case SCREEN_ADD_EVENT:            UpdateAddScreen(false); break;
        case SCREEN_DETAIL_ANNOUNCEMENT:
        case SCREEN_DETAIL_EVENT:         UpdateDetailView(); break; // NEW
    }

    // Draw logic
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw header
    DrawRectangle(0, 0, screenWidth, 60, DARKBLUE);
    DrawText("Admin Panel", 20, 15, 30, RAYWHITE);

    // Draw content
    switch (currentScreen) {
        case SCREEN_MAIN_MENU:            DrawMainMenu(); break;
        case SCREEN_LIST_ANNOUNCEMENTS:   DrawListScreen(true); break;
        case SCREEN_LIST_EVENTS:          DrawListScreen(false); break;
        case SCREEN_ADD_ANNOUNCEMENT:     DrawAddScreen(true); break;
        case SCREEN_ADD_EVENT:            DrawAddScreen(false); break;
        case SCREEN_DETAIL_ANNOUNCEMENT:  DrawDetailView(true); break; // NEW
        case SCREEN_DETAIL_EVENT:         DrawDetailView(false); break; // NEW
    }

    EndDrawing();
}

// ----------------------------------------------------------------------------------
// Main Menu Screen
// ----------------------------------------------------------------------------------
void UpdateMainMenu(void) {
    int btnWidth = 500;
    int btnX = screenWidth / 2 - btnWidth / 2; // Centered
    
    Rectangle btnListAnn = { btnX, 200, btnWidth, 80 };
    Rectangle btnAddAnn  = { btnX, 300, btnWidth, 80 };
    Rectangle btnListEvt = { btnX, 400, btnWidth, 80 };
    Rectangle btnAddEvt  = { btnX, 500, btnWidth, 80 };

    if (CheckCollisionPointRec(g_mouse_pos, btnListAnn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentScreen = SCREEN_LIST_ANNOUNCEMENTS; g_scroll_y = 0;
    }
    if (CheckCollisionPointRec(g_mouse_pos, btnAddAnn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentScreen = SCREEN_ADD_ANNOUNCEMENT; ClearTextBoxes();
    }
    if (CheckCollisionPointRec(g_mouse_pos, btnListEvt) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentScreen = SCREEN_LIST_EVENTS; g_scroll_y = 0;
    }
    if (CheckCollisionPointRec(g_mouse_pos, btnAddEvt) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentScreen = SCREEN_ADD_EVENT; ClearTextBoxes();
    }
}

void DrawMainMenu(void) {
    DrawText("Main Menu", screenWidth / 2 - MeasureText("Main Menu", 40) / 2, 100, 40, DARKGRAY);

    int btnWidth = 500;
    int btnX = screenWidth / 2 - btnWidth / 2;
    
    Rectangle btnListAnn = { btnX, 200, btnWidth, 80 };
    Rectangle btnAddAnn  = { btnX, 300, btnWidth, 80 };
    Rectangle btnListEvt = { btnX, 400, btnWidth, 80 };
    Rectangle btnAddEvt  = { btnX, 500, btnWidth, 80 };

    DrawRectangleRec(btnListAnn, CheckCollisionPointRec(g_mouse_pos, btnListAnn) ? SKYBLUE : LIGHTGRAY);
    DrawText("Manage Announcements", btnListAnn.x + (btnWidth - MeasureText("Manage Announcements", 20))/2, btnListAnn.y + 30, 20, DARKGRAY);
    
    DrawRectangleRec(btnAddAnn, CheckCollisionPointRec(g_mouse_pos, btnAddAnn) ? SKYBLUE : LIGHTGRAY);
    DrawText("Add New Announcement", btnAddAnn.x + (btnWidth - MeasureText("Add New Announcement", 20))/2, btnAddAnn.y + 30, 20, DARKGRAY);
    
    DrawRectangleRec(btnListEvt, CheckCollisionPointRec(g_mouse_pos, btnListEvt) ? SKYBLUE : LIGHTGRAY);
    DrawText("Manage Events", btnListEvt.x + (btnWidth - MeasureText("Manage Events", 20))/2, btnListEvt.y + 30, 20, DARKGRAY);
    
    DrawRectangleRec(btnAddEvt, CheckCollisionPointRec(g_mouse_pos, btnAddEvt) ? SKYBLUE : LIGHTGRAY);
    DrawText("Add New Event", btnAddEvt.x + (btnWidth - MeasureText("Add New Event", 20))/2, btnAddEvt.y + 30, 20, DARKGRAY);
}

// ----------------------------------------------------------------------------------
// List View Screen
// ----------------------------------------------------------------------------------
void UpdateListScreen(bool isAnnouncements) {
    int itemCount = isAnnouncements ? g_ann_count : g_evt_count;
    float itemHeight = 60;

    // Handle Back Button
    Rectangle backBtnRec = { 20, 80, 100, 40 };
    if (CheckCollisionPointRec(g_mouse_pos, backBtnRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentScreen = SCREEN_MAIN_MENU;
    }

    // Handle Mouse Wheel Scroll
    g_scroll_y += GetMouseWheelMove() * itemHeight / 2;
    float maxScroll = (itemCount * itemHeight) - (screenHeight - 150);
    if (maxScroll < 0) maxScroll = 0;
    if (g_scroll_y > 0) g_scroll_y = 0;
    if (g_scroll_y < -maxScroll) g_scroll_y = -maxScroll;

    // Handle Delete Button
    for (int i = 0; i < itemCount; i++) {
        Rectangle itemRec = { 20, 150 + g_scroll_y + (i * itemHeight), screenWidth - 150, itemHeight - 5 };
        Rectangle delBtnRec = { screenWidth - 120, 150 + g_scroll_y + (i * itemHeight), 100, itemHeight - 5 };
        
        // Only check clicks for items visible on screen
        if (itemRec.y > 140 && itemRec.y < screenHeight)
        {
            // Check Delete click
            if (CheckCollisionPointRec(g_mouse_pos, delBtnRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
            {
                if (isAnnouncements) {
                    for (int j = i; j < g_ann_count - 1; j++) g_ann[j] = g_ann[j+1];
                    g_ann_count--;
                    save_announcements(g_ann, g_ann_count);
                } else {
                    for (int j = i; j < g_evt_count - 1; j++) g_evt[j] = g_evt[j+1];
                    g_evt_count--;
                    save_events(g_evt, g_evt_count);
                }
                ReloadData(); // Reload to get fresh pointers/counts
                break; 
            }
            
            // NEW: Check for Item click (to view details)
            if (CheckCollisionPointRec(g_mouse_pos, itemRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                g_selected_id = isAnnouncements ? g_ann[i].id : g_evt[i].id;
                currentScreen = isAnnouncements ? SCREEN_DETAIL_ANNOUNCEMENT : SCREEN_DETAIL_EVENT;
                g_scroll_y = 0; // NEW: Reset scroll for detail view
                break;
            }
        }
    }
}

void DrawListScreen(bool isAnnouncements) {
    const char *title = isAnnouncements ? "Manage Announcements" : "Manage Events";
    int itemCount = isAnnouncements ? g_ann_count : g_evt_count;
    
    DrawText(title, 140, 90, 30, DARKGRAY);

    // Draw Back Button
    Rectangle backBtnRec = { 20, 80, 100, 40 };
    bool backHover = CheckCollisionPointRec(g_mouse_pos, backBtnRec);
    DrawRectangleRec(backBtnRec, backHover ? SKYBLUE : LIGHTGRAY);
    DrawText("Back", backBtnRec.x + 30, backBtnRec.y + 10, 20, backHover ? DARKBLUE : DARKGRAY);

    // Draw List Items
    BeginScissorMode(0, 140, screenWidth, screenHeight - 140);
    
    if (itemCount == 0) {
        DrawText("No items found.", 20, 150, 20, GRAY);
    }

    for (int i = 0; i < itemCount; i++) {
        Rectangle itemRec = { 20, 150 + g_scroll_y + (i * 60), screenWidth - 150, 55 };
        Rectangle delBtnRec = { screenWidth - 120, 150 + g_scroll_y + (i * 60), 100, 55 };

        // Culling
        if (itemRec.y > screenHeight || itemRec.y + itemRec.height < 140) continue;
        
        // NEW: Add hover effect to item
        bool itemHover = CheckCollisionPointRec(g_mouse_pos, itemRec);
        DrawRectangleRec(itemRec, itemHover ? WHITE : RAYWHITE);
        DrawRectangleLinesEx(itemRec, 1, itemHover ? SKYBLUE : LIGHTGRAY);
        
        bool delHover = CheckCollisionPointRec(g_mouse_pos, delBtnRec);
        DrawRectangleRec(delBtnRec, delHover ? RED : MAROON);
        DrawText("Delete", delBtnRec.x + 20, delBtnRec.y + 18, 20, WHITE);

        if (isAnnouncements) {
            DrawText(TextFormat("ID %d: %s", g_ann[i].id, g_ann[i].title), itemRec.x + 10, itemRec.y + 10, 20, BLACK);
            DrawText(g_ann[i].created, itemRec.x + 10, itemRec.y + 30, 16, GRAY);
        } else {
            DrawText(TextFormat("ID %d: %s", g_evt[i].id, g_evt[i].title), itemRec.x + 10, itemRec.y + 10, 20, BLACK);
            DrawText(g_evt[i].date, itemRec.x + 10, itemRec.y + 30, 16, GRAY);
        }
    }
    
    EndScissorMode();
}

// ----------------------------------------------------------------------------------
// Add View Screen
// ----------------------------------------------------------------------------------
void UpdateAddScreen(bool isAnnouncements) {
    // Handle Back Button
    Rectangle backBtnRec = { 20, 80, 100, 40 };
    if (CheckCollisionPointRec(g_mouse_pos, backBtnRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentScreen = SCREEN_MAIN_MENU;
    }

    // Activate Text Boxes
    int boxCount = isAnnouncements ? 2 : 3;
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !CheckCollisionPointRec(g_mouse_pos, backBtnRec)) {
        bool boxClicked = false;
        for (int i = 0; i < boxCount; i++) {
            if (CheckCollisionPointRec(g_mouse_pos, addTextBoxes[i].bounds)) {
                for (int j = 0; j < 3; j++) addTextBoxes[j].isActive = false;
                addTextBoxes[i].isActive = true;
                boxClicked = true;
                break;
            }
        }
        if (!boxClicked) {
            for (int j = 0; j < 3; j++) addTextBoxes[j].isActive = false;
        }
    }
    
    // MODIFIED: Handle Text Input with new function
    if (addTextBoxes[0].isActive) HandleTextBoxInput(&addTextBoxes[0], false); // Title
    if (addTextBoxes[1].isActive) HandleTextBoxInput(&addTextBoxes[1], true);  // Body (allow newlines)
    if (addTextBoxes[2].isActive) HandleTextBoxInput(&addTextBoxes[2], false); // Date

    // Handle Save Button
    Rectangle saveBtnRec = { screenWidth / 2 - 100, 800, 200, 50 }; // Moved down
    if (CheckCollisionPointRec(g_mouse_pos, saveBtnRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        
        if (strlen(addTitle) == 0 || strlen(addBody) == 0) {
            strcpy(g_status_msg, "Title and Body cannot be empty.");
            g_status_color = MAROON;
            return;
        }

        if (isAnnouncements) {
            // Add Announcement logic
            Announcement *a = NULL; int n = 0;
            load_announcements(&a, &n);
            Announcement x;
            x.id = (n == 0 ? 1 : a[n-1].id + 1);
            strcpy(x.title, addTitle);
            strcpy(x.body, addBody);
            now_str(x.created, sizeof(x.created));
            
            Announcement *newarr = realloc(a, sizeof(Announcement) * (n + 1));
            if (!newarr) {
                strcpy(g_status_msg, "Memory error!"); g_status_color = RED; free(a); return;
            }
            newarr[n] = x; // Corrected
            if (save_announcements(newarr, n+1) != 0) {
                strcpy(g_status_msg, "Save failed!"); g_status_color = RED;
            }
            free(newarr);
            ReloadData();
            currentScreen = SCREEN_LIST_ANNOUNCEMENTS;

        } else {
            // Add Event logic
            if (!valid_date(addDate)) {
                strcpy(g_status_msg, "Invalid date format. Use YYYY-MM-DD.");
                g_status_color = MAROON;
                return;
            }
            Event *e = NULL; int n = 0;
            load_events(&e, &n);
            Event x;
            x.id = (n == 0 ? 1 : e[n-1].id + 1);
            strcpy(x.title, addTitle);
            strcpy(x.body, addBody);
            strcpy(x.date, addDate);
            
            Event *newarr = realloc(e, sizeof(Event) * (n + 1));
            if (!newarr) {
                strcpy(g_status_msg, "Memory error!"); g_status_color = RED; free(e); return;
            }
            newarr[n] = x;
            if (save_events(newarr, n+1) != 0) {
                strcpy(g_status_msg, "Save failed!"); g_status_color = RED;
            }
            free(newarr);
            ReloadData();
            currentScreen = SCREEN_LIST_EVENTS;
        }
    }
}

void DrawAddScreen(bool isAnnouncements) {
    const char *title = isAnnouncements ? "Add New Announcement" : "Add New Event";
    int boxCount = isAnnouncements ? 2 : 3;

    DrawText(title, screenWidth / 2 - MeasureText(title, 30) / 2, 90, 30, DARKGRAY);

    // Draw Back Button
    Rectangle backBtnRec = { 20, 80, 100, 40 };
    bool backHover = CheckCollisionPointRec(g_mouse_pos, backBtnRec);
    DrawRectangleRec(backBtnRec, backHover ? SKYBLUE : LIGHTGRAY);
    DrawText("Back", backBtnRec.x + 30, backBtnRec.y + 10, 20, backHover ? DARKBLUE : DARKGRAY);

    // --- Draw Text Boxes and Labels ---
    for (int i = 0; i < boxCount; i++) {
        DrawText(addTextBoxes[i].label, addTextBoxes[i].bounds.x, addTextBoxes[i].bounds.y - 25, 20, GRAY);
        DrawRectangleRec(addTextBoxes[i].bounds, WHITE);
        DrawRectangleLinesEx(addTextBoxes[i].bounds, 1, addTextBoxes[i].isActive ? SKYBLUE : GRAY);
        
        // MODIFIED: Use DrawTextRec for Body (i == 1), DrawText for others
        Rectangle textBounds = { 
            addTextBoxes[i].bounds.x + 5, 
            addTextBoxes[i].bounds.y + 5, // Use 5 for padding
            addTextBoxes[i].bounds.width - 10, 
            addTextBoxes[i].bounds.height - 10 
        };
        
        if (i == 1) { // Body text box (index 1)
            // --- COMPATIBILITY FIX ---
            // Manually draw text with newlines using strtok
            // This makes the "Enter" key work as expected.
            // No scrolling in this box, but it shows newlines.
            
            char textCopy[BODY_LEN];
            strncpy(textCopy, addTextBoxes[i].text, BODY_LEN - 1);
            textCopy[BODY_LEN - 1] = '\0';

            int lineSpacing = 20 + 5;
            int drawY = (int)textBounds.y;

            BeginScissorMode((int)textBounds.x, (int)textBounds.y, (int)textBounds.width, (int)textBounds.height);

            char *line = strtok(textCopy, "\n");
            while (line != NULL) {
                if (drawY < textBounds.y + textBounds.height) {
                    DrawText(line, (int)textBounds.x, drawY, 20, BLACK);
                }
                drawY += lineSpacing;
                line = strtok(NULL, "\n");
            }
            
            EndScissorMode();

        } else {
            // Use default DrawText, clipped
            BeginScissorMode((int)textBounds.x, (int)addTextBoxes[i].bounds.y - 5, (int)textBounds.width, (int)textBounds.height + 10);
            DrawText(addTextBoxes[i].text, (int)textBounds.x, (int)textBounds.y, 20, BLACK);
            EndScissorMode();
        }
    }

    // --- Draw Blinking Cursor ---
    for(int i = 0; i < boxCount; i++) {
        if (i == 1) continue; // Skip cursor for multi-line body box (it's too complex)

        if (addTextBoxes[i].isActive && ((int)(GetTime() * 2.0f)) % 2 == 0) {
            int textWidth = MeasureText(addTextBoxes[i].text, 20);
            // Don't let cursor go outside the box
            if (textWidth < addTextBoxes[i].bounds.width - 20) {
                DrawLine(addTextBoxes[i].bounds.x + 5 + textWidth, addTextBoxes[i].bounds.y + 10,
                         addTextBoxes[i].bounds.x + 5 + textWidth, addTextBoxes[i].bounds.y + 30, BLACK);
            }
        }
    }

    // --- Draw Action Button ---
    Rectangle saveBtnRec = { screenWidth / 2 - 100, 800, 200, 50 }; // Moved down
    bool saveHover = CheckCollisionPointRec(g_mouse_pos, saveBtnRec);
    DrawRectangleRec(saveBtnRec, saveHover ? LIME : GREEN);
    DrawText("SAVE", saveBtnRec.x + (saveBtnRec.width - MeasureText("SAVE", 20))/2, saveBtnRec.y + 15, 20, DARKGREEN);

    // --- Draw Status Message ---
    DrawText(g_status_msg, screenWidth / 2 - MeasureText(g_status_msg, 20) / 2, 870, 20, g_status_color); // Moved down
}

// ----------------------------------------------------------------------------------
// Detail View Screen (NEW FOR ADMIN)
// ----------------------------------------------------------------------------------
void UpdateDetailView() {
    // Check for Back Button click
    Rectangle backBtnRec = { 20, 80, 100, 40 };
    if (CheckCollisionPointRec(g_mouse_pos, backBtnRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentScreen = (currentScreen == SCREEN_DETAIL_ANNOUNCEMENT) ? SCREEN_LIST_ANNOUNCEMENTS : SCREEN_LIST_EVENTS;
        g_scroll_y = 0; // Reset scroll when going back to list
    }
    
    // NEW: Handle Mouse Wheel Scroll for the body text
    g_scroll_y += GetMouseWheelMove() * 20; // Move 20 pixels per wheel tick
    if (g_scroll_y > 0) g_scroll_y = 0;
    // NEW: Add a max scroll limit
    if (g_scroll_y < -(BODY_LEN / 20 * 25)) g_scroll_y = -(BODY_LEN / 20 * 25);
}

void DrawDetailView(bool isAnnouncement) {
    // Draw Back Button
    Rectangle backBtnRec = { 20, 80, 100, 40 };
    bool backHover = CheckCollisionPointRec(g_mouse_pos, backBtnRec);
    DrawRectangleRec(backBtnRec, backHover ? SKYBLUE : LIGHTGRAY);
    DrawText("Back", backBtnRec.x + 30, backBtnRec.y + 10, 20, backHover ? DARKBLUE : DARKGRAY);

    // Find and draw the selected item
    const char *title = "Not Found";
    const char *subtext = "";
    const char *body = "The selected item could not be found.";
    
    if (isAnnouncement) {
        for (int i = 0; i < g_ann_count; i++) {
            if (g_ann[i].id == g_selected_id) {
                title = g_ann[i].title;
                subtext = g_ann[i].created;
                body = g_ann[i].body;
                break;
            }
        }
    } else {
        for (int i = 0; i < g_evt_count; i++) {
            if (g_evt[i].id == g_selected_id) {
                title = g_evt[i].title;
                subtext = TextFormat("Date: %s", g_evt[i].date);
                body = g_evt[i].body;
                break;
            }
        }
    }

    // Draw Content
    DrawText(title, 20, 150, 30, BLACK);
    DrawText(subtext, 20, 190, 20, GRAY);
    
    DrawRectangle(20, 220, screenWidth - 40, 2, LIGHTGRAY);

    // Draw the body text with word wrapping
    Rectangle bodyRec = { 20, 240, screenWidth - 40, screenHeight - 280 };
    
    // --- MODIFICATION ---
    // Use the new helper function to draw the text.
    // This function handles newlines and scrolling.
    DrawTextScrollable(body, bodyRec, g_scroll_y, 20, DARKGRAY);
}