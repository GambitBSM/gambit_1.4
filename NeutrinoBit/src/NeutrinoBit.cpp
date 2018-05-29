//   GAMBIT: Global and Modular BSM Inference Tool
//   *********************************************
///  \file
///
///  Function definitions of NeutrinoBit.
///
///  *********************************************
///
///  Authors (add name and date if you modify):
///
///  \author Tomas Gonzalo
///          (t.e.gonzalo@fys.uio.no)
///  \date 2017 July
///
///  \author Julia Harz
///          (jharz@lpthe.jussieu.fr)
///  \date 2018 April
///
///  *********************************************
#define _USE_MATH_DEFINES
#include "gambit/Elements/gambit_module_headers.hpp"
#include "gambit/NeutrinoBit/NeutrinoBit_rollcall.hpp"
#include <unsupported/Eigen/MatrixFunctions>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include "gambit/NeutrinoBit/spline.h"
#include "gambit/Utils/statistics.hpp"

namespace Gambit
{

  namespace NeutrinoBit
  {

    using namespace LogTags;

    // Aux function
    double gauss_like(double x, double x0, double xerr)
    {
      return -0.5*pow(x-x0, 2)/xerr/xerr;
    }

    // Module functions
    void ordering(bool &ord)
    {
      using namespace Pipes::ordering;

      if(*Param["mNu3"] < *Param["mNu1"])
        ord = 0; // Inverted ordering
      else
        ord = 1; // Normal ordering
    }


    // Neutrino mass matrix from true SM neutrino model
    void M_nu(Eigen::Matrix3cd& m_nu)
    {
      using namespace Pipes::M_nu;

      double mnu1 = *Param["mNu1"];
      double mnu2 = *Param["mNu2"];
      double mnu3 = *Param["mNu3"];

       
      m_nu(0,1) = 0.0;
      m_nu(0,2) = 0.0;
      m_nu(1,0) = 0.0;
      m_nu(1,2) = 0.0;
      m_nu(2,0) = 0.0;
      m_nu(2,1) = 0.0;

 
      m_nu(0,0) = mnu1;
      m_nu(1,1) = mnu2;
      m_nu(2,2) = mnu3;

      // If there is an option to set a specific ordering, invalidate the other type
      int ord = runOptions->getValueOrDef<int>(-1,"ordering");
      if(ord == 1 and *Dep::ordering == 0)
      {
        std::ostringstream msg;
        msg << "Wrong ordering";
        logger() << msg.str() << EOM;
        invalid_point().raise(msg.str());
      }
      else if(ord == 0 and *Dep::ordering == 1)
      {
        std::ostringstream msg;
        msg << "Wrong ordering";
        logger() << msg.str() << EOM;
        invalid_point().raise(msg.str());
      }

    }

    void md21(double &m21)
    {
      using namespace Pipes::md21;
      Eigen::Matrix3cd mnu = *Dep::m_nu;

      m21 = pow(mnu(1,1).real(),2) - pow(mnu(0,0).real(), 2);
    }

    void md31(double &m31)
    {
      using namespace Pipes::md31;
      Eigen::Matrix3cd mnu = *Dep::m_nu;

      m31 = pow(mnu(2,2).real(),2) - pow(mnu(0,0).real(), 2);
    }

    void md32(double &m32)
    {
      using namespace Pipes::md32;
      Eigen::Matrix3cd mnu = *Dep::m_nu;

      m32 = pow(mnu(2,2).real(),2) - pow(mnu(1,1).real(), 2);
    }


    void min_mass(double &minmass)
    {
      using namespace Pipes::min_mass;
      Eigen::Matrix3cd mnu = *Dep::m_nu;

      if(*Dep::ordering == 1) // Normal ordering
        minmass = mnu(0,0).real();
      else // Inverted ordering
        minmass = mnu(2,2).real();
    }
 
