#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <pthread.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#ifndef REPOURL
#define REPOURL      "[Source code: https://github.com/SalvatoreBia/TypiT.git]"
#endif

#define LISTPATH     "words.txt"
#define WORDCHUNK    45
#define TESTDURATION 30
#define TESTLINELEN  15
#define LINEMAXLEN   50
#define UITITLE      "<<< TypiT >>>"
#define UISTOP       "[Ctrl+C to quit]"
#define UITESTRESET  "[Tab to reset]"

// ==================== GLOBAL VARIABLES ===================

char **words_g;
int  words_len_g;

static volatile int running = 1;

// ==================== DATA STRUCTURES ====================

typedef struct
{
	int y, x;
	WINDOW *win;
}ui_t;

typedef struct
{
	int wpm;
	float accuracy;
}stats_t;

typedef struct
{
	int total_correct_words;
	int curr_correct_words;
	int curr_idx;

	char **curr;
	char **next;
} player_t;

// ==================== UTILITIES ==========================

void int_handler(int sig)
{
	endwin();
	exit(42);
}

int fcount_lines(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("***Error trying to opening word list file!");
        exit(EXIT_FAILURE);
    }
    char buf[LINEMAXLEN];
    int count = 0;
    while (fgets(buf, LINEMAXLEN, file))
        count++;
    fclose(file);
    return count;
}

