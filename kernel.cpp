#include <iostream>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include "resource.h"
#include "process.h"
#include "kernel.h"
#include "scheduler.h"
using namespace std;

pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;

struct PCBNode {
    PCB*     pcb;
    PCBNode* next;
    PCBNode(PCB* p) : pcb(p), next(nullptr) {}
};

static PCBNode* head = nullptr;

void addProcess(PCB* pcb) {
    PCBNode* node = new PCBNode(pcb);
    if (!head) { head = node; return; }
    PCBNode* temp = head;
    while (temp->next) temp = temp->next;
    temp->next = node;
}

void removeProcess(PCB* pcb) {
    if (!head) return;
    PCBNode* temp = head;
    PCBNode* prev = nullptr;
    while (temp) {
        if (temp->pcb->pid == pcb->pid) {
            if (!prev) head = temp->next;
            else       prev->next = temp->next;
            delete temp->pcb;  
            delete temp;      
            return;
        }
        prev = temp;
        temp = temp->next;
    }
}

void listProcess() {
    if (!head) { cout << "No processes running" << endl; return; }
    const char* stateNames[] = { "Ready", "Running", "Blocked", "Terminated" };
    const char* levelNames[] = { "Foreground(RR)", "Background(FCFS)", "System(Priority)" };
    PCBNode* temp = head;
    while (temp) {
        cout << "PID:       " << temp->pcb->pid                          << endl;
        cout << "PPID:      " << temp->pcb->ppid                         << endl;
        cout << "Name:      " << temp->pcb->name                         << endl;
        cout << "State:     " << stateNames[temp->pcb->state]            << endl;
        cout << "Level:     " << levelNames[temp->pcb->taskLevel]        << endl;
        cout << "Priority:  " << temp->pcb->priority                     << endl;
        cout << "RAM:       " << temp->pcb->requiredRAM   << " MB"       << endl;
        cout << "HDD:       " << temp->pcb->requiredHardDisk << " MB"   << endl;
        cout << "------------------------" << endl;
        temp = temp->next;
    }
}


void killProcess(int pid) {
    PCBNode* temp = head;
    while (temp) {
        if (temp->pcb->pid == pid) {
            kill(pid, SIGKILL);
            temp->pcb->state = Terminated;
            cout << "Process " << pid << " killed." << endl;
            return;
        }
        temp = temp->next;
    }
    cout << "PID " << pid << " not found." << endl;
}

void stopProcess(int pid) {
    PCBNode* temp = head;
    while (temp) {
        if (temp->pcb->pid == pid) {
            kill(pid, SIGSTOP);
            temp->pcb->state = Blocked;
            cout << "Process " << pid << " paused (minimized)." << endl;
            return;
        }
        temp = temp->next;
    }
    cout << "PID " << pid << " not found." << endl;
}

void resumeProcess(int pid) {
    PCBNode* temp = head;
    while (temp) {
        if (temp->pcb->pid == pid) {
            if (temp->pcb->state == Blocked) {
                temp->pcb->state = Ready;
                cout << "Process " << pid << " resumed (Moved to Ready Queue)." << endl;
                scheduleProcess(temp->pcb); 
            } else {
                cout << "Process " << pid << " is not blocked." << endl;
            }
            return;
        }
        temp = temp->next;
    }
    cout << "PID " << pid << " not found." << endl;
}

void killAllProcesses() {
    PCBNode* temp = head;
    while (temp) {
        kill(temp->pcb->pid, SIGKILL);
        freeRam(temp->pcb->requiredRAM);
        PCBNode* next = temp->next;
        delete temp->pcb;
        delete temp;
        temp = next;
    }
    head = nullptr;
    availableCores = totalCores;
    cout << "All processes terminated. Resources freed." << endl;
}


void checkAndCleanProcesses() {
    int    status;
    pid_t  dead;

    while ((dead = waitpid(-1, &status, WNOHANG)) > 0) {

        PCBNode* temp = head;
        while (temp) {
            if (temp->pcb->pid == dead) {
                freeRam(temp->pcb->requiredRAM);
                removeProcess(temp->pcb);
                reprintStatusInPlace(); 
                break;
            }
            temp = temp->next;
        }
    }
}