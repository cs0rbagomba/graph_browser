#include <cstring>
#include <algorithm>
#include <cassert>

#include "graph_browser.hpp"

namespace {
  std::string historyToString(const std::deque<std::string>& d, size_t max_length)
  {
    if (d.empty())
      return std::string();

    std::string s(d.back());
    for (auto rit = d.crbegin()+1; rit != d.rend(); ++rit)
      if (s.length() + (*rit).length() + 3 < max_length)
        s.insert(0, std::string(*rit) + " | ");

    return s;
  }

  std::string toCommaSeparatedList(const std::vector<std::string>& v)
  {
    std::string s;
    for (size_t i = 0; i < v.size()-1; ++i) {
      s += v[i];
      s += ",";
    }
    s += v.back();
    return s;
  }

  void inline  removeFromHistory(std::deque<std::string>& h, const std::string& s)
  {
    std::remove(h.begin(), h.end(), s);
  }

  ITEM* getNthItem(const MENU* menu, int n)
  {
    const int number_of_items = item_count(menu);
    if (number_of_items == 0 || n > number_of_items)
      return 0;

    ITEM** items = menu_items(menu);
    ITEM* retval = *items;
    for (int i = 1; i <= n; ++i)
      retval = retval->down;

    return retval;
  }
}


void GraphBrowser::init()
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

void GraphBrowser::destroy()
{
  clrtoeol();
  refresh();
  endwin();
}

GraphBrowser::GraphBrowser(Graph<std::string>& g)
  : menu_(0)
  , current_win_(0)
  , n_win(0)
  , n_of_n_win_(0)
  , graph_(g)
  , history_()
{
  // window of the current node
  menu_ = new_menu((ITEM **)0);

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
  mvprintw(TERM_MAX_Y-1, 0, "ESC:exit, cursors:navigate, d/i:del/ins edge, D/I:del/ins node");
  refresh();
}

GraphBrowser::~GraphBrowser()
{
  unpost_menu(menu_);
  free_menu(menu_);
  const int number_of_items = item_count(menu_);
  ITEM** old_items = menu_items(menu_);
  for(int i = 0; i < number_of_items; ++i)
    free_item(old_items[i]);

  /// @todo delete windows and windows' subwindows
}

void GraphBrowser::mainLoop()
{
  int c;
  while((c = wgetch(current_win_)) != KEY_ESC) {
    switch(c) {

      case KEY_DOWN:
        menu_driver(menu_, REQ_DOWN_ITEM);
        updateNeighbours();
        break;
      case KEY_UP:
        menu_driver(menu_, REQ_UP_ITEM);
        updateNeighbours();
        break;
      case KEY_LEFT: {
        if (history_.size() == 1)
          break;

        history_.pop_back();
        const std::string prev = history_.back();
        updateCurrent(prev);
        updateNeighbours();
        break;
      }
      case KEY_RIGHT: {
        ITEM* c_item = current_item(menu_);
        if (c_item == 0)
          break;

        std::string next = item_name(c_item);
        history_.push_back(next);
        updateCurrent(next);
        updateNeighbours();
        break;
      }
      case KEY_NPAGE: {
        menu_driver(menu_, REQ_SCR_DPAGE);
        break;
      }
      case KEY_PPAGE: {
        menu_driver(menu_, REQ_SCR_UPAGE);
        break;
      }

      case KEY_UPPER_CASE_D: { // delete node
        const std::string current = history_.back();
        const ITEM* c_item = current_item(menu_);
        if (c_item == 0)
          break;

        const std::string selected = item_name(c_item);

        if (selected == history_.front()) // cannot delete start node
          break;

        graph_.removeVertex(selected);
        removeFromHistory(history_, selected);

        const int selected_index = item_index(c_item);
        updateCurrent(current);

        const int number_of_items = item_count(menu_);
        if (number_of_items == 0)
          break;

        ITEM* newly_selected_item = getNthItem(menu_, std::min(selected_index, number_of_items-1));
        const int r = set_current_item(menu_, newly_selected_item);
        assert (r == E_OK);

        updateNeighbours();
        break;
      }
      case KEY_UPPER_CASE_I: { // insert node
        break;
      }
      case KEY_LOWER_CASE_D: { // delete edge
        const std::string current = history_.back();
        const ITEM* c_item = current_item(menu_);
        if (c_item == 0)
          break;

        const std::string selected = item_name(c_item);
        graph_.removeEdge(current, selected);

        const int selected_index = item_index(c_item);
        updateCurrent(current);

        const int number_of_items = item_count(menu_);
        if (number_of_items == 0)
          break;

        ITEM* newly_selected_item = getNthItem(menu_, std::min(selected_index, number_of_items-1));
        const int r = set_current_item(menu_, newly_selected_item);
        assert (r == E_OK);

        updateNeighbours();
        break;
      }
      case KEY_LOWER_CASE_I: { // insert edge
        break;
      }
    }

    wrefresh(current_win_);
  }
}