    // PMNS matrix in the Casas-Ibarra paramtetrization
    void UPMNS(Eigen::Matrix3cd& U_nu)
    {
      using namespace Pipes::UPMNS;
     
      Eigen::Matrix3cd V_23, V_13, V_12, U_pd, U_nd, Maj_phase;
      double theta23 = *Param["theta23"];
      double theta12 = *Param["theta12"];
      double theta13 = *Param["theta13"];
      double delta = *Param["delta13"];
      double alpha1 = *Param["alpha1"];
      double alpha2 = *Param["alpha2"];
      std::complex<double> I(0.0, 1.0);

      V_23 << 1.0, 0.0, 0.0,
              0.0, cos(theta23), sin(theta23),
              0.0, -sin(theta23), cos(theta23);
      V_13 << cos(theta13), 0.0, sin(theta13),
              0.0, 1.0, 0.0,
              -sin(theta13), 0.0, cos(theta13);
      V_12 << cos(theta12), sin(theta12), 0.0,
              -sin(theta12), cos(theta12), 0.0,
              0.0, 0.0, 1.0;
      U_pd << exp(-I*delta/2.0), 0.0, 0.0,
              0.0, 1.0, 0.0,
              0.0, 0.0, exp(I*delta/2.0);
      U_nd << exp(I*delta/2.0), 0.0, 0.0,
              0.0, 1.0, 0.0,
              0.0, 0.0, exp(-I*delta/2.0);
      Maj_phase << exp(I*alpha1/2.0), 0.0, 0.0,
                   0.0, exp(I*alpha2/2.0), 0.0,
                   0.0, 0.0, 1.0;
      U_nu = V_23 * U_pd * V_13 * U_nd * V_12 * Maj_phase;

    }

    // Helper function for the heavy neutrino masses
    double l_M(double M, const double m_Z, const double m_H)
    {
      if(!M)
       return 0.0;
      return 1.0/pow(4.0*pi, 2.0) * ( (3.0*log(pow(M/m_Z, 2.0)))/((pow(M/m_Z, 2.0)) - 1.0) + (log(pow(M/m_H, 2.0)))/((pow(M/m_H, 2.0)) - 1.0));
    }

