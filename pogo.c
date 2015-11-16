/* -------------------------------------------------------------------
 *  pogo - A small POGO game simulator. Based on the POGO board-game.
 *  Laurent GREGOIRE for the computer game. (laurent.gregoire@icam.fr)
 *  Copyright (c) FilsFils international for the original game.
 * -------------------------------------------------------------------
 *  pogo - Un petit simulateur du jeu du meme nom. Base sur le jeu
 *  de plateau POGO.
 * -------------------------------------------------------------------
 *  This program is released as a freeware. In place of a unreadable
 *  licence, here is a (SQLite-style) blessing:
 *  	May you do good and not evil.
 *  	May you find forgiveness for yourself and forgive others.
 *  	May you share freely, never taking more than you give.
 * -------------------------------------------------------------------
 *  Ce programme est mis dans le domaine public. Utilisez-le comme
 *  bon vous semble!
 * -------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define EMPTY	0
#define BLACK	1
#define WHITE	2

#define MAX_DEEP 16

#define LOST_W	-999
#define WON_W	999

static const char pogo_version[] = "@VER@";

int max_deep[3] = { 5, 8, 10 };

int mmax[3][MAX_DEEP] = {
	{ 10, 6, 6, 3, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 99, 8, 8, 4, 4, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0 },
	{ 99, 10, 8, 6, 5, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0 }
};

typedef struct
{
	int n;
	int owner;
	int c[12];
} t_cell;

typedef struct
{
	t_cell cell[3][3];
	int c_w;
	int b_w;
} t_board;

typedef struct
{
	int n1;
	int n2;
	int n3;
	long c_wt;
	int ndw;
	long d_wt;
} t_dstat;

typedef struct
{
	int total;
	t_dstat dstat[MAX_DEEP];
} t_stat;

typedef struct
{
	int human_black;
	int human_white;
	int verbose;
	int max_deep;
	int *mmax;
} t_cfg;

static t_stat stats;
static t_cfg  cfg;

const char *disp_coord(int i, int j)
{
	static const char letters[4] = "ABC";
	static char retval[8];
	snprintf(retval, sizeof(retval), "%c,%d", letters[j], i + 1);
	return retval;
}

const char *strwho(int who)
{
	return (who == BLACK ? "BLACK" : "WHITE");
}

void won(int who)
{
	fprintf(stdout, "%s won!\n", strwho(who));
	exit(0);
}

void lost(int who)
{
	fprintf(stdout, "%s lost!\n", strwho(who));
	exit(1);
}

void display_stats(int who)
{
	char c;
	int i;
	fprintf(stderr,
	 "deep     boards      moves    m/b    eval'ed    e/b mm   e/m  "
	//  01 0123456789 0123456789  99.99 0123456789  99.99 99 999.9%
	 "    cw  bw-cw\n");
	//-999.9 -999.9
	c = (who == BLACK ? 'B' : 'W');
	for (i = 0; i <= cfg.max_deep; i++)
	{
		int n1 = stats.dstat[i].n1;
		int n2 = stats.dstat[i].n2;
		int n3 = stats.dstat[i].n3;
		int ndw = stats.dstat[i].ndw;
		long c_wt = stats.dstat[i].c_wt;
		long d_wt = stats.dstat[i].d_wt;
		int n2n1 = 0;
		int n3n1 = 0;
		int n3n2 = 0;
		int c_w  = 0;
		int d_w  = 0;
		if (n1 != 0) n2n1 = (long)n2 * 100L / (long)n1;
		if (n1 != 0) n3n1 = (long)n3 * 100L / (long)n1;
		if (n2 != 0) n3n2 = (long)n3 * 1000L / (long)n2;
		if (n2 != 0) c_w = c_wt * 10L / n2;
		if (ndw != 0) d_w = d_wt * 10L / ndw;
		fprintf(stderr, "%c %2d %10d %10d  %2d.%02d %10d  "
			"%2d.%02d %2d %3d.%01d%% %4d.%01d %4d.%01d\n",
			c, i, n1, n2, n2n1 / 100, n2n1 % 100,
			n3, n3n1 / 100, n3n1 % 100,
			cfg.mmax[i], n3n2 / 10, n3n2 % 10,
			c_w / 10, abs(c_w % 10),
			d_w / 10, abs(d_w % 10));
		c = (c == 'W' ? 'B' : 'W');
	}
	fprintf(stderr, "\n");
}

void display_board(const t_board *b)
{
	int i, j, k, nmax = 5;

	for (i = 0; i < 3; i++)
	for (j = 0; j < 3; j++)
	{
		if (b->cell[i][j].n > nmax)
			nmax = b->cell[i][j].n;
	}

	fprintf(stdout, "         A           B           C      \n");
	fprintf(stdout, "   +-----------+-----------+-----------+\n");
	for (i = 0; i < 3; i++)
	{
		for (k = 0; k < nmax; k++)
		{
			if (k == nmax - 3)
				fprintf(stdout, " %d ", i + 1);
			else	fprintf(stdout, "   ");
			for (j = 0; j < 3; j++)
			{
				int c = b->cell[i][j].c[nmax - k - 1];
				char *d = "       ";
				switch(c) {
					case BLACK: d = "@@@@@@@"; break;
					case WHITE: d = "[     ]"; break;
				}
				fprintf(stdout, "|  %s  ", d);
			}
			fprintf(stdout, "|\n");
		}
		fprintf(stdout, "   +-----------+-----------+-----------+\n");
	}
	fprintf(stdout, "\n");
}

void set_cell(t_board *b, int i, int j, t_cell *c)
{
	memcpy(&(b->cell[i][j]), c, sizeof(t_cell));
}

void init_board(t_board *b)
{
	int i;
	t_cell white2 = { 2, WHITE, { WHITE, WHITE, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0 } };
	t_cell black2 = { 2, BLACK, { BLACK, BLACK, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0 } };
	memset(b, '\0', sizeof(t_board));
	for (i = 0; i < 3; i++)
	{
		set_cell(b, 0, i, &white2);
		set_cell(b, 2, i, &black2);
	}
}

int evaluate_board(int who, t_board *b)
{
	int i, j, k, n1 = 0, n2 = 0, n3 = 0, n4 = 0, n5 = 0, n6 = 0, f, n;
	for (i = 0; i < 3; i++)
	for (j = 0; j < 3; j++)
	{
		t_cell *c = &(b->cell[i][j]);
		if (c->owner == who)
		{	n1++;
			f = 1;
			for (k = c->n - 1; k >= 0; k--)
			if (c->c[k] == who)
			{	if (f) n3++;
			} else
			{	f = 0;
				n5++;
			}
		}
		else if (c->owner != EMPTY)
		{	n2++;
			f = 1;
			for (k = c->n - 1; k >= 0; k--)
			if (c->c[k] != who)
			{	if (f) n4++;
			} else
			{	f = 0;
				n6++;
			}
		}
	}
	if (n1 == 0) return LOST_W;
	if (n2 == 0) return WON_W;
	n = n1 - n2;
	if (n1 == 1) n -= 2;
	if (n2 == 1) n += 2;
	n *= 5;
	n += (n3 - n4) * 2;
	n += (n5 - n6);
	return n;
}

int check_move(int i, int j, int k, int l, int n)
{
	int retval = 0;
	int d = abs(i - k) + abs(j - l);

	if (n == 1) retval = (d == 1);
	else if (n == 2) retval = (d == 2);
	else retval = (d == 3 || d == 1);
/*	fprintf(stdout, "check_move: dist(%s-", disp_coord(i, j));
	fprintf(stdout, "%s)=%d, n=%d, ok=%d\n",
		disp_coord(k, l), d, n, retval); */
	return retval;
}

