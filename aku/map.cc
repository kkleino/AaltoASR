#include <fstream>
#include <string>
#include <string.h>
#include <iostream>

#include "io.hh"
#include "str.hh"
#include "conf.hh"
#include "HmmSet.hh"
#include "FeatureGenerator.hh"
#include "Recipe.hh"

using namespace aku;
  
std::string stat_file;
std::string out_file;

int info;
bool transtat;

conf::Config config;
FeatureGenerator fea_gen;
HmmSet model;

std::vector<double> min_d_values;
std::vector<double> max_d_values;


void set_ebw_d_from_file(const std::string &d_file_name)
{
  PDFPool *pool = model.get_pool();

  std::ifstream d_file(d_file_name.c_str());
  if (!d_file.is_open())
    throw std::string("Error opening file") + d_file_name;

  min_d_values.resize(pool->size());
  max_d_values.resize(pool->size());
  
  for (int i = 0; i < pool->size(); i++)
  {
    if (!d_file.good())
      throw std::string("Failed to read D values");
    char buf[256];
    std::string temp;
    std::vector<std::string> fields;
    d_file.getline(buf, 256);
    temp.assign(buf);
    str::split(&temp, " ", false, &fields, 4);
    if (fields.size() >= 1)
    {
      double value = strtod(fields[0].c_str(), NULL);
      if (value < 0)
      {
        // Truncate invalid values
        value = 0;
        //throw std::string("Invalid value in D file");
      }
      PDF *base_pdf = model.get_pool_pdf(i);
      Gaussian *temp = dynamic_cast< Gaussian* > (base_pdf);
      if (temp != NULL)
        temp->set_ebw_fixed_d(value);
      if (fields.size() >= 3)
      {
        min_d_values[i] = strtod(fields[1].c_str(), NULL);
        max_d_values[i] = strtod(fields[2].c_str(), NULL);
      }
      else
      {
        min_d_values[i] = -1;
        max_d_values[i] = -1;
      }
    }
    else
      throw std::string("Invalid format in D file");
  }
  d_file.close();
}

void write_ebw_d_file(const std::string &d_file_name)
{
  PDFPool *pool = model.get_pool();
  
  std::ofstream d_file(d_file_name.c_str(), std::ios_base::out);
  for (int i = 0; i < pool->size(); i++)
  {
    PDF *base_pdf = model.get_pool_pdf(i);
    Gaussian *temp = dynamic_cast< Gaussian* > (base_pdf);
    if (temp != NULL)
    {
      double min_value = temp->get_minimum_d();
      if ((int)min_d_values.size() > i && min_value < min_d_values[i])
        min_value = min_d_values[i];
      double max_value = temp->get_realized_d();
      if ((int)max_d_values.size() > i &&
          (max_d_values[i] <= 0 || max_value < max_d_values[i]))
        max_value = max_d_values[i];
      d_file << temp->get_realized_d() << " " << min_value <<
        " " << max_value << std::endl;
    }
  }
  d_file.close();
}



