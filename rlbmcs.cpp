#include <iostream>
#include <vector> 
#include <queue>
#include <sstream> 
#include <algorithm>
#include <random> 
#include <fstream>
#include <unordered_map>
using namespace std;


struct task
{
    string name;

    int startTime;
    int executionTime;
    int deadline;
    int remainingTime;
    int completionTime;
    int turnAround;
    int waitingTime;

    int assignedPriority; //priority assigned by our agent, needs implementation
    int defaultPriority; //actual priority of process

    bool deadlineMet = false;
    vector<int> curState(); //for use in ML implementation
    void checkDeadline(float clock);
};
struct multiLevelQ //4 queues defined in research article, doubled time quantum to allow integer time value
{
    queue<task*> ready;
    vector<queue<task*>> levels;//1,2,3,4
    multiLevelQ()
    {
        for(int i=0; i < 4; i++)
        {
            queue<task*> temp;
            levels.push_back(temp);
        }
    }
    void checkReady(float c);
    int qSelection();
    bool empty();
    void visualizeSys();
};
struct cpu //accepts given number of cores
{  

    vector<queue<task*>> cores;
    int freeCores;
    int nCores;
    vector<int> totalBusytime;
    vector<int> idleTime;
    vector<float> exitTime;
    vector<task*> completed;

    void initCores();
    void runProcess(int t);
    int coreAvailable(task* t, multiLevelQ *qSys);
    void status(multiLevelQ *qSys, float c);
    cpu(int n)
    {
        this->nCores = n;
        this->freeCores = n;
        initCores();
    }   
};

void multiLevelQ::visualizeSys()
{
    int sum = 0;
    for(int i = 0; i < 4; i++)
    {
        queue<task*> temp(this->levels[i]);
        cout << "\nQueue level " << i << endl;
        while(!temp.empty())
        {
            cout << temp.front()->name << " ";
            temp.pop();
        }
    }
}
void task::checkDeadline(float clock)
{
    if(clock > this->deadline)
    {
        this->deadlineMet = false;
    }
    this->deadlineMet = true;
}
int multiLevelQ::qSelection()
{
    int qToRun = -1; 
    for(int i=3; i >= 0; i--)
    {
        if(this->levels[i].size() > 0) //highest priority nonempty queue
        {
            qToRun = i;
        }
    }
    return qToRun;
}

int cpu::coreAvailable(task *t, multiLevelQ *qSys) //will return available core or signal to preempt
{
    if(this->freeCores > 0)
    {
        for(int i = 0; i < this->nCores; i++)
        {
            if(this->cores[i].size() == 0)
            {
                return i;//return first free core
            }
        }
        cout << "Core count might be messed up\n";
    }
    else //check for preemption
    {
        cout << "Checking for preempt\n";
        for(int i = 0; i < this->nCores; i++)
        {
            //if incoming task is higher priority, need to preempt
            cout << "Current running " << this->cores[i].front()->name << " versus " << t->name << endl;
            if(this->cores[i].front()->defaultPriority > t->defaultPriority) // 1 is highest priority, 4 is the least
            {
                cout << this->cores[i].front()->name << " is being preempted\n";
                qSys->ready.push(this->cores[i].front());
                this->cores[i].pop();
                this->freeCores++;
                return i;
            }
        }
    }

    return -1; //no core available for scheduling, must wait

}