void init_words_g(void)
{
    int lines = fcount_lines(LISTPATH);
    words_len_g = lines;

    FILE *file = fopen(LISTPATH, "r");
    if (!file)
    {
        perror("***ERROR: failed to open file!");
        exit(EXIT_FAILURE);
    }

    words_g = malloc(lines * sizeof(char *));
    if (!words_g)
    {
        perror("***ERROR: failed to allocate words_g!");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    char buf[LINEMAXLEN];
    int count = 0;

    while (fgets(buf, LINEMAXLEN, file) && count < lines)
    {
        buf[strcspn(buf, "\n")] = '\0';
        words_g[count] = malloc(strlen(buf) + 1);
        if (!words_g[count])
        {
            perror("***ERROR: malloc failed while copying a word");
            for (int i = 0; i < count; i++)
                free(words_g[i]);
            free(words_g);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        strcpy(words_g[count], buf);
        count++;
    }
    fclose(file);
}

char **get_chunk(void)
{
    char **chunk = (char **)malloc(WORDCHUNK * sizeof(char *));
    if (!chunk)
    {
        perror("***ERROR: failed to allocate memory for word chunk!");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < WORDCHUNK; i++)
    {
        int random_idx = rand() % words_len_g;
        const char *tmp = words_g[random_idx];
        chunk[i] = (char *)malloc(strlen(tmp) + 1);
        if (!chunk[i])
        {
            perror("***ERROR: malloc failed while populating word chunk!");
            for (int j = 0; j < i; j++)
                free(chunk[j]);
            free(chunk);
            exit(EXIT_FAILURE);
        }
        strcpy(chunk[i], tmp);
    }
    return chunk;
}

void free_chunk(char **list)
{
	if (!list) return;

	for (int i = 0; i < WORDCHUNK; i++)
		if (list[i]) free(list[i]);

	free(list);
}

// ==================== THREADS ============================

// coming soon...

// ==================== TYPING FUNCTIONS ===================

int print_test_line(WINDOW *win, player_t *p, int skip, int y, int x, int needs_coloring, int use_next)
{

	if (needs_coloring)
	{
		// supposse we have a line and the cursor like this
		// word1 word2 wor|d3 word4
		//
		// we have to print the left section colored, the right section normal

		// we need to calculate the total length of the left half,
		// so we first count how many words are in the left half 
		// BEFORE the current word
		size_t len_left = 0;
		size_t len_total = 0;
		int line_start = skip * TESTLINELEN;
		int offset = p->curr_correct_words % TESTLINELEN;
		for (int i = 0; i < TESTLINELEN; i++)
		{
			int tmp_len = strlen(p->curr[line_start + i]);
			len_left += (i < offset) ? tmp_len + 1 : 0;
			len_total += tmp_len + 1;
		}
		
		// here we consider also the slice of the current word (up to cursor)
		len_left += p->curr_idx;

		// subtract 1 for the extra space included by the for
		len_total--;

		char *left_half = (char *) malloc(len_left + 1); // add 1 for string termination
		char *tmp = left_half;

		// we now need to copy all of the strings to the left_half
		// that will be colored
		for (int i = 0; i < offset; i++)
		{
			size_t wlen = strlen(p->curr[line_start + i]);
			memcpy(tmp, p->curr[line_start + i], wlen);
			tmp += wlen;
			*tmp = ' ';
			tmp++;
		}

		// the last thing to do is to copy the characters in the current word
		memcpy(tmp, p->curr[line_start + offset], p->curr_idx);
		tmp += p->curr_idx;
		*tmp = '\0';

		// the next step is to do the same thing for the right half...
		size_t len_right = len_total - len_left;
		char *right_half = (char *) malloc(len_right + 1);
		tmp = right_half;

		// copy the right half of the current word (from cursor onwards)
		size_t len_curr_word = strlen(p->curr[line_start + offset]);
		if (p->curr_idx < len_curr_word) 
		{
			size_t rem = len_curr_word - p->curr_idx;
			memcpy(tmp, p->curr[line_start + offset] + p->curr_idx, rem);
			tmp += rem;
		}

		// add a space if there are more words
		if (offset < TESTLINELEN - 1)
		{
			*tmp = ' ';
			tmp++;
		}

		// copy the remaining words
		for (int i = offset + 1; i < TESTLINELEN; i++)
		{
			size_t wlen = strlen(p->curr[line_start + i]);
			memcpy(tmp, p->curr[line_start + i], wlen);
			tmp += wlen;
			if (i != TESTLINELEN - 1) 
			{
				*tmp = ' ';
				tmp++;
			}
		}

		*tmp = '\0';

		// here we are ready to print the left half 
		// colored and the right half clean
		wattron(win, COLOR_PAIR(1));
		mvwprintw(win, y, x, left_half);
		wattroff(win, COLOR_PAIR(1));

		wprintw(win, right_half);

		free(left_half);
		free(right_half);

		// this branch returns the cursor position,
		// in order to make it easy for the caller
		// to reposition the cursor
		return len_left;
	}
	else
	{
		if (use_next) skip = 0;
		int line_start = skip * TESTLINELEN;
		size_t len_total = 0;
		for (int i = 0; i < TESTLINELEN; i++)
			len_total += (use_next) ? strlen(p->next[line_start + i]) + 1 : strlen(p->curr[line_start + i]) + 1;

		len_total--;

		char *line = (char *) malloc(len_total + 1);
		char *tmp = line;

		for (int i = 0; i < TESTLINELEN; i++)
		{
			size_t wlen = (use_next) ? strlen(p->next[line_start + i]) : strlen(p->curr[line_start + i]);
			memcpy(
				tmp,
				(use_next) ? p->next[line_start + i] : p->curr[line_start + i], 
				wlen
			);
			tmp += wlen;
			if (i != TESTLINELEN - 1)
			{
				*tmp = ' ';
				tmp++;
			}
		}

		*tmp = '\0';

		mvwprintw(win, y, x, line);
		free(line);
	}

	// default value
	return -1;
}

void show_countdown(WINDOW *win, int n)
{
	mvwprintw(win, 3, 2, "TIME LEFT: %ds", n);
}

void clear_countdown(WINDOW *win)
{
	wmove(win, 3, 2);
	clrtoeol();
}

void handle_resize(ui_t *ui, player_t *p)
{	
	static const int LINEY = 8;
	static const int LINEX = 8;

	int row, col;
	getmaxyx(stdscr, row, col);
	ui->y = row - 2;
	ui->x = col - 2;

	wresize(ui->win, ui->y, ui->x);
	mvwin(ui->win, 1, 1);

	werase(ui->win);
	
	// draw frame
	box(ui->win, 0, 0);
	mvwprintw(ui->win, 0, (((ui->x - 2) - strlen(UITITLE)) / 2), UITITLE);

	// print current lines on the screen...
	int start = p->curr_correct_words / TESTLINELEN;
	int cursor_x = print_test_line(ui->win, p, start, LINEY, LINEX, 1, 0);

	// if the first line is the last row of the curr list
	// the second line must be taken from the next list
	int use_next = 0;
	if (start + 1 == (WORDCHUNK / TESTLINELEN)) use_next = 1;
	print_test_line(ui->win, p, start+1, LINEY+1, LINEX, 0, use_next);

	// print link and shortcuts on screen
	int tab_start = (int) (strlen(UISTOP) + strlen(UITESTRESET));
	int ctrlc_start = tab_start - ((int) strlen(UISTOP));
	mvwprintw(ui->win, ui->y-2, ui->x-5 - tab_start, UITESTRESET);
	mvwprintw(ui->win, ui->y-2, ui->x-5 - ctrlc_start, UISTOP);
	mvwprintw(ui->win, ui->y-2, 2, REPOURL);

	// move cursor to the current word index
	wmove(ui->win, LINEY, LINEX + cursor_x);

	refresh();
	wrefresh(ui->win);
}

void reset_ui(ui_t *ui, player_t *p)
{
	curs_set(0);
	
	int row, col;
	getmaxyx(stdscr, row, col);

	ui->y = row - 2;
	ui->x = col - 2;

	if (ui->win) delwin(ui->win);
	ui->win = newwin(ui->y, ui->x, 1, 1);
	if (!ui->win)
	{
		endwin();
		exit(EXIT_FAILURE);
	}

	if (p->curr) free_chunk(p->curr);
	if (p->next) free_chunk(p->next);

	p->curr = get_chunk();
	p->next = get_chunk();
	
	p->total_correct_words = 0;
	p->curr_correct_words  = 0;
	p->curr_idx            = 0;

	handle_resize(ui, p);

	curs_set(1);
}

ui_t init_ui(player_t *p)
{
	curs_set(0);
	ui_t ui;
	reset_ui(&ui, p);
	curs_set(1);
	
	return ui;
}

void init_environment()
{
	initscr();
	cbreak();
	noecho();

	if (has_colors())
	{
		start_color();
		init_pair(1, COLOR_GREEN, COLOR_BLACK);
	}

}

int main()
{
	init_environment();
	
	srand(time(NULL));
	signal(SIGINT, int_handler);
	init_words_g();
	
	player_t p;
	ui_t main_ui = init_ui(&p);

	while (running)
	{
		
	    int ch = wgetch(main_ui.win);
	    switch (ch)
	    {
	    
	        case KEY_RESIZE:
	            handle_resize(&main_ui, &p);
	            break;
	
	        // 9 is for TAB, resets the test
	        case 9:
	            reset_ui(&main_ui, &p);
	            break;
	
	        default: {
	            if (ch == ' ')
	            {
	                size_t wlen = strlen(p.curr[p.curr_correct_words]);
	
	                if (p.curr_idx >= wlen)
	                {
	                    p.curr_idx = 0;
	                    p.curr_correct_words++;
	                    p.total_correct_words++;
	
	                    if (p.curr_correct_words == WORDCHUNK)
	                    {
	                        p.curr_correct_words = 0;
	                        free_chunk(p.curr);
	                        p.curr = p.next;
	                        p.next = get_chunk();
	                        handle_resize(&main_ui, &p);
	                    }
	                    else if (p.curr_correct_words % TESTLINELEN == 0)
	                    {
	                        handle_resize(&main_ui, &p);
	                    }
	                    else
	                    {
	                        int y, x;
	                        getyx(main_ui.win, y, x);
	                        wmove(main_ui.win, y, x + 1);
	                    }
	                }
	            }
	            else
	            {
	                char expected = p.curr[p.curr_correct_words][p.curr_idx];
	
	                if (ch == expected)
	                {
	                    int y, x;
	                    getyx(main_ui.win, y, x);
	
	                    wattron(main_ui.win, COLOR_PAIR(1));
	                    mvwaddch(main_ui.win, y, x, ch);
	                    wattroff(main_ui.win, COLOR_PAIR(1));
	
	                    wmove(main_ui.win, y, x + 1);
	                    p.curr_idx++;
	                }
	            }
	            break;
	        }
	    }
	}
	
	
	endwin();
	return 0;
}
