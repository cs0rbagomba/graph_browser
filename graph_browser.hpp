#ifndef GRAPH_BROWSER_HPP
#define GRAPH_BROWSER_HPP

#include <curses.h>
#include <menu.h>

#include <deque>

#include <graph/graph.hpp>

class GraphBrowser {

  static constexpr int KEY_ESC = 27;
  static constexpr int KEY_LOWER_CASE_D = 100;
  static constexpr int KEY_LOWER_CASE_I = 105;
  static constexpr int KEY_UPPER_CASE_D = 68;
  static constexpr int KEY_UPPER_CASE_I = 73;

  size_t term_max_x_ = 80;
  size_t term_max_y_ = 24;
  size_t window_height = term_max_y_-2;
  size_t current_window_width = term_max_x_/4;
  size_t n_window_width = term_max_x_/4;

public:

  void static init();
  void static destroy();

  GraphBrowser(Graph<std::string>& g);
  ~GraphBrowser();

  void mainLoop();
  void setStartVertex(const std::string& s);
  void terminalResizedEvent(int);

private:

  void initLayout();
  void cleanUp();
  void updateCurrent(const std::string& s, bool first_run = false);
  void updateNeighbours();
  void addItems(const std::vector<std::string>& stringVector, bool first_run);
  void deleteElement(bool delete_node);
  void updateDimensions();

  MENU *menu_;
  WINDOW *current_win_, *menu_subwindow_, *n_win_, * n_of_n_win_;
  Graph<std::string>& graph_;

  std::deque<std::string> history_;
};

#endif // GRAPH_BROWSER_HPP
