#include <curses.h>
#include <menu.h>

#include <deque>

#include <cstring>

#include <graph/graph.hpp>

class GraphBrowser {
public:

  static constexpr size_t TERM_MAX_X = 80;
  static constexpr size_t TERM_MAX_Y = 24;
  static constexpr int KEY_ESC = 27;
  static constexpr size_t window_height = TERM_MAX_Y-2;
  static constexpr size_t current_window_width = TERM_MAX_X/4;
  static constexpr size_t n_window_width = TERM_MAX_X/4;

  void static init()
  {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    //Needed to have ‘immediate’ ESC-Key behavior:
    if (getenv ("ESCDELAY") == NULL) {
      set_escdelay(25);
    }
  }

  void static destroy()
  {
    clrtoeol();
    refresh();
    endwin();
  }

  GraphBrowser(const Graph<std::string>& g)
    : menu_(0)
    , current_win_(0)
    , n_win(0)
    , n_of_n_win_(0)
    , items_(0)
    , graph_(g)
    , history_()
  {
    // window of the current node
    menu_ = new_menu((ITEM **)items_);

    current_win_ = newwin(window_height, current_window_width, 1, 0);
    keypad(current_win_, TRUE);

    set_menu_win(menu_, current_win_);
    set_menu_sub(menu_, derwin(current_win_, window_height-2, current_window_width-2, 1, 1));
    set_menu_format(menu_, window_height-2, 1);

    set_menu_mark(menu_, " ");

    wborder(current_win_, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE,
                          ACS_ULCORNER, ACS_TTEE, ACS_LLCORNER, ACS_BTEE);
    post_menu(menu_);
    refresh();
    wrefresh(current_win_);

    n_win = newwin(window_height, n_window_width, 1, current_window_width);
    wborder(n_win, ' ', ' ', ACS_HLINE, ACS_HLINE,
                   ACS_HLINE, ACS_HLINE, ACS_HLINE, ACS_HLINE);
    refresh();
    wrefresh(n_win);

    // window of the neighbours'neighbours of the current vertex
    n_of_n_win_ = newwin(window_height, TERM_MAX_X-current_window_width-n_window_width,
                      1, n_window_width+current_window_width);
    wborder(n_of_n_win_, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE,
                      ACS_TTEE, ACS_URCORNER, ACS_BTEE, ACS_LRCORNER);
    refresh();
    wrefresh(n_of_n_win_);

    // bottom text
    mvprintw(TERM_MAX_Y-1, 0, "ESC to exit, cursor keys to navigate");
    refresh();
  }

  ~GraphBrowser()
  {
    unpost_menu(menu_);
    free_menu(menu_);
    const int number_of_items = item_count(menu_);
    for(int i = 0; i < number_of_items; ++i)
      free_item(items_[i]);

    /// @todo delete windows and windows' subwindows
  }

  void mainLoop()
  {
    int c;
    while((c = wgetch(current_win_)) != KEY_ESC) {
      switch(c) {
        case KEY_DOWN:
          menu_driver(menu_, REQ_DOWN_ITEM);
          update_neighbours();
          break;
        case KEY_UP:
          menu_driver(menu_, REQ_UP_ITEM);
          update_neighbours();
          break;
        case KEY_LEFT: {
          if (history_.size() == 1)
            break;

          history_.pop_back();
          const std::string prev = history_.back();
          update_current(prev);
          update_neighbours();
          break;
        }
        case KEY_RIGHT: {
          std::string next = item_name(current_item(menu_));
          history_.push_back(next);
          update_current(next);
          update_neighbours();
          break;
        }
        case KEY_NPAGE:
          menu_driver(menu_, REQ_SCR_DPAGE);
          break;
        case KEY_PPAGE:
          menu_driver(menu_, REQ_SCR_UPAGE);
          break;

        }
      wrefresh(current_win_);
    }
  }



  void setCurrentVertex(const std::string& s)
  {
    history_.push_back(s);
    update_current(s);
  }

private:

  void update_current(const std::string& s)
  {
    const std::vector<std::string>& n = graph_.neighboursOf(s);
    addItems(n);

    mvprintw(0, 0, "%s",std::string(TERM_MAX_X,' ').c_str());
    mvprintw(0, 0, historyToString().c_str());
    refresh();
  }

  void update_neighbours()
  {

    const size_t n_width = n_window_width-1;
    const size_t n_of_n_width = TERM_MAX_X-current_window_width-n_window_width-3;
    for (int i = 1; i < window_height-1; ++i) {
      mvwprintw(n_win, i, 1, "%s", std::string(n_width,' ').c_str());
      mvwprintw(n_of_n_win_, i, 1, "%s", std::string(n_of_n_width,' ').c_str());
    }

    const std::string current = item_name(current_item(menu_));
    const std::vector<std::string>& n = graph_.neighboursOf(current);
    for (size_t i = 0; i < n.size() && i < window_height-2; ++i) {
      mvwprintw(n_win, i+1, 1, std::string(n[i], 0, std::min(n[i].length(), n_width)).c_str());

      const std::vector<std::string>& n_of_n = graph_.neighboursOf(n[i]);
      const std::string n_of_n_string = neighboursToString(n_of_n);
      mvwprintw(n_of_n_win_, i+1, 2, std::string(n_of_n_string, 0, std::min(n_of_n_string.length(), n_of_n_width)).c_str());
    }

    wrefresh(n_win);
    wrefresh(n_of_n_win_);
  }

  void addItems(const std::vector<std::string>& stringVector)
  {
    unpost_menu(menu_);
    const int number_of_items = item_count(menu_);
    for(int i = 0; i < number_of_items; ++i)
      free_item(items_[i]);

    const int number_of_new_items = stringVector.size();
    items_ = (ITEM **)calloc(number_of_new_items+1, sizeof(ITEM *));
    for(size_t i = 0; i < number_of_new_items; ++i)
      items_[i] = new_item(stringVector[i].c_str(), 0);

    items_[number_of_new_items] = new_item(0, 0);

    set_menu_items(menu_, items_);
    set_menu_format(menu_, window_height-2, 1);

    post_menu(menu_);
    wrefresh(current_win_);
  }

  std::string historyToString() const
  {
    if (history_.empty())
      return std::string();

    std::string s(history_.back());
    for (auto rit = history_.crbegin()+1; rit != history_.rend(); ++rit)
      if (s.length() + (*rit).length() + 3 < TERM_MAX_X)
        s.insert(0, std::string(*rit) + " | ");

    return s;
  }

  std::string neighboursToString(const std::vector<std::string>& n) const
  {
    std::string s;
    for (size_t i = 0; i < n.size()-1; ++i) {
      s += n[i];
      s += ",";
    }
    s += n.back();
    return s;
  }

  MENU *menu_;
  WINDOW *current_win_, *n_win, * n_of_n_win_;
  ITEM **items_;
  const Graph<std::string>& graph_;

  std::deque<std::string> history_;
};