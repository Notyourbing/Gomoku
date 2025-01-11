#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <algorithm>
#include <unordered_map>

// #define TEST
#define LIMIT_TIME
// #define DISPLAY

using namespace std;

const int EMPTY = 0; // 空地
const int BLACK = 1; // 黑棋
const int WHITE = 2; // 白棋
const int MAX_DEPTH = 5; // 搜索的最大深度
const int SIZE = 12;  // 棋盘大小
const int MIN_SCORE = -1000000;
const int MAX_SCORE = 1000000;
const int WIDTH = 12;
const int TOTAL = 10;

// 棋形编号
const int WIN_FIVE = 7;	 // 连五
const int LIVE_FOUR = 6; // 活四
const int RUSH_FOUR = 5; // 冲四
const int LIVE_THREE = 4; // 活三
const int JUMP_THREE = 3; // 跳三
const int SLEEP_THREE = 2; // 眠三
const int LIVE_TWO = 1; // 活二
const int SLEEP_TWO = 0; // 眠二
const int MAX_STEP_THINK_TIME = 1700; // 每一步大脑程序思考的最大时间为1700ms
const int TOTAL_THINK_TIME = 90000; // 大脑程序一句棋总思考时间为90000ms
const int MAX_SET_NUM = (12 * 12 - 4) / 2; // 大脑程序整盘棋最多落子数
const int SCORE_TYPE[7] = { 100, 300, 500, 1000, 1800, 2000, 0 }; // 评分类型的权重
typedef unsigned long long ULL;

clock_t start_time;           // 记录AI开始思考的时间
int left_time;  // 一局棋剩余的时间
int step_time;  // 当前允许的每一步棋的最大思考时间
int current_step;             // 当前的步数
bool timeout_occurred = false; // 超时标志位
int cur_depth;

struct TypeFlag {
    int flag_color; // 用于表明棋子颜色
    int type[TOTAL] = { 0 }; // 用于记录不同棋型的数量，按棋型
};

struct Move {
    int x;
    int y;
    int score;
};

struct ZobristEntry {
    int score;
    int depth; // 深度
};

class ZobristHash {
public:
    ULL zobrist_table[SIZE][SIZE][3]; // 随机数表
    ULL current_hash; // 当前哈希值

    // 初始化随机数表
    ZobristHash() {
        srand(time(NULL));
        for (int i = 0; i < SIZE; ++i) {
            for (int j = 0; j < SIZE; ++j) {
                for (int k = 0; k < 3; ++k) {
                    zobrist_table[i][j][k] = rand64();
                }
            }
        }
    }

    // 生成64位随机数
    ULL rand64() {
        return ((ULL)rand() << 32 ^ rand());
    }

    void UpdateHash(const int x, const int y, const int color) {
        current_hash ^= zobrist_table[x][y][color]; // 更新哈希值
    }
};

// 启发式搜索的排序比较函数
int Compare(const void* a, const void* b) {
    return ((struct Move*)b)->score - ((struct Move*)a)->score;
}

// 棋盘类
class Board {
public:
    int board[SIZE][SIZE];  // 棋盘状态
    int board_record[SIZE][SIZE][4]; // 对棋子搜索的四个方向的状态记录
    TypeFlag my_flag;
    TypeFlag enemy_flag;
    Move final_move;


