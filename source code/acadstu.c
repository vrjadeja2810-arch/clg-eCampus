// acadstu.c (Student Window)
//
// USER MODS (Nov 10, 2025 - v3):
// - MODIFIED: `Tab_AssignRO` layout.
// - Columns are now spread across the 1920px screen for better readability.
// - "Issued By" and "Description" columns are now wider.
// - All other tabs and functions remain unchanged from v2.

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STR 256
#define MAX_ENTRIES 512

// --- [Data Structures: Unchanged] ---
typedef struct {
    int day; int start_h, start_m; int end_h, end_m;
    char title[MAX_STR]; char room[MAX_STR]; char professor[MAX_STR];
    int is_lab; int id;
} TimetableEntry;

typedef enum { ASSIGNED=0, PENDING=1, SUBMITTED=2 } AssignStatus;

typedef struct {
    int id; char title[MAX_STR]; char due_date[MAX_STR]; char submitted_date[MAX_STR];
    AssignStatus status; char issued_by[MAX_STR]; 
    char desc[MAX_STR];
} Assignment;

typedef struct {
    char semester_name[MAX_STR]; char start_date[MAX_STR];
    char end_date[MAX_STR]; int total_credits;
} SemesterInfo;

typedef struct {
    char subject[MAX_STR]; int total_lectures; int attended;
} AttendanceRecord;

typedef struct {
    char student_id[MAX_STR]; char subject[MAX_STR]; int total_credits;
    int credits_earned; float SPI; char grade[MAX_STR]; char remarks[MAX_STR];
    int passed;
} ResultRecord;

// --- [Storage & Files: Unchanged] ---
static TimetableEntry tt_entries[MAX_ENTRIES];  static int tt_count = 0;
static Assignment assignments[MAX_ENTRIES];     static int assign_count = 0;
static SemesterInfo seminfo;
static AttendanceRecord attendance[MAX_ENTRIES];static int attendance_count = 0;
static ResultRecord results[MAX_ENTRIES];       static int results_count = 0;

static const char* FILE_TT    = "tt_entries.txt";
static const char* FILE_ASSIGN= "assignments.txt";
static const char* FILE_SEM   = "semester.txt";
static const char* FILE_ATT   = "attendance.txt";
static const char* FILE_RES   = "results.txt";

// --- [Util Functions: Unchanged] ---
static void trimnl(char *s) { int n = (int)strlen(s); while (n>0 && (s[n-1]=='\n' || s[n-1]=='\r')) { s[--n] = 0; } }
static void scpy(char *dst, const char *src) { if (!src) { dst[0]=0; return; } strncpy(dst, src, MAX_STR-1); dst[MAX_STR-1]=0; }

// --- [Load Functions: Unchanged (Bug fix is already here)] ---
static void load_timetable(void) {
    tt_count = 0; FILE *f = fopen(FILE_TT,"r"); if (!f) return; char line[1024];
    while (fgets(line,sizeof(line),f)) {
        trimnl(line); if (!line[0]) continue;
        TimetableEntry e; memset(&e,0,sizeof(e));
        char *tok = strtok(line,"|"); if (!tok) continue; e.day = atoi(tok);
        tok = strtok(NULL,"|"); if (!tok) continue; sscanf(tok,"%d:%d",&e.start_h,&e.start_m);
        tok = strtok(NULL,"|"); if (!tok) continue; sscanf(tok,"%d:%d",&e.end_h,&e.end_m);
        tok = strtok(NULL,"|"); if (!tok) continue; e.is_lab = atoi(tok);
        tok = strtok(NULL,"|"); if (tok) scpy(e.title, tok);
        tok = strtok(NULL,"|"); if (tok) scpy(e.room, tok);
        tok = strtok(NULL,"|"); if (tok) scpy(e.professor, tok);
        tok = strtok(NULL,"|"); if (tok) e.id = atoi(tok);
        if (tt_count < MAX_ENTRIES) tt_entries[tt_count++] = e;
    } fclose(f);
}

