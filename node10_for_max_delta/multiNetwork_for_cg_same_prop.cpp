#include <iostream>
#include <fstream>
#include <random>
#include <queue>
#include <cmath>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm> // std::max_element

// プリプロセッサディレクティブ 定数を定義
#define MAX_RN 10
#define MAX_N 10
#define MAX_Round 1000000
using namespace std;
typedef unsigned long long ull;
typedef long long ll;

struct block
{
    ll height;
    block *prevBlock;
    int minter; // 0 = Alice, 1 = Bob
};

struct task
{
    ll time;
    int flag;   // 0 = block, 1 = propagation
    int minter; // 0
    int from;   // 1
    int to;     // 1
    block *propagatedBlock;
};

ll currentRound;
ll numBlock;
ll numFork;
ll currentTime = 0;
std::vector<std::vector<long long>> propagationTimes;
// decimalPointに10の3乗を代入 ミリ秒
ll decimalPoint = 1000;
ll generationTime = 600 * decimalPoint;
block *currentBlock[MAX_N];
ll numGenerated[MAX_N];
ll numStaled[MAX_N];
ll numRound[MAX_N];
ll hashrate[MAX_N];
ll totalHashrate;
ll numMain[MAX_N];
double forkRate[MAX_N][MAX_N];
ll endRound = MAX_Round;
ll numStaledPerRound[MAX_Round];
ll mainLength;
// node i がブロックを作ったときの一番最後のノードに届いたときの時刻にnode iのハッシュレートをかけたものの和
ll hashrateWeightedMaxPropTime;
// 各ノードがブロックを作ったときの一番最後のノードに届いたときの時刻の和をノード数で割ったもの
ll aveMaxPropTime;
// 最大の伝搬時間
ll maxPropTime;
// ノード間の伝搬時間の固定値
ll delay = 5303;
int N = MAX_N;
double fairness[MAX_N];

void chooseMainchain(block *block1, block *block2, int from, int to);
void deleteBlock(block *block1, block *block2);
void mainChain(block *block1);
double p_exp(double v);
ll prop(int i, int j);
std::vector<std::vector<long long> > readPropagationTimes(const std::string &filename);
// 配列の最大値を求める関数
long long findMaxElement(long long arr[], int size);

void generateHashrates(ll hashrate[], int N, double s)
{
    double normalizing_constant = 0;
    for (int n = 1; n <= N; ++n)
    {
        normalizing_constant += 1.0 / std::pow(n, s);
    }

    for (int k = 1; k <= N; ++k)
    {
        hashrate[k - 1] = static_cast<ll>((1.0 / std::pow(k, s)) / normalizing_constant * 1000000);
    }
}

