#include <ncurses.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

/*
	Pentru aceasta structura am folosit un vector de structuri : *round
	Pe poziția 0 din vetor salvez variabilele de tip int
	Pe pozițiile 1, ... , argc - 1 în variabila q_file_name salvez numele fisierelor de intrebari
	Pe pozițiile 1,...k - 1 salvez intrebarea, variantele, si raspunsul corect
*/
typedef struct questions_struct
{
	char *q, *A, *B, *C, *D, *correct, *q_file_name;
	int a_rand_up, b_rand_up, c_rand_up, d_rand_up, nr_correct, nr_wrong;
	int skip_just_ended, q_file_count, cap, cap_from_main;
}questions_struct;



/*
nr_q = pozitia din vector la care se afla intrebarea curenta
Functia se apeleaza dupa ce jucatorul a dat raspunsul
flag_answer = 1 => raspuns corect
*/
void end_of_round(questions_struct *round, int flag_answer, int nr_q)
{
	curs_set(0);
	clear();

	if(flag_answer)
	{
		attron(A_BOLD | COLOR_PAIR(2));
		mvprintw(LINES / 4, (COLS - 14) / 2,"Raspuns CORECT :)");
		attroff(A_BOLD | COLOR_PAIR(2));
		
		refresh();
		sleep(1);
		
		/*folosesc flushinp() pentru a bloca input-ul venit de la jucator
		pentru a nu raspunde din greseala la urmatoarea intrebare*/
		flushinp();
	}
	else
	{
		if(!round[0].skip_just_ended)
		{
			attron(A_BOLD | COLOR_PAIR(3));
			mvprintw(LINES / 4, (COLS - 14) / 2,"Raspuns GRESIT :(");
			attroff(A_BOLD | COLOR_PAIR(3));
		}

		attron(A_BOLD | COLOR_PAIR(2));
		
		if(round[nr_q].correct[0] == 'a')
			mvprintw(LINES / 2, (COLS - 18 - strlen(round[nr_q].A)) / 2,"Varianta corecta: %s", round[nr_q].A);

		if(round[nr_q].correct[0] == 'b')
			mvprintw(LINES / 2, (COLS - 18 - strlen(round[nr_q].B)) / 2,"Varianta corecta: %s", round[nr_q].B);

		if(round[nr_q].correct[0] == 'c')
			mvprintw(LINES / 2, (COLS - 18 - strlen(round[nr_q].C)) / 2,"Varianta corecta: %s", round[nr_q].C);

		if(round[nr_q].correct[0] == 'd')
			mvprintw(LINES / 2, (COLS - 18 - strlen(round[nr_q].D)) / 2,"Varianta corecta: %s", round[nr_q].D);
		
		attroff(A_BOLD | COLOR_PAIR(2));	
		
		refresh();
		sleep(3);
		flushinp();

	}
		
	curs_set(1);
}

/*
Se afișează clasamentul
Format fișier: "Nume scor"

Folosesc verificare cu FSEEK deoarece am implementat functionalitatea de scoreboard
astfel incat fisierul poate sa fie gol si la fiecare joc terminat, jucatorul sa fie adaugat
in clasament daca in clasament nu sunt deja 10 inregistrari. Iar dacă folosesc fscanf() intr-un fisier
gol se petrec lucruri necreștine cu programul.
*/
void Wall_of_fame()
{
	WINDOW *score_board_margins;
	FILE *score_board;
	char ch, buffer[100];
	unsigned long len;	
	int score, y = 1, flag_first_place = 1;
	clear();
	
	attron(A_BOLD);
	mvprintw(1, (COLS - 10) / 2, "SCOREBOARD");
	attroff(A_BOLD);
	
	score_board_margins = newwin(21, 30, 2, (COLS - 30) / 2);
	refresh();
	box(score_board_margins, 0, 0);
	wrefresh(score_board_margins);
	
	score_board = fopen("Wall_of_fame","r");
	
	fseek(score_board, 0, SEEK_END);
	len = ftell(score_board);
	if(len > 0)
	{
		rewind(score_board);
		while(! feof(score_board))
		{
			fscanf(score_board, "%s %d", buffer, &score);
			if(buffer == NULL)
				break;

			if(flag_first_place == 1)
			{
				wattron(score_board_margins, A_BOLD | COLOR_PAIR(3));
				mvwprintw(score_board_margins, y, 2, "%d. %s : %d",flag_first_place, buffer, score);
				wattroff(score_board_margins, A_BOLD | COLOR_PAIR(3));	
				flag_first_place++;
			}
			else
			{
				mvwprintw(score_board_margins, y, 2, "%d. %s : %d",flag_first_place, buffer, score);
				flag_first_place++;
			}
			y = y + 2;
			wrefresh(score_board_margins);
		}
	}

	attron(A_BLINK);
	mvprintw(1, 2,"Press q to return");
	attroff(A_BLINK);
	refresh();

	while(1)
	{	
		ch = tolower(getch());
		if(ch == 'q')		
			break;
	}
	delwin(score_board_margins);
	fclose(score_board);
}

