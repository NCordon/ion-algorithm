#include "metaheuristic.h"

void updateFitness(vector<Solution> &population, int &eval){
    for (unsigned int i=0; i < population.size(); i++){
        population[i].updateFitness();
        eval++;
    }
}


bool ionOrder(Solution i, Solution j){
    return i.getFitness() < j.getFitness();
}


void updateLocations(vector<Solution> &ions, Solution ref){
    double force;

    for(unsigned int i = 0; i < ions.size(); i++){
        for(unsigned int j = 0; j < ions[i].size(); j++){
            force = (1.0/(1 + exp(-0.1/abs(ions[i][j] - ref[j]))));
            ions[i][j] = ions[i][j] + force*(ref[j] - ions[i][j]);
        }
    }
}

void normalize(Solution &ion){
    for(unsigned int j=0; j < ion.size(); j++){
        if(ion[j] < lbound)
            ion[j] = lbound;
        else if(ion[j] > ubound)
            ion[j] = ubound;
    }
}

void normalize(vector<Solution> &ions){

    for (unsigned int i=0; i < ions.size(); i++){
        normalize(ions[i]);
    }
}



void initialize(vector<Solution> &ions){
    vector<double> position(dimension);

    for (unsigned int i=0; i < ions.size(); i++){
        for (unsigned int j=0; j < dimension; j++)
            position[j] = myrandom.randreal(lbound, ubound);

        ions[i] = Solution(position);
    }
}



void redistribute(vector<Solution> &ions, Solution ref, Solution ref_old){

    double phi;

    for (unsigned int i = 0; i < ions.size(); i++){
        // -1 < phi < 1
        phi = myrandom.randreal(-1,1);

        if (myrandom.randreal(0,1) > 0.5)
            for (unsigned int j = 0; j < ions[i].size(); j++){
                ions[i][j] = ions[i][j] + phi * ref_old[j];
                //if(myrandom.randreal(0,1) < prob_mutation)
                //    ions[i][j] = myrandom.randreal(lbound, ubound);
            }
        else
            for (unsigned int j = 0; j < ions[i].size(); j++){
                ions[i][j] = ions[i][j] + phi * ref[j];
                //if(myrandom.randreal(0,1) < prob_mutation)
                //    ions[i][j] = myrandom.randreal(lbound, ubound);
            }
        if(myrandom.randreal(0,1) < prob_restart)
            for (unsigned int j = 0; j < ions[i].size(); j++)
                ions[i][j] = myrandom.randreal(lbound, ubound);
    }
}

void updateBestSolution(Solution &best_solution, Solution ref){
    if (ref.getFitness() < best_solution.getFitness()){
        best_solution = Solution(ref);
    }
}


vector<double> ion_algorithm(){
    vector<Solution> cations(population_size/2);
    vector<Solution> anions(population_size/2);
    Solution best_solution(vector<double>(dimension), numeric_limits<double>::infinity());
    int eval = 0;
    bool liquid_phase = false;
    bool best_updated = false;

    // Inicialización
    initialize(cations);
    initialize(anions);
    updateFitness(cations, eval);
    updateFitness(anions, eval);
    auto best_cation = *min_element(cations.begin(), cations.end(), ionOrder);
    auto best_anion = *min_element(anions.begin(), anions.end(), ionOrder);
    auto worst_cation = *max_element(cations.begin(), cations.end(), ionOrder);
    auto worst_anion = *max_element(anions.begin(), anions.end(), ionOrder);
    auto best_cation_old = best_cation;
    auto best_anion_old = best_anion;


    while (eval < max_eval){
        // Fase líquida
        if (liquid_phase){
            updateLocations(anions, best_cation);
            updateLocations(cations, best_anion);
            // Actualización de los mejores valores
            updateFitness(cations, eval);
            updateFitness(anions, eval);

            best_cation_old = best_cation;
            best_anion_old = best_anion;
            best_cation = *min_element(cations.begin(), cations.end(), ionOrder);
            best_anion = *min_element(anions.begin(), anions.end(), ionOrder);
            worst_cation = *max_element(cations.begin(), cations.end(), ionOrder);
            worst_anion = *max_element(anions.begin(), anions.end(), ionOrder);


            if (best_cation.getFitness() >= worst_cation.getFitness() &&
                best_anion.getFitness() >= worst_anion.getFitness()){
                // Salimos de la fase liquida, entramos en la sólida
                liquid_phase = false;
            }

            updateBestSolution(best_solution, best_cation);
            updateBestSolution(best_solution, best_anion);
            //applyRealeaLS(best_solution, eval, dimension*10, "sw");
            //applyRealeaLS(best_solution, eval, dimension*20, "cmaes");

            eval++;
        }
        // Fase sólida
        else{
            redistribute(cations, best_anion, best_anion_old);
            redistribute(anions, best_cation, best_cation_old);
            normalize(cations);
            normalize(anions);
            // Entramos en la siguiente iteración en fase líquida
            liquid_phase = true;
        }
    }

    return ((vector<double>) best_solution);
}



double computeNorm(vector<double> v){
    double suma=0;

    for(int i=0; i<v.size(); i++){
        suma += v[i]*v[i];
    }

    return sqrt(suma);
}

void applyRealeaLS(Solution &solution, int &eval, int evals_ls, string type_ls){
    //tChromosomeReal sol(dim);
    //cerr << "Eval antes de búsqueda local" << eval << endl;
    tChromosomeReal sol(solution);
    ProblemCEC2014 cec2014(dimension);
    ProblemPtr problem = cec2014.get(func_num);
    ILocalSearch *ls;
    ILSParameters *ls_options;

    if (type_ls == "sw") {
        // Get the Solis Wets problem
        SolisWets *sw = new SolisWets();
        // Set the delta values
        sw->setDelta(0.2);
        ls = sw;
    }
    else if (type_ls == "simplex") {
        ls = new SimplexDim();
    }
    else if (type_ls == "cmaes") {
        CMAESHansen *cmaes = new CMAESHansen("cmaesinit.par");
        cmaes->searchRange(0.1);
        ls = cmaes;
    }

    ls -> setProblem(problem.get());
    ls -> setRandom(&myrandom);
    ls_options = ls -> getInitOptions(sol);

    // Apply the local search
    tFitness fitness = problem->eval(sol);
//    tFitness before(fitness);

    unsigned evals = ls->apply(ls_options, sol, fitness, evals_ls);

/*    if (fitness < before){
        cerr << "La búsqueda local está haciendo algo" << endl;
        cerr << "Before: " << before << endl;
        cerr << "After: " << fitness << endl;
    }
*/
    eval += evals;
    solution = Solution(sol, fitness);
    //cerr << "Eval después de búsqueda local" << eval << endl;
}

void applyLocalSearch(Solution &solution, int &eval){
    int i=0;
    int tope=solution.size();
    //double epsilon = local_epsilon;
    double norma;
    vector<double> direccion(solution.size());
    Solution current(solution);
    bool mejora = false;

    while(i < tope){
        //if (!mejora){
            for (int j=0; j<direccion.size(); j++){
                direccion[j] = myrandom.randreal(-1,1);
            }


            norma = computeNorm(direccion);
        //}

        for (int j=0; j<direccion.size(); j++){
            current[j] += myrandom.randreal(0, epsilon)*(direccion[j] / norma);
        }

        normalize(current);

        current.updateFitness();

        if (current.getFitness() <= solution.getFitness()){
            mejora = true;
            solution = Solution(current);
        }
        else{
            mejora = false;
            current = Solution(solution);
        }
        eval++;
        i++;
    }

}
