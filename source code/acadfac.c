// acadfac.c (Faculty Window)
//
// USER MODS (Nov 10, 2025 - v3):
// - FIXED: Form labels and input boxes were overlapping.
//   - All sidebar forms (in all 6 tabs) are now drawn sequentially to prevent clashes.
// - FIXED: Font size inside timetable event boxes was too small.
//   - Increased font size in DrawTimetableGridFixed() to 20px/18px.
// - Kept all other layout and font changes from v2.

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STR 256
#define MAX_ENTRIES 512

// --- [Data Structures: Unchanged] ---
typedef enum { TT_LECTURE = 0, TT_LAB = 1 } TTType;

typedef struct {
    int day; int start_h, start_m; int end_h, end_m;
    char title[MAX_STR]; char room[MAX_STR]; char professor[MAX_STR];
    TTType type; int id;
} TimetableEntry;

typedef enum { ASSIGNED=0, PENDING=1, SUBMITTED=2 } AssignStatus;
typedef struct {
    int id; char title[MAX_STR]; char due_date[MAX_STR]; char submitted_date[MAX_STR];
    AssignStatus status; char issued_by[MAX_STR]; char desc[MAX_STR];
} Assignment;

typedef struct {
    int id; char title[MAX_STR]; char date[MAX_STR]; char desc[MAX_STR];
} Event;

typedef struct {
    char semester_name[MAX_STR]; char start_date[MAX_STR]; char end_date[MAX_STR];
    int total_credits;
    Event holidays[MAX_ENTRIES]; int holidays_count;
    Event exams[MAX_ENTRIES]; int exams_count;
    Event tests[MAX_ENTRIES]; int tests_count;
    Event orientations[MAX_ENTRIES]; int orientations_count;
    Event deadlines[MAX_ENTRIES]; int deadlines_count;
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
static TimetableEntry tt_entries[MAX_ENTRIES]; static int tt_count = 0;
static Assignment assignments[MAX_ENTRIES]; static int assign_count = 0;
static Event events_list[MAX_ENTRIES]; static int events_count = 0;
static SemesterInfo seminfo;
static AttendanceRecord attendance[MAX_ENTRIES]; static int attendance_count = 0;
static ResultRecord results[MAX_ENTRIES]; static int results_count = 0;

const char* FILE_TT = "tt_entries.txt";
const char* FILE_ASSIGN = "assignments.txt";
const char* FILE_EVENTS = "events.txt";
const char* FILE_SEM = "semester.txt";
const char* FILE_ATT = "attendance.txt";
const char* FILE_RES = "results.txt";

// --- [Util Functions: Unchanged] ---
static void trimnl(char *s) {
    int n = (int)strlen(s);
    while (n>0 && (s[n-1]=='\n' || s[n-1]=='\r')) { s[--n] = 0; }
}
static void scpy(char *dst, const char *src) {
    if (!src) { dst[0]=0; return; }
    strncpy(dst, src, MAX_STR-1);
    dst[MAX_STR-1]=0;
}
static int times_overlap_minutes(int a_start_min, int a_end_min, int b_start_min, int b_end_min) {
    return !(a_end_min <= b_start_min || b_end_min <= a_start_min);
}

// --- [Save/Load Functions: Unchanged] ---
void save_timetable() {
    FILE *f = fopen(FILE_TT,"w"); if (!f) return;
    for (int i=0;i<tt_count;i++) {
        TimetableEntry *e = &tt_entries[i];
        fprintf(f, "%d|%02d:%02d|%02d:%02d|%d|%s|%s|%s|%d\n",
            e->day, e->start_h, e->start_m, e->end_h, e->end_m,
            (int)e->type, e->title, e->room, e->professor, e->id);
    } fclose(f);
}
void load_timetable() {
    tt_count = 0; FILE *f = fopen(FILE_TT,"r"); if (!f) return; char line[1024];
    while (fgets(line,sizeof(line),f)) {
        trimnl(line); if (strlen(line)==0) continue;
        TimetableEntry e; memset(&e,0,sizeof(e));
        char *tok = strtok(line,"|"); if (!tok) continue; e.day = atoi(tok);
        tok = strtok(NULL,"|"); if (!tok) continue; sscanf(tok,"%d:%d",&e.start_h,&e.start_m);
        tok = strtok(NULL,"|"); if (!tok) continue; sscanf(tok,"%d:%d",&e.end_h,&e.end_m);
        tok = strtok(NULL,"|"); if (!tok) continue; e.type = (TTType)atoi(tok);
        tok = strtok(NULL,"|"); if (tok) scpy(e.title, tok);
        tok = strtok(NULL,"|"); if (tok) scpy(e.room, tok);
        tok = strtok(NULL,"|"); if (tok) scpy(e.professor, tok);
        tok = strtok(NULL,"|"); if (tok) e.id = atoi(tok);
        if (tt_count < MAX_ENTRIES) tt_entries[tt_count++] = e;
    } fclose(f);
}
void save_assignments() {
    FILE *f = fopen(FILE_ASSIGN,"w"); if (!f) return;
    for (int i=0;i<assign_count;i++) {
        Assignment *a = &assignments[i];
        fprintf(f, "%d|%s|%s|%d|%s|%s|%s\n",
            a->id, a->title, a->due_date, (int)a->status, a->submitted_date, a->issued_by, a->desc);
    } fclose(f);
}
void load_assignments() {
    assign_count = 0; FILE *f = fopen(FILE_ASSIGN,"r"); if (!f) return; char line[2048];
    while (fgets(line,sizeof(line),f)) {
        trimnl(line); if (strlen(line)==0) continue;
        Assignment a; memset(&a,0,sizeof(a));
        char *tok = strtok(line,"|"); if (!tok) continue; a.id = atoi(tok);
        tok = strtok(NULL,"|"); if (tok) scpy(a.title, tok);
        tok = strtok(NULL,"|"); if (tok) scpy(a.due_date, tok);
        tok = strtok(NULL,"|"); if (tok) a.status = atoi(tok);
        tok = strtok(NULL,"|"); if (tok) scpy(a.submitted_date, tok);
        tok = strtok(NULL,"|"); if (tok) scpy(a.issued_by, tok);
        tok = strtok(NULL,"|"); if (tok) scpy(a.desc, tok);
        if (assign_count < MAX_ENTRIES) assignments[assign_count++] = a;
    } fclose(f);
}
void save_events() { /* ...unchanged... */ }
void load_events() { /* ...unchanged... */ }
void save_semester() { /* ...unchanged... */ }
void load_semester() { /* ...unchanged... */ }
void save_attendance() { /* ...unchanged... */ }
void load_attendance() { /* ...unchanged... */ }

void save_results() {
    FILE *f = fopen(FILE_RES,"w"); if (!f) return;
    for (int i=0;i<results_count;i++) {
        ResultRecord *r = &results[i];
        fprintf(f, "%s|%s|%d|%d|%.2f|%s|%s|%d\n", r->student_id, r->subject, r->total_credits, r->credits_earned, r->SPI, r->grade, r->remarks, r->passed);
    } fclose(f);
}
void load_results() {
    results_count = 0; FILE *f = fopen(FILE_RES,"r"); if (!f) return; char line[1024];
    while (fgets(line,sizeof(line),f)) {
        trimnl(line); if (strlen(line)==0) continue;
        ResultRecord r; memset(&r,0,sizeof(r));
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

// ---------------- UI HELPERS (Unchanged from v2) ----------------
typedef struct { Rectangle rect; char text[MAX_STR]; int active; float caretTimer; float backspaceTimer; } InputBox;

InputBox MakeInput(int x, int y, int w, int h, const char* init) {
    InputBox ib; ib.rect.x = (float)x; ib.rect.y = (float)y; ib.rect.width = (float)w; ib.rect.height = (float)h;
    ib.active = 0; ib.caretTimer = 0.0f; ib.backspaceTimer = 0.0f; memset(ib.text,0,MAX_STR);
    if (init) strncpy(ib.text, init, MAX_STR-1);
    return ib;
}

void DrawInput(InputBox *ib, const char* placeholder) {
    DrawRectangleRec(ib->rect, LIGHTGRAY);
    DrawRectangleLines((int)ib->rect.x,(int)ib->rect.y,(int)ib->rect.width,(int)ib->rect.height,GRAY);
    const char* d = ib->text[0] ? ib->text : placeholder;
    Color col = ib->text[0] ? BLACK : DARKGRAY;
    
    DrawText(d, (int)ib->rect.x + 10, (int)ib->rect.y + (int)(ib->rect.height / 2) - 10, 20, col);

    if (ib->active) {
        ib->caretTimer += GetFrameTime();
        if (ib->caretTimer >= 1.0f) ib->caretTimer = 0.0f;
        if (ib->caretTimer < 0.5f) {
            int tw = MeasureText(ib->text, 20);
            int cx = (int)(ib->rect.x + 10 + tw);
            int cy = (int)ib->rect.y + 6;
            DrawRectangle(cx, cy, 2, (int)ib->rect.height - 12, BLACK);
        }
    } else {
        ib->caretTimer = 0.0f;
    }
}

void HandleInputs(InputBox *ibs, int count) {
    Vector2 mp = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        for (int i=0;i<count;i++) ibs[i].active = CheckCollisionPointRec(mp, ibs[i].rect);
    }

    int key = GetCharPressed();
    while (key > 0) {
        for (int i=0;i<count;i++) {
            if (!ibs[i].active) continue;
            int len = (int)strlen(ibs[i].text);
            if (key >= 32 && key <= 125 && len < MAX_STR-2) {
                if (MeasureText(ibs[i].text, 20) < ibs[i].rect.width - 20) {
                    ibs[i].text[len] = (char)key; ibs[i].text[len+1]=0;
                }
            }
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_DELETE)) {
        for (int i=0;i<count;i++) {
            if (!ibs[i].active) continue;
            int len = (int)strlen(ibs[i].text);
            if (len > 0) ibs[i].text[len-1] = 0;
            ibs[i].backspaceTimer = 0.5f;
        }
    }

    if (IsKeyDown(KEY_BACKSPACE) || IsKeyDown(KEY_DELETE)) {
        float ft = GetFrameTime();
        for (int i=0;i<count;i++) {
            if (!ibs[i].active) continue;
            ibs[i].backspaceTimer -= ft;
            if (ibs[i].backspaceTimer <= 0.0f) {
                int len = (int)strlen(ibs[i].text);
                if (len > 0) ibs[i].text[len-1] = 0;
                ibs[i].backspaceTimer = 0.06f;
            }
        }
    } else {
        for (int i=0;i<count;i++) {
            if (!ibs[i].active) ibs[i].backspaceTimer = 0.0f;
        }
    }
}

int ButtonRect(Rectangle r, const char* text) {
    DrawRectangleRec(r, Fade(SKYBLUE, 0.95f));
    DrawRectangleLines((int)r.x,(int)r.y,(int)r.width,(int)r.height,BLUE);
    int tw = MeasureText(text, 20);
    DrawText(text, (int)(r.x + r.width/2 - tw/2), (int)(r.y + r.height/2 - 10), 20, BLACK);
    Vector2 mp = GetMousePosition();
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mp, r)) return 1;
    return 0;
}