/*
Funcția start_game este comună opțiunilor New Game și Resume

*/
int start_game(questions_struct *round, int k, int *current_question, int *score, char *name, int *skip_up, int *fifty_up, int *fifty_running)
{
	int i, j, y , x, l, count_clear = 0, nr_random, deny_input_count = 0, question_box_length, len;
	int flag_answer, player_score[10], has_been_increased;
	char ch, ch_random, deny_input[2], players[10][19];
	WINDOW *game_win, *question, *a, *b, *c, *d, *answer;
	FILE *write;
	struct tm *local;
	time_t t;

	clear();
	curs_set(1);
	
	game_win = newwin(LINES, COLS, 0, 0);
	
	question = newwin(6, COLS * 9 / 10 + 2, LINES / 8 - 1, COLS / 20 - 1);
	question_box_length = COLS * 9 / 10 + 2;
	
	a = newwin(3, COLS * 8 / 20, LINES / 3, COLS / 20);
	b = newwin(3, COLS * 8 / 20, LINES / 3, COLS * 11 / 20); 
	c = newwin(3, COLS * 8 / 20, LINES / 2, COLS / 20);
	d = newwin(3, COLS * 8 / 20, LINES / 2, COLS * 11 / 20);
	answer = newwin(3, 3, LINES * 5 / 12, COLS / 2 - 1);
	
	refresh();

	for(i = *current_question; i < k; i++)
	{

		wclear(game_win);
		box(game_win, 0, 0);
		
		mvwprintw(game_win, 1, 1,"Question: %d", i + 1);
		wattron(game_win, A_BOLD);
		mvwprintw(game_win, 1, (COLS - strlen(name) - 10) / 2, "Good luck %s", name);
		wattroff(game_win, A_BOLD);
		
		//Afișare cu verde sau roșu
		if(*skip_up)
		{
			wattron(game_win, A_BOLD | COLOR_PAIR(2));
			mvwprintw(game_win, LINES - 4, 2, "Skip the question (Press 1)");
			wattroff(game_win, A_BOLD | COLOR_PAIR(2));
		}
		else
		{
			wattron(game_win, A_BOLD | COLOR_PAIR(3));
			mvwprintw(game_win, LINES - 4, 2, "Skip the question (Press 1)");
			wattroff(game_win, A_BOLD | COLOR_PAIR(3));
		}
		
		//Afișare cu verde sau roșu
		if(*fifty_up)
		{
			wattron(game_win, A_BOLD | COLOR_PAIR(2));
			mvwprintw(game_win, LINES - 2, 2, "50/50 (Press 2)");
			wattroff(game_win, A_BOLD | COLOR_PAIR(2));
		}
		else
		{
			wattron(game_win, A_BOLD | COLOR_PAIR(3));
			mvwprintw(game_win, LINES - 2, 2, "50/50 (Press 2)");
			wattroff(game_win, A_BOLD | COLOR_PAIR(3));
		}

		//Afișare cu verde sau roșu
		if(*score >= 0)
		{
			wattron(game_win, A_BOLD | COLOR_PAIR(2));
			mvwprintw(game_win, 1, COLS - 11, "Score: %d", *score);
			wattroff(game_win, A_BOLD | COLOR_PAIR(2));
		}
		else
		{
			wattron(game_win, A_BOLD | COLOR_PAIR(3));
			mvwprintw(game_win, 1, COLS - 11, "Score: %d", *score);
			wattroff(game_win, A_BOLD | COLOR_PAIR(3));
		}

		wrefresh(game_win);

		box(question, 0, 0);
		wrefresh(question);

		/*
		Dacă aș fi folosit mvwprintw nu aș fi putut afișa întrebările pe mai multe linii

		Folosind trick-ul ăsta reușesc să afișez caracter cu caracter pe mai multe linii 
		*/
		x = 1; y = 1;
		for(j = 0; j < strlen(round[i].q); j++)
		{
			mvwaddch(question, y, x++, round[i].q[j]);
			if(x == question_box_length - 1)
			{
				y++;
				x = 1;
			}
		}

		wrefresh(question);

			
		
		box(answer, 0, 0);
		wrefresh(answer);
		

		/*
		Aici se găsește inima opțiunii de 50/50
		Variabilele round[0].{a,b,c,d}_rand_up sunt 1 inițial deci nu au fost alese random pentru a fi sterse
		*fifty_running este 1 doar când jucătorul se află la intrebarea cu variantele înjumătățite sau revine 
		în starea asta din Resume.
		If-ul următor se execută o singură dată și 2 dintre variabilele ...rand_up vor deveni 0
		*/
		if(*fifty_running && round[0].a_rand_up && round[0].b_rand_up && round[0].c_rand_up && round[0].d_rand_up)
		{
			srand(time(NULL));
			
			while(count_clear < 2)
			{
				ch_random = rand() % 4 + 97;	
				if(ch_random != round[i].correct[0])
				{
					nr_random = ch_random - 97;
					
					if(nr_random  == 0 && round[0].a_rand_up)
					{
						wclear(a);
						wrefresh(a);	
						round[0].a_rand_up = 0;
						count_clear++;
					}

					if(nr_random  == 1 && round[0].b_rand_up)
					{
						wclear(b);
						wrefresh(b);
						round[0].b_rand_up = 0;	
						count_clear++;
					}

					if(nr_random  == 2 && round[0].c_rand_up)
					{
						wclear(c);
						wrefresh(c);
						round[0].c_rand_up = 0;	
						count_clear++;
					}

					if(nr_random  == 3 && round[0].d_rand_up)
					{
						wclear(d);
						wrefresh(d);	
						round[0].d_rand_up = 0;
						count_clear++;
					}
				}	
							
			}
			
		}

		/*
		If-urile de mai jos și if-ul de sus sunt construite în așa fel încât să mențină starea
		ecranului când se reia jocul din Resume.
		
		Următoarele 4 if-uri afișează căsuțele de întrebări dacă jucătorul nu se află în situația
		cu întrebările înjumătățite. În caz contrar (cand nu afiseaza casutele) salveaza litera 
		corespunzatoare în string-ul deny_input.
		
		deny_input va avea după ce varianta 50/50 e folosita stocate pe pozitiile 0 si 1 caracterele
		corespunzatoare casutelor sterse, adica raspunsurile gresite, pentru a nu-i permite jucatorului
		sa aleaga variantele gresite.
		*/

		if( !(*fifty_running && round[0].a_rand_up == 0) )
		{
			box(a, 0, 0);
			wrefresh(a);	
			mvwprintw(a, 1, 1, "%s", round[i].A);
			wrefresh(a);
		}
		else
		{
			deny_input[deny_input_count] = 'a';
			deny_input_count++;
			
		}
		
			
		if( !(*fifty_running && round[0].b_rand_up == 0) )
		{
			box(b, 0, 0);
			wrefresh(b);
			mvwprintw(b, 1, 1, "%s", round[i].B);
			wrefresh(b);
		}
		else
		{
			deny_input[deny_input_count] = 'b';
			deny_input_count++;
			
		}
		

		if( !(*fifty_running && round[0].c_rand_up == 0) )
		{
			box(c, 0, 0);
			wrefresh(c);
			mvwprintw(c, 1, 1, "%s", round[i].C);
			wrefresh(c);
		}
		else
		{
			deny_input[deny_input_count] = 'c';
			deny_input_count++;
			
		}

		if( !(*fifty_running && round[0].d_rand_up == 0) )
		{
			box(d, 0, 0);
			wrefresh(d);	
			mvwprintw(d, 1, 1, "%s", round[i].D);
			wrefresh(d);
		}
		else
		{
			deny_input[deny_input_count] = 'd';
			deny_input_count++;
			
		}
		
		keypad(answer, TRUE);
		
		while(1)
		{
			t=time(NULL);
			local=localtime(&t);
		 	
		 	mvwprintw(game_win, LINES - 2, COLS - 14, "%d : %d : %d", local->tm_hour, local->tm_min, local->tm_sec);
		 	wrefresh(game_win);

		 	wmove(answer, 1, 1);

			if(*fifty_running && deny_input != NULL )//Întrebarea cu variante înjumătățite
				do
				{
					ch = tolower(wgetch(answer));
				
				}while(ch == deny_input[0] || ch == deny_input[1]);
			else
				ch = tolower(wgetch(answer));

			if(ch == 'q')
			{
				*current_question = i;
				delwin(a);
				delwin(b);
				delwin(c);
				delwin(d);
				delwin(answer);
				delwin(question);
				delwin(game_win);
				return 1;
			}
			
			/*
			*fifty_running = 0;	-> Daca ecranul era în starea "înjumătățită", trecându-se la întrebarea
			următoare se termină starea "înjumatățită"

			skip_just_ended = 1; -> Se folosește pentru a evita afișarea particularizată a ecranului de final de rundă
			*/
			if(ch == '1' && *skip_up)
			{
				*fifty_running = 0;
				*skip_up = 0;
				round[0].skip_just_ended = 1;
				flag_answer = 0;
				break;
			}
			/*
			folosesc "--i;" deoarece ne aflam intr-un loop și pentru ca vreau sa sterg jumatate din variante
			vreau ca urmatoarea iteratie sa fie aceeași întrebare
			*/
			if(ch == '2' && *fifty_up)
			{
				*fifty_up = 0;
				*fifty_running = 1;
				--i;
				break;
			}
			
			if(ch == 'a' || ch == 'b' || ch == 'c' || ch == 'd')
			{
				if(*fifty_running)
					*fifty_running = 0;

				if(ch == round[i].correct[0])
				{	
					round[0].nr_correct++;
					*score += 10;
					flag_answer = 1;
					break;
				}
				else
				{
					round[0].nr_wrong++;
					*score += -5;
					flag_answer = 0;
					break;
				}
			}
		}

		wclear(question);
		wclear(a);
		wclear(b);
		wclear(c);
		wclear(d);

		if(!*fifty_running)
			end_of_round(round, flag_answer, i);
		
		if(round[0].skip_just_ended)
			round[0].skip_just_ended = 0;
	}	

	clear();
	mvprintw(LINES / 4, (COLS - 16 - strlen(name)) / 2, "Congratulations %s", name);
	mvprintw(LINES /2 , (COLS - 15) / 2, "FINAL SCORE: %d", *score);
	
	attron(A_BOLD | COLOR_PAIR(2));
	mvprintw(LINES / 2, 2, "Correct answers : %d", round[0].nr_correct);
	attroff(A_BOLD | COLOR_PAIR(2));
	
	attron(A_BOLD | COLOR_PAIR(3));
	mvprintw(LINES / 2, COLS - 19, "Wrong answers : %d", round[0].nr_wrong);
	attroff(A_BOLD | COLOR_PAIR(3));

	attron(A_BLINK | COLOR_PAIR(4));
	mvprintw(LINES * 7 / 8, (COLS - 26) / 2, "Press any key to continue!");
	attroff(A_BLINK | COLOR_PAIR(4));	

	curs_set(0);

	write = fopen("Wall_of_fame","r");//se citeste doar daca e gol
	
	i = 0;

	fseek(write, 0, SEEK_END);
	len = ftell(write);
	if(len > 0)
	{
		rewind(write);
		while( ! feof(write))
		{
			fscanf(write, "%s %d", players[i], &player_score[i]);
			players[i][strlen(players[i])] = 0;
			i++;
		}
	}
	fclose(write);

	/*
	Am construit clasamentul să se completeze pe parcurs până se ating 10 inregistrari.
	*/
	write = fopen("Wall_of_fame","w+");

	if(i < 10)
	{
		i++;
		has_been_increased = 1;
	}
	

	for(j = 0; j < i; j++)
	{
		if(*score >= player_score[j])//stabileste unde trebuie plasat jucatorul
		{
			for(l = i - 1; l > j; l--)//fiecare jucator de sub el este retrogradat cu o pozitie
			{
				strcpy(players[l], players[l - 1]);
				player_score[l] = player_score[l - 1];
			}
			strcpy(players[j], name);
			player_score[j] = *score;
			break;
		}

		/*
		If-ul asta se executa atunci cand jucatorul are scorul mai mic decat oricine din clasament
		însă în clasament nu sunt 10 îregistrări
		*/
		if(j == i - 1 && has_been_increased)
		{
			strcpy(players[j], name);
			player_score[j] = *score;
		}	
	}

	for(j = 0; j < i; j++)
	{
		if(j == i - 1)
			fprintf(write, "%s %d", players[j], player_score[j]);
		else
			fprintf(write, "%s %d\n", players[j], player_score[j]);
	}
	fclose(write);
	
	*current_question = 0;
	*score = 0;

	getch();

	delwin(a);
	delwin(b);
	delwin(c);
	delwin(d);
	delwin(answer);
	delwin(question);
	delwin(game_win);
	
	return 0;
}