void adjust_owner(t_cell *c)
{
	if (c->n == 0)
		c->owner = EMPTY;
	else	c->owner = c->c[c->n - 1];
}

void do_move(t_board *b, t_cell *c1, t_cell *c2, int n)
{
	int i;
	assert(c1->n >= n);
	for (i = 0; i < n; i++)
	{
		c2->c[c2->n + i] = c1->c[c1->n - n + i];
		c1->c[c1->n - n + i] = EMPTY;
	}
	c1->n -= n;
	c2->n += n;
	adjust_owner(c1);
	adjust_owner(c2);
}

void ask_pos(const char *msg, int *i, int *j)
{
	static char input[64];
	*i = -1;
	*j = -1;
	while(1)
	{
		fprintf(stdout, "%s", msg);
		fgets(input, sizeof(input), stdin);
		if (input[0] >= 'a' && input[0] <= 'c')
			*j = input[0] - 'a';
		else	*j = input[0] - 'A';
		*i = input[1] - '1';
		if (*i >= 0 && *i < 3 && *j >= 0 && *j < 3)
			return;
		fprintf(stdout, "incorrect input. try again.\n");
	}
}

void ask_n(const char *msg, int *n)
{
	static char input[64];
	*n = -1;
	while(1)
	{
		fprintf(stdout, "%s", msg);
		fgets(input, sizeof(input), stdin);
		*n = input[0] - '0';
		if (*n > 0 && *n <= 3)
			return;
		fprintf(stdout, "incorrect input. try again.\n");
	}
}

