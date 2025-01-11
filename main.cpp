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

const int EMPTY = 0; // �յ�
const int BLACK = 1; // ����
const int WHITE = 2; // ����
const int MAX_DEPTH = 5; // ������������
const int SIZE = 12;  // ���̴�С
const int MIN_SCORE = -1000000;
const int MAX_SCORE = 1000000;
const int WIDTH = 12;
const int TOTAL = 10;

// ���α��
const int WIN_FIVE = 7;	 // ����
const int LIVE_FOUR = 6; // ����
const int RUSH_FOUR = 5; // ����
const int LIVE_THREE = 4; // ����
const int JUMP_THREE = 3; // ����
const int SLEEP_THREE = 2; // ����
const int LIVE_TWO = 1; // ���
const int SLEEP_TWO = 0; // �߶�
const int MAX_STEP_THINK_TIME = 1700; // ÿһ�����Գ���˼�������ʱ��Ϊ1700ms
const int TOTAL_THINK_TIME = 90000; // ���Գ���һ������˼��ʱ��Ϊ90000ms
const int MAX_SET_NUM = (12 * 12 - 4) / 2; // ���Գ������������������
const int SCORE_TYPE[7] = { 100, 300, 500, 1000, 1800, 2000, 0 }; // �������͵�Ȩ��
typedef unsigned long long ULL;

clock_t start_time;           // ��¼AI��ʼ˼����ʱ��
int left_time;  // һ����ʣ���ʱ��
int step_time;  // ��ǰ�����ÿһ��������˼��ʱ��
int current_step;             // ��ǰ�Ĳ���
bool timeout_occurred = false; // ��ʱ��־λ
int cur_depth;

struct TypeFlag {
    int flag_color; // ���ڱ���������ɫ
    int type[TOTAL] = { 0 }; // ���ڼ�¼��ͬ���͵�������������
};

struct Move {
    int x;
    int y;
    int score;
};

struct ZobristEntry {
    int score;
    int depth; // ���
};

class ZobristHash {
public:
    ULL zobrist_table[SIZE][SIZE][3]; // �������
    ULL current_hash; // ��ǰ��ϣֵ

    // ��ʼ���������
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

    // ����64λ�����
    ULL rand64() {
        return ((ULL)rand() << 32 ^ rand());
    }

    void UpdateHash(const int x, const int y, const int color) {
        current_hash ^= zobrist_table[x][y][color]; // ���¹�ϣֵ
    }
};

// ����ʽ����������ȽϺ���
int Compare(const void* a, const void* b) {
    return ((struct Move*)b)->score - ((struct Move*)a)->score;
}

// ������
class Board {
public:
    int board[SIZE][SIZE];  // ����״̬
    int board_record[SIZE][SIZE][4]; // �������������ĸ������״̬��¼
    TypeFlag my_flag;
    TypeFlag enemy_flag;
    Move final_move;


    // ���캯������ʼ�����̣�������Zobrist��ϣ����
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

    // �ж��Ƿ���������
    bool IsInBoard(const int x, const int y) {
        return (x >= 0 && x < SIZE && y >= 0 && y < SIZE);
    }

    // ����
    void SetPiece(const int x, const int y, const int color) {
        board[x][y] = color;
    }

    // ��������
    void RemovePiece(const int x, const int y) {
        board[x][y] = EMPTY;
    }

    // ��һ���ж��Ƿ�������
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

    // ����÷�
    int GetScore(TypeFlag* mine, TypeFlag* opponent) {
        int mine_score = 0, ene_score = 0, final = 0;
        // �жϱ�ʤ����
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
        // ���յ÷�
        final += mine_score - ene_score;
        return final;
    }