    // Theta matrix in Seesaw I in the Casas-Ibarra parametrization
    void CI_Theta(Eigen::Matrix3cd& Theta)  // capability: SeesawI_Theta
    {
      using namespace Pipes::CI_Theta;
      SMInputs sminputs = *Dep::SMINPUTS;
      Eigen::Matrix3cd mnu = *Dep::m_nu;

      std::complex<double> I(0.0, 1.0);

      Eigen::Matrix3d M_I;  // M_I not complex; circumvents type mismatch in l(M)
      Eigen::Matrix3cd M_twid, R_23, R_13, R_12, R;

      double mZ = sminputs.mZ;
      double mH = *Param["mH"];
      double vev = 1. / sqrt(sqrt(2.)*sminputs.GF);

      double x23 = *Param["ReOm23"];
      double y23 = *Param["ImOm23"];
      double x13 = *Param["ReOm13"];
      double y13 = *Param["ImOm13"];
      double x12 = *Param["ReOm12"];
      double y12 = *Param["ImOm12"];

      M_I << *Param["M_1"], 0.0, 0.0,
             0.0, *Param["M_2"], 0.0,
             0.0, 0.0, *Param["M_3"];

      // Invalidate point if any M_I is zero
      if(!*Param["M_1"] or !*Param["M_2"] or !*Param["M_3"])
      {
        std::ostringstream msg;
        msg << "Casas-Ibarra parametrization is undefined for M_I = 0";
        logger() << msg.str() << EOM;
        invalid_point().raise(msg.str());
      }

      M_twid(0,0) = sqrt(M_I(0,0)  * (1.0 - (pow(M_I(0,0),2.0)*l_M(M_I(0,0),mZ,mH)/pow(vev,2.0))));
      M_twid(0,1) = 0.0;
      M_twid(0,2) = 0.0;
      M_twid(1,0) = 0.0;
      M_twid(1,1) = sqrt(M_I(1,1)  * (1.0 - (pow(M_I(1,1),2.0)*l_M(M_I(1,1),mZ,mH)/pow(vev,2.0))));
      M_twid(1,2) = 0.0;
      M_twid(2,0) = 0.0;
      M_twid(2,1) = 0.0;
      M_twid(2,2) = sqrt(M_I(2,2)  * (1.0 - (pow(M_I(2,2),2.0)*l_M(M_I(2,2),mZ,mH)/pow(vev,2.0))));

      R_23(0,0) = 1.0;
      R_23(0,1) = 0.0;
      R_23(0,2) = 0.0;
      R_23(1,0) = 0.0;
      R_23(1,1) = cos(x23)*cosh(y23) - I*sin(x23)*sinh(y23);
      R_23(1,2) = sin(x23)*cosh(y23) + I*cos(x23)*sinh(y23);
      R_23(2,0) = 0.0;
      R_23(2,1) = -sin(x23)*cosh(y23) - I*cos(x23)*sinh(y23);
      R_23(2,2) = cos(x23)*cosh(y23) - I*sin(x23)*sinh(y23);
      R_13(0,0) = cos(x13)*cosh(y13) - I*sin(x13)*sinh(y13);;
      R_13(0,1) = 0.0;
      R_13(0,2) = sin(x13)*cosh(y13) + I*cos(x13)*sinh(y13);
      R_13(1,0) = 0.0;
      R_13(1,1) = 1.0;
      R_13(1,2) = 0.0;
      R_13(2,0) = -sin(x13)*cosh(y13) - I*cos(x13)*sinh(y13);
      R_13(2,1) = 0.0;
      R_13(2,2) = cos(x13)*cosh(y13) - I*sin(x13)*sinh(y13);
      R_12(0,0) = cos(x12)*cosh(y12) - I*sin(x12)*sinh(y12);
      R_12(0,1) = sin(x12)*cosh(y12) + I*cos(x12)*sinh(y12);
      R_12(0,2) = 0.0;
      R_12(1,0) = -sin(x12)*cosh(y12) - I*cos(x12)*sinh(y12);
      R_12(1,1) = cos(x12)*cosh(y12) - I*sin(x12)*sinh(y12);
      R_12(1,2) = 0.0;
      R_12(2,0) = 0.0;
      R_12(2,1) = 0.0;
      R_12(2,2) = 1.0;

      //std::string order = runOptions->getValueOrDef<std::string>("321", "R_order");
      float Rorder = *Param["Rorder"];

      //if (order == "321")
      if ((0. <= Rorder) && (Rorder < 1.))
        R = R_23 * R_13 * R_12;
      //if (order == "213")
      if ((1. <= Rorder) && (Rorder < 2.))
        R = R_13 * R_12 * R_23;
      //if (order == "132")
      if ((2. <= Rorder) && (Rorder < 3.))
        R = R_12 * R_23 * R_13;
      //if (order == "231")
      if ((3. <= Rorder) && (Rorder < 4.))
        R = R_13 * R_23 * R_12;
      //if (order == "312")
      if ((4. <= Rorder) && (Rorder < 5.))
        R = R_23 * R_12 * R_13;
      //if (order == "123")
      if ((5. <= Rorder) && (Rorder <= 6.))
        R = R_12 * R_13 * R_23;

      if(mnu != Eigen::Matrix3cd::Zero() and M_twid != Eigen::Matrix3cd::Zero())
        Theta = I * *Dep::UPMNS * mnu.sqrt() * R * M_twid.inverse();

      // This parametrisation is not valid when |Theta|^2_ij > 1, so invalidate those points
      Eigen::Matrix3d ThetaNorm = (Theta.adjoint() * Theta).real();
      Eigen::Matrix3d ThetaNorm2 = (Theta * Theta.adjoint()).real();
      for(int i=0; i<3; i++)
        for(int j=0; j<3; j++)
        {
          if(ThetaNorm(i,j) > 1 or ThetaNorm2(i,j) > 1 or abs(Theta(i,j)) > 1)
          {
            std::ostringstream msg;
            msg << "Casas-Ibarra parametrization breaks down for parameter point";
            logger() << msg.str() << EOM;
            invalid_point().raise(msg.str());
          }
        }

      if(ThetaNorm(0,0)  + ThetaNorm(1,1)  > 1 or ThetaNorm(0,0)  + ThetaNorm(2,2)  > 1 or ThetaNorm(1,1)  + ThetaNorm(2,2)  > 1 or
         ThetaNorm2(0,0) + ThetaNorm2(1,1) > 1 or ThetaNorm2(0,0) + ThetaNorm2(2,2) > 1 or ThetaNorm2(1,1) + ThetaNorm2(2,2) > 1)
      {
        std::ostringstream msg;
        msg << "Casas-Ibarra parametrization breaks down for parameter point";
        logger() << msg.str() << EOM;
        invalid_point().raise(msg.str());
      }
    }


    void Vnu(Eigen::Matrix3cd &V)
    {
      using namespace Pipes::Vnu;
      Eigen::Matrix3cd Theta = *Dep::SeesawI_Theta;
      Eigen::Matrix3cd U = *Dep::UPMNS;

      V = U - 0.5*Theta*Theta.adjoint()*U;

    }