int main(void)
{
    // 計測開始時間を記録
    auto start = std::chrono::high_resolution_clock::now();
    // 計測開始時間を出力
    std::cout << "start: " << std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count() << std::endl;
    // ファイルの保存名をnode20_for_cg_same_prop/simulation_results_T{generationTimeをdecimalPointで割ったもの}.csvにする
    std::ofstream csvFile("simulation_results_T" + std::to_string(generationTime / decimalPoint) + ".csv");
    csvFile << "s,theoretical_forkRate,real_forkRate,theoretical_staleRate,real_staleRate,hashrateWeightedMaxPropTime,aveMaxPropTime,maxPropTime\n";
    for (int i = 0; i < MAX_N; i++)
    {
        std::vector<long long> row;
        // cout << "hoge2" << endl;
        for (int j = 0; j < MAX_N; j++)
        {
            // cout << "hoge3" << endl;
            if (i == j)
            {
                row.push_back(0);
            }
            else
            {
                row.push_back(delay);
            }

            // cout << "hoge4" << endl;
        }
        propagationTimes.push_back(row);
    }
    // cout << "hoge4" << endl;
    N = propagationTimes.size();

    for (double s = 0.0; s <= 2.01; s += 0.025)
    {
        cout << "Running simulation for s = " << s << endl;

        std::random_device seed_gen;
        std::mt19937_64 engine(1);
        std::uniform_real_distribution<double> dist1(0., 1.0);
        std::exponential_distribution<double> dist2(1);

        // propagationTimes = readPropagationTimes("./node20/input_node20.txt");
        // propagationTimesの全要素にdelayを格納する
        // cout << "hoge" << endl;
        // std::vector<std::vector<long long>> propagationTimes;のpropagationTimesの各要素にdelayを格納する．
        // for (int i = 0; i < MAX_N; i++)
        // {
        //     std::vector<long long> row;
        //     // cout << "hoge2" << endl;
        //     for (int j = 0; j < MAX_N; j++)
        //     {
        //         // cout << "hoge3" << endl;
        //         row.push_back(delay);
        //         // cout << "hoge4" << endl;
        //     }
        //     propagationTimes.push_back(row);
        // }
        // // cout << "hoge4" << endl;
        // N = propagationTimes.size();
        // cout << "hoge5" << endl;

        generateHashrates(hashrate, N, s);
        totalHashrate = 0;
        for (int i = 0; i < N; i++)
        {
            totalHashrate += hashrate[i];
        }

        // hashrateWeightedMaxPropTimeを求める
        hashrateWeightedMaxPropTime = 0;
        aveMaxPropTime = 0;
        maxPropTime = 0;
        for (int i = 0; i < N; i++)
        {
            // propagationTimesのi行目の最大値を求める
            hashrateWeightedMaxPropTime += hashrate[i] * findMaxElement(propagationTimes[i].data(), N) / totalHashrate;
            // findMaxElement(propagationTimes[i].data(), N)をprint
            // cout << findMaxElement(propagationTimes[i].data(), N) << endl;
            aveMaxPropTime += findMaxElement(propagationTimes[i].data(), N);
            // cout << "hoge6" << endl;
            maxPropTime = max(maxPropTime, findMaxElement(propagationTimes[i].data(), N));
            // cout << "hoge7" << endl;
        }
        aveMaxPropTime = aveMaxPropTime / N;

        auto compare = [](task *a, task *b)
        {
            return a->time > b->time;
        };

        std::priority_queue<
            task *,
            std::vector<task *>,
            decltype(compare)>
            taskQue{compare};

        std::queue<block *> blockQue;
        block *genesisBlock = new block;
        blockQue.push(genesisBlock);
        genesisBlock->prevBlock = nullptr;
        genesisBlock->height = 0;
        genesisBlock->minter = -1;

        for (int i = 0; i < N; i++)
        {
            currentBlock[i] = genesisBlock;
            task *nextBlockTask = new task;
            nextBlockTask->time = (ll)(dist2(engine) * generationTime * totalHashrate / hashrate[i]);
            nextBlockTask->flag = 0;
            nextBlockTask->minter = i;
            taskQue.push(nextBlockTask);
        }

        currentRound = 0;
        numBlock = 0;
        numFork = 0;
        currentTime = 0;
        for (int i = 0; i < N; i++)
        {
            numGenerated[i] = 0;
            numStaled[i] = 0;
            numRound[i] = 0;
            numMain[i] = 0;
        }
        for (int i = 0; i < endRound; i++)
        {
            numStaledPerRound[i] = 0;
        }

        while (taskQue.size() > 0 && currentRound < endRound)
        {
            task *currentTask = taskQue.top();
            taskQue.pop();
            currentTime = currentTask->time;

            if (currentTask->flag == 0)
            {
                int minter = currentTask->minter;
                block *newBlock = new block;
                newBlock->prevBlock = currentBlock[minter];
                newBlock->height = currentBlock[minter]->height + 1;
                newBlock->minter = minter;
                currentBlock[minter] = newBlock;
                numGenerated[minter]++;
                blockQue.push(newBlock);
                if (blockQue.size() > 10000)
                {
                    block *deleteBlock = blockQue.front();
                    blockQue.pop();
                    delete deleteBlock;
                }

                task *nextBlockTask = new task;
                nextBlockTask->time = currentTime + (ll)(dist2(engine) * generationTime * totalHashrate / hashrate[minter]);
                nextBlockTask->flag = 0;
                nextBlockTask->minter = minter;
                taskQue.push(nextBlockTask);

                for (int i = 0; i < N; i++)
                {
                    if (i != minter)
                    {
                        task *nextPropTask = new task;
                        nextPropTask->time = currentTime + propagationTimes[minter][i];
                        nextPropTask->flag = 1;
                        nextPropTask->to = i;
                        nextPropTask->from = minter;
                        nextPropTask->propagatedBlock = newBlock;
                        taskQue.push(nextPropTask);
                    }
                }

                if (currentRound < newBlock->height)
                {
                    currentRound = newBlock->height;
                    numBlock = 1;
                    numRound[minter]++;
                    mainChain(newBlock);
                }
                else
                {
                    numStaledPerRound[currentRound]++;
                    numBlock++;
                    numFork++;
                }
            }
            else
            {
                int to = currentTask->to;
                int from = currentTask->from;
                chooseMainchain(currentTask->propagatedBlock, currentBlock[to], from, to);
            }

            delete currentTask;
        }

        // フォーク率はフォークした確率．フォークしているラウンドの率
        double real_forkRate = 0;
        for (int i = 0; i < endRound; i++)
        {
            if (numStaledPerRound[i] != 0)
                real_forkRate++;
        }
        real_forkRate = real_forkRate / endRound;
        // theoretical_staleRate
        double theoretical_staleRate = 0;
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                theoretical_staleRate += hashrate[i] * hashrate[j] * double(propagationTimes[i][j]) / generationTime;
            }
        }
        theoretical_staleRate = theoretical_staleRate / totalHashrate / totalHashrate;
        // メインチェーンに入ってない率．全ブロック中にメインチェーンに含まれていない確率．ステール率．
        // （総ブロック数 - ラウンド数） / 総ブロック数
        // real_staleRate
        double real_staleRate = 0;
        for (int i = 0; i < endRound; i++)
        {
            real_staleRate += numStaledPerRound[i];
        }
        cout << "real_num_of_staleBlock: " << real_staleRate << endl;
        real_staleRate = real_staleRate / endRound;
        // cout << "real_staleRate: " << real_staleRate << endl;

        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                if (i == j)
                    forkRate[i][j] = 0;
                else
                    forkRate[i][j] = 1 - p_exp(-double(propagationTimes[i][j]) / generationTime);
            }
        }

        double theoretical_forkRate = 0;
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                if (i == j)
                    theoretical_forkRate += hashrate[i] * hashrate[j];
                else
                    theoretical_forkRate += hashrate[i] * hashrate[j] * p_exp(-double(propagationTimes[i][j]) / generationTime);
            }
        }
        theoretical_forkRate = 1 - (theoretical_forkRate / totalHashrate / totalHashrate);
        // cout << "hoge8" << endl;

        csvFile << s << "," << theoretical_forkRate << "," << real_forkRate << "," << theoretical_staleRate << "," << real_staleRate << "," << hashrateWeightedMaxPropTime << "," << aveMaxPropTime << "," << maxPropTime << "\n";
    }

    csvFile.close();
    // 計測終了時間を記録
    auto end = std::chrono::high_resolution_clock::now();

    // 実行時間を計算（ナノ秒単位）
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    // 実行時間をミリ秒単位で出力
    std::cout << "実行時間: " << duration / 1000000.0 << " ミリ秒" << std::endl;
    return 0;
}

