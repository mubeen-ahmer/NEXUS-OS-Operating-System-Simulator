#include <iostream>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sys/wait.h>
#include <pthread.h>
#include "config.h"
#include "kernel.h"
#include "resource.h"
#include "process.h"
#include "scheduler.h"
#include "ready_queue.h"
using namespace std;
void gotoxy(int x, int y);
void welcomeScreen();
void goodbye();
void kernelModeMenu();


volatile bool osRunning = true;

void* reaperThread(void* arg) {
    while (osRunning) {
        pthread_mutex_lock(&gMutex);
        checkAndCleanProcesses();
        pthread_mutex_unlock(&gMutex);
        sleep(1);
    }
    return nullptr;
}

void launchTask(const char* name, const char* execPath, int ramAmount, TASK_LEVEL level = FOREGROUND, int priority = 0) {
    int p1fd[2];// pipe1 : parent writes, child reads
    int p2fd[2];// pipe2 : child writes, parent reads

    if (pipe(p1fd) == -1 || pipe(p2fd) == -1) {
        perror("pipe");
        return;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(p1fd[0]); close(p1fd[1]);
        close(p2fd[0]); close(p2fd[1]);
        return;
    }
    if (pid == 0) { // child
        close(p1fd[1]);
        close(p2fd[0]);

        string message = "REQUEST " + to_string(ramAmount);
        const char* msg = message.c_str();
        write(p2fd[1], msg, strlen(msg));
        close(p2fd[1]); 

        char buffer[100];
        int n = read(p1fd[0], buffer, sizeof(buffer));
        buffer[n] = '\0';
        close(p1fd[0]); 

        if (strcmp(buffer, "GRANT") == 0) {
            execlp("xterm", "xterm", "-fa", "Monospace", "-fs", "11", "-e", execPath, NULL);
            // execlp("xterm", "xterm", "-e", execPath, NULL);
            perror("execlp: xterm failed"); // only reached if execlp fails
        }
        exit(1);
    }
    else { // parent
        close(p1fd[0]);
        close(p2fd[1]);

        char buffer[100];
        int n = read(p2fd[0], buffer, sizeof(buffer));
        buffer[n] = '\0';
        close(p2fd[0]); 

        string received(buffer);

        const char* permission;
        string msg;

        PCB* pcb = nullptr;
        int requestedRam = stoi(received.substr(8));

        pthread_mutex_lock(&gMutex); // FIX: lock before touching resources + PCB

        if (allocateRam(requestedRam) != 0) {
            msg = "DENY";
            permission = msg.c_str();
            write(p1fd[1], permission, strlen(permission));
            close(p1fd[1]); 
            printStatus();
            cin.ignore();
            cin.get();
        }
        else {
            msg = "GRANT";
            permission = msg.c_str();
            write(p1fd[1], permission, strlen(permission));
            close(p1fd[1]); 
            pcb = new PCB(pid, getpid(), name, Ready, ramAmount, 0, level, priority);
            addProcess(pcb);
            kill(pid, SIGSTOP);   // Pause immediately! It must wait for a Core.
            scheduleProcess(pcb); // Hand to scheduler queue
        }

        pthread_mutex_unlock(&gMutex); // FIX: unlock after done
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cout << "Usage: ./OS <RAM_MB> <HDD_GB> <Cores>" << endl;
        return 1;
    }
    int ram  = atoi(argv[1]);
    int hdd  = atoi(argv[2]);
    int core = atoi(argv[3]);
    if (initialize(ram, hdd, core) != 0) {
        cout << "Invalid arguments — values must be positive" << endl;
        return 1;
    }

    welcomeScreen();

    // Start background threads
    initScheduler(core);
    pthread_t reaperTid;
    pthread_create(&reaperTid, nullptr, reaperThread, nullptr);
    pthread_detach(reaperTid);
    
    // Spin up exactly as many Scheduler Threads as we have CPU Cores!
    pthread_t schedTid[core];
    for (int i = 0; i < core; i++) {
        pthread_create(&schedTid[i], nullptr, schedulerThread, nullptr);
        pthread_detach(schedTid[i]);
    }   

    int choice;
    do {
        system("clear");
        printStatus();
        cout << "\n===== NexusOS =====" << endl;
        cout << "\n[Level 0: FOREGROUND - Round Robin]" << endl;
        cout << " 1. Calculator         [RAM: " << CALC_RAM           << " MB]" << endl;
        cout << " 2. Notepad            [RAM: " << NOTEPAD_RAM        << " MB]" << endl;
        cout << "11. Word Count         [RAM: " << WORD_COUNT_RAM     << " MB]" << endl;
        cout << "12. Mine Sweeper       [RAM: " << MINESWEEPER_RAM    << " MB]" << endl;
        cout << "16. Unit Convertor     [RAM: " << UNIT_CONVERTOR_RAM << " MB]" << endl;
        cout << "15. Coin Flip          [RAM: " << COIN_FLIP_RAM     << "  MB]" << endl;

        cout << "\n[Level 1: BACKGROUND - FCFS]" << endl;
        cout << "13. Music Sim          [RAM: " << MUSIC_SIM_RAM      << " MB]" << endl;
        cout << "14. Print Sim          [RAM: " << PRINT_SIM_RAM      << " MB]" << endl;
        cout << " 5. File Copy          [RAM: " << FILE_COPY_RAM      << " MB]" << endl;
        cout << " 6. File Move          [RAM: " << FILE_MOVE_RAM      << " MB]" << endl;
        cout << " 7. File Delete        [RAM: " << FILE_DELETE_RAM    << " MB]" << endl;

        cout << "\n[Level 2: SYSTEM - Priority]" << endl;
        cout << " 3. Clock              [Pri:1 | RAM: " << CLOCK_RAM          << " MB]" << endl;
        cout << " 4. Calendar           [Pri:1 | RAM: " << CALENDAR_RAM       << " MB]" << endl;
        cout << " 8. File Create        [Pri:2 | RAM: " << FILE_CREATE_RAM    << " MB]" << endl;
        cout << " 9. File Info          [Pri:2 | RAM: " << FILE_INFO_RAM      << " MB]" << endl;
        cout << "19. Timer/Alarm        [Pri:2 | RAM: " << TIMER_RAM          << " MB]" << endl;
        cout << "10. Random Number      [Pri:3 | RAM: " << RAND_GEN_RAM       << " MB]" << endl;
        cout << "17. Log Generator      [Pri:3 | RAM: " << LOG_GEN_RAM        << " MB]" << endl;
        cout << "18. Log Viewer         [Pri:3 | RAM: " << LOG_VIEWER_RAM     << " MB]" << endl;
        cout << "20. Password Generator [Pri:3 | RAM: " << PASSWORD_GENERATOR_RAM << " MB]" << endl;

        cout << "\n[ADMIN]" << endl;
        cout << "21. *** KERNEL MODE ***" << endl;
        cout << " 0. Shutdown" << endl;
        
        cout << "\nSelect: ";
        cin >> choice;

        switch (choice) {
            // Foreground/Interactive — Level 0, Round Robin
            case  1: launchTask("calculator",  "tasks/calculator",  CALC_RAM,      FOREGROUND); break;
            case  2: launchTask("notepad",      "tasks/notepad",      NOTEPAD_RAM,   FOREGROUND); break;
            case 11: launchTask("wordCount",    "tasks/wordCount",    WORD_COUNT_RAM,FOREGROUND); break;
            case 12: launchTask("mineSweeper",  "tasks/mineSweeper",  MINESWEEPER_RAM,FOREGROUND); break;
            case 16: launchTask("unitConvertor",   "tasks/unitConvertor",   UNIT_CONVERTOR_RAM,FOREGROUND); break;
            case 15: launchTask("coinFlip",    "tasks/coinFlip",      COIN_FLIP_RAM, FOREGROUND); break;
            // Background/Auto-finish — Level 1, FCFS
            case 13: launchTask("musicSim",     "tasks/musicSim",     MUSIC_SIM_RAM, BACKGROUND); break;
            case 14: launchTask("printSim",     "tasks/printSim",     PRINT_SIM_RAM, BACKGROUND); break;
            case  5: launchTask("fileCopy",     "tasks/fileCopy",     FILE_COPY_RAM, BACKGROUND); break;
            case  6: launchTask("fileMove",     "tasks/fileMove",     FILE_MOVE_RAM, BACKGROUND); break;
            case  7: launchTask("fileDelete",   "tasks/fileDelete",   FILE_DELETE_RAM,BACKGROUND); break;
            // System/Utility — Level 2, Priority (lower number = runs first)
            case  3: launchTask("clock",        "tasks/clock",        CLOCK_RAM,     SYSTEM, 1); break;
            case  4: launchTask("calender",     "tasks/calender",     CALENDAR_RAM,  SYSTEM, 1); break;
            case  8: launchTask("fileCreate",   "tasks/fileCreate",   FILE_CREATE_RAM,SYSTEM,2); break;
            case  9: launchTask("fileInfo",     "tasks/fileInfo",     FILE_INFO_RAM,  SYSTEM,2); break;
            case 19: launchTask("timer",        "tasks/timer",        TIMER_RAM,      SYSTEM,2); break;
            case 10: launchTask("randomNumGen", "tasks/randomNumGen", RAND_GEN_RAM,   SYSTEM,3); break;
            case 17: launchTask("logGenerator", "tasks/logGenerator", LOG_GEN_RAM,    SYSTEM,3); break;
            case 18: launchTask("logViewer",    "tasks/logViewer",    LOG_VIEWER_RAM, SYSTEM,3); break;
            case 20: launchTask("passwordGenerator","tasks/passwordGenerator",PASSWORD_GENERATOR_RAM,SYSTEM,3); break;
            // Kernel Mode
            case 21: kernelModeMenu(); break;
            case  0: goodbye(); break;
            default:
                cout << "Invalid choice" << endl;
                sleep(1);
        }

    } while (choice != 0);

    osRunning = false;
    pthread_mutex_lock(&gMutex);
    killAllProcesses();
    pthread_mutex_unlock(&gMutex);
    sleep(1);
    return 0;
}