//checking for available processor executing task from highest priority queue
void checkQueues(multiLevelQ *qSys, float &clock, cpu *p)
{
    int qToRun = qSys->qSelection();// -1 is bad
    int available = -2;
    if(qToRun != -1)
    {
        available = p->coreAvailable(qSys->levels[qToRun].front(), qSys);
        cout << "Core: " << available << " available at time " << clock << endl;
        cout << "Highest priority queue " << qToRun << endl;
        qSys->visualizeSys();
    }
    
    //designated time slice for each priority level
    unordered_map<int, int> quantum({
        {0, 6}, {1, 5}, {2, 4}, {3, 3}
    });
    if(qToRun != -1 && available != -1)
    {
        cout << "\nSelected qLevel " << qToRun << " from system below at time: " << clock << endl;

        p->cores[available].push(qSys->levels[qToRun].front()); //pushing to available core
        p->freeCores--;

        p->exitTime[available] = min(clock + quantum[qToRun], clock + qSys->levels[qToRun].front()->remainingTime); //full time quantum may not be needed
        cout <<"\nCurrent time " << clock << ": "<<  qSys->levels[qToRun].front()->name << " should exit cpu at time: " << p->exitTime[available] << endl;

        qSys->levels[qToRun].pop();//remove from queue after pushing to processor
    }
    if(available == -1)//no cores available
    {
        cout << "\tCores busy, execution blocked\n";
    }

}

//once task is ready, send to their priority queue
void multiLevelQ::checkReady(float time)
{
    while(!(this->ready.empty()) && time >= this->ready.front()->startTime)
    {
        int priority = this->ready.front()->defaultPriority; //implement qLearning priority assignment here
        this->levels[priority].push(this->ready.front());
        cout << "\tProcess " << this->ready.front()->name << " sent to queue " << priority << " at time " << time << endl; 
        this->ready.pop();
    }
}
bool multiLevelQ::empty()
{
    for(int i = 0; i < 4; i++)
    {
        if( !this->levels[i].empty() )
        {
            return false;
        }
    }
    return true;
}
vector<int> task::curState() //function for RL environment 
{
    vector<int> temp;
    temp.insert(temp.end(), {this->executionTime, this->remainingTime, this->deadline});

    return temp;
}

void cpu::initCores()
{
    for(int i = 0; i < this->nCores; i++)
    {
        queue<task*> temp;
        this->cores.push_back(temp);
        this->exitTime.push_back(0);
        this->totalBusytime.push_back(0);
        this->idleTime.push_back(0);
    }
}
void cpu::status(multiLevelQ *qSys, float clock)
{
    for(int i = 0; i < this->nCores; i++)
    {
        if(this->cores[i].size() > 0)
        {
            if(clock == this->exitTime[i]) //time slice completed
            {
                if(this->cores[i].front()->remainingTime == 0) //task completed
                {
                    cout <<this->cores[i].front()->name << " completed execution at time " << clock << endl;
                    this->cores[i].front()->checkDeadline(clock);
                    this->cores[i].front()->completionTime = clock;
                    this->cores[i].front()->turnAround = clock - this->cores[i].front()->startTime;
                    this->cores[i].front()->waitingTime = this->cores[i].front()->turnAround - this->cores[i].front()->executionTime;        
                    this->completed.push_back(this->cores[i].front());
                    this->cores[i].pop();
                    this->freeCores++;
                }
                else //need to send back to queue
                {
                    cout << this->cores[i].front()->name << " completed time slice and going back to queue at time " << clock <<
                        " with remaining time of " << this->cores[i].front()->remainingTime <<  endl;
                    qSys->levels[this->cores[i].front()->defaultPriority].push(this->cores[i].front());
                    this->cores[i].pop();
                    this->freeCores++;
                }
            }
            else
            {
                this->totalBusytime[i]++;
                this->cores[i].front()->remainingTime--;
            }
            

        }
        else
        {
            this->idleTime[i]++;   
        }
    }
}

/*standard implementation of random number generation*/
int randomGen(int lowerBound, int upperBound)
{
    random_device rd; 
    mt19937 gen(rd()); 
    uniform_int_distribution<> distr(lowerBound, upperBound); //range inclusive

    return distr(gen);
}

