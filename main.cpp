#include <graph/graph.hpp>

#include <string>
#include <signal.h>

#include <functional>

#include "graph_browser.hpp"

std::string generateRandomString(size_t length)
{
  const char charset[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
  const size_t max_index = (sizeof(charset) - 1);

  std::string str(length,0);
  for (size_t i = 0; i < length; ++i) {
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
  for (size_t i = 0; i < number_of_strings; ++i) {
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
  for (size_t i = 0; i < number_of_edges; ++i) {
    const std::string source = v[rand()%number_of_vertices];
    const std::string destination = v[rand()%number_of_vertices];
    graph.addEdge(source, destination);
  }
  return graph;
}

std::function<void(int)> callback_wrapper;

void callback_function( int value )
{
  callback_wrapper(value);
}

int main(int argc, char* argv[])
{
  Graph<std::string> g = generateRandomGraph(10, 200, 5, 20);

  GraphBrowser::init();
  {
    GraphBrowser gb(g);
    gb.setStartVertex(*g.begin());

    callback_wrapper = std::bind(&GraphBrowser::terminalResizedEvent,
                                 &gb,
                                 std::placeholders::_1);
    struct sigaction act;
    act.sa_handler = callback_function;
    sigaction(SIGWINCH, &act, NULL);

    gb.mainLoop();
  }

  GraphBrowser::destroy();
  return EXIT_SUCCESS;
}