const char* DAYS[7] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};

// ---------------- Timetable grid (MODIFIED) ----------------
// MODIFIED: Increased font size *inside* the event boxes
void DrawTimetableGridFixed(int x, int y, int w, int h, int showLabs) {
    int timeColW = 120;
    int daysCols = 5;
    int contentW = w - timeColW;
    int cellW = contentW / daysCols;
    int rows = 4;
    int cellH = h / rows;

    DrawRectangle(x, y, w, h, RAYWHITE);
    DrawRectangle(x, y, timeColW, h, Fade(LIGHTGRAY, 0.3f));
    DrawRectangleLines(x, y, timeColW, h, LIGHTGRAY);

    const float visibleStart = showLabs ? 14.0f : 8.0f;
    const float visibleEnd   = showLabs ? 18.0f : 12.0f;
    const float visibleRange = visibleEnd - visibleStart;

    for (int r=0; r<rows; r++) {
        int hour = (int)(visibleStart + r);
        char buf[32]; sprintf(buf, "%02d:00-%02d:00", hour, hour+1);
        int tw = MeasureText(buf, 18);
        DrawRectangleLines(x, y + r*cellH, timeColW, cellH, LIGHTGRAY);
        DrawText(buf, x + (timeColW/2 - tw/2), y + r*cellH + cellH/2 - 9, 18, BLACK);
    }

    for (int d=0; d<daysCols; d++) {
        int colX = x + timeColW + d*cellW;
        DrawText(DAYS[d], colX + 12, y - 30, 20, BLACK);
        DrawRectangleLines(colX, y, cellW, h, LIGHTGRAY);
    }
    for (int r=0;r<=rows;r++) {
        DrawLine(x + timeColW, y + r*cellH, x + timeColW + contentW, y + r*cellH, LIGHTGRAY);
    }

    for (int i=0;i<tt_count;i++) {
        TimetableEntry *e = &tt_entries[i];
        if (e->day < 0 || e->day >= 5) continue;
        if (showLabs) { if (e->type != TT_LAB) continue; } 
        else { if (e->type != TT_LECTURE) continue; }

        float startTotal = e->start_h + e->start_m/60.0f;
        float endTotal   = e->end_h   + e->end_m/60.0f;
        if (endTotal <= visibleStart || startTotal >= visibleEnd) continue;
        if (startTotal < visibleStart) startTotal = visibleStart;
        if (endTotal > visibleEnd) endTotal = visibleEnd;

        float startOffset = startTotal - visibleStart;
        float endOffset = endTotal - visibleStart;
        float ry = y + (startOffset / visibleRange) * h;
        float rh = (endOffset - startOffset) / visibleRange * h;
        if (rh < 6.0f) rh = 6.0f;

        int rx = x + timeColW + e->day * cellW + 4;
        int rw = cellW - 8;

        Color bg = (e->type == TT_LAB) ? Fade(GREEN,0.8f) : Fade(SKYBLUE,0.6f);
        DrawRectangle(rx, (int)ry + 4, rw, (int)rh - 6, bg);
        DrawRectangleLines(rx, (int)ry + 4, rw, (int)rh - 6, DARKGRAY);

        // MODIFIED: Fonts and y-offsets
        DrawText(e->title, rx + 6, (int)ry + 8, 20, BLACK); 
        char info[128]; sprintf(info, "%02d:%02d-%02d:%02d @%s", e->start_h, e->start_m, e->end_h, e->end_m, e->room);
        DrawText(info, rx + 6, (int)ry + 34, 18, DARKGRAY);
    }
}

