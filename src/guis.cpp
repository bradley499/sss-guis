#include "guis.hpp"
#include "generation.hpp"

using namespace sss::guis;

Guis::Guis(std::filesystem::path const &configuration_file, std::filesystem::path const &m_output_directory)
    : m_configuration_directory(configuration_file),
      m_output_directory(m_output_directory)
{
}

Guis::~Guis()
{
}

void Guis::generate(bool const disallow_conflicts, bool const flatten_dependency_references, std::ostream const *debug_stream)
{
    Generation(m_configuration_directory, m_output_directory).build_all(disallow_conflicts, flatten_dependency_references, debug_stream);
}