// ─── Kernel Mode Menu ─────────────────────────────────────────────────────────
void kernelModeMenu() {
    int kChoice;
    do {
        system("clear");
        gotoxy(0,10);
        cout << "╔══════════════════════════════╗" << endl;
        cout << "║       *** KERNEL MODE ***    ║" << endl;
        cout << "╚══════════════════════════════╝" << endl;
        printStatus();
        cout << endl;
        cout << "1. List all processes"    << endl;
        cout << "2. Kill process  (SIGKILL)" << endl;
        cout << "3. Pause process (SIGSTOP)" << endl;
        cout << "4. Resume process(SIGCONT)" << endl;
        cout << "5. Reset — kill all & free resources" << endl;
        cout << "0. Back to User Mode" << endl;
        cout << "\nKernel> ";
        cin >> kChoice;

        int pid;
        switch (kChoice) {
            case 1:
                system("clear");
                pthread_mutex_lock(&gMutex);
                gotoxy(0,10);
                listProcess();
                pthread_mutex_unlock(&gMutex);
                cout << "\nPress ENTER to continue...";
                cin.ignore(); cin.get();
                break;
            case 2:
                cout << "Enter PID to kill: ";
                cin >> pid;
                pthread_mutex_lock(&gMutex);
                killProcess(pid);
                pthread_mutex_unlock(&gMutex);
                sleep(1);
                break;
            case 3:
                cout << "Enter PID to pause: ";
                cin >> pid;
                pthread_mutex_lock(&gMutex);
                stopProcess(pid);
                pthread_mutex_unlock(&gMutex);
                sleep(1);
                break;
            case 4:
                cout << "Enter PID to resume: ";
                cin >> pid;
                pthread_mutex_lock(&gMutex);
                resumeProcess(pid);
                pthread_mutex_unlock(&gMutex);
                sleep(1);
                break;
            case 5:
                pthread_mutex_lock(&gMutex);
                killAllProcesses();
                pthread_mutex_unlock(&gMutex);
                reprintStatusInPlace();
                sleep(1);
                break;
            case 0:
                break;
            default:
                cout << "Invalid." << endl;
                sleep(1);
        }
    } while (kChoice != 0);
}

