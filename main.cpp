#include <graph/graph.hpp>

#include <string>
#include <queue>
#include <functional>

// https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
std::string GenerateRandomString(size_t length)
{
    auto GenerateRandomChar = []() -> char
    {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, GenerateRandomChar );
    return str;
}

std::vector<std::string> GenerateRandomStrings(size_t number_of_strings,
                                               size_t min_length,
                                               size_t max_length)
{
  std::vector<std::string> v;
  v.resize(number_of_strings);
  for (int i = 0; i < number_of_strings; ++i) {
    const size_t l = min_length + rand()%(max_length - min_length);
    v[i] = GenerateRandomString(l);
  }

  return v;
}

Graph<std::string> GenerateRandomGraph(size_t number_of_vertices,
                                       size_t number_of_edges,
                                       size_t vertex_text_min_length,
                                       size_t vertex_text_max_length)
{
  Graph<std::string> graph;
  const std::vector<std::string> v = GenerateRandomStrings(number_of_vertices,
                                                           vertex_text_min_length,
                                                           vertex_text_max_length);
  for (int i = 0; i < number_of_edges; ++i)
    graph.addEdge(v[rand()%number_of_vertices], v[rand()%number_of_vertices]);

  return graph;
}

int main(int argc, char* argv[])
{
  const Graph<std::string> g = GenerateRandomGraph(10, 200, 5, 20);



  return EXIT_SUCCESS;
}