    // ��������״̬�����μ�¼
    void Reset(TypeFlag* mine) {
        // ��������״̬
        for (int i = 0; i < TOTAL; i++)
        {
            mine->type[i] = 0;
        }
        // �������̼�¼
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

    // �������̼�¼����ʾ�����ٿ��Ǵ�λ��
    void SetRep(const int x, const int y, const int left, const int right, const int dir[], const int dir_num) {
        // ������ʼ������
        int start_x = x + (left - 5) * dir[0];
        int start_y = y + (left - 5) * dir[1];
        // ��ǲ����ٿ��ǵ�λ��
        for (int i = left; i < right + 1; i++)
        {
            start_x += dir[0];
            start_y += dir[1];
            board_record[start_x][start_y][dir_num] = 1; //��ʾ�����ٿ�
        }
    }

    // ��ȡһֱ�������ӵ�״̬
    void GetLine(const int x, const int y, TypeFlag opponent, const int dir[], int* line) {
        int start_x = x + (-5 * dir[0]);
        int start_y = y + (-5 * dir[1]);
        // ��ȡֱ���ϵ�����״̬
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

    // �����ɵ�������Ϊ3ʱ�����γɵ�����
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

    // �����ɵ�������Ϊ2ʱ�����γɵ�����
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

    // �����ɵ�������Ϊ1ʱ�����γɵ�����
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

    // ���ÿһ���������γɵ�����
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
        }//���������Χ
        while (r_mine < 8)
        {
            if (line[r_mine + 1] != mine->flag_color) break;
            r_mine++;
        }//��������ҷ�Χ
        int l_range = l_mine, r_range = r_mine;
        while (l_range > 0)
        {
            if (line[l_range - 1] == opponent->flag_color) break;
            l_range--;
        }//���������Χ
        while (r_range < 8)
        {
            if (line[r_range + 1] == opponent->flag_color) break;
            r_range++;
        }//��������ҷ�Χ
        int range = r_range - l_range + 1;
        SetRep(x, y, l_mine, r_mine, dir, dir_num);//����ǰ������Ϊ����
        if (range < 5)
        {
            return;
        }
        int cu_range = r_mine - l_mine + 1;
        if (cu_range >= 5)
        {
            mine->type[WIN_FIVE]++;
        }
        else if (cu_range == 4) //�ȳ��ĺ����
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

    // �Ե�����ַ����ϵ����ͽ���ͳ��
    void EvaluatePoint(const int x, const int y, TypeFlag* mine, TypeFlag* opponent) {
        const int every_dir[4][2] = { {0,1},{1,0},{1,1},{-1,1} };
        for (int i = 0; i < 4; i++) {
            if (board_record[x][y][i] == 0)  //δ�����
                CheckLine(x, y, mine, opponent, every_dir[i], i);
        }
    }

    // ��������
    void Evaluate(TypeFlag* mine, TypeFlag* opponent) {
        Reset(mine);
        Reset(opponent);
        for (int x = 0; x < SIZE; x++) {
            for (int y = 0; y < SIZE; y++) {
                if (board[x][y] == mine->flag_color) { //���Ϊ�ҷ����Ӽ���������ɵ�����        
                    EvaluatePoint(x, y, mine, opponent);
                }
                else if (board[x][y] == opponent->flag_color) {//���Ϊ�Է����Ӽ���������ɵ�����
                    EvaluatePoint(x, y, opponent, mine);
                }
            }
        }
    }

    // �ж�ʤ������
    bool Win(const TypeFlag* mine, const TypeFlag* oppenent) { // ֻ��Լ������ӵ�һ��
        if (mine->type[WIN_FIVE] > 0) {
            return true;
        }
        else if (mine->type[LIVE_FOUR] > 0) {
            return true;
        }
        return false;
    }

    // ����ʽ����
    struct Move* GetMove(struct Move* move, TypeFlag* turn, TypeFlag* oppenent) {
        for (int i = 0; i < 145; i++) {
            move[i].x = -1;
            move[i].y = -1;
            move[i].score = -0x7fffffff + 10000010;// ��ֹ����int����
        }// ��ʼ��move
        int wide = 0;
        for (int x = 0; x < SIZE; x++) {
            for (int y = 0; y < SIZE; y++) {
                if (board[x][y] == EMPTY) {
                    SetPiece(x, y, turn->flag_color);
                    Evaluate(turn, oppenent);
                    int score = GetScore(turn, oppenent); // �жϵ÷�
                    move[wide].x = x;
                    move[wide].y = y;
                    move[wide].score = score; // ���µ�����
                    wide++;
                    RemovePiece(x, y);
                }
            }
        }
        qsort(move, 145, sizeof(Move), Compare); // ����
        return move;
    }

    // ���������Ĺ��
    void CaculateWide(int* wide, struct Move* move) {
        for (int i = 0; i < WIDTH; i++) {
            if (move[i].x != -1) {
                *wide = i + 1;
            }
        }
    }

    // ����
    int MaxSearch(const int x, const int y, const int depth, int alpha, int beta) {
#ifdef LIMIT_TIME
        // ����Ƿ�ʱ
        double elapsed_time_ms = (double)(clock() - start_time) * 1000.0 / CLOCKS_PER_SEC;
        if (elapsed_time_ms > step_time) {
            timeout_occurred = true; // ���ó�ʱ��־
            return  alpha;  // ��ʱ�򷵻ص�ǰalpha��betaֵ
        }
#endif
        // ��������״̬
        Reset(&my_flag);
        Reset(&enemy_flag);

        //�Ե�һ����Ч����������
        if (x != -1 && y != -1) {
            EvaluatePoint(x, y, &enemy_flag, &my_flag);
            // ����Է�Ӯ�ˣ����ؼ�Сֵ
            if (Win(&enemy_flag, &my_flag)) return MIN_SCORE;
        }


        // ����ﵽ����������ƣ�ֱ�ӷ��ص�ǰ����ĵ÷�
        if (depth == 0) {
            Evaluate(&my_flag, &enemy_flag);
            int sco = GetScore(&my_flag, &enemy_flag);
            return sco;
        }
        // ��ȡ����ʽ���������������λ��
        struct Move move[145];
        GetMove(move, &my_flag, &enemy_flag);

        // �������λ�õ�����
        int wide = 0;
        CaculateWide(&wide, move);
        // ����޷����壬��ֱ�ӷ��ص�ǰ����ĵ÷�
        if (wide == 0) {
            Evaluate(&my_flag, &enemy_flag);
            int sco = GetScore(&my_flag, &enemy_flag);
            return sco;
        }
        // �������п���λ��
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
                    if (alpha == MAX_SCORE) { //���ѵ���ʤ��ֱ�ӷ���
                        return alpha;
                    }
                }
            }
            if (alpha >= beta)
                break;   //��֦
        }
        return alpha;
    }

    // ��С��
    int MinSearch(const int x, const int y, const int depth, int alpha, int beta) {
#ifdef LIMIT_TIME
        // ����Ƿ�ʱ
        double elapsed_time_ms = (double)(clock() - start_time) * 1000.0 / CLOCKS_PER_SEC;
        if (elapsed_time_ms > step_time) {
            timeout_occurred = true; // ���ó�ʱ��־
            return  beta;  // ��ʱ�򷵻ص�ǰalpha��betaֵ
        }
#endif
        // ��������״̬
        Reset(&my_flag);
        Reset(&enemy_flag);
        // ������ǰλ��
        EvaluatePoint(x, y, &my_flag, &enemy_flag);
        //����ҷ�Ӯ�ˣ����ؼ���ֵ
        if (Win(&enemy_flag, &my_flag)) return MAX_SCORE;
        // ����ﵽ����������ƣ�ֱ�ӷ��ص�ǰ����ĵ÷�
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
                break; //��֦
        }
        return beta;
    }

    // ������ʾ����
    void DisplayBoard() {
        printf("   ");
        for (int i = 0; i < SIZE; ++i) {
            printf("%2d  ", i); // ������ĸ����
        }
        printf("\n");

        for (int x = 0; x < SIZE; ++x) {
            printf("%2d ", x); // �����������
            for (int y = 0; y < SIZE; ++y) {
                string c = "+"; // �����
                if (board[x][y] == BLACK) {
                    c = "\b��"; // ��ɫ����
                }
                else if (board[x][y] == WHITE) {
                    c = "\b��"; // ��ɫ����
                }
                cout << c;

                if (y < SIZE - 1) { // ���һ�в���Ҫ��ӡ����
                    printf("---");
                }
            }
            printf("\n");

            if (x < SIZE - 1) { // ���һ�в���Ҫ��ӡ����
                printf("   |");
                for (int i = 1; i < SIZE; ++i) {
                    printf("   |"); // ���߽��㴦
                }
                printf("\n");
            }
        }
        printf("\n");
    }
};