/*
Afișează marginile(prelungite) ale meniului
Marginile create de funcția respectivă nu coincid cu marginile ferestrei meniului
*/
void print_menu_border(int init_y, int init_x, int menu_height, int menu_width)
{
	int i;
	for(i=0; i < COLS; i++)
	{
		mvaddch(init_y, i, '-');
		mvaddch(init_y + menu_height, i, '-');	
	}
	for(i=0; i < LINES; i++)
	{
		mvaddch(i, init_x, '|');
		mvaddch(i, init_x + menu_width, '|');	
	}

	mvaddch(init_y, init_x, '+');//stanga-sus
	mvaddch(init_y, init_x + menu_width, '+');//dreapta sus
	mvaddch(init_y + menu_height, init_x, '+');//stanga jos
	mvaddch(init_y + menu_height, init_x + menu_width, '+');//dreapta jos
}

/*
Afișează opțiunile meniului cu verde și roșu(doar Resume)
resume_up = 0 => Resume indisponibil
*/
void print_menu_items(WINDOW *menu_win, int highlight, int resume_up)
{
	char *items[]={"New Game", "Resume Game", "Wall of fame", "Quit"};
	int x, y = 1, i;
	
	for(i = 0; i < 4; i++)
	{
		x = (15 - strlen(items[i]))/2;
		if(highlight == i + 1)
		{
			if(i == 1 && resume_up == 0)
			{
				wattron(menu_win, A_BOLD | COLOR_PAIR(3));
				mvwprintw(menu_win, y, x, "%s", items[i]);
				wattroff(menu_win, A_BOLD | COLOR_PAIR(3));								
			}
			else
			{
				wattron(menu_win, A_BOLD | COLOR_PAIR(2));
				mvwprintw(menu_win, y, x, "%s", items[i]);
				wattroff(menu_win, A_BOLD | COLOR_PAIR(2));				
			}

		}
		else
			mvwprintw(menu_win, y, x, "%s", items[i]);
		y++;
		
	}
	wrefresh(menu_win);
}


