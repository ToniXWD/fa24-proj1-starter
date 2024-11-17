#include "state.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snake_utils.h"

/* Helper function definitions */
static void set_board_at(game_state_t *state, unsigned int row,
                         unsigned int col, char ch);
static bool is_tail(char c);
static bool is_head(char c);
static bool is_snake(char c);
static char body_to_tail(char c);
static char head_to_body(char c);
static unsigned int get_next_row(unsigned int cur_row, char c);
static unsigned int get_next_col(unsigned int cur_col, char c);
static void find_head(game_state_t *state, unsigned int snum);
static char next_square(game_state_t *state, unsigned int snum);
static void update_tail(game_state_t *state, unsigned int snum);
static void update_head(game_state_t *state, unsigned int snum);

/* Task 1 */
game_state_t *create_default_state() {
  char default_board[] = "####################\n"
                         "#                  #\n"
                         "# d>D    *         #\n"
                         "#                  #\n"
                         "#                  #\n"
                         "#                  #\n"
                         "#                  #\n"
                         "#                  #\n"
                         "#                  #\n"
                         "#                  #\n"
                         "#                  #\n"
                         "#                  #\n"
                         "#                  #\n"
                         "#                  #\n"
                         "#                  #\n"
                         "#                  #\n"
                         "#                  #\n"
                         "####################\n";
  game_state_t *state = malloc(sizeof(game_state_t));
  if (!state) {
    perror("Failed to allocate memory for game state");
    return NULL;
  }

  state->board = malloc(sizeof(char *) * 18);
  if (!state->board) {
    perror("Failed to allocate memory for board");
    free(state);
    return NULL;
  }

  for (int i = 0; i < 18; i++) {
    state->board[i] =
        malloc(sizeof(char) * 22); // 20 个字符 + 1 个换行符 + 1 个终结符
    if (!state->board[i]) {
      perror("Failed to allocate memory for board row");
      // Free previously allocated rows and state
      for (int j = 0; j < i; j++) {
        free(state->board[j]);
      }
      free(state->board);
      free(state);
      return NULL;
    }
    strncpy(state->board[i], default_board + i * 21, 21);
    state->board[i][21] = '\0'; // 确保字符串以 '\0' 结尾
  }

  state->num_rows = 18;
  state->num_snakes = 1;
  snake_t *snakes = malloc(sizeof(snake_t));
  if (!snakes) {
    perror("Failed to allocate memory for snakes");
    // Free allocated board
    for (int i = 0; i < 18; i++) {
      free(state->board[i]);
    }
    free(state->board);
    free(state);
    return NULL;
  }

  snakes->tail_row = 2;
  snakes->tail_col = 2;
  snakes->head_row = 2;
  snakes->head_col = 4;
  snakes->live = true;
  state->snakes = snakes;

  return state;
}

/* Task 2 */
void free_state(game_state_t *state) {
  for (int i = 0; i < state->num_rows; i++) {
    free(state->board[i]);
  }
  free(state->board);

  if (state->snakes) {
    free(state->snakes);
  }

  free(state);
  return;
}

/* Task 3 */
void print_board(game_state_t *state, FILE *fp) {
  for (int i = 0; i < state->num_rows; i++) {
    fprintf(fp, "%s", state->board[i]);
  }
  return;
}

/*
  Saves the current state into filename. Does not modify the state object.
  (already implemented for you).
*/
void save_board(game_state_t *state, char *filename) {
  FILE *f = fopen(filename, "w");
  print_board(state, f);
  fclose(f);
}

/* Task 4.1 */

/*
  Helper function to get a character from the board
  (already implemented for you).
*/
char get_board_at(game_state_t *state, unsigned int row, unsigned int col) {
  return state->board[row][col];
}

/*
  Helper function to set a character on the board
  (already implemented for you).
*/
static void set_board_at(game_state_t *state, unsigned int row,
                         unsigned int col, char ch) {
  state->board[row][col] = ch;
}

/*
  Returns true if c is part of the snake's tail.
  The snake consists of these characters: "wasd"
  Returns false otherwise.
*/
static bool is_tail(char c) {
  return c == 'w' || c == 'a' || c == 's' || c == 'd';
}

/*
  Returns true if c is part of the snake's head.
  The snake consists of these characters: "WASDx"
  Returns false otherwise.
*/
static bool is_head(char c) {
  return c == 'W' || c == 'A' || c == 'S' || c == 'D' || c == 'x';
}

/*
  Returns true if c is part of the snake.
  The snake consists of these characters: "wasd^<v>WASDx"
*/
static bool is_snake(char c) {
  return is_tail(c) || is_head(c) || c == '^' || c == '<' || c == 'v' ||
         c == '>';
}