static void load_assignments(void) {
    assign_count = 0; FILE *f = fopen(FILE_ASSIGN,"r"); if (!f) return; char line[2048];
    while (fgets(line,sizeof(line),f)) {
        trimnl(line); if (!line[0]) continue; Assignment a; memset(&a,0,sizeof(a));
        char *tok = strtok(line,"|"); if (!tok) continue; a.id = atoi(tok);
        tok = strtok(NULL,"|"); if (tok) scpy(a.title, tok);
        tok = strtok(NULL,"|"); if (tok) scpy(a.due_date, tok);
        tok = strtok(NULL,"|"); if (tok) a.status = (AssignStatus)atoi(tok);
        tok = strtok(NULL,"|"); if (tok) scpy(a.submitted_date, tok);
        tok = strtok(NULL,"|"); if (tok) scpy(a.issued_by, tok);
        tok = strtok(NULL,"|"); if (tok) scpy(a.desc, tok);
        if (assign_count < MAX_ENTRIES) assignments[assign_count++] = a;
    } fclose(f);
}
static void load_semester(void) {
    scpy(seminfo.semester_name, "Semester 1");
    scpy(seminfo.start_date, "2025-01-10");
    scpy(seminfo.end_date,   "2025-05-30");
    seminfo.total_credits = 20;
    FILE *f = fopen(FILE_SEM,"r"); if (!f) return; char line[512];
    if (fgets(line,sizeof(line),f)) {
        trimnl(line);
        char *tok = strtok(line,"|");
        if (tok) scpy(seminfo.semester_name, tok);
        tok = strtok(NULL,"|"); if (tok) scpy(seminfo.start_date, tok);
        tok = strtok(NULL,"|"); if (tok) scpy(seminfo.end_date, tok);
        tok = strtok(NULL,"|"); if (tok) seminfo.total_credits = atoi(tok);
    }
    fclose(f);
}
static void load_attendance(void) {
    attendance_count = 0; FILE *f = fopen(FILE_ATT,"r"); if (!f) return; char line[512];
    while (fgets(line,sizeof(line),f)) {
        trimnl(line); if (!line[0]) continue; AttendanceRecord a; memset(&a,0,sizeof(a));
        char *tok = strtok(line,"|"); if (!tok) continue; scpy(a.subject, tok);
        tok = strtok(NULL,"|"); if (tok) a.total_lectures = atoi(tok);
        tok = strtok(NULL,"|"); if (tok) a.attended = atoi(tok);
        if (attendance_count < MAX_ENTRIES) attendance[attendance_count++] = a;
    } fclose(f);
}
static void load_results(void) {
    results_count = 0; FILE *f = fopen(FILE_RES,"r"); if (!f) return; char line[1024];
    while (fgets(line,sizeof(line),f)) {
        trimnl(line); if (!line[0]) continue; ResultRecord r; memset(&r,0,sizeof(r));
        char *tok = strtok(line,"|"); if (!tok) continue; scpy(r.student_id, tok);
        tok = strtok(NULL,"|"); if (tok) scpy(r.subject, tok);
        tok = strtok(NULL,"|"); if (tok) r.total_credits = atoi(tok);
        tok = strtok(NULL,"|"); if (tok) r.credits_earned = atoi(tok);
        tok = strtok(NULL,"|"); if (tok) r.SPI = (float)atof(tok);
        tok = strtok(NULL,"|"); if (tok) scpy(r.grade, tok);
        tok = strtok(NULL,"|"); if (tok) scpy(r.remarks, tok);
        tok = strtok(NULL,"|"); if (tok) r.passed = atoi(tok);
        if (results_count < MAX_ENTRIES) results[results_count++] = r;
    } fclose(f);
}

static const char* DAYS[7] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};

