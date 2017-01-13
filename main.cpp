#include <graph/graph.hpp>

#include <string>
#include <queue>
#include <deque>

#include <cstring>

#include <curses.h>
#include <menu.h>

std::string generateRandomString(size_t length)
{
  const char charset[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
  const size_t max_index = (sizeof(charset) - 1);

  std::string str(length,0);
  for (int i = 0; i < length; ++i) {
    const size_t r = rand() % max_index;
    str[i] =  charset[r];
  }
  return str;
}

std::vector<std::string> generateRandomStrings(size_t number_of_strings,
                                               size_t min_length,
                                               size_t max_length)
{
  std::vector<std::string> v;
  v.resize(number_of_strings);
  for (int i = 0; i < number_of_strings; ++i) {
    const size_t l = min_length + rand()%(max_length - min_length);
    v[i] = generateRandomString(l);
  }
  return v;
}

Graph<std::string> generateRandomGraph(size_t number_of_vertices,
                                       size_t number_of_edges,
                                       size_t vertex_text_min_length,
                                       size_t vertex_text_max_length)
{
  Graph<std::string> graph;
  const std::vector<std::string> v = generateRandomStrings(number_of_vertices,
                                                           vertex_text_min_length,
                                                           vertex_text_max_length);
  for (int i = 0; i < number_of_edges; ++i) {
    const std::string source = v[rand()%number_of_vertices];
    const std::string destination = v[rand()%number_of_vertices];
    graph.addEdge(source, destination);
  }
  return graph;
}

class NCursesInterface {
public:

  static constexpr size_t TERM_MAX_X = 80;
  static constexpr size_t TERM_MAX_Y = 24;
  static constexpr int KEY_ESC = 27;
  static constexpr size_t current_window_width = TERM_MAX_X/4;
  static constexpr size_t current_window_height = TERM_MAX_Y-2;

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

  NCursesInterface(const Graph<std::string>& g)
    : menu_(0)
    , current_win_(0)
    , items_(0)
    , number_of_choices_(0)

    , graph_(g)

    , current_()
    , history_()
    , history_string_()
  {
    menu_ = new_menu((ITEM **)items_);

    current_win_ = newwin(current_window_height, current_window_width, 1, 0);
    keypad(current_win_, TRUE);

    set_menu_win(menu_, current_win_);
    set_menu_sub(menu_, derwin(current_win_, current_window_height-4, current_window_width-2, 3, 1));
    set_menu_format(menu_, current_window_height-4, 1);

    set_menu_mark(menu_, " ");

    box(current_win_, 0, 0);

    mvwaddch(current_win_, 2, 0, ACS_LTEE);
    mvwhline(current_win_, 2, 1, ACS_HLINE, current_window_width-2);
    mvwaddch(current_win_, 2, current_window_width-1, ACS_RTEE);
    mvprintw(TERM_MAX_Y-1, 0, "ESC to exit, cursor keys to navigate");
    refresh();

    post_menu(menu_);
    wrefresh(current_win_);
  }

  ~NCursesInterface()
  {
    unpost_menu(menu_);
    free_menu(menu_);
    for(int i = 0; i < number_of_choices_; ++i)
      free_item(items_[i]);
  }

  void mainLoop()
  {
    int c;
    while((c = wgetch(current_win_)) != KEY_ESC) {
      switch(c) {
        case KEY_DOWN:
          menu_driver(menu_, REQ_DOWN_ITEM);
          break;
        case KEY_UP:
          menu_driver(menu_, REQ_UP_ITEM);
          break;
        case KEY_LEFT: {
          if (history_.empty())
            break;

          const std::string prev = history_.back();
          history_.pop_back();
          update(prev);
          break;
        }
        case KEY_RIGHT: {
          std::string next = item_name(current_item(menu_));
          history_.push_back(current_);
          update(next);
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
    current_ = s;
    const std::vector<std::string>& n = graph_.neighboursOf(current_);
    printInTheMiddle(current_win_, current_.c_str());
    addItems(n);
  }

private:

  void update(const std::string& s) {
    const std::vector<std::string>& n = graph_.neighboursOf(s);
    current_ = s;
    printInTheMiddle(current_win_, s.c_str());
    addItems(n);

    history_string_ = historyToString();
    while (history_string_.length() > TERM_MAX_X) {
      history_.pop_front();
      history_string_ = historyToString();
    }
    mvprintw(0, 0, "%s",std::string(TERM_MAX_X,' ').c_str());
    mvprintw(0, 0, history_string_.c_str());
    refresh();
  }

  void addItems(const std::vector<std::string>& stringVector)
  {
    unpost_menu(menu_);
    for(int i = 0; i < number_of_choices_; ++i)
      free_item(items_[i]);

    number_of_choices_ = stringVector.size();
    items_ = (ITEM **)calloc(number_of_choices_+1, sizeof(ITEM *));
    for(size_t i = 0; i < number_of_choices_; ++i)
      items_[i] = new_item(stringVector[i].c_str(), 0);

    items_[number_of_choices_] = new_item(0, 0);
    set_menu_items(menu_, items_);
    set_menu_format(menu_, 6, 1);

    post_menu(menu_);
    wrefresh(current_win_);
  }

  std::string historyToString() const {
    if (history_.empty())
      return std::string();

    std::string s;
    for (size_t i = 0; i < history_.size()-1; ++i) {
      s += history_[i];
      s += " | ";
    }
    s += history_[history_.size()-1];
    return s;
  }

  void printInTheMiddle(WINDOW *win, const std::string& s, int line = 1, int border = 1)
  {
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);

    const int usable_x = max_x-border*2 -1;
    const int x = std::max(2, (int)((max_x - s.length()) / 2));
    const std::string s1 = s.length() > usable_x ? std::string(s, 0, usable_x) : s;

    mvwprintw(win, line, border, "%s", std::string(usable_x+1,' ').c_str());
    mvwprintw(win, line, x, "%s", s1.c_str());
    refresh();
  }

  MENU *menu_;
  WINDOW *current_win_;
  ITEM **items_;
  int number_of_choices_;

  const Graph<std::string>& graph_;

  std::string current_;
  std::deque<std::string> history_;
  std::string history_string_;
};



int main(int argc, char* argv[])
{
  const Graph<std::string> g = generateRandomGraph(10, 200, 5, 20);

  NCursesInterface::init();

  NCursesInterface ni(g);
  ni.setCurrentVertex(*g.begin());
  ni.mainLoop();

  NCursesInterface::destroy();
  return EXIT_SUCCESS;
}
