/*
 * Copyright (C) 1998-2016 ALPS Collaboration
 * 
 *     This program is free software; you can redistribute it and/or modify it
 *     under the terms of the GNU General Public License as published by the Free
 *     Software Foundation; either version 2 of the License, or (at your option)
 *     any later version.
 * 
 *     This program is distributed in the hope that it will be useful, but WITHOUT
 *     ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *     FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
 *     more details.
 * 
 *     You should have received a copy of the GNU General Public License along
 *     with this program; if not, write to the Free Software Foundation, Inc., 59
 *     Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * For use in publications, see ACKNOWLEDGE.TXT
 */

#include "maxent.hpp"
#include "default_model.hpp"
#include <alps/utilities/fs/remove_extensions.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/exception/diagnostic_information.hpp> 
#include <alps/hdf5/vector.hpp>


int main(int argc,const char** argv)
{
  alps::params parms(argc,argv); 
  MaxEntSimulation::define_parameters(parms);
  //other help messages
  parms.define("help.models","show help for default model");
  parms.define("help.grids","show help for grids");

  if (parms.help_requested(std::cout)) {
    return 0;
  }
  //currently ALPSCore doesn't have a good way 
  //to show multilple error messages
  //This is a temp hack, so we can have 
  //inline grid and default model
  //descriptions
  
  bool exitEarly = false;

  if(parms["help.models"]){
    DefaultModel::print_help();
    exitEarly = true;
  }
  if(parms["help.grids"]){
    if(exitEarly) //display both messages
      std::cout << std::endl;
    grid::print_help();
    exitEarly = true;
  }

  if(exitEarly)
    return 0;

  if(!parms.exists("BETA")){
    std::cout<<"Please supply BETA"<<std::endl;
    exitEarly = true;
  }
  if(!parms.exists("NDAT")){
    std::cout<<"Please supply NDAT"<<std::endl;
    exitEarly = true;
  }
  if(parms["DATA"].as<std::string>().empty() && !parms.exists("X_0")
      && parms["DATA_IN_HDF5"]!=true){
    std::cout<<"Please supply input data"<<std::endl;
    exitEarly = true;
  }

  std::string basename;
  if(parms.defaulted("BASENAME")){
    basename = alps::fs::remove_extensions(origin_name(parms)) + ".out";
    parms["BASENAME"] = basename;
  }
  else
    basename=parms["BASENAME"].as<std::string>();
  
  try{
        if(exitEarly){
          throw std::runtime_error("Critical parameters not defined");
        }

        //allow for multiple default model runs
        //set MODEL_RUNS = #runs
        //then place:
        //RUN_0 = "Model1"
        //RUN_1 = "Model2"
        //..etc
        if(parms.exists("MODEL_RUNS")){
            int nruns=parms["MODEL_RUNS"];
            std::cout<<"Performing " << nruns <<" runs" <<std::endl;
	          //ALPSCore requires all params are defined
 	          for(int i=0;i<nruns;i++)
		          parms.define<std::string>("RUN_" + boost::lexical_cast<std::string>(i),"Run");
            //vectors to hold spectra
            std::vector<vector_type> max_spectra(nruns);
            std::vector<vector_type> av_spectra(nruns);

            vector_type omega_grid;
            for(int i=0;i<nruns;i++){
                std::string currModel = parms["RUN_" + boost::lexical_cast<std::string>(i)];
                parms["DEFAULT_MODEL"]= currModel;
                
                //run a simulation with the new default model.
                //Change the basename to match
                parms["BASENAME"] = basename+'.'+currModel;
                MaxEntSimulation my_sim(parms);
                my_sim.run();
                my_sim.evaluate();

                //save spectra for later
                max_spectra[i] = my_sim.getMaxspec();
                av_spectra[i]  = my_sim.getAvspec();
                if(i==0){
                  omega_grid = my_sim.getOmegaGrid();
                }
            }
          
            //calculate variance
            const int nfreq = max_spectra[0].size();
            vector_type mean_max = vector_type::Zero(nfreq);
            vector_type stdev_max(nfreq);
            vector_type mean_av = vector_type::Zero(nfreq);
            vector_type stdev_av(nfreq);

            determineVariance(max_spectra,mean_max,stdev_max);
            determineVariance(av_spectra,mean_av,stdev_av);

            //save to file
            if(parms["TEXT_OUTPUT"]){
            ofstream_ spec_file;
            spec_file.open((basename+".varspec.dat").c_str());
            spec_file << "#omega mean_maxspec stdev_maxspec mean_avspec stdev_avspec" <<std::endl;
            for (std::size_t  i=0; i<omega_grid.size(); ++i)
              spec_file << omega_grid(i)
                << " " << mean_max(i) << " " << stdev_max(i)
                << " " << mean_av(i)  << " " << stdev_av(i) 
                << std::endl;
            }
        }
        else{
          MaxEntSimulation my_sim(parms);
          my_sim.run();
          my_sim.evaluate();
        }
  }
    catch(const std::exception &e){
        std::cerr << "Caught Exception " << boost::diagnostic_information(e);
    }
    catch(...){
        std::cerr << "Caught Exception" << boost::current_exception_diagnostic_information();
    }
}