// ---------------- UI STATE ----------------
enum { TAB_TIMETABLE=0, TAB_LABS=1, TAB_ASSIGN=2, TAB_SEM=3, TAB_ATT=4, TAB_RES=5 };
int active_tab = 0;

// Inputs
InputBox ib_tt_title, ib_tt_room, ib_tt_prof, ib_tt_start, ib_tt_end; int ib_tt_day = 0;
InputBox ib_lab_title, ib_lab_room, ib_lab_prof, ib_lab_start, ib_lab_end; int ib_lab_day = 0;
InputBox ib_assign_title, ib_assign_due, ib_assign_desc, ib_assign_issuedby;
InputBox ib_event_title, ib_event_date, ib_event_desc;
InputBox ib_sem_name, ib_sem_start, ib_sem_end, ib_sem_credit;
InputBox ib_att_subject, ib_att_total, ib_att_attended;
InputBox ib_res_subject, ib_res_credits, ib_res_earned, ib_res_spi, ib_res_grade, ib_res_remarks, ib_res_id;

// ---------------- UI Draw Functions (MODIFIED) ----------------
void DrawHeader(int screenWidth) {
    DrawRectangle(0,0, screenWidth, 64, BLUE);
    DrawText("Academics - Faculty Portal", 20, 18, 30, WHITE);
    
    const char* tabs[] = {"Lecture TT", "Lab TT", "Assignments", "Semester Info", "Attendance", "Results"};
    int numTabs = 6;
    int tabWidth = 240;
    int tabSpacing = 10;
    int startX = 20;

    for (int i=0;i<numTabs;i++) {
        Rectangle r = {startX + i*(tabWidth + tabSpacing), 70, tabWidth, 40};
        DrawRectangleRec(r, i==active_tab ? Fade(SKYBLUE,0.9f) : LIGHTGRAY);
        DrawRectangleLines((int)r.x,(int)r.y,(int)r.width,(int)r.height, GRAY);
        int tw = MeasureText(tabs[i], 20);
        DrawText(tabs[i], (int)(r.x + r.width/2 - tw/2), (int)r.y + 10, 20, BLACK);
        Vector2 mp = GetMousePosition();
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mp, r)) active_tab = i;
    }
}

