/*******************************************************************************************
*
* University Portal System - Student/Faculty Portal
*
* This program is the main portal after a user logs in.
* - It is launched by 'login_system.exe' (main.c).
* - It receives the user's ID and type as command-line arguments.
* - It uses Raylib to display announcements and events from .dat files.
*
* How to Compile (Windows with MinGW-w64):
* This file DOES take command-line arguments, so we do NOT use -mwindows.
* gcc student_portal.c -o afterlog -lraylib -lgdi32 -lwinmm
*
********************************************************************************************/

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----------------------------------------------------------------------------------
// Data Definitions (from original student.c)
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
    char date[11];
} Event;

// ----------------------------------------------------------------------------------
// Global Variables
// ----------------------------------------------------------------------------------
const int screenWidth = 1920;  // Increased from 1280
const int screenHeight = 1080; // Increased from 720

// Screen state
typedef enum {
    SCREEN_MAIN_MENU,
    SCREEN_LIST_ANNOUNCEMENTS,
    SCREEN_LIST_EVENTS,
    SCREEN_DETAIL_ANNOUNCEMENT,
    SCREEN_DETAIL_EVENT
} PortalScreen;

PortalScreen currentScreen = SCREEN_MAIN_MENU;

// Loaded data
Announcement *g_announcements = NULL;
int g_announcement_count = 0;
Event *g_events = NULL;
int g_event_count = 0;

// Logged-in user info
char g_user_id[64] = "Unknown";
char g_user_type[64] = "User";

// UI state
Vector2 g_mouse_pos = { 0 };
float g_scroll_y = 0;       // For list scrolling
int g_selected_id = -1;     // ID of item to view in detail

// ----------------------------------------------------------------------------------
// File Loading Functions (from original student.c)
// ----------------------------------------------------------------------------------
int load_announcements(Announcement **out, int *count) {
    *out = NULL; *count = 0;
    FILE *f = fopen(ANN_FILE, "rb");
    if (!f) return 0;
    if (fseek(f,0,SEEK_END)!=0){ fclose(f); return -1; }
    long sz = ftell(f); if (sz <= 0){ fclose(f); return 0; }
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

// ----------------------------------------------------------------------------------
// Module Functions Declaration
// ----------------------------------------------------------------------------------
void UpdateDrawFrame(void);
void UpdateMainMenu(void);
void DrawMainMenu(void);
void UpdateListView(bool isAnnouncements);
void DrawListView(bool isAnnouncements);
void UpdateDetailView(void);
void DrawDetailView(bool isAnnouncement);

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
    // Capture user info from command-line args (passed by login_system)
    if (argc >= 3) {
        strncpy(g_user_id, argv[1], 63);
        strncpy(g_user_type, argv[2], 63);
        // Capitalize user type
        if (g_user_type[0] >= 'a' && g_user_type[0] <= 'z') g_user_type[0] -= 32;
    }
    if (argc < 3){
        return 0;
    }

    InitWindow(screenWidth, screenHeight, "University Portal - Announcements");
    
    // Load data
    load_announcements(&g_announcements, &g_announcement_count);
    load_events(&g_events, &g_event_count);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }

    // Unload data
    free(g_announcements);
    free(g_events);

    CloseWindow();
    return 0;
}

// ----------------------------------------------------------------------------------
// Main Update and Draw Loop
// ----------------------------------------------------------------------------------
void UpdateDrawFrame(void) {
    g_mouse_pos = GetMousePosition();

    // Update logic based on screen
    switch (currentScreen) {
        case SCREEN_MAIN_MENU:            UpdateMainMenu(); break;
        case SCREEN_LIST_ANNOUNCEMENTS:   UpdateListView(true); break;
        case SCREEN_LIST_EVENTS:          UpdateListView(false); break;
        case SCREEN_DETAIL_ANNOUNCEMENT:
        case SCREEN_DETAIL_EVENT:         UpdateDetailView(); break;
    }

    // Draw logic
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw header
    DrawRectangle(0, 0, screenWidth, 60, SKYBLUE);
    DrawText("University Portal", 20, 15, 30, DARKBLUE);
    DrawText(TextFormat("Welcome, %s %s", g_user_type, g_user_id), 
             screenWidth - MeasureText(TextFormat("Welcome, %s %s", g_user_type, g_user_id), 20) - 20, 
             20, 20, DARKBLUE);

    // Draw content based on screen
    switch (currentScreen) {
        case SCREEN_MAIN_MENU:            DrawMainMenu(); break;
        case SCREEN_LIST_ANNOUNCEMENTS:   DrawListView(true); break;
        case SCREEN_LIST_EVENTS:          DrawListView(false); break;
        case SCREEN_DETAIL_ANNOUNCEMENT:  DrawDetailView(true); break;
        case SCREEN_DETAIL_EVENT:         DrawDetailView(false); break;
    }

    EndDrawing();
}