// ---------------- Timetable Grids (Unchanged from v2) ----------------
static void DrawLectureGridRO(int x, int y, int w, int h) {
    int timeColW = 120, daysCols = 5, rows = 4;
    int contentW = w - timeColW, cellW = contentW / daysCols, cellH = h / rows;
    DrawRectangle(x, y, w, h, RAYWHITE);
    DrawRectangle(x, y, timeColW, h, Fade(LIGHTGRAY, 0.3f));
    DrawRectangleLines(x, y, timeColW, h, LIGHTGRAY);
    const char* timeLabels[4] = {"8:00-9:00","9:00-10:00","10:00-11:00","11:00-12:00"};
    int r; for (r=0;r<rows;r++) {
        DrawRectangleLines(x, y + r*cellH, timeColW, cellH, LIGHTGRAY);
        int tw = MeasureText(timeLabels[r], 18);
        DrawText(timeLabels[r], x + (timeColW/2 - tw/2), y + r*cellH + cellH/2 - 9, 18, BLACK);
    }
    int d; for (d=0; d<daysCols; d++) {
        int colX = x + timeColW + d*cellW;
        DrawText(DAYS[d], colX + 12, y - 30, 20, BLACK);
        DrawRectangleLines(colX, y, cellW, h, LIGHTGRAY);
    }
    for (r=0;r<=rows;r++) DrawLine(x + timeColW, y + r*cellH, x + timeColW + contentW, y + r*cellH, LIGHTGRAY);

    const float VSTART = 8.0f, VEND = 12.0f, VRANGE = VEND - VSTART;
    int i; for (i=0;i<tt_count;i++) {
        TimetableEntry *e = &tt_entries[i];
        if (e->day < 0 || e->day >= 5) continue; if (e->is_lab) continue;
        float st = e->start_h + e->start_m/60.0f, en = e->end_h + e->end_m/60.0f;
        if (en <= VSTART || st >= VEND) continue; if (st < VSTART) st = VSTART; if (en > VEND) en = VEND;
        float ry = y + ((st - VSTART) / VRANGE) * h, rh = ((en - st) / VRANGE) * h; if (rh < 6.0f) rh = 6.0f;
        int rx = x + timeColW + e->day * cellW + 4, rw = cellW - 8;
        DrawRectangle(rx, (int)ry + 4, rw, (int)rh - 6, Fade(SKYBLUE,0.65f));
        DrawRectangleLines(rx, (int)ry + 4, rw, (int)rh - 6, DARKGRAY);
        DrawText(e->title, rx + 6, (int)ry + 8, 18, BLACK);
        char info[160]; snprintf(info, sizeof(info), "%02d:%02d-%02d:%02d  %s", e->start_h, e->start_m, e->end_h, e->end_m, e->room);
        DrawText(info, rx + 6, (int)ry + 30, 16, DARKGRAY);
    }
}
static void DrawLabGridRO(int x, int y, int w, int h) {
    int timeColW = 120, daysCols = 5, rows = 4;
    int contentW = w - timeColW, cellW = contentW / daysCols, cellH = h / rows;
    DrawRectangle(x, y, w, h, RAYWHITE);
    DrawRectangle(x, y, timeColW, h, Fade(LIGHTGRAY, 0.3f));
    DrawRectangleLines(x, y, timeColW, h, LIGHTGRAY);
    const char* timeLabels[4] = {"2:00-3:00","3:00-4:00","4:00-5:00","5:00-6:00"};
    int r; for (r=0;r<rows;r++) {
        DrawRectangleLines(x, y + r*cellH, timeColW, cellH, LIGHTGRAY);
        int tw = MeasureText(timeLabels[r], 18);
        DrawText(timeLabels[r], x + (timeColW/2 - tw/2), y + r*cellH + cellH/2 - 9, 18, BLACK);
    }
    int d; for (d=0; d<daysCols; d++) {
        int colX = x + timeColW + d*cellW;
        DrawText(DAYS[d], colX + 12, y - 30, 20, BLACK);
        DrawRectangleLines(colX, y, cellW, h, LIGHTGRAY);
    }
    for (r=0;r<=rows;r++) DrawLine(x + timeColW, y + r*cellH, x + timeColW + contentW, y + r*cellH, LIGHTGRAY);

    const float VSTART = 14.0f, VEND = 18.0f, VRANGE = VEND - VSTART;
    int i; for (i=0;i<tt_count;i++) {
        TimetableEntry *e = &tt_entries[i];
        if (e->day < 0 || e->day >= 5) continue; if (!e->is_lab) continue;
        float st = e->start_h + e->start_m/60.0f, en = e->end_h + e->end_m/60.0f;
        if (en <= VSTART || st >= VEND) continue; if (st < VSTART) st = VSTART; if (en > VEND) en = VEND;
        float ry = y + ((st - VSTART) / VRANGE) * h, rh = ((en - st) / VRANGE) * h; if (rh < 6.0f) rh = 6.0f;
        int rx = x + timeColW + e->day * cellW + 4, rw = cellW - 8;
        DrawRectangle(rx, (int)ry + 4, rw, (int)rh - 6, Fade(GREEN,0.75f));
        DrawRectangleLines(rx, (int)ry + 4, rw, (int)rh - 6, DARKGRAY);
        DrawText(e->title, rx + 6, (int)ry + 8, 18, BLACK);
        char info[160]; snprintf(info, sizeof(info), "%02d:%02d-%02d:%02d  %s", e->start_h, e->start_m, e->end_h, e->end_m, e->room);
        DrawText(info, rx + 6, (int)ry + 30, 16, DARKGRAY);
    }
}

