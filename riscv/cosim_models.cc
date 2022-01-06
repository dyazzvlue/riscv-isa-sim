#include "cosim_model.h"
#include <string>
#include <map>
#include <dlfcn.h>

static std::map<std::string, std::function<cosim_model_t*()>>& cosim_models()
{
  static std::map<std::string, std::function<cosim_model_t*()>> v;
  return v;
}

void register_cosim_model(const char* name, std::function<cosim_model_t*()> f)
{
  cosim_models()[name] = f;
}

std::function<cosim_model_t*()> find_cosim_model(const char* name)
{
  if (!cosim_models().count(name)) {
    // try to find cosim_model xyz by loading libxyz.so
    std::string libname = std::string("lib") + name + ".so";
    std::string libdefault = "libcustomext.so";
    bool is_default = false;
    auto dlh = dlopen(libname.c_str(), RTLD_LAZY);
    if (!dlh) {
      dlh = dlopen(libdefault.c_str(), RTLD_LAZY);
      if (!dlh) {
        fprintf(stderr, "couldn't find shared library either '%s' or '%s')\n",
                libname.c_str(), libdefault.c_str());
        exit(-1);
      }

      is_default = true;
    }

    if (!cosim_models().count(name)) {
      fprintf(stderr, "couldn't find cosim_model '%s' in shared library '%s'\n",
              name, is_default ? libdefault.c_str() : libname.c_str());
      exit(-1);
    }
  }

  return cosim_models()[name];