int
main(int argc, char *argv[])
{
  std::map< std::string, double > sum_statistics;
  std::string base_file_name;
  PDF::EstimationMode mode;
  
  try {
    config("usage: estimate [OPTION...]\n")
      ('h', "help", "", "", "display help")
      ('b', "base=BASENAME", "arg", "", "Previous base filename for model files")
      ('g', "gk=FILE", "arg", "", "Previous mixture base distributions")
      ('m', "mc=FILE", "arg", "", "Previous mixture coefficients for the states")
      ('p', "ph=FILE", "arg", "", "Previous HMM definitions")
      ('c', "config=FILE", "arg", "", "feature configuration (required for MLLT)")
      ('L', "list=LISTNAME", "arg must", "", "file with one statistics file per line")
      ('C', "coeffs=NAME", "arg", "", "Precomputed precision/subspace Gaussians")
      ('o', "out=BASENAME", "arg must", "", "base filename for output models")
      ('t', "transitions", "", "", "estimate also state transitions")
      ('i', "info=INT", "arg", "0", "info level")
      ('\0', "tau=FLOAT", "arg", "5.0", "weight of prior in MAP (default 5.0)")
      ('\0', "maptype=INT", "arg", "2", "Switch between MAP1 and MAP2 (default 2)")
      ('\0', "mllt=MODULE", "arg", "", "update maximum likelihood linear transform")
      ('\0', "ml", "", "", "maximum likelihood estimation")
      ('\0', "mmi", "", "", "maximum mutual information estimation")
      ('\0', "mpe", "", "", "minimum phone error estimation")
      ('\0', "minvar=FLOAT", "arg", "0.1", "minimum variance (default 0.1)")
      ('\0', "covsmooth", "arg", "0", "covariance smoothing (default 0.0)")
      ('\0', "C1=FLOAT", "arg", "2.0", "constant \"C1\" for EBW updates (default 2.0)")
      ('\0', "C2=FLOAT", "arg", "2.0", "constant \"C2\" for EBW updates (default 2.0)")
      ('\0', "ismooth=FLOAT", "arg", "0.0", "I-smoothing constant")
      ('\0', "mmi-prior-ismooth=FLOAT", "arg", "0.0", "Use MMI prior when I-smoothing MPE model")
      ('\0', "prev-prior", "", "", "Use previous model as prior in I-smoothing")
      ('\0', "limit", "arg", "0.0", "Global KLD limit for parameter change")
      ('\0', "delete=FLOAT", "arg", "0.0", "delete Gaussians with occupancies below the threshold")
      ('\0', "mremove=FLOAT", "arg", "0.0", "remove mixture components below the weight threshold")
      ('\0', "split", "", "", "Enable Gaussian splitting")
      ('\0', "minocc=FLOAT", "arg", "0.0", "Occupancy threshold for Gaussian splitting")
      ('\0', "maxmixgauss=INT", "arg", "0", "maximum number of Gaussians per mixture for splitting")
      ('\0', "numgauss=INT", "arg", "-1", "Target number of Gaussians in the final model")
      ('\0', "splitalpha=FLOAT", "arg", "1.0", "Occupancy smoothing power for splitting")
      ('\0', "no-silence-update", "", "", "Don't update silence state parameters")
      ('\0', "no-mixture-update", "", "", "Do not update mixture coefficients")
      ('\0', "silence-d=FLOAT", "arg", "0", "Set a fixed EBW D for silence Gaussians")
      ('D', "ebwd=FILE", "arg", "", "Read Gaussian specific EBW D values (and limits)")
      ('\0', "write-ebwd=FILE", "arg", "", "Write Gaussian specific D and minimum D values")
      ('\0', "no-write", "", "", "Don't write anything")
      ('s', "savesum=FILE", "arg", "", "save summary information")
      ('\0', "hcl-bfgs-cfg=FILE", "arg", "", "configuration file for HCL biconjugate gradient algorithm")
      ('\0', "hcl-line-cfg=FILE", "arg", "", "configuration file for HCL line search algorithm")
      ;
    config.default_parse(argc, argv);

    transtat = config["transitions"].specified;    
    info = config["info"].get_int();
    out_file = config["out"].get_str();

    int count = 0;
    if (config["ml"].specified) {
      count++;
      mode = PDF::ML_EST;
    }
    if (config["mmi"].specified) {
      count++;
      mode = PDF::MMI_EST;
    }
    if (config["mpe"].specified) {
      count++;
      mode = PDF::MPE_EST;
    }
    if (count != 1)
      throw std::string("Define exactly one of --ml, --mmi and --mpe!");

    if (config["split"].specified && !(config["minocc"].specified ||
                                       config["numgauss"].specified))
    {
      fprintf(stderr, "Either --minocc or --numgauss is required with --split\n");
      exit(1);
    }

    if (config["ismooth"].specified &&
        (!config["mmi"].specified && !config["mpe"].specified))
      fprintf(stderr, "Warning: --ismooth ignored without --mmi or --mpe\n");
    if (config["prev-prior"].specified)
      model.set_ismooth_prev_prior(true);
    else if (config["mmi-prior-ismooth"].specified)
    {
      if (mode == PDF::MPE_EST)
        mode = PDF::MPE_MMI_PRIOR_EST;
      else
        fprintf(stderr, "Warning: --mmi-prior-ismooth ignored without --mpe\n");
    }
    
    // Load the previous models
    if (config["base"].specified)
    {
      model.read_all(config["base"].get_str());
      base_file_name = config["base"].get_str();
    }
    else if (config["gk"].specified && config["mc"].specified &&
             config["ph"].specified)
    {
      model.read_gk(config["gk"].get_str());
      model.read_mc(config["mc"].get_str());
      model.read_ph(config["ph"].get_str());
      base_file_name = config["gk"].get_str();
    }
    else
    {
      throw std::string("Must give either --base or all --gk, --mc and --ph");
    }

    if (config["no-silence-update"].specified)
    {
      for (int i = 0; i < model.num_hmms(); i++)
      {
        Hmm &hmm = model.hmm(i);
        if (hmm.label[0] == '_' &&
            hmm.label.find('-') == std::string::npos &&
            hmm.label.find('+') == std::string::npos)
        {
          // Assume this is a silence state, disable updating
          for (int j = 0; j < hmm.num_states(); j++)
            model.set_state_update(hmm.state(j), false);
        }
      }
    }

    if (config["silence-d"].specified && config["ebwd"].specified)
      throw std::string("Only one of '--silence-d' and '--ebwd' can be specified at the same time");

    if (config["silence-d"].specified)
    {
      double fixed_d = config["silence-d"].get_float();
      for (int i = 0; i < model.num_hmms(); i++)
      {
        Hmm &hmm = model.hmm(i);
        if (hmm.label[0] == '_' &&
            hmm.label.find('-') == std::string::npos &&
            hmm.label.find('+') == std::string::npos)
        {
          // Assume this is a silence state, set a fixed EBW D
          for (int j = 0; j < hmm.num_states(); j++)
          {
            int pdf_index = model.state(hmm.state(j)).emission_pdf;
            Mixture *m = model.get_emission_pdf(pdf_index);
            for (int k = 0; k < (int)m->size(); k++)
            {
              PDF *base_pdf = model.get_pool_pdf(m->get_base_pdf_index(k));
              Gaussian *temp = dynamic_cast< Gaussian* > (base_pdf);
              if (temp != NULL)
                temp->set_ebw_fixed_d(fixed_d);
            }
          }
        }
      }
    }
    if (config["ebwd"].specified)
      set_ebw_d_from_file(config["ebwd"].get_str());

    if (config["mllt"].specified)
    {
      if (!config["ml"].specified)
        throw std::string("--mllt is only supported with --ml");
    }
    
    if (config["config"].specified) {
      fea_gen.load_configuration(io::Stream(config["config"].get_str()));
    }
    else if (config["mllt"].specified) {
      throw std::string("Must specify configuration file with MLLT");      
    }
    
    // Open the list of statistics files
    std::ifstream filelist(config["list"].get_str().c_str());
    if (!filelist)
    {
      fprintf(stderr, "Could not open %s\n", config["list"].get_str().c_str());
      exit(1);
    }

    // Accumulate statistics
    while (filelist >> stat_file && stat_file != " ") {
      model.accumulate_gk_from_dump(stat_file+".gks");
      model.accumulate_mc_from_dump(stat_file+".mcs");
      if (transtat)
        model.accumulate_ph_from_dump(stat_file+".phs");
      std::string lls_file_name = stat_file+".lls";
      std::ifstream lls_file(lls_file_name.c_str());
      while (lls_file.good())
      {
        char buf[256];
        std::string temp;
        std::vector<std::string> fields;
        lls_file.getline(buf, 256);
        temp.assign(buf);
        str::split(&temp, ":", false, &fields, 2);
        if (fields.size() == 2)
        {
          double value = strtod(fields[1].c_str(), NULL);
          if (sum_statistics.find(fields[0]) == sum_statistics.end())
            sum_statistics[fields[0]] = value;
          else
            sum_statistics[fields[0]] = sum_statistics[fields[0]] + value;
        }
      }
      lls_file.close();
    }

    // Estimate parameters
    model.set_gaussian_parameters(config["minvar"].get_double(),
                                  config["covsmooth"].get_double(),
                                  config["C1"].get_double(),
                                  config["C2"].get_double(),
                                  config["ismooth"].get_double(),
                                  config["mmi-prior-ismooth"].get_double(),
                                  config["limit"].get_double());

    // Linesearch for subspace models
#ifdef USE_SUBSPACE_COV
    HCL_LineSearch_MT_d ls;
    if (config["hcl-line-cfg"].specified)
      ls.Parameters().Merge(config["hcl-line-cfg"].get_str().c_str());

    // lmBFGS for subspace models
    HCL_UMin_lbfgs_d bfgs(&ls);
    if (config["hcl-bfgs-cfg"].specified)
      bfgs.Parameters().Merge(config["hcl-bfgs-cfg"].get_str().c_str());

    model.set_hcl_optimization(&ls, &bfgs, config["hcl-line-cfg"].get_str(), config["hcl-bfgs-cfg"].get_str());

    // Load precomputed coefficients
    if (config["coeffs"].specified) {
      std::ifstream coeffs_files(config["coeffs"].get_str().c_str());
      std::string coeff_file_name;
      while (coeffs_files >> coeff_file_name) {
        std::ifstream coeff_file(coeff_file_name.c_str());
        int g;
        while (coeff_file >> g) {
          PrecisionConstrainedGaussian *pc = dynamic_cast< PrecisionConstrainedGaussian* > (model.get_pool_pdf(g));
          if (pc != NULL) {
            pc->read(coeff_file);
          }
          
          SubspaceConstrainedGaussian *sc = dynamic_cast< SubspaceConstrainedGaussian* > (model.get_pool_pdf(g));
          if (sc != NULL) {
            sc->read(coeff_file);
          }
        }
      }

      // Re-estimate only mixture parameters in this case
      model.estimate_parameters(mode, false, true);
    }
#else
    if (config["hcl-line-cfg"].specified ||
        config["hcl-bfgs-cfg"].specified ||
        config["coeffs"].specified)
      throw std::runtime_error("not compiled with USE_SUBSPACE_COV");
#endif

    // Normal training, FIXME: MLLT + precomputed coefficients?
    else {

      if (transtat)
        model.estimate_transition_parameters();
      if (config["mllt"].specified)
        model.estimate_mllt(fea_gen, config["mllt"].get_str());
      else
      {  
		// MAP
	
		// Set debugprint
		fprintf(stderr, "Estimate MAP1 parameters\n");

		// Save gaussian means
		PDFPool *pool = model.get_pool();
		Vector old_means[pool->size()]; // Save old means
		Vector old_covariances[pool->size()]; // Save old variances
		for (int i = 0; i < pool->size(); i++)
		{
		  PDF *base_pdf = model.get_pool_pdf(i);
		  Gaussian *temp = dynamic_cast< Gaussian* > (base_pdf);
		  if (temp != NULL) 
		  {
			Vector mean; temp->get_mean(mean);
			old_means[i] = mean;
			Vector covariance; temp->get_covariance(covariance);
			old_covariances[i] = covariance;
		  }
		}

		// Update new  parameters to model
		model.estimate_parameters(mode, true, !config["no-mixture-update"].specified);


		// MAP update
		if (config["maptype"].get_int() == 1)
        {
			fprintf(stderr, "Estimate MAP1 parameters\n");
	
			double tau = config["tau"].get_double();
			for (int i = 0; i < model.num_hmms(); i++)
			{
			  Hmm &hmm = model.hmm(i);
			  for (int j = 0; j < hmm.num_states(); j++)
			  {
				int pdf_index = model.state(hmm.state(j)).emission_pdf;
				Mixture *m = model.get_emission_pdf(pdf_index);
			
						
				for (int k = 0; k < (int)m->size(); k++)
				{
				  // Weights
				  double w = m->get_mixture_coefficient(k);
				  //w = 2.0;
				  double w_map = w + tau; 
				   
				  // Gaussian distributions
				  PDF *base_pdf = model.get_pool_pdf(m->get_base_pdf_index(k));
				  Gaussian *temp = dynamic_cast< Gaussian* > (base_pdf);

				  // Only if enugh observations
				  if (temp != NULL && w > tau/10) 
				  {

			 	    // Gaussian means
				    Vector mean; temp->get_mean(mean);

					// mean = (tau*mean_prior + w*mean)/(w+tau)
					Blas_Scale(w, mean); // w*mean
					// tau*mean_prior + w*mean
					Blas_Add_Mult(mean, tau, old_means[m->get_base_pdf_index(k)]); 
					Blas_Scale((1/w_map), mean); // *1/(w+tau)
					temp->set_mean(mean);

					// Covariances stay same
					temp->set_covariance(old_covariances[m->get_base_pdf_index(k)]); 
		            // No mean update
		            //temp->set_mean(old_means[m->get_base_pdf_index(k)]);

				  }
				  else if (temp != NULL)
				  {
				    // else {mean = mean_prior}
					temp->set_mean(old_means[m->get_base_pdf_index(k)]);
					// Covariances stay same
					temp->set_covariance(old_covariances[m->get_base_pdf_index(k)]); 
				  }	 

				}
			  }
			} 
        } else if (config["maptype"].get_int() == 2)
		{
			fprintf(stderr, "Estimate MAP2.5 parameters\n");

			// MAP update using occupancy
			double tau = config["tau"].get_double();
		    double occupancy; // Occupancy
			for (int i = 0; i < pool->size(); i++)
			{
		      occupancy = pool->get_gaussian_occupancy(i);
		      //occupancy = 5;
			  PDF *base_pdf = model.get_pool_pdf(i);
			  Gaussian *temp = dynamic_cast< Gaussian* > (base_pdf);
			  if (temp != NULL && occupancy >= 0) 
			  {
		        // Updating mean
				Vector mean; temp->get_mean(mean);
		        // occupancy/(occupancy+tau)*mean_new + tau/(occupancy+tau)*mean_prior
		        double fl = occupancy/(occupancy+tau);
		        double fp = tau/(occupancy+tau);
				//debugfile << "fl:" << fl << " fp" << fp << "\n";
				//debugfile << "likelihood mean: " << mean << "\n";
				//debugfile << "prior mean:" << old_means[i] << "\n";
		        Blas_Scale(fl, mean);
		        Blas_Add_Mult(mean, fp, old_means[i]);
		        temp->set_mean(mean);

/*
				// Covariance update
			    Matrix covariance;
			    temp->get_covariance(covariance);
				// tau*cov_prior
				Blas_Scale(tau, old_covariances[i]);
				// w*(cov_mle + m_mle^2 - 2*m_mle*m + m^2)
				
	*/			
		        
				
		        // Debug
		        //debugfile << "fl:" << fl << "fp" << fp << "\n";
				//temp->set_mean(old_means[i]);
			  } 
		      else if (temp != NULL)
			  {
			  // else {mean = mean_prior}
		        temp->set_mean(old_means[i]);
		        //temp->set_covariance(old_covariances[i]); 
		      }
			  temp->set_covariance(old_covariances[i]); 
			}	

		}

      }
	
    //fprintf(stderr, "No deleting or splitting Gaussians\n");
  
      // Delete Gaussians
      if (config["delete"].specified)
        model.delete_gaussians(config["delete"].get_double());
      
      // Remove mixture components
      if (config["mremove"].specified)
        model.remove_mixture_components(config["mremove"].get_double());
      
      /*
      // Split Gaussians if desired
      /if (config["split"].specified)
        model.split_gaussians(config["minocc"].get_double(),
                              config["maxmixgauss"].get_int(),
                              config["numgauss"].get_int(), 
                              config["splitalpha"].get_double());
      */  
     
      model.stop_accumulating();
    }

    if (config["write-ebwd"].specified)
      write_ebw_d_file(config["write-ebwd"].get_str());
    
    // Write final models
    if (!config["no-write"].specified)
    {
      model.write_all(out_file);
      if (config["config"].specified) {
        fea_gen.write_configuration(io::Stream(out_file + ".cfg","w"));
      }
    }

    if (config["savesum"].specified && !config["no-write"].specified) {
      std::string summary_file_name = config["savesum"].get_str();
      std::ofstream summary_file(summary_file_name.c_str(),
                                 std::ios_base::app);
      if (!summary_file)
        fprintf(stderr, "Could not open summary file: %s\n",
                summary_file_name.c_str());
      else
      {
        summary_file.precision(12); 
        summary_file << base_file_name << std::endl;
        for (std::map<std::string, double>::const_iterator it =
               sum_statistics.begin(); it != sum_statistics.end(); it++)
        {
          summary_file << "  " << (*it).first << ": " << (*it).second <<
            std::endl;
        }
      }
      summary_file.close();
    }
  }
  
  catch (std::exception &e) {
    fprintf(stderr, "exception: %s\n", e.what());
    abort();
  }
  
  catch (std::string &str) {
    fprintf(stderr, "exception: %s\n", str.c_str());
    abort();
  }
}
