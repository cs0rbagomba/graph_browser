#include <graph/graph.hpp>

#include <string>

int main(int argc, char* argv[])
{
  Graph<std::string> g{ "one", "two", "three" };

  return EXIT_SUCCESS;
}