enum { TAB_TIMETABLE=0, TAB_LABS=1, TAB_ASSIGN=2, TAB_SEM=3, TAB_ATT=4, TAB_RES=5 };
static int active_tab = 0;

// ---------------- Header (Unchanged from v2) ----------------
static void DrawHeaderRO(int screenWidth) {
    DrawRectangle(0,0,screenWidth, 64, BLUE);
    DrawText("Academics - Student Page", 20, 18, 30, WHITE);
    
    const char* tabs[] = {"Lecture TT", "Lab TT", "Assignments", "Semester Info", "Attendance", "Results"};
    int numTabs = 6;
    int tabWidth = 240;
    int tabSpacing = 10;
    int startX = 20;

    for (int i=0;i<numTabs;i++) {
        Rectangle r = {startX + i*(tabWidth + tabSpacing), 70, tabWidth, 40};
        DrawRectangleRec(r, i==active_tab ? Fade(SKYBLUE,0.95f) : LIGHTGRAY);
        DrawRectangleLines((int)r.x,(int)r.y,(int)r.width,(int)r.height, GRAY);
        int tw = MeasureText(tabs[i], 20);
        DrawText(tabs[i], (int)(r.x + r.width/2 - tw/2), (int)r.y + 10, 20, BLACK);
        Vector2 mp = GetMousePosition();
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mp, r)) active_tab = i;
    }
}

// ---------------- Tab Functions (MODIFIED) ----------------
static void Tab_TimetableRO(int x, int y, int w) { 
    DrawText("Lecture Timetable", x, y, 30, DARKBLUE);
    DrawLectureGridRO(x, y+50, w, 700);
}
static void Tab_LabsRO(int x, int y, int w) {
    DrawText("Lab Timetable", x, y, 30, DARKBLUE);
    DrawLabGridRO(x, y+50, w, 700);
}

// MODIFIED: All fonts, layout, and new columns
static void Tab_AssignRO(int x, int y, int w) {
    DrawText("Assignments", x, y, 30, DARKBLUE);
    
    int listX=x, listY=y+50, listW=w, listH = 700;
    DrawRectangle(listX, listY, listW, listH, RAYWHITE);
    DrawRectangleLines(listX, listY, listW, listH, LIGHTGRAY);
    
    // MODIFIED: New column coordinates for better spacing
    int col1 = listX + 20;  // Title
    int col2 = listX + 440; // Due
    int col3 = listX + 610; // Status
    int col4 = listX + 780; // Issued By
    int col5 = listX + 1100; // Description
    
    DrawText("Title",       col1, listY+10, 20, DARKGRAY);
    DrawText("Due",         col2, listY+10, 20, DARKGRAY);
    DrawText("Status",      col3, listY+10, 20, DARKGRAY);
    DrawText("Issued By",   col4, listY+10, 20, DARKGRAY);
    DrawText("Description", col5, listY+10, 20, DARKGRAY);
    DrawLine(listX, listY+40, listX+listW, listY+40, LIGHTGRAY);

    int ly=listY+50, i;
    for (i=0;i<assign_count;i++) {
        Assignment *a=&assignments[i];
        DrawText(a->title,      col1, ly, 20, BLACK);
        DrawText(a->due_date,   col2, ly, 20, BLACK);
        const char* st="Assigned"; if (a->status==PENDING) st="Pending"; if (a->status==SUBMITTED) st="Submitted";
        DrawText(st,            col3, ly, 20, BLACK);
        DrawText(a->issued_by,  col4, ly, 20, DARKGRAY); // This is the prof name
        DrawText(a->desc,       col5, ly, 20, DARKGRAY);
        ly += 44; if (ly > listY + listH - 30) break;
    }
}