// ---------------- Lecture Tab (MODIFIED) ----------------
// MODIFIED: Reworked sidebar layout to be sequential
void Tab_Timetable(int x, int y, int w, int sidebarX) {
    DrawText("Lecture Timetable", x, y, 30, DARKBLUE);
    DrawTimetableGridFixed(x, y+50, w, 700, 0); // MODIFIED: Height

    // --- Sidebar Form (Sequentially Drawn) ---
    DrawText("Add Lecture", sidebarX, y, 26, DARKBLUE);
    
    int formY = y + 40; // Current Y-position for drawing
    int inputW = 440; // Full width of sidebar area
    int inputW_half = 210;
    int inputH = 40;
    int labelOffset = 28; // Space between label and box
    int fieldSpacing = 10; // Space between fields

    DrawText("Title:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_tt_title.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, (float)inputH};
    DrawInput(&ib_tt_title, "Subject title");
    formY += inputH + fieldSpacing;

    DrawText("Room:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_tt_room.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, (float)inputH};
    DrawInput(&ib_tt_room, "Room");
    formY += inputH + fieldSpacing;

    DrawText("Professor:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_tt_prof.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, (float)inputH};
    DrawInput(&ib_tt_prof, "Professor's name");
    formY += inputH + fieldSpacing;

    DrawText("Start (HH:MM):", sidebarX, formY, 20, BLACK);
    DrawText("End (HH:MM):", sidebarX + inputW_half + 20, formY, 20, BLACK);
    formY += labelOffset;
    ib_tt_start.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW_half, (float)inputH};
    DrawInput(&ib_tt_start, "08:00");
    ib_tt_end.rect = (Rectangle){(float)sidebarX + inputW_half + 20, (float)formY, (float)inputW_half, (float)inputH};
    DrawInput(&ib_tt_end, "09:00");
    formY += inputH + fieldSpacing;

    DrawText("Day:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    for (int d=0; d<5; d++) {
        Rectangle r = {sidebarX + d*52, formY, 48, 34};
        DrawRectangleRec(r, d==ib_tt_day ? Fade(BLUE,0.85f) : LIGHTGRAY);
        DrawRectangleLines((int)r.x,(int)r.y,(int)r.width,(int)r.height,GRAY);
        DrawText(DAYS[d], r.x+10, r.y+8, 16, BLACK);
        Vector2 mp = GetMousePosition();
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mp, r)) ib_tt_day = d;
    }
    formY += 34 + fieldSpacing;

    if (ButtonRect((Rectangle){(float)sidebarX, (float)formY, 220, 40}, "Add Lecture")) {
        // ... [Add logic is unchanged] ...
        if (tt_count < MAX_ENTRIES) {
            TimetableEntry e; memset(&e,0,sizeof(e));
            e.day = ib_tt_day;
            e.start_h = 8; e.start_m = 0; e.end_h = 9; e.end_m = 0;
            if (sscanf(ib_tt_start.text, "%d:%d", &e.start_h, &e.start_m) < 1) { e.start_h = 8; e.start_m = 0; }
            if (sscanf(ib_tt_end.text, "%d:%d", &e.end_h, &e.end_m) < 1) { e.end_h = e.start_h + 1; e.end_m = 0; }
            if ((e.end_h*60 + e.end_m) <= (e.start_h*60 + e.start_m)) { e.end_h = e.start_h + 1; e.end_m = 0; }
            if (e.start_h < 8) { e.start_h = 8; e.start_m = 0; }
            if (e.end_h > 12) { e.end_h = 12; e.end_m = 0; }
            if (e.start_h > 12) { e.start_h = 12; e.start_m = 0; }
            if (e.end_h < 8) { e.end_h = 8; e.end_m = 0; }
            scpy(e.title, ib_tt_title.text[0] ? ib_tt_title.text : "Untitled Lecture");
            scpy(e.room, ib_tt_room.text[0] ? ib_tt_room.text : "Room");
            scpy(e.professor, ib_tt_prof.text[0] ? ib_tt_prof.text : "Prof");
            e.type = TT_LECTURE;
            e.id = (tt_count>0 ? tt_entries[tt_count-1].id + 1 : 1);
            int new_start = e.start_h*60 + e.start_m; int new_end   = e.end_h*60 + e.end_m; int overlap = 0;
            for (int k=0;k<tt_count;k++) {
                TimetableEntry *ex = &tt_entries[k];
                if (ex->day != e.day || ex->type != TT_LECTURE) continue;
                int ex_start = ex->start_h*60 + ex->start_m; int ex_end = ex->end_h*60 + ex->end_m;
                if (times_overlap_minutes(new_start,new_end, ex_start, ex_end)) { overlap = 1; break; }
            }
            if (!overlap) { tt_entries[tt_count++] = e; save_timetable(); }
        }
    }
    if (ButtonRect((Rectangle){(float)sidebarX + 230, (float)formY, 180, 40}, "Save Timetable")) save_timetable();
    formY += 40 + fieldSpacing;

    DrawText("Entries (Lectures Only):", sidebarX, formY, 20, DARKGRAY);
    formY += 30;
    int ly = formY;
    for (int i=0;i<tt_count;i++) {
        TimetableEntry *e = &tt_entries[i];
        if (e->type != TT_LECTURE) continue;
        char buf[256];
        sprintf(buf, "%s %02d:%02d @%s", DAYS[e->day], e->start_h, e->start_m, e->title);
        DrawText(buf, sidebarX, ly, 20, BLACK);
        Rectangle del = {sidebarX + 300, ly - 5, 100, 30};
        if (ButtonRect(del, "Delete")) {
            for (int k=i;k<tt_count-1;k++) tt_entries[k] = tt_entries[k+1];
            tt_count--; save_timetable(); break;
        }
        ly += 40;
        if (ly > 820) break;
    }
}

// ---------------- Lab Tab (MODIFIED) ----------------
// MODIFIED: Reworked sidebar layout to be sequential
void Tab_Labs(int x, int y, int w, int sidebarX) {
    DrawText("Lab Timetable", x, y, 30, DARKBLUE);
    DrawTimetableGridFixed(x, y+50, w, 700, 1);

    // --- Sidebar Form (Sequentially Drawn) ---
    DrawText("Add Lab", sidebarX, y, 26, DARKBLUE);
    
    int formY = y + 40; // Current Y-position for drawing
    int inputW = 440; // Full width of sidebar area
    int inputW_half = 210;
    int inputH = 40;
    int labelOffset = 28; // Space between label and box
    int fieldSpacing = 10; // Space between fields

    DrawText("Title:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_lab_title.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, (float)inputH};
    DrawInput(&ib_lab_title, "Lab title");
    formY += inputH + fieldSpacing;

    DrawText("Room:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_lab_room.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, (float)inputH};
    DrawInput(&ib_lab_room, "Room");
    formY += inputH + fieldSpacing;

    DrawText("Professor:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_lab_prof.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, (float)inputH};
    DrawInput(&ib_lab_prof, "Prof. name");
    formY += inputH + fieldSpacing;

    DrawText("Start (HH:MM):", sidebarX, formY, 20, BLACK);
    DrawText("End (HH:MM):", sidebarX + inputW_half + 20, formY, 20, BLACK);
    formY += labelOffset;
    ib_lab_start.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW_half, (float)inputH};
    DrawInput(&ib_lab_start, "14:00");
    ib_lab_end.rect = (Rectangle){(float)sidebarX + inputW_half + 20, (float)formY, (float)inputW_half, (float)inputH};
    DrawInput(&ib_lab_end, "15:00");
    formY += inputH + fieldSpacing;

    DrawText("Day:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    for (int d=0; d<5; d++) {
        Rectangle r = {sidebarX + d*52, formY, 48, 34};
        DrawRectangleRec(r, d==ib_lab_day ? Fade(BLUE,0.85f) : LIGHTGRAY);
        DrawRectangleLines((int)r.x,(int)r.y,(int)r.width,(int)r.height,GRAY);
        DrawText(DAYS[d], r.x+10, r.y+8, 16, BLACK);
        Vector2 mp = GetMousePosition();
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mp, r)) ib_lab_day = d;
    }
    formY += 34 + fieldSpacing;
    
    if (ButtonRect((Rectangle){(float)sidebarX, (float)formY, 220, 40}, "Add Lab")) {
        // ... [Add logic is unchanged] ...
        if (tt_count < MAX_ENTRIES) {
            TimetableEntry e; memset(&e,0,sizeof(e));
            e.day = ib_lab_day;
            e.start_h = 14; e.start_m = 0; e.end_h = 15; e.end_m = 0;
            if (sscanf(ib_lab_start.text, "%d:%d", &e.start_h, &e.start_m) < 1) { e.start_h = 14; e.start_m = 0; }
            if (sscanf(ib_lab_end.text, "%d:%d", &e.end_h, &e.end_m) < 1) { e.end_h = e.start_h + 1; e.end_m = 0; }
            if ((e.end_h*60 + e.end_m) <= (e.start_h*60 + e.start_m)) { e.end_h = e.start_h + 1; e.end_m = 0; }
            if (e.start_h < 14) { e.start_h = 14; e.start_m = 0; }
            if (e.end_h > 18) { e.end_h = 18; e.end_m = 0; }
            if (e.start_h > 18) { e.start_h = 18; e.start_m = 0; }
            if (e.end_h < 14) { e.end_h = 14; e.end_m = 0; }
            scpy(e.title, ib_lab_title.text[0] ? ib_lab_title.text : "Lab");
            scpy(e.room, ib_lab_room.text[0] ? ib_lab_room.text : "Room");
            scpy(e.professor, ib_lab_prof.text[0] ? ib_lab_prof.text : "Prof");
            e.type = TT_LAB;
            e.id = (tt_count>0 ? tt_entries[tt_count-1].id + 1 : 1);
            int new_start = e.start_h*60 + e.start_m; int new_end   = e.end_h*60 + e.end_m; int overlap = 0;
            for (int k=0;k<tt_count;k++) {
                TimetableEntry *ex = &tt_entries[k];
                if (ex->day != e.day || ex->type != TT_LAB) continue;
                int ex_start = ex->start_h*60 + ex->start_m; int ex_end = ex->end_h*60 + ex->end_m;
                if (times_overlap_minutes(new_start,new_end, ex_start, ex_end)) { overlap = 1; break; }
            }
            if (!overlap) { tt_entries[tt_count++] = e; save_timetable(); }
        }
    }

    formY += 40 + fieldSpacing;
    DrawText("Entries (Labs Only):", sidebarX, formY, 20, DARKGRAY);
    formY += 30;
    int ly = formY;
    for (int i=0;i<tt_count;i++) {
        if (tt_entries[i].type != TT_LAB) continue;
        TimetableEntry *e = &tt_entries[i];
        char buf[256]; sprintf(buf, "%s %02d:%02d @%s", DAYS[e->day], e->start_h,e->start_m, e->title);
        DrawText(buf, sidebarX, ly, 20, BLACK);
        Rectangle del = {sidebarX + 300, ly - 5, 100, 30};
        if (ButtonRect(del, "Delete")) {
            for (int k=i;k<tt_count-1;k++) tt_entries[k] = tt_entries[k+1];
            tt_count--; save_timetable(); break;
        }
        ly += 40; if (ly > 820) break;
    }
}

