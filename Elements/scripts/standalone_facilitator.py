#!/bin/python
#
# GAMBIT: Global and Modular BSM Inference Tool
#*********************************************
# \file
#
#  Functor type aglomeration script for
#  GAMBIT standalones.  Requires module_harvester
#  script to have been run previously, in order
#  to generate <module_name>.functors in the
#  build dir.
#
#*********************************************
#
#  Authors (add name and date if you modify):
#
#  \author Pat Scott
#          (p.scott@imperial.ac.uk)
#    \date 2016 Aug
#
#*********************************************

import pickle
execfile("./Utils/scripts/harvesting_tools.py")

def main(argv):

    # List of modules to include
    modules=set([])

    # Handle command line options
    verbose = False
    try:
        executablename = argv[0]
        opts, args = getopt.getopt(argv[1:],"vm:",["verbose","use-modules="])
    except getopt.GetoptError:
        print 'Usage: standalone_facilitator.py [flags]'
        print ' flags:'
        print '        -v                     : More verbose output'
        print '        -m module1,module2,... : Link against module1, module2, etc.'
        sys.exit(2)
    for opt, arg in opts:
      if opt in ('-v','--verbose'):
        verbose = True
        print 'standalone_facilitator.py: verbose=True'
      elif opt in ('-m','--use-modules'):
        modules_list = neatsplit(",",arg)
        modules.update(modules_list)

    # Retrieve the pickled module types.
    try:
        with open('./scratch/harvested_types.pickle', 'rb') as handle:
            returned_types = pickle.load(handle)
    except:
        print "Could not open previously harvested module functor types."
        print "filename: ./scratch/harvested_types.pickle"
        exit(2)

    # Generate the union of types.
    all_types = set([])
    for x in modules:
        if x in returned_types:
            all_types.update(returned_types[x])

    # Generate a c++ source file containing all the explicit functor template specialisations needed by the standalone program.
    towrite = "\
//   GAMBIT: Global and Modular BSM Inference Tool\n\
//   *********************************************\n\
///  \\file                                       \n\
///                                               \n\
///  Explicit functor template class              \n\
///  instantiations needed by standalone program  \n\
///  "+executablename+".                          \n\
///                                               \n\
///  This file was automatically generated by     \n\
///  standalone_facilitator.py. Do not modify.    \n\
///  The content is harvested from the rollcall   \n\
///  headers registered in module_rollcall.hpp    \n\
///  and the types registered in                  \n\
///  types_rollcall.hpp.                          \n\
///                                               \n\
///  *********************************************\n\
///                                               \n\
///  Authors:                                     \n\
///                                               \n\
///  \\author The GAMBIT Collaboration            \n\
///  \date "+datetime.datetime.now().strftime("%I:%M%p on %B %d, %Y")+"\n\
///                                               \n\
///  *********************************************\n\
                                                  \n\
#include \"gambit/Elements/functor_definitions.hpp\"\n\
#include \"gambit/Elements/types_rollcall.hpp\"   \n\
#include \"gambit/Backends/backend_functor_types.hpp\"\n\
                                                  \n\
namespace Gambit                                  \n\
{                                                 \n\
  // Non-module types                             \n\
  template class module_functor<void>;            \n"
    for t in returned_types["non_module"]:
        towrite += "  template class module_functor<"+t+">;\n"
    if all_types:
        towrite += "  // Module types\n"
        for t in all_types: towrite += "  template class module_functor<"+t+">;\n"
    else:
        towrite += "  // No module-specific types required.\n"
    towrite += "}\n\n\
// Define standalone versions of functor signal helpers (that do nothing)\n\
namespace Gambit                                                      \n\
{                                                                     \n\
  namespace FunctorHelp                                               \n\
  {                                                                   \n\
    void check_for_shutdown_signal(module_functor_common&) {}         \n\
    bool emergency_shutdown_begun() { return false; }                 \n\
    void entering_multithreaded_region(module_functor_common&) {}     \n\
    void leaving_multithreaded_region(module_functor_common&) {}      \n\
  }                                                                   \n\
}\n"

    # Don't touch any existing file unless it is actually different from what we will create
    filename = "./"+modules_list[1]+"/examples/functors_for_"+executablename+".cpp"
    candidate = "./scratch/functors_for_"+executablename+".cpp.candidate"
    with open(candidate,"w") as f: f.write(towrite)
    update_only_if_different(filename, candidate)

    if verbose:
        print "Generated "+filename+"."


# Handle command line arguments (verbosity)
if __name__ == "__main__":
   main(sys.argv[1:])