    // 构造函数，初始化棋盘，并关联Zobrist哈希对象
    Board() {
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j)
                board[i][j] = EMPTY;
        for (int i = 0; i < SIZE; ++i) {
            for (int j = 0; j < SIZE; ++j) {
                for (int k = 0; k < 4; ++k) {
                    board_record[i][j][k] = 0;
                }
            }
        }
        SetPiece(5, 5, WHITE);
        SetPiece(6, 6, WHITE);
        SetPiece(5, 6, BLACK);
        SetPiece(6, 5, BLACK);
    }

    ~Board() {
    }

    // 判断是否在棋盘内
    bool IsInBoard(const int x, const int y) {
        return (x >= 0 && x < SIZE && y >= 0 && y < SIZE);
    }

    // 落子
    void SetPiece(const int x, const int y, const int color) {
        board[x][y] = color;
    }

    // 撤销棋子
    void RemovePiece(const int x, const int y) {
        board[x][y] = EMPTY;
    }

    // 进一步判断是否是连五
    bool CheckWinFive(const int x, const int y, const int color, const int dx, const int dy) {
        int count = 1;
        int i = 1;
        while (1) {
            if (!IsInBoard(x + i * dx, y + i * dy) || board[x + i * dx][y + i * dy] != color) {
                break;
            }
            else {
                ++i;
                ++count;
            }
        }
        i = 1;
        while (1) {
            if (!IsInBoard(x - i * dx, y - i * dy) || board[x - i * dx][y - i * dy] != color) {
                break;
            }
            else {
                ++i;
                ++count;
            }
        }
        if (count >= 5) {
            return 1;
        }
        else {
            return 0;
        }
    }

    // 计算得分
    int GetScore(TypeFlag* mine, TypeFlag* opponent) {
        int mine_score = 0, ene_score = 0, final = 0;
        // 判断必胜条件
        if (mine->type[WIN_FIVE] > 0) {
            final = MAX_SCORE;
        }
        else if (opponent->type[WIN_FIVE] > 0) {
            final = MIN_SCORE;
        }
        else if (opponent->type[LIVE_FOUR] > 0 || opponent->type[RUSH_FOUR] > 0) {
            final = -980000;
            if (opponent->type[RUSH_FOUR]) {
                opponent->type[RUSH_FOUR] = 0;
            }
        }
        else if (mine->type[LIVE_FOUR] > 0 || mine->type[RUSH_FOUR] > 1) {
            final = 970000;
            if (mine->type[RUSH_FOUR] > 1) {
                mine->type[RUSH_FOUR] = 1;
            }
        }
        else if ((mine->type[LIVE_THREE] > 0 || mine->type[JUMP_THREE] > 0) && mine->type[RUSH_FOUR] > 0)
        {
            final = 960000;
            mine->type[LIVE_THREE] = 0;
            mine->type[RUSH_FOUR] = 0;
        }
        else if (mine->type[RUSH_FOUR] == 0 && (opponent->type[LIVE_THREE] > 0 || opponent->type[JUMP_THREE] > 0))
        {
            final = -950000;
            if (opponent->type[LIVE_THREE]) opponent->type[LIVE_THREE] = 0;
            if (opponent->type[JUMP_THREE]) opponent->type[JUMP_THREE] = 0;
        }
        else if ((mine->type[LIVE_THREE] > 1 || mine->type[JUMP_THREE] > 1) && opponent->type[LIVE_THREE] == 0 && opponent->type[JUMP_THREE] == 0)
        {
            final = 940000;
            if (mine->type[LIVE_THREE] > 1) mine->type[LIVE_THREE] = 1;
            if (mine->type[JUMP_THREE] > 1) mine->type[JUMP_THREE] = 1;
        }
        for (int i = 0; i < 7; ++i) {
            mine_score += mine->type[i] * SCORE_TYPE[i];
            ene_score += opponent->type[i] * SCORE_TYPE[i];
        }
        // 最终得分
        final += mine_score - ene_score;
        return final;
    }

    // 重置棋子状态和期盼记录
    void Reset(TypeFlag* mine) {
        // 重置棋子状态
        for (int i = 0; i < TOTAL; i++)
        {
            mine->type[i] = 0;
        }
        // 重置棋盘记录
        for (int i = 0; i < SIZE; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                for (int k = 0; k < 4; k++)
                {
                    board_record[i][j][k] = 0;
                }
            }
        }
    }

    // 设置棋盘记录，表示不用再考虑此位置
    void SetRep(const int x, const int y, const int left, const int right, const int dir[], const int dir_num) {
        // 计算起始点坐标
        int start_x = x + (left - 5) * dir[0];
        int start_y = y + (left - 5) * dir[1];
        // 标记不用再考虑的位置
        for (int i = left; i < right + 1; i++)
        {
            start_x += dir[0];
            start_y += dir[1];
            board_record[start_x][start_y][dir_num] = 1; //表示不用再看
        }
    }

    // 获取一直线上棋子的状态
    void GetLine(const int x, const int y, TypeFlag opponent, const int dir[], int* line) {
        int start_x = x + (-5 * dir[0]);
        int start_y = y + (-5 * dir[1]);
        // 获取直线上的棋子状态
        for (int i = 0; i < 9; ++i) {
            start_x += dir[0];
            start_y += dir[1];
            if (!IsInBoard(start_x, start_y)) {
                line[i] = opponent.flag_color;
            }
            else {
                line[i] = board[start_x][start_y];
            }

        }
    }

    // 当连成的棋子数为3时所能形成的棋型
    void Range3(int line[], TypeFlag* mine, TypeFlag* opponent, int* r_mine, int* l_mine) {
        if (line[*r_mine + 1] == EMPTY && line[*r_mine + 2] == mine->flag_color) {//MMMEM only right
            mine->type[RUSH_FOUR]++;
        }
        else if (line[*l_mine - 1] == EMPTY && line[*r_mine + 1] == EMPTY) { //EMMME
            if (line[*l_mine - 2] == opponent->flag_color && line[*r_mine + 2] == opponent->flag_color) { //OEMMMEO
                mine->type[SLEEP_THREE]++;
            }
            else if (line[*l_mine - 2] == opponent->flag_color && line[*r_mine + 2] == EMPTY) { //OEMMMEE
                mine->type[LIVE_THREE]++;
            }
            else if (line[*l_mine - 2] == EMPTY && line[*r_mine + 2] == opponent->flag_color) { //EEMMMEO
                mine->type[LIVE_THREE]++;
            }
            else if (line[*l_mine - 2] == EMPTY && line[*r_mine + 2] == EMPTY) { //EEMMMEE
                mine->type[LIVE_THREE]++;
            }
        }
        else if (line[*l_mine - 1] == opponent->flag_color || line[*r_mine + 1] == opponent->flag_color) {
            if (line[*r_mine + 1] == EMPTY && line[*r_mine + 2] == EMPTY) { //OMMMEE
                mine->type[SLEEP_THREE]++;
            }
            else if (line[*l_mine - 1] == EMPTY && line[*l_mine - 2] == EMPTY) { //EEMMMO      
                mine->type[SLEEP_THREE]++;
            }
        }
    }

    // 当连成的棋子数为2时所能形成的棋型
    void Range2(int line[], TypeFlag* mine, TypeFlag* opponent, int* r_mine, int* l_mine) {
        if (line[*r_mine + 1] == EMPTY && line[*r_mine + 2] == mine->flag_color && line[*r_mine + 3] == mine->flag_color) { //MMEMM R
            mine->type[RUSH_FOUR]++;
        }
        else if (line[*r_mine + 1] == EMPTY && line[*r_mine + 2] == mine->flag_color) //MMEM
        {
            if (line[*r_mine + 3] == opponent->flag_color || line[*l_mine - 1] == opponent->flag_color) //MMEMO || OMMEM only right
            {
                mine->type[SLEEP_THREE]++;
            }
            else mine->type[JUMP_THREE]++;//MMEM
        }

        else if (line[*r_mine + 1] == EMPTY && line[*r_mine + 2] == EMPTY && line[*r_mine + 3] == mine->flag_color)//MMEEM only right
        {
            mine->type[SLEEP_THREE]++;
        }
        else if (line[*l_mine - 1] == EMPTY && line[*r_mine + 1] == EMPTY) //EMME
        {
            if (line[*l_mine - 2] == opponent->flag_color && line[*r_mine + 2] == EMPTY)//OEMMEE
            {
                mine->type[LIVE_TWO]++;
            }
            else if (line[*r_mine + 2] == opponent->flag_color && line[*l_mine - 2] == EMPTY && line[*l_mine - 3] == opponent->flag_color)//OEEMMEO
            {
                mine->type[SLEEP_TWO]++;
            }
            else if (line[*l_mine - 2] == EMPTY && line[*r_mine + 2] == EMPTY)//EEMMEE
            {
                mine->type[LIVE_TWO]++;
            }
        }

        else if (line[*l_mine - 1] == opponent->flag_color && line[*r_mine + 1] == EMPTY && line[*r_mine + 2] == EMPTY)//OMMEE
        {
            mine->type[SLEEP_TWO]++;
        }
        else if (line[*r_mine + 1] == opponent->flag_color && line[*l_mine - 1] == EMPTY && line[*l_mine - 2] == EMPTY && line[*l_mine - 3] == EMPTY)//EEEMMO
        {
            mine->type[SLEEP_TWO]++;
        }

        else if (line[*l_mine - 1] == EMPTY && line[*l_mine - 2] == EMPTY && line[*l_mine - 3] == opponent->flag_color)//OEEMM
        {
            mine->type[SLEEP_TWO]++;
        }
        else if (line[*r_mine + 1] == EMPTY && line[*r_mine + 2] == EMPTY && line[*r_mine + 3] == opponent->flag_color)//MMEEO
        {
            mine->type[SLEEP_TWO]++;
        }
    }

    // 当连成的棋子数为1时所能形成的棋型
    void Range1(int line[], TypeFlag* mine, TypeFlag* opponent, int* r_mine, int* l_mine)
    {
        if (line[*r_mine + 1] == EMPTY && line[*r_mine + 2] == mine->flag_color && line[*r_mine + 3] == mine->flag_color && line[*r_mine + 4] == mine->flag_color)
        {
            mine->type[RUSH_FOUR]++;
        }//MEMMM

        else if (line[*r_mine + 1] == EMPTY && line[*r_mine + 2] == mine->flag_color && line[*r_mine + 3] == EMPTY && line[*r_mine + 4] == mine->flag_color)
        {
            mine->type[SLEEP_THREE]++;
        }//MEMEM

        else if (line[*r_mine + 1] == EMPTY && line[*r_mine + 2] == mine->flag_color && line[*r_mine + 3] == mine->flag_color)
        {
            if (line[*l_mine - 1] == opponent->flag_color)//OMEMM
            {
                mine->type[SLEEP_THREE]++;
            }
            else if (line[*r_mine + 4] == opponent->flag_color)//MEMMO
            {
                mine->type[SLEEP_THREE]++;
            }
            else mine->type[JUMP_THREE]++;//MEMM
        }

        else if (line[*r_mine + 1] == EMPTY && line[*r_mine + 2] == EMPTY && line[*r_mine + 3] == mine->flag_color && line[*r_mine + 4] == mine->flag_color) //MEEMM
        {
            if (line[*r_mine + 3] == mine->flag_color) //MEEMM
            {
                mine->type[SLEEP_THREE]++;
            }
            else if (line[*r_mine + 3] == EMPTY)//MEEEM
            {
                mine->type[SLEEP_TWO]++;
            }
        }

        else if (line[*l_mine - 1] == EMPTY && line[*r_mine + 1] == EMPTY && line[*r_mine + 2] == mine->flag_color && line[*r_mine + 3] == EMPTY && line[*l_mine - 2] == opponent->flag_color && line[*r_mine + 4] == opponent->flag_color)
        {
            mine->type[SLEEP_TWO]++;
        }//OEMEMEO

        else if (line[*r_mine + 1] == EMPTY && line[*r_mine + 2] == EMPTY && line[*r_mine + 3] == mine->flag_color)
        {
            if (line[*l_mine - 1] == opponent->flag_color || line[*r_mine + 4] == opponent->flag_color) // MEEMO  || OMEEM
            {
                mine->type[SLEEP_TWO]++;
            }
            else mine->type[LIVE_TWO]++; //MEEM
        }

        else if (line[*r_mine + 1] == EMPTY && line[*r_mine + 2] == mine->flag_color) //MEM
        {
            if (line[*r_mine + 3] == EMPTY && line[*r_mine + 4] == opponent->flag_color)//MEMEO
            {
                mine->type[SLEEP_TWO]++;
            }
            else if (line[*l_mine - 1] == opponent->flag_color && line[*r_mine + 3] == EMPTY)//OMEME
            {
                mine->type[SLEEP_TWO]++;
            }
            else if (line[*l_mine - 1] == EMPTY && line[*r_mine + 3] == EMPTY) //EMEME
            {
                mine->type[LIVE_TWO]++;
            }
            else if (line[*l_mine - 1] == opponent->flag_color && line[*r_mine + 3] == EMPTY && line[*r_mine + 4] == EMPTY)//OMEMEE
            {
                mine->type[SLEEP_TWO]++;
            }
            else if (line[*r_mine + 3] == opponent->flag_color && line[*l_mine - 1] == EMPTY && line[*l_mine - 2] == EMPTY)//EEMEMO
            {
                mine->type[SLEEP_TWO]++;
            }
        }
    }

    // 检查每一条线上所形成的棋型
    void CheckLine(const int x, const int y, TypeFlag* mine, TypeFlag* opponent, const int* dir, int dir_num)
    {
        int line[9] = { 0 };
        memset(line, 0, 9);
        GetLine(x, y, *opponent, dir, line);
        int l_mine = 4, r_mine = 4;
        while (l_mine > 0)
        {
            if (line[l_mine - 1] != mine->flag_color) break;
            l_mine--;
        }//连子最大左范围
        while (r_mine < 8)
        {
            if (line[r_mine + 1] != mine->flag_color) break;
            r_mine++;
        }//连子最大右范围
        int l_range = l_mine, r_range = r_mine;
        while (l_range > 0)
        {
            if (line[l_range - 1] == opponent->flag_color) break;
            l_range--;
        }//棋子最大左范围
        while (r_range < 8)
        {
            if (line[r_range + 1] == opponent->flag_color) break;
            r_range++;
        }//棋子最大右范围
        int range = r_range - l_range + 1;
        SetRep(x, y, l_mine, r_mine, dir, dir_num);//将当前棋子置为看过
        if (range < 5)
        {
            return;
        }
        int cu_range = r_mine - l_mine + 1;
        if (cu_range >= 5)
        {
            mine->type[WIN_FIVE]++;
        }
        else if (cu_range == 4) //先冲四后活四
        {
            int l_empty = 1, r_empty = 1;
            if (line[l_mine - 1] != EMPTY) l_empty = 0;
            if (line[r_mine + 1] != EMPTY) r_empty = 0;
            if (l_empty)
            {
                if (!r_empty) mine->type[RUSH_FOUR]++; //EMMMMO
                else mine->type[LIVE_FOUR]++; //EMMMME
            }
            else if (r_empty) mine->type[RUSH_FOUR]++; //OMMMME
        }
        else if (cu_range == 3)
        {
            Range3(line, mine, opponent, &r_mine, &l_mine);
        }
        else if (cu_range == 2)
        {
            Range2(line, mine, opponent, &r_mine, &l_mine);
        }
        else if (cu_range == 1)
        {
            Range1(line, mine, opponent, &r_mine, &l_mine);
        }
    }

    // 对点的四种方向上的棋型进行统计
    void EvaluatePoint(const int x, const int y, TypeFlag* mine, TypeFlag* opponent) {
        const int every_dir[4][2] = { {0,1},{1,0},{1,1},{-1,1} };
        for (int i = 0; i < 4; i++) {
            if (board_record[x][y][i] == 0)  //未被标记
                CheckLine(x, y, mine, opponent, every_dir[i], i);
        }
    }

    // 评估棋盘
    void Evaluate(TypeFlag* mine, TypeFlag* opponent) {
        Reset(mine);
        Reset(opponent);
        for (int x = 0; x < SIZE; x++) {
            for (int y = 0; y < SIZE; y++) {
                if (board[x][y] == mine->flag_color) { //如果为我方棋子检测棋子所成的棋型        
                    EvaluatePoint(x, y, mine, opponent);
                }
                else if (board[x][y] == opponent->flag_color) {//如果为对方棋子检测棋子所成的棋型
                    EvaluatePoint(x, y, opponent, mine);
                }
            }
        }
    }

    // 判断胜利条件
    bool Win(const TypeFlag* mine, const TypeFlag* oppenent) { // 只针对即将落子的一方
        if (mine->type[WIN_FIVE] > 0) {
            return true;
        }
        else if (mine->type[LIVE_FOUR] > 0) {
            return true;
        }
        return false;
    }

    // 启发式搜索
    struct Move* GetMove(struct Move* move, TypeFlag* turn, TypeFlag* oppenent) {
        for (int i = 0; i < 145; i++) {
            move[i].x = -1;
            move[i].y = -1;
            move[i].score = -0x7fffffff + 10000010;// 防止超过int下限
        }// 初始化move
        int wide = 0;
        for (int x = 0; x < SIZE; x++) {
            for (int y = 0; y < SIZE; y++) {
                if (board[x][y] == EMPTY) {
                    SetPiece(x, y, turn->flag_color);
                    Evaluate(turn, oppenent);
                    int score = GetScore(turn, oppenent); // 判断得分
                    move[wide].x = x;
                    move[wide].y = y;
                    move[wide].score = score; // 更新点数据
                    wide++;
                    RemovePiece(x, y);
                }
            }
        }
        qsort(move, 145, sizeof(Move), Compare); // 排序
        return move;
    }

    // 计算搜索的广度
    void CaculateWide(int* wide, struct Move* move) {
        for (int i = 0; i < WIDTH; i++) {
            if (move[i].x != -1) {
                *wide = i + 1;
            }
        }
    }

    // 最大层
    int MaxSearch(const int x, const int y, const int depth, int alpha, int beta) {
#ifdef LIMIT_TIME
        // 检查是否超时
        double elapsed_time_ms = (double)(clock() - start_time) * 1000.0 / CLOCKS_PER_SEC;
        if (elapsed_time_ms > step_time) {
            timeout_occurred = true; // 设置超时标志
            return  alpha;  // 超时则返回当前alpha或beta值
        }
#endif
        // 重置棋子状态
        Reset(&my_flag);
        Reset(&enemy_flag);

        //对第一个无效点作出限制
        if (x != -1 && y != -1) {
            EvaluatePoint(x, y, &enemy_flag, &my_flag);
            // 如果对方赢了，返回极小值
            if (Win(&enemy_flag, &my_flag)) return MIN_SCORE;
        }


        // 如果达到搜索深度限制，直接返回当前局面的得分
        if (depth == 0) {
            Evaluate(&my_flag, &enemy_flag);
            int sco = GetScore(&my_flag, &enemy_flag);
            return sco;
        }
        // 获取启发式评估后的所有落子位置
        struct Move move[145];
        GetMove(move, &my_flag, &enemy_flag);

        // 计算可走位置的数量
        int wide = 0;
        CaculateWide(&wide, move);
        // 如果无法走棋，则直接返回当前局面的得分
        if (wide == 0) {
            Evaluate(&my_flag, &enemy_flag);
            int sco = GetScore(&my_flag, &enemy_flag);
            return sco;
        }
        // 遍历所有可走位置
        for (int i = 0; i < wide; i++) {
            SetPiece(move[i].x, move[i].y, my_flag.flag_color);
            int alpha_score = MinSearch(move[i].x, move[i].y, depth - 1, alpha, beta);
            RemovePiece(move[i].x, move[i].y);
            if (alpha_score > alpha)
            {
                alpha = alpha_score;
                if (depth == cur_depth && move[i].x != -1 && move[i].y != -1) {
                    final_move.x = move[i].x;
                    final_move.y = move[i].y;
                    if (alpha == MAX_SCORE) { //若搜到必胜点直接返回
                        return alpha;
                    }
                }
            }
            if (alpha >= beta)
                break;   //剪枝
        }
        return alpha;
    }

    // 最小层
    int MinSearch(const int x, const int y, const int depth, int alpha, int beta) {
#ifdef LIMIT_TIME
        // 检查是否超时
        double elapsed_time_ms = (double)(clock() - start_time) * 1000.0 / CLOCKS_PER_SEC;
        if (elapsed_time_ms > step_time) {
            timeout_occurred = true; // 设置超时标志
            return  beta;  // 超时则返回当前alpha或beta值
        }
#endif
        // 重置棋子状态
        Reset(&my_flag);
        Reset(&enemy_flag);
        // 评估当前位置
        EvaluatePoint(x, y, &my_flag, &enemy_flag);
        //如果我方赢了，返回极大值
        if (Win(&enemy_flag, &my_flag)) return MAX_SCORE;
        // 如果达到搜索深度限制，直接返回当前局面的得分
        if (depth == 0) {
            Evaluate(&my_flag, &enemy_flag);
            int sco = GetScore(&my_flag, &enemy_flag);
            return sco;
        }
        struct Move move[145];
        GetMove(move, &enemy_flag, &my_flag);
        int wide = 0;
        CaculateWide(&wide, move);
        if (wide == 0) {
            Evaluate(&my_flag, &enemy_flag);
            int sco = GetScore(&my_flag, &enemy_flag);
            return sco;
        }
        for (int i = 0; i < wide; i++) {
            SetPiece(move[i].x, move[i].y, enemy_flag.flag_color);
            int beta_score = MaxSearch(move[i].x, move[i].y, depth - 1, alpha, beta);;
            RemovePiece(move[i].x, move[i].y);
            if (beta_score < beta) {
                beta = beta_score;
            }
            if (alpha >= beta)
                break; //剪枝
        }
        return beta;
    }

    // 棋盘显示函数
    void DisplayBoard() {
        printf("   ");
        for (int i = 0; i < SIZE; ++i) {
            printf("%2d  ", i); // 顶部字母坐标
        }
        printf("\n");

        for (int x = 0; x < SIZE; ++x) {
            printf("%2d ", x); // 左侧数字坐标
            for (int y = 0; y < SIZE; ++y) {
                string c = "+"; // 交汇点
                if (board[x][y] == BLACK) {
                    c = "\b○"; // 黑色棋子
                }
                else if (board[x][y] == WHITE) {
                    c = "\b●"; // 白色棋子
                }
                cout << c;

                if (y < SIZE - 1) { // 最后一列不需要打印横线
                    printf("---");
                }
            }
            printf("\n");

            if (x < SIZE - 1) { // 最后一行不需要打印竖线
                printf("   |");
                for (int i = 1; i < SIZE; ++i) {
                    printf("   |"); // 谱线交汇处
                }
                printf("\n");
            }
        }
        printf("\n");
    }
};

