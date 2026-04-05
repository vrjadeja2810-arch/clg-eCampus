/*******************************************************************************************
*
* Student Management System - C and Raylib
*
* REFACFORED VERSION 3.4 (DEFAULT FONT)
* - All custom fonts ("Inter") have been removed.
* - All text rendering reverted to default Raylib font.
* - Retains all features: animations, scrollbar, themes, etc.
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h" // For Lerp(), Clamp(), fabsf()
#include <stdio.h>
#include <string.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define MAX_SEMESTERS 8
#define MAX_COURSES_PER_SEM 10
#define MAX_DESCRIPTIONS 20
#define MAX_TABS 10 // Overview(1) + Semesters(8) + Descriptions(1)
#define PROGRAM_COUNT 2 // Number of programs (ICT, MnC)

// --- High-contrast text colors ---
#define TEXT_DARK (Color){ 40, 40, 40, 255 }
#define TEXT_LIGHT (Color){ 100, 100, 100, 255 }

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// Represents the different screens in the application
typedef enum {
    SCREEN_SELECTION,
    SCREEN_PROGRAM_STRUCTURE
} GameScreen;

// Represents the selected program (used as an index)
typedef enum {
    PROGRAM_ICT = 0,
    PROGRAM_MNC = 1
} ProgramType;

// --- Data Structures for Program Structure Component ---
typedef struct {
    const char *code;
    const char *name;
    int credits;
    const char *type;
    const char *prereq;
} Course;

typedef struct {
    const char *name;       // e.g., "Semester 1"
    const char *tabName;    // e.g., "Sem 1"
    Course courses[MAX_COURSES_PER_SEM];
    int courseCount;
} Semester;

typedef struct {
    const char *name;
    const char *description;
} CourseDesc;

// --- UNIFIED PROGRAM STRUCT ---
typedef struct {
    const char *name; // e.g., "B.Tech (ICT)"
    const char *title; // e.g., "Program Structure (B.Tech ICT)"
    char tabNames[MAX_TABS][32];
    Semester semesters[MAX_SEMESTERS];
    CourseDesc descriptions[MAX_DESCRIPTIONS];
    int descriptionCount;
    Color themeColor; // Program-specific theme color
} Program;


//----------------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------------
const int screenWidth = 1920;
const int screenHeight = 1080;

GameScreen currentScreen = SCREEN_SELECTION;

// --- Unified Data Storage ---
Program programs[PROGRAM_COUNT]; // 0 = ICT, 1 = MnC
Program *activeProgramPtr = NULL; // Pointer to the currently active program

// --- Globals for Program Structure Component ---
static int activeTab = 0; // 0=Overview, 1-8=Semesters, 9=Descriptions
static Vector2 scroll = { 0 }; // For text scrolling in component
static float maxScroll = 0; // For scroll bounds

// --- Animation state variables ---
static int prevTab = 0;
static float contentAlpha = 1.0f;
static float hoverAlphas[MAX_COURSES_PER_SEM] = {0}; // For smooth row hover
static float scrollVelocity = 0.0f; // For scroll momentum
static float titleAlpha = 0.0f; 

// --- NO FONT GLOBALS NEEDED ---


//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void InitProgramData(void); // Loads ALL program data

// Screen-specific update/draw functions
static void UpdateDrawSelectionScreen(void);
static void UpdateDrawProgramStructureScreen(void);
static void UpdateDrawFrame(void); // Main update and drawing loop function

//----------------------------------------------------------------------------------
// Main Entry Point
//----------------------------------------------------------------------------------

/**
 * @brief Main entry point of the application.
 * Initializes the window, loads all program data, and runs the main loop.
 */
