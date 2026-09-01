#include <array>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <utility>

class TicTacToe {
 private:
  std::array<std::array<char, 3>, 3> board{};
  char current_player;
  bool game_over = false;
  char winner = '-';
  mutable std::mutex board_mutex;
  std::condition_variable turn_changed;

  void display_board_unlocked() const {
    std::cout << "\nJogada realizada:\n";
    for (int i = 0; i < 3; ++i) {
      std::cout << ' ' << board[i][0] << " | " << board[i][1] << " | "
                << board[i][2] << '\n';
      if (i != 2) std::cout << "-----------\n";
    }
    std::cout << std::flush;
  }

  bool check_win_unlocked(char player) {
    for (int i = 0; i < 3; ++i) {
      if (player == board[i][0] && player == board[i][1] &&
          player == board[i][2]) {
        winner = player;
        return true;
      }
      if (player == board[0][i] && player == board[1][i] &&
          player == board[2][i]) {
        winner = player;
        return true;
      }
    }
    if ((player == board[0][0] && player == board[1][1] &&
         player == board[2][2]) ||
        (player == board[0][2] && player == board[1][1] &&
         player == board[2][0])) {
      winner = player;
      return true;
    }
    return false;
  }

  bool check_draw_unlocked() const {
    for (const auto& row : board)
      for (char position : row)
        if (position == ' ') return false;
    return true;
  }

 public:
  TicTacToe() {
    for (auto& row : board) row.fill(' ');
    std::mt19937 generator(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<int> distribution(0, 1);
    current_player = distribution(generator) == 0 ? 'X' : 'O';
  }

  void display_board() const {
    std::lock_guard<std::mutex> lock(board_mutex);
    display_board_unlocked();
    std::cout << "Primeiro jogador: " << current_player << "\n";
  }

  bool make_move(char player, int row, int col) {
    std::unique_lock<std::mutex> lock(board_mutex);
    turn_changed.wait(lock, [this, player] {
      return game_over || current_player == player;
    });

    if (game_over) return true;
    if (row < 0 || row >= 3 || col < 0 || col >= 3 ||
        board[row][col] != ' ') {
      return false;
    }

    board[row][col] = player;
    display_board_unlocked();

    if (check_win_unlocked(player)) {
      game_over = true;
    } else if (check_draw_unlocked()) {
      winner = 'D';
      game_over = true;
    } else {
      current_player = player == 'X' ? 'O' : 'X';
    }

    lock.unlock();
    turn_changed.notify_all();
    return true;
  }

  char get_winner() const {
    std::lock_guard<std::mutex> lock(board_mutex);
    return winner;
  }
};

class Player {
 private:
  TicTacToe& game;
  char symbol;
  std::string strategy;

  void play_sequential() {
    for (int row = 0; row < 3; ++row)
      for (int col = 0; col < 3; ++col)
        if (game.make_move(symbol, row, col)) return;
  }

  void play_random() {
    thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(0, 2);
    while (!game.make_move(symbol, distribution(generator),
                           distribution(generator))) {
    }
  }

 public:
  Player(TicTacToe& game_instance, char player_symbol,
         std::string player_strategy)
      : game(game_instance),
        symbol(player_symbol),
        strategy(std::move(player_strategy)) {}

  void play() {
    while (game.get_winner() == '-') {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (strategy == "sequential")
        play_sequential();
      else
        play_random();
    }
  }
};

int main() {
  TicTacToe game;
  game.display_board();
  Player player_x(game, 'X', "sequential");
  Player player_o(game, 'O', "random");

  std::thread thread_x(&Player::play, &player_x);
  std::thread thread_o(&Player::play, &player_o);
  thread_x.join();
  thread_o.join();

  const char winner = game.get_winner();
  if (winner == 'D')
    std::cout << "\nResultado: empate!\n";
  else
    std::cout << "\nResultado: vencedor " << winner << "!\n";
  return 0;
}