// AI类
class GomokuAI {
private:
    Board chess_board;            // 棋盘实例
public:
    // 构造函数
    GomokuAI() {  // 初始化棋盘
        srand(time(NULL));  // 初始化随机数种子
        left_time = TOTAL_THINK_TIME;
        step_time = MAX_STEP_THINK_TIME;
        current_step = 0;  // 初始化为第一步
    }

    // 计算当前步数的思考时间
    int CalculateStepTime() {
        int base_time = 1600;  // 每步的基础时间
        // 动态调整每步的最大思考时间
        if (current_step < 15) {
            // 开局阶段
            return min(base_time + (current_step * 20), MAX_STEP_THINK_TIME);
        }
        else if (current_step < 50) {
            // 中期阶段
            return min(base_time + (current_step * 10), MAX_STEP_THINK_TIME);
        }
        else {
            // 后期阶段，尽可能使用剩余的时间
            return min(MAX_STEP_THINK_TIME, left_time / (MAX_SET_NUM - current_step + 1));
        }
    }

    // 初始化AI颜色
    void StartGame(const int color) {
        chess_board.my_flag.flag_color = color;
        chess_board.enemy_flag.flag_color = 3 - color;
        printf("OK\n");
        fflush(stdout);
    }

    // 判断胜利条件
    bool Win(const TypeFlag* mine, const TypeFlag* oppenent) { // 只针对即将落子的乙方
        if (mine->type[WIN_FIVE] > 0) {
            return true;
        }
        else if (mine->type[LIVE_FOUR] > 0) {
            return true;
        }
        return false;
    }