void GraphBrowser::setStartVertex(const std::string& s)
{
  history_.push_back(s);
  updateCurrent(s);
  updateNeighbours();
}


void GraphBrowser::updateCurrent(const std::string& s)
{
  const std::vector<std::string>& n = graph_.neighboursOf(s);
  addItems(n);
  updateNeighbours();

  mvprintw(0, 0, "%s",std::string(TERM_MAX_X,' ').c_str());
  mvprintw(0, 0, historyToString(history_, TERM_MAX_X).c_str());
  refresh();
}

void GraphBrowser::updateNeighbours()
{

  const size_t n_width = n_window_width-1;
  const size_t n_of_n_width = TERM_MAX_X-current_window_width-n_window_width-2;
  for (int i = 1; i < window_height-1; ++i) {
    mvwprintw(n_win, i, 1, "%s", std::string(n_width,' ').c_str());
    mvwprintw(n_of_n_win_, i, 1, "%s", std::string(n_of_n_width,' ').c_str());
  }

  ITEM* c_item = current_item(menu_);
  if (c_item != 0) {
    const std::string current = item_name(c_item);
    const std::vector<std::string>& n = graph_.neighboursOf(current);
    for (size_t i = 0; i < n.size() && i < window_height-2; ++i) {
      const size_t n_win_w = std::min(n[i].length(), n_width);
      mvwprintw(n_win, i+1, 1, std::string(n[i], 0, n_win_w).c_str());

      const std::vector<std::string>& n_of_n = graph_.neighboursOf(n[i]);
      const std::string n_of_n_string = toCommaSeparatedList(n_of_n);
      const size_t n_of_n_win__w = std::min(n_of_n_string.length(), n_of_n_width-1);
      mvwprintw(n_of_n_win_, i+1, 2,
                std::string(n_of_n_string, 0, n_of_n_win__w).c_str());
    }
  }

  assert(wrefresh(n_win) == E_OK);
  assert(wrefresh(n_of_n_win_) == E_OK);
}

void GraphBrowser::addItems(const std::vector<std::string>& stringVector)
{
  // keep pointed to deallocate old items
  ITEM** old_items = menu_items(menu_);
  const int old_items_count = item_count(menu_);

  const int u = unpost_menu(menu_);
  assert (u == E_OK || u == E_NOT_POSTED);

  const int number_of_new_items = stringVector.size();
  ITEM** new_items = 0;
  if (number_of_new_items > 0) {
    new_items = (ITEM **)calloc(number_of_new_items+1, sizeof(ITEM *));
    for(size_t i = 0; i < number_of_new_items; ++i) {
      new_items[i] = new_item(stringVector[i].c_str(), 0);
      assert (new_items[i]);
    }
    new_items[number_of_new_items] = 0; // terminating empty item
  }
  assert(set_menu_items(menu_, new_items) == E_OK);

  if (number_of_new_items > 0) {
    assert(set_menu_format(menu_, window_height-2, 1) == E_OK);
    assert(post_menu(menu_) == E_OK);
  }
  assert(wrefresh(current_win_) == OK);

  // deallocate old items
  for (int i = 0; i < old_items_count; ++i)
    assert(free_item(old_items[i]) == E_OK);
}
