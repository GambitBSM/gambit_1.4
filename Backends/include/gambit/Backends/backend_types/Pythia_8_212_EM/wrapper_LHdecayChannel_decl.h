#ifndef __wrapper_LHdecayChannel_decl_Pythia_8_212_EM_h__
#define __wrapper_LHdecayChannel_decl_Pythia_8_212_EM_h__

#include <cstddef>
#include "forward_decls_wrapper_classes.h"
#include "gambit/Backends/wrapperbase.hpp"
#include "abstract_LHdecayChannel.h"
#include <vector>
#include <string>

#include "identification.hpp"

namespace CAT_3(BACKENDNAME,_,SAFE_VERSION)
{
    
    namespace Pythia8
    {
        
        class LHdecayChannel : public WrapperBase<Pythia8::Abstract_LHdecayChannel>
        {
            public:
                typedef WrapperBase<Pythia8::Abstract_LHdecayChannel> wrapperbase;
                using WrapperBase<Pythia8::Abstract_LHdecayChannel>::BEptr;
        
                // Member variables: 
            public:
                // -- Static factory pointers: 
                static Pythia8::Abstract_LHdecayChannel* (*__factory0)();
                static Pythia8::Abstract_LHdecayChannel* (*__factory1)(double, int, ::std::vector<int, std::allocator<int> >, ::std::basic_string<char, std::char_traits<char>, std::allocator<char> >);
                static Pythia8::Abstract_LHdecayChannel* (*__factory2)(double, int, ::std::vector<int, std::allocator<int> >);
        
                // -- Other member variables: 
        
                // Member functions: 
            public:
                void setChannel(double bratIn, int nDaIn, ::std::vector<int, std::allocator<int> > idDaIn, ::std::basic_string<char, std::char_traits<char>, std::allocator<char> > cIn);
        
                void setChannel(double bratIn, int nDaIn, ::std::vector<int, std::allocator<int> > idDaIn);
        
                void setBrat(double bratIn);
        
                void setIdDa(::std::vector<int, std::allocator<int> > idDaIn);
        
                double getBrat();
        
                int getNDa();
        
                ::std::vector<int, std::allocator<int> > getIdDa();
        
                ::std::basic_string<char, std::char_traits<char>, std::allocator<char> > getComment();
        
        
                // Wrappers for original constructors: 
            public:
                LHdecayChannel();
                LHdecayChannel(double bratIn, int nDaIn, ::std::vector<int, std::allocator<int> > idDaIn, ::std::basic_string<char, std::char_traits<char>, std::allocator<char> > cIn);
                LHdecayChannel(double bratIn, int nDaIn, ::std::vector<int, std::allocator<int> > idDaIn);
        
                // Special pointer-based constructor: 
                LHdecayChannel(Pythia8::Abstract_LHdecayChannel* in);
                LHdecayChannel(Pythia8::Abstract_LHdecayChannel* const & in, bool);
        
                // Copy constructor: 
                LHdecayChannel(const LHdecayChannel& in);
        
                // Assignment operator: 
                LHdecayChannel& operator=(const LHdecayChannel& in);
        
                // Destructor: 
                ~LHdecayChannel();
        
            private:
                // Member variable initialiser: 
                void _memberVariablesInit();
        
        };
    }
    
}


#include "gambit/Backends/backend_undefs.hpp"

#endif /* __wrapper_LHdecayChannel_decl_Pythia_8_212_EM_h__ */