// AI��
class GomokuAI {
private:
    Board chess_board;            // ����ʵ��
public:
    // ���캯��
    GomokuAI() {  // ��ʼ������
        srand(time(NULL));  // ��ʼ�����������
        left_time = TOTAL_THINK_TIME;
        step_time = MAX_STEP_THINK_TIME;
        current_step = 0;  // ��ʼ��Ϊ��һ��
    }

    // ���㵱ǰ������˼��ʱ��
    int CalculateStepTime() {
        int base_time = 1600;  // ÿ���Ļ���ʱ��
        // ��̬����ÿ�������˼��ʱ��
        if (current_step < 15) {
            // ���ֽ׶�
            return min(base_time + (current_step * 20), MAX_STEP_THINK_TIME);
        }
        else if (current_step < 50) {
            // ���ڽ׶�
            return min(base_time + (current_step * 10), MAX_STEP_THINK_TIME);
        }
        else {
            // ���ڽ׶Σ�������ʹ��ʣ���ʱ��
            return min(MAX_STEP_THINK_TIME, left_time / (MAX_SET_NUM - current_step + 1));
        }
    }

    // ��ʼ��AI��ɫ
    void StartGame(const int color) {
        chess_board.my_flag.flag_color = color;
        chess_board.enemy_flag.flag_color = 3 - color;
        printf("OK\n");
        fflush(stdout);
    }

