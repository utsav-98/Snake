#include <getopt.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

#include "snake.h"

void print_usage(void) {
  printf("./snake [color_directive] or [print-usage]\n");
  printf("  Color-Directives:\n");
  printf("\t-c Cyan\n");
  printf("\t-r Red\n");
  printf("\t-y Yellow\n");
  printf("\t-p Magenta\n");
  printf("\t-w White\n");
  printf("\t-m Rainbow\n");
  printf("  Print-Usage:\n");
  printf("\t-h Help\n");

  exit(1);
}

#define COLOR_NUMS 6

short colors[] = {COLOR_BLUE,   COLOR_RED,     COLOR_GREEN,
                  COLOR_YELLOW, COLOR_MAGENTA, COLOR_WHITE};

static inline void init_all_colors(void) {
  for (int i = 0; i < COLOR_NUMS; i++)
    init_pair(i + 1, colors[i], COLOR_BLACK);
}

int main(int argc, char *argv[]) {
  short color = COLOR_BLUE;
  opterr = 0;

  switch (getopt(argc, argv, "+rgcypmwh")) {

  case ('r'):
    color = COLOR_RED;
    break;

  case ('g'):
    color = COLOR_GREEN;
    break;

  case ('c'):
    color = COLOR_CYAN;
    break;

  case ('y'):
    color = COLOR_YELLOW;
    break;

  case ('p'):
    color = COLOR_MAGENTA;
    break;

  case ('w'):
    color = COLOR_WHITE;
    break;

  case ('m'):
    color = 'm';
    break;

  case ('h'):
    print_usage();
    break;

  case ('?'):
    OPT_ERROR(argv[1][1]);
  }

  initscr();
  start_color();

  if (color != 'm') {
    init_pair(1, color, COLOR_BLACK);
    attron(COLOR_PAIR(1));
  } else
    init_all_colors();

  noecho();
  cbreak();

  keypad(stdscr, TRUE);
  curs_set(0);
  nodelay(stdscr, TRUE);

  struct Head *head = init_snake_head();
  if (!head)
    goto end_game;

  head->multi_color = color == 'm' ? 1 : 0;
  mainloop(head);

end_game:
  endwin();

  return 0;
}

static inline void update_segment_direction(struct Head *head) {
  struct Snake *current = head->body;

  current->prev_direction = current->direction;
  current->direction = head->direction;

  while (current->next_segment != NULL) {
    int direction = current->prev_direction;
    current = current->next_segment;
    current->prev_direction = current->direction;
    current->direction = direction;
  }
}

static inline void update_segment_coordinates(struct Snake *segment) {
  switch (segment->direction) {

  case (NORTH):
    segment->y--;
    break;

  case (SOUTH):
    segment->y++;
    break;

  case (EAST):
    segment->x++;
    break;

  case (WEST):
    segment->x--;
    break;
  }
}

void mainloop(struct Head *head) {
  int game_speed = 100;

  while (1) {

    move_snake(head);

    if (draw_snake(head))
      break;

    refresh();

    if (head->ate) {
      head->ate = 0;
      place_snake_food(head);
      game_speed = (head->length - 4) % 5 == 0 && game_speed > 25
                       ? game_speed - 5
                       : game_speed;
    } else
      mvaddch(head->food_y, head->food_x, ACS_PI);

    switch (getch()) {

    case ('k'): {
      if (head->direction != SOUTH)
        head->direction = NORTH;
      break;
    }

    case ('j'): {
      if (head->direction != NORTH)
        head->direction = SOUTH;
      break;
    }

    case ('h'): {
      if (head->direction != EAST)
        head->direction = WEST;
      break;
    }

    case ('l'): {
      if (head->direction != WEST)
        head->direction = EAST;
      break;
    }

    case (KEY_UP): {
      if (head->direction != SOUTH)
        head->direction = NORTH;
      break;
    }

    case (KEY_DOWN): {
      if (head->direction != NORTH)
        head->direction = SOUTH;
      break;
    }

    case (KEY_LEFT): {
      if (head->direction != EAST)
        head->direction = WEST;
      break;
    }

    case (KEY_RIGHT): {
      if (head->direction != WEST)
        head->direction = EAST;
      break;
    }

    case (KEY_P): {
      pause();
      break;
    }

    case (KEY_Q): {
      free_snake(head);
      goto end_loop;
    }

    default:
      break;
    }

    // flash/beep
    // delay_output(game_speed);
    napms(game_speed);
    update_segment_direction(head);

    getmaxyx(stdscr, head->max_y, head->max_x);

    clear();
  }

  int score = head->length - 5;
  int print_x = (head->max_x - 8) / 2;
  int print_y = head->max_y / 2;

  print_score(score, print_x, print_y, head);

end_loop:
  return;
}

int draw_snake(struct Head *head) {
  struct Snake *current = head->body;

  int max_x = head->max_x;
  int max_y = head->max_y;

  int multi_color = head->multi_color;
  int current_color = 0;

  while (current != NULL) {

    if (current->x >= max_x || current->y >= max_y || current->x < 0 ||
        current->y < 0)
      return 1;

    if (check_segment_intersections(head, current))
      return 1;

    if (multi_color) {
      attron(COLOR_PAIR(current_color % COLOR_NUMS + 1));
      current_color++;
    }

    mvaddch(current->y, current->x, ACS_DIAMOND);
    current = current->next_segment;
  }

  return 0;
}