    void Unitarity_UPMNS(bool &unitarity)
    {
      using namespace Pipes::Unitarity_UPMNS;

      Eigen::Matrix3cd Id;
      Id << 1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0;
      Eigen::Matrix3d Epsilon;
      Epsilon << 1e-08, 1e-08, 1e-08,
                 1e-08, 1e-08, 1e-08,
                 1e-08, 1e-08, 1e-08;

      Eigen::Matrix3cd Norm = Dep::UPMNS->adjoint() * *Dep::UPMNS;
      unitarity = true;
      for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
          if(std::abs(Norm(i,j) - Id(i,j)) > Epsilon(i,j))
            unitarity = false;

      if(!unitarity)
        return ;

      for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
          if(std::real((*Dep::m_nu)(i,j)*pow((*Dep::UPMNS)(i,j),2)) > Epsilon(i,j))
            unitarity = false;

   
    }

    void Unitarity_SeesawI(bool &unitarity)
    {
      using namespace Pipes::Unitarity_SeesawI;

      Eigen::Matrix3cd Id;
      Id << 1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0;
      Eigen::Matrix3d Epsilon;
      Epsilon << 1e-08, 1e-08, 1e-08,
                 1e-08, 1e-08, 1e-08,
                 1e-08, 1e-08, 1e-08;

      Eigen::Matrix3cd Vnu = *Dep::SeesawI_Vnu;
      Eigen::Matrix3cd Theta = *Dep::SeesawI_Theta;
      Eigen::Matrix3cd m_nu = *Dep::m_nu;

      Eigen::Matrix3cd Norm = Vnu.adjoint() * Vnu + Theta.adjoint() * Theta;
      unitarity = true;
      for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
          if(std::norm(Norm(i,j) - Id(i,j)) > Epsilon(i,j))
            unitarity = false;

      if(!unitarity)
        return ;

      unitarity = true;
      Eigen::Matrix3d MN;
      MN << *Param["M_1"], 0.0, 0.0,
            0.0, *Param["M_2"], 0.0,
            0.0, 0.0, *Param["M_3"];
      Eigen::Matrix3cd Unit;
      for(int i = 0; i < 3; i++)
      {
        Unit(i,i) = 0;
        for(int j = 0; j < 3; j++)
          Unit(i,i) += m_nu(j,j)*pow(Vnu(i,j),2) + MN(j,j) * pow(Theta(i,j),2);
        if(std::real(Unit(i,i)) > Epsilon(i,i))
          unitarity = false;
      }
    }


// Function to fill a spline object from a file
    tk::spline filling_spline(std::string file)
    {
      tk::spline s;
      std::vector<double> M_temp, U_temp;

      std::vector<std::pair<double,double> > array;
      std::ifstream f(GAMBIT_DIR "/"+file);
      while(f.good())
      {
        std::string line;
        getline(f, line);
        if (!f.good())
          break;
        std::stringstream iss(line);
        std::pair<double,double> point;
        iss >> point.first;
        iss.ignore();
        iss >> point.second;
        array.push_back(point);
      }

      for (unsigned int i=0; i<array.size(); i++)
      {
        M_temp.push_back(array[i].first);
        U_temp.push_back(array[i].second);
      }
      s.set_points(M_temp, U_temp);

      return s;
    }
    
    
    // Active neutrino likelihoods
    void theta12(double &result)
    {
      using namespace Pipes::theta12;
      result = *Param["theta12"];
    }

    
    void theta12_lnL(double &result)
    {
      using namespace Pipes::theta12_lnL;

      static bool read_table_n = true;
      static bool read_table_i = true;
      static tk::spline spline_t12_n;
      static tk::spline spline_t12_i;
      static double low_lim = 0.170;  
      static double upp_lim = 0.830;  
      

      if (read_table_n and (*Dep::ordering == 1)) // Normal odering
      {
        spline_t12_n = filling_spline("NeutrinoBit/data/T12n.csv");
        read_table_n = false;
      }
      else if (read_table_i and (*Dep::ordering == 0)) // inverted odering
      {
        spline_t12_i = filling_spline("NeutrinoBit/data/T12i.csv");
        read_table_i = false;
      }
      
      
      
      if ((pow(sin(*Dep::theta12),2) < low_lim) or (pow(sin(*Dep::theta12),2) > upp_lim))
      {
        std::ostringstream msg;
        msg << "theta12 outside NuFit range; point is invalidated by active neutrino constraint.";
        logger() << msg.str() << EOM;
        invalid_point().raise(msg.str());
        return;
      }
      else
      {
         if   (*Dep::ordering == 1)
         {    
           result = -0.5*spline_t12_n(pow(sin(*Dep::theta12),2));
         }
         else if (*Dep::ordering == 0)
         {
           result = -0.5*spline_t12_i(pow(sin(*Dep::theta12),2));
         }
      }
    }
    
    