    // �ж�ʤ������
    bool Win(const TypeFlag* mine, const TypeFlag* oppenent) { // ֻ��Լ������ӵ��ҷ�
        if (mine->type[WIN_FIVE] > 0) {
            return true;
        }
        else if (mine->type[LIVE_FOUR] > 0) {
            return true;
        }
        return false;
    }

    // Ԥ����Ƿ���WIN_FIVE�ľ��棬����У��Ͳ���������
    bool PreCheckWinFive(int& pre_x, int& pre_y) {
        for (int i = 0; i < SIZE; ++i) {
            for (int j = 0; j < SIZE; ++j) {
                if (chess_board.board[i][j] == EMPTY) {
                    // �ĸ�����
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

    // ����Է�����
    void PlaceOpponentMove(const int x, const int y) {
        chess_board.SetPiece(x, y, chess_board.enemy_flag.flag_color);
    }

    // �ҵ���õĵ�
    pair<int, int> FindBestMove() {
        start_time = clock();
        int alpha = -0x7fffffff;
        int beta = 0x7fffffff;
        int best_x = -1;
        int best_y = -1;
        // ��ֹ����Ϊ�˶¶Է����壬�������Լ�����
        int pre_x = -1, pre_y = -1;
        if (PreCheckWinFive(pre_x, pre_y)) {
            return { pre_x, pre_y };
        }
        for (cur_depth = MAX_DEPTH; cur_depth >= 2; cur_depth -= 3) {
            // ����û���
            timeout_occurred = false; // ���ó�ʱ��־
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

    // ����AI�غ�
    void Turn() {
        start_time = clock(); // ��ʼ��ʱ
        current_step++;       // ��¼����
        step_time = CalculateStepTime();  // ���µ�ǰ����������˼��ʱ��
        pair<int, int> move = FindBestMove();
        chess_board.SetPiece(move.first, move.second, chess_board.my_flag.flag_color);
#ifdef DISPLAY
        chess_board.DisplayBoard();
#endif
        printf("%d %d\n", move.first, move.second);
        fflush(stdout);
        double time_taken = (double)(clock() - start_time) / CLOCKS_PER_SEC * 1000.0;
        left_time -= int(time_taken); // ʣ��ʱ�����
#ifdef TEST
        // ���˼��ʱ��
        printf("Time taken: %.2f ms\n", time_taken);
        printf("Left time: %d ms\n", left_time);
        fflush(stdout);
#endif
    }
};

int main() {
    GomokuAI ai;  // ����AIʵ��
    char command[18];
    int x, y, color;
    while (scanf("%s", command) != EOF) {
        if (strcmp(command, "START") == 0) {
            scanf("%d", &color);
            ai.StartGame(color);  // ������Ϸ
        }
        else if (strcmp(command, "PLACE") == 0) {
            scanf("%d %d", &x, &y);
            ai.PlaceOpponentMove(x, y);  // ��������
        }
        else if (strcmp(command, "TURN") == 0) {
            ai.Turn();  // AI����
        }
        else if (strcmp(command, "END") == 0) {
            break;  // ������Ϸ
        }
    }
    return 0;
}