void add_segment(struct Head *head) {
  struct Snake *segment = head->body;
  head->length++;

  while (segment->next_segment != NULL)
    segment = segment->next_segment;

  int x = segment->x;
  int y = segment->y;

  switch (segment->direction) {

  case (NORTH): {
    y++;
    break;
  }

  case (SOUTH): {
    y--;
    break;
  }

  case (EAST): {
    x--;
    break;
  }

  case (WEST): {
    x++;
    break;
  }
  }

  segment->next_segment = create_segment(segment->prev_direction, x, y);

  if (!segment->next_segment) {
    free_snake(head);
    endwin();
    exit(1);
  }
}

int check_segment_intersections(struct Head *head, struct Snake *segment) {
  struct Snake *current = head->body;

  int segment_x = segment->x;
  int segment_y = segment->y;

  while (current != NULL) {

    if (!(current == segment) && segment_x == current->x &&
        segment_y == current->y)
      return 1;

    current = current->next_segment;
  }

  return 0;
}

void move_snake(struct Head *head) {
  struct Snake *current = head->body;

  int food_x = head->food_x;
  int food_y = head->food_y;
  head->dist++;

  while (current != NULL) {
    update_segment_coordinates(current);

    if (current->x == food_x && current->y == food_y)
      head->ate = 1;

    current = current->next_segment;
  }

  if (head->ate) {
    head->total_dist += head->dist;
    head->min_dist += head->food_dist;
    if (head->dist == head->food_dist)
      head->bonus++;
    add_segment(head);
  }
}

void place_snake_food(struct Head *head) {
  struct Snake *segment = head->body;
  int food_x, food_y;

  while (1) {
    food_x = rand() % head->max_x;
    food_y = rand() % head->max_y;

    if (is_valid_position(segment, food_x, food_y) == 0)
      break;
  }

  mvaddch(food_y, food_x, ACS_PI);
  head->food_x = food_x;
  head->food_y = food_y;
  head->food_dist = abs(head->body->x - food_x) + abs(head->body->y - food_y);
  head->dist = 0;
}

int is_valid_position(struct Snake *segment, int x, int y) {
  for (; segment != NULL; segment = segment->next_segment)
    if (segment->x == x && segment->y == y)
      return 1;
  return 0;
}

struct Head *init_snake_head(void) {
  struct Head *new_head;
  if (!(new_head = malloc(sizeof *new_head)))
    return NULL;

  getmaxyx(stdscr, new_head->max_y, new_head->max_x);
  new_head->direction = WEST;
  new_head->length = SNAKE_INIT_LEN;
  new_head->ate = 1;
  new_head->body = init_snake_body(new_head->max_x, new_head->max_y);

  if (!new_head->body) {
    free(new_head);
    return NULL;
  }

  return new_head;
}

struct Snake *init_snake_body(int max_x, int max_y) {
  int start_x, start_y;

  start_x = max_x / 2;
  start_y = max_y / 2;

  struct Snake *segment = create_segment(WEST, start_x, start_y);
  if (!segment)
    return NULL;

  struct Snake *current = segment;

  for (int i = 0; i < SNAKE_INIT_LEN - 1; i++) {
    struct Snake *next =
        create_segment(current->prev_direction, current->x + 1, current->y);
    if (!next) {
      free_snake_body(segment);
      return NULL;
    }

    current->next_segment = next;
    current = current->next_segment;
  }

  return segment;
}

struct Snake *create_segment(int direction, int x, int y) {
  struct Snake *segment;

  if (!(segment = malloc(sizeof *segment)))
    return NULL;

  segment->next_segment = NULL;
  segment->prev_direction = direction;
  segment->direction = direction;
  segment->x = x;
  segment->y = y;

  return segment;
}

void free_snake(struct Head *snake_head) {
  free_snake_body(snake_head->body);
  free(snake_head);
}

void free_snake_body(struct Snake *body) {
  while (body != NULL) {
    struct Snake *prev_segment = body;
    body = body->next_segment;
    free(prev_segment);
  }
}

void pause(void) {
  while (1) {

    int key = getch();

    switch (key) {

    case (KEY_P):
      return;

    case (KEY_Q): {
      ungetch(key);
      return;
    }
    }
  }
}

void print_score(int score, int x, int y, struct Head *head) {
  clear();
  mvprintw(y, x, "SCORE: %d", score);
  mvprintw(y - 2, x - 2, "DISTANCE: %d", head->total_dist);
  mvprintw(y - 3, x - 4, "MIN-DISTANCE: %d", head->min_dist);
  mvprintw(y - 4, x - 1, "BONUS: %d", head->bonus);
  refresh();
  play_again(x, y + 2, head);
}

void play_again(int x, int y, struct Head *head) {
  mvprintw(y, x - 5, "PLAY AGAIN? (Y/N)");

  while (1) {
    switch (getch()) {

    case (KEY_Y): {
      clear();
      int multi = head->multi_color;
      free_snake(head);
      head = init_snake_head();
      if (!head)
        goto end_game;
      head->multi_color = multi;
      mainloop(head);
      break;
    }

    case (KEY_N): {
      mvprintw(y - 1, x - 1, "GOOD GAME!");
      refresh();
      napms(1000);
      free_snake(head);
      goto end_game;
    }

    default:
      break;
    }
  }

end_game:
  return;
}