    // 预检查是否有WIN_FIVE的局面，如果有，就不进行搜索
    bool PreCheckWinFive(int& pre_x, int& pre_y) {
        for (int i = 0; i < SIZE; ++i) {
            for (int j = 0; j < SIZE; ++j) {
                if (chess_board.board[i][j] == EMPTY) {
                    // 四个方向
                    const int directions[4][2] = { {1, 0}, {0, 1}, {1, 1}, {1, -1} };
                    for (int k = 0; k < 4; ++k) {
                        if (chess_board.CheckWinFive(i, j, chess_board.my_flag.flag_color, directions[k][0], directions[k][1])) {
                            pre_x = i, pre_y = j;
                            return 1;
                        }
                    }
                }
            }
        }
        return 0;
    }

    // 处理对方下子
    void PlaceOpponentMove(const int x, const int y) {
        chess_board.SetPiece(x, y, chess_board.enemy_flag.flag_color);
    }

    // 找到最好的点
    pair<int, int> FindBestMove() {
        start_time = clock();
        int alpha = -0x7fffffff;
        int beta = 0x7fffffff;
        int best_x = -1;
        int best_y = -1;
        // 防止出现为了堵对方连五，而放弃自己连五
        int pre_x = -1, pre_y = -1;
        if (PreCheckWinFive(pre_x, pre_y)) {
            return { pre_x, pre_y };
        }
        for (cur_depth = MAX_DEPTH; cur_depth >= 2; cur_depth -= 3) {
            // 清空置换表
            timeout_occurred = false; // 重置超时标志
            start_time = clock();
            int score = chess_board.MaxSearch(-1, -1, cur_depth, alpha, beta);
            best_x = chess_board.final_move.x;
            best_y = chess_board.final_move.y;

#ifdef LIMIT_TIME
            if (timeout_occurred) {
                best_x = -1;
                best_y = -1;
                left_time -= step_time;
                start_time = clock();
                continue;
            }
#endif
            return { best_x, best_y };
        }
        return { best_x, best_y };
    }