/*
  Converts a character in the snake's body ("^<v>")
  to the matching character representing the snake's
  tail ("wasd").
*/
static char body_to_tail(char c) {
  switch (c) {
  case '^':
    return 'w';
  case '<':
    return 'a';
  case 'v':
    return 's';
  case '>':
    return 'd';
  }
  // 不可能走到这里
  printf("Error: body_to_tail: c is not a body: %c\n", c);
  return '?';
}

/*
  Converts a character in the snake's head ("WASD")
  to the matching character representing the snake's
  body ("^<v>").
*/
static char head_to_body(char c) {
  switch (c) {
  case 'W':
    return '^';
  case 'A':
    return '<';
  case 'S':
    return 'v';
  case 'D':
    return '>';
  }
  return '?';
}

/*
  Returns cur_row + 1 if c is 'v' or 's' or 'S'.
  Returns cur_row - 1 if c is '^' or 'w' or 'W'.
  Returns cur_row otherwise.
*/
static unsigned int get_next_row(unsigned int cur_row, char c) {
  if (c == 'v' || c == 's' || c == 'S') {
    return cur_row + 1;
  }
  if (c == '^' || c == 'w' || c == 'W') {
    return cur_row - 1;
  }
  return cur_row;
}

/*
  Returns cur_col + 1 if c is '>' or 'd' or 'D'.
  Returns cur_col - 1 if c is '<' or 'a' or 'A'.
  Returns cur_col otherwise.
*/
static unsigned int get_next_col(unsigned int cur_col, char c) {
  if (c == '>' || c == 'd' || c == 'D') {
    return cur_col + 1;
  }
  if (c == '<' || c == 'a' || c == 'A') {
    return cur_col - 1;
  }
  return cur_col;
}

/*
  Task 4.2

  Helper function for update_state. Return the character in the cell the snake
  is moving into.

  This function should not modify anything.
*/
static char next_square(game_state_t *state, unsigned int snum) {
  snake_t *snakes = &state->snakes[snum];
  unsigned int next_row = get_next_row(
      snakes->head_row, state->board[snakes->head_row][snakes->head_col]);
  unsigned int next_col = get_next_col(
      snakes->head_col, state->board[snakes->head_row][snakes->head_col]);
  return get_board_at(state, next_row, next_col);
}

/*
  Task 4.3

  Helper function for update_state. Update the head...

  ...on the board: add a character where the snake is moving

  ...in the snake struct: update the row and col of the head

  Note that this function ignores food, walls, and snake bodies when moving the
  head.
*/
static void update_head(game_state_t *state, unsigned int snum) {
  unsigned int next_row = get_next_row(
      state->snakes[snum].head_row,
      state->board[state->snakes[snum].head_row][state->snakes[snum].head_col]);
  unsigned int next_col = get_next_col(
      state->snakes[snum].head_col,
      state->board[state->snakes[snum].head_row][state->snakes[snum].head_col]);

  char cur_head_char =
      state->board[state->snakes[snum].head_row][state->snakes[snum].head_col];

  // 这里假定蛇不会死
  set_board_at(state, state->snakes[snum].head_row,
               state->snakes[snum].head_col, head_to_body(cur_head_char));
  set_board_at(state, next_row, next_col, cur_head_char);
  state->snakes[snum].head_row = next_row;
  state->snakes[snum].head_col = next_col;

  return;
}

/*
  Task 4.4

  Helper function for update_state. Update the tail...

  ...on the board: blank out the current tail, and change the new
  tail from a body character (^<v>) into a tail character (wasd)

  ...in the snake struct: update the row and col of the tail
*/
static void update_tail(game_state_t *state, unsigned int snum) {
  unsigned int next_row = get_next_row(
      state->snakes[snum].tail_row,
      state->board[state->snakes[snum].tail_row][state->snakes[snum].tail_col]);
  unsigned int next_col = get_next_col(
      state->snakes[snum].tail_col,
      state->board[state->snakes[snum].tail_row][state->snakes[snum].tail_col]);

  char cur_tail_char =
      state->board[state->snakes[snum].tail_row][state->snakes[snum].tail_col];
  if (!is_tail(cur_tail_char)) {
    // 不可能走到这里
    printf("Error: update_tail: cur_tail_char is not a tail: %c\n",
           cur_tail_char);
    return;
  }

  // 这里假定蛇没有进食
  set_board_at(state, next_row, next_col,
               body_to_tail(state->board[next_row][next_col]));
  set_board_at(state, state->snakes[snum].tail_row,
               state->snakes[snum].tail_col, ' ');
  state->snakes[snum].tail_row = next_row;
  state->snakes[snum].tail_col = next_col;
  return;
}

