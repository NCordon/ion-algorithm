#ifndef __METAHEURISTIC_H__
#define __METAHEURISTIC_H__


#include <algorithm>
#include "solution.h"
using namespace std;


void updateFitness(vector<Solution> &population, int &eval);

bool ionOrder(Solution i, Solution j);

void updateLocations(vector<Solution> &current, Solution reference);

void normalize(vector<Solution> &ions);

void normalize(Solution &ion);

void initialize(vector<Solution> &ions);

void redistribute(vector<Solution> &ions, Solution ref);

vector<double> ion_algorithm();

void updateBestSolution(Solution &best_solution, Solution ref);

void applyLocalSearch(Solution &solution, int &eval);

double computeNorm(vector<double> v);

#endif