    // 处理AI回合
    void Turn() {
        start_time = clock(); // 开始计时
        current_step++;       // 记录步数
        step_time = CalculateStepTime();  // 更新当前步允许的最大思考时间
        pair<int, int> move = FindBestMove();
        chess_board.SetPiece(move.first, move.second, chess_board.my_flag.flag_color);
#ifdef DISPLAY
        chess_board.DisplayBoard();
#endif
        printf("%d %d\n", move.first, move.second);
        fflush(stdout);
        double time_taken = (double)(clock() - start_time) / CLOCKS_PER_SEC * 1000.0;
        left_time -= int(time_taken); // 剩余时间减少
#ifdef TEST
        // 输出思考时间
        printf("Time taken: %.2f ms\n", time_taken);
        printf("Left time: %d ms\n", left_time);
        fflush(stdout);
#endif
    }
};

int main() {
    GomokuAI ai;  // 创建AI实例
    char command[18];
    int x, y, color;
    while (scanf("%s", command) != EOF) {
        if (strcmp(command, "START") == 0) {
            scanf("%d", &color);
            ai.StartGame(color);  // 启动游戏
        }
        else if (strcmp(command, "PLACE") == 0) {
            scanf("%d %d", &x, &y);
            ai.PlaceOpponentMove(x, y);  // 对手落子
        }
        else if (strcmp(command, "TURN") == 0) {
            ai.Turn();  // AI落子
        }
        else if (strcmp(command, "END") == 0) {
            break;  // 结束游戏
        }
    }
    return 0;
}