    void theta23(double &result)
    {
      using namespace Pipes::theta23;
      result = *Param["theta23"];
    }


    void theta23_lnL(double &result)
    {
      using namespace Pipes::theta23_lnL;

      static bool read_table_n = true;
      static bool read_table_i = true;
      static tk::spline spline_t23_n;
      static tk::spline spline_t23_i;
      static double low_lim = 0.250;  
      static double upp_lim = 0.750;  
      
      if (read_table_n and *Dep::ordering == 1) // Normal odering
      {
        spline_t23_n = filling_spline("NeutrinoBit/data/T23n.csv");
        read_table_n = false;
      }
      else if (read_table_i and *Dep::ordering == 0) // inverted odering
      {
        spline_t23_i = filling_spline("NeutrinoBit/data/T23i.csv");
        read_table_i = false;
      }
      
      
      if  ((pow(sin(*Dep::theta23),2) < low_lim)  or (pow(sin(*Dep::theta23),2) > upp_lim)) 
      {
        std::ostringstream msg;
        msg << "theta23 outside NuFit range; point is invalidated by active neutrino constraint.";
        logger() << msg.str() << EOM;
        invalid_point().raise(msg.str());
        return;
      }     
      else
      {
        if (*Dep::ordering == 1)
        {    
          result = -0.5*spline_t23_n(pow(sin(*Dep::theta23),2));
        }
        else if (*Dep::ordering == 0)
        {
          result = -0.5*spline_t23_i(pow(sin(*Dep::theta23),2));
        }
      }
    }
    
    
    void theta13(double &result)
    {
      using namespace Pipes::theta13;
      result = *Param["theta13"];
    }

    
    void theta13_lnL(double &result)
    {
      using namespace Pipes::theta13_lnL;

      static bool read_table_n = true;
      static bool read_table_i = true;
      static tk::spline spline_t13_n;
      static tk::spline spline_t13_i;
      static double low_lim = 0.00;  
      static double upp_lim = 0.07;  
      
      if (read_table_n and *Dep::ordering == 1) // Normal odering
      {
        spline_t13_n = filling_spline("NeutrinoBit/data/T13n.csv");
        read_table_n = false;
      }
      else if (read_table_i and *Dep::ordering == 0) // inverted odering
      {
        spline_t13_i = filling_spline("NeutrinoBit/data/T13i.csv");
        read_table_i = false;
      }      
      if  ((pow(sin(*Dep::theta13),2) < low_lim) or (pow(sin(*Dep::theta13),2) > upp_lim))
      {
          std::ostringstream msg;
          msg << "theta13 outside NuFit range; point is invalidated by active neutrino constraint.";
          logger() << msg.str() << EOM;
          invalid_point().raise(msg.str());
          return;
      }
      else
      {
        if (*Dep::ordering == 1)
        {    
          result = -0.5*spline_t13_n(pow(sin(*Dep::theta13),2));
        }
        else if (*Dep::ordering == 0)
        {
          result = -0.5*spline_t13_i(pow(sin(*Dep::theta13),2));
        }
      }
    }    
    