void start_menu(int init_y, int init_x, int menu_height, int menu_width, questions_struct *round)
{
	WINDOW *menu_win,*input_name;
	int i, ch, highlight = 1,choice = 0, current_question, score, resume_up = 0;
	int nr_rand, skip_up, fifty_up, fifty_running, k, sessions_count = 0;
	char name[19], string_buff[300];
	FILE *questions;

	print_menu_border(init_y, init_x, menu_height, menu_width);
	refresh();   
	
	menu_win = newwin(menu_height - 1, menu_width - 1, init_y + 1, init_x + 1);
	print_menu_items(menu_win, highlight, resume_up);
	wrefresh(menu_win);
	

	round[0].cap_from_main = round[0].cap;

	keypad(menu_win, TRUE);
	
	while(1)
	{
		curs_set(0);
		ch = wgetch(menu_win);
		switch(ch)
		{
			case KEY_UP:
				if(highlight == 1)
					highlight = 4;
				else
					highlight--;
				break;

			case KEY_DOWN:
				if(highlight == 4)
					highlight = 1;
				else
					highlight++;
				break;
			
			case 10:
				choice = highlight;
				break;
		}
		print_menu_items(menu_win, highlight, resume_up);
		
		/*
			Înainte de a se intra efectiv într-o runda nouă de joc se verifică dacă s-a mai jucat până atunci
		și în caz afirmativ de eliberează intrebarile anterioare din vectorul de structuri. Apoi se realoca
		memoria vectorului cu round[0].cap_from_main (in funcție de numărul de argumente date la execuție).
			Fac chestia asta pentru ca în diferite fișiere pot fi numere variate de "expresii de intrebari"
		(o expresie contine 6 linii: intrebare, 4 variante si raspunsul corect) și vrând ca la fiecare începere a unui
		joc nou să am întrebări noi, în felul ăsta nu consum mai multa memorie decât se cuvine.

			Se inițializează variabilele necesare începerii jocului și apoi se începe jocul
		*/
		refresh();
		if(choice == 1)
		{
			if(sessions_count)
				for(i = 0; i < k; i++)
				{
					free(round[i].q);
					free(round[i].A);
					free(round[i].B);
					free(round[i].C);
					free(round[i].D);
					free(round[i].correct);	
				}
			sessions_count++;
			
			srand(time(NULL));
			nr_rand = rand() % (round[0].q_file_count - 1) + 1;


			k = 0;
			questions = fopen(round[nr_rand].q_file_name, "r");

			round[0].cap = round[0].cap_from_main;

			while(fgets(string_buff, 300, questions) != NULL)
			{
				if(k + 1 == round[0].cap)
				{
					round[0].cap = round[0].cap * 2;
					round = realloc(round, round[0].cap * sizeof(questions_struct));
				}
				string_buff[strlen(string_buff) - 1] = 0;
				
				round[k].q = strdup(string_buff);
				
				fgets(string_buff, 30, questions);
				string_buff[strlen(string_buff) - 1] = 0;
				round[k].A = strdup(string_buff);
				
				fgets(string_buff, 30, questions);
				string_buff[strlen(string_buff) - 1] = 0;
				round[k].B = strdup(string_buff);
				
				fgets(string_buff, 30, questions);
				string_buff[strlen(string_buff) - 1] = 0;
				round[k].C = strdup(string_buff);
				
				fgets(string_buff, 30, questions);
				string_buff[strlen(string_buff) - 1] = 0;
				round[k].D = strdup(string_buff);
				
				fgets(string_buff, 3, questions);
				round[k].correct = strdup(string_buff);
				k++;

			}
			fclose(questions);

			current_question = 0;
			score = 0;

			clear();
	
			echo();
			curs_set(1);
			input_name = newwin(3, COLS / 4, LINES / 2, COLS * 3 / 8);
			mvprintw(LINES / 2 - 1, (COLS - 16) / 2, "What's your name?");			
			wmove(input_name, 1, 1);
	
			refresh();
	
			box(input_name, 0, 0);
			wrefresh(input_name);
	
			wscanw(input_name, "%18s",name);
			delwin(input_name);
			noecho();
			
			skip_up = 1;
			fifty_up = 1;
			fifty_running = 0;
			round[0].a_rand_up = 1;
			round[0].b_rand_up = 1;
			round[0].c_rand_up = 1;
			round[0].d_rand_up = 1;
			round[0].nr_correct = 0;
			round[0].nr_wrong = 0;
			round[0].skip_just_ended = 0;

			resume_up = start_game(round, k, &current_question, &score, name, &skip_up, &fifty_up, &fifty_running);
			
			choice = 0;
			clear();

			print_menu_border(init_y, init_x, menu_height, menu_width);
			refresh();
			print_menu_items(menu_win, highlight, resume_up);
			refresh();

		}

		/*
			Dacă resume_up = 1 (variabila devine 1 când se iese din start_game la apasarea tastei q/Q)
			se poate apela resume_up pentru a continua jocul.
		*/
		if(choice == 2 && resume_up)
		{
			clear();
			resume_up = start_game(round, k, &current_question, &score, name, &skip_up, &fifty_up, &fifty_running);

			choice = 0;
			clear();

			print_menu_border(init_y, init_x, menu_height, menu_width);
			refresh();
			print_menu_items(menu_win, highlight, resume_up);
			refresh();
		}

		if(choice == 3)
		{
			Wall_of_fame();

			choice = 0;
			clear();

			print_menu_border(init_y, init_x, menu_height, menu_width);
			refresh();
			print_menu_items(menu_win, highlight, resume_up);
			refresh();
		}
		if(choice == 4)
		{
			if(sessions_count)
			{	
				for(i = 0; i < k; i++)
				{
					free(round[i].q);
					free(round[i].A);
					free(round[i].B);
					free(round[i].C);
					free(round[i].D);
					free(round[i].correct);
				}
			}
			
			for(i = 1; i < round[0].q_file_count; i++)
					free(round[i].q_file_name);
			
			free(round);

			break;
		}	
	}
	delwin(menu_win);
}
/*
vectorul de structuri *round este inițializat cu o capacitate de 5 structuri initial și
valoarea asta e pastrată și transmisă pentru a reinițializa dimensiunea
*/
int main(int argc, char *argv[])
{
	questions_struct *round;
	FILE *test;
	int i, menu_width = 15, menu_height = 7, init_y, init_x;

	if(argc == 1)
	{
		printf("[Eroare]: Nu s-au dat argumente in linia de comanda.");
		return 1;		
	}		
	for(i = 1; i < argc; i++)
	{
		test = fopen(argv[i], "r");
		if ( ! test)
		{
			fprintf(stderr, "[Eroare]: Fisierul %s nu poate fi deschis.", argv[i]);
			return 1;
		}
		fclose(test);
	}	

	round = malloc(5 * sizeof(questions_struct));
	round[0].cap = 5;
	round[0].q_file_count = argc;

	for(i = 1; i < argc; i++)
	{	
		if(i + 1 == round[0].cap)
		{
			round[0].cap = round[0].cap * 2;
			round = realloc(round, round[0].cap * sizeof(questions_struct));
		}
		round[i].q_file_name = strdup(argv[i]);
	}
	

	initscr();
	noecho();
	cbreak();
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_RED, COLOR_BLACK);
	init_pair(4, COLOR_YELLOW, COLOR_BLACK);
	bkgd(COLOR_PAIR(1));
	
	init_y = (LINES - menu_height)/4; 
	init_x = (COLS - menu_width)/2;

	start_menu(init_y, init_x, menu_height, menu_width, round);
	

	endwin();
	return 0;
}