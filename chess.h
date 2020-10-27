#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/*-MACRO DEFINITIONS----------------------------------------------------------*/

#define N_PCS   6   // number of pieces
#define PAWN    1   // piece ids
#define KNIGHT  2
#define BISHOP  3
#define ROOK    4
#define QUEEN   5
#define KING    6
#define EP_FLAG 99  // en passant flag

/*-TYPE DEFINITIONS-----------------------------------------------------------*/

// playing board
typedef struct Board_
{
    int** b; // 8x8 piece map {-N_PIECES..N_PIECES}
    int** t; // 8x8 touch map {0,1}
} Board;
// type of piece movement functions
typedef int*** (*Piece_f)(Board, int*, int);

/*-FUNCTION DECLARATIONS------------------------------------------------------*/

// game / board state
int play_game(Board);
int can_move(Board, int);
int* find(Board, int, int);
// move evaluation
int is_legal(Board, int**, int);
int is_hit(Board, int*, int);
Board make_move(Board, int**, int);
// handling boards
Board cpy_board(Board);
void free_board(Board);
// IO
int** read_move(char*);
void print_board(Board, int);
// helper functions
int signum(int);
int*** malloc_from_tmp(int[][64][2], int[2]);
void cast_dydx(Board, int[][64][2], int*, int, int, int, int[2]);
// piece movement functions
int*** P__(Board, int*, int ); // PAWN
int*** N__(Board, int*, int ); // KNIGHT
int*** B__(Board, int*, int ); // BISHOP
int*** R__(Board, int*, int ); // ROOK
int*** Q__(Board, int*, int ); // QUEEN
int*** K__(Board, int*, int ); // KING
// array of piece movement functions
Piece_f list_moves[N_PCS] = {P__,N__,B__,R__,Q__,K__};

/*-FUNCTION DEFINITIONS-------------------------------------------------------*/

/*-helper functions-----------------------------------------------------------*/
int signum(int x)
{
    return (x>0) - (x<0);
}

void cast_dydx(Board B, int tmp[][64][2],
                int* pos, int dy, int dx,
                int player, int count[2])
{
    // go as far as possible in direction pointed to by {dy,dx}
    int y, x, cap;
    for(int i=1; 1; i++)
    {
        y = pos[0] + dy*i, x = pos[1] + dx*i;

        if(y<0 || y>=8 || x<0 || x>=8) return;

        cap = (B.b[y][x]) && (abs(B.b[y][x])!=EP_FLAG);

        if(signum(B.b[y][x]) != player)
        {
            tmp[cap][count[cap]][0] = y;
            tmp[cap][count[cap]][1] = x;

            count[cap]++;
        }
        if(cap) return;
    }
}

int*** malloc_from_tmp(int tmp[][64][2], int count[2])
{
    // create return pointer
    int*** moves = malloc(2*sizeof(int**));

    for(int i=0; i<2; i++)
    {
        // if there are no moves in this list, make it NULL
        if(!count[i])
        {
            moves[i] = NULL;
            continue;
        }
        // otherwise, create array
        moves[i] = malloc((count[i]+1) * sizeof(int*));
        // first element = number of elements
        moves[i][0] = malloc(sizeof(int));
        *moves[i][0] = count[i];
        // the rest of the elements are the moves.
        for(int j=0; j<count[i]; j++)
        {
            // make space
            moves[i][j+1] = malloc(2*sizeof(int));
            // copy values
            for(int k=0;k<2; k++) moves[i][j+1][k] = tmp[i][j][k];
        }
    }
    return moves;
}

/*-Piece movement functions---------------------------------------------------*/
int*** P__(Board B, int* pos, int player) // PAWN
{
    int tmp[2][64][2] = {0};
    int count[2] = {0};
    int y, x, dx;

    // first non-captures
    y = pos[0] + player;
    x = pos[1];
    // if nothing's directly in front of the pawn
    if(!B.b[y][x])
    {
        // it can move forward 1
        tmp[0][count[0]][0] = y;
        tmp[0][count[0]][1] = x;
        count[0]++;

        y += player;
        // if pawn is at home & nothing is 2 squares ahead
        if(!(B.t[y][x] || B.b[y][x]))
        {
            tmp[0][count[0]][0] = y;
            tmp[0][count[0]][1] = x;
            count[0]++;
        }
    }
    y = pos[0] + player;
    // now captures
    for(int i=0; i<2; i++)
    {
        x = pos[1] + 1-2*i;
        // check if it's possible to capture in this direction
        if(signum(B.b[y][x]) == -player)
        {
            tmp[1][count[1]][0] = y;
            tmp[1][count[1]][1] = x;
            count[1]++;
        }
    }
    return malloc_from_tmp(tmp, count);
}