// a function copied from internet, it move the cursor at x, y cordinate on console
void gotoxy(int x, int y) {
    cout << "\033[" << y << ";" << x << "H";
}

void welcomeScreen() {
    system("clear");
    cout << "\n\n";
    cout << "    _   _                        ___  ____  " << endl;
    cout << "   | \\ | | _____  ___   _ ___   / _ \\/ ___| " << endl;
    cout << "   |  \\| |/ _ \\ \\/ / | | / __| | | | \\___ \\ " << endl;
    cout << "   | |\\  |  __/>  <| |_| \\__ \\ | |_| |___) |" << endl;
    cout << "   |_| \\_|\\___/_/\\_\\\\__,_|___/  \\___/|____/ " << endl;
    cout << "                                            " << endl;
    cout << "          v1.0 - Initializing Kernel...     " << endl;
    cout << "\n\n   [";
    cout.flush();
    
    // Simulate loading bar
    for (int i = 0; i < 30; i++) {
        cout << "#";
        cout.flush();
        usleep(50000); // 50ms delay per block (1.5 seconds total)
    }
    cout << "] 100%\n" << endl;
    usleep(1000000); // half second pause before menu
    system("clear");
}

void goodbye() {
    system("clear");
    gotoxy(29, 12);
    cout << "Shutting down..." << endl;
    sleep(1);
    system("clear");
}