// MODIFIED: All fonts and layout
static void Tab_SemRO(int x, int y, int w) {
    DrawText("Semester Information", x, y, 30, DARKBLUE);

    int mainY = y + 50;
    DrawText("Semester:", x, mainY, 20, BLACK); DrawText(seminfo.semester_name, x+120, mainY, 20, DARKGRAY); mainY += 30;
    DrawText("Start:",    x, mainY, 20, BLACK); DrawText(seminfo.start_date, x+120, mainY, 20, DARKGRAY); mainY += 30;
    DrawText("End:",      x, mainY, 20, BLACK); DrawText(seminfo.end_date, x+120, mainY, 20, DARKGRAY); mainY += 30;
    char cred[64]; snprintf(cred, sizeof(cred), "Total Credits: %d", seminfo.total_credits);
    DrawText(cred, x, mainY, 20, BLACK);

    int col1x = x, topY = y + 200;
    DrawText("Official Holiday List 2025", col1x, topY, 24, DARKBLUE);
    
    int hy = topY + 40;
    DrawText("26/01/2025 (Sunday)  -  Republic Day", x, hy, 20, BLACK); hy += 30;
    DrawText("26/02/2025 (Wednesday)  -  Maha Shivaratri", x, hy, 20, BLACK); hy += 30;
    DrawText("14/03/2025 (Friday)  -  Holi / Dhulivandan", x, hy, 20, BLACK); hy += 30;
    DrawText("15/08/2025 (Friday)  -  Independence Day", x, hy, 20, BLACK); hy += 30;
    DrawText("02/10/2025 (Thursday)  -  Mahatma Gandhi’s Birthday", x, hy, 20, BLACK); hy += 30;
    DrawText("20/10/2025 (Monday)  -  Diwali (Deepavali)", x, hy, 20, BLACK); hy += 30;
    DrawText("25/12/2025 (Thursday)  -  Christmas Day", x, hy, 20, BLACK); hy += 30;

    int examsX = x + 700;
    DrawText("Exams (Official):", examsX, topY, 24, DARKBLUE);
    DrawText("MIDSEM  :  03-10-2025  --->  08-10-2025", examsX, topY + 40, 20, BLACK);
    DrawText("ENDSEM  :  03-12-2025  --->  08-12-2025", examsX, topY + 70, 20, BLACK);
}

// MODIFIED: All fonts and layout
static void Tab_AttRO(int x, int y, int w) {
    DrawText("Student Attendance", x, y, 30, DARKBLUE);

    int listX=x, listY=y+50, listW=w, listH = 700;
    DrawRectangle(listX, listY, listW, listH, RAYWHITE);
    DrawRectangleLines(listX, listY, listW, listH, LIGHTGRAY);
    
    DrawText("Subject", x+10, y+60, 20, DARKGRAY);
    DrawText("Total", x+400, y+60, 20, DARKGRAY);
    DrawText("Attended", x+500, y+60, 20, DARKGRAY);
    DrawText("Missed", x+620, y+60, 20, DARKGRAY);
    DrawText("Percent", x+720, y+60, 20, DARKGRAY);
    DrawLine(listX, listY+40, listX+listW, listY+40, LIGHTGRAY);

    int ly = y+100, i;
    for (i=0;i<attendance_count;i++) {
        AttendanceRecord *a = &attendance[i];
        DrawText(a->subject, x+10, ly, 20, BLACK);
        char buf[64];
        snprintf(buf, sizeof(buf), "%d", a->total_lectures); DrawText(buf, x+400, ly, 20, BLACK);
        snprintf(buf, sizeof(buf), "%d", a->attended);       DrawText(buf, x+500, ly, 20, BLACK);
        snprintf(buf, sizeof(buf), "%d", a->total_lectures - a->attended); DrawText(buf, x+620, ly, 20, BLACK);
        float pct = 0.0f; if (a->total_lectures) pct = 100.0f * a->attended / a->total_lectures;
        char pctbuf[32]; snprintf(pctbuf, sizeof(pctbuf), "%.1f%%", pct); DrawText(pctbuf, x+720, ly, 20, BLACK);
        ly += 44; if (ly > listY + listH - 30) break;
    }
}

