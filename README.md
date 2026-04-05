# **University Portal (eCampus) for College.**
### **--> Table of Contents.**

* Introduction
* Features
* Project overview 
* How to Run?
* Instruction to compile
* Credits:

## Introduction

Our project is a simple c based University Portal for windows. This c language project is made with GUI and the library used for the same is Raylib which is free to use and easier compared to other GUI based libraries. 
This project is made by section A group 3.

## Features

1. Seperate login system for students and faculties.![Login page ss](<Media/Screenshot 2026-04-05 215626.png>)

2. For different operations have different .exe to make your flow simple.

3. University Portal has Students and Faculty profile, Academics (individual), Program Structure (ICT & MnC), Announcements (for events too), Administrative Window (for admins and students).![profile page ss](<Media/Screenshot 2026-04-05 215727.png>) ![academics ss](<Media/Screenshot 2026-04-05 215739.png>) ![program structure ss](<Media/Screenshot 2026-04-05 215809.png>) ![announcement page ss](<Media/Screenshot 2026-04-05 215826.png>) ![admin window ss](<Media/Screenshot 2026-04-05 215844.png>)

4. All the data will saved in files. Different type of data will stored in different files.

5. Saved data is easy to edit by faculties or admins (in .exe and file too.)

6. Can't run any exe directly to make things secure.

## Project Overview

As mentioned above, there are individual c code and exe files, and each component is made by one or two students and later linked togehter. We started from zero knowledge about Raylib. So we made some backend ideas about features to include, divided the parts, made raw code from AI, understood the raylib library and some other parts like how AI defines structures for different purposes, made some changes in the AI generated code, connected everything from file to exe with each other.

We also faced some minor and some major problems while using AI generated code like linking, taking details from files that has been already created by other code, making two different code from one example, AI made a single file for academics that allows everyone to alter the data that should be only altered by authorized users.To solve this we prgorammed a similar code that allows only authorized users to view and alter data, and created a seperate student administration window for the students to only view the detials.

## How to run the full project ?

* All the compressed file must be extracted and saved in a single folder.
* After doing so you will be able to open main.exe file by double clicking it that will open the University Portal's login page.
* Start by registering yourself as student or faculty.
* You will not be able to directly open any other exe files, as all the files are linked together and can only be opened in a specific flow. You can start main from mingw64 compiler as shown below.

```
$cd <Your folder path>
$./main

```

## How to compile ?

* Once all the .c files are installed, the program can be changed using any code editor provided the program is then compiled in mingw64 compiler to implement those changes in the exe files.
For examples, if you have made any changes in the program structure file, you must compile "program_structure.c" in mingw64 as shown below after saving it. 
``` 
$gcc program_structure.c -o prostr -lraylib -lgdi32 -lwinmm
```
* Remember you must save your output exe file with old exe name otherwise code will open old one or give error of cannot find prostr.exe .

## Credits:

* Our valuable team did very good job to complete this project successfully and got to learn about a new GUI library and enjoyed the work.
As the leader Vishvdeepsinh Jadeja i got the opportunity to make it opensource for upcoming students.

This is everything about our project hope you'll enjoy.