// ---------------- Assignments Tab (MODIFIED) ----------------
// MODIFIED: Reworked sidebar layout to be sequential
void Tab_Assign(int x, int y, int w, int sidebarX) {
    DrawText("Assignments", x, y, 30, DARKBLUE);

    // Left: list area
    int listX = x, listY = y + 50, listW = w, listH = 700;
    DrawRectangle(listX, listY, listW, listH, RAYWHITE);
    DrawRectangleLines(listX, listY, listW, listH, LIGHTGRAY);

    DrawText("Title", listX+10, listY+10, 20, DARKGRAY);
    DrawText("Due", listX+320, listY+10, 20, DARKGRAY);
    DrawText("Status", listX+450, listY+10, 20, DARKGRAY);
    DrawText("Issued By", listX+580, listY+10, 20, DARKGRAY);
    DrawText("Description", listX+750, listY+10, 20, DARKGRAY);
    DrawLine(listX, listY+40, listX+listW, listY+40, LIGHTGRAY);

    int ly = listY + 50;
    for (int i=0;i<assign_count;i++) {
        Assignment *a = &assignments[i];
        DrawText(a->title, listX+10, ly, 20, BLACK);
        DrawText(a->due_date, listX+320, ly, 20, BLACK);
        const char* st = a->status==ASSIGNED ? "Assigned" : a->status==PENDING ? "Pending" : "Submitted";
        Rectangle btn = {listX+450, ly-4, 110, 30};
        if (ButtonRect(btn, st)) {
            if (a->status == ASSIGNED) a->status = PENDING;
            else if (a->status == PENDING) { a->status = SUBMITTED; scpy(a->submitted_date, "2025-10-27"); }
            else a->status = ASSIGNED;
            save_assignments();
        }
        DrawText(a->issued_by, listX+580, ly, 20, DARKGRAY);
        DrawText(a->desc, listX+750, ly, 20, DARKGRAY);
        
        Rectangle del = {listX+listW-100, ly-4, 80, 30};
        if (ButtonRect(del, "Delete")) {
            for (int k=i;k<assign_count-1;k++) assignments[k]=assignments[k+1];
            assign_count--; save_assignments(); break;
        }
        ly += 44; if (ly > listY + listH - 30) break;
    }

    // --- Sidebar Form (Sequentially Drawn) ---
    DrawText("Add / Edit Assignment", sidebarX, y, 26, DARKBLUE);
    
    int formY = y + 40;
    int inputW = 440;
    int inputH = 40;
    int labelOffset = 28;
    int fieldSpacing = 10;

    DrawText("Title:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_assign_title.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, (float)inputH};
    DrawInput(&ib_assign_title, "Title");
    formY += inputH + fieldSpacing;

    DrawText("Due (YYYY-MM-DD):", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_assign_due.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, (float)inputH};
    DrawInput(&ib_assign_due, "YYYY-MM-DD");
    formY += inputH + fieldSpacing;

    DrawText("Issued By:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_assign_issuedby.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, (float)inputH};
    DrawInput(&ib_assign_issuedby, "Prof. name");
    formY += inputH + fieldSpacing;

    DrawText("Desc:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_assign_desc.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, 120}; // Taller box
    DrawInput(&ib_assign_desc, "Short desc");
    formY += 120 + fieldSpacing;

    if (ButtonRect((Rectangle){(float)sidebarX, (float)formY, 200, 40}, "Add Assignment")) {
        if (assign_count < MAX_ENTRIES) {
            Assignment a; memset(&a,0,sizeof(a));
            a.id = assign_count>0 ? assignments[assign_count-1].id + 1 : 1;
            scpy(a.title, ib_assign_title.text[0] ? ib_assign_title.text : "Untitled");
            scpy(a.due_date, ib_assign_due.text[0] ? ib_assign_due.text : "YYYY-MM-DD");
            scpy(a.issued_by, ib_assign_issuedby.text[0] ? ib_assign_issuedby.text : "Prof");
            scpy(a.desc, ib_assign_desc.text[0] ? ib_assign_desc.text : "-");
            a.status = ASSIGNED; scpy(a.submitted_date, "");
            assignments[assign_count++] = a;
            save_assignments();
        }
    }
    if (ButtonRect((Rectangle){(float)sidebarX + 210, (float)formY, 200, 40}, "Save Assignments")) save_assignments();
}

// ---------------- Semester Tab (MODIFIED) ----------------
// MODIFIED: Reworked sidebar layout to be sequential
void Tab_Sem(int x, int y, int w, int sidebarX) {
    DrawText("Semester Information & Calendar", x, y, 30, DARKBLUE);
    
    int mainY = y + 50;
    DrawText("Semester: ", x, mainY, 20, BLACK);
    DrawText(seminfo.semester_name, x+120, mainY, 20, DARKGRAY); mainY += 30;
    DrawText("Start: ", x, mainY, 20, BLACK); DrawText(seminfo.start_date, x+120, mainY, 20, DARKGRAY); mainY += 30;
    DrawText("End: ", x, mainY, 20, BLACK); DrawText(seminfo.end_date, x+120, mainY, 20, DARKGRAY); mainY += 30;
    char credBuf[64]; sprintf(credBuf, "Total Credits: %d", seminfo.total_credits);
    DrawText(credBuf, x, mainY, 20, BLACK);

    mainY += 60;
    DrawText("Official Holiday List 2025", x, mainY, 24, DARKBLUE);
    int hy = mainY + 40;
    DrawText("26/01/2025 (Sunday)  -  Republic Day", x, hy, 20, BLACK); hy += 30;
    DrawText("26/02/2025 (Wednesday)  -  Maha Shivaratri", x, hy, 20, BLACK); hy += 30;
    DrawText("14/03/2025 (Friday)  -  Holi / Dhulivandan", x, hy, 20, BLACK); hy += 30;
    DrawText("15/08/2025 (Friday)  -  Independence Day", x, hy, 20, BLACK); hy += 30;
    DrawText("02/10/2025 (Thursday)  -  Mahatma Gandhi’s Birthday", x, hy, 20, BLACK); hy += 30;
    DrawText("25/12/2025 (Thursday)  -  Christmas Day", x, hy, 20, BLACK); hy += 30;
    
    int ex = x + 700;
    int ey = mainY + 40;
    DrawText("Exams:", ex, mainY, 24, DARKBLUE);
    DrawText("MIDESEM: 03/10/2025 -> 08/10/2025", ex, ey, 20, BLACK); ey += 40;
    DrawText("ENDESEM: 03/12/2025 -> 08/12/2025", ex, ey, 20, BLACK); ey += 20;

    // --- Sidebar Form (Sequentially Drawn) ---
    DrawText("Edit Semester / Add Items", sidebarX, y, 26, DARKBLUE);
    
    int formY = y + 40;
    int inputW = 440;
    int inputW_half = 210;
    int inputH = 40;
    int labelOffset = 28;
    int fieldSpacing = 10;

    DrawText("Name:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_sem_name.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, (float)inputH};
    DrawInput(&ib_sem_name, "Semester name");
    formY += inputH + fieldSpacing;

    DrawText("Start:", sidebarX, formY, 20, BLACK);
    DrawText("End:", sidebarX + inputW_half + 20, formY, 20, BLACK);
    formY += labelOffset;
    ib_sem_start.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW_half, (float)inputH};
    DrawInput(&ib_sem_start, "YYYY-MM-DD");
    ib_sem_end.rect = (Rectangle){(float)sidebarX + inputW_half + 20, (float)formY, (float)inputW_half, (float)inputH};
    DrawInput(&ib_sem_end, "YYYY-MM-DD");
    formY += inputH + fieldSpacing;

    DrawText("Credits:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_sem_credit.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW_half, (float)inputH};
    DrawInput(&ib_sem_credit, "20");
    formY += inputH + fieldSpacing;
    
    if (ButtonRect((Rectangle){(float)sidebarX, (float)formY, 240, 40}, "Save Semester")) {
        scpy(seminfo.semester_name, ib_sem_name.text[0] ? ib_sem_name.text : seminfo.semester_name);
        scpy(seminfo.start_date, ib_sem_start.text[0] ? ib_sem_start.text : seminfo.start_date);
        scpy(seminfo.end_date, ib_sem_end.text[0] ? ib_sem_end.text : seminfo.end_date);
        seminfo.total_credits = atoi(ib_sem_credit.text);
        save_semester();
    }
}

// ---------------- Attendance Tab (MODIFIED) ----------------
// MODIFIED: Reworked sidebar layout to be sequential
void Tab_Attendance(int x, int y, int w, int sidebarX) {
    DrawText("Student Attendance (lecture-wise)", x, y, 30, DARKBLUE);
    
    int listX = x, listY = y + 50, listW = w, listH = 700;
    DrawRectangle(listX, listY, listW, listH, RAYWHITE);
    DrawRectangleLines(listX, listY, listW, listH, LIGHTGRAY);
    
    DrawText("Subject", x+10, y+60, 20, DARKGRAY);
    DrawText("Total", x+400, y+60, 20, DARKGRAY);
    DrawText("Attended", x+500, y+60, 20, DARKGRAY);
    DrawText("Missed", x+620, y+60, 20, DARKGRAY);
    DrawText("Percent", x+720, y+60, 20, DARKGRAY);
    DrawLine(listX, listY+40, listX+listW, listY+40, LIGHTGRAY);
    
    int ly = y+100;
    for (int i=0;i<attendance_count;i++) {
        AttendanceRecord *a = &attendance[i];
        DrawText(a->subject, x+10, ly, 20, BLACK);
        char buf[64]; sprintf(buf, "%d", a->total_lectures); DrawText(buf, x+400, ly, 20, BLACK);
        sprintf(buf, "%d", a->attended); DrawText(buf, x+500, ly, 20, BLACK);
        sprintf(buf, "%d", a->total_lectures - a->attended); DrawText(buf, x+620, ly, 20, BLACK);
        float pct = a->total_lectures ? (100.0f * a->attended / a->total_lectures) : 0.0f;
        char pctbuf[32]; sprintf(pctbuf, "%.1f%%", pct); DrawText(pctbuf, x+720, ly, 20, BLACK);
        Rectangle del = {x+w-100, ly-6, 80, 30};
        if (ButtonRect(del, "Delete")) {
            for (int k=i;k<attendance_count-1;k++) attendance[k]=attendance[k+1];
            attendance_count--; save_attendance(); break;
        }
        ly += 44; if (ly > listY + listH - 30) break;
    }
    
    // --- Sidebar Form (Sequentially Drawn) ---
    DrawText("Add Subject Attendance", sidebarX, y, 26, DARKBLUE);

    int formY = y + 40;
    int inputW = 440;
    int inputW_half = 210;
    int inputH = 40;
    int labelOffset = 28;
    int fieldSpacing = 10;

    DrawText("Subject:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_att_subject.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, (float)inputH};
    DrawInput(&ib_att_subject, "Subject");
    formY += inputH + fieldSpacing;

    DrawText("Total lects:", sidebarX, formY, 20, BLACK);
    DrawText("Attended:", sidebarX + inputW_half + 20, formY, 20, BLACK);
    formY += labelOffset;
    ib_att_total.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW_half, (float)inputH};
    DrawInput(&ib_att_total, "Total");
    ib_att_attended.rect = (Rectangle){(float)sidebarX + inputW_half + 20, (float)formY, (float)inputW_half, (float)inputH};
    DrawInput(&ib_att_attended, "Attended");
    formY += inputH + fieldSpacing;
    
    if (ButtonRect((Rectangle){(float)sidebarX, (float)formY, 300, 40}, "Add Attendance")) {
        if (attendance_count < MAX_ENTRIES) {
            AttendanceRecord a; memset(&a,0,sizeof(a));
            scpy(a.subject, ib_att_subject.text[0] ? ib_att_subject.text : "Subject");
            a.total_lectures = atoi(ib_att_total.text);
            a.attended = atoi(ib_att_attended.text);
            attendance[attendance_count++] = a; save_attendance();
        }
    }
}

// ---------------- Results Tab (MODIFIED) ----------------
// MODIFIED: Reworked sidebar layout to be sequential
void Tab_Results(int x, int y, int w, int sidebarX) {
    DrawText("Results / Grades", x, y, 30, DARKBLUE);
    
    int listX = x, listY = y + 50, listW = w, listH = 700;
    DrawRectangle(listX, listY, listW, listH, RAYWHITE);
    DrawRectangleLines(listX, listY, listW, listH, LIGHTGRAY);
    
    DrawText("Student ID", x+10, y+60, 20, DARKGRAY);
    DrawText("Subject", x+180, y+60, 20, DARKGRAY);
    DrawText("Credits", x+450, y+60, 20, DARKGRAY);
    DrawText("Earned", x+550, y+60, 20, DARKGRAY);
    DrawText("SPI", x+650, y+60, 20, DARKGRAY);
    DrawText("Grade", x+730, y+60, 20, DARKGRAY);
    DrawText("Remarks", x+810, y+60, 20, DARKGRAY);
    DrawLine(listX, listY+40, listX+listW, listY+40, LIGHTGRAY);

    int ly = y+100;
    float totalSPI = 0.0f; int spi_count = 0;
    int totalCredits = 0, creditsEarnedTotal = 0;
    
    for (int i=0;i<results_count;i++) {
        ResultRecord *r = &results[i];
        DrawText(r->student_id, x+10, ly, 20, BLACK);
        DrawText(r->subject, x+180, ly, 20, BLACK);
        char tmp[64];
        sprintf(tmp, "%d", r->total_credits); DrawText(tmp, x+450, ly, 20, BLACK);
        sprintf(tmp, "%d", r->credits_earned); DrawText(tmp, x+550, ly, 20, BLACK);
        sprintf(tmp, "%.2f", r->SPI); DrawText(tmp, x+650, ly, 20, BLACK);
        DrawText(r->grade, x+730, ly, 20, BLACK);
        DrawText(r->remarks, x+810, ly, 20, DARKGRAY);
        Rectangle del = {x+w-100, ly-6, 80, 30};
        if (ButtonRect(del, "Delete")) {
            for (int k=i;k<results_count-1;k++) results[k]=results[k+1];
            results_count--; save_results(); break;
        }
        if (r->SPI > 0.0f) { totalSPI += r->SPI; spi_count++; }
        totalCredits += r->total_credits;
        creditsEarnedTotal += r->credits_earned;
        ly += 44; if (ly > listY + listH - 30) break;
    }
    float SPIavg = spi_count ? totalSPI / spi_count : 0.0f;
    char stats[256]; sprintf(stats, "Total credits: %d  Earned: %d  Avg SPI: %.2f", totalCredits, creditsEarnedTotal, SPIavg);
    DrawText(stats, x+10, listY + listH + 10, 20, DARKGRAY);

    // --- Sidebar Form (Sequentially Drawn) ---
    DrawText("Add Result", sidebarX, y, 26, DARKBLUE);

    int formY = y + 40;
    int inputW = 440;
    int inputW_half = 210;
    int inputH = 40;
    int labelOffset = 28;
    int fieldSpacing = 10;

    DrawText("Student ID:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_res_id.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, (float)inputH};
    DrawInput(&ib_res_id, "Student ID");
    formY += inputH + fieldSpacing;

    DrawText("Subject:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_res_subject.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, (float)inputH};
    DrawInput(&ib_res_subject, "Subject");
    formY += inputH + fieldSpacing;

    DrawText("Credits:", sidebarX, formY, 20, BLACK);
    DrawText("Earned:", sidebarX + inputW_half + 20, formY, 20, BLACK);
    formY += labelOffset;
    ib_res_credits.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW_half, (float)inputH};
    DrawInput(&ib_res_credits, "Credits");
    ib_res_earned.rect = (Rectangle){(float)sidebarX + inputW_half + 20, (float)formY, (float)inputW_half, (float)inputH};
    DrawInput(&ib_res_earned, "Earned");
    formY += inputH + fieldSpacing;

    DrawText("SPI:", sidebarX, formY, 20, BLACK);
    DrawText("Grade:", sidebarX + inputW_half + 20, formY, 20, BLACK);
    formY += labelOffset;
    ib_res_spi.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW_half, (float)inputH};
    DrawInput(&ib_res_spi, "SPI");
    ib_res_grade.rect = (Rectangle){(float)sidebarX + inputW_half + 20, (float)formY, (float)inputW_half, (float)inputH};
    DrawInput(&ib_res_grade, "A, B+, ...");
    formY += inputH + fieldSpacing;

    DrawText("Remarks:", sidebarX, formY, 20, BLACK);
    formY += labelOffset;
    ib_res_remarks.rect = (Rectangle){(float)sidebarX, (float)formY, (float)inputW, (float)inputH};
    DrawInput(&ib_res_remarks, "Remarks");
    formY += inputH + fieldSpacing;
    
    if (ButtonRect((Rectangle){(float)sidebarX, (float)formY, 300, 40}, "Add Result")) {
        if (results_count < MAX_ENTRIES) {
            ResultRecord r; memset(&r,0,sizeof(r));
            scpy(r.student_id, ib_res_id.text[0] ? ib_res_id.text : "N/A");
            scpy(r.subject, ib_res_subject.text[0] ? ib_res_subject.text : "Subject");
            r.total_credits = atoi(ib_res_credits.text);
            r.credits_earned = atoi(ib_res_earned.text);
            r.SPI = (float)atof(ib_res_spi.text);
            scpy(r.grade, ib_res_grade.text);
            scpy(r.remarks, ib_res_remarks.text);
            r.passed = (strcmp(r.grade,"F")!=0 && r.SPI > 0.0f) ? 1 : 0;
            results[results_count++] = r; save_results();
        }
    }
}

// ---------------- MAIN (MODIFIED) ----------------
int main(int argc, char *argv[]) {
    if(argc<3) return 0;
    const int screenWidth = 1920;
    const int screenHeight = 1080;
    InitWindow(screenWidth, screenHeight, "University Portal - Academics (Faculty Window)");
    SetTargetFPS(60);

    // load persistent data
    load_timetable(); load_assignments(); load_events();
    load_semester(); load_attendance(); load_results();

    // MODIFIED: Init InputBoxes with empty rects.
    // Their positions will be set dynamically in the Tab_... functions.
    ib_tt_title = MakeInput(0,0,0,0, "");
    ib_tt_room  = MakeInput(0,0,0,0, "");
    ib_tt_prof  = MakeInput(0,0,0,0, "");
    ib_tt_start = MakeInput(0,0,0,0, "08:00");
    ib_tt_end   = MakeInput(0,0,0,0, "09:00");

    ib_lab_title = MakeInput(0,0,0,0, "");
    ib_lab_room  = MakeInput(0,0,0,0, "");
    ib_lab_prof  = MakeInput(0,0,0,0, "");
    ib_lab_start = MakeInput(0,0,0,0, "14:00");
    ib_lab_end   = MakeInput(0,0,0,0, "15:00");

    ib_assign_title    = MakeInput(0,0,0,0, "");
    ib_assign_due      = MakeInput(0,0,0,0, "YYYY-MM-DD");
    ib_assign_issuedby = MakeInput(0,0,0,0, "Prof. name");
    ib_assign_desc     = MakeInput(0,0,0,0, "");

    ib_event_title = MakeInput(0,0,0,0, "");
    ib_event_date  = MakeInput(0,0,0,0, "");
    ib_event_desc  = MakeInput(0,0,0,0, "");

    ib_sem_name   = MakeInput(0,0,0,0, seminfo.semester_name);
    ib_sem_start  = MakeInput(0,0,0,0, seminfo.start_date);
    ib_sem_end    = MakeInput(0,0,0,0, seminfo.end_date);
    char credbuf[32]; sprintf(credbuf, "%d", seminfo.total_credits);
    ib_sem_credit = MakeInput(0,0,0,0, credbuf);

    ib_att_subject  = MakeInput(0,0,0,0, "");
    ib_att_total    = MakeInput(0,0,0,0, "0");
    ib_att_attended = MakeInput(0,0,0,0, "0");

    ib_res_id      = MakeInput(0,0,0,0, "");
    ib_res_subject = MakeInput(0,0,0,0, "");
    ib_res_credits = MakeInput(0,0,0,0, "4");
    ib_res_earned  = MakeInput(0,0,0,0, "4");
    ib_res_spi     = MakeInput(0,0,0,0, "9.00");
    ib_res_grade   = MakeInput(0,0,0,0, "A");
    ib_res_remarks = MakeInput(0,0,0,0, "");

    while (!WindowShouldClose()) {
        // This array-copy method is complex but required for HandleInputs
        InputBox* all_inputs[] = {
            &ib_tt_title, &ib_tt_room, &ib_tt_prof, &ib_tt_start, &ib_tt_end,
            &ib_lab_title, &ib_lab_room, &ib_lab_prof, &ib_lab_start, &ib_lab_end,
            &ib_assign_title, &ib_assign_due, &ib_assign_desc, &ib_assign_issuedby,
            &ib_event_title, &ib_event_date, &ib_event_desc,
            &ib_sem_name, &ib_sem_start, &ib_sem_end, &ib_sem_credit,
            &ib_att_subject, &ib_att_total, &ib_att_attended,
            &ib_res_subject, &ib_res_credits, &ib_res_earned, &ib_res_spi, &ib_res_grade, &ib_res_remarks,
            &ib_res_id
        };
        int nInputs = sizeof(all_inputs)/sizeof(all_inputs[0]);
        
        // MODIFIED: Create a temporary array of structs (not pointers) for HandleInputs
        InputBox inputs_copy[nInputs];
        for (int i=0; i<nInputs; i++) inputs_copy[i] = *all_inputs[i];
        
        HandleInputs(inputs_copy, nInputs);
        
        // Copy data back to globals
        for (int i=0; i<nInputs; i++) *all_inputs[i] = inputs_copy[i];


        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawHeader(screenWidth);
        
        int contentX = 50;
        int contentY = 130;
        int mainContentWidth = (int)(screenWidth * 0.65) - 70; // Adjusted width
        int sideBarX = (int)(screenWidth * 0.65) + 30;

        switch(active_tab) {
            case TAB_TIMETABLE: Tab_Timetable(contentX, contentY, mainContentWidth, sideBarX); break;
            case TAB_LABS:      Tab_Labs(contentX, contentY, mainContentWidth, sideBarX); break;
            case TAB_ASSIGN:    Tab_Assign(contentX, contentY, mainContentWidth, sideBarX); break;
            case TAB_SEM:       Tab_Sem(contentX, contentY, mainContentWidth, sideBarX); break;
            case TAB_ATT:       Tab_Attendance(contentX, contentY, mainContentWidth, sideBarX); break;
            case TAB_RES:       Tab_Results(contentX, contentY, mainContentWidth, sideBarX); break;
            default: Tab_Timetable(contentX, contentY, mainContentWidth, sideBarX); break;
        }
        EndDrawing();
    }

    // Save on exit
    save_timetable(); save_assignments(); save_events();
    save_semester(); save_attendance(); save_results();

    CloseWindow();
    return 0;
}