/* Task 4.5 */
void update_state(game_state_t *state, int (*add_food)(game_state_t *state)) {
  bool need_add_food = false;
  for (unsigned int i = 0; i < state->num_snakes; i++) {
    if (!state->snakes[i].live) {
      continue;
    }

    char next_square_char = next_square(state, i);
    switch (next_square_char) {
    case ' ':
      update_head(state, i);
      update_tail(state, i);
      break;
    case '*':
      update_head(state, i);
      need_add_food = true;
      break;
    default:
      // TODO: 碰上其他蛇的尾巴不一定会死，别人的尾巴可能之后会移动
      state->snakes[i].live = false;
      set_board_at(state, state->snakes[i].head_row, state->snakes[i].head_col,
                   'x');
      break;
    }
  }
  if (need_add_food) {
    add_food(state);
  }
  return;
}

/* Task 5.1 */
char *read_line(FILE *fp) {
  size_t capacity = 32; // 初始容量
  size_t size = 0;      // 当前使用的大小
  char *buffer = malloc(capacity);

  if (!buffer) {
    perror("Failed to allocate memory for buffer");
    return NULL;
  }

  int c;
  while ((c = fgetc(fp)) != EOF && c != '\n') {
    if (size + 1 >= capacity) {
      capacity *= 2;
      char *new_buffer = realloc(buffer, capacity);
      if (!new_buffer) {
        perror("Failed to reallocate memory");
        free(buffer);
        return NULL;
      }
      buffer = new_buffer;
    }
    buffer[size++] = (char)c;
  }

  if (size == 0 && c == EOF) {
    free(buffer);
    return NULL;
  }

  buffer[size++] = '\n'; // 添加换行符
  buffer[size] = '\0';   // 添加字符串结束符

  return realloc(buffer, size + 1); // 收缩到实际大小
}

/* Task 5.2 */
game_state_t *load_board(FILE *fp) {
  game_state_t *state = malloc(sizeof(game_state_t));
  if (!state) {
    perror("Failed to allocate memory for state");
    return NULL;
  }

  state->num_rows = 0;
  state->num_snakes = 0;

  char *line;
  unsigned int row_id = 0;
  unsigned int row_capacity = 1;
  state->board = malloc(sizeof(char *) * 1);

  while ((line = read_line(fp)) != NULL) {
    if (row_id == row_capacity) {
      row_capacity *= 2;
      state->board = realloc(state->board, sizeof(char *) * row_capacity);
    }
    state->board[row_id++] = line;
  }

  state->board = realloc(state->board, sizeof(char *) * row_id);
  state->num_rows = row_id;
  state->snakes = NULL;

  return state;
}

/*
  Task 6.1

  Helper function for initialize_snakes.
  Given a snake struct with the tail row and col filled in,
  trace through the board to find the head row and col, and
  fill in the head row and col in the struct.
*/
static void find_head(game_state_t *state, unsigned int snum) {
  int cur_id = -1;
  for (unsigned int i = 0; i < state->num_rows; i++) {
    for (unsigned int j = 0; j < strlen(state->board[i]) - 1; j++) {
      // 需要排除 \n
      char cur_char = state->board[i][j];
      if (!is_tail(cur_char)) {
        continue;
      }
      cur_id++;
      if (cur_id == snum) {
        unsigned int cur_row = i;
        unsigned int cur_col = j;
        char cur_char = state->board[cur_row][cur_col];
        while (!is_head(cur_char)) {
          unsigned int next_row = get_next_row(cur_row, cur_char);
          unsigned int next_col = get_next_col(cur_col, cur_char);
          cur_char = state->board[next_row][next_col];
          cur_row = next_row;
          cur_col = next_col;
        }
        state->snakes[snum].head_row = cur_row;
        state->snakes[snum].head_col = cur_col;
        return;
      }
    }
  }
  return;
}

/* Task 6.2 */
game_state_t *initialize_snakes(game_state_t *state) {
  state->snakes = malloc(sizeof(snake_t) * 1);
  unsigned int snake_capacity = 1;
  unsigned int snake_id = 0;
  for (unsigned int i = 0; i < state->num_rows; i++) {
    for (unsigned int j = 0; j < strlen(state->board[i]) - 1; j++) {
      char cur_char = state->board[i][j];
      if (!is_tail(cur_char)) {
        continue;
      }

      if (snake_id == snake_capacity) {
        snake_capacity *= 2;
        state->snakes =
            realloc(state->snakes, sizeof(snake_t) * snake_capacity);
      }
      state->snakes[snake_id].tail_row = i;
      state->snakes[snake_id].tail_col = j;
      find_head(state, snake_id);
      if (state->board[state->snakes[snake_id].head_row]
                      [state->snakes[snake_id].head_col] == 'x') {
        state->snakes[snake_id].live = false;
      } else {
        state->snakes[snake_id].live = true;
      }
      snake_id++;
    }
  }
  state->num_snakes = snake_id;
  state->snakes = realloc(state->snakes, sizeof(snake_t) * state->num_snakes);
  return state;
}