int*** N__(Board B, int* pos, int player) // KNIGHT
{
    int tmp[2][64][2];
    int count[2] = {0};
    // candidate moves
    int dydx[8][2] =
    {
        { 2,-1},{ 2, 1},{-2,-1},{-2, 1},
        { 1,-2},{ 1, 2},{-1,-2},{-1, 2}
    };

    int x, y, cap;
    for(int i=0; i<8; i++)
    {
        y=pos[0]+dydx[i][0], x=pos[1]+dydx[i][1];

        if( y>=0 && y<8 && // within bounds
            x>=0 && x<8 && // ^^^
            signum(B.b[y][x]) != player // not occupied by friendly piece
        ){
            // cap = 1 if capture, 0 if not
            cap = B.b[y][x];
            cap = (abs(cap)!=EP_FLAG) && (cap);
            // copy over
            tmp[cap][count[cap]][0] = y;
            tmp[cap][count[cap]][1] = x;
            count[cap]++;
        }
    }
    return malloc_from_tmp(tmp, count);
}

int*** B__(Board B, int* pos, int player) // BISHOP
{
    // a bishop on an open board can reach 13 squares from the center
    int tmp[2][64][2];
    int count[2] = {0};

    int dy, dx;
    for(int a=0; a<2; a++)
    for(int b=0; b<2; b++)
    {
        dy = 1-2*a;
        dx = 1-2*b;
        //printf("dy=%d dx=%d\n",dy,dx);
        cast_dydx(B, tmp, pos, dy, dx, player, count);
    }
    return malloc_from_tmp(tmp, count);
}

int*** R__(Board B, int* pos, int player) // ROOK
{
    // a rook on an open board can always reach 14 squares
    int tmp[2][64][2];
    int count[2] = {0};

    int dy, dx;
    for(int a=0; a<2; a++)
    for(int b=0; b<2; b++)
    {
        dy =  a*(1-2*b);
        dx = !a*(1-2*b);
        cast_dydx(B, tmp, pos, dy, dx, player, count);
    }
    return malloc_from_tmp(tmp, count);
}

int*** Q__(Board B, int* pos, int player) // QUEEN
{
    // a queen on an open board can reach 27 squares from the center
    int tmp[2][64][2];
    int count[2] = {0};

    int dy, dx;
    for(int dy=-1; dy<2; dy++)
    for(int dx=-1; dx<2; dx++)
    {
        if(!dy && !dx) continue;

        cast_dydx(B, tmp, pos, dy, dx, player, count);
    }
    return malloc_from_tmp(tmp, count);
}

int*** K__(Board B, int* pos, int player) // KING
{
    int tmp[2][64][2];
    int count[2] = {0};

    int y, x, cap;
    for(int dy=-1; dy<=1; dy++)
    for(int dx=-1; dx<=1; dx++)
    {
        if(dy || dx)
        {
            y = pos[0] + dy;
            x = pos[1] + dx;

            if( y>=0 && y<8 && // within bounds
                x>=0 && x<8 && // ^^^
                signum(B.b[y][x]) != player // not occupied by friendly piece
            ){
                // cap = 1 if capture, 0 if not
                cap = B.b[y][x];
                cap = (abs(cap)!=EP_FLAG) && (cap);
                // copy over
                tmp[cap][count[cap]][0] = y;
                tmp[cap][count[cap]][1] = x;

                count[cap]++;
            }
        }
    }
    return malloc_from_tmp(tmp, count);
}

/*-Move evaluation functions--------------------------------------------------*/
Board make_move(Board B, int** move, int player)
{
    // helpful shortcuts
    int y[2] = {move[0][0],move[1][0]};
    int x[2] = {move[0][1],move[1][1]};
    int dy = y[1]-y[0], dx = x[1]-x[0];

    int piece = B.b[y[1]][x[0]];

    // special cases go here. (en passant, castling)
    switch(abs(piece))
    {
        case KING: // castling
        {
            if(!dy && abs(dx)==2)
            {
                // take care of the rook
                B.b[y[0]][(int)(4+(3.5f*signum(dx)))] = 0;
                B.b[y[0]][x[0]+signum(dx)] = ROOK*player;
            }
        } break;
        case PAWN:
        {
            // enable en passant
            if(!dx && dy==2*player)
            B.b[y[0]+player][x[0]] = player*EP_FLAG;

            // enable pawn promotions
            else if(y[1]==(int)(4+(3.5*player)))
            B.b[y[0]][x[0]] = QUEEN*player;
        } break;
    }

    // perform desired move
    B.b[y[1]][x[1]] = B.b[y[0]][x[0]];
    B.b[y[0]][x[0]] = 0;

    // remember these squares were touched
    B.t[y[0]][x[0]] = B.t[y[1]][x[1]] = 0;

    // return new board state (useful for when making a move from cpy_board)
    return B;
}