// ----------------------------------------------------------------------------------
// Main Menu Screen
// ----------------------------------------------------------------------------------
void UpdateMainMenu(void) {
    // Button for Announcements
    Rectangle annBtnRec = { screenWidth / 2 - 250, 300, 500, 100 };
    if (CheckCollisionPointRec(g_mouse_pos, annBtnRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentScreen = SCREEN_LIST_ANNOUNCEMENTS;
        g_scroll_y = 0;
    }

    // Button for Events
    Rectangle evtBtnRec = { screenWidth / 2 - 250, 420, 500, 100 };
    if (CheckCollisionPointRec(g_mouse_pos, evtBtnRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentScreen = SCREEN_LIST_EVENTS;
        g_scroll_y = 0;
    }
}

void DrawMainMenu(void) {
    DrawText("Main Menu", screenWidth / 2 - MeasureText("Main Menu", 40) / 2, 150, 40, DARKGRAY);

    // Button for Announcements
    Rectangle annBtnRec = { screenWidth / 2 - 250, 300, 500, 100 };
    bool annHover = CheckCollisionPointRec(g_mouse_pos, annBtnRec);
    DrawRectangleRec(annBtnRec, annHover ? SKYBLUE : LIGHTGRAY);
    DrawText("View Announcements", annBtnRec.x + (annBtnRec.width - MeasureText("View Announcements", 20)) / 2, annBtnRec.y + 40, 20, annHover ? DARKBLUE : DARKGRAY);

    // Button for Events
    Rectangle evtBtnRec = { screenWidth / 2 - 250, 420, 500, 100 };
    bool evtHover = CheckCollisionPointRec(g_mouse_pos, evtBtnRec);
    DrawRectangleRec(evtBtnRec, evtHover ? SKYBLUE : LIGHTGRAY);
    DrawText("View Events", evtBtnRec.x + (evtBtnRec.width - MeasureText("View Events", 20)) / 2, evtBtnRec.y + 40, 20, evtHover ? DARKBLUE : DARKGRAY);
}

// ----------------------------------------------------------------------------------
// List View Screen (Announcements or Events)
// ----------------------------------------------------------------------------------
void UpdateListView(bool isAnnouncements) {
    int itemCount = isAnnouncements ? g_announcement_count : g_event_count;
    float itemHeight = 70; // Increased from 60
    
    // Handle Mouse Wheel Scroll
    g_scroll_y += GetMouseWheelMove() * itemHeight / 2;
    float maxScroll = (itemCount * itemHeight) - (screenHeight - 150);
    if (maxScroll < 0) maxScroll = 0;
    if (g_scroll_y > 0) g_scroll_y = 0;
    if (g_scroll_y < -maxScroll) g_scroll_y = -maxScroll;

    // Check for Back Button click
    Rectangle backBtnRec = { 20, 80, 100, 40 };
    if (CheckCollisionPointRec(g_mouse_pos, backBtnRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentScreen = SCREEN_MAIN_MENU;
    }

    // Check for item clicks
    for (int i = 0; i < itemCount; i++) {
        Rectangle itemRec = { 20, 150 + g_scroll_y + (i * itemHeight), screenWidth - 40, itemHeight - 5 };
        
        // Only check clicks for items visible on screen
        //
        // --- THIS IS THE FIX ---
        // Changed (itemRec.y > 150) to (itemRec.y > 140)
        // The original check was failing for the first item (where y == 150).
        // This now matches the scissor mode (which starts at 140) and the admin panel.
        // ---
        if (itemRec.y > 140 && itemRec.y < screenHeight &&
            CheckCollisionPointRec(g_mouse_pos, itemRec) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
        {
            g_selected_id = isAnnouncements ? g_announcements[i].id : g_events[i].id;
            currentScreen = isAnnouncements ? SCREEN_DETAIL_ANNOUNCEMENT : SCREEN_DETAIL_EVENT;
            g_scroll_y = 0; // NEW: Reset scroll for detail view
            break;
        }
    }
}

void DrawListView(bool isAnnouncements) {
    const char *title = isAnnouncements ? "Announcements" : "Events";
    int itemCount = isAnnouncements ? g_announcement_count : g_event_count;
    
    DrawText(title, 140, 90, 30, DARKGRAY);

    // Draw Back Button
    Rectangle backBtnRec = { 20, 80, 100, 40 };
    bool backHover = CheckCollisionPointRec(g_mouse_pos, backBtnRec);
    DrawRectangleRec(backBtnRec, backHover ? SKYBLUE : LIGHTGRAY);
    DrawText("Back", backBtnRec.x + 30, backBtnRec.y + 10, 20, backHover ? DARKBLUE : DARKGRAY);

    // Draw List Items
    BeginScissorMode(0, 140, screenWidth, screenHeight - 140); // Clip content
    
    if (itemCount == 0) {
        DrawText("No items found.", 20, 150, 20, GRAY);
    }

    for (int i = 0; i < itemCount; i++) {
        Rectangle itemRec = { 20, 150 + g_scroll_y + (i * 70), screenWidth - 40, 65 }; // Increased height
        
        // Culling: Don't draw items off-screen
        if (itemRec.y > screenHeight || itemRec.y + itemRec.height < 140) continue;

        bool itemHover = CheckCollisionPointRec(g_mouse_pos, itemRec);
        DrawRectangleRec(itemRec, itemHover ? WHITE : RAYWHITE);
        DrawRectangleLinesEx(itemRec, 1, itemHover ? SKYBLUE : LIGHTGRAY);

        if (isAnnouncements) {
            DrawText(g_announcements[i].title, itemRec.x + 10, itemRec.y + 12, 20, BLACK);
            DrawText(g_announcements[i].created, itemRec.x + 10, itemRec.y + 35, 18, GRAY); // Moved date under title
        } else {
            DrawText(g_events[i].title, itemRec.x + 10, itemRec.y + 12, 20, BLACK);
            DrawText(g_events[i].date, itemRec.x + 10, itemRec.y + 35, 18, GRAY); // Moved date under title
        }
    }
    
    EndScissorMode();
}

// ----------------------------------------------------------------------------------
// Detail View Screen
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
    // Note: No max scroll limit, can be added if needed but text is finite
    // NEW: Add a max scroll limit (based on text height, though hard to calculate
    // without knowing line count. We'll set a reasonable floor).
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
        for (int i = 0; i < g_announcement_count; i++) {
            if (g_announcements[i].id == g_selected_id) {
                title = g_announcements[i].title;
                subtext = g_announcements[i].created;
                body = g_announcements[i].body;
                break;
            }
        }
    } else {
        for (int i = 0; i < g_event_count; i++) {
            if (g_events[i].id == g_selected_id) {
                title = g_events[i].title;
                subtext = TextFormat("Date: %s", g_events[i].date);
                body = g_events[i].body;
                break;
            }
        }
    }

    // Draw Content
    DrawText(title, 20, 150, 30, BLACK);
    DrawText(subtext, 20, 190, 20, GRAY);
    
    DrawRectangle(20, 220, screenWidth - 40, 2, LIGHTGRAY);

    // Define the area where the body text will be drawn
    Rectangle bodyRec = { 20, 240, screenWidth - 40, screenHeight - 280 };
    
    // --- MODIFICATION ---
    // Use the new helper function to draw the text.
    // This function handles newlines and scrolling.
    DrawTextScrollable(body, bodyRec, g_scroll_y, 20, DARKGRAY);
}