    void deltaCP(double &result)
    {
      using namespace Pipes::deltaCP;
      result = *Param["delta13"];
    }

    
    void deltaCP_lnL(double &result)
    {
      using namespace Pipes::deltaCP_lnL;

      static bool read_table_n = true;
      static bool read_table_i = true;
      static tk::spline spline_CP_i;
      static tk::spline spline_CP_n;
      static double low_lim = -180;  
      static double upp_lim = 360;  
      
      if (read_table_n and *Dep::ordering == 1) // Normal odering
      {
        spline_CP_n = filling_spline("NeutrinoBit/data/CPn.csv");
        read_table_n = false;
      }
      else if (read_table_i and *Dep::ordering == 0) // inverted odering
      {
        spline_CP_i = filling_spline("NeutrinoBit/data/CPi.csv");
        read_table_i = false;
      }

      
      if  (((*Dep::deltaCP*360.0)/(2.0*M_PI) < low_lim) or ((*Dep::deltaCP*360.0)/(2.0*M_PI) > upp_lim))
      {
        std::ostringstream msg;
        msg << "deltaCP outside NuFit range; point is invalidated by active neutrino constraint.";
        logger() << msg.str() << EOM;
        invalid_point().raise(msg.str());
        return;
      }
      else
      {
        if (*Dep::ordering == 1)
        {    
          result = -0.5*spline_CP_n((*Dep::deltaCP*360.0)/(2.0*M_PI));
        }
        else if (*Dep::ordering == 0)
        {
          result = -0.5*spline_CP_i((*Dep::deltaCP*360.0)/(2.0*M_PI));
        }
      }
    }
    

    
    void md21_lnL(double &result)
    {
      using namespace Pipes::md21_lnL;

      static bool read_table_n = true;
      static bool read_table_i = true;
      static tk::spline spline_md21_n;
      static tk::spline spline_md21_i;
      static double low_lim = -6.0;  
      static double upp_lim = -3.0;  
      

      if (read_table_n and *Dep::ordering == 1) // Normal odering
      {
        // Removed highly disfavoured local minima to avoid confusing scans
        spline_md21_n = filling_spline("NeutrinoBit/data/DMS1n.csv");
        read_table_n = false;
      }
      else if (read_table_i and *Dep::ordering == 0) // inverted odering
      {
        // Removed highly disfavoured local minima to avoid confusing scans
        spline_md21_i = filling_spline("NeutrinoBit/data/DMS1i.csv");
        read_table_i = false;
      }

      
      if  ((log10(*Dep::md21 * pow(10,18)) < low_lim) or (log10(*Dep::md21 * pow(10,18)) > upp_lim) or (*Dep::md21 * pow(10,18)<0))
       {
          std::ostringstream msg;
          msg << "md12 outside NuFit range; point is invalidated by active neutrino constraint.";
          logger() << msg.str() << EOM;
          invalid_point().raise(msg.str());
          return;
      }   
      else
      {
        if (*Dep::ordering == 1)
        {    
          result = -0.5*spline_md21_n(log10(*Dep::md21 * pow(10,18)));
        }
        else if (*Dep::ordering == 0)
        {
          result = -0.5*spline_md21_i(log10(*Dep::md21 * pow(10,18)));
        }
      }
    }  
 
    
    void md3l_lnL(double &result)
    {
      using namespace Pipes::md3l_lnL;

      static bool read_table_n = true;
      static bool read_table_i = true;
      static tk::spline spline_md31_n;
      static tk::spline spline_md32_i;
      static double low_lim_n = 0.2;  
      static double upp_lim_n = 7.0; 
      static double low_lim_i = -7.0;  
      static double upp_lim_i = -0.2; 
          

      if (read_table_n and *Dep::ordering == 1) // Normal odering
      {
        spline_md31_n = filling_spline("NeutrinoBit/data/DMAn.csv");
        read_table_n = false;
      }  
      else if (read_table_i and *Dep::ordering == 0) // inverted odering
      {
        spline_md32_i = filling_spline("NeutrinoBit/data/DMAi.csv");
        read_table_i = false;
      }
      
      if (*Dep::ordering == 1)
      {
        if ((*Dep::md31 * pow(10,21) < low_lim_n) or (*Dep::md31 * pow(10,21) > upp_lim_n))
        {
          std::ostringstream msg;
          msg << "md31 outside NuFit range; point is invalidated by active neutrino constraint.";
          logger() << msg.str() << EOM;
          invalid_point().raise(msg.str());
          return;
        }
        else result = -0.5*spline_md31_n(*Dep::md31 * pow(10,21));
      }
      else if (*Dep::ordering == 0)
      {
        if ((*Dep::md32 * pow(10,21) < low_lim_i) or (*Dep::md32 * pow(10,21) > upp_lim_i))
        {
          std::ostringstream msg;
          msg << "md32 outside NuFit range; point is invalidated by active neutrino constraint.";
          logger() << msg.str() << EOM;
          invalid_point().raise(msg.str());
          return;
        }
        else result = -0.5*spline_md32_i(*Dep::md32 * pow(10,21));
      }     
    }

    // Sum of neutrino likelihoods
    void sum_mnu_lnL(double &result)
    {
      using namespace Pipes::sum_mnu_lnL;

      double sum_mnu = *Param["mNu1"] + *Param["mNu2"] + *Param["mNu3"];

      if(sum_mnu < 2.3E-10)
        result = 0.0;
      else
      {
        std::ostringstream msg;
        msg << "Sum of neutrino masses over the cosmological limit";
        logger() << msg.str() << EOM;
        invalid_point().raise(msg.str());
      }
    }
  }
}
