
// - CPU Utilization(C)
// - Throughput (T)
// - Turnaround time (TA)
// - Waiting time (WT)
// - Response time (RT)
// - Deadline meet ratio (DR)
// 


/*
Structure as define by the paper 
    Environment: Multicore Processor
    Agent: Dispatcher
    States:
        1. Initial State(S1)
        2. Objective Degradation stage(S2)
        3. Objective Progression Stage(S3)
        4. Objective Stabilization Stage(S4)
    Action: Mapping the task to one of the N queues
    Reward: The value of the multi objective function.
        O=w1*C+w2*T-w3*([TA/TAe] -1)-w4*([WT/WTe] -1)-w5*([RT/RTe] -1)+ w6*DR
*/
/*
struct agent //this is our RL agent
{
    queue<task*> tasksToAssign;
    int actionToTake;

    int totalAssignments;
    void assignTask(task *t, queue<task*> &q);
    void testAssign(task &t, queue<task*> &q);
    void getEvironmentInfo();
};

void zeroQtable(vector<vector<int>> &table)
{
    for(int i = 0; i < 5; i++)
    {
        vector<int> temp(5);
        table.push_back(temp);
    }
}

vector<int> task::curState()
{
    vector<int> temp;
    temp.insert(temp.end(), {this->executionTime, this->remainingTime, this->deadline});

    return temp;
}
void agent::assignTask(task *t, queue<task*> &q)
{
    q.push(t);
}

void qLearning(vector<task*> &tasks, cpu *processor, 
                vector<queue<task*>> multiLevelQ, int lRate, int dRate, int epsilon, unordered_map<vector<int>, float, VectorHasher> qTable)
{
    int nArrivals = 0;
    int clock = 0;
    while(nArrivals < tasks.size())
    {
        for(int i = 0; i < tasks.size(); i++) //sending tasks to dispatcher to begin assignment to Queues
        {
            if(tasks[i]->arrivalTime == clock)
            {
                dispatcher->tasksToAssign.push(tasks[i]);
                nArrivals++;
            }
        }

        double r = ((double) rand() / (RAND_MAX));
        vector<int> state = dispatcher->tasksToAssign.front()->curState();
        int nextAction;
        if(r < 1 - epsilon)
        {
            int maxReward = 0;
            for(int j = 0; j < 3; j++)
            {
                state.push_back(j);
                if(qTable[state] > maxReward)
                {
                    nextAction = j;
                    maxReward = qTable[state];
                }
                state.pop_back();
            }
            state.push_back(nextAction);
        }
        else
        {
            nextAction = randomGen(0,3);
            state.push_back(nextAction);
        }

        dispatcher->assignTask(dispatcher->tasksToAssign.front(), multiLevelQ[nextAction]);
        checkMultiLevelQ(multiLevelQ, processor);
        processor->checkCondition
        clock++;
    }
}

*/