/*receives line from autogenerated task list and returns object with desired data*/
task *inputData(string line)
{
    task *temp = new task;
    replace(line.begin(), line.end(), ',', ' ');
    
    string Name,Jitter,BCET,WCET,Period,Deadline,PE;
    stringstream inSS(line);

    while(inSS >> Name >> Jitter >> BCET >> WCET >> Period >> Deadline >> PE) //Jitter, BCET, PE are not used in this implementation
    {
        temp->name = Name;
        temp->executionTime = stoi(WCET);
        temp->remainingTime = temp->executionTime;
        temp->deadline = stoi(Deadline);
        temp->defaultPriority = randomGen(0,3); //randomGenerated priority because task generation doesnt provide   
        temp->startTime = randomGen(1, temp->deadline - temp->executionTime); //randomGenerated start because task generation doesnt provide
    }
    return temp;
}
void readFile(vector<task*> &list, char* argv) //reading lines and inserting taskings into list
{
    string taskData;
    task *temp;
    fstream inFS(argv);

    while(inFS >> taskData)
    {
        if(taskData != "Name,Jitter,BCET,WCET,Period,Deadline,PE")
        {
            temp = inputData(taskData);
            list.push_back(temp );
        }
    }
}
void printTaskInfo(vector<task*> taskList)
{
    for(int i = 0; i < taskList.size(); i++)
    {
        cout << taskList[i]->name << " " <<taskList[i]->executionTime << " " <<taskList[i]->deadline << " " << taskList[i]->defaultPriority << " " << endl;
    }
}
void checkMultiLevelQueue()
{
    for(int i =0; i < 4; i++) //needs implementation, each queue should also be using it's own scheduling algorithm(edf, ljf, llf, etc..)
    {
        switch (i)
        {
        case 0:
            
            break;
        case 1:
            
            break;
        case 2:
            
            break;
        case 3:
            
            break;
        default:
            break;
        }
    }
}


void initiateScheduling(vector<task*> &tasks, multiLevelQ *queueSystem, cpu *p, float &clock)
{
    int nStarts = 0;
    while(nStarts < tasks.size())
    {
        for(int i = 0; i < tasks.size(); i++)
        {
            if(tasks[i]->startTime == clock)
            {
                cout << "\nAt time: " << clock << " pushing " << tasks[i]->name << " to ready queue with deadline: " << tasks[i]->deadline << endl;
                queueSystem->ready.push(tasks[i]);
                nStarts++;
            }
        }
        queueSystem->checkReady(clock);
        checkQueues(queueSystem, clock, p);
        p->status(queueSystem, clock);
        clock++;
    }
}
void printStatistics(vector<task*> &tasks, cpu *p)
{
    cout << "-----------Statistics-----------\n";
    double avgTAT = 0;
    double avgWait = 0;
    int deadLinesMet= 0;
    for(int i = 0; i < tasks.size(); i++)
    {
        avgTAT += (double)tasks[i]->turnAround;
        avgWait += (double)tasks[i]->waitingTime;
        deadLinesMet += tasks[i]->deadlineMet;
    }
    avgTAT = avgTAT / p->completed.size();
    avgWait = avgWait / p->completed.size();

    cout << "Average TAT: " << avgTAT << endl;
    cout << "Average waiting time: " << avgWait << endl;
    cout << "Number of Deadlines missed: " << p->completed.size() - deadLinesMet << endl;

    double idle, busy = 0;

    for(int i = 0; i < p->nCores; i++)
    {
        idle += p->idleTime[i];
        busy += p->totalBusytime[i];
    }

    cout << "Utilization: " << busy / (idle + busy) << endl;
    cout << "------------------------------\n";
}
int main(int argc, char* argv[]) //inputFile = argv[1], number of cores = argv[2] 
{
    if(argc < 3 )
    {
        cout << "Missing one or more required arguments\n";
        cout << "Run program as so: ./a.out <filename> <number of cores>\n";
        return 0;
    }

    float clock = 0;
    int numCores = stoi(argv[2]); //from command line argument
    vector<task*> allTasks;
    
    multiLevelQ *queueSystem = new multiLevelQ();
    cpu *processor = new cpu(numCores);

    readFile(allTasks, argv[1]);

    initiateScheduling(allTasks, queueSystem, processor, clock);
    printStatistics(allTasks, processor);
    

    //Need qLEarning Envrionment implementation
    // int learningRate, discountRate, epsilon; //need to set values
    // qLearning(allTasks, processor, multiLevelQ, learningRate, discountRate, epsilon, qTable);
}