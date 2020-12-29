#include <algorithm>
#include <iostream>
#include <random>
#include <set>
#include <unistd.h>
#include <vector>

#include <ncurses.h>

struct Vec {
  int row;
  int col;

  bool operator<(const Vec &rhs) const {
    if (row == rhs.row) {
      return col < rhs.col;
    }
    return row < rhs.row;
  }
  bool operator==(const Vec &rhs) const {
    return row == rhs.row && col == rhs.col;
  }

  bool operator!=(const Vec &rhs) const { return !(*this == rhs); }

  Vec operator+(const Vec &rhs) const {
    return Vec{row + rhs.row, col + rhs.col};
  }
};

const int BOARD_WIDTH = 80;
const int BOARD_HEIGHT = 30;
const int NUM_CHERRIES = 10;

std::vector<Vec> cherries;

Vec snake_head;
std::vector<Vec> snake_tail;
std::set<Vec> snake_tail_set;

const Vec Up{-1, 0};
const Vec Down{1, 0};
const Vec Left{0, -1};
const Vec Right{0, 1};

Vec snake_facing;

std::default_random_engine gen;

void add_cherry() {
  std::uniform_int_distribution<int> row_dist(0, BOARD_HEIGHT - 1);
  std::uniform_int_distribution<int> col_dist(0, BOARD_WIDTH - 1);

  Vec cherry;

  bool in_snake;
  do {
    cherry.row = row_dist(gen);
    cherry.col = col_dist(gen);

    in_snake = (cherry == snake_head) ||
               (snake_tail_set.find(cherry) != snake_tail_set.end());

  } while (in_snake);

  cherries.push_back(cherry);
}

void setup() {
  gen.seed(std::random_device()());

  snake_facing = Right;
  snake_head = Vec{BOARD_HEIGHT / 2, BOARD_WIDTH / 2};
  snake_tail.clear();
  snake_tail_set.clear();
  snake_tail.push_back(snake_head + Left);
  snake_tail_set.insert(snake_head + Left);

  for (int i = 0; i < NUM_CHERRIES; ++i) {
    add_cherry();
  }
}

void game_over() {
  timeout(-1);
  mvprintw(BOARD_HEIGHT, BOARD_WIDTH / 2, "You lost!");
  getch();
  endwin();
  exit(69);
}

void wrap(Vec &square) {
  square.row = (square.row % BOARD_HEIGHT + BOARD_HEIGHT) % BOARD_HEIGHT;
  square.col = (square.col % BOARD_WIDTH + BOARD_WIDTH) % BOARD_WIDTH;
}

void tick() {
  Vec new_snake_head = snake_head + snake_facing;

  wrap(new_snake_head);

  for (const Vec &square : snake_tail) {
    if (new_snake_head == square) {
      game_over();
    }
  }

  bool snake_grows = false;

  bool ate_cherry = false;

  for (auto it = cherries.begin(); it != cherries.end(); ++it) {
    if (new_snake_head == *it) {
      snake_grows = true;
      cherries.erase(it);
      ate_cherry = true;
      break;
    }
  }

  snake_tail.insert(snake_tail.begin(), snake_head);
  snake_tail_set.insert(snake_head);

  snake_head = new_snake_head;
  if (!snake_grows) {
    snake_tail_set.erase(snake_tail.back());
    snake_tail.pop_back();
  }

  if (ate_cherry) {
    add_cherry();
  }
}

void print_game() {
  clear();

  for (int row = 0; row < BOARD_HEIGHT; ++row) {
    for (int col = 0; col < BOARD_WIDTH; ++col) {
      Vec square{row, col};
      if (square == snake_head) {
        char head;
        if (snake_facing == Up) {
          head = '^';
        } else if (snake_facing == Down) {
          head = 'v';
        } else if (snake_facing == Right) {
          head = '>';
        } else if (snake_facing == Left) {
          head = '<';
        }
        mvprintw(row, 2 * col, "%c", head);
      } else if (std::find(cherries.begin(), cherries.end(), square) !=
                 cherries.end()) {
        mvprintw(row, 2 * col, "O");
      } else if (snake_tail_set.find(square) != snake_tail_set.end()) {
        mvprintw(row, 2 * col, "x");
      } else {
        mvprintw(row, 2 * col, " ");
      }
    }
  }
}

void player() {
  int c = getch();

  if (c == 'a' && snake_facing != Right) {
    snake_facing = Left;
  } else if (c == 'w' && snake_facing != Down) {
    snake_facing = Up;
  } else if (c == 's' && snake_facing != Up) {
    snake_facing = Down;
  } else if (c == 'd' && snake_facing != Left) {
    snake_facing = Right;
  }
}

void cpu() {
  int map[BOARD_HEIGHT][BOARD_WIDTH];

  for (int row = 0; row < BOARD_HEIGHT; ++row) {
    for (int col = 0; col < BOARD_WIDTH; ++col) {
      map[row][col] = 999999;
    }
  }

  for (const Vec &cherry : cherries) {
    map[cherry.row][cherry.col] = 0;
  }

  bool changed;
  do {
    changed = false;
    for (int row = 0; row < BOARD_HEIGHT; ++row) {
      for (int col = 0; col < BOARD_WIDTH; ++col) {
        Vec square{row, col};
        if (square == snake_head ||
            snake_tail_set.find(square) != snake_tail_set.end()) {
          continue;
        }

        Vec l = square + Left;
        Vec r = square + Right;
        Vec u = square + Up;
        Vec d = square + Down;

        int min = 999999;
        for (Vec dir : {Up, Left, Down, Right}) {
          Vec s = square + dir;
          wrap(s);
          min = std::min(min, map[s.row][s.col]);
        }

        if (map[row][col] > min + 1) {
          map[row][col] = min + 1;
          changed = true;
        }
      }
    }
  } while (changed);

  int min = 999999;
  for (Vec dir : {Up, Left, Down, Right}) {
    Vec s = snake_head + dir;
    wrap(s);
    if (map[s.row][s.col] < min) {
      min = map[s.row][s.col];
      snake_facing = dir;
    }
  }
}

int main() {
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  timeout(50);

  setup();

  print_game();
  while (true) {
    cpu();
    // player();

    tick();

    print_game();
    refresh();
    // usleep(100 * 1000);
  }
}