void human_play(int who, t_board *b)
{
	int i, j, k, l, n;
	static char prompt[128];
	t_cell *c1, *c2;
	while(1)
	{
		
		snprintf(prompt, sizeof(prompt), "%s, your move? FROM > ",
			strwho(who));
		ask_pos(prompt, &i, &j);
		ask_pos("                    TO > ", &k, &l);
		ask_n("              HOW MANY > ", &n);
		c1 = &(b->cell[i][j]);
		c2 = &(b->cell[k][l]);
		if (c1->owner == who && n <= c1->n &&
			check_move(i, j, k, l, n))
		{
			int w;
			do_move(b, c1, c2, n);
			w = evaluate_board(who, b);
			display_board(b);
			if (w >= WON_W)	won(who);
			if (w <= LOST_W) lost(who);
			return;
		}
		fprintf(stdout, "Invalid move. Try again.\n");
	}
}

/*if (deep < 0)
{
	for (r = 0; r < deep; r++)
		fprintf(stdout, "   ");
	fprintf(stdout, "%s MOVE: (%s) -> ",
		strwho(who), disp_coord(i, j));
	fprintf(stdout, "(%s) [%d] w=%d/%d\n",
		disp_coord(k, l), n, b2->c_w, b2->b_w);
}*/

int qsort_cmp_fct(const void *a, const void *b)
{
	int delta = (((t_board*)b)->c_w - ((t_board*)a)->c_w);
	if (delta == 0) return (rand() % 3 - 1);
	return delta;
}

int move_computer(int deep, int who, t_board *b)
{
	t_board boards[64];
	t_board *b2;
	t_cell *c1, *c2;
	int i, j, k, l, m = 0, n, b_wmax, c_wmax, d_wmax;
	stats.dstat[deep].n1++;
	for (i = 0; i < 3; i++)
	for (j = 0; j < 3; j++)
	{
		t_cell *c = &(b->cell[i][j]);
		if (c->owner != who) continue;
		for (n = 1; n <= c->n && n <= 3; n++)
		for (k = 0; k < 3; k++)
		for (l = 0; l < 3; l++)
		if (i != k || j != l)
		if (check_move(i, j, k, l, n))
		{
			b2 = &(boards[m]);
			memcpy(b2, b, sizeof(t_board));
			c1 = &(b2->cell[i][j]);
			c2 = &(b2->cell[k][l]);
			do_move(b2, c1, c2, n);
			b2->c_w = evaluate_board(who, b2);
			b2->b_w = b2->c_w;
			stats.total++;
			stats.dstat[deep].n2++;
			stats.dstat[deep].c_wt += b2->c_w;
			m++;
	}	}

	qsort(boards, m, sizeof(t_board), qsort_cmp_fct);
	if (m > cfg.mmax[deep]) m = cfg.mmax[deep];

	d_wmax = LOST_W;
	for (i = 0; i < m; i++)
	{
		b2 = &(boards[i]);
		if (b2->c_w == WON_W)
			break;
		if (deep < cfg.max_deep)
		{	b2->b_w = move_computer(deep + 1,
				(who == BLACK ? WHITE : BLACK), b2);
			stats.dstat[deep].n3++;
			if (b2->b_w != WON_W && b2->b_w != LOST_W)
			{
				stats.dstat[deep].ndw++;
				stats.dstat[deep].d_wt +=
					//abs(b2->b_w - b2->c_w);
					b2->b_w - b2->c_w;
			}
			if (b2->b_w == WON_W)
				break;
		}
		if (deep == 0)
		{
			if (b2->b_w > d_wmax)
				d_wmax = b2->b_w;
			fprintf(stdout, "%d moves (%d %%) best %d   \r",
				stats.total, i * 100 / m, d_wmax);
			fflush(stdout);
	}	}

	b_wmax = LOST_W - 1;
	c_wmax = LOST_W - 1;
	for (i = 0; i < m; i++)
	{
		b2 = &(boards[i]);
		if (b2->b_w > b_wmax ||
		    (b2->b_w == b_wmax && b2->c_w > c_wmax))
		{	b_wmax = b2->b_w;
			c_wmax = b2->c_w;
			if (deep == 0)
				memcpy(b, b2, sizeof(t_board));
	}	}
	if (b_wmax == LOST_W - 1) b_wmax = LOST_W;
	return -b_wmax;
}