int is_legal(Board B, int** move, int player)
{
    // indices & stuff
    int y[2] = {move[0][0], move[1][0]};
    int x[2] = {move[0][1], move[1][1]};
    int dy = y[1]-y[0];
    int dx = x[1]-x[0];
    // pieces
    int p[2] = {B.b[y[0]][x[0]], B.b[y[1]][x[1]]};
    // filled from other functions
    int*** list;
    int* pos;
    Board H;
    // logical
    int ischeck;
    int islegal;

    if(signum(p[0]) != player) return 0;

    // check for special cases
    switch(abs(p[0]))
    {
        case KING:
        {
            // the move is to castle
            if( // moving correctly
                dx==2 && !dy &&
                // is on the right square
                *y==4+(3.5*player) && *x==3 &&
                // has not touched king or rook yet
                !(B.t[*y][*x] || B.t[*y][dx>0?7:0])
            )
            {
                for(int i=3; i>0 && i<6; i+=dx/2)
                    // make sure other's no piece there
                    if(B.b[*y][3+i]) return 0;

                pos = malloc(2*sizeof(int));

                for(int i=3; i>1 && i<7; i+=dx/2)
                {
                    pos[0] = *y;
                    pos[1] = 3+i;
                    // make sure you're not in check
                    if(is_hit(B, pos, player))
                    {
                        free(pos);
                        return 0;
                    }
                    free(pos);
                }
                return 1;
            }
        } break;
    }
    // for normal moves

    // see if the move is pseudo-legal
    list = list_moves[abs(p[0])-1](B, move[0], player);

    // only check captures or non-captures-- whichever is needed
    if(list[!p[1]])
    {
        // delete elements
        for(int i=**list[!p[1]]; i>=0; i--) free(list[!p[1]][i]);
        free(list[!p[1]]); // delete pointer
    }

    if(!list[!!p[1]]) return 0;

    for(int i=1; i<=**list[!!p[1]]; i++)
    {
        //grab this pointer
        pos = list[!!p[1]][i];
        // if we've found the desired move
        if( pos[0] == move[1][0]
        &&  pos[1] == move[1][1] )
        {
            // free everything
            for(;i<=**list[!!p[1]]; i++) free(list[!!p[1]][i]);

            islegal = 1;
            break;
        }
        free(pos);
    }
    free(list[!!p[1]]);
    free(list);

    if(!islegal) return 0;

    // now we just need to make sure the move doesn't hang the king
    H = make_move(cpy_board(B), move, player);
    // find the king
    pos = find(H, KING, player);
    // see if he's in check
    ischeck = is_hit(H, pos, player);
    // free stuff
    free(pos); free_board(H);

    return !ischeck;
}

int is_hit(Board B, int* pos, int player)
{
    // for list_moves[]();
    int*** list;
    int y, x;

    for(int i=0; i<N_PCS; i++)
    {
        // get all moves possible from piece from pos
        list = list_moves[i](B, pos, player);

        // we don't need to check the non-captures
        if(list[0])
        {
            for(int j=**list[0]; j>=0; j--) free(list[0][j]); // elements
            free(list[0]); // delete pointer
        }

        // check all potential captures here
        if(!list[1]) return 0;

        for(int j=1; j<=**list[1]; j++)
        {
            y = list[1][j][0];
            x = list[1][j][1];

            // if this is the right type of piece
            if(abs(B.b[y][x]) == i+1)
            {
                // free everything
                for(;j<**list[1]; j++) free(list[1][j]);
                free(list[1][0]); free(list[1]);

                return 1;
            }
            free(list[1][j]);
        }
        free(list[1]);
        free(list);
    }
    return 0;
}

/*-Game / board state handling functions--------------------------------------*/
int play_game(Board B)
{
    char input[100]; // input buffer

    // begin game
    for(int i=1; 1; i=-i)
    {
        print_board(B, i);

        // check for mate at the start of every move.
        if(!can_move(B, i)) // if player has no valid moves
        {
            // find the king
            int* pos = find(B, KING, i);
            // is he in check?
            int isCheck = is_hit(B, pos, i);
            free(pos); // (done with this)

            return isCheck * -i; // return {-1, 0, 1}
        }
        else
        {
            // otherwise, the game goes on.
            printf("\n%s's turn: > ", (i>0) ? "White" : "Black");

            // get move ("g1f3 instead of Nf3" - PGN is HARD to program)
            fgets(input, 100, stdin);
            int** m = read_move(input); // translate input; move is in m

            if(!m) // if input is invalid
            {
                puts("\n (?) Invalid input! Try again.\n");
                i =- i;
                continue;
            }

            if(is_legal(B, m, i))
            {
                B = make_move(B, m, i);
                //remove opponent's en passant flag if it exists
                int* pos = find(B, EP_FLAG, -i);
                if(pos)
                {
                    B.b[pos[0]][pos[1]] = 0;
                    free(pos);
                }
            }
            else
            {
                printf("\n (!) Illegal move! Try again.\n");
                i=-i;
            }
            free(m);
        }
    }
}