int main(int argc, char *argv[]) 
{   
    if(argc<3){
        return 0;
    }

    InitWindow(screenWidth, screenHeight, "University Portal - Program Structures");
    
    // --- Fonts are no longer loaded ---

    // Load all program data at startup
    InitProgramData();
    
    SetTargetFPS(60); // We will use 60 FPS for smooth animations

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
#endif

    // --- Fonts are no longer unloaded ---

    CloseWindow();
    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

/**
 * @brief Main router function, called 60 times per second.
 * Checks the 'currentScreen' global variable and calls the appropriate
 * update/draw function for that screen.
 */
void UpdateDrawFrame(void) {
    // This function acts as a router
    switch (currentScreen) {
        case SCREEN_SELECTION:
            UpdateDrawSelectionScreen();
            break;
        case SCREEN_PROGRAM_STRUCTURE:
            UpdateDrawProgramStructureScreen();
            break;
        default:
            break;
    }
}

/**
 * @brief Loads all hard-coded curriculum data into the global 'programs' array.
 * This function runs only once at startup. It populates all semester
 * and description data for both B.Tech (ICT) and B.Tech (MnC).
 */
void InitProgramData(void) {
    //==================================================================
    // Program 0: B.Tech (ICT)
    //==================================================================
    programs[PROGRAM_ICT].name = "B.Tech (ICT)";
    programs[PROGRAM_ICT].title = "Program Structure (B.Tech ICT)";
    programs[PROGRAM_ICT].themeColor = (Color){ 230, 240, 255, 255 }; // Light Blue
    
    // --- POPULATE ICT TAB NAMES ---
    strcpy(programs[PROGRAM_ICT].tabNames[0], "Overview");
    for (int i = 1; i <= 8; i++) {
        sprintf(programs[PROGRAM_ICT].tabNames[i], "Sem %d", i);
    }
    strcpy(programs[PROGRAM_ICT].tabNames[9], "Descriptions");

    // --- POPULATE ICT SEMESTERS ---
    programs[PROGRAM_ICT].semesters[0] = (Semester){ "Semester 1", "Sem 1", .courseCount = 6 };
    programs[PROGRAM_ICT].semesters[0].courses[0] = (Course){ "IC101", "Introduction to ICT", 2, "Core", "None" };
    programs[PROGRAM_ICT].semesters[0].courses[1] = (Course){ "SC107", "Calculus", 4, "Core", "None" };
    programs[PROGRAM_ICT].semesters[0].courses[2] = (Course){ "IT112", "Introduction to Programming", 3, "Core", "None" };
    programs[PROGRAM_ICT].semesters[0].courses[3] = (Course){ "IT113", "Programming Lab", 1, "Lab", "CS101" };
    programs[PROGRAM_ICT].semesters[0].courses[4] = (Course){ "EL111", "Basic ELectronic Circuit", 2, "Core", "None" };
    programs[PROGRAM_ICT].semesters[0].courses[5] = (Course){ "PC110", "Language and Literature", 3, "Core", "None" };

    programs[PROGRAM_ICT].semesters[1] = (Semester){ "Semester 2", "Sem 2", .courseCount = 6 };
    programs[PROGRAM_ICT].semesters[1].courses[0] = (Course){ "MA102", "Discrete Mathematics", 4, "Core", "MA101" };
    programs[PROGRAM_ICT].semesters[1].courses[1] = (Course){ "PH101", "Physics for Engineers", 4, "Core", "None" };
    programs[PROGRAM_ICT].semesters[1].courses[2] = (Course){ "EC101", "Basic Electronic Circuits", 4, "Core", "None" };
    programs[PROGRAM_ICT].semesters[1].courses[3] = (Course){ "CS102", "Object Oriented Programming", 3, "Core", "CS101" };
    programs[PROGRAM_ICT].semesters[1].courses[4] = (Course){ "CS102L", "OOP Lab", 1, "Lab", "CS102" };
    programs[PROGRAM_ICT].semesters[1].courses[5] = (Course){ "SS101", "Science, Technology & Society", 2, "Core", "None" };

    programs[PROGRAM_ICT].semesters[2] = (Semester){ "Semester 3", "Sem 3", .courseCount = 6 };
    programs[PROGRAM_ICT].semesters[2].courses[0] = (Course){ "MA201", "Probability & Statistics", 4, "Core", "MA102" };
    programs[PROGRAM_ICT].semesters[2].courses[1] = (Course){ "EC201", "Signals and Systems", 4, "Core", "MA101" };
    programs[PROGRAM_ICT].semesters[2].courses[2] = (Course){ "CS201", "Data Structures", 4, "Core", "CS102" };
    programs[PROGRAM_ICT].semesters[2].courses[3] = (Course){ "CS201L", "Data Structures Lab", 1, "Lab", "CS201" };
    programs[PROGRAM_ICT].semesters[2].courses[4] = (Course){ "IC201", "Digital Logic Design", 4, "Core", "EC101" };
    programs[PROGRAM_ICT].semesters[2].courses[5] = (Course){ "IC202", "Exploration Project", 2, "Core", "None" };
    
    programs[PROGRAM_ICT].semesters[3] = (Semester){ "Semester 4", "Sem 4", .courseCount = 6 };
    programs[PROGRAM_ICT].semesters[3].courses[0] = (Course){ "MA202", "Linear Algebra & Diff. Eqs.", 4, "Core", "MA201" };
    programs[PROGRAM_ICT].semesters[3].courses[1] = (Course){ "CS202", "Algorithms", 4, "Core", "CS201" };
    programs[PROGRAM_ICT].semesters[3].courses[2] = (Course){ "CS203", "Computer Organization", 4, "Core", "IC201" };
    programs[PROGRAM_ICT].semesters[3].courses[3] = (Course){ "EC202", "Communication Systems", 4, "Core", "EC201" };
    programs[PROGRAM_ICT].semesters[3].courses[4] = (Course){ "HSS201", "Humanities Elective", 3, "Elective", "None" };
    programs[PROGRAM_ICT].semesters[3].courses[5] = (Course){ "IC203", "Design Project", 2, "Core", "IC202" };

    programs[PROGRAM_ICT].semesters[4] = (Semester){ "Semester 5", "Sem 5", .courseCount = 6 };
    programs[PROGRAM_ICT].semesters[4].courses[0] = (Course){ "CS301", "Operating Systems", 4, "Core", "CS202" };
    programs[PROGRAM_ICT].semesters[4].courses[1] = (Course){ "CS302", "Database Systems", 4, "Core", "CS201" };
    programs[PROGRAM_ICT].semesters[4].courses[2] = (Course){ "CS303", "Computer Networks", 4, "Core", "CS202" };
    programs[PROGRAM_ICT].semesters[4].courses[3] = (Course){ "IC301", "Embedded Systems", 4, "Core", "IC201" };
    programs[PROGRAM_ICT].semesters[4].courses[4] = (Course){ "OE301", "Open Elective I", 3, "Elective", "None" };
    programs[PROGRAM_ICT].semesters[4].courses[5] = (Course){ "HSS301", "HASS Elective", 3, "Elective", "None" };

    programs[PROGRAM_ICT].semesters[5] = (Semester){ "Semester 6", "Sem 6", .courseCount = 6 };
    programs[PROGRAM_ICT].semesters[5].courses[0] = (Course){ "CS304", "Software Engineering", 4, "Core", "CS302" };
    programs[PROGRAM_ICT].semesters[5].courses[1] = (Course){ "EC301", "Digital Communication", 4, "Core", "EC202" };
    programs[PROGRAM_ICT].semesters[5].courses[2] = (Course){ "IC302", "Microprocessors & Interfacing", 4, "Core", "IC301" };
    programs[PROGRAM_ICT].semesters[5].courses[3] = (Course){ "TE301", "Technical Elective I", 3, "Elective", "None" };
    programs[PROGRAM_ICT].semesters[5].courses[4] = (Course){ "OE302", "Open Elective II", 3, "Elective", "None" };
    programs[PROGRAM_ICT].semesters[5].courses[5] = (Course){ "IC304", "Minor Project", 2, "Core", "None" };

    programs[PROGRAM_ICT].semesters[6] = (Semester){ "Semester 7", "Sem 7", .courseCount = 6 };
    programs[PROGRAM_ICT].semesters[6].courses[0] = (Course){ "TE401", "Technical Elective II", 3, "Elective", "None" };
    programs[PROGRAM_ICT].semesters[6].courses[1] = (Course){ "TE402", "Technical Elective III", 3, "Elective", "None" };
    programs[PROGRAM_ICT].semesters[6].courses[2] = (Course){ "TE403", "Technical Elective IV", 3, "Elective", "None" };
    programs[PROGRAM_ICT].semesters[6].courses[3] = (Course){ "OE401", "Open Elective III", 3, "Elective", "None" };
    programs[PROGRAM_ICT].semesters[6].courses[4] = (Course){ "IC401", "Industrial Internship", 4, "Core", "None" };
    programs[PROGRAM_ICT].semesters[6].courses[5] = (Course){ "IC402", "Project Phase I", 4, "Core", "IC304" };

    programs[PROGRAM_ICT].semesters[7] = (Semester){ "Semester 8", "Sem 8", .courseCount = 5 };
    programs[PROGRAM_ICT].semesters[7].courses[0] = (Course){ "TE404", "Technical Elective V", 3, "Elective", "None" };
    programs[PROGRAM_ICT].semesters[7].courses[1] = (Course){ "TE405", "Technical Elective VI", 3, "Elective", "None" };
    programs[PROGRAM_ICT].semesters[7].courses[2] = (Course){ "OE402", "Open Elective IV", 3, "Elective", "None" };
    programs[PROGRAM_ICT].semesters[7].courses[3] = (Course){ "IC403", "Project Phase II", 6, "Core", "IC402" };
    programs[PROGRAM_ICT].semesters[7].courses[4] = (Course){ "HSS401", "Professional Ethics", 3, "Core", "None" };

    // --- POPULATE (ICT) COURSE DESCRIPTIONS (ALL 16) ---
    programs[PROGRAM_ICT].descriptionCount = 16;
    programs[PROGRAM_ICT].descriptions[0] = (CourseDesc){"Introduction to Programming", "Introduces fundamental programming concepts (variables, control structures, functions) in C/Python and uses problem-solving and algorithmic thinking to build basic applications."};
    programs[PROGRAM_ICT].descriptions[1] = (CourseDesc){"Calculus", "Covers differential and integral calculus of one and multiple variables along with ordinary differential equations, providing mathematical tools for engineering analysis."};
    programs[PROGRAM_ICT].descriptions[2] = (CourseDesc){"Discrete Mathematics", "Teaches logic, sets, relations, functions, recurrence, combinatorics, graphs and trees - forming the mathematical backbone for computer algorithms."};
    programs[PROGRAM_ICT].descriptions[3] = (CourseDesc){"Data Structures", "Explores storage and retrieval of data efficiently by studying arrays, linked lists, stacks, queues, hash tables, trees, heaps and associated algorithms."};
    programs[PROGRAM_ICT].descriptions[4] = (CourseDesc){"Digital Logic and Computer Organization", "Introduces design and implementation of digital circuits, flip-flops, registers, finite state machines, and the architecture of CPU/memory systems."};
    programs[PROGRAM_ICT].descriptions[5] = (CourseDesc){"Algorithms", "Focuses on designing, analysing and implementing algorithmic strategies such as divide-and-conquer, greedy, dynamic programming, graphs and trees for efficient problem solving."};
    programs[PROGRAM_ICT].descriptions[6] = (CourseDesc){"Operating Systems (Sem 1-4)", "Covers how operating systems manage processes, memory, I/O, file systems, concurrency and scheduling so software runs reliably on hardware."};
    programs[PROGRAM_ICT].descriptions[7] = (CourseDesc){"Software Engineering (Sem 1-4)", "Introduces software development lifecycle, modelling, architecture, testing, DevOps practices and quality assurance for building large-scale software systems"};
    programs[PROGRAM_ICT].descriptions[8] = (CourseDesc){"Operating Systems (Sem 5-8)", "Examines how modern operating systems manage hardware and software resources-including processes, threads, memory, I/O and file systems-to enable reliable computing."};
    programs[PROGRAM_ICT].descriptions[9] = (CourseDesc){"Database Systems", "Introduces relational database design, query languages (SQL), transactions, indexing and techniques for efficient data storage and retrieval in large systems."};
    programs[PROGRAM_ICT].descriptions[10] = (CourseDesc){"Computer Networks", "Explores network architecture, protocols (TCP/IP stack), wired & wireless systems, routing/switching and network applications in real-world scenarios."};
    programs[PROGRAM_ICT].descriptions[11] = (CourseDesc){"Software Engineering (Sem 5-8)", "Covers the full software development lifecycle: requirements engineering, design patterns, version control, testing, deployment and maintenance of large-scale systems."};
    programs[PROGRAM_ICT].descriptions[12] = (CourseDesc){"Digital Communication", "Focuses on the theoretical and practical aspects of communicating information over channels, covering modulation, coding, signal-to-noise, and error control."};
    programs[PROGRAM_ICT].descriptions[13] = (CourseDesc){"Embedded Systems / Microprocessors", "Explores embedded hardware-software systems: microcontrollers, interfacing sensors/actuators, real-time constraints and performance optimisation."};
    programs[PROGRAM_ICT].descriptions[14] = (CourseDesc){"Industrial Internship", "Provides practical industry exposure where students apply theoretical knowledge in real-world settings through a structured work-placement or project-based internship."};
    programs[PROGRAM_ICT].descriptions[15] = (CourseDesc){"Final Project (Phase I & II)", "A capstone sequence where students conceive, design, build and demonstrate a substantial system or research prototype, integrating prior coursework under faculty supervision"};

    //==================================================================
    // Program 1: B.Tech (MnC)
    //==================================================================
    programs[PROGRAM_MNC].name = "B.Tech (MnC)";
    programs[PROGRAM_MNC].title = "Program Structure (B.Tech MnC)";
    programs[PROGRAM_MNC].themeColor = (Color){ 240, 230, 255, 255 }; // Light Purple
    
    // --- POPULATE MnC TAB NAMES ---
    strcpy(programs[PROGRAM_MNC].tabNames[0], "Overview");
    for (int i = 1; i <= 8; i++) {
        sprintf(programs[PROGRAM_MNC].tabNames[i], "Sem %d", i);
    }
    strcpy(programs[PROGRAM_MNC].tabNames[9], "Descriptions");

    // --- POPULATE MnC SEMESTERS ---
    programs[PROGRAM_MNC].semesters[0] = (Semester){ "Semester 1", "Sem 1", .courseCount = 6 };
    programs[PROGRAM_MNC].semesters[0].courses[0] = (Course){ "MC101", "Mathematical, Algorithmic & Comp. Thinking", 4, "Core", "None" };
    programs[PROGRAM_MNC].semesters[0].courses[1] = (Course){ "MC112", "Computer Organization & Programming", 3, "Core", "None" };
    programs[PROGRAM_MNC].semesters[0].courses[2] = (Course){ "IT101L", "Programming Lab", 1, "Lab", "CS101" };
    programs[PROGRAM_MNC].semesters[0].courses[3] = (Course){ "MC101", "Discrete Mathematics", 4, "Core", "None" };
    programs[PROGRAM_MNC].semesters[0].courses[4] = (Course){ "MC116", "Digital Logic Design", 3, "Core", "None" };
    programs[PROGRAM_MNC].semesters[0].courses[5] = (Course){ "PC110", "Language and Litrature", 3, "Core", "None" };
    
    programs[PROGRAM_MNC].semesters[1] = (Semester){ "Semester 2", "Sem 2", .courseCount = 6 };
    programs[PROGRAM_MNC].semesters[1].courses[0] = (Course){ "MA102", "Calculus & Differential Equations", 4, "Core", "MA101" };
    programs[PROGRAM_MNC].semesters[1].courses[1] = (Course){ "CS102", "Object Oriented Programming", 3, "Core", "CS101" };
    programs[PROGRAM_MNC].semesters[1].courses[2] = (Course){ "CS102L", "OOP Lab", 1, "Lab", "CS102" };
    programs[PROGRAM_MNC].semesters[1].courses[3] = (Course){ "CS103", "Data Structures & Algorithms I", 4, "Core", "CS101" };
    programs[PROGRAM_MNC].semesters[1].courses[4] = (Course){ "MA103", "Linear Algebra", 4, "Core", "MA101" };
    programs[PROGRAM_MNC].semesters[1].courses[5] = (Course){ "HS102", "Approaches to Indian Society", 3, "Core", "None" };

    programs[PROGRAM_MNC].semesters[2] = (Semester){ "Semester 3", "Sem 3", .courseCount = 6 };
    programs[PROGRAM_MNC].semesters[2].courses[0] = (Course){ "MA201", "Probability & Random Processes", 4, "Core", "MA102" };
    programs[PROGRAM_MNC].semesters[2].courses[1] = (Course){ "CS201", "Operating Systems", 4, "Core", "CS102" };
    programs[PROGRAM_MNC].semesters[2].courses[2] = (Course){ "CS202", "Design & Analysis of Algorithms", 4, "Core", "CS103" };
    programs[PROGRAM_MNC].semesters[2].courses[3] = (Course){ "MA202", "Multivariable Calculus & PDEs", 4, "Core", "MA102" };
    programs[PROGRAM_MNC].semesters[2].courses[4] = (Course){ "CS203", "Database Management Systems", 4, "Core", "CS102" };
    programs[PROGRAM_MNC].semesters[2].courses[5] = (Course){ "HS201", "Science, Technology & Society", 3, "Core", "None" };
    
    programs[PROGRAM_MNC].semesters[3] = (Semester){ "Semester 4", "Sem 4", .courseCount = 5 };
    programs[PROGRAM_MNC].semesters[3].courses[0] = (Course){ "MA204", "Numerical Methods", 4, "Core", "MA103" };
    programs[PROGRAM_MNC].semesters[3].courses[1] = (Course){ "MA205", "Abstract Algebra", 4, "Core", "MA101" };
    programs[PROGRAM_MNC].semesters[3].courses[2] = (Course){ "CS204", "Computer Networks", 4, "Core", "CS201" };
    programs[PROGRAM_MNC].semesters[3].courses[3] = (Course){ "CS205", "Computer Architecture", 3, "Core", "CS101" };
    programs[PROGRAM_MNC].semesters[3].courses[4] = (Course){ "HS202", "Humanities Elective I", 3, "Elective", "None" };

    programs[PROGRAM_MNC].semesters[4] = (Semester){ "Semester 5", "Sem 5", .courseCount = 6 };
    programs[PROGRAM_MNC].semesters[4].courses[0] = (Course){ "MA301", "Complex Analysis", 4, "Core", "MA204" };
    programs[PROGRAM_MNC].semesters[4].courses[1] = (Course){ "MA302", "Optimization Techniques", 4, "Core", "MA103" };
    programs[PROGRAM_MNC].semesters[4].courses[2] = (Course){ "CS301", "Machine Learning Foundations", 4, "Core", "MA201" };
    programs[PROGRAM_MNC].semesters[4].courses[3] = (Course){ "CS302", "Software Engineering", 3, "Core", "CS202" };
    programs[PROGRAM_MNC].semesters[4].courses[4] = (Course){ "TE301", "Technical Elective I", 3, "Elective", "Varies" };
    programs[PROGRAM_MNC].semesters[4].courses[5] = (Course){ "PR301", "Mini Project I", 2, "Project", "CS202" };
    
    programs[PROGRAM_MNC].semesters[5] = (Semester){ "Semester 6", "Sem 6", .courseCount = 5 };
    programs[PROGRAM_MNC].semesters[5].courses[0] = (Course){ "MA303", "Stochastic Models & Simulation", 4, "Core", "MA201" };
    programs[PROGRAM_MNC].semesters[5].courses[1] = (Course){ "CS303", "Advanced Algorithms", 4, "Core", "CS202" };
    programs[PROGRAM_MNC].semesters[5].courses[2] = (Course){ "CS304", "Compiler Design", 4, "Core", "CS201" };
    programs[PROGRAM_MNC].semesters[5].courses[3] = (Course){ "TE302", "Technical Elective II", 3, "Elective", "Varies" };
    programs[PROGRAM_MNC].semesters[5].courses[4] = (Course){ "PR302", "Mini Project II", 2, "Project", "PR301" };

    programs[PROGRAM_MNC].semesters[6] = (Semester){ "Semester 7", "Sem 7", .courseCount = 5 };
    programs[PROGRAM_MNC].semesters[6].courses[0] = (Course){ "MA401", "Statistical Learning & Data Analysis", 4, "Core", "MA303" };
    programs[PROGRAM_MNC].semesters[6].courses[1] = (Course){ "CS401", "Deep Learning", 4, "Core", "CS301" };
    programs[PROGRAM_MNC].semesters[6].courses[2] = (Course){ "TE401", "Technical Elective III", 3, "Elective", "Varies" };
    programs[PROGRAM_MNC].semesters[6].courses[3] = (Course){ "TE402", "Technical Elective IV", 3, "Elective", "Varies" };
    programs[PROGRAM_MNC].semesters[6].courses[4] = (Course){ "PR401", "Major Project I", 4, "Project", "PR302" };

    programs[PROGRAM_MNC].semesters[7] = (Semester){ "Semester 8", "Sem 8", .courseCount = 4 };
    programs[PROGRAM_MNC].semesters[7].courses[0] = (Course){ "TE403", "Technical Elective V", 3, "Elective", "Varies" };
    programs[PROGRAM_MNC].semesters[7].courses[1] = (Course){ "OE401", "Open Elective", 3, "Elective", "None" };
    programs[PROGRAM_MNC].semesters[7].courses[2] = (Course){ "PR402", "Major Project II / Internship", 10, "Project", "PR401" };
    programs[PROGRAM_MNC].semesters[7].courses[3] = (Course){ "HS401", "Professional Ethics", 3, "Core", "None" };

    // --- POPULATE (MnC) COURSE DESCRIPTIONS (ALL 15) ---
    // --- ALL TYPOS FIXED HERE ---
    programs[PROGRAM_MNC].descriptionCount = 15;
    programs[PROGRAM_MNC].descriptions[0] = (CourseDesc){"Math, Algorithmic & Comp. Thinking", "Intro to logical reasoning, algorithms, abstraction and problem-solving."};
    programs[PROGRAM_MNC].descriptions[1] = (CourseDesc){"Computer Organization & Programming", "Basics of C programming and fundamentals of computer architecture."};
    programs[PROGRAM_MNC].descriptions[2] = (CourseDesc){"Discrete Mathematics", "Logic, sets, combinatorics, relations, functions, graphs."};
    programs[PROGRAM_MNC].descriptions[3] = (CourseDesc){"Data Structures & Algorithms I", "Lists, stacks, queues, trees; complexity basics."};
    programs[PROGRAM_MNC].descriptions[4] = (CourseDesc){"Linear Algebra", "Matrix theory, vector spaces, eigenvalues, orthogonality."};
    programs[PROGRAM_MNC].descriptions[5] = (CourseDesc){"Probability & Random Processes", "Random variables, distributions, expectation, Markov models."};
    programs[PROGRAM_MNC].descriptions[6] = (CourseDesc){"Operating Systems", "Processes, scheduling, memory mgmt, files, OS design."};
    programs[PROGRAM_MNC].descriptions[7] = (CourseDesc){"Design & Analysis of Algorithms", "Greedy, divide-and-conquer, DP, graph algorithms."};
    programs[PROGRAM_MNC].descriptions[8] = (CourseDesc){"Database Management Systems", "Relational models, SQL, normalization, transactions."};
    programs[PROGRAM_MNC].descriptions[9] = (CourseDesc){"Abstract Algebra", "Groups, rings, fields, algebraic structures and applications."};
    programs[PROGRAM_MNC].descriptions[10] = (CourseDesc){"Machine Learning Foundations", "Regression, classification, overfitting, model evaluation."};
    programs[PROGRAM_MNC].descriptions[11] = (CourseDesc){"Software Engineering", "SDLC, UML, version control, testing, dev-team workflows."};
    programs[PROGRAM_MNC].descriptions[12] = (CourseDesc){"Stochastic Models & Simulation", "Poisson processes, queueing models, Monte-Carlo simulation."};
    programs[PROGRAM_MNC].descriptions[13] = (CourseDesc){"Deep Learning", "Neural nets, CNNs, RNNs, training pipelines, deployment."};
    programs[PROGRAM_MNC].descriptions[14] = (CourseDesc){"Statistical Learning & Data Analysis", "Linear models, PCA, clustering, empirical inference."};
}


/**
 * @brief Handles logic and drawing for the Program Selection screen.
 * This screen allows the user to choose between B.Tech (ICT) and B.Tech (MnC).
 */
void UpdateDrawSelectionScreen(void) {
    //----------------------------------------------------------------------------------
    // UPDATE LOGIC
    //----------------------------------------------------------------------------------
    Vector2 mousePos = GetMousePosition();

    Rectangle ictButton = { screenWidth / 2 - 250, screenHeight / 2 - 100, 500, 80 };
    Rectangle mncButton = { screenWidth / 2 - 250, screenHeight / 2 + 20, 500, 80 };

    if (CheckCollisionPointRec(mousePos, ictButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        activeProgramPtr = &programs[PROGRAM_ICT]; // Set pointer to ICT data
        currentScreen = SCREEN_PROGRAM_STRUCTURE;
        activeTab = 0; // Reset to overview tab
        prevTab = 0; // Reset animation state
        contentAlpha = 0.0f; // Start fade-in
        titleAlpha = 0.0f; // Reset title fade
        scroll = (Vector2){ 0, 0 }; // Reset scroll
        scrollVelocity = 0.0f; // Reset scroll momentum
    }
    
    if (CheckCollisionPointRec(mousePos, mncButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        activeProgramPtr = &programs[PROGRAM_MNC]; // Set pointer to MnC data
        currentScreen = SCREEN_PROGRAM_STRUCTURE;
        activeTab = 0; // Reset to overview tab
        prevTab = 0; // Reset animation state
        contentAlpha = 0.0f; // Start fade-in
        titleAlpha = 0.0f; // Reset title fade
        scroll = (Vector2){ 0, 0 }; // Reset scroll
        scrollVelocity = 0.0f; // Reset scroll momentum
    }

    //----------------------------------------------------------------------------------
    // DRAWING
    //----------------------------------------------------------------------------------
    BeginDrawing();
    
    // --- Draw Gradient Background ---
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, (Color){ 245, 245, 245, 255 }, (Color){ 220, 220, 220, 255 });
    
    DrawText("Select Program Structure", screenWidth/2 - MeasureText("Select Program Structure", 40)/2, 200, 40, TEXT_DARK);

    // Draw buttons with hover effect
    Color ictColor = CheckCollisionPointRec(mousePos, ictButton) ? programs[PROGRAM_ICT].themeColor : LIGHTGRAY;
    Color mncColor = CheckCollisionPointRec(mousePos, mncButton) ? programs[PROGRAM_MNC].themeColor : LIGHTGRAY;
    
    DrawRectangleRec(ictButton, ictColor);
    DrawText(programs[PROGRAM_ICT].name, ictButton.x + (ictButton.width - MeasureText(programs[PROGRAM_ICT].name, 20)) / 2, ictButton.y + 30, 20, TEXT_DARK);
    
    DrawRectangleRec(mncButton, mncColor);
    DrawText(programs[PROGRAM_MNC].name, mncButton.x + (mncButton.width - MeasureText(programs[PROGRAM_MNC].name, 20)) / 2, mncButton.y + 30, 20, TEXT_DARK);

    EndDrawing();
}


/**
 * @brief Handles all logic and drawing for the main Program Structure screen.
 * This function is generic and renders data from the 'activeProgramPtr'.
 * It manages tabs, scrolling physics, animations, and all UI drawing.
 */
void UpdateDrawProgramStructureScreen(void) {
    //----------------------------------------------------------------------------------
    // Safety check
    //----------------------------------------------------------------------------------
    if (activeProgramPtr == NULL) {
        currentScreen = SCREEN_SELECTION;
        return;
    }

    //----------------------------------------------------------------------------------
    // UPDATE LOGIC
    //----------------------------------------------------------------------------------
    Vector2 mousePos = GetMousePosition();
    Rectangle backButton = { 50, 50, 150, 40 };
    bool backButtonHover = CheckCollisionPointRec(mousePos, backButton);

    // Back button logic
    if (backButtonHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentScreen = SCREEN_SELECTION; // Go back to selection
        activeProgramPtr = NULL; // Clear active program pointer
        return; // --- CRITICAL: Stop execution to prevent crash
    }

    // Tab button logic
    Rectangle semesterTabs[MAX_TABS];
    int tabWidth = Clamp((screenWidth - 100) / MAX_TABS - 5, 100, 180);
    int tabHeight = 40;
    int tabSpacing = 5;
    int startX = 50;
    int startY = 150;

    for (int i = 0; i < MAX_TABS; i++) {
        semesterTabs[i] = (Rectangle){ startX + i * (tabWidth + tabSpacing), startY, tabWidth, tabHeight };
        if (CheckCollisionPointRec(mousePos, semesterTabs[i]) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            activeTab = i;
        }
    }
    
    // --- Tab fade-in logic ---
    if (activeTab != prevTab) {
        contentAlpha = 0.0f; // Start fade
        prevTab = activeTab;
        scroll = (Vector2){ 0, 0 }; // Reset scroll
        scrollVelocity = 0.0f; // Reset momentum
        memset(hoverAlphas, 0, sizeof(hoverAlphas)); // Reset all hover alphas
    }
    contentAlpha = Lerp(contentAlpha, 1.0f, 0.12f); // Faster fade
    
    // --- Title fade-in logic ---
    if (titleAlpha < 1.0f) titleAlpha = Lerp(titleAlpha, 1.0f, 0.02f);


    Rectangle contentBox = { startX, startY + tabHeight + tabSpacing, screenWidth - (2 * startX), screenHeight - startY - tabHeight - 70 };
    int lineSpacing = 35; // Reverted to 35

    int contentStartY = contentBox.y + 20 + scroll.y;

    // --- Calculate Scroll Bounds ---
    maxScroll = 0; // Default: no scroll
    
    if (activeTab == 0) { // Overview Tab
        maxScroll = contentBox.height - 500; 
    } else if (activeTab >= 1 && activeTab <= 8) { // Semester Tabs
        Semester *activeSem = &activeProgramPtr->semesters[activeTab - 1];
        int contentHeight = (activeSem->courseCount + 5) * lineSpacing; 
        maxScroll = contentBox.height - contentHeight;
    } else if (activeTab == 9) { // Descriptions Tab
        // --- Reverted to static guess to fix compile error ---
        int contentHeight = (activeProgramPtr->descriptionCount * 4) * lineSpacing; // 4 lines per desc
        maxScroll = contentBox.height - contentHeight;
    }
    
    if (maxScroll > 0) maxScroll = 0; // Don't scroll if content is smaller than box

    // --- Scroll Momentum Logic ---
    if (CheckCollisionPointRec(mousePos, contentBox)) {
        scrollVelocity += GetMouseWheelMove() * 40; // Add velocity
    }
    scroll.y += scrollVelocity * GetFrameTime() * 60.0f; // Apply velocity (frame-independent)
    scrollVelocity *= 0.90f; // Apply friction
    if (fabsf(scrollVelocity) < 0.1f) scrollVelocity = 0.0f; // Stop when slow

    // Clamp scroll
    if (scroll.y < maxScroll) {
        scroll.y = maxScroll;
        scrollVelocity = 0.0f;
    }
    if (scroll.y > 0) {
        scroll.y = 0;
        scrollVelocity = 0.0f;
    }
    
    // --- Update Row Hover Alphas (Semester Tab Only) ---
    if (activeTab >= 1 && activeTab <= 8) {
        Semester *activeSem = &activeProgramPtr->semesters[activeTab - 1];
        int y = contentStartY + (3 * lineSpacing); // Starting Y of the first course row
        
        for (int i = 0; i < activeSem->courseCount; i++) {
            Rectangle courseRow = { contentBox.x, (float)y - 5, contentBox.width, (float)lineSpacing - 5 };
            
            float targetAlpha = (CheckCollisionPointRec(mousePos, courseRow) && contentAlpha > 0.9f) ? 1.0f : 0.0f;
            hoverAlphas[i] = Lerp(hoverAlphas[i], targetAlpha, 0.1f); // Smooth fade
            
            y += lineSpacing;
        }
    }


    //----------------------------------------------------------------------------------
    // DRAWING
    //----------------------------------------------------------------------------------
    BeginDrawing();
    
    // --- Draw Themed Gradient Background ---
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, activeProgramPtr->themeColor, WHITE);

    // --- Draw Visual Depth Circles ---
    for (int i = 0; i < 5; i++) {
        DrawCircleGradient(300 + i * 400, 250 + (i % 2) * 200, 250, 
                           Fade(activeProgramPtr->themeColor, 0.08f), 
                           Fade(WHITE, 0.0f));
    }

    // --- Animated Back Button ---
    Color backColor = backButtonHover ? SKYBLUE : LIGHTGRAY;
    float backTextSize = backButtonHover ? 22 : 20;
    DrawRectangleRec(backButton, backColor);
    DrawText("< Back", backButton.x + (backButtonHover ? 35 : 40), backButton.y + (backButtonHover ? 9 : 10), backTextSize, TEXT_DARK);


    // --- Draw Fading Title ---
    DrawText(activeProgramPtr->title, screenWidth/2 - MeasureText(activeProgramPtr->title, 40)/2, 80, 40, Fade(TEXT_DARK, titleAlpha));
    
    // --- Theme Accent Strip ---
    DrawRectangleGradientH(0, 130, screenWidth / 2, 4, Fade(activeProgramPtr->themeColor, 0.0f), activeProgramPtr->themeColor);
    DrawRectangleGradientH(screenWidth / 2, 130, screenWidth / 2, 4, activeProgramPtr->themeColor, Fade(activeProgramPtr->themeColor, 0.0f));


    // Draw Semester Tabs
    for (int i = 0; i < MAX_TABS; i++) {
        Color tabColor = LIGHTGRAY;
        if (CheckCollisionPointRec(mousePos, semesterTabs[i])) {
            tabColor = SKYBLUE; // Hovered tab
        }
        
        DrawRectangleRec(semesterTabs[i], tabColor);
        DrawRectangleLinesEx(semesterTabs[i], 1, GRAY);
        DrawText(activeProgramPtr->tabNames[i], semesterTabs[i].x + (tabWidth - MeasureText(activeProgramPtr->tabNames[i], 20))/2, semesterTabs[i].y + 10, 20, TEXT_DARK);
        
        // --- Active Tab Underline ---
        if (i == activeTab) {
            DrawRectangle(semesterTabs[i].x, semesterTabs[i].y + tabHeight - 4, tabWidth, 4, BLUE);
        }
    }

    // Draw Content Area Box
    DrawRectangleRec(contentBox, WHITE);
    DrawRectangleLinesEx(contentBox, 2, LIGHTGRAY);
    
    // --- Header Shadow ---
    if (scroll.y < 0) {
        DrawRectangleGradientV(contentBox.x, contentBox.y, contentBox.width, 10, Fade(BLACK, 0.15f), Fade(BLACK, 0.0f));
    }
    
    // --- Draw Scrollbar ---
    if (maxScroll < 0) { 
        float scrollRatio = fabsf(scroll.y) / fabsf(maxScroll);
        float barHeight = Clamp(contentBox.height * 0.2f, 30, contentBox.height - 20);
        float barY = contentBox.y + scrollRatio * (contentBox.height - barHeight);
        
        DrawRectangle(contentBox.x + contentBox.width - 12, contentBox.y, 8, contentBox.height, Fade(GRAY, 0.2f));
        DrawRectangle(contentBox.x + contentBox.width - 12, barY, 8, barHeight, BLUE);
    }
    
    
    BeginScissorMode(contentBox.x, contentBox.y, contentBox.width - 15, contentBox.height); // -15 to not draw over scrollbar
    
    // --- Begin Alpha Blend for Fade-in ---
    BeginBlendMode(BLEND_ALPHA);

    int contentStartX = contentBox.x + 20;

    // --- Draw content based on activeTab ---
    if (activeTab == 0) { // Overview Tab
        int y = contentStartY;
        DrawText("Program Overview", contentStartX, y, 30, Fade(TEXT_DARK, contentAlpha));
        y += lineSpacing + 10;
        if (activeProgramPtr == &programs[PROGRAM_ICT]) {
            DrawText("This University offers a four-year undergraduate program in Information and Communication Technology (ICT) leading to:", contentStartX, y, 20, Fade(TEXT_LIGHT, contentAlpha));
            y += lineSpacing;
            DrawText("- B.Tech. (ICT)", contentStartX + 20, y, 20, Fade(TEXT_LIGHT, contentAlpha));
            y += lineSpacing;
            DrawText("- B.Tech. (Honours) in ICT", contentStartX + 20, y, 20, Fade(TEXT_LIGHT, contentAlpha));
            y += lineSpacing;
            DrawText("- B.Tech. (Honours) in ICT with minor in Robotics and Autonomous Systems", contentStartX + 20, y, 20, Fade(TEXT_LIGHT, contentAlpha));
        } else if (activeProgramPtr == &programs[PROGRAM_MNC]) {
            DrawText("B.Tech (Mathematics and Computing) is a rigorous program focusing on the intersection", contentStartX, y, 20, Fade(TEXT_LIGHT, contentAlpha));
            y += lineSpacing;
            DrawText("of advanced mathematics, algorithmic thinking, and high-performance computing.", contentStartX, y, 20, Fade(TEXT_LIGHT, contentAlpha));
            y += lineSpacing;
            DrawText("Core topics include Abstract Algebra, Machine Learning, Stochastic Models, and Deep Learning.", contentStartX + 20, y, 20, Fade(TEXT_LIGHT, contentAlpha));
        }
        
    } else if (activeTab >= 1 && activeTab <= 8) { // Semester Tabs
        
        Semester *activeSem = &activeProgramPtr->semesters[activeTab - 1];
        int totalCredits = 0; 
        
        // Draw Header
        int y = contentStartY;
        DrawText(activeSem->name, contentStartX, y, 30, Fade(TEXT_DARK, contentAlpha));
        y += lineSpacing + 10;
        DrawText("Code", contentStartX, y, 20, Fade(TEXT_DARK, contentAlpha));
        DrawText("Course Name", contentStartX + 150, y, 20, Fade(TEXT_DARK, contentAlpha));
        DrawText("Credits", contentStartX + 600, y, 20, Fade(TEXT_DARK, contentAlpha));
        DrawText("Type", contentStartX + 750, y, 20, Fade(TEXT_DARK, contentAlpha));
        DrawText("Prerequisites", contentStartX + 900, y, 20, Fade(TEXT_DARK, contentAlpha));
        y += lineSpacing * 0.7;
        DrawLine(contentStartX, y, contentBox.x + contentBox.width - 40, y, Fade(GRAY, contentAlpha));

        for (int i = 0; i < activeSem->courseCount; i++) {
            y += lineSpacing;
            Course* course = &activeSem->courses[i];
            totalCredits += course->credits;

            // --- Draw Smooth Hover Row ---
            Rectangle courseRow = { contentBox.x, (float)y - 5, contentBox.width, (float)lineSpacing - 5 };
            DrawRectangleRec(courseRow, Fade(SKYBLUE, 0.2f * hoverAlphas[i])); // Use alpha

            // --- Color-Coded Types ---
            Color typeColor = TEXT_LIGHT;
            if (strcmp(course->type, "Core") == 0) typeColor = MAROON;
            else if (strcmp(course->type, "Lab") == 0) typeColor = ORANGE;
            else if (strcmp(course->type, "Elective") == 0) typeColor = DARKGREEN;
            else if (strcmp(course->type, "Project") == 0) typeColor = PURPLE;
            
            DrawText(course->code, contentStartX, y, 20, Fade(TEXT_LIGHT, contentAlpha));
            
            // --- Text Truncation ---
            DrawText(TextFormat("%.35s%s", course->name, (strlen(course->name) > 35) ? "..." : ""), contentStartX + 150, y, 20, Fade(TEXT_LIGHT, contentAlpha));
            
            DrawText(TextFormat("%d", course->credits), contentStartX + 600, y, 20, Fade(TEXT_LIGHT, contentAlpha));
            DrawText(course->type, contentStartX + 750, y, 20, Fade(typeColor, contentAlpha));
            DrawText(course->prereq, contentStartX + 900, y, 20, Fade(TEXT_LIGHT, contentAlpha));
        }
        
        // --- Draw Total Credits ---
        y += lineSpacing * 1.5;
        DrawLine(contentStartX, y, contentBox.x + contentBox.width - 40, y, Fade(GRAY, contentAlpha));
        y += lineSpacing * 0.5;
        DrawText(TextFormat("Total Semester Credits: %d", totalCredits), contentStartX, y, 20, Fade(TEXT_DARK, contentAlpha));

    } else if (activeTab == 9) { // Descriptions Tab
        
        // --- REVERTED TO DrawText TO FIX COMPILE ERROR ---
        // This will not word-wrap.
        
        int y = contentStartY;
        DrawText("Key Course Descriptions", contentStartX, y, 30, Fade(TEXT_DARK, contentAlpha));
        y += lineSpacing + 10;

        for (int i = 0; i < activeProgramPtr->descriptionCount; i++) {
            y += lineSpacing;
            DrawText(activeProgramPtr->descriptions[i].name, contentStartX, y, 20, Fade(DARKBLUE, contentAlpha));
            y += lineSpacing;
            DrawText(activeProgramPtr->descriptions[i].description, contentStartX + 20, y, 20, Fade(TEXT_LIGHT, contentAlpha));
            y += lineSpacing;
        }
    }
    
    // --- End Alpha Blend ---
    EndBlendMode();
    
    EndScissorMode();
    // --- End Scissor Mode ---

    EndDrawing();
}