void computer_play(int who, t_board *b)
{
	int w1, w2;
	memset(&stats, '\0', sizeof(t_stat));
	w1 = -move_computer(0, who, b);
	w2 = evaluate_board(who, b);
	fprintf(stdout, "%s: OK! Analyzed %d moves, evaluation: best %d, "
		"current %d\n",
		strwho(who),
		stats.total, w1, w2);
	if (cfg.verbose)
		display_stats(who);
	display_board(b);
	if (w2 == LOST_W) lost(who);
	if (w2 == WON_W) won(who);
}

void usage(const char *prg)
{
	fprintf(stderr, "pogo Version %s\n", pogo_version);
	fprintf(stderr, "Usage: %s [-h] [-v] [-hb] [-cw] [-1|-2|-3]\n",
		prg);
	fprintf(stderr, "   -h   Display this help and exit.\n");
	fprintf(stderr, "   -v   Be verbose.\n");
	fprintf(stderr, "   -hb  Set BLACK player to be human.\n");
	fprintf(stderr, "   -cw  Set WHITE player to be droid.\n");
	fprintf(stderr, "   -1   Set computer strength to easy (default).\n");
	fprintf(stderr, "   -2   Set computer strength to medium.\n");
	fprintf(stderr, "   -3   Set computer strength to harsh.\n");
	exit(1);
}

void parse_args(int argc, char *argv[])
{
	int i, strength = 0;
	cfg.verbose = 0;
	cfg.human_white = 1;
	cfg.human_black = 0;
	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-h"))
			usage(argv[0]);
		else if (!strcmp(argv[i], "-v"))
			cfg.verbose = 1;
		else if (!strcmp(argv[i], "-hb"))
			cfg.human_black = 1;
		else if (!strcmp(argv[i], "-cw"))
			cfg.human_white = 0;
		else if (!strcmp(argv[i], "-1"))
			strength = 0;
		else if (!strcmp(argv[i], "-2"))
			strength = 1;
		else if (!strcmp(argv[i], "-3"))
			strength = 2;
		else 
		{
			fprintf(stderr, "Bad argument: %s\n", argv[i]);
			usage(argv[0]);
		}
	}
	cfg.max_deep = max_deep[strength];
	cfg.mmax = mmax[strength];
}

int main(int argc, char *argv[])
{
	t_board b;

	fprintf(stdout, "Welcome to POGO v%s\n", pogo_version);
	parse_args(argc, argv);
	init_board(&b);
	display_board(&b);

	while(1)
	{
		if (cfg.human_white)
			human_play(WHITE, &b);
		else	computer_play(WHITE, &b);

		if (cfg.human_black)
			human_play(BLACK, &b);
		else	computer_play(BLACK, &b);
	}

	return 0;
}