int can_move(Board B, int player)
{
    // for list_moves[]();
    int*** list;
    // for feeding moves into is_legal
    int** move = malloc(2*sizeof(int*));
    move[0]    = malloc(2*sizeof(int));
    // for iterating through moves
    int piece;
    int* i = &move[0][0];
    int* j = &move[0][1];
    // for determining if the move leaves you in check
    int* pos;
    Board H;
    int ischeck;

    // iterate through entire board
    for(*i=0; *i<8; (*i)++)
    for(*j=0; *j<8; (*j)++)
    {
        piece = B.b[*i][*j];
        //
        if(signum(piece)==player)
        {
            // get the list of possible moves
            list = list_moves[abs(piece)-1](B, move[0], player);

            // iterate through possible moves
            for(int k=0; k<2; k++)
            {
                if(!list[k]) continue;

                for(int l=1; l<=**list[k]; l++)
                {
                    // copy move destination
                    move[1] = list[k][l];
                    /*
                     * We know the move is pseudo-legal because it was
                     * generated using the move-generation functions. Just
                     * make sure it's not check.
                     */
                    H = make_move(cpy_board(B), move, player);
                    // find the king
                    pos = find(H, KING, player);
                    // see if he's in check
                    ischeck = is_hit(H, pos, player);
                    // free stuff
                    free(pos); free_board(H);

                    if(!ischeck)
                    {
                        // free everything
                        for(;k<2; k++)
                        {
                            if(list[k])
                            {
                                for(;l<=**list[k]; l++) free(list[k][l]);
                            }
                            free(list[k]);
                        }
                        return 1;
                    }
                    free(list[k][l]);
                }
                free(list[k]);
            }
        }
    }
    return 0;
}

int* find(Board B, int piece, int player)
{
    int* pos = malloc(sizeof(int)*2);

    for(int i=0; i<8; i++)
    for(int j=0; j<8; j++)
    {
        if(B.b[i][j] == piece*player)
        {
            pos[0] = i, pos[1] = j;
            return pos;
        }
    }
    return NULL;
}

/*-Board handling functions---------------------------------------------------*/
Board cpy_board(Board B)
{
    Board C;

    C.b = malloc(8*sizeof(int*));
    C.t = malloc(8*sizeof(int*));

    for(int i=0; i<8; i++)
    {
        C.b[i] = malloc(8*sizeof(int));
        C.t[i] = malloc(8*sizeof(int));

        for(int j=0; j<8; j++)
        {
            C.b[i][j] = B.b[i][j];
            C.t[i][j] = B.t[i][j];
        }
    }
    return C;
}

void free_board(Board B)
{
    for(int i=0; i<8; i++)
    {
        free(B.b[i]);
        free(B.t[i]);
    }
    free(B.b);
    free(B.t);
}

/*-IO functions---------------------------------------------------------------*/
int** read_move(char* str)
{
    //if(strlen(str)!=4) return NULL;
    const char abcs[8] = "hgfedcba";
    const char nums[8] = "12345678";

    int** r = malloc(sizeof(int*)*2); // return array
    for(int i=0;i<2;i++) r[i] = malloc(sizeof(int)*2);

    char* c;
    for(int i=0; i<4; i++)
    {
        if((c = strchr(abcs, str[i]))) // if youre using abc notation...
        r[i/2][1-i%2] = (int)(c-abcs);

        else if((c = strchr(nums, str[i]))) // number notation...
        r[i/2][1-i%2] = (int)(c-nums);

        else return NULL;
    }
    return r;
}

void print_board(Board B, int player)
{
    int v, p = -player;
    for(int i=7*(p<0); i<8 && i>=0; i+=p)
    {
        printf("\n\t%d |  ", i+1); // label 123
        for(int j=7*(p<0); j<8 && j>=0; j+=p)
        {
            int v = B.b[i][j];
            printf( "%2c ",
                abs(v)==EP_FLAG || !v
                    ? i%2 == j%2 ? ':' : '~'    // empty squares
                    : "kqrbnp_PNBRQK"[6+v]      // pieces
            );
        }
    }
    // bottom border
    printf("\n\n\t      ");
    for(int j=0;j<7*3+1;j++) printf("-");
    // label abc
    printf("\n\n\t    ");
    for(int i=7*(p<0);i<8&&i>=0;i+=p) printf(" %2c", "hgfedcba"[i]);
    printf("\n\n");
}