void chooseMainchain(block *block1, block *block2, int from, int to)
{
    if (block1)
        if (block1->height > block2->height)
        {
            currentBlock[to] = block1;
            deleteBlock(block2, block1);
        }
    return;
}

void deleteBlock(block *block1, block *block2)
{
    if (block1->height > block2->height)
    {
        deleteBlock(block2, block1);
    }
    else
    {
        block *b1 = block1;
        block *b2 = block2;
        while (b2->height > b1->height)
            b2 = b2->prevBlock;
        while (b1 != b2)
        {
            numStaled[b1->minter]++;
            b1 = b1->prevBlock;
            b2 = b2->prevBlock;
        }
    }
}

double p_exp(double v)
{
    double res = 0;
    int N = 100;
    double V = 1;
    for (int i = 0; i < N; i++)
    {
        if (i != 0)
        {
            V = V * v / i;
        }
        res += V;
    }
    return res;
}

ll prop(int i, int j)
{
    if (i == j)
        return 0;
    else
        return propagationTimes[i][j];
}

void mainChain(block *block1)
{
    if (block1->height != endRound)
    {
        int height = block1->height;
        block *curBlock = block1;
        while (curBlock->height > 0 && curBlock->height != height - 100)
        {
            curBlock = curBlock->prevBlock;
            if (curBlock == nullptr)
                cout << "sugeeeee" << endl;
        }
        if (curBlock->height > 0)
            numMain[curBlock->minter]++;
        mainLength = max(mainLength, curBlock->height);
        return;
    }
    else
    {
        block *curBlock = block1;
        while (curBlock->height > mainLength)
        {
            numMain[curBlock->minter]++;
            curBlock = curBlock->prevBlock;
        }
    }
}

std::vector<std::vector<long long>> readPropagationTimes(const std::string &filename)
{
    std::vector<std::vector<long long>> data;
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return data;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::vector<long long> row;
        std::istringstream iss(line);
        long long value;
        while (iss >> value)
        {
            row.push_back(value);
        }
        data.push_back(row);
    }

    file.close();
    return data;
}

// 配列の最大値を求める関数
long long findMaxElement(long long arr[], int size)
{
    // std::max_elementは最大値のポインタを返すので、デリファレンスして値を取得
    return *std::max_element(arr, arr + size);
}