// MODIFIED: All fonts, layout, and NO SPI
static void Tab_ResRO(int x, int y, int w) {
    DrawText("Results / Grades", x, y, 30, DARKBLUE);

    int listX=x, listY=y+50, listW=w, listH = 700;
    DrawRectangle(listX, listY, listW, listH, RAYWHITE);
    DrawRectangleLines(listX, listY, listW, listH, LIGHTGRAY);
    
    DrawText("Student ID", x+10, y+60, 20, DARKGRAY);
    DrawText("Subject", x+180, y+60, 20, DARKGRAY);
    DrawText("Credits", x+450, y+60, 20, DARKGRAY);
    DrawText("Earned",  x+550, y+60, 20, DARKGRAY);
    DrawText("Grade",   x+650, y+60, 20, DARKGRAY);
    DrawText("Remarks", x+750, y+60, 20, DARKGRAY);
    DrawLine(listX, listY+40, listX+listW, listY+40, LIGHTGRAY);

    int ly = y+100, i;
    for (i=0;i<results_count;i++) {
        ResultRecord *r = &results[i];
        DrawText(r->student_id, x+10, ly, 20, BLACK);
        DrawText(r->subject, x+180, ly, 20, BLACK);
        char tmp[64];
        snprintf(tmp, sizeof(tmp), "%d", r->total_credits);  DrawText(tmp, x+450, ly, 20, BLACK);
        snprintf(tmp, sizeof(tmp), "%d", r->credits_earned); DrawText(tmp, x+550, ly, 20, BLACK);
        DrawText(r->grade,   x+650, ly, 20, BLACK);
        DrawText(r->remarks, x+750, ly, 20, DARKGRAY);
        ly += 44; if (ly > listY + listH - 30) break;
    }
}

// ---------------- MAIN (Unchanged from v2) ----------------
int main(int argc, char *argv[]) {
    if(argc<3) return 0;
    const int screenWidth = 1920;
    const int screenHeight = 1080;
    InitWindow(screenWidth, screenHeight, "University Portal - Academics (Student Window)");
    SetTargetFPS(60);

    load_timetable(); load_assignments(); load_semester(); load_attendance(); load_results();

    while (!WindowShouldClose()) {
        BeginDrawing(); ClearBackground(RAYWHITE); 
        DrawHeaderRO(screenWidth);
        
        int contentX = 50;
        int contentY = 130;
        int mainContentWidth = screenWidth - 100;
        
        {
          if (active_tab == TAB_TIMETABLE)      Tab_TimetableRO(contentX, contentY, mainContentWidth);
          else if (active_tab == TAB_LABS)      Tab_LabsRO(contentX, contentY, mainContentWidth);
          else if (active_tab == TAB_ASSIGN)    Tab_AssignRO(contentX, contentY, mainContentWidth);
          else if (active_tab == TAB_SEM)       Tab_SemRO(contentX, contentY, mainContentWidth);
          else if (active_tab == TAB_ATT)       Tab_AttRO(contentX, contentY, mainContentWidth);
          else if (active_tab == TAB_RES)       Tab_ResRO(contentX, contentY, mainContentWidth);
          else                                  Tab_TimetableRO(contentX, contentY